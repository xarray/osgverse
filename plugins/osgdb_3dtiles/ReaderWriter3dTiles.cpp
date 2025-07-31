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
#include "pipeline/Global.h"
#include "readerwriter/Utilities.h"
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

// OSGB:    osgviewer G:\OsgData\metadata.xml.verse_tiles
// OSGB:    osgviewer G:\OsgData\Data.verse_tiles
// 3DTILES: osgviewer G:\3DTilesData\tileset.json.verse_tiles
// 3DTILES: osgviewer https://earthsdk.com/v/last/Apps/assets/dayanta/tileset.json.verse_web -O "Extension=verse_tiles"
// Folder:  osgviewer G:\FolderOfIVEs.verse_tiles
class ReaderWriter3dtiles : public osgDB::ReaderWriter
{
public:
    ReaderWriter3dtiles() : _maxScreenSpaceError(16.0)
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
        std::string ext; std::string fileName = getRealFileName(path, ext);
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

        picojson::value document; std::string err = picojson::parse(document, fin);
        osg::ref_ptr<osgDB::Options> opt = const_cast<osgDB::Options*>(options);
        if (err.empty())
        {
            picojson::value& asset = document.get("asset"); bool yAxisUp = true;
            if (asset.is<picojson::object>() && asset.contains("gltfUpAxis"))
            {
                picojson::value& upAxis = asset.get("gltfUpAxis");
                std::string val = upAxis.is<std::string>() ? upAxis.get<std::string>() : "";
                if (val == "Z" || val == "z")
                {
                    if (!opt) opt = new osgDB::Options;
                    opt->setPluginStringData("UpAxis", "1"); yAxisUp = false;
                }
            }

            picojson::value& root = document.get("root");
            if (root.is<picojson::object>())
            {
                std::string name = opt.valid() ? opt->getPluginStringData("simple_name") : "";
                osg::ref_ptr<osg::Node> node = createTile(root, prefix, name, "", opt.get());
                if (node.valid())
                {
                    std::string sub_tile = opt.valid() ? opt->getPluginStringData("sub_tile") : "";
                    if (yAxisUp && sub_tile.empty())  // no sub_tile, it should be root
                    {
                        // FIXME: any more transformations?
                    }
#if WRITE_TO_OSG
                    osgDB::writeNodeFile(*node, prefix + "/root.osgt");
#endif
                    return node.get();
                }
            }
            else
                OSG_WARN << "[ReaderWriter3dtiles] Bad <root> type" << std::endl;
        }
        else
            OSG_WARN << "[ReaderWriter3dtiles] Failed to parse JSON: " << err << std::endl;
        return ReadResult::ERROR_IN_READING_FILE;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_tiles");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

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
            if (tName[0] == '.') continue;
            tileProxy->setFileName(i, file);
        }

        if (tileProxy->getNumFileNames() == 0)
        {
            osg::ref_ptr<osg::Group> group = new osg::Group;
            for (size_t i = 0; i < tiles.size(); ++i)
            {
                const std::string& tName = tiles[i];
                if (tName[0] == '.') continue;

                osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(prefix + "/" + tName);
                if (node.valid())
                {
                    OSG_NOTICE << "[ReaderWriter3dtiles] Loaded " << tName << std::endl;
                    group->addChild(node.get());
                }
            }
            return group.release();
        }
        return tileProxy.release();
    }
    
    osg::Node* createTileChildren(picojson::array& children, const std::string& name,
                                  const osgDB::Options* localOptions) const
    {
        osg::ref_ptr<osgDB::Options> opt = localOptions ? localOptions->cloneOptions() : new osgDB::Options;
        std::string refine = localOptions->getPluginStringData("refinement");
        std::string prefix = localOptions->getPluginStringData("prefix");

        osg::Group* group = new osg::Group; group->setName("TileGroup:" + name);
        for (size_t i = 0; i < children.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createTile(
                children[i], prefix, name + "," + std::to_string(i), refine, opt.get());
            if (child.valid()) group->addChild(child.get());
        }

        if (group->getNumChildren() == 0)
        {
            std::string fallback = localOptions->getPluginStringData("fallback");
            if (!fallback.empty()) group->addChild(osgDB::readNodeFile(fallback, opt.get()));
        }
#if WRITE_TO_OSG
        osgDB::writeNodeFile(*group, prefix + "/" + name + ".osgt");
#endif
        return group;
    }

    osg::Node* createTile(picojson::value& root, const std::string& prefix, const std::string& name,
                          const std::string& parentRefine, const osgDB::Options* options) const
    {
        picojson::value& bound = root.get("boundingVolume");
        picojson::value& content = root.get("content");
        picojson::value& rangeV = root.get("geometricError");
        picojson::value& rangeSt = root.get("refine");
        picojson::value& children = root.get("children");
        picojson::value& trans = root.get("transform");
        osg::ref_ptr<osgDB::Options> opt = options ? options->cloneOptions() : new osgDB::Options;
        opt->setPluginStringData("sub_tile", name);

        double range = rangeV.is<double>() ? rangeV.get<double>() : 0.0;
        double sseDenominator = 0.5629, height = 1080.0; // FIXME
        if (range < 0.0 || range > 99999.0) range = FLT_MAX;  // invalid range
        range = (range * height) / (_maxScreenSpaceError * sseDenominator);

        bool isAbsoluteBound = false;  // FIXME: how to handle <region>?
        osg::BoundingSphered bs = getBoundingSphere(bound, isAbsoluteBound);
        std::string st = rangeSt.is<std::string>() ? rangeSt.get<std::string>() : "";
        if (st.empty()) st = parentRefine;

        osg::ref_ptr<osg::Node> tile = createTile(
            content, children, bs, range, st, prefix, name, opt.get(), isAbsoluteBound);
        if (trans.is<picojson::array>())
        {
            picojson::array& tArray = trans.get<picojson::array>();
            osg::MatrixTransform* mt = new osg::MatrixTransform; double m[16];
            for (int i = 0; i < 16; ++i)
            {
                picojson::value& v = tArray.at(i);
                if (v.is<double>()) m[i] = v.get<double>();
            }
            mt->setMatrix(osg::Matrix(m)); mt->setName("TileTransform");
            mt->addChild(tile.get()); return mt;
        }
        else return tile.release();
    }

    osg::Node* createTile(picojson::value& content, picojson::value& children,
                          const osg::BoundingSphered& bound, double range, const std::string& st,
                          const std::string& prefix, const std::string& name,
                          const osgDB::Options* options, bool absBound) const
    {
        std::string uri = (content.is<picojson::object>() && content.contains("uri"))
                         ? content.get("uri").to_str() : "";
        if (uri.empty())
        {
            uri = (content.is<picojson::object>() && content.contains("url"))
                ? content.get("url").to_str() : "";
        }
        uri = osgVerse::urlDecode(uri);  // some data converted from CesiumLab may have encoded characters...

        std::string ext = osgDB::getFileExtension(uri);
        if (!uri.empty() && !osgDB::isAbsolutePath(uri))
        {
            if (osgDB::getServerProtocol(prefix) != "") uri = prefix + "/" + uri;
            else uri = prefix + osgDB::getNativePathSeparator() + uri;
        }

        bool additive = (st == "ADD" || st == "add");
        if (children.is<picojson::array>())
        {
            osg::ref_ptr<osg::Node> child0;
            if (ext == "json") child0 = osgDB::readNodeFile(uri + ".verse_tiles", options);
            else if (!ext.empty()) child0 = osgDB::readNodeFile(uri + ".verse_gltf", options);

            osg::PagedLOD* plod = new osg::PagedLOD;
            plod->setName("TileLod:" + name); plod->setDatabasePath(prefix);
            plod->addChild(child0.valid() ? child0.get() : new osg::Node);
            if (!child0 && !uri.empty())
            {
                OSG_WARN << "[ReaderWriter3dtiles] Missing rough-level child: "
                         << uri << ", result will be lack of certain tiles" << std::endl;
                plod->getChild(0)->setName(uri);
            }

            // Put <children> to a virtual file with options to fit OSG's LOD structure
            osgDB::StringList parts; osgDB::split(name, parts, '-');
            osgDB::Options* childOpt = options ? options->cloneOptions() : new osgDB::Options;
            childOpt->setOptionString(children.serialize());
            childOpt->setPluginStringData("fallback", uri + (ext == "json" ? ".verse_tiles" : ".verse_gltf"));
            childOpt->setPluginStringData("refinement", st);
            plod->setDatabaseOptions(childOpt);
            plod->setFileName(1, name + "-" + std::to_string(parts.size()) + ".children.verse_tiles");

            /*if (child0.valid())
                std::cout << uri << ": CHILD = " << child0->getBound().center() << "; " << child0->getBound().radius()
                          << ";; REGION = " << bound.center() <<"; " << bound.radius() << "\n";
            else
                std::cout << uri << ": REGION = " << bound.center() << "; " << bound.radius() << "\n";*/

            if (child0.valid() && bound.valid())
            {
                const osg::BoundingSphere& childBs = child0->getBound();
                float diff = (childBs.center() - bound.center()).length();
                if (bound.radius() < diff)
                {
                    OSG_WARN << "[ReaderWriter3dtiles] Given bounding volume (center = " << bound.center() << ", r = "
                             << bound.radius() << ") is totally different with current child bound (center = " << childBs.center()
                             << ", r = " << childBs.radius() << "). Result may be unexpected." << std::endl;
                }
            }

            if (child0.valid())
            {   // FIXME: some <boundingVolume> too far away?
                plod->setCenterMode(osg::LOD::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
                plod->setCenter(bound.center()); plod->setRadius(bound.radius());
            }
            else if (bound.valid())
            {
                plod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                plod->setCenter(bound.center()); plod->setRadius(bound.radius());
            }
            else
                OSG_WARN << "[ReaderWriter3dtiles] Missing <boundingVolume>?" << std::endl;

            plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
            if (additive) plod->setRange(0, 0.0f, FLT_MAX);
            else plod->setRange(0, (float)range, FLT_MAX);
            plod->setRange(1, 0.0f, (float)range);
            return plod;
        }
        else
        {
            if (ext.empty()) return new osg::Node;
            else if (ext == "json") return osgDB::readNodeFile(uri + ".verse_tiles", options);
            else return osgDB::readNodeFile(uri + ".verse_gltf", options);
        }
    }

    osg::BoundingSphered getBoundingSphere(picojson::value& bv, bool& absolutely) const
    {
        osg::BoundingSphered result; absolutely = false;
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
                double x = 0.0, y = 0.0, z = 0.0; absolutely = true;
                _ellipsoid->convertLatLongHeightToXYZ(lat0, lng0, h0, x, y, z);
                result.expandBy(osg::Vec3d(x, z, -y));
                _ellipsoid->convertLatLongHeightToXYZ(lat0, lng0, h1, x, y, z);
                result.expandBy(osg::Vec3d(x, z, -y));
                _ellipsoid->convertLatLongHeightToXYZ(lat1, lng1, h0, x, y, z);
                result.expandBy(osg::Vec3d(x, z, -y));
                _ellipsoid->convertLatLongHeightToXYZ(lat1, lng1, h1, x, y, z);
                result.expandBy(osg::Vec3d(x, z, -y));
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
