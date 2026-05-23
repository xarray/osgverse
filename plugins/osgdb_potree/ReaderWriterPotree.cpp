//----------------------------------------------------------------------------//
//                                                                            //
// osgdb_potree is hosted at https://gitee.com/osg_opensource/osg-potree.git  //
// and distributed under the Apache-2.0 License.                              //
//                                                                            //
#include <osg/Notify>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osg/Texture2D>
#include <osg/Image>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <picojson.h>
#include "Attributes.h"

class HNode : public osg::Referenced
{
public:
    enum TYPE
    {
        NORMAL = 0,
        LEAF = 1,
        PROXY = 2,
    };

public:
    HNode::HNode()
    {
        _name = "";
        _numPoints = 0;
        _childMask = 0;
        _byteOffset = 0;
        _byteSize = 0;
        _type = TYPE::LEAF;
        _children.resize(8);
        for (int i = 0; i < 8; i++)
        {
            _children[i] = NULL;
        }
    }

    std::string   _name = "";
    int      _numPoints = 0;
    uint8_t  _childMask = 0;
    uint64_t _byteOffset = 0;
    uint64_t _byteSize = 0;
    TYPE     _type = TYPE::LEAF;
    std::vector<osg::ref_ptr<HNode> > _children;
};

class PotreeContainer : public osg::Object
{
public:
    PotreeContainer()
    {
    }

    PotreeContainer(const PotreeContainer& other, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) : osg::Object(other, copyop)
    {
        _hNode = other._hNode;
    }

    explicit PotreeContainer(osg::ref_ptr<HNode> hNode, osg::ref_ptr<Attributes> attributes)
    {
        _hNode = hNode;
        _attributes = attributes;
    }

    osg::ref_ptr<HNode> node() const
    {
        return _hNode;
    }

    osg::ref_ptr<Attributes> attributes()  const
    {
        return _attributes;
    }

    META_Object(OSG, PotreeContainer)

protected:
    osg::ref_ptr<HNode> _hNode;
    osg::ref_ptr<Attributes> _attributes;
};

class ReaderWriterPotree : public osgDB::ReaderWriter
{
public:
    ReaderWriterPotree()
    {
        supportsExtension("potree", "potree reader");
        supportsExtension("pchildren", "Internal use of potree <children> tag");
    }

    virtual const char* className() const
    {
        return "Potree Reader";
    }

    void ParseToHNode(const std::vector<char>& data, osg::ref_ptr<HNode> node) const
    {
        node->_type = HNode::TYPE(data[0]);
        node->_childMask = uint8_t(data[1]);
        node->_numPoints = ((uint32_t*)(data.data() + 2))[0];
        node->_byteOffset = ((uint64_t*)(data.data() + 6))[0];
        node->_byteSize = ((uint64_t*)(data.data() + 14))[0];
    }

    bool ParseHierarchy(const std::string& hierarchyPath, int offset, osg::ref_ptr<HNode>& hNode) const
    {
        std::fstream fin(hierarchyPath, std::ios::binary | std::ios::in);
        if (!fin)
        {
            OSG_WARN << "cannot open file:" << hierarchyPath << std::endl;
            return false;
        }
        fin.seekg(offset, std::ios::beg);
        hNode = new HNode;
        std::vector<char> data(22, 0);
        fin.read(data.data(), 22);
        ParseToHNode(data, hNode);
        std::list<osg::ref_ptr<HNode> > queues;
        queues.push_back(hNode);
        while (queues.size() > 0)
        {
            osg::ref_ptr<HNode> hNode = queues.front();
            queues.pop_front();
            if (hNode->_type == HNode::TYPE::PROXY)
            {
                continue;
            }
            for (int i = 0; i < 8; i++)
            {
                if (hNode->_childMask & (1 << i))
                {
                    hNode->_children[i] = new HNode;
                    std::vector<char> data(22, 0);
                    fin.read(data.data(), 22);
                    ParseToHNode(data, hNode->_children[i]);
                    queues.push_back(hNode->_children[i]);
                }
            }
        }
        fin.close();
        return true;
    }

    bool ParseMetadata(const std::string& path, osg::ref_ptr<Attributes>& attributes) const
    {
        attributes = new Attributes;
        std::ifstream file(path);
        if (!file.is_open())
        {
            OSG_WARN << "canot open file:" << path << std::endl;
            return false;
        }

        picojson::value data;
        std::string err = picojson::parse(data, file);
        file.close();

        if (!err.empty())
        {
            OSG_WARN << "JSON parse error: " << err << std::endl;
            return false;
        }

        // Parse BoundingBox
        const picojson::value& bbox = data.get("boundingBox");
        const picojson::array& bbox_min_arr = bbox.get("min").get<picojson::array>();
        const picojson::array& bbox_max_arr = bbox.get("max").get<picojson::array>();

        std::vector<double> bbox_min;
        std::vector<double> bbox_max;
        for (const auto& v : bbox_min_arr) bbox_min.push_back(v.get<double>());
        for (const auto& v : bbox_max_arr) bbox_max.push_back(v.get<double>());

        attributes->_box = osg::BoundingBoxd(
            osg::Vec3d(bbox_min[0], bbox_min[1], bbox_min[2]),
            osg::Vec3d(bbox_max[0], bbox_max[1], bbox_max[2])
        );

        // Parse offset, scale
        const picojson::array& offset_arr = data.get("offset").get<picojson::array>();
        const picojson::array& scale_arr = data.get("scale").get<picojson::array>();

        std::vector<double> offset;
        std::vector<double> scale;
        for (const auto& v : offset_arr) offset.push_back(v.get<double>());
        for (const auto& v : scale_arr) scale.push_back(v.get<double>());

        attributes->_posOffset = osg::Vec3d(offset[0], offset[1], offset[2]);
        attributes->_posScale = osg::Vec3d(scale[0], scale[1], scale[2]);

        // Parse attributes array
        std::vector<Attribute> vecAttribute;
        const picojson::array& attributes_arr = data.get("attributes").get<picojson::array>();

        for (const auto& attr_val : attributes_arr)
        {
            const picojson::value& attr = attr_val;
            Attribute a;
            a.name = attr.get("name").get<std::string>();
            a.description = attr.get("description").get<std::string>();
            a.size = static_cast<uint32_t>(attr.get("size").get<double>());
            a.numElements = static_cast<uint32_t>(attr.get("numElements").get<double>());
            a.elementSize = static_cast<uint32_t>(attr.get("elementSize").get<double>());
            a.type = typenameToType(attr.get("type").get<std::string>());

            // min
            const picojson::array& mindata_arr = attr.get("min").get<picojson::array>();
            std::vector<double> mindata;
            for (const auto& v : mindata_arr) mindata.push_back(v.get<double>());
            if (mindata.size() == 3)
                a.min = osg::Vec3d(mindata[0], mindata[1], mindata[2]);
            else
                a.min = osg::Vec3d(mindata[0], mindata[0], mindata[0]);

            // max
            const picojson::array& maxdata_arr = attr.get("max").get<picojson::array>();
            std::vector<double> maxdata;
            for (const auto& v : maxdata_arr) maxdata.push_back(v.get<double>());
            if (maxdata.size() == 3)
                a.max = osg::Vec3d(maxdata[0], maxdata[1], maxdata[2]);
            else
                a.max = osg::Vec3d(maxdata[0], maxdata[0], maxdata[0]);

            // scale
            const picojson::array& scaledata_arr = attr.get("scale").get<picojson::array>();
            std::vector<double> scaledata;
            for (const auto& v : scaledata_arr) scaledata.push_back(v.get<double>());
            if (scaledata.size() == 3)
                a.scale = osg::Vec3d(scaledata[0], scaledata[1], scaledata[2]);
            else
                a.scale = osg::Vec3d(scaledata[0], scaledata[0], scaledata[0]);

            // offset
            const picojson::array& offsetdata_arr = attr.get("offset").get<picojson::array>();
            std::vector<double> offsetdata;
            for (const auto& v : offsetdata_arr) offsetdata.push_back(v.get<double>());
            if (offsetdata.size() == 3)
                a.offset = osg::Vec3d(offsetdata[0], offsetdata[1], offsetdata[2]);
            else
                a.offset = osg::Vec3d(offsetdata[0], offsetdata[0], offsetdata[0]);

            // 特殊处理 histogram (只有 classification 有)
            const picojson::object& attr_obj = attr.get<picojson::object>();
            if (attr_obj.find("histogram") != attr_obj.end())
            {
                const picojson::array& hist_arr = attr.get("histogram").get<picojson::array>();
                std::vector<int64_t> histogram;
                for (const auto& v : hist_arr)
                {
                    // picojson 的整数可能以 double 存储，需要转换
                    histogram.push_back(static_cast<int64_t>(v.get<double>()));
                }
                a.histogram = histogram;
            }

            vecAttribute.push_back(a);
        }

        attributes->setAttribute(vecAttribute);
        return true;
    }

    virtual ReadResult readNode(const std::string& location, const Options* options) const
    {
        std::string ext = osgDB::getFileExtension(location);
        if (!acceptsExtension(ext))
            return ReadResult::FILE_NOT_HANDLED;
        std::string filePathDir = osgDB::getNameLessExtension(location);
        if (ext == "pchildren" && options && options->getUserDataContainer())
        {
            osg::ref_ptr<const PotreeContainer> parentHNodeContainer = dynamic_cast<const PotreeContainer*> (options->getUserDataContainer()->getUserObject("ParentTile"));
            if (parentHNodeContainer)
            {
                return createTileChildren(parentHNodeContainer->node(), parentHNodeContainer->attributes(), osgDB::getNameLessExtension(filePathDir), options);
            }
        }
        else
        {
            std::string hierarchyPath = osgDB::concatPaths(filePathDir, "hierarchy.bin");
            osg::ref_ptr<HNode> hNode;
            if (ParseHierarchy(hierarchyPath, 0, hNode))
            {
                osg::ref_ptr<Attributes> attributes = NULL;
                ParseMetadata(osgDB::concatPaths(filePathDir, "metadata.json"), attributes);
                return createTile(hNode, attributes, filePathDir, options);
            }
        }
        return NULL;
    }

    osg::Node* createTileChildren(osg::ref_ptr<HNode> parentHNode, osg::ref_ptr<Attributes> attributes, const std::string& filePathDir, const Options* options) const
    {
        if (!parentHNode)
        {
            return NULL;
        }

        osg::ref_ptr<osg::Group> grp = new osg::Group;
        for (int i = 0; i < 8; i++)
        {
            if (parentHNode->_children[i])
            {
                osg::ref_ptr<osg::Node> node = createTile(parentHNode->_children[i], attributes, filePathDir, options);
                if (node)
                {
                    grp->addChild(node);
                }
            }
        }
        return grp.release();
    }

    osg::Node* createPointCloudNode(const std::vector<char>& data, int pointNum, osg::ref_ptr<Attributes> attributes) const
    {
        osg::Vec3d center = attributes->_box.center();
        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform(osg::Matrix::translate(center));
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
        mt->addChild(geometry);
        osg::ref_ptr<osg::Vec3Array> v3a = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4ubArray> v4c = new osg::Vec4ubArray;
        for (int i = 0; i < pointNum; i++)
        {
            const char* pData = data.data() + (attributes->_bytes * i);
            for (int j = 0; j < attributes->_list.size(); j++)
            {
                if ("position" == attributes->_list[j].name)
                {
                    if (INT32 == attributes->_list[j].type || UINT32 == attributes->_list[j].type)
                    {
                        int* pDataInt = (int*)pData;
                        osg::Vec3d position = osg::Vec3d(attributes->_posOffset.x() + attributes->_posScale.x() * pDataInt[0] - center.x(),
                            attributes->_posOffset.y() + attributes->_posScale.y() * pDataInt[1] - center.y(),
                            attributes->_posOffset.z() + attributes->_posScale.z() * pDataInt[2] - center.z());
                        v3a->push_back(position);
                    }
                    else
                    {
                        OSG_FATAL << "position type is:" << attributes->_list[j].type << ", please improve" << std::endl;
                    }
                }
                else if ("rgb" == attributes->_list[j].name)
                {
                    if (INT16 == attributes->_list[j].type || UINT16 == attributes->_list[j].type)
                    {
                        uint16_t* pDataUInt16 = (uint16_t*)pData;
                        osg::Vec4ub color = osg::Vec4ub(pDataUInt16[0] >> 8,
                            pDataUInt16[1] >> 8,
                            pDataUInt16[2] >> 8,
                            255);
                        v4c->push_back(color);
                    }
                    else
                    {
                        OSG_FATAL << "color type is:" << attributes->_list[j].type << ", please improve" << std::endl;
                    }
                }
                pData += attributes->_list[j].size;
            }
        }
        geometry->setVertexArray(v3a);
        if (v4c->size() > 0)
        {
            geometry->setColorArray(v4c, osg::Array::BIND_PER_VERTEX);
        }
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, v3a->size()));
        return mt.release();
    }

    osg::Node* createTile(osg::ref_ptr<HNode> hNode, osg::ref_ptr<Attributes> attributes, const std::string& filePathDir, const Options* options) const
    {
        if (!hNode)
        {
            return NULL;
        }
        std::string binPath = osgDB::concatPaths(filePathDir, "octree.bin");
        if (HNode::TYPE::PROXY == hNode->_type)
        {
            std::string hierarchyPath = osgDB::concatPaths(filePathDir, "hierarchy.bin");
            osg::ref_ptr<HNode> hChunkNode = NULL;
            if (ParseHierarchy(hierarchyPath, hNode->_byteOffset, hChunkNode))
            {
                return createTile(hChunkNode, attributes, filePathDir, options);
            }
        }
        else
        {
            std::fstream fin(binPath, std::ios::binary | std::ios::in);
            if (!fin)
            {
                OSG_WARN << "cannot open file:" << binPath << std::endl;
                return NULL;
            }
            fin.seekg(hNode->_byteOffset, std::ios::beg);
            std::vector<char> data(hNode->_byteSize, 0);
            fin.read(data.data(), hNode->_byteSize);
            fin.close();
            osg::ref_ptr<osg::Node> pointcloudNode = createPointCloudNode(data, hNode->_numPoints, attributes);
            if (0 != hNode->_childMask)
            {
                osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                plod->addChild(pointcloudNode);
                osgDB::Options* childOpt = new osgDB::Options;
                osg::ref_ptr<PotreeContainer> parentContainer = new PotreeContainer(hNode, attributes);
                parentContainer->setName("ParentTile");
                childOpt->getOrCreateUserDataContainer()->addUserObject(parentContainer);
                plod->setDatabaseOptions(childOpt);
                plod->setFileName(1, filePathDir + "." + std::to_string(hNode->_byteOffset) + ".pchildren");

                if (pointcloudNode.valid())
                {
                    osg::BoundingSphered bound2;
                    osg::BoundingSphere bound0 = pointcloudNode->getBound();
                    bound2.expandBy(osg::BoundingSphered(bound0.center(), bound0.radius()));
                    plod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                    plod->setCenter(bound2.center()); 
                    plod->setRadius(bound2.radius());
                }
                else
                {
                    OSG_WARN << "[ReaderWriterPotree] Missing <boundingVolume>?" << std::endl;
                }
                plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
                plod->setRange(0, 0.0f, FLT_MAX);
                double radius = plod->getRadius();
                //Assuming the screen height in pixels is 1080.
                //The maximum cross-sectional area of the data on the screen is 4 * radius * radius
                //The field of view is 60 degrees, with tan(30) = 0.5629
                //assuing every points is rendered as a 1 pixel square
                double range = 1080 * std::sqrt(4 * radius * radius / hNode->_numPoints) /2/ 0.5629;
                plod->setRange(1, 0, range);
                return plod.release();
            }
            else
            {
                return pointcloudNode.release();
            }
        }

        return NULL;
    }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& location, const Options* options) const
    {
        std::string ext = osgDB::getFileExtension(location);
        if (!acceptsExtension(ext))
            return WriteResult::FILE_NOT_HANDLED;
        OSG_WARN << "not implemented" << std::endl;
        return WriteResult::FILE_SAVED;
    }

};

REGISTER_OSGPLUGIN(potree, ReaderWriterPotree)
