#include <osg/io_utils>
#include <osg/PositionAttitudeTransform>
#include <iostream>

#include "RecastManager.h"
#include "RecastManager_Private.h"
using namespace osgVerse;

RecastManager::RecastManager()
{
    _recastData = new NavData;
    _lastSimulationTime = -1.0f;
}

RecastManager::~RecastManager()
{
}

bool RecastManager::initializeNavMesh(const osg::Vec3& o, float tileW, float tileH,
                                      int maxPolys, int maxTiles)
{
    dtNavMeshParams params{};
    params.orig[0] = o[0]; params.orig[1] = o[1]; params.orig[2] = o[2];
    params.tileWidth = tileW; params.tileHeight = tileH;
    params.maxPolys = maxPolys; params.maxTiles = maxTiles;

    NavData* navData = static_cast<NavData*>(_recastData.get());
    navData->clear(); navData->navMesh = dtAllocNavMesh();
    if (dtStatusFailed(navData->navMesh->init(&params))) return false;
    else return true;
}

bool RecastManager::initializeQuery()
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->navMesh) { OSG_WARN << "[RecastManager] Nav-mesh not created" << std::endl; return false; }
    if (navData->navQuery != NULL) return true;

    bool numTiles = navData->navMesh->getMaxTiles();
    if (numTiles > 0)
    {
        navData->navQuery = dtAllocNavMeshQuery();
        if (dtStatusFailed(navData->navQuery->init(navData->navMesh, _settings.maxSearchNodes)))
        { OSG_WARN << "[RecastManager] Failed to create query object" << std::endl; return false; }
    }
    else
        OSG_WARN << "[RecastManager] Nav-mesh created without any tiles" << std::endl;
    return (numTiles > 0);
}

bool RecastManager::build(osg::Node* node, bool loadingFineLevels)
{
    MeshCollector collector; osg::Vec2i tStart, tEnd;
    collector.setWeldingVertices(true); collector.setUseGlobalVertices(false);
    collector.setOnlyVertexAndIndices(true);
    collector.setLoadingFineLevels(loadingFineLevels);
    if (!node) return false; else node->accept(collector);
    if (collector.getVertices().empty()) return false;

    osg::BoundingBox worldBounds = collector.getBoundingBox();
    osg::Vec3 padding(_settings.padding * _settings.tileSize,
                      _settings.padding * _settings.tileSize, _settings.padding);
    worldBounds._min -= padding; worldBounds._max += padding;

    NavData* navData = static_cast<NavData*>(_recastData.get());
    int maxTiles = navData->calculateMaxTiles(
        worldBounds, tStart, tEnd, _settings.tileSize, _settings.cellSize);
    float tileWidth = _settings.tileSize * _settings.cellSize;
    int maxPolys = 1u << (22 - navData->logBaseTwo(maxTiles));
    if (!initializeNavMesh(osg::Vec3(), tileWidth, tileWidth, maxPolys, maxTiles)) return false;
    return buildTiles(collector.getVertices(), collector.getTriangles(), worldBounds, tStart, tEnd);
}

bool RecastManager::initializeAgents(int maxAgents)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->navMesh)
    { OSG_WARN << "[RecastManager] Nav-mesh not created" << std::endl; return false; }

    navData->clearCrowd(); navData->crowd = dtAllocCrowd();
    navData->crowd->init(maxAgents, _settings.agentRadius, navData->navMesh);
    navData->crowd->getEditableFilter(0)->setExcludeFlags(POLYFLAGS_DISABLED);

    dtObstacleAvoidanceParams params;  // Use mostly default settings, copy from dtCrowd
    memcpy(&params, navData->crowd->getObstacleAvoidanceParams(0), sizeof(dtObstacleAvoidanceParams));

    params.velBias = 0.5f; params.adaptiveDivs = 5;
    params.adaptiveRings = 2; params.adaptiveDepth = 1;
    navData->crowd->setObstacleAvoidanceParams(0, &params);  // Low (11)

    params.velBias = 0.5f; params.adaptiveDivs = 5;
    params.adaptiveRings = 2; params.adaptiveDepth = 2;
    navData->crowd->setObstacleAvoidanceParams(1, &params);  // Medium (22)

    params.velBias = 0.5f; params.adaptiveDivs = 7;
    params.adaptiveRings = 2; params.adaptiveDepth = 3;
    navData->crowd->setObstacleAvoidanceParams(2, &params);  // Good (45)

    params.velBias = 0.5f; params.adaptiveDivs = 7;
    params.adaptiveRings = 3; params.adaptiveDepth = 3;
    navData->crowd->setObstacleAvoidanceParams(3, &params);  // High (66)
    return true;
}

void RecastManager::updateAgent(Agent* agent)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->crowd) { OSG_WARN << "[RecastManager] Crowd not created" << std::endl; return; }
    if (_agents.find(agent) == _agents.end()) _agents.insert(agent);

    osg::Matrix matrix; bool newlyCreated = (agent->id < 0);
    if (agent->transform.valid())
    {
        agent->transform->computeLocalToWorldMatrix(matrix, NULL);
        agent->position = matrix.getTrans();
    }

    float pos[3] = { agent->position[0], agent->position[2], -agent->position[1] };
    float dst[3] = { agent->target[0], agent->target[2], -agent->target[1] };

    dtCrowdAgentParams ap; memset(&ap, 0, sizeof(ap));
    ap.radius = _settings.agentRadius; ap.height = _settings.agentHeight;
    ap.maxAcceleration = agent->maxAcceleration; ap.maxSpeed = agent->maxSpeed;
    ap.collisionQueryRange = ap.radius * 4.0f;
    ap.pathOptimizationRange = ap.radius * 30.0f;
    ap.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OPTIMIZE_VIS |
                     DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
    ap.obstacleAvoidanceType = 1.0f;  // (0, DT_CROWD_MAX_OBSTAVOIDANCE_PARAMS]
    ap.separationWeight = agent->separationAggressivity;  // (0, 20]
    if (newlyCreated)
        agent->id = navData->crowd->addAgent(pos, &ap);
    else if (agent->dirtyParams)
        navData->crowd->updateAgentParameters(agent->id, &ap);

    const dtCrowdAgent* dt = navData->crowd->getAgent(agent->id);
    if (dt && dt->active)
    {
        if (agent->byVelocity)
        {
            float vel[3] = { 0.0f };
            navData->computeVelocity(vel, dt->npos, dst, agent->maxSpeed);
            navData->crowd->requestMoveVelocity(agent->id, vel);
            agent->state = dt->state | (dt->active ? 0xf0 : 0);
        }
        else
        {
            const dtQueryFilter* filter = navData->crowd->getFilter(0);
            const float* halfExtents = navData->crowd->getQueryExtents();
            navData->navQuery->findNearestPoly(dst, halfExtents, filter,
                                               &navData->nearestReference, navData->nearestPointOnRef);
            navData->crowd->requestMoveTarget(agent->id, navData->nearestReference, dst);
        }
        agent->velocity.set(dt->vel[0], dt->vel[2], -dt->vel[1]);
    }
    agent->dirtyParams = false;
}

void RecastManager::cancelAgent(Agent* agent)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->crowd) { OSG_WARN << "[RecastManager] Crowd not created" << std::endl; return; }
    if (_agents.find(agent) != _agents.end()) navData->crowd->resetMoveTarget(agent->id);
}

void RecastManager::removeAgent(Agent* agent)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->crowd) { OSG_WARN << "[RecastManager] Crowd not created" << std::endl; return; }
    if (_agents.find(agent) != _agents.end())
    { _agents.erase(_agents.find(agent)); navData->crowd->removeAgent(agent->id); }
}

void RecastManager::clearAllAgents()
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->crowd) { OSG_WARN << "[RecastManager] Crowd not created" << std::endl; return; }

    for (std::set<osg::ref_ptr<Agent>>::iterator itr = _agents.begin();
         itr != _agents.end(); ++itr) navData->crowd->removeAgent((*itr)->id);
    _agents.clear();
}

void RecastManager::advance(float simulationTime)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->crowd) { OSG_WARN << "[RecastManager] Crowd not created" << std::endl; return; }
    if (_lastSimulationTime < 0.0f) { _lastSimulationTime = simulationTime; return; }

    float dt = simulationTime - _lastSimulationTime;
    navData->crowd->update(dt, &navData->agentDebugger);
    for (std::set<osg::ref_ptr<Agent>>::iterator itr = _agents.begin();
         itr != _agents.end(); ++itr)
    {
        Agent* agent = itr->get(); if (agent->id < 0) continue;
        const dtCrowdAgent* dt = navData->crowd->getAgent(agent->id);

        agent->position.set(dt->npos[0], -dt->npos[2], dt->npos[1]);
        agent->velocity.set(dt->vel[0], -dt->vel[2], dt->vel[1]);
        agent->state = dt->state | (dt->active ? 0xf0 : 0);
        if (agent->transform.valid())
        {
            osg::Vec3 dir = agent->velocity; bool canRotate = (dir.length2() > 0.01f);
            osg::Quat q; dir.normalize(); if (canRotate) q.makeRotate(osg::X_AXIS, dir);
            osg::MatrixTransform* mt = agent->transform->asMatrixTransform();
            osg::PositionAttitudeTransform* pat = agent->transform->asPositionAttitudeTransform();

            if (mt)
            {
                osg::Matrix m = mt->getMatrix(); if (canRotate) m.setRotate(q);
                m.setTrans(agent->position); mt->setMatrix(m);
            }
            else if (pat)
            {
                if (canRotate) pat->setAttitude(q);
                pat->setPosition(agent->position);
            }
        }
    }
    _lastSimulationTime = simulationTime;
}

std::vector<osg::Vec3> RecastManager::findPath(std::vector<int>& flags,
                                               const osg::Vec3& s, const osg::Vec3& e, const osg::Vec3& ex)
{
    std::vector<osg::Vec3> path;
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->navQuery) { OSG_WARN << "[RecastManager] nav-query not created" << std::endl; return path; }

    float start[3] = { s[0], s[2], -s[1] }, end[3] = { e[0], e[2], -e[1] }, end1[3];
    float extents[3] = { ex[0], ex[2], ex[1] }; dtPolyRef startRef, endRef;
    navData->navQuery->findNearestPoly(start, extents, navData->queryFilter, &startRef, NULL);
    navData->navQuery->findNearestPoly(end, extents, navData->queryFilter, &endRef, NULL);
    if (!startRef || !endRef) return path;

    int numPolys = 0, numPathPoints = 0; FindPathData& pathData = navData->pathData;
    navData->navQuery->findPath(startRef, endRef, start, end, navData->queryFilter,
                                pathData.polygons, &numPolys, MAX_POLYS);
    if (!numPolys) return path;

    end1[0] = end[0]; end1[1] = end[1]; end1[2] = end[2];
    if (pathData.polygons[numPolys - 1] != endRef)
        navData->navQuery->closestPointOnPoly(pathData.polygons[numPolys - 1], end, end1, NULL);
    navData->navQuery->findStraightPath(start, end1, pathData.polygons, numPolys,
                                        (float*)&pathData.pathPoints[0], pathData.pathFlags,
                                        pathData.pathPolygons, &numPathPoints, MAX_POLYS);
    for (int i = 0; i < numPathPoints; ++i)
    {
        const osg::Vec3& pos = pathData.pathPoints[i];
        path.push_back(osg::Vec3(pos[0], -pos[2], pos[1]));
        flags.push_back(pathData.pathFlags[i]);
    }
    return path;
}

bool RecastManager::hitWall(osg::Vec3& result, osg::Vec3& resultNormal,
                            const osg::Vec3& s, const osg::Vec3& e, const osg::Vec3& ex)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->navQuery) { OSG_WARN << "[RecastManager] nav-query not created" << std::endl; return false; }

    float start[3] = { s[0], s[2], -s[1] }, end[3] = { e[0], e[2], -e[1] };
    dtPolyRef startRef; float extents[3] = { ex[0], ex[2], ex[1] }, norm[3] = { 0.0f, -1.0f, 0.0f };
    navData->navQuery->findNearestPoly(start, extents, navData->queryFilter, &startRef, NULL);
    if (!startRef) { result = e; return false; }

    float t = 1.0f; int numPolys = 0; FindPathData& pathData = navData->pathData;
    navData->navQuery->raycast(startRef, start, end, navData->queryFilter, &t,
                               norm, pathData.polygons, &numPolys, MAX_POLYS);
    if (t == FLT_MAX) { result = e; return false; }
    resultNormal.set(norm[0], -norm[2], norm[1]);
    result = s * (1.0f - t) + e * t; return true;
}
