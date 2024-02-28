#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/CoordinateSystemNode>
#include <osg/MatrixTransform>
#include <osg/ProxyNode>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <nanoid/nanoid.h>
#include <picojson.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits.h>

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
        supportsExtension("verse_3dtiles", "Pseudo file extension");
        supportsExtension("json", "Decription file of 3dtiles");
        supportsExtension("children", "Internal use of 3dtile <children> tag");
    }

    virtual const char* className() const
    {
        return "[osgVerse] Cesium 3dtiles Reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_3dtiles");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        osg::ref_ptr<Options> localOptions = NULL;
        if (options)
            localOptions = options->cloneOptions();
        else
            localOptions = new osgDB::Options();
        localOptions->setPluginStringData("prefix", osgDB::getFilePath(path));

        if (ext == "children" && options)
        {
            picojson::value children;
            std::string err = picojson::parse(children, localOptions->getOptionString());
            if (err.empty() && children.is<picojson::array>())
                return createTileChildren(children.get<picojson::array>(),
                                          osgDB::getStrippedName(fileName),
                                          localOptions->getPluginStringData("prefix"));
            else
                OSG_WARN << "[ReaderWriter3dtiles] Failed to parse from "
                         << options->getOptionString() << ": " << err << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }
        else
        {
            std::ifstream fin(fileName.c_str());
            return (!fin) ? ReadResult::FILE_NOT_FOUND : readNode(fin, localOptions.get());
        }
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        picojson::value document;
        std::string err = picojson::parse(document, fin);
        std::string prefix = options ? options->getPluginStringData("prefix") : "";
        if (err.empty())
        {
            picojson::value& root = document.get("root");
            if (root.is<picojson::object>())
            {
                osg::ref_ptr<osg::Node> node = createTile(root, prefix);
                //osgDB::writeNodeFile(*node, prefix + "/root.osg");
                if (node.valid()) return node;
            }
            else
                OSG_WARN << "[ReaderWriter3dtiles] Bad <root> type" << std::endl;
        }
        else
            OSG_WARN << "[ReaderWriter3dtiles] Failed to parse JSON: " << err << std::endl;
        return ReadResult::ERROR_IN_READING_FILE;
    }

protected:
    osg::Node* createTile(picojson::value& root, const std::string& prefix) const
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
        osg::ref_ptr<osg::Node> tile = createTile(content, children, bs, range, st, prefix);

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
                                  const std::string& prefix) const
    {
        osg::Group* group = new osg::Group;
        for (size_t i = 0; i < children.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createTile(children[i], prefix);
            if (child.valid()) group->addChild(child.get());
        }
        //osgDB::writeNodeFile(*group, prefix + "/" + name + ".osg");
        return group;
    }

    osg::Node* createTile(picojson::value& content, picojson::value& children,
                          const osg::BoundingSphered& bound, double range,
                          const std::string& st, const std::string& prefix) const
    {
        std::string uri = (content.is<picojson::object>() && content.contains("uri"))
                        ? content.get("uri").to_str() : "";
        std::string ext = osgDB::getFileExtension(uri);
        if (!uri.empty() && !osgDB::isAbsolutePath(uri))
            uri = prefix + osgDB::getNativePathSeparator() + uri;

        if (children.is<picojson::array>())
        {
            osg::PagedLOD* plod = new osg::PagedLOD;
            plod->setDatabasePath(prefix);
            if (ext.empty()) plod->addChild(new osg::Node);
            else if (ext == "json") plod->addChild(osgDB::readNodeFile(uri + ".verse_3dtiles"));
            else plod->addChild(osgDB::readNodeFile(uri + ".verse_gltf"));

            // Put <children> to a virtual file with options to fit OSG's LOD structure
            osgDB::Options* opt = new osgDB::Options(children.serialize());
            plod->setDatabaseOptions(opt);
            plod->setFileName(1, nanoid::generate(8) + ".children.verse_3dtiles");

            if (bound.valid())
            {
                plod->setCenterMode(osg::LOD::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
                plod->setCenter(bound.center());
                plod->setRadius(bound.radius());
            }
            else
                OSG_WARN << "[ReaderWriter3dtiles] Missing <boundingVolume>?" << std::endl;
#if true
            plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
            plod->setRange(0, (float)range, FLT_MAX);  // TODO: consider REPLACE/ADD?
            plod->setRange(1, 0.0f, (float)range);
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
            else if (ext == "json") return osgDB::readNodeFile(uri + ".verse_3dtiles");
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
REGISTER_OSGPLUGIN(verse_3dtiles, ReaderWriter3dtiles)
