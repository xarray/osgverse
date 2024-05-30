#ifndef MANA_AI_RECASTMANAGER_HPP
#define MANA_AI_RECASTMANAGER_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>

namespace osgVerse
{

    struct RecastSettings
    {
        float cellSize;  // Cell size in world units
        float cellHeight;  // Cell height in world units
        
        float agentHeight;  // Agent height in world units
        float agentRadius;  // Agent radius in world units
        float agentMaxClimb;  // Agent max climb in world units
        float agentMaxSlope;  // Agent max slope in degrees
        
        float regionMinSize;  // Region minimum size in voxels
        float regionMergeSize;  // Region merge size in voxels
        
        float edgeMaxLen;  // Edge max length in world units
        float edgeMaxError;  // Edge max error in voxels
        float vertsPerPoly;  // Number of vertices per navigation polygon
        
        float detailSampleDist;  // Detail sample distance in voxels
        float detailSampleMaxError;  // Detail sample max error in voxel heights
        float tileSize;  // Size of the tiles in voxels
        int partitionType;  // Partition type (WATERSHE / MONOTONE / LAYERS)

        RecastSettings() :
            cellSize(0.3f), cellHeight(0.2f), agentHeight(2.0f), agentRadius(0.6f), agentMaxClimb(0.9f),
            agentMaxSlope(45.0f), regionMinSize(8.0f), regionMergeSize(20.0f), edgeMaxLen(12.0f),
            edgeMaxError(1.3f), vertsPerPoly(6.0f), detailSampleDist(6.0f), detailSampleMaxError(1.0f),
            tileSize(32.0f), partitionType(0) {}
    };

    class RecastManager : public osg::Referenced
    {
    public:
        RecastManager();

        bool build(osg::Node* node);

    protected:
        virtual ~RecastManager();

        bool buildTiles(const std::vector<osg::Vec3>& va, const std::vector<unsigned int>& indices,
                        const osg::Vec2i& tileStart, const osg::Vec2i& tileEnd, void* context);

        osg::ref_ptr<osg::Referenced> _recastData;
        RecastSettings _settings;
    };

}

#endif
