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

        void setSettings(const RecastSettings& s) { _settings = s; }
        const RecastSettings& getSettings() const { return _settings; }

        void setAlwaysUseChunkyMesh(bool b) { _alwaysUseChunkyMesh = b; }
        bool getAlwaysUseChunkyMesh() const { return _alwaysUseChunkyMesh; }

        /** Get debug nav-mesh of all current tiles */
        osg::Node* getDebugMesh() const;

        /** Build nav-mesh tiles from scene graph */
        bool build(osg::Node* node, bool loadingFineLevels = false);

        /** Read from stream and add tiles to nav-mesh */
        bool read(std::istream& in);

        /** Save current nav-mesh tiles to stream */
        bool save(std::ostream& out);

        // Agent structure
        struct Agent : public osg::Referenced
        {
            osg::observer_ptr<osg::Transform> transform;  // (in/out) The node as an agent (optional)
            osg::Vec3 position, velocity;                 // (in/out) Current position and (out) velocity
            osg::Vec3 target;                             // Target position
            float maxSpeed, maxAcceleration;              // Max speed and acceleration
            float separationAggressivity;                 // How aggressive to be separated from others
            int id, state;                                // (out) ID and state (active?0xf0 + CrowdAgentState)
            bool dirtyParams, byVelocity;                 // If dirty parameters, and if computed by velocity

            Agent(osg::Transform* node, const osg::Vec3& t)
            :   transform(node), target(t), maxSpeed(4.0f), maxAcceleration(8.0f),
                separationAggressivity(-1.0f), id(-1), state(0), dirtyParams(true), byVelocity(false) {}
            osg::BoundingBox getBoundingBox() const;
        };

        /** Initialize agent manager */
        bool initializeAgents(int maxAgents = 128, int obstacleAvoidType = -1);

        /** Update/add agent */
        void updateAgent(Agent* agent, const osg::Vec2& rangeFactor = osg::Vec2(4.0f, 30.0f));

        /** Cancel agent target */
        void cancelAgent(Agent* agent);

        /** Remove agent */
        void removeAgent(Agent* agent);

        /** Get agent from node */
        Agent* getAgentFromNode(osg::Node* node);

        std::set<osg::ref_ptr<Agent>>& getAgents() { return _agents; }
        void clearAllAgents();

        /** Advance the scene to update all agents */
        void advance(float simulationTime, float multiplier = 1.0f);

        /** Find a path on nav-mesh surface. For flags, see 'enum dtStraightPathFlags' */
        std::vector<osg::Vec3> findPath(std::vector<int>& flags, const osg::Vec3& s, const osg::Vec3& e,
                                        const osg::Vec3& extents = osg::Vec3(1.0f, 1.0f, 1.0f));

        /** Casts a 'walkability' ray along nav-mesh surface to find nearest wall */
        bool hitWall(osg::Vec3& result, osg::Vec3& resultNormal, const osg::Vec3& s, const osg::Vec3& e,
                     const osg::Vec3& extents = osg::Vec3(1.0f, 1.0f, 1.0f));

    protected:
        virtual ~RecastManager();

        bool initializeNavMesh(const osg::Vec3& o, float tileW, float tileH, int maxPolys, int maxTiles);
        bool initializeQuery();
        bool buildTiles(const std::vector<osg::Vec3>& va, const std::vector<unsigned int>& indices,
                        const osg::BoundingBoxd& worldBB, const osg::Vec2d& tileStart, const osg::Vec2d& tileEnd);

        std::map<osg::Node*, osg::observer_ptr<Agent>> _agentFinderMap;
        std::set<osg::ref_ptr<Agent>> _agents;
        osg::ref_ptr<osg::Referenced> _recastData;
        RecastSettings _settings;
        int _obstacleAvoidingType;
        float _lastSimulationTime;
        bool _alwaysUseChunkyMesh;
    };

}

#endif
