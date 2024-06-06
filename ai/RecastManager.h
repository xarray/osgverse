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
        int maxSearchNodes;  // Max search nodes of NavMeshQuery

        RecastSettings() :
            cellSize(0.3f), cellHeight(0.2f), agentHeight(2.0f), agentRadius(0.6f), agentMaxClimb(0.9f),
            agentMaxSlope(45.0f), regionMinSize(8.0f), regionMergeSize(20.0f), edgeMaxLen(12.0f),
            edgeMaxError(1.3f), vertsPerPoly(6.0f), detailSampleDist(6.0f), detailSampleMaxError(1.0f),
            tileSize(128.0f), padding(1.0f), partitionType(PARTITION_WATERSHED), maxSearchNodes(2048) {}
    };

    /** Recast-navigation implementation
        OSG vertex(x, y, z) -> Recast vertex(x, z, -y)
    */
    class RecastManager : public osg::Referenced
    {
    public:
        RecastManager();

        /** Build nav-mesh tiles from scene graph */
        bool build(osg::Node* node);

        /** Get debug nav-mesh of all current tiles */
        osg::Node* getDebugMesh() const;

        /** Save current nav-mesh tiles to stream */
        bool save(std::ostream& out);

        /** Read from stream and add tiles to nav-mesh */
        bool read(std::istream& in);

        // Agent structure
        struct Agent : public osg::Referenced
        {
            std::string name;
            osg::Vec3 position, velocity;
            osg::Vec3 target, targetVelocity;
        };

        /** Update/add agent */
        void updateAgent(Agent* agent);

        /** Remove agent */
        void removeAgent(Agent* agent);

        std::set<osg::ref_ptr<Agent>>& getAgents() { return _agents; }
        void clearAllAgents();

        /** Advance the scene to update all agents */
        void advance(float simulationTime);

        /** Find a path on nav-mesh surface */
        std::vector<osg::Vec3> findPath(const osg::Vec3& s, const osg::Vec3& e,
                                        const osg::Vec3& extents = osg::Vec3(1.0f, 1.0f, 1.0f));

        /** Recast on the nav-mesh */
        bool recast(osg::Vec3& result, const osg::Vec3& s, const osg::Vec3& e,
                    const osg::Vec3& extents = osg::Vec3(1.0f, 1.0f, 1.0f));

    protected:
        virtual ~RecastManager();

        bool buildTiles(const std::vector<osg::Vec3>& va, const std::vector<unsigned int>& indices,
                        const osg::BoundingBox& worldBounds, const osg::Vec2i& tileStart,
                        const osg::Vec2i& tileEnd);

        std::set<osg::ref_ptr<Agent>> _agents;
        osg::ref_ptr<osg::Referenced> _recastData;
        RecastSettings _settings;
    };

}

#endif
