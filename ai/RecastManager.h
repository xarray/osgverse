#ifndef MANA_AI_RECASTMANAGER_HPP
#define MANA_AI_RECASTMANAGER_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/MatrixTransform>

#define PARTITION_WATERSHED 0
#define PARTITION_MONOTONE 1

namespace osgVerse
{

    enum RecastPolyArea
    {
        POLYAREA_NULL = 0,    // RC_NULL_AREA
        POLYAREA_WATER,
        POLYAREA_GROUND = 63  // RC_WALKABLE_AREA
    };

    enum RecastPolyFlags
    {
        POLYFLAGS_WALK = 0x01,      // Ability to walk (ground, grass, road)
        POLYFLAGS_SWIM = 0x02,      // Ability to swim (water)
        POLYFLAGS_DISABLED = 0x80,  // Disabled polygon
    };

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
        float padding;   // Padding of bounding box
        int partitionType;  // Partition type (WATERSHED / MONOTONE)

        RecastSettings() :
            cellSize(0.3f), cellHeight(0.2f), agentHeight(2.0f), agentRadius(0.6f), agentMaxClimb(0.9f),
            agentMaxSlope(45.0f), regionMinSize(8.0f), regionMergeSize(20.0f), edgeMaxLen(12.0f),
            edgeMaxError(1.3f), vertsPerPoly(6.0f), detailSampleDist(6.0f), detailSampleMaxError(1.0f),
            tileSize(128.0f), padding(1.0f), partitionType(PARTITION_WATERSHED) {}
    };

    /// Recast-navigation implementation
    /// OSG vertex(x, y, z) -> Recast vertex(x, z, -y)
    class RecastManager : public osg::Referenced
    {
    public:
        RecastManager();

        bool build(osg::Node* node);
        osg::Node* getDebugMesh() const;

    protected:
        virtual ~RecastManager();

        bool buildTiles(const std::vector<osg::Vec3>& va, const std::vector<unsigned int>& indices,
                        const osg::BoundingBox& worldBounds, const osg::Vec2i& tileStart,
                        const osg::Vec2i& tileEnd);

        osg::ref_ptr<osg::Referenced> _recastData;
        RecastSettings _settings;
    };

}

#endif
