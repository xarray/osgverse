#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
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

bool RecastManager::build(osg::Node* node)
{
    MeshCollector collector; osg::Vec2i tStart, tEnd;
    collector.setWeldingVertices(true);
    collector.setUseGlobalVertices(true);
    if (!node) return false; else node->accept(collector);
    if (collector.getVertices().empty()) return false;

    osg::ComputeBoundsVisitor cbv; node->accept(cbv);
    osg::BoundingBox worldBounds = cbv.getBoundingBox();
    osg::Vec3 padding(_settings.padding * _settings.tileSize,
                      _settings.padding * _settings.tileSize, _settings.padding);
    worldBounds._min -= padding; worldBounds._max += padding;

    NavData* navData = static_cast<NavData*>(_recastData.get());
    int maxTiles = navData->calculateMaxTiles(
        worldBounds, tStart, tEnd, _settings.tileSize, _settings.cellSize);
    dtNavMeshParams params{};
    params.tileWidth = _settings.tileSize * _settings.cellSize;
    params.tileHeight = params.tileWidth;
    params.maxPolys = 1u << (22 - navData->logBaseTwo(maxTiles));
    params.maxTiles = maxTiles;
    navData->clear(); navData->navMesh = dtAllocNavMesh();

    if (dtStatusFailed(navData->navMesh->init(&params))) return false;
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
    ap.collisionQueryRange = ap.radius * 12.0f;
    ap.pathOptimizationRange = ap.radius * 30.0f;
    ap.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_TOPO |
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
        agent->velocity.set(dt->nvel[0], dt->nvel[2], -dt->nvel[1]);
    }
    agent->dirtyParams = false;
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
        agent->velocity.set(dt->nvel[0], -dt->nvel[2], dt->nvel[1]);
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
