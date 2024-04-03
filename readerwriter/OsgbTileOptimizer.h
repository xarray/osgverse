#ifndef MANA_READERWRITER_OSGBTILEOPTIMIZER_HPP
#define MANA_READERWRITER_OSGBTILEOPTIMIZER_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osgDB/ReaderWriter>
#include "Export.h"

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT TileOptimizer : public osg::Referenced
    {
    public:
        typedef std::vector<std::string> TileNameList;
        TileOptimizer(const std::string& outFolder, const std::string& outFormat = "Tile_%s_%s");

        bool prepare(const std::string& inputFolder, const std::string& inRegex = "([+-]?\\d+)",
                     bool withDraco = true, bool withBasisuTex = true);
        bool processAdjacency(int adjacentX = 2, int adjacentY = 2);
        bool processGroundLevel(int combinedX = 2, int combinedY = 2, bool atOutput = true);

    protected:
        virtual ~TileOptimizer();
        void processTileFiles(const std::string& outTileFolder, const TileNameList& srcTiles);

        osg::Vec2s getNumberFromTileName(const std::string& name, const std::string& inRegex);
        osg::Node* mergeNodes(const std::vector<osg::ref_ptr<osg::Node>>& loadedNodes,
                              const std::map<std::string, std::string>& plodNameMap);
        void mergeGeometries(const std::vector<osg::Geometry*>& geomList);

        std::map<std::string, osg::Vec2s> _srcToDstTileMap;
        std::map<osg::Vec2s, std::string> _srcNumberMap;
        std::string _inFolder, _outFolder, _outFormat;
        osg::Vec2s _minNum, _maxNum;
        bool _withDraco, _withBasisu;
    };

}

#endif
