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

    // Compute scene bounding box from a list of nodes
    static osg::BoundingBox computeNodesBounds(const std::vector<osg::ref_ptr<osg::Node>>& nodes)
    {
        osg::BoundingBox bb;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].valid()) bb.expandBy(nodes[i]->getBound());
        }
        return bb;
    }

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
}

osg::ref_ptr<osg::Node> loadSplatFromXGrids2(
    std::istream& in, const std::string& path,
    osgVerse::GaussianGeometry::RenderMethod method)
{
    // 1. Read and parse meta.lcc2 JSON (with trailing-comma tolerance)
    std::string metaText((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    picojson::value document;
    std::string err = picojson::parse(document, metaText);
    if (!err.empty())
    {
        // Retry with trailing commas stripped
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
        OSG_WARN << "[ReaderWriter3DGS] Invalid XGrids' LCC2 meta: root is not an object" << std::endl;
        return NULL;
    }
    picojson::object metaObj = document.get<picojson::object>();

    // 2. Old protocol compatibility branch
    // Old protocol uses total_splats / lod_3dgs_info / lod_level + files / child_num / datatype
    bool isOldProtocol = (metaObj.find("total_splats") != metaObj.end() &&
                          metaObj.find("lod_3dgs_info") != metaObj.end() &&
                          metaObj.find("lod_level") != metaObj.end());
    if (isOldProtocol)
    {
        metaObj["totalSplats"] = metaObj["total_splats"];
        metaObj.erase("total_splats");

        picojson::array lodInfo = getArray(metaObj, "lod_3dgs_info");
        picojson::array lodSplats;
        for (auto it = lodInfo.rbegin(); it != lodInfo.rend(); ++it)
            lodSplats.push_back(*it);
        metaObj["lodSplats"] = picojson::value(lodSplats);
        metaObj.erase("lod_3dgs_info");

        metaObj["totalLevels"] = metaObj["lod_level"];
        metaObj.erase("lod_level");

        // root.files -> root.splatFiles: drop leading '/', append '.sog'
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

    // 3. Extract normalized fields
    int totalLevels = (int)getDouble(metaObj, "totalLevels", 0);
    int totalSplats = (int)getDouble(metaObj, "totalSplats", 0);
    if (totalLevels <= 0)
    {
        OSG_WARN << "[ReaderWriter3DGS] Invalid XGrids' LCC2 meta: totalLevels = " << totalLevels << std::endl;
        return NULL;
    }

    // splatType: '.sog' or '.spz', default to '.sog'
    std::string splatType = getString(metaObj, "splatType", ".sog");
    if (splatType.empty()) splatType = ".sog";

    // splatFiles array
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
        OSG_WARN << "[ReaderWriter3DGS] Invalid XGrids' LCC2 meta: root.splatFiles is empty" << std::endl;
        return NULL;
    }

    // Bounding box
    osg::BoundingBoxd worldBox = parseBoundingBox(metaObj);

    // Environment file index detection (root.data.env.name)
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

    // 4. Resolve LOD selection (default: all levels)
    // TODO: support external lodSelect parameter if needed
    std::vector<int> lodSelect;  // empty = all
    std::vector<int> inputLods = resolveLodSelection(lodSelect, totalLevels);
    if (inputLods.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] No valid LODs selected for LCC2: " << path << std::endl;
        return NULL;
    }

    // 5. Collect chunk files by LOD level via octree traversal
    std::map<int, std::vector<std::pair<int, int>>> chunksByLevel;
    collectChunksByLevel(rootIt->second, 1, totalLevels, envFileIndex, chunksByLevel);

    // Build ordered decode tasks: by selected LOD order, then by ascending file index
    struct ChunkTask { int outputLod; int fileIndex; int count; std::string filePath; };
    std::vector<ChunkTask> tasks;
    for (size_t oi = 0; oi < inputLods.size(); ++oi)
    {
        int inputLod = inputLods[oi];
        auto it = chunksByLevel.find(inputLod);
        if (it == chunksByLevel.end()) continue;

        // Deduplicate by file index (same file may be referenced by multiple nodes)
        // Keep the first count; if any contributor lacks count, mark as unknown (-1)
        std::map<int, int> fileToCount;
        for (size_t j = 0; j < it->second.size(); ++j)
        {
            int fidx = it->second[j].first;
            int cnt = it->second[j].second;
            auto fit = fileToCount.find(fidx);
            if (fit == fileToCount.end())
                fileToCount[fidx] = cnt;
            else if (fit->second >= 0 && cnt >= 0)
                fit->second += cnt;
            else
                fit->second = -1;  // unknown if any contributor lacks count
        }

        for (auto fit = fileToCount.begin(); fit != fileToCount.end(); ++fit)
        {
            int fidx = fit->first;
            if (fidx < 0 || fidx >= (int)splatFiles.size() || !splatFiles[fidx].is<std::string>())
            {
                OSG_WARN << "[ReaderWriter3DGS] Invalid chunk file index " << fidx
                         << " in LCC2 (splatFiles has " << splatFiles.size() << " entries)" << std::endl;
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

    // 6. Decode chunks using osgDB::readNodeFile (reuses existing .sog/.spz ReaderWriter)
    // Organize by output LOD level
    std::map<int, std::vector<osg::ref_ptr<osg::Node>>> nodesByOutputLod;
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        const ChunkTask& task = tasks[i];
        if (!osgDB::fileExists(task.filePath))
        {
            // Try basename fallback (e.g. drag-and-drop without directories)
            std::string baseName = osgDB::getSimpleFileName(task.filePath);
            if (baseName != task.filePath)
            {
                std::string altPath = path.empty() ? baseName : (path + "/" + baseName);
                if (osgDB::fileExists(altPath))
                {
                    const_cast<ChunkTask&>(task).filePath = altPath;
                }
            }
        }

        osg::ref_ptr<osg::Node> chunkNode = osgDB::readNodeFile(task.filePath + ".verse_3dgs");
        if (!chunkNode)
        {
            OSG_WARN << "[ReaderWriter3DGS] Failed to load LCC2 chunk: " << task.filePath << std::endl;
            continue;
        }
        nodesByOutputLod[task.outputLod].push_back(chunkNode);
    }

    if (nodesByOutputLod.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] All chunks failed to decode for LCC2: " << path << std::endl;
        return NULL;
    }

    // 7. Build scene graph: MatrixTransform root -> LOD levels
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName(getString(metaObj, "name", "LCC2"));

    std::string description = getString(metaObj, "description");
    if (!description.empty()) root->addDescription(description);

    // Apply LCC2 coordinate transform (Y-up to engine, same as LCC v1)
    // fromEulers(90, 0, 180) -> rotate 90 around X then 180 around Z
    osg::Matrix coordMatrix;
    coordMatrix.makeRotate(osg::DegreesToRadians(90.0), osg::X_AXIS, 0.0, osg::Y_AXIS,
                           osg::DegreesToRadians(180.0), osg::Z_AXIS);
    //root->setMatrix(coordMatrix);

    // Compute overall bounding box for LOD distance setup
    osg::BoundingBox totalBB;
    for (auto it = nodesByOutputLod.begin(); it != nodesByOutputLod.end(); ++it)
    {
        for (size_t j = 0; j < it->second.size(); ++j)
            if (it->second[j].valid()) totalBB.expandBy(it->second[j]->getBound());
    }
    if (!totalBB.valid() && worldBox.valid())
    {
        totalBB._min = worldBox._min;
        totalBB._max = worldBox._max;
    }

    // If only one LOD level, add children directly to root
    float sceneRadius = totalBB.valid() ? totalBB.radius() : 100.0f;
    if (nodesByOutputLod.size() == 1)
    {
        auto it = nodesByOutputLod.begin();
        osg::ref_ptr<osg::Group> lodGroup = new osg::Group;
        lodGroup->setName("LOD_" + std::to_string(it->first));
        for (size_t j = 0; j < it->second.size(); ++j)
            lodGroup->addChild(it->second[j].get());
        root->addChild(lodGroup.get());
    }
    else
    {
        // Multi-LOD: use osg::LOD for distance-based switching
        // Higher outputLod index = higher detail = closer range
        osg::ref_ptr<osg::LOD> lodSwitcher = new osg::LOD;
        lodSwitcher->setName("LODSwitcher");
        lodSwitcher->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        lodSwitcher->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
        if (totalBB.valid())
        {
            lodSwitcher->setCenter(totalBB.center());
            lodSwitcher->setRadius(sceneRadius);
        }

        int maxOutputLod = (int)nodesByOutputLod.size() - 1;
        float baseDist = sceneRadius * 0.3f;
        for (auto it = nodesByOutputLod.begin(); it != nodesByOutputLod.end(); ++it)
        {
            int outputLod = it->first;
            osg::ref_ptr<osg::Group> lodGroup = new osg::Group;
            lodGroup->setName("LOD_" + std::to_string(outputLod));
            for (size_t j = 0; j < it->second.size(); ++j)
                lodGroup->addChild(it->second[j].get());

            // LOD distance: lowest detail at farthest range, highest detail at closest
            float minDist = (outputLod == maxOutputLod) ? 0.0f : baseDist * std::pow(1.5f, outputLod);
            float maxDist = (outputLod == 0) ? FLT_MAX : baseDist * std::pow(1.5f, outputLod + 1);
            lodSwitcher->addChild(lodGroup.get(), minDist, maxDist);
        }
        root->addChild(lodSwitcher.get());
    }

    // 8. Optional environment chunk (lod = -1)  // FIXME: disabled because of confusing sorting
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
                OSG_NOTICE << "[ReaderWriter3DGS] Failed to load LCC2 environment chunk: "
                           << envPath << std::endl;
            }
        }
    }*/
    return root;
}
