#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/Geometry>
#include <osg/CoordinateSystemNode>
#include <osg/MatrixTransform>
#include <osg/ProxyNode>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "3rdparty/rapidxml/rapidxml.hpp"
#include "3rdparty/picojson.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits.h>
#define WRITE_TO_OSG 0

static std::vector<std::string> split(const std::string& src, const char* seperator, bool ignoreEmpty)
{
    std::vector<std::string> slist;
    std::string sep = (seperator == NULL) ? " " : std::string(seperator);
    std::string::size_type start = src.find_first_not_of(sep);
    while (start != std::string::npos)
    {
        std::string::size_type end = src.find_first_of(sep, start);
        if (end != std::string::npos)
        {
            slist.push_back(std::string(src, start, end - start));
            if (ignoreEmpty) start = src.find_first_not_of(sep, end);
            else start = end + 1;
        }
        else
        {
            slist.push_back(std::string(src, start, src.size() - start));
            start = end;
        }
    }
    return slist;
}

class ReaderWriter3dtiles : public osgDB::ReaderWriter
{
public:
    ReaderWriter3dtiles() : _maxScreenSpaceError(16.0 * 0.1)
    {
        _ellipsoid = new osg::EllipsoidModel;
        supportsExtension("verse_tiles", "Pseudo file extension");
        supportsExtension("xml", "coordinate file of ContextCapture (metadata.xml)");
        supportsExtension("json", "Decription file of 3dtiles");
        supportsExtension("children", "Internal use of 3dtiles' <children> tag");
    }

    virtual const char* className() const
    {
        return "[osgVerse] Osgb and Cesium-3dtiles Reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_tiles");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        if (ext.empty()) return createFromFolder(fileName);

        osg::ref_ptr<Options> localOptions = NULL;
        if (options) localOptions = options->cloneOptions();
        else localOptions = new osgDB::Options();
        localOptions->setPluginStringData("prefix", osgDB::getFilePath(path));
        if (ext == "children" && options)
        {
            picojson::value children;
            std::string err = picojson::parse(children, localOptions->getOptionString());
            if (err.empty() && children.is<picojson::array>())
                return createTileChildren(children.get<picojson::array>(),
                                          osgDB::getStrippedName(fileName), localOptions.get());
            else
                OSG_WARN << "[ReaderWriter3dtiles] Failed to parse from "
                         << options->getOptionString() << ": " << err << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }
        else
        {
            std::ifstream fin(fileName.c_str());
            localOptions->setPluginStringData("simple_name", osgDB::getStrippedName(fileName));
            localOptions->setPluginStringData("extension", ext);
            return (!fin) ? ReadResult::FILE_NOT_FOUND : readNode(fin, localOptions.get());
        }
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string ext = options ? options->getPluginStringData("extension") : "";
        std::string prefix = options ? options->getPluginStringData("prefix") : "";
        if (ext == "xml")
        {
            std::string xml_contents((std::istreambuf_iterator<char>(fin)),
                                     std::istreambuf_iterator<char>());
            rapidxml::xml_document<> doc;
            doc.parse<rapidxml::parse_default>((char*)xml_contents.data());

            rapidxml::xml_node<>* root = doc.first_node("ModelMetadata");
            if (root != NULL)
            {
                rapidxml::xml_node<>* srs = root->first_node("SRS");
                rapidxml::xml_node<>* origin = root->first_node("SRSOrigin");
                return createFromMetadata(
                    prefix, srs ? srs->value() : NULL, origin ? origin->value() : NULL);
            }
            else
                return ReadResult::ERROR_IN_READING_FILE;
        }

        picojson::value document;
        std::string err = picojson::parse(document, fin);
        if (err.empty())
        {
            picojson::value& root = document.get("root");
            if (root.is<picojson::object>())
            {
                std::string name = options ? options->getPluginStringData("simple_name") : "";
                osg::ref_ptr<osg::Node> node = createTile(root, prefix, name);
                node->setUserValue("RTC_ROOT", true);  // to work with RTC_CENTER in b3dm
#if WRITE_TO_OSG
                osgDB::writeNodeFile(*node, prefix + "/root.osgt");
#endif
                if (node.valid()) return node.get();
            }
            else
                OSG_WARN << "[ReaderWriter3dtiles] Bad <root> type" << std::endl;
        }
        else
            OSG_WARN << "[ReaderWriter3dtiles] Failed to parse JSON: " << err << std::endl;
        return ReadResult::ERROR_IN_READING_FILE;
    }

protected:
    osg::Node* createFromMetadata(const std::string& prefix, char* srs, char* origin) const
    {
        std::string dataFolder = prefix + "/Data/";
        osg::ref_ptr<osg::Node> tileGroup = createFromFolder(dataFolder);
        if (origin != NULL)
        {
            std::vector<std::string> coords; osgDB::split(origin, coords, ',');
            if (coords.size() > 2)
            {
                osg::Vec3d center(std::atof(coords[0].data()), std::atof(coords[1].data()),
                                  std::atof(coords[2].data()));
                tileGroup->setUserValue("SRSOrigin", center);
            }
        }
        if (srs != NULL) tileGroup->setUserValue("SRS", std::string(srs));
        return tileGroup.release();
    }

    osg::Node* createFromFolder(const std::string& prefix) const
    {
        osg::ref_ptr<osg::ProxyNode> tileProxy = new osg::ProxyNode;
        osgDB::DirectoryContents tiles = osgDB::getDirectoryContents(prefix);
        for (size_t i = 0; i < tiles.size(); ++i)
        {
            const std::string& tName = tiles[i];
            std::string ext = osgDB::getFileExtension(tName);
            std::string file = prefix + "/" + tName + "/" + tName + ".osgb";
            if (tName.empty() || !ext.empty()) continue;
            if (tName[0] < 'A' || tName[0] > 'z') continue;
            tileProxy->setFileName(i, file);
        }
        return tileProxy.release();
    }

    osg::Node* createTile(picojson::value& root, const std::string& prefix, const std::string& name) const
    {
        picojson::value& bound = root.get("boundingVolume");
        picojson::value& content = root.get("content");
        picojson::value& rangeV = root.get("geometricError");
        picojson::value& rangeSt = root.get("refine");
        picojson::value& children = root.get("children");
        picojson::value& trans = root.get("transform");

        double range = rangeV.is<double>() ? rangeV.get<double>() : 0.0;
        double sseDenominator = 0.5629, height = 1080.0; // FIXME
        if (range < 0.0 || range > 99999.0) range = FLT_MAX;  // invalid range
        range = (range * height) / (_maxScreenSpaceError * sseDenominator);

        std::string st = rangeSt.is<std::string>() ? rangeSt.get<std::string>() : "";
        osg::BoundingSphered bs = getBoundingSphere(bound);
        osg::ref_ptr<osg::Node> tile = createTile(content, children, bs, range, st, prefix, name);

        if (trans.is<picojson::array>())
        {
            picojson::array& tArray = trans.get<picojson::array>();
            osg::MatrixTransform* mt = new osg::MatrixTransform; double m[16];
            for (int i = 0; i < 16; ++i)
            {
                picojson::value& v = tArray.at(i);
                if (v.is<double>()) m[i] = v.get<double>();
            }
            mt->setMatrix(osg::Matrix(m));
            mt->addChild(tile.get()); return mt;
        }
        else return tile.release();
    }

    osg::Node* createTileChildren(picojson::array& children, const std::string& name,
                                  const osgDB::Options* localOptions) const
    {
        std::string prefix = localOptions->getPluginStringData("prefix");
        osg::Group* group = new osg::Group;
        for (size_t i = 0; i < children.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createTile(children[i], prefix, name);
            if (child.valid()) group->addChild(child.get());
        }

        if (group->getNumChildren() == 0)
        {
            std::string fallback = localOptions->getPluginStringData("fallback");
            if (!fallback.empty()) group->addChild(osgDB::readNodeFile(fallback));
        }
#if WRITE_TO_OSG
        osgDB::writeNodeFile(*group, prefix + "/" + name + ".osgt");
#endif
        return group;
    }

    osg::Node* createTile(picojson::value& content, picojson::value& children,
                          const osg::BoundingSphered& bound, double range, const std::string& st,
                          const std::string& prefix, const std::string& name) const
    {
        std::string uri = (content.is<picojson::object>() && content.contains("uri"))
                         ? content.get("uri").to_str() : "";
        std::string ext = osgDB::getFileExtension(uri);
        if (!uri.empty() && !osgDB::isAbsolutePath(uri))
            uri = prefix + osgDB::getNativePathSeparator() + uri;

        if (children.is<picojson::array>())
        {
            osg::ref_ptr<osg::Node> child0;
            if (ext == "json") child0 = osgDB::readNodeFile(uri + ".verse_tiles");
            else if (!ext.empty()) child0 = osgDB::readNodeFile(uri + ".verse_gltf");

            osg::PagedLOD* plod = new osg::PagedLOD;
            plod->setDatabasePath(prefix);
            plod->addChild(child0.valid() ? child0.get() : new osg::Node);
            if (!child0 && !uri.empty())
            {
                OSG_WARN << "[ReaderWriter3dtiles] Missing rough-level child: "
                         << uri << ", result will be lack of certain tiles" << std::endl;
                plod->getChild(0)->setName(uri);
            }

            // Put <children> to a virtual file with options to fit OSG's LOD structure
            osgDB::StringList parts; osgDB::split(name, parts, '-');
            osgDB::Options* opt = new osgDB::Options(children.serialize());
            opt->setPluginStringData("fallback", uri + (ext == "json" ? ".verse_tiles" : ".verse_gltf"));
            plod->setDatabaseOptions(opt);
            plod->setFileName(1, name + "-" + std::to_string(parts.size()) + ".children.verse_tiles");

            if (child0.valid())
            {
                osg::BoundingSphered bound2;// = bound;  // FIXME: some <boundingVolume> too far away?
                osg::BoundingSphere bound0 = child0->getBound();
                bound2.expandBy(osg::BoundingSphered(bound0.center(), bound0.radius()));

                plod->setCenterMode(osg::LOD::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
                plod->setCenter(bound2.center()); plod->setRadius(bound2.radius());
            }
            else if (bound.valid())
            {
                plod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                plod->setCenter(bound.center()); plod->setRadius(bound.radius());
            }
            else
                OSG_WARN << "[ReaderWriter3dtiles] Missing <boundingVolume>?" << std::endl;
#if true
            plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
            plod->setRange(0, (float)range * 0.25f, FLT_MAX);  // TODO: consider REPLACE/ADD?
            plod->setRange(1, 0.0f, (float)range * 0.25f);
#else
            plod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
            plod->setRange(0, 0.0f, 5000.0f / (float)range);
            plod->setRange(1, 5000.0f / (float)range, FLT_MAX);
#endif
            return plod;
        }
        else
        {
            if (ext.empty()) return new osg::Node;
            else if (ext == "json") return osgDB::readNodeFile(uri + ".verse_tiles");
            else return osgDB::readNodeFile(uri + ".verse_gltf");
        }
    }

    osg::BoundingSphered getBoundingSphere(picojson::value& bv) const
    {
        osg::BoundingSphered result;
        if (bv.contains("box"))
        {
            picojson::value& bb = bv.get("box");
            if (bb.is<picojson::array>())
            {
                picojson::array& bArray = bb.get<picojson::array>();
                osg::Vec3d center(bArray.at(0).get<double>(), bArray.at(1).get<double>(),
                                  bArray.at(2).get<double>());
                osg::Vec3d xWidth(bArray.at(3).get<double>(), bArray.at(4).get<double>(),
                                  bArray.at(5).get<double>());
                osg::Vec3d yWidth(bArray.at(6).get<double>(), bArray.at(7).get<double>(),
                                  bArray.at(8).get<double>());
                osg::Vec3d zWidth(bArray.at(9).get<double>(), bArray.at(10).get<double>(),
                                  bArray.at(11).get<double>());
                result.expandBy(center); result.expandBy(center + xWidth);
                result.expandBy(center + yWidth); result.expandBy(center + zWidth);
            }
        }
        else if (bv.contains("sphere"))
        {
            picojson::value& bs = bv.get("sphere");
            if (bs.is<picojson::array>())
            {
                picojson::array& bArray = bs.get<picojson::array>();
                osg::Vec3d center(bArray.at(0).get<double>(), bArray.at(1).get<double>(),
                                  bArray.at(2).get<double>());
                result = osg::BoundingSphered(center, bArray.at(3).get<double>());
            }
        }
        else if (bv.contains("region"))
        {
            picojson::value& br = bv.get("region");
            if (br.is<picojson::array>())
            {
                picojson::array& bArray = br.get<picojson::array>();
                double lng0 = bArray.at(0).get<double>(), lat0 = bArray.at(1).get<double>();
                double lng1 = bArray.at(2).get<double>(), lat1 = bArray.at(3).get<double>();
                double h0 = bArray.at(4).get<double>(), h1 = bArray.at(5).get<double>();
                double x = 0.0, y = 0.0, z = 0.0;
                _ellipsoid->convertLatLongHeightToXYZ(lat0, lng0, h0, x, y, z);
                result.expandBy(osg::Vec3d(x, y, z));
                _ellipsoid->convertLatLongHeightToXYZ(lat0, lng0, h1, x, y, z);
                result.expandBy(osg::Vec3d(x, y, z));
                _ellipsoid->convertLatLongHeightToXYZ(lat1, lng1, h0, x, y, z);
                result.expandBy(osg::Vec3d(x, y, z));
                _ellipsoid->convertLatLongHeightToXYZ(lat1, lng1, h1, x, y, z);
                result.expandBy(osg::Vec3d(x, y, z));
            }
        }
        else
            OSG_WARN << "[ReaderWriter3dtiles] Unknown tag in <boundingVolume>" << std::endl;
        return result;
    }

    osg::ref_ptr<osg::EllipsoidModel> _ellipsoid;
    double _maxScreenSpaceError;
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_tiles, ReaderWriter3dtiles)
