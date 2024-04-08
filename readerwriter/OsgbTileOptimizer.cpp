#include "OsgbTileOptimizer.h"
#include "DracoProcessor.h"
#include "Utilities.h"
#include "modeling/GeometryMerger.h"
#include <osg/PagedLOD>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <vector>
#include <regex>
#include <limits>
#include <iomanip>
using namespace osgVerse;

class FindPlodVisitor : public osg::NodeVisitor
{
public:
    FindPlodVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    std::vector<osg::PagedLOD*> plodList;

    virtual void apply(osg::PagedLOD& node)
    { plodList.push_back(&node); traverse(node); }
};

class FindGeometryVisitor : public osg::NodeVisitor
{
public:
    FindGeometryVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    std::vector<osg::Geometry*> geomList;

    virtual void apply(osg::Geode& node)
    {
        for (size_t i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (geom) geomList.push_back(geom);
        }
        traverse(node);
    }
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
}

TileOptimizer::~TileOptimizer()
{}

bool TileOptimizer::prepare(const std::string& inputFolder, const std::string& inRegex,
                            bool withDraco, bool withBasisuTex)
{
    _inFolder = inputFolder; _withDraco = withDraco; _withBasisu = withBasisuTex;
    if (!_inFolder.empty() && !inRegex.empty())
    {
        char endChar = *_inFolder.rbegin();
        if (endChar != '/' && endChar != '\\') _inFolder += '/';
    }
    else return false;

    osgDB::DirectoryContents contents = osgDB::getDirectoryContents(_inFolder);
    _minNum.set(SHRT_MAX, SHRT_MAX); _maxNum.set(-SHRT_MAX, -SHRT_MAX);
    _srcNumberMap.clear(); _srcToDstTileMap.clear();

    // Collect all tiles
    for (size_t i = 0; i < contents.size(); ++i)
    {
        std::string tileName = contents[i];
        osg::Vec2s tileNum = getNumberFromTileName(tileName, inRegex);
        if (tileNum.x() == SHRT_MAX || tileNum.y() == SHRT_MAX) continue;
        if (tileNum.x() < _minNum.x()) _minNum.x() = tileNum.x();
        if (tileNum.x() > _maxNum.x()) _maxNum.x() = tileNum.x();
        if (tileNum.y() < _minNum.y()) _minNum.y() = tileNum.y();
        if (tileNum.y() > _maxNum.y()) _maxNum.y() = tileNum.y();
        _srcNumberMap[tileNum] = tileName;
    }
    return true;
}

bool TileOptimizer::processAdjacency(int adjacentX, int adjacentY)
{
    std::map<osg::Vec2s, TileNameList> dstToSrcTileMap;
    if ((_maxNum.x() - _minNum.x()) < 1 && (_maxNum.y() - _minNum.y()) < 1) return false;
    if (adjacentX < 1 || adjacentY < 1) return false;

    // Get map of source-tile list and destination tile
    for (short y = _minNum.y(); y <= _maxNum.y(); y += adjacentY)
    {
        for (short x = _minNum.x(); x <= _maxNum.x(); x += adjacentX)
        {
            osg::Vec2s numD(x, y); TileNameList& tList = dstToSrcTileMap[numD];
            for (short dy = 0; dy < adjacentY; ++dy)
                for (short dx = 0; dx < adjacentX; ++dx)
                {
                    osg::Vec2s numS(x + dx, y + dy);
                    if (_srcNumberMap.find(numS) == _srcNumberMap.end()) continue;
                    const std::string& srcTile = _srcNumberMap[numS];
                    _srcToDstTileMap[srcTile] = numD; tList.push_back(srcTile);
                }
            if (tList.empty()) dstToSrcTileMap.erase(numD);
        }
    }

    // Process the map to merge source tiles and generate destination folder;
    if (dstToSrcTileMap.empty()) return false;
    std::string adjX = "+" + std::to_string(adjacentX), adjY = "+" + std::to_string(adjacentY);
    char outSubFolder[1024] = ""; osgDB::makeDirectory(_outFolder);
    for (std::map<osg::Vec2s, TileNameList>::iterator itr = dstToSrcTileMap.begin();
         itr != dstToSrcTileMap.end(); ++itr)
    {
        std::string dstX = tileNumberToString(itr->first.x()) + adjX;
        std::string dstY = tileNumberToString(itr->first.y()) + adjY;
        snprintf(outSubFolder, 1024, _outFormat.c_str(), dstX.data(), dstY.data());

        TileNameList& srcTiles = itr->second;
        std::string outTileFolder = std::string(outSubFolder) + '/';
        osgDB::makeDirectory(_outFolder + outTileFolder);
        processTileFiles(outTileFolder, srcTiles);
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
            std::string fileName = contents[n], levelName = "L0";
            size_t pos = fileName.find("_L"); if (fileName[0] == '.') continue;
            if (pos != std::string::npos) levelName = fileName.substr(pos + 1);
            levelToFileMap[levelName].push_back(inTileFolder + fileName);
        }
    }

    // Start merging every level-set
    std::map<std::string, std::string> plodNameMap;
    for (std::map<std::string, TileNameList>::reverse_iterator itr = levelToFileMap.rbegin();
         itr != levelToFileMap.rend(); ++itr)
    {
        TileNameList& tileFiles = itr->second;
        if (tileFiles.empty()) continue;
        
        // Get output tile name
        std::string outTileName = outTileFolder, postfix = itr->first;
        if (*outTileFolder.rbegin() == '/')
            outTileName = outTileFolder.substr(0, outTileFolder.size() - 1);
        outTileName += (postfix == "L0") ? ".osgb" : ("_" + postfix);

        // Map source tiles to output tile name
        std::string outFileName = outTileFolder + outTileName;
        std::vector<osg::ref_ptr<osg::Node>> loadedNodes;
        for (size_t i = 0; i < tileFiles.size(); ++i)
        {
            osg::ref_ptr<osg::Node> tile = osgDB::readNodeFile(tileFiles[i]);
            plodNameMap[tileFiles[i]] = outFileName;
            if (tile.valid()) loadedNodes.push_back(tile); 
        }

        // Merge and generate new tile node
        osg::ref_ptr<osg::Node> newTile = mergeNodes(loadedNodes, plodNameMap);
        if (newTile.valid())
        {
            TextureOptimizer opt(true);
            if (_withBasisu) newTile->accept(opt);

            osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
            options->setPluginStringData("UseBASISU", "1");
            osgDB::writeNodeFile(*newTile, _outFolder + outFileName, options.get());
            if (_withBasisu) opt.deleteSavedTextures();
        }
    }
}

osg::Vec2s TileOptimizer::getNumberFromTileName(const std::string& name, const std::string& inRegex)
{
    std::vector<int> numbers; std::regex re(inRegex);
    auto words_begin = std::sregex_iterator(name.begin(), name.end(), re);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator it = words_begin; it != words_end; ++it)
    {
        std::smatch match = *it;
        numbers.push_back(std::stoi(match.str()));
    }

    if (numbers.size() > 1) return osg::Vec2s(numbers[0], numbers[1]);
    else return osg::Vec2s(SHRT_MAX, SHRT_MAX);
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
    for (std::map<std::string, std::vector<osg::PagedLOD*>>::iterator itr = plodGroupMap.begin();
         itr != plodGroupMap.end(); ++itr)
    {
        std::vector<osg::PagedLOD*>& plodList = itr->second;
        osg::PagedLOD* ref = plodList[0];

        osg::BoundingSphere bs; std::vector<osg::Geometry*> geomList;
        for (size_t i = 0; i < plodList.size(); ++i)
        {
            osg::PagedLOD* n = plodList[i]; FindGeometryVisitor fgv; n->accept(fgv);
            geomList.insert(geomList.end(), fgv.geomList.begin(), fgv.geomList.end());

            osg::BoundingSphere bs0(n->getCenter(), n->getRadius());
            if (i == 0) bs = bs0;
            else bs.expandBy(bs0.valid() ? bs0 : n->getBound());
        }

        osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
        plod->setRangeMode(ref->getRangeMode());
        plod->setCenterMode(ref->getCenterMode());
        plod->setCenter(bs.center());
        plod->setRadius(bs.radius());
        plod->addChild(mergeGeometries(geomList));
        for (size_t i = 0; i < ref->getNumFileNames(); ++i)
        {
            float minV = ref->getMinRange(i), maxV = ref->getMaxRange(i);
            if (i == 0) maxV *= 2.0f;
            else if (i == ref->getNumFileNames() - 1) minV *= 2.0f;
            else { minV *= 2.0f; maxV *= 2.0f; }
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
        std::vector<osg::Geometry*> geomList;
        for (size_t i = 0; i < loadedNodes.size(); ++i)
        {
            FindGeometryVisitor fgv; loadedNodes[i]->accept(fgv);
            geomList.insert(geomList.end(), fgv.geomList.begin(), fgv.geomList.end());
        }
        if (!geomList.empty()) root->addChild(mergeGeometries(geomList));
    }
    return (root->getNumChildren() > 0) ? root.release() : NULL;
}

osg::Node* TileOptimizer::mergeGeometries(const std::vector<osg::Geometry*>& geomList)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
#if true
    GeometryMerger merger;
    osg::ref_ptr<osg::Geometry> result = merger.process(geomList);
    if (result.valid())
    {
        osg::ref_ptr<osgVerse::DracoGeometry> geom2 = new osgVerse::DracoGeometry(*result);
        geode->addDrawable(result.get());
    }
#else
    for (size_t i = 0; i < geomList.size(); ++i)
    {
        osg::ref_ptr<osgVerse::DracoGeometry> geom2 = new osgVerse::DracoGeometry(*geomList[i]);
        geode->addDrawable(geom2.get());
    }
#endif
    return geode.release();
}
