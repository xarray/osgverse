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
        TileOptimizer(const std::string& outFolder, const std::string& outFormat = "%s_%s");

        void setUseThreads(int num) { _numThreads = num; _withThreads = (num > 0); }
        void setMergingSimplifyRatio(float r) { _simplifyRatio = r; }
        void setLodScale(float adjacency, float groundLv, float mulForDistanceMode)
        {
            _lodScaleAdjacency = adjacency; _lodScaleTopLevels = groundLv;
            _mulForDistanceMode = mulForDistanceMode;
        }

        bool prepare(const std::string& inputFolder, const std::string& inRegex = "([+-]?\\d+)",
                     bool withDraco = true, bool withBasisuTex = true, bool withGpuMerger = false);
        bool processAdjacency(int adjacentX = 2, int adjacentY = 2);
        bool processGroundLevel(int combinedX = 2, int combinedY = 2, const std::string& subDir = "0");

        typedef std::vector<std::string> TileNameList;
        typedef std::vector<std::pair<std::string, osg::ref_ptr<osg::Node>>> TileNameAndRoughList;
        void processTileFiles(const std::string& outTileFolder, const TileNameList& srcTiles);
        osg::Node* processTopTileFiles(const std::string& outTileFileName, bool isRootNode,
                                       const TileNameAndRoughList& srcTiles);

        struct FilterNodeCallback : public osg::Referenced
        {
            virtual void prefilter(const std::string& name, osg::Node& node) {}
            virtual void postfilter(const std::string& name, osg::Node& node) {}
        };
        void setFilterNodeCallback(FilterNodeCallback* cb) { _filterNodeCallback = cb; }
        FilterNodeCallback* getFilterNodeCallback() const { return _filterNodeCallback.get(); }

    protected:
        virtual ~TileOptimizer();
        osg::Vec3s getNumberFromTileName(const std::string& name, const std::string& inRegex,
                                         std::string* textPrefix = NULL);
        osg::Node* mergeNodes(const std::vector<osg::ref_ptr<osg::Node>>& loadedNodes,
                              const std::map<std::string, std::string>& plodNameMap);
        osg::Node* mergeGeometries(const std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList,
                                   int highestRes, bool simplify);

        typedef std::map<osg::Vec2s, std::string> NumberMap;
        std::map<std::string, NumberMap> _srcNumberMap;
        std::map<std::string, std::pair<osg::Vec2s, osg::Vec2s>> _minMaxMap;
        osg::ref_ptr<FilterNodeCallback> _filterNodeCallback;
        std::string _inFolder, _outFolder, _inFormat, _outFormat;
        float _lodScaleAdjacency, _lodScaleTopLevels, _mulForDistanceMode, _simplifyRatio;
        int _numThreads; bool _withDraco, _withBasisu, _withThreads, _withGpuBaker;
    };

}

#endif
