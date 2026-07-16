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
        int vertexCount = 0;
        int faceCount = 0;
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

        // Depth = number of underscore-separated segments in id, e.g. "0"=0, "0_7"=1, "0_7_0_0"=3
        int depth() const
        {
            int d = 0; if (id.empty()) return 0;
            for (size_t i = 0; i < id.size(); ++i) if (id[i] == '_') ++d; return d;
        }
        int lodLevel(int totalLevels) const { return totalLevels - depth(); }
        bool isLeaf() const { return children.empty(); }

        // Save sub graph data
        void serialize(std::ostream& fout) const
        {
            int count = (int)id.length(); fout.write((char*)&count, sizeof(int)); fout.write((char*)id.data(), count);
            fout.write((char*)&(bbox._min), sizeof(osg::Vec3d));
            fout.write((char*)&(bbox._max), sizeof(osg::Vec3d)); fout.write((char*)&childNum, sizeof(int));
            fout.write((char*)&(d3dgs.name), sizeof(int));
            fout.write((char*)&(d3dgs.start), sizeof(int)); fout.write((char*)&(d3dgs.count), sizeof(int));
            fout.write((char*)&(mesh.name), sizeof(int));
            fout.write((char*)&(mesh.vertexCount), sizeof(int)); fout.write((char*)&(mesh.faceCount), sizeof(int));
            fout.write((char*)&(bvh.name), sizeof(int));
            for (size_t i = 0; i < childNum; ++i) children[i].serialize(fout);
        }

        // Restore sub graph data
        void deserialize(std::istream& fin)
        {
            int count = 0; fin.read((char*)&count, sizeof(int));
            id.resize(count); fin.read((char*)id.data(), count);
            fin.read((char*)&(bbox._min), sizeof(osg::Vec3d));
            fin.read((char*)&(bbox._max), sizeof(osg::Vec3d));
            fin.read((char*)&childNum, sizeof(int));
            fin.read((char*)&(d3dgs.name), sizeof(int));
            fin.read((char*)&(d3dgs.start), sizeof(int)); fin.read((char*)&(d3dgs.count), sizeof(int));
            fin.read((char*)&(mesh.name), sizeof(int));
            fin.read((char*)&(mesh.vertexCount), sizeof(int)); fin.read((char*)&(mesh.faceCount), sizeof(int));
            fin.read((char*)&(bvh.name), sizeof(int));
            hasData = d3dgs.valid() || mesh.valid() | bvh.valid(); children.resize(childNum);
            for (size_t i = 0; i < childNum; ++i) children[i].deserialize(fin);
        }
    };

    struct Lcc2Tree
    {
        Lcc2TreeNode root;                          // octree root
        std::vector<std::string> splatFiles;        // index -> relative path
        std::vector<std::string> meshFiles;         // index -> relative path
        std::vector<std::string> bvhFiles;          // index -> relative path
        int totalLevels = 0;                        // total LOD level count
        int totalSplats = 0;                        // total splat count
        std::string splatType = ".sog";             // ".sog" or ".spz"
        std::string name, description;              // scene name
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

        // Save splat/mesh/bvh file data
        void serialize(std::ostream& fout) const
        {
            int count = totalLevels; fout.write((char*)&count, sizeof(int));
            size_t numFiles = splatFiles.size(); fout.write((char*)&numFiles, sizeof(size_t));
            numFiles = meshFiles.size(); fout.write((char*)&numFiles, sizeof(size_t));
            numFiles = bvhFiles.size(); fout.write((char*)&numFiles, sizeof(size_t));

            for (size_t i = 0; i < splatFiles.size(); ++i)
            {
                const std::string& s = splatFiles[i]; count = (int)s.length();
                fout.write((char*)&count, sizeof(int)); fout.write(s.data(), count);
            }
            for (size_t i = 0; i < meshFiles.size(); ++i)
            {
                const std::string& s = meshFiles[i]; count = (int)s.length();
                fout.write((char*)&count, sizeof(int)); fout.write(s.data(), count);
            }
            for (size_t i = 0; i < bvhFiles.size(); ++i)
            {
                const std::string& s = bvhFiles[i]; count = (int)s.length();
                fout.write((char*)&count, sizeof(int)); fout.write(s.data(), count);
            }
        }

        // Restore splat/mesh/bvh file data
        void deserialize(std::istream& fin)
        {
            size_t numFiles = 0; int count = 0;
            fin.read((char*)&totalLevels, sizeof(int));
            fin.read((char*)&numFiles, sizeof(size_t)); splatFiles.resize(numFiles);
            fin.read((char*)&numFiles, sizeof(size_t)); meshFiles.resize(numFiles);
            fin.read((char*)&numFiles, sizeof(size_t)); bvhFiles.resize(numFiles);
            for (size_t i = 0; i < splatFiles.size(); ++i)
            {
                std::string& s = splatFiles[i]; fin.read((char*)&count, sizeof(int));
                s.resize(count); if (count > 0) fin.read((char*)s.data(), count);
            }
            for (size_t i = 0; i < meshFiles.size(); ++i)
            {
                std::string& s = meshFiles[i]; fin.read((char*)&count, sizeof(int));
                s.resize(count); if (count > 0) fin.read((char*)s.data(), count);
            }
            for (size_t i = 0; i < bvhFiles.size(); ++i)
            {
                std::string& s = bvhFiles[i]; fin.read((char*)&count, sizeof(int));
                s.resize(count); if (count > 0) fin.read((char*)s.data(), count);
            }
        }
    };

    // Parse a new Lcc2TreeNode recursely
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

    static std::string getGaussianFileName(const std::vector<std::string>& fileList,
                                           int name, const std::string& path)
    {
        if (fileList.size() <= name) return "";
        std::string filePath = fileList[name];

        if (!osgDB::fileExists(filePath))
        {
            if (!path.empty())
            {
                filePath = path + "/" + filePath;
                if (osgDB::fileExists(filePath)) return filePath;
            }
            
            std::string baseName = osgDB::getSimpleFileName(filePath);
            if (baseName != filePath)
            {
                std::string altPath = path.empty() ? baseName : (path + "/" + baseName);
                if (osgDB::fileExists(altPath)) filePath = altPath;
            }
        }
        return filePath;
    }
}  // namespace

// Traverse recursely to constrcut the scene graph
static void traverseLcc2TreeNode(Lcc2TreeNode& lcc2Node, const Lcc2Tree& lcc2Tree,
                                 osg::Group& parent, const std::string& path, const std::string& optString)
{
    if (lcc2Node.d3dgs.valid())
    {
        std::string filePath = getGaussianFileName(lcc2Tree.splatFiles, lcc2Node.d3dgs.name, path);
        std::string opt = optString + " LoadVertexOffset=" + std::to_string(lcc2Node.d3dgs.start)
                        + " LoadVertexCount=" + std::to_string(lcc2Node.d3dgs.count);
        osg::ref_ptr<osg::Node> chunkNode = osgDB::readNodeFile(filePath + ".verse_3dgs", new osgDB::Options(opt));
        if (chunkNode.valid()) { chunkNode->setName(filePath); }
        else { OSG_WARN << "[ReaderWriter3DGS] Failed to load LCC2 chunk: " << filePath << std::endl; }
    
        if (!lcc2Node.isLeaf())
        {   //  create PLOD
            osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
            plod->setName(lcc2Node.id); parent.addChild(plod.get());
            plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
            plod->setCenterMode(osg::LOD::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
            plod->setCenter(lcc2Node.bbox.center()); plod->setRadius(lcc2Node.bbox.radius());

            float lodFactor = 1.5f, exponent = osg::maximum(lcc2Tree.totalLevels - lcc2Node.depth(), 0);
            float baseDist = plod->getRadius() * std::pow(lodFactor, exponent);
            plod->addChild(chunkNode.valid() ? chunkNode.get() : new osg::Node, baseDist, FLT_MAX);
            plod->setFileName(1, lcc2Node.id + ".lcc2_node.verse_3dgs");
            plod->setRange(1, 0.0f, baseDist); plod->setDatabasePath(path);

            std::ostringstream nOut(std::ios::out | std::ios::binary); lcc2Node.serialize(nOut);
            std::ostringstream tOut(std::ios::out | std::ios::binary); lcc2Tree.serialize(tOut);
            osg::ref_ptr<osgDB::Options> infoOpt = new osgDB::Options(optString);
            infoOpt->setPluginStringData("Lcc2NodeData", nOut.str());
            infoOpt->setPluginStringData("Lcc2TreeData", tOut.str());
            plod->setDatabaseOptions(infoOpt.get());
        }
        else  // leaf node, directly add data
            parent.addChild(chunkNode.get());
    }
    else
    {   // create normal group
        osg::ref_ptr<osg::Group> group = new osg::Group;
        group->setName(lcc2Node.id); parent.addChild(group.get());
        for (size_t i = 0; i < lcc2Node.children.size(); ++i)
            traverseLcc2TreeNode(lcc2Node.children[i], lcc2Tree, *group, path, optString);
    }

    // Load mesh & bvh files
    if (lcc2Node.mesh.valid())
    {
        osg::ref_ptr<osg::Group> meshGroup = new osg::Group;
        meshGroup->setName(lcc2Node.id + "_mesh");
        meshGroup->setNodeMask(0);  // not renderable, collision / raycast use only
        meshGroup->setUserValue("Collision", true);

        std::string filePath = getGaussianFileName(lcc2Tree.meshFiles, lcc2Node.mesh.name, path);
        osg::ref_ptr<osg::Node> meshNode = osgDB::readNodeFile(filePath + ".verse_mesh");
        if (!meshNode)
        {
            OSG_WARN << "[ReaderWriter3DGS] Failed to load mesh PLY: " << filePath << std::endl;
            return;  // no need to check BVH data
        }
        meshGroup->setUserValue("LCC2_Mesh_Vertices ", lcc2Node.mesh.vertexCount);
        meshGroup->setUserValue("LCC2_Mesh_Faces ", lcc2Node.mesh.faceCount);
        meshGroup->addChild(meshNode.get());

        // Load paired BVH btree if available
        if (lcc2Node.bvh.valid())
        {
            filePath = getGaussianFileName(lcc2Tree.bvhFiles, lcc2Node.bvh.name, path);
            std::vector<uint8_t> btreeData = loadBtreeRaw(filePath);
            if (!btreeData.empty())
            {
                osg::ref_ptr<BvhUserData> bvhObj = new BvhUserData;
                bvhObj->setRawData(btreeData); meshNode->setUserData(bvhObj.get());
                meshGroup->setUserValue("LCC2_BVH_Nodes", (int)bvhObj->getNodeCount());
            }
            else
                { OSG_NOTICE << "[ReaderWriter3DGS] Failed to load BVH btree: " << filePath << std::endl; }
        }
        parent.addChild(meshGroup.get());
    }
}

osg::ref_ptr<osg::Node> loadSubSplatFromXGrids2(const std::string& file, const osgDB::Options* opt)
{
    if (!opt) { OSG_WARN << "[ReaderWriter3DGS] Invalid LCC2 subgraph: " << file << std::endl; return NULL; }
    std::string nodeData = opt->getPluginStringData("Lcc2NodeData");
    std::string treeData = opt->getPluginStringData("Lcc2TreeData");
    if (nodeData.empty() || treeData.empty())
    { OSG_WARN << "[ReaderWriter3DGS] Invalid LCC2 subgraph: " << file << std::endl; return NULL; }

    std::stringstream nIn(std::ios::in | std::ios::out | std::ios::binary);
    std::stringstream tIn(std::ios::in | std::ios::out | std::ios::binary);
    Lcc2TreeNode lcc2Node; nIn.write(nodeData.data(), nodeData.size()); lcc2Node.deserialize(nIn);
    Lcc2Tree lcc2Tree; tIn.write(treeData.data(), treeData.size()); lcc2Tree.deserialize(tIn);
    std::string path = osgDB::getFilePath(file);

    osg::ref_ptr<osg::Group> group = new osg::Group;
    for (size_t i = 0; i < lcc2Node.childNum; ++i)  // save children directly
        traverseLcc2TreeNode(lcc2Node.children[i], lcc2Tree, *group, path, opt->getOptionString());
    group->setName(lcc2Node.id); return group;
}

osg::ref_ptr<osg::Node> loadSplatFromXGrids2(std::istream& in, const std::string& path,
                                             osgVerse::GaussianGeometry::RenderMethod method)
{
    // 1. Read and parse meta.lcc2 JSON (with trailing-comma tolerance)
    std::string metaText((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    
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
    std::string optString = "RenderMethod=" + std::to_string((int)method);

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

    // 3. Traverse Lcc2TreeNode and construct PagedLOD based scene graph
    Lcc2Tree tree; tree = parseLcc2Tree(metaObj);
    if (tree.splatFiles.empty() || tree.root.id.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Invalid LCC2 scene graph" << std::endl;
        return NULL;
    }
    //else
    //    tree.root.print(std::cout, 0);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName(getString(metaObj, "name", "LCC2"));
    traverseLcc2TreeNode(tree.root, tree, *root, path, optString);

    std::string description = getString(metaObj, "description");
    if (!description.empty()) root->addDescription(description);
    return root;
}
