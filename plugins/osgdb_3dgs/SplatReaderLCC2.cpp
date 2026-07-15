#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/MatrixTransform>
#include <osg/LOD>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

#include "modeling/GaussianGeometry.h"
#include "modeling/Utilities.h"
#include "readerwriter/Utilities.h"
#include "3rdparty/picojson.h"

namespace
{
    // Safely get a double value from picojson object
    static double getDouble(const picojson::object& obj, const std::string& key, double defaultVal = 0.0)
    {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.is<double>()) return it->second.get<double>();
        return defaultVal;
    }

    // Safely get an array from picojson object
    static picojson::array getArray(const picojson::object& obj, const std::string& key)
    {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.is<picojson::array>()) return it->second.get<picojson::array>();
        return picojson::array();
    }

    // Safely get a string from picojson object
    static std::string getString(const picojson::object& obj, const std::string& key, const std::string& defaultVal = "")
    {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.is<std::string>()) return it->second.get<std::string>();
        return defaultVal;
    }

    // Get children from an octree node (tolerates both array and string-keyed object map)
    static picojson::array getOctreeChildren(const picojson::value& node)
    {
        picojson::array result;
        if (!node.is<picojson::object>()) return result;

        const picojson::object& obj = node.get<picojson::object>();
        auto it = obj.find("child");
        if (it == obj.end()) return result;

        const picojson::value& child = it->second;
        if (child.is<picojson::array>())
        {
            result = child.get<picojson::array>();
        }
        else if (child.is<picojson::object>())
        {
            const picojson::object& childObj = child.get<picojson::object>();
            for (auto cit = childObj.begin(); cit != childObj.end(); ++cit)
                result.push_back(cit->second);
        }
        return result;
    }

    // Recursively normalize old-protocol octree node field names (datatype->dataType, child_num->childNum)
    static void normalizeOctreeNode(picojson::value& node)
    {
        if (!node.is<picojson::object>()) return;
        picojson::object& obj = node.get<picojson::object>();

        if (obj.find("datatype") != obj.end())
        {
            obj["dataType"] = obj["datatype"];
            obj.erase("datatype");
        }
        if (obj.find("child_num") != obj.end())
        {
            obj["childNum"] = obj["child_num"];
            obj.erase("child_num");
        }
        picojson::array children = getOctreeChildren(node);
        if (children.empty()) return;

        // Update children back (in case they were in object map form)
        auto it = obj.find("child");
        if (it != obj.end() && it->second.is<picojson::object>())
        {
            picojson::object& childObj = it->second.get<picojson::object>();
            size_t idx = 0;
            for (auto cit = childObj.begin(); cit != childObj.end() && idx < children.size(); ++cit, ++idx)
            {
                normalizeOctreeNode(children[idx]);
                cit->second = children[idx];
            }
        }
        else if (it != obj.end() && it->second.is<picojson::array>())
        {
            picojson::array& childArr = it->second.get<picojson::array>();
            for (size_t i = 0; i < childArr.size() && i < children.size(); ++i)
            {
                normalizeOctreeNode(children[i]);
                childArr[i] = children[i];
            }
        }
    }

#if true
    // Collect chunk file indices by LOD level from the octree.
    // Each entry: (fileIndex, splatCount), where count < 0 means unknown.
    static void collectChunksByLevel(const picojson::value& node, int depth, int totalLevels,
                                     int envFileIndex, std::map<int, std::vector<std::pair<int, int>>>& result)
    {
        picojson::array children = getOctreeChildren(node);
        for (size_t i = 0; i < children.size(); ++i)
        {
            const picojson::value& child = children[i];
            if (!child.is<picojson::object>()) continue;
            const picojson::object& childObj = child.get<picojson::object>();

            // Check for 3dgs data
            auto dataIt = childObj.find("data");
            if (dataIt != childObj.end() && dataIt->second.is<picojson::object>())
            {
                const picojson::object& dataObj = dataIt->second.get<picojson::object>();
                auto d3dgsIt = dataObj.find("3dgs");
                if (d3dgsIt != dataObj.end() && d3dgsIt->second.is<picojson::object>())
                {
                    const picojson::object& d3dgs = d3dgsIt->second.get<picojson::object>();
                    auto nameIt = d3dgs.find("name");
                    if (nameIt != d3dgs.end() && nameIt->second.is<double>())
                    {
                        int fileIndex = (int)nameIt->second.get<double>();
                        if (fileIndex != envFileIndex)
                        {
                            int level = totalLevels - depth;
                            int count = -1;
                            auto countIt = d3dgs.find("count");
                            if (countIt != d3dgs.end() && countIt->second.is<double>())
                                count = (int)countIt->second.get<double>();
                            result[level].push_back(std::make_pair(fileIndex, count));
                        }
                    }
                }
            }

            // Recurse into children
            collectChunksByLevel(child, depth + 1, totalLevels, envFileIndex, result);
        }
    }

    // Resolve LOD selection. Negative indices count from the end.
    // Empty selection means all levels [0, totalLevels).
    static std::vector<int> resolveLodSelection(const std::vector<int>& lodSelect, int totalLevels)
    {
        std::vector<int> result;
        if (!lodSelect.empty())
        {
            for (size_t i = 0; i < lodSelect.size(); ++i)
            {
                int lod = lodSelect[i];
                if (lod < 0) lod = totalLevels + lod;
                if (lod >= 0 && lod < totalLevels) result.push_back(lod);
            }
        }
        else
        {
            result.reserve(totalLevels);
            for (int i = 0; i < totalLevels; ++i) result.push_back(i);
        }
        return result;
    }
#endif

    // Strip trailing commas before } or ] outside string literals (non-strict JSON fix)
    static std::string stripTrailingCommas(const std::string& text)
    {
        std::string result;
        result.reserve(text.size());
        bool inString = false, escaped = false;
        for (size_t i = 0; i < text.size(); ++i)
        {
            char c = text[i];
            if (inString)
            {
                if (escaped) { escaped = false; }
                else if (c == '\\') { escaped = true; }
                else if (c == '"') { inString = false; }
                result += c;
                continue;
            }
            if (c == '"') { inString = true; result += c; continue; }
            if (c == ',')
            {
                size_t j = i + 1;
                while (j < text.size() && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r')) j++;
                if (j < text.size() && (text[j] == '}' || text[j] == ']')) continue;
            }
            result += c;
        }
        return result;
    }

    // Parse boundingBox from meta JSON
    static osg::BoundingBoxd parseBoundingBox(const picojson::object& metaObj)
    {
        osg::BoundingBoxd bb;
        auto bbIt = metaObj.find("boundingBox");
        if (bbIt == metaObj.end() || !bbIt->second.is<picojson::object>()) return bb;

        const picojson::object& bbObj = bbIt->second.get<picojson::object>();
        picojson::array minArr = getArray(bbObj, "min");
        picojson::array maxArr = getArray(bbObj, "max");
        if (minArr.size() >= 3 && maxArr.size() >= 3)
        {
            bb._min.set(minArr[0].get<double>(), minArr[1].get<double>(), minArr[2].get<double>());
            bb._max.set(maxArr[0].get<double>(), maxArr[1].get<double>(), maxArr[2].get<double>());
        }
        return bb;
    }

    // =========================================================================
    // LCC2 .btree BVH format support
    // =========================================================================
    static const uint32_t BTREE_NODE_SIZE = 32;

    // Single BVH node (32 bytes, no dynamic allocation)
    struct BtreeNode
    {
        float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
        float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
        uint32_t field6 = 0, field7 = 0;

        bool isLeaf() const { return ((field7 >> 16) & 0xFFFF) == 0xFFFF; }
        uint32_t triangleOffset() const { return field6; }  // leaf only
        uint16_t triangleCount() const { return (uint16_t)(field7 & 0xFFFF); }  // leaf only
        uint32_t rightChildByteOffset() const { return field6 * 4; }  // internal only
        uint32_t splitAxis() const { return field7; }  // internal only (0=X, 1=Y, 2=Z)

        osg::BoundingBoxf getBounds() const
        { return osg::BoundingBoxf(osg::Vec3f(minX, minY, minZ), osg::Vec3f(maxX, maxY, maxZ)); }
    };

    // Parse a single node from raw btree data at the given byte offset
    static BtreeNode parseBtreeNode(const uint8_t* data, uint32_t byteOffset)
    {
        BtreeNode node;
        const float* fp = (const float*)(data + byteOffset);
        const uint32_t* u32p = (const uint32_t*)(data + byteOffset);
        node.minX = fp[0]; node.minY = fp[1]; node.minZ = fp[2];
        node.maxX = fp[3]; node.maxY = fp[4]; node.maxZ = fp[5];
        node.field6 = u32p[6]; node.field7 = u32p[7];
        return node;
    }

#if true
    // Mesh + BVH info collected from octree nodes
    struct MeshNodeInfo
    {
        std::string nodeId;           // octree node id, e.g. "0_3_0_0"
        int meshFileIndex = -1;       // index into root.meshFiles
        int bvhFileIndex = -1;        // index into root.bvhFiles
        uint32_t vertexCount = 0;
        uint32_t faceCount = 0;
        osg::BoundingBoxd bbox;       // node bounding box
    };

    // Recursively collect octree nodes that carry mesh data
    static void collectMeshNodes(const picojson::value& node, std::vector<MeshNodeInfo>& result)
    {
        picojson::array children = getOctreeChildren(node);
        for (size_t i = 0; i < children.size(); ++i)
        {
            const picojson::value& child = children[i];
            if (!child.is<picojson::object>()) continue;
            const picojson::object& childObj = child.get<picojson::object>();

            auto dataIt = childObj.find("data");
            if (dataIt != childObj.end() && dataIt->second.is<picojson::object>())
            {
                const picojson::object& dataObj = dataIt->second.get<picojson::object>();
                auto meshIt = dataObj.find("mesh");
                if (meshIt != dataObj.end() && meshIt->second.is<picojson::object>())
                {
                    const picojson::object& meshObj = meshIt->second.get<picojson::object>();
                    MeshNodeInfo info;
                    info.nodeId = getString(childObj, "id");
                    info.meshFileIndex = (int)getDouble(meshObj, "name", -1.0);
                    info.vertexCount = (uint32_t)getDouble(meshObj, "vertex");
                    info.faceCount = (uint32_t)getDouble(meshObj, "face");
                    info.bbox = parseBoundingBox(childObj);

                    // Optional paired BVH
                    auto bvhIt = dataObj.find("bvh");
                    if (bvhIt != dataObj.end() && bvhIt->second.is<picojson::object>())
                    {
                        const picojson::object& bvhObj = bvhIt->second.get<picojson::object>();
                        info.bvhFileIndex = (int)getDouble(bvhObj, "name", -1.0);
                    }
                    result.push_back(info);
                }
            }
            collectMeshNodes(child, result);
        }
    }
#endif

    // Load raw .btree file into memory; return empty vector on failure
    static std::vector<uint8_t> loadBtreeRaw(const std::string& btreePath)
    {
        std::vector<uint8_t> result;
        std::ifstream file(btreePath.c_str(), std::ios::binary | std::ios::ate);
        if (!file)
        {
            OSG_WARN << "[ReaderWriter3DGS] Cannot open btree: " << btreePath << std::endl;
            return result;
        }

        std::streamsize size = file.tellg(); file.seekg(0, std::ios::beg);
        if (size <= 0 || size % BTREE_NODE_SIZE != 0)
        {
            OSG_WARN << "[ReaderWriter3DGS] Invalid btree file size: " << btreePath
                     << " (" << size << " bytes, not multiple of " << BTREE_NODE_SIZE << ")" << std::endl;
            return result;
        }

        result.resize(size);
        if (!file.read((char*)result.data(), size))
        {
            OSG_WARN << "[ReaderWriter3DGS] Failed to read btree: " << btreePath << std::endl;
            result.clear();
        }
        return result;
    }

    // =============================================================================
    // BvhUserData: osg::Object wrapper for btree BVH data, attachable to any Node
    // =============================================================================
    class BvhUserData : public osg::Object
    {
    public:
        BvhUserData() : osg::Object() {}
        BvhUserData(const BvhUserData& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Object(copy, copyop), _rawData(copy._rawData) {}
        META_Object(osgVerse, BvhUserData);

        void setRawData(const std::vector<uint8_t>& data) { _rawData = data; }
        const std::vector<uint8_t>& getRawData() const { return _rawData; }
        bool valid() const { return !_rawData.empty() && (_rawData.size() % BTREE_NODE_SIZE == 0); }
        uint32_t getNodeCount() const { return valid() ? (uint32_t)_rawData.size() / BTREE_NODE_SIZE : 0; }
        uint32_t getDataSize() const { return (uint32_t)_rawData.size(); }

        // Access a node at a given byte offset (no bounds check in release)
        BtreeNode getNode(uint32_t byteOffset) const
        {
            if (!valid() || byteOffset + BTREE_NODE_SIZE > _rawData.size()) return BtreeNode();
            return parseBtreeNode(_rawData.data(), byteOffset);
        }

        // Convenience: get root node (always at byte offset 0)
        BtreeNode getRoot() const { return getNode(0); }

    protected:
        std::vector<uint8_t> _rawData;
    };

    // ============================================================================
    // LCC2 tree data structures: preserves the complete octree hierarchy
    // ============================================================================
    struct Lcc2NodeData3dgs
    {
        int name = -1;   // index into splatFiles array; -1 = absent
        int start = 0;   // start row within the chunk file
        int count = -1;  // splat count for this node; -1 = unknown
        bool valid() const { return name >= 0; }
    };

    struct Lcc2NodeDataMesh
    {
        int name = -1;            // index into meshFiles array; -1 = absent
        uint32_t vertexCount = 0;
        uint32_t faceCount = 0;
        bool valid() const { return name >= 0; }
    };

    struct Lcc2NodeDataBvh
    {
        int name = -1;   // index into bvhFiles array; -1 = absent
        bool valid() const { return name >= 0; }
    };

    struct Lcc2NodeDataEnv
    {
        int name = -1;   // index into splatFiles (env chunk); -1 = absent
        bool valid() const { return name >= 0; }
    };

    struct Lcc2TreeNode
    {
        std::string id;                    // node id, e.g. "0_7_0_0"
        osg::BoundingBoxd bbox;            // node bounding box (from meta)
        int childNum = 0;                  // number of child slots (may be 0)
        bool hasData = false;              // whether this node has a "data" block

        Lcc2NodeData3dgs d3dgs;            // data['3dgs']  (Gaussian splat chunk)
        Lcc2NodeDataMesh mesh;             // data['mesh']  (triangle mesh PLY)
        Lcc2NodeDataBvh bvh;               // data['bvh']   (BVH btree)
        Lcc2NodeDataEnv env;               // data['env']   (only on root node)
        std::vector<Lcc2TreeNode> children; // child nodes (empty if leaf)

        void print(std::ostream& out, int indent) const
        {
            out << std::string(indent, ' ') << id << ": child = " << childNum << ", data = " << hasData;
            if (hasData && d3dgs.valid())
                out << ", 3dgs = " << d3dgs.name << "/" << d3dgs.start << "/" << d3dgs.count;
            if (mesh.valid()) out << ", mesh = " << mesh.name;
            if (bvh.valid()) out << ", bvh = " << bvh.name;
            if (env.valid()) out << ", env = " << env.name; out << "\n";
            for (size_t i = 0; i < children.size(); ++i)
                children[i].print(out, indent + 2);
        }

        // Helpers
        bool isLeaf() const { return children.empty(); }
        int depth() const;   // computed from id (e.g. "0_7_0_0" -> depth=4)
        int lodLevel(int totalLevels) const { return totalLevels - depth(); }
    };

    // Depth = number of underscore-separated segments in id, e.g. "0"=0, "0_7"=1, "0_7_0_0"=3
    inline int Lcc2TreeNode::depth() const
    {
        int d = 0; if (id.empty()) return 0;
        for (size_t i = 0; i < id.size(); ++i) if (id[i] == '_') ++d; return d;
    }

    struct Lcc2Tree
    {
        Lcc2TreeNode root;                          // octree root
        std::vector<std::string> splatFiles;        // index -> relative path
        std::vector<std::string> meshFiles;         // index -> relative path
        std::vector<std::string> bvhFiles;          // index -> relative path
        int totalLevels = 0;                        // total LOD level count
        int totalSplats = 0;                        // total splat count
        std::string splatType = ".sog";             // ".sog" or ".spz"
        std::string name;                           // scene name
        std::string description;
        osg::BoundingBoxd worldBox;                 // scene bounding box (from meta.boundingBox)
        osg::Vec3d offset, shift, scale;            // scene transform params
        int epsg = 0;

        std::string resolveSplatPath(int index, const std::string& baseDir) const
        {
            if (index < 0 || index >= (int)splatFiles.size()) return "";
            return baseDir.empty() ? splatFiles[index] : (baseDir + "/" + splatFiles[index]);
        }

        std::string resolveMeshPath(int index, const std::string& baseDir) const
        {
            if (index < 0 || index >= (int)meshFiles.size()) return "";
            return baseDir.empty() ? meshFiles[index] : (baseDir + "/" + meshFiles[index]);
        }

        std::string resolveBvhPath(int index, const std::string& baseDir) const
        {
            if (index < 0 || index >= (int)bvhFiles.size()) return "";
            return baseDir.empty() ? bvhFiles[index] : (baseDir + "/" + bvhFiles[index]);
        }

        // Get the env splat file index from root.data.env (convenience)
        int envFileIndex() const { return root.env.valid() ? root.env.name : -1; }
    };

    static Lcc2TreeNode parseLcc2TreeNode(const picojson::value& nodeValue)
    {
        Lcc2TreeNode node;
        if (!nodeValue.is<picojson::object>()) return node;

        const picojson::object& obj = nodeValue.get<picojson::object>();
        node.id = getString(obj, "id");
        node.bbox = parseBoundingBox(obj);
        node.childNum = (int)getDouble(obj, "childNum", 0.0);

        // Parse data block
        auto dataIt = obj.find("data");
        if (dataIt != obj.end() && dataIt->second.is<picojson::object>())
        {
            node.hasData = true;
            const picojson::object& dataObj = dataIt->second.get<picojson::object>();

            // 3dgs
            auto d3dgsIt = dataObj.find("3dgs");
            if (d3dgsIt != dataObj.end() && d3dgsIt->second.is<picojson::object>())
            {
                const picojson::object& d3dgsObj = d3dgsIt->second.get<picojson::object>();
                node.d3dgs.name = (int)getDouble(d3dgsObj, "name", -1.0);
                node.d3dgs.start = (int)getDouble(d3dgsObj, "start", 0.0);
                node.d3dgs.count = (int)getDouble(d3dgsObj, "count", -1.0);
            }

            // mesh
            auto meshIt = dataObj.find("mesh");
            if (meshIt != dataObj.end() && meshIt->second.is<picojson::object>())
            {
                const picojson::object& meshObj = meshIt->second.get<picojson::object>();
                node.mesh.name = (int)getDouble(meshObj, "name", -1.0);
                node.mesh.vertexCount = (uint32_t)getDouble(meshObj, "vertex", 0.0);
                node.mesh.faceCount = (uint32_t)getDouble(meshObj, "face", 0.0);
            }

            // bvh
            auto bvhIt = dataObj.find("bvh");
            if (bvhIt != dataObj.end() && bvhIt->second.is<picojson::object>())
            {
                const picojson::object& bvhObj = bvhIt->second.get<picojson::object>();
                node.bvh.name = (int)getDouble(bvhObj, "name", -1.0);
            }

            // env (typically only on the root node)
            auto envIt = dataObj.find("env");
            if (envIt != dataObj.end() && envIt->second.is<picojson::object>())
            {
                const picojson::object& envObj = envIt->second.get<picojson::object>();
                node.env.name = (int)getDouble(envObj, "name", -1.0);
            }
        }

        // Recurse into children
        picojson::array childArr = getOctreeChildren(nodeValue);
        for (size_t i = 0; i < childArr.size(); ++i)
        {
            Lcc2TreeNode child = parseLcc2TreeNode(childArr[i]);
            // Keep the child even if it has no id (some nodes may be sparse)
            node.children.push_back(child);
        }
        return node;
    }

    // Parse the full meta.lcc2 into an Lcc2Tree (preserves hierarchy)
    static Lcc2Tree parseLcc2Tree(const picojson::object& metaObj)
    {
        Lcc2Tree tree;
        tree.totalLevels = (int)getDouble(metaObj, "totalLevels", 0);
        tree.totalSplats = (int)getDouble(metaObj, "totalSplats", 0);
        tree.splatType = getString(metaObj, "splatType", ".sog");
        tree.name = getString(metaObj, "name", "LCC2");
        tree.description = getString(metaObj, "description");
        tree.worldBox = parseBoundingBox(metaObj);
        tree.epsg = (int)getDouble(metaObj, "epsg", 0.0);

        // offset / shift / scale arrays
        picojson::array ofArr = getArray(metaObj, "offset");
        if (ofArr.size() >= 3) tree.offset.set(ofArr[0].get<double>(), ofArr[1].get<double>(), ofArr[2].get<double>());
        picojson::array shArr = getArray(metaObj, "shift");
        if (shArr.size() >= 3) tree.shift.set(shArr[0].get<double>(), shArr[1].get<double>(), shArr[2].get<double>());
        picojson::array scArr = getArray(metaObj, "scale");
        if (scArr.size() >= 3) tree.scale.set(scArr[0].get<double>(), scArr[1].get<double>(), scArr[2].get<double>());

        // Parse root
        auto rootIt = metaObj.find("root");
        if (rootIt != metaObj.end() && rootIt->second.is<picojson::object>())
        {
            const picojson::object& rootObj = rootIt->second.get<picojson::object>();
            picojson::array sf = getArray(rootObj, "splatFiles");
            tree.splatFiles.reserve(sf.size());
            for (size_t i = 0; i < sf.size(); ++i)
                if (sf[i].is<std::string>()) tree.splatFiles.push_back(sf[i].get<std::string>());

            picojson::array mf = getArray(rootObj, "meshFiles");
            tree.meshFiles.reserve(mf.size());
            for (size_t i = 0; i < mf.size(); ++i)
                if (mf[i].is<std::string>()) tree.meshFiles.push_back(mf[i].get<std::string>());

            picojson::array bf = getArray(rootObj, "bvhFiles");
            tree.bvhFiles.reserve(bf.size());
            for (size_t i = 0; i < bf.size(); ++i)
                if (bf[i].is<std::string>()) tree.bvhFiles.push_back(bf[i].get<std::string>());
            tree.root = parseLcc2TreeNode(rootIt->second);
        }
        return tree;
    }
}  // namespace

// =============================================================================
// Main entry: load LCC2 scene (splats + optional mesh/BVH + optional env)
// =============================================================================
osg::ref_ptr<osg::Node> loadSplatFromXGrids2(std::istream& in, const std::string& path, bool loadMeshes,
                                             osgVerse::GaussianGeometry::RenderMethod method)
{
    // 1. Read and parse meta.lcc2 JSON (with trailing-comma tolerance)
    std::string metaText((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    picojson::value document;
    std::string err = picojson::parse(document, metaText);
    if (!err.empty())
    {
        std::string fixed = stripTrailingCommas(metaText);
        err = picojson::parse(document, fixed);
        if (!err.empty())
        {
            OSG_WARN << "[ReaderWriter3DGS] Failed to parse XGrids' LCC2 meta: " << err << std::endl;
            return NULL;
        }
    }

    if (!document.is<picojson::object>())
    {
        OSG_WARN << "[ReaderWriter3DGS] Invalid LCC2 meta: root is not an object" << std::endl;
        return NULL;
    }
    picojson::object metaObj = document.get<picojson::object>();
    osg::ref_ptr<osgDB::Options> options = new osgDB::Options("RenderMethod=" + std::to_string((int)method));

    // 2. Old protocol compatibility branch
    bool isOldProtocol = (metaObj.find("total_splats") != metaObj.end() &&
                          metaObj.find("lod_3dgs_info") != metaObj.end() &&
                          metaObj.find("lod_level") != metaObj.end());
    if (isOldProtocol)
    {
        metaObj["totalSplats"] = metaObj["total_splats"];
        metaObj.erase("total_splats");

        picojson::array lodInfo = getArray(metaObj, "lod_3dgs_info");
        picojson::array lodSplats;
        for (auto it = lodInfo.rbegin(); it != lodInfo.rend(); ++it) lodSplats.push_back(*it);
        metaObj["lodSplats"] = picojson::value(lodSplats);
        metaObj.erase("lod_3dgs_info");

        metaObj["totalLevels"] = metaObj["lod_level"];
        metaObj.erase("lod_level");

        auto rootIt = metaObj.find("root");
        if (rootIt != metaObj.end() && rootIt->second.is<picojson::object>())
        {
            picojson::object rootObj = rootIt->second.get<picojson::object>();
            picojson::array oldFiles = getArray(rootObj, "files");
            picojson::array splatFiles;
            for (size_t i = 0; i < oldFiles.size(); ++i)
            {
                if (!oldFiles[i].is<std::string>()) continue;
                std::string f = oldFiles[i].get<std::string>();
                if (!f.empty() && f[0] == '/') f = f.substr(1);
                if (f.size() < 4 || f.substr(f.size() - 4) != ".sog") f += ".sog";
                splatFiles.push_back(picojson::value(f));
            }
            rootObj["splatFiles"] = picojson::value(splatFiles);
            normalizeOctreeNode(rootIt->second);
            metaObj["root"] = picojson::value(rootObj);
        }
    }

    Lcc2Tree tree; tree = parseLcc2Tree(metaObj);
    if (tree.splatFiles.empty() || tree.root.id.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Invalid LCC2 scene graph" << std::endl;
        return NULL;
    }
    else
        tree.root.print(std::cout, 0);

#if true
    // 3. Extract normalized fields
    int totalLevels = (int)getDouble(metaObj, "totalLevels", 0);
    int totalSplats = (int)getDouble(metaObj, "totalSplats", 0);
    if (totalLevels <= 0)
    {
        OSG_WARN << "[ReaderWriter3DGS] Invalid LCC2 meta: totalLevels = " << totalLevels << std::endl;
        return NULL;
    }

    std::string splatType = getString(metaObj, "splatType", ".sog");
    if (splatType.empty()) splatType = ".sog";

    auto rootIt = metaObj.find("root");
    if (rootIt == metaObj.end() || !rootIt->second.is<picojson::object>())
    {
        OSG_WARN << "[ReaderWriter3DGS] Invalid XGrids' LCC2 meta: missing root" << std::endl;
        return NULL;
    }

    picojson::object rootObj = rootIt->second.get<picojson::object>();
    picojson::array splatFiles = getArray(rootObj, "splatFiles");
    if (splatFiles.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Invalid LCC2 meta: root.splatFiles is empty" << std::endl;
        return NULL;
    }

    // Also read meshFiles / bvhFiles (may be empty, that's fine)
    picojson::array meshFiles = getArray(rootObj, "meshFiles");
    picojson::array bvhFiles = getArray(rootObj, "bvhFiles");
    osg::BoundingBoxd worldBox = parseBoundingBox(metaObj);
    
    int envFileIndex = -1;
    auto dataIt = rootObj.find("data");
    if (dataIt != rootObj.end() && dataIt->second.is<picojson::object>())
    {
        const picojson::object& dataObj = dataIt->second.get<picojson::object>();
        auto envIt = dataObj.find("env");
        if (envIt != dataObj.end() && envIt->second.is<picojson::object>())
        {
            const picojson::object& envObj = envIt->second.get<picojson::object>();
            auto envNameIt = envObj.find("name");
            if (envNameIt != envObj.end() && envNameIt->second.is<double>())
                envFileIndex = (int)envNameIt->second.get<double>();
        }
    }

    // 4. Resolve LOD selection
    std::vector<int> lodSelect;
    std::vector<int> inputLods = resolveLodSelection(lodSelect, totalLevels);
    if (inputLods.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] No valid LODs selected for LCC2: " << path << std::endl;
        return NULL;
    }

    // 5. Collect chunk files by LOD level
    std::map<int, std::vector<std::pair<int, int>>> chunksByLevel;
    collectChunksByLevel(rootIt->second, 1, totalLevels, envFileIndex, chunksByLevel);

    struct ChunkTask { int outputLod; int fileIndex; int count; std::string filePath; };
    std::vector<ChunkTask> tasks;
    for (size_t oi = 0; oi < inputLods.size(); ++oi)
    {
        int inputLod = inputLods[oi];
        auto it = chunksByLevel.find(inputLod);
        if (it == chunksByLevel.end()) continue;

        std::map<int, int> fileToCount;
        for (size_t j = 0; j < it->second.size(); ++j)
        {
            int fidx = it->second[j].first, cnt = it->second[j].second;
            auto fit = fileToCount.find(fidx);
            if (fit == fileToCount.end()) fileToCount[fidx] = cnt;
            else if (fit->second >= 0 && cnt >= 0) fit->second += cnt;
            else fit->second = -1;
        }

        for (auto fit = fileToCount.begin(); fit != fileToCount.end(); ++fit)
        {
            int fidx = fit->first;
            if (fidx < 0 || fidx >= (int)splatFiles.size() || !splatFiles[fidx].is<std::string>())
            {
                OSG_WARN << "[ReaderWriter3DGS] Invalid chunk file index " << fidx
                         << " (splatFiles has " << splatFiles.size() << " entries)" << std::endl;
                continue;
            }
            std::string fileName = splatFiles[fidx].get<std::string>();
            std::string fullPath = path.empty() ? fileName : (path + "/" + fileName);
            tasks.push_back({ (int)oi, fidx, fit->second, fullPath });
        }
    }

    if (tasks.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] No chunks found for selected LODs in LCC2: " << path << std::endl;
        return NULL;
    }

    // 6. Decode splat chunks
    std::map<int, std::vector<osg::ref_ptr<osg::Node>>> nodesByOutputLod;
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        const ChunkTask& task = tasks[i];
        std::string filePath = task.filePath;
        if (!osgDB::fileExists(filePath))
        {
            std::string baseName = osgDB::getSimpleFileName(filePath);
            if (baseName != filePath)
            {
                std::string altPath = path.empty() ? baseName : (path + "/" + baseName);
                if (osgDB::fileExists(altPath)) filePath = altPath;
            }
        }

        osg::ref_ptr<osg::Node> chunkNode = osgDB::readNodeFile(filePath + ".verse_3dgs", options.get());
        if (!chunkNode)
        {
            OSG_WARN << "[ReaderWriter3DGS] Failed to load LCC2 chunk: " << filePath << std::endl;
            continue;
        }
        chunkNode->setName(filePath);
        nodesByOutputLod[task.outputLod].push_back(chunkNode);
    }

    if (nodesByOutputLod.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] All chunks failed to decode for LCC2: " << path << std::endl;
        return NULL;
    }

    // 7. Build scene graph root + LOD
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName(getString(metaObj, "name", "LCC2"));
    std::string description = getString(metaObj, "description");
    if (!description.empty()) root->addDescription(description);

    osg::BoundingBox totalBB;
    for (auto it = nodesByOutputLod.begin(); it != nodesByOutputLod.end(); ++it)
        for (size_t j = 0; j < it->second.size(); ++j)
            if (it->second[j].valid()) totalBB.expandBy(it->second[j]->getBound());
    if (!totalBB.valid() && worldBox.valid()) { totalBB._min = worldBox._min; totalBB._max = worldBox._max; }

    float sceneRadius = totalBB.valid() ? totalBB.radius() : 100.0f;
    if (nodesByOutputLod.size() == 1)
    {
        auto it = nodesByOutputLod.begin();
        osg::ref_ptr<osg::Group> lodGroup = new osg::Group;
        lodGroup->setName("LOD_" + std::to_string(it->first));
        for (size_t j = 0; j < it->second.size(); ++j) lodGroup->addChild(it->second[j].get());
        root->addChild(lodGroup.get());
    }
    else
    {
        osg::ref_ptr<osg::LOD> lodSwitcher = new osg::LOD;
        lodSwitcher->setName("LODSwitcher");
        lodSwitcher->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        lodSwitcher->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
        if (totalBB.valid()) { lodSwitcher->setCenter(totalBB.center()); lodSwitcher->setRadius(sceneRadius); }

        int maxOutputLod = (int)nodesByOutputLod.size() - 1;
        float baseDist = sceneRadius * 0.3f;

        for (auto it = nodesByOutputLod.begin(); it != nodesByOutputLod.end(); ++it)
        {
            int outputLod = it->first;
            osg::ref_ptr<osg::Group> lodGroup = new osg::Group;
            lodGroup->setName("LOD_" + std::to_string(outputLod));
            for (size_t j = 0; j < it->second.size(); ++j) lodGroup->addChild(it->second[j].get());

            float exponent = (float)(maxOutputLod - outputLod), lodFactor = 1.5f;
            float minDist = (outputLod == maxOutputLod) ? 0.0f : baseDist * std::pow(lodFactor, exponent);
            float maxDist = (outputLod == 0) ? FLT_MAX : baseDist * std::pow(lodFactor, exponent + 1.0f);

            std::cout << lodSwitcher->getName() << " / " << outputLod << ": " << minDist << ", " << maxDist << "\n";
            for (size_t j = 0; j < it->second.size(); ++j) std::cout << "   " << it->second[j]->getName() << "\n";

            lodSwitcher->addChild(lodGroup.get(), minDist, maxDist);
        }
        root->addChild(lodSwitcher.get());
    }

    // 8. Optional mesh + BVH loading
    // Collect mesh nodes from octree, then load each PLY + paired .btree
    if (loadMeshes)
    {
        std::vector<MeshNodeInfo> meshNodes; collectMeshNodes(rootIt->second, meshNodes);
        if (!meshNodes.empty() && !meshFiles.empty())
        {
            osg::ref_ptr<osg::Group> meshGroup = new osg::Group;
            meshGroup->setName("Meshes");
            meshGroup->setNodeMask(0);  // not renderable, collision / raycast use only
            meshGroup->setUserValue("Collision", true);

            for (size_t i = 0; i < meshNodes.size(); ++i)
            {
                const MeshNodeInfo& info = meshNodes[i];
                if (info.meshFileIndex < 0 || info.meshFileIndex >= (int)meshFiles.size() ||
                    !meshFiles[info.meshFileIndex].is<std::string>())
                {
                    OSG_WARN << "[ReaderWriter3DGS] Invalid mesh file index " << info.meshFileIndex
                            << " for node " << info.nodeId << std::endl;
                    continue;
                }

                std::string meshFileName = meshFiles[info.meshFileIndex].get<std::string>();
                std::string meshPath = path.empty() ? meshFileName : (path + "/" + meshFileName);
                if (!osgDB::fileExists(meshPath))
                {
                    std::string baseName = osgDB::getSimpleFileName(meshPath);
                    if (baseName != meshPath)
                    {
                        std::string altPath = path.empty() ? baseName : (path + "/" + baseName);
                        if (osgDB::fileExists(altPath)) meshPath = altPath;
                    }
                }

                osg::ref_ptr<osg::Node> meshNode = osgDB::readNodeFile(meshPath + ".verse_mesh");
                if (!meshNode)
                {
                    OSG_WARN << "[ReaderWriter3DGS] Failed to load mesh PLY: " << meshPath << std::endl;
                    continue;
                }
                meshNode->setName(info.nodeId + "_mesh");
                meshNode->addDescription("LCC2_Mesh_Vertices " + std::to_string(info.vertexCount));
                meshNode->addDescription("LCC2_Mesh_Faces " + std::to_string(info.faceCount));

                // Load paired BVH btree if available
                if (info.bvhFileIndex >= 0 && info.bvhFileIndex < (int)bvhFiles.size() &&
                    bvhFiles[info.bvhFileIndex].is<std::string>())
                {
                    std::string bvhFileName = bvhFiles[info.bvhFileIndex].get<std::string>();
                    std::string bvhPath = path.empty() ? bvhFileName : (path + "/" + bvhFileName);
                    if (!osgDB::fileExists(bvhPath))
                    {
                        std::string baseName = osgDB::getSimpleFileName(bvhPath);
                        if (baseName != bvhPath)
                        {
                            std::string altPath = path.empty() ? baseName : (path + "/" + baseName);
                            if (osgDB::fileExists(altPath)) bvhPath = altPath;
                        }
                    }

                    std::vector<uint8_t> btreeData = loadBtreeRaw(bvhPath);
                    if (!btreeData.empty())
                    {
                        osg::ref_ptr<BvhUserData> bvhObj = new BvhUserData;
                        bvhObj->setRawData(btreeData);
                        meshNode->setUserData(bvhObj.get());
                        meshNode->setUserValue("LCC2_BVH", true);
                        meshNode->setUserValue("LCC2_BVH_Nodes", (int)bvhObj->getNodeCount());
                    }
                    else
                        { OSG_NOTICE << "[ReaderWriter3DGS] Failed to load BVH btree: " << bvhPath << std::endl; }
                }

                // If the node has a bounding box, also set it as the node's initial bound
                if (info.bbox.valid())
                {
                    osg::BoundingBoxf bb(osg::Vec3f(info.bbox.xMin(), info.bbox.yMin(), info.bbox.zMin()),
                                        osg::Vec3f(info.bbox.xMax(), info.bbox.yMax(), info.bbox.zMax()));
                    meshNode->setInitialBound(bb);
                }
                meshGroup->addChild(meshNode.get());
            }
            if (meshGroup->getNumChildren() > 0) root->addChild(meshGroup.get());
        }
    }
    
    // 9. Optional environment chunk (lod = -1)  // FIXME: disabled because of confusing sorting
    /*if (envFileIndex >= 0 && envFileIndex < (int)splatFiles.size() &&
          splatFiles[envFileIndex].is<std::string>())
    {
        std::string envFileName = splatFiles[envFileIndex].get<std::string>();
        std::string envPath = path.empty() ? envFileName : (path + "/" + envFileName);
        if (!osgDB::fileExists(envPath))
        {
            std::string baseName = osgDB::getSimpleFileName(envPath);
            if (baseName != envPath)
            {
                std::string altPath = path.empty() ? baseName : (path + "/" + baseName);
                if (osgDB::fileExists(altPath)) envPath = altPath;
            }
        }
        if (osgDB::fileExists(envPath))
        {
            osg::ref_ptr<osg::Node> envNode = osgDB::readNodeFile(envPath + ".verse_3dgs");
            if (envNode.valid())
            {
                envNode->setName("Environment");
                envNode->setUserValue("LCC2_Environment", true);
                root->addChild(envNode.get());
            }
            else
            {
                OSG_NOTICE << "[ReaderWriter3DGS] Failed to load LCC2 environment chunk: " << envPath << std::endl;
            }
        }
    }*/
#endif
    return root;
}
