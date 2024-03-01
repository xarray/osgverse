#include "MeshDeformer.h"
#include "3rdparty/laplacian_deformation.hpp"
using namespace osgVerse;
using namespace osgVerse::helper;

MeshDeformer::MeshDeformer()
    : _boundaryBegin(-1), _updatingCount(-1)
{
}

MeshDeformer::~MeshDeformer()
{
    if (_updatingCount >= 0) freeDeform();
}

void MeshDeformer::initialize(const std::vector<vec3>& pos, const std::vector<int>& cells)
{
    // Compute shared vertex & cells
    std::map<vec3, std::vector<int>> sharedPosMap;
    _positions0 = pos; _sharedIndexMap.clear(); _sharedToOriginIndexMap.clear();
    for (size_t i = 0; i < pos.size(); ++i) sharedPosMap[pos[i]].push_back((int)i);
    
    std::vector<vec3> newPos; std::vector<int> newCells;
    for (std::map<vec3, std::vector<int>>::iterator itr = sharedPosMap.begin();
         itr != sharedPosMap.end(); ++itr)
    {
        int ptIndex = (int)newPos.size(); newPos.push_back(itr->first);
        for (size_t i = 0; i < itr->second.size(); ++i)
        {
            _sharedToOriginIndexMap[ptIndex].push_back(itr->second[i]);
            _sharedIndexMap[itr->second[i]] = ptIndex;
        }
    }
    for (size_t i = 0; i < cells.size(); ++i)
        newCells.push_back(_sharedIndexMap[cells[i]]);

    // Set variables
    if (_updatingCount >= 0) freeDeform();
    _positions = newPos; _cells = newCells; _updatingCount = -1;
    _adjacancies = computeAdjacancy((int)_positions.size(), _cells);
}

bool MeshDeformer::setHandle(int mainHandle, int handleSize, int unconstrainedSize)
{
    if (_updatingCount >= 0)
    { _handles.clear(); _unconstrained.clear(); freeDeform(); }
    
    int numVertices = (int)_positions.size();
    if (numVertices == 0) return false; else _updatingCount = 0;

    std::vector<bool> unconstrainedSet(numVertices, false);
    std::vector<bool> handlesSet(numVertices, false);
    std::vector<bool> visited(numVertices, false);
    std::vector<int> currentRing;
    currentRing.push_back(_sharedIndexMap[mainHandle]);

    for (int k = 0; k < handleSize; ++k)
    {
        std::vector<int> nextRing;
        for (int i = 0; i < currentRing.size(); ++i)
        {
            int e = currentRing[i]; if (visited[e]) continue;
            visited[e] = true; handlesSet[e] = true;
            _handles.push_back(e);

            const std::vector<int>& adjs = _adjacancies[e];
            for (int j = 0; j < adjs.size(); ++j) nextRing.push_back(adjs[j]);
        }
        currentRing = nextRing;
    }

    for (int k = 0; k < unconstrainedSize; ++k)
    {
        std::vector<int> nextRing;
        for (int i = 0; i < currentRing.size(); ++i)
        {
            int e = currentRing[i]; if (visited[e]) continue;
            visited[e] = true; unconstrainedSet[e] = true;
            _unconstrained.push_back(e);

            const std::vector<int>& adjs = _adjacancies[e];
            for (int j = 0; j < adjs.size(); ++j) nextRing.push_back(adjs[j]);
        }
        currentRing = nextRing;
    }

    _boundaryBegin = (int)_handles.size();
    for (int i = 0; i < currentRing.size(); ++i)
    {
        int e = currentRing[i]; if (visited[e]) continue;
        _handles.push_back(e); visited[e] = true;
    }

    // Prepare deformation
    int unconstrainedBegin = (int)_handles.size(); _combined.clear();
    _combined.insert(_combined.end(), _handles.begin(), _handles.end());
    _combined.insert(_combined.end(), _unconstrained.begin(), _unconstrained.end());
    prepareDeform(_cells.data(), (int)_cells.size(),
                  (double*)_positions.data(), numVertices * 3,
                  _combined.data(), (int)_combined.size(), unconstrainedBegin, true);
    return true;
}

bool MeshDeformer::updateDeformation(double dx, double dy, double dz)
{
    std::vector<double> inputData(_handles.size() * 3);
    if (_boundaryBegin < 0 || _handles.empty()) return false;

    int j = 0, handleSize = (int)_handles.size();
    for (int i = 0; i < _boundaryBegin; ++i)
    {
        inputData[j * 3 + 0] = _positions[_handles[i]].x + dx;
        inputData[j * 3 + 1] = _positions[_handles[i]].y + dy;
        inputData[j * 3 + 2] = _positions[_handles[i]].z + dz; ++j;
    }

    for (int i = _boundaryBegin; i < handleSize; ++i)
    {
        inputData[j * 3 + 0] = _positions[_handles[i]].x;
        inputData[j * 3 + 1] = _positions[_handles[i]].y;
        inputData[j * 3 + 2] = _positions[_handles[i]].z; ++j;
    }
    
    // Compute deformation
    doDeform(&inputData[0], handleSize, (double*)_positions.data());

    // Reconstruct original vertex list
    for (std::map<int, int>::iterator itr = _sharedIndexMap.begin();
         itr != _sharedIndexMap.end(); ++itr)
    { _positions0[itr->first] = _positions[itr->second]; }
    _updatingCount++; return true;
}

void MeshDeformer::getHandles(std::vector<int>& handles, std::vector<int>& unconstrained)
{
    handles.clear(); unconstrained.clear();
    for (size_t i = 0; i < _handles.size(); ++i)
    {
        std::vector<int>& vl = _sharedToOriginIndexMap[_handles[i]];
        handles.insert(handles.end(), vl.begin(), vl.end());
    }

    for (size_t i = 0; i < _unconstrained.size(); ++i)
    {
        std::vector<int>& vl = _sharedToOriginIndexMap[_unconstrained[i]];
        unconstrained.insert(unconstrained.end(), vl.begin(), vl.end());
    }
}

const std::vector<vec3>& MeshDeformer::getPositions(bool original) const
{ return original ? _positions0 : _positions; }

MeshDeformer::AdjacancyList MeshDeformer::computeAdjacancy(int numVertices, const std::vector<int>& cells)
{
    std::vector<std::vector<int>> adj;
    for (int i = 0; i < numVertices; ++i)
        adj.push_back(std::vector<int>());

    for (int i = 0; i < cells.size(); i += 3)
        for (int j = 0; j < 3; ++j)
        {
            int a = cells[i + (j + 0)];
            int b = cells[i + ((j + 1) % 3)];
            adj[a].push_back(b);
        }
    return adj; 
}
