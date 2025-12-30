#include "OsgbTileOptimizer.h"
#include "DracoProcessor.h"
#include "Utilities.h"
#include "LoadTextureKTX.h"
#include "modeling/GeometryMerger.h"
#include "modeling/Utilities.h"
#include "pipeline/Utilities.h"
#include "nanoid/nanoid.h"

#include <osg/io_utils>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/Simplifier>
#include <vector>
#include <regex>
#include <limits>
#include <iomanip>
using namespace osgVerse;

struct DefaultGpuBaker : public GeometryMerger::GpuBaker
{
    virtual osg::Image* bakeTextureImage(osg::Node* node)
    {
        osg::ref_ptr<osg::Image> image = createSnapshot(node, 1024, 1024);
        return image.release();
    }

    virtual osg::Geometry* bakeGeometry(osg::Node* node)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false);
        geom->setUseVertexBufferObjects(true);

        osg::ref_ptr<osg::HeightField> hf = createHeightField(node, 128, 128);
        osgVerse::ShapeGeometryVisitor bsgv(geom.get(), NULL); hf->accept(bsgv);

        osgUtil::Simplifier simplifier(0.2f);
        simplifier.simplify(*geom); return geom.release();
    }
};

class ProcessThread : public OpenThreads::Thread
{
public:
    ProcessThread(size_t id, TileOptimizer* opt) : _optimizer(opt), _id(id), _done(false) {}
    bool done() const { return _done; }

    void add(const std::string& name, const TileOptimizer::TileNameList& src)
    { _sourceTiles.push_back(TileNamePair(name, src)); }

    virtual void run()
    {
        while (!_done)
        {
            if (_sourceTiles.empty()) { _done = true; break; }
            std::string outTileFolder = _sourceTiles[0].first;
            TileOptimizer::TileNameList srcTiles = _sourceTiles[0].second;
            _sourceTiles.erase(_sourceTiles.begin());

            std::cout << outTileFolder << " is being processed at " << "Thread-" << _id
                      << "... " << _sourceTiles.size() << " remains." << std::endl;
            _optimizer->processTileFiles(outTileFolder, srcTiles);
            OpenThreads::Thread::microSleep(50);
        }
        _done = true;
    }

    virtual int cancel()
    { _done = true; return OpenThreads::Thread::cancel(); }

protected:
    typedef std::pair<std::string, TileOptimizer::TileNameList> TileNamePair;
    std::vector<TileNamePair> _sourceTiles;
    TileOptimizer* _optimizer;
    size_t _id;  bool _done;
};

class FindPlodVisitor : public osg::NodeVisitor
{
public:
    FindPlodVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    std::vector<osg::PagedLOD*> plodList;

    virtual void apply(osg::PagedLOD& node)
    { plodList.push_back(&node); traverse(node); }
};

static std::string tileNumberToString(int num, int digits = 3)
{
    std::stringstream ss; ss.fill('0'); if (num >= 0) ss << "+"; else ss << "-";
    ss << std::setw(digits) << std::abs(num); return ss.str();
}

TileOptimizer::TileOptimizer(const std::string& outFolder, const std::string& outFormat)
    : _outFolder(outFolder), _outFormat(outFormat), _withDraco(false), _withBasisu(false)
{
    if (!_outFolder.empty())
    {
        char endChar = *_outFolder.rbegin();
        if (endChar != '/' && endChar != '\\') _outFolder += '/';
    }
    _lodScaleAdjacency = 1.0f; _lodScaleTopLevels = 1.0f; _mulForDistanceMode = 2.0f;
    _simplifyRatio = 0.4f; _numThreads = 10; _withThreads = true;
}

TileOptimizer::~TileOptimizer()
{}

bool TileOptimizer::prepare(const std::string& inputFolder, const std::string& inRegex,
                            bool withDraco, bool withBasisuTex, bool withGpuMerger)
{
    _inFolder = inputFolder; _withDraco = withDraco;
    _withBasisu = withBasisuTex; _withGpuBaker = withGpuMerger;
    if (!_inFolder.empty() && !inRegex.empty())
    {
        char endChar = *_inFolder.rbegin();
        if (endChar != '/' && endChar != '\\') _inFolder += '/';
    }
    else return false;

    osgDB::DirectoryContents contents = osgDB::getDirectoryContents(_inFolder);
    _srcNumberMap.clear(); _inFormat = inRegex;

    // Collect all tiles
    for (size_t i = 0; i < contents.size(); ++i)
    {
        std::string tileName = contents[i], tilePrefix; bool validDir = false;
        osgDB::DirectoryContents subContents = osgDB::getDirectoryContents(_inFolder + tileName);
        for (size_t j = 0; j < subContents.size(); ++j)
        {
            std::string ext = osgDB::getFileExtension(subContents[j]);
            if (ext == "osgb" || ext == "OSGB") { validDir = true; break; }
        }

        osg::Vec3s tileNum = getNumberFromTileName(tileName, inRegex, &tilePrefix);
        if (!validDir || tileNum.x() == SHRT_MAX || tileNum.y() == SHRT_MAX) continue;

        OSG_NOTICE << "[TileOptimizer] Preparing tile folder " << tileName << ": Grid = "
                   << tileNum[0] << "x" << tileNum[1] << ", Prefix = " << tilePrefix << std::endl;
        if (_minMaxMap.find(tilePrefix) == _minMaxMap.end())
        {
            _minMaxMap[tilePrefix] = std::pair<osg::Vec2s, osg::Vec2s>(
                osg::Vec2s(SHRT_MAX, SHRT_MAX), osg::Vec2s(-SHRT_MAX, -SHRT_MAX));
        }

        std::pair<osg::Vec2s, osg::Vec2s>& minMax = _minMaxMap[tilePrefix];
        if (tileNum.x() < minMax.first.x()) minMax.first.x() = tileNum.x();
        if (tileNum.x() > minMax.second.x()) minMax.second.x() = tileNum.x();
        if (tileNum.y() < minMax.first.y()) minMax.first.y() = tileNum.y();
        if (tileNum.y() > minMax.second.y()) minMax.second.y() = tileNum.y();
        _srcNumberMap[tilePrefix][osg::Vec2s(tileNum.x(), tileNum.y())] = tileName;
    }
    return true;
}

bool TileOptimizer::processGroundLevel(int combinedX0, int combinedY0, const std::string& subDir)
{
    std::vector<std::string> rootFileNames;
    osgDB::makeDirectory(_outFolder); osgDB::makeDirectory(_outFolder + subDir);
    for (std::map<std::string, NumberMap>::iterator itr = _srcNumberMap.begin();
         itr != _srcNumberMap.end(); ++itr)
    {
        const std::string& tilePrefix = itr->first;
        const std::pair<osg::Vec2s, osg::Vec2s>& minMax = _minMaxMap[tilePrefix];
        const osg::Vec2s& minNum = minMax.first, maxNum = minMax.second;
        int combinedX = combinedX0, combinedY = combinedY0;

        NumberMap& srcNumberMap = itr->second;
        std::map<osg::Vec2s, TileNameList> dstToSrcTileMap;
        if ((maxNum.x() - minNum.x()) < 1 && (maxNum.y() - minNum.y()) < 1) continue;

        // Get map of source-tile list and destination tile
        for (short y = minNum.y(); y <= maxNum.y(); y += combinedY)
        {
            for (short x = minNum.x(); x <= maxNum.x(); x += combinedX)
            {
                osg::Vec2s numD(x, y); TileNameList& tList = dstToSrcTileMap[numD];
                for (short dy = 0; dy < combinedY; ++dy)
                    for (short dx = 0; dx < combinedX; ++dx)
                    {
                        osg::Vec2s numS(x + dx, y + dy);
                        if (srcNumberMap.find(numS) == srcNumberMap.end()) continue;
                        tList.push_back(srcNumberMap[numS]);
                    }
                if (tList.empty()) dstToSrcTileMap.erase(numD);
            }
        }

        // Handle first combinations
        typedef std::pair<std::string, osg::ref_ptr<osg::Node>> NameAndRoughLevel;
        std::map<osg::Vec2s, NameAndRoughLevel> combination; char outSubName[1024] = "";
        std::string outFormat = tilePrefix + _outFormat; if (dstToSrcTileMap.empty()) continue;
        for (std::map<osg::Vec2s, TileNameList>::iterator itr2 = dstToSrcTileMap.begin();
             itr2 != dstToSrcTileMap.end(); ++itr2)
        {
            std::string dstX = tileNumberToString(itr2->first.x()) + "_D" + std::to_string(combinedX);
            std::string dstY = tileNumberToString(itr2->first.y()) + "_D" + std::to_string(combinedY);
            snprintf(outSubName, 1024, outFormat.c_str(), dstX.data(), dstY.data());

            TileNameList& srcTiles = itr2->second; TileNameAndRoughList srcTiles2;
            for (size_t n = 0; n < srcTiles.size(); ++n)
                srcTiles2.push_back(NameAndRoughLevel(srcTiles[n], NULL));

            std::string outFileName = subDir + "/" + outSubName + ".osgb";
            OSG_NOTICE << "[TileOptimizer] Processing combination: " << outSubName << std::endl;
            osg::ref_ptr<osg::Node> rough = processTopTileFiles(outFileName, false, srcTiles2);
            combination[itr2->first] = NameAndRoughLevel(outFileName, rough);
        }

        // Loop combination until the top
        bool isRootNode = false;
        while (!isRootNode)
        {
            std::map<osg::Vec2s, TileNameAndRoughList> dstToSrcTileMap2;
            combinedX *= 2; combinedY *= 2;
            for (short y = minNum.y(); y <= maxNum.y(); y += combinedY)
            {
                for (short x = minNum.x(); x <= maxNum.x(); x += combinedX)
                {
                    osg::Vec2s numD(x, y);
                    std::vector<NameAndRoughLevel>& tList = dstToSrcTileMap2[numD];
                    for (short dy = 0; dy < combinedY; ++dy)
                        for (short dx = 0; dx < combinedX; ++dx)
                        {
                            osg::Vec2s numS(x + dx, y + dy);
                            if (combination.find(numS) == combination.end()) continue;
                            tList.push_back(combination[numS]);
                        }
                    if (tList.empty()) dstToSrcTileMap2.erase(numD);
                }
            }

            // Handle new combinations
            if (dstToSrcTileMap2.size() < 2) isRootNode = true; combination.clear();
            for (std::map<osg::Vec2s, TileNameAndRoughList>::iterator itr2 = dstToSrcTileMap2.begin();
                 itr2 != dstToSrcTileMap2.end(); ++itr2)
            {
                std::string dstX = tileNumberToString(itr2->first.x()) + "_D" + std::to_string(combinedX);
                std::string dstY = tileNumberToString(itr2->first.y()) + "_D" + std::to_string(combinedY);
                snprintf(outSubName, 1024, outFormat.c_str(), dstX.data(), dstY.data());

                TileNameAndRoughList& srcTiles = itr2->second;
                //std::string outFileName = isRootNode ? (tilePrefix + "root.osgb")
                //                        : (subDir + "/" + outSubName + ".osgb");
                std::string outFileName = subDir + "/" + outSubName + ".osgb";
                OSG_NOTICE << "[TileOptimizer] Processing combination: " << outSubName << std::endl;
                osg::ref_ptr<osg::Node> rough = processTopTileFiles(outFileName, isRootNode, srcTiles);
                combination[itr2->first] = NameAndRoughLevel(outFileName, rough);
                if (isRootNode) rootFileNames.push_back(outFileName);
            }
        }
    }

    osg::ref_ptr<osg::ProxyNode> root = new osg::ProxyNode;
    for (size_t i = 0; i < rootFileNames.size(); ++i) root->setFileName(i, rootFileNames[i]);
    osgDB::writeNodeFile(*root, _outFolder + "Tile_Root.osgb");
    return true;
}

bool TileOptimizer::processAdjacency(int adjacentX, int adjacentY)
{
    std::string adjX, adjY; adjX.resize(adjacentX); adjY.resize(adjacentY);
    std::fill(adjX.begin(), adjX.end(), 'x'); std::fill(adjY.begin(), adjY.end(), 'y');
    if (adjacentX < 1 || adjacentY < 1) return false;

    char outSubFolder[1024] = ""; osgDB::makeDirectory(_outFolder);
    std::vector<ProcessThread*> threads(_numThreads); size_t ptr = 0;
    for (std::map<std::string, NumberMap>::iterator itr = _srcNumberMap.begin();
        itr != _srcNumberMap.end(); ++itr)
    {
        const std::string& tilePrefix = itr->first;
        const std::pair<osg::Vec2s, osg::Vec2s>& minMax = _minMaxMap[tilePrefix];
        const osg::Vec2s& minNum = minMax.first, maxNum = minMax.second;

        NumberMap& srcNumberMap = itr->second;
        std::map<osg::Vec2s, TileNameList> dstToSrcTileMap;
        if ((maxNum.x() - minNum.x()) < 1 && (maxNum.y() - minNum.y()) < 1) continue;

        // Get map of source-tile list and destination tile
        for (short y = minNum.y(); y <= maxNum.y(); y += adjacentY)
        {
            for (short x = minNum.x(); x <= maxNum.x(); x += adjacentX)
            {
                osg::Vec2s numD(x, y); TileNameList& tList = dstToSrcTileMap[numD];
                for (short dy = 0; dy < adjacentY; ++dy)
                    for (short dx = 0; dx < adjacentX; ++dx)
                    {
                        osg::Vec2s numS(x + dx, y + dy);
                        if (srcNumberMap.find(numS) == srcNumberMap.end()) continue;
                        const std::string& srcTile = srcNumberMap[numS];
                        tList.push_back(srcTile);
                    }
                if (tList.empty()) dstToSrcTileMap.erase(numD);
            }
        }

        // Process the map to merge source tiles and generate destination folder;
        std::string outFormat = tilePrefix + _outFormat; if (dstToSrcTileMap.empty()) continue;
        for (std::map<osg::Vec2s, TileNameList>::iterator itr2 = dstToSrcTileMap.begin();
             itr2 != dstToSrcTileMap.end(); ++itr2)
        {
            std::string dstX = tileNumberToString(itr2->first.x()) + "+" + adjX;
            std::string dstY = tileNumberToString(itr2->first.y()) + "+" + adjY;
            snprintf(outSubFolder, 1024, outFormat.c_str(), dstX.data(), dstY.data());

            TileNameList& srcTiles = itr2->second;
            std::string outTileFolder = std::string(outSubFolder) + '/';
            osgDB::makeDirectory(_outFolder + outTileFolder);

            if (_numThreads > 0)
            {
                size_t index = (ptr++) % _numThreads; ProcessThread* th = threads[index];
                if (!th) { th = new ProcessThread(index, this); threads[index] = th; }
                th->add(outTileFolder, srcTiles);
            }
            else
                processTileFiles(outTileFolder, srcTiles);
        }
    }

    int numCPUs = OpenThreads::GetNumberOfProcessors(); if (numCPUs < 1) numCPUs = 1;
    for (size_t i = 0; i < _numThreads; ++i)
    {
        ProcessThread* th = threads[i]; if (th == NULL) continue;
        th->setProcessorAffinity(i % numCPUs); th->start();
    }

    bool allThreadDone = false;
    while (!allThreadDone)
    {
        allThreadDone = true;
        for (size_t i = 0; i < _numThreads; ++i)
        {
            ProcessThread* th = threads[i]; if (!th) continue; else allThreadDone = false;
            if (th->done()) { th->join(); delete th; threads[i] = NULL; }
        }
    }
    return true;
}

void TileOptimizer::processTileFiles(const std::string& outTileFolder, const TileNameList& srcTiles)
{
    // Get all matched tile files for merging
    std::map<std::string, TileNameList> levelToFileMap;
    for (size_t i = 0; i < srcTiles.size(); ++i)
    {
        std::string inTileFolder = _inFolder + srcTiles[i] + '/';
        osgDB::DirectoryContents contents = osgDB::getDirectoryContents(inTileFolder);
        for (size_t n = 0; n < contents.size(); ++n)
        {
            std::string fileName = contents[n];
            if (fileName[0] == '.') continue;
            //size_t pos = fileName.find("_L");
            //if (pos != std::string::npos) levelName = fileName.substr(pos + 1);

            osg::Vec3s tileNum = getNumberFromTileName(fileName, _inFormat);
            std::string levelName = std::to_string(tileNum[2]);
            size_t pos = fileName.find("L" + levelName, 2);
            if (pos != std::string::npos) levelName = fileName.substr(pos + 1);
            else
            {
                pos = fileName.find("_" + levelName, 2);
                if (pos != std::string::npos) levelName = fileName.substr(pos + 1);
            }
            levelToFileMap["L" + levelName].push_back(inTileFolder + fileName);
        }
    }

    // Merge every level-set, starting from the highest one
    std::map<std::string, std::string> plodNameMap;
    for (std::map<std::string, TileNameList>::reverse_iterator itr = levelToFileMap.rbegin();
         itr != levelToFileMap.rend(); ++itr)
    {
        TileNameList& tileFiles = itr->second;
        if (tileFiles.empty()) continue;
        if (_withThreads) OpenThreads::Thread::YieldCurrentThread();

        // Get output tile name
        std::string outTileName = outTileFolder, postfix = itr->first;
        if (*outTileFolder.rbegin() == '/')
            outTileName = outTileFolder.substr(0, outTileFolder.size() - 1);
        outTileName += ((postfix == "L0") ? ".osgb" : ("_" + postfix));

        // Map source tiles to output tile name
        std::string outFileName = outTileFolder + outTileName;
        std::vector<osg::ref_ptr<osg::Node>> loadedNodes;
        for (size_t i = 0; i < tileFiles.size(); ++i)
        {
            osg::ref_ptr<osg::Node> tile = osgDB::readNodeFile(tileFiles[i]);
            plodNameMap[tileFiles[i]] = outFileName;
            if (tile.valid())
            {
                if (_filterNodeCallback.valid())
                    _filterNodeCallback->prefilter(tileFiles[i], *tile);
                loadedNodes.push_back(tile);
            }
        }
        if (_withThreads) OpenThreads::Thread::YieldCurrentThread();

        // Merge and generate new tile node
#if true
        osg::ref_ptr<osg::Node> newTile = mergeNodes(loadedNodes, plodNameMap);
        if (newTile.valid())
        {
            osg::ref_ptr<TextureOptimizer> opt;
            if (_filterNodeCallback.valid())
                _filterNodeCallback->postfilter(_outFolder + outFileName, *newTile);
            if (_withBasisu)
            {
                opt = new TextureOptimizer(true, "optimize_tex_" + nanoid::generate(8));
                newTile->accept(*opt);
            }

            osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
            options->setPluginStringData("UseBASISU", "1");
            osgDB::writeNodeFile(*newTile, _outFolder + outFileName, options.get());
            if (_withBasisu) opt->deleteSavedTextures();
        }
#endif
    }
}

osg::Node* TileOptimizer::processTopTileFiles(const std::string& outTileFileName, bool isRootNode,
                                              const TileNameAndRoughList& srcTiles)
{
    osg::ref_ptr<osg::Group> root = new osg::Group;
    std::vector<std::pair<osg::ref_ptr<osg::Geometry>, osg::Matrix>> geomList;

    // Have to merge textures later, so must read RGBA
    setReadingKtxFlag(osgVerse::ReadKtx_ToRGBA, 1);
    for (size_t t = 0; t < srcTiles.size(); ++t)
    {
        const std::pair<std::string, osg::ref_ptr<osg::Node>>& nameAndRough = srcTiles[t];
        std::string fileName = nameAndRough.first, ext = osgDB::getFileExtension(nameAndRough.first);
        if (ext.empty()) fileName = fileName + "/" + fileName + ".osgb";

        osg::ref_ptr<osg::Node> roughNode = nameAndRough.second; osg::PagedLOD* refPlod = NULL;
        osg::ref_ptr<osg::Node> fineNode = osgDB::readNodeFile(_outFolder + fileName);
        if (fineNode.valid())
        {
            if (ext.empty() && _filterNodeCallback.valid())
                _filterNodeCallback->prefilter(fileName, *fineNode);

            FindPlodVisitor fpv; fineNode->accept(fpv);
            if (!fpv.plodList.empty())
            {
                refPlod = fpv.plodList.front();
                refPlod->setDatabasePath("");
                refPlod->setFileName(1, "../" + fileName);
            }

            FindGeometryVisitor fgv(true); fineNode->accept(fgv);
            std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList = fgv.getGeometries();
            geomList.insert(geomList.end(), geomList.begin(), geomList.end());
        }
        else if (roughNode.valid())
        {
            FindGeometryVisitor fgv(true); roughNode->accept(fgv);
            std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList = fgv.getGeometries();
            geomList.insert(geomList.end(), geomList.begin(), geomList.end());
        }

        fileName = "../" + fileName;
        if (roughNode.valid())
        {
            osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
            plod->addChild(roughNode.get());
            plod->setFileName(1, fileName);
            if (refPlod != NULL)
            {
                plod->setRangeMode(refPlod->getRangeMode());
                plod->setCenterMode(refPlod->getCenterMode());
                plod->setCenter(roughNode->getBound().center());
                plod->setRadius(roughNode->getBound().radius());
                for (size_t i = 0; i < refPlod->getNumFileNames(); ++i)
                {
                    float minV = refPlod->getMinRange(i), maxV = refPlod->getMaxRange(i);
                    if (plod->getRangeMode() == osg::LOD::DISTANCE_FROM_EYE_POINT)
                    {
                        float lodScale = _lodScaleTopLevels * _mulForDistanceMode;
                        if (i == refPlod->getNumFileNames() - 1) maxV /= lodScale;
                        else if (i == 0) minV /= lodScale;
                        else { minV /= lodScale; maxV /= lodScale; }
                    }
                    else
                    {
                        if (i == 0) maxV *= _lodScaleTopLevels;
                        else if (i == refPlod->getNumFileNames() - 1) minV *= _lodScaleTopLevels;
                        else { minV *= _lodScaleTopLevels; maxV *= _lodScaleTopLevels; }
                    }
                    plod->setRange(i, minV, maxV);
                    //OSG_NOTICE << "[TileOptimizer::processTopTileFiles] " << outTileFileName << ": "
                    //           << i << ", Range = " << minV << ", " << maxV << std::endl;
                }
            }
            root->addChild(plod.get());
        }
        else  // "xxx_D2" top tiles just include 4 tile root nodes
            if (fineNode.valid()) root->addChild(fineNode.get());
    }

    setReadingKtxFlag(osgVerse::ReadKtx_ToRGBA, 0);
    if (root.valid())
    {
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
        if (_filterNodeCallback.valid())
            _filterNodeCallback->postfilter(_outFolder + outTileFileName, *root);
        osgDB::writeNodeFile(*root, _outFolder + outTileFileName, options.get());
    }

    std::vector<std::pair<osg::Geometry*, osg::Matrix>> geomList2;
    geomList2.assign(geomList.begin(), geomList.end());
    osg::ref_ptr<osg::Node> mergedNode = mergeGeometries(geomList2, 1024, true);
    return mergedNode.release();  // return merged rough level node
}

osg::Vec3s TileOptimizer::getNumberFromTileName(const std::string& name, const std::string& inRegex,
                                                std::string* textPrefix)
{
    std::vector<int> numbers; std::string text = name, text0;
    while (text[0] > 'z' || text[0] < 'A')
    { text0.push_back(text[0]); text.erase(text.begin()); if (text.empty()) break; }

    std::regex re(inRegex); std::smatch results;
    while (std::regex_search(text, results, re))
    {
        if (numbers.empty() && textPrefix != NULL)
            *textPrefix = text0 + results.prefix().str();
        numbers.push_back(std::stoi(results[0]));
        text = results.suffix().str();
    }
    if (numbers.size() > 2) return osg::Vec3s(numbers[0], numbers[1], numbers[2]);
    else if (numbers.size() > 1) return osg::Vec3s(numbers[0], numbers[1], 0);
    else return osg::Vec3s(SHRT_MAX, SHRT_MAX, 0);
}

osg::Node* TileOptimizer::mergeNodes(const std::vector<osg::ref_ptr<osg::Node>>& loadedNodes,
                                     const std::map<std::string, std::string>& plodNameMap)
{
    std::map<std::string, std::vector<osg::PagedLOD*>> plodGroupMap;
    if (loadedNodes.empty()) return NULL;
    //else if (loadedNodes.size() == 1) return loadedNodes[0].get();

    // Find all paged-LODs and group them
    for (size_t i = 0; i < loadedNodes.size(); ++i)
    {
        FindPlodVisitor fpv; loadedNodes[i]->accept(fpv);
        for (size_t j = 0; j < fpv.plodList.size(); ++j)
        {
            std::string plodListString;
            osg::PagedLOD* plod = fpv.plodList[j];
            for (size_t n = 0; n < plod->getNumFileNames(); ++n)
            {
                std::string fileName = plod->getFileName(n);
                if (fileName.empty()) continue;
                else fileName = plod->getDatabasePath() + fileName;

                if (plodNameMap.find(fileName) != plodNameMap.end())
                {
                    fileName = plodNameMap.find(fileName)->second;
                    plodListString += fileName + "\n";
                }
                else
                    OSG_WARN << "[TileOptimizer] Failed to map PagedLOD child file: "
                             << fileName << " of index " << n << std::endl;
            }
            if (!plodListString.empty()) plodGroupMap[plodListString].push_back(plod);
        }
    }

    // Merge plod nodes and rough-level geometries
    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (!plodGroupMap.empty())
    {
        OSG_NOTICE << "[TileOptimizer] Merging " << plodGroupMap.size() << " paged LODs from "
                   << loadedNodes.size() << " tile nodes" << std::endl;
    }

    for (std::map<std::string, std::vector<osg::PagedLOD*>>::iterator itr = plodGroupMap.begin();
         itr != plodGroupMap.end(); ++itr)
    {
        std::vector<osg::PagedLOD*> plodList = itr->second;
        osg::PagedLOD* ref = plodList[0];
        if (_withThreads) OpenThreads::Thread::YieldCurrentThread();

        osg::BoundingSphere bs; std::vector<std::pair<osg::Geometry*, osg::Matrix>> geomList;
        for (size_t i = 0; i < plodList.size(); ++i)
        {
            osg::PagedLOD* n = plodList[i]; FindGeometryVisitor fgv(true); n->accept(fgv);
            std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList = fgv.getGeometries();
            geomList.insert(geomList.end(), geomList.begin(), geomList.end());

            osg::BoundingSphere bs0(n->getCenter(), n->getRadius());
            if (i == 0) bs = bs0; else bs.expandBy(bs0.valid() ? bs0 : n->getBound());
        }

        osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
        plod->setRangeMode(ref->getRangeMode());
        plod->setCenterMode(ref->getCenterMode());
        plod->setCenter(bs.center());
        plod->setRadius(bs.radius());
        plod->addChild(mergeGeometries(geomList, 2048, false));
        for (size_t i = 0; i < ref->getNumFileNames(); ++i)
        {
            float minV = ref->getMinRange(i), maxV = ref->getMaxRange(i);
            if (plod->getRangeMode() == osg::LOD::DISTANCE_FROM_EYE_POINT)
            {
                float lodScale = _lodScaleAdjacency * _mulForDistanceMode;
                if (i == ref->getNumFileNames() - 1) maxV /= lodScale;
                else if (i == 0) minV /= lodScale;
                else { minV /= lodScale; maxV /= lodScale; }
            }
            else
            {
                if (i == 0) maxV *= _lodScaleAdjacency;
                else if (i == ref->getNumFileNames() - 1) minV *= _lodScaleAdjacency;
                else { minV *= _lodScaleAdjacency; maxV *= _lodScaleAdjacency; }
            }
            plod->setRange(i, minV, maxV);

            std::string fileName = ref->getFileName(i);
            if (fileName.empty()) continue; else fileName = ref->getDatabasePath() + fileName;
            fileName = plodNameMap.find(fileName)->second;  // this should be valid
            plod->setFileName(i, osgDB::getSimpleFileName(fileName));
        }
        root->addChild(plod.get());
    }

    // If no paged LOD nodes found, this may be a leaf tile with only geometries
    if (plodGroupMap.empty())
    {
        std::vector<std::pair<osg::Geometry*, osg::Matrix>> geomList;
        for (size_t i = 0; i < loadedNodes.size(); ++i)
        {
            FindGeometryVisitor fgv(true); loadedNodes[i]->accept(fgv);
            std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList = fgv.getGeometries();
            geomList.insert(geomList.end(), geomList.begin(), geomList.end());
        }

        OSG_NOTICE << "[TileOptimizer] Merging " << geomList.size() << " geometries from "
                   << loadedNodes.size() << " leaf nodes" << std::endl;
        if (!geomList.empty()) root->addChild(mergeGeometries(geomList, 4096, false));
        if (_withThreads) OpenThreads::Thread::YieldCurrentThread();
    }
    return (root->getNumChildren() > 0) ? root.release() : NULL;
}

osg::Node* TileOptimizer::mergeGeometries(const std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList,
                                          int highestRes, bool simplify)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
#if true
    for (size_t i = 0; i < geomList.size(); i += 16)
    {
        GeometryMerger merger(_withGpuBaker ? GeometryMerger::GPU_BAKING : GeometryMerger::COMBINED_GEOMETRY);
        if (_withGpuBaker) merger.setGpkBaker(new DefaultGpuBaker);

        osg::ref_ptr<osg::Geometry> result = merger.process(geomList, i, 16, highestRes);
        if (result.valid())
        {
            if (simplify && _simplifyRatio > 0.0f)
            {
                // FIXME: not good to weld vertices, it makes wrong texture mapping
                // A better way is to render all to textures to get heightmap for use.
                MeshCollector collector;
                collector.setWeldingVertices(true);
                collector.apply(*result);

                const std::vector<osg::Vec3>& va = collector.getVertices();
                const std::vector<unsigned int>& tri = collector.getTriangles();
                std::vector<osg::Vec4>& na = collector.getAttributes(MeshCollector::NormalAttr);
                std::vector<osg::Vec4>& ca = collector.getAttributes(MeshCollector::ColorAttr);
                std::vector<osg::Vec4>& ta = collector.getAttributes(MeshCollector::UvAttr);

                osg::Vec3Array* vaArray = new osg::Vec3Array(va.begin(), va.end());
                osg::Vec4Array* caArray = new osg::Vec4Array(ca.begin(), ca.end());
                osg::Vec3Array* naArray = new osg::Vec3Array(na.size());
                osg::Vec2Array* uvArray = new osg::Vec2Array(ta.size());

                result->setVertexArray(vaArray);
                if (va.size() != ca.size()) result->setColorArray(NULL);
                else { result->setColorArray(caArray); result->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }

                if (va.size() != na.size()) result->setNormalArray(NULL);
                else
                {
                    for (size_t n = 0; n < na.size(); ++n) (*naArray)[n] = osg::Vec3(na[n][0], na[n][1], na[n][2]);
                    result->setNormalArray(naArray); result->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
                }

                if (va.size() != ta.size()) result->setTexCoordArray(0, NULL);
                else
                {
                    for (size_t n = 0; n < ta.size(); ++n) (*uvArray)[n] = osg::Vec2(ta[n][0], ta[n][1]);
                    result->setTexCoordArray(0, uvArray);
                }

                osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES);
                de->assign(tri.begin(), tri.end());
                result->removePrimitiveSet(0, result->getNumPrimitiveSets());
                result->addPrimitiveSet(de);

                osgUtil::Simplifier simplifier(_simplifyRatio);
                simplifier.simplify(*result);
            }

            if (_withDraco)
            {
                osg::ref_ptr<osgVerse::DracoGeometry> geom2 = new osgVerse::DracoGeometry(*result);
                geode->addDrawable(geom2.get());
            }
            else geode->addDrawable(result.get());
        }
    }
#else
    for (size_t i = 0; i < geomList.size(); ++i)
    {
        if (_withDraco)
        {
            osg::ref_ptr<osgVerse::DracoGeometry> geom2 = new osgVerse::DracoGeometry(*geomList[i]);
            geode->addDrawable(geom2.get());
        }
        else geode->addDrawable(geomList[i]);
    }
#endif
    return geode.release();
}
