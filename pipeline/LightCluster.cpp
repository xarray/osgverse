#include <osg/Math>
#include <osg/BoundingBox>
#include <osg/Image>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <iostream>
#include "LightCluster.h"
#include "modeling/Octree.h"
using namespace osgVerse;

namespace
{
    /** Just a wrapper carrying the light-table index, so that the octree (which stores
        osg::ref_ptr<T> objects) can be queried and the result mapped back to the table */
    struct BoundedLight : public osg::Referenced
    {
        BoundedLight(int i) : index(i) {}
        int index;
    };

    // FNV-1a over the raw bits of the build inputs: cheap enough to run every frame, and it
    // lets a still frame (fixed camera and lights) reuse the previous cluster assignment
    inline void hashBits(unsigned long long& h, const void* data, size_t size)
    {
        const unsigned char* p = (const unsigned char*)data;
        for (size_t i = 0; i < size; ++i) { h ^= (unsigned long long)p[i]; h *= 1099511628211ULL; }
    }
    inline void hashFloat(unsigned long long& h, float v) { hashBits(h, &v, sizeof(float)); }
    inline void hashInt(unsigned long long& h, int v) { hashBits(h, &v, sizeof(int)); }
}

LightCluster::LightCluster(int tileSize, int numSlices)
:   _tileSize(osg::maximum(4, tileSize)), _numSlices(osg::maximum(1, numSlices)),
    _numIndexEntries(0), _indexCapacity(0), _clusterCapacity(0), _lastSignature(0),
    _enabled(true), _capacityWarned(false)
{
    _gridSize.set(0, 0, 0); _tilePixels.set(1.0f, 1.0f); _nearFar.set(0.1f, 1000.0f);

    // Only the texture objects are created here: the pipeline binds them to a stage state set at
    // stage-creation time, so they have to stay the same. The images behind them are (re)allocated
    // on demand by the ensure*Capacity() functions below
    _indexBuffer = new osg::TextureBuffer;
    _indexBuffer->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _indexBuffer->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _clusterBuffer = new osg::TextureBuffer;
    _clusterBuffer->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _clusterBuffer->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
}

LightCluster::~LightCluster() {}

void LightCluster::ensureIndexCapacity(int needed)
{
    // The image has to exist even for an empty list: the caller marks it dirty afterwards, and the
    // first build may well happen before any light was collected (an empty scene, or the first
    // frame), which would otherwise leave a null image behind
    if (needed <= _indexCapacity && _indexImage.valid()) return;
    int cap = 4096; while (cap < needed) cap <<= 1;

    // Light-index buffer: one float per entry, read with texelFetch() in the lighting shader
    _indexImage = new osg::Image;
    _indexImage->allocateImage(cap, 1, 1, GL_RED, GL_FLOAT);
    _indexImage->setInternalTextureFormat(GL_R32F);
    memset(_indexImage->data(), 0, _indexImage->getTotalSizeInBytes());
    _indexBuffer->setImage(_indexImage.get());

    // The image of a buffer texture can not be replaced without invalidating the GL texture object:
    // osg::TextureBuffer attaches the buffer object to the texture with glTexBuffer() only when the
    // texture object is created, while setBufferData() -- which setImage() goes through -- swaps
    // that buffer object and resets its per-context modified count without touching the texture
    // object. The texture would then keep sampling the previous, smaller buffer for the rest of the
    // run, so the shader reads stale light indices as soon as the light list outgrows the first
    // allocation (which shows up as cluster-shaped patches that never go away again)
    _indexBuffer->dirtyTextureObject();
    _indexCapacity = cap;
}

void LightCluster::ensureClusterCapacity(int needed)
{
    if (needed <= _clusterCapacity && _clusterImage.valid()) return;
    int cap = 1024; while (cap < needed) cap <<= 1;

    // Cluster buffer: (offset, count) as a float pair per cluster
    _clusterImage = new osg::Image;
    _clusterImage->allocateImage(cap, 1, 1, GL_RG, GL_FLOAT);
    _clusterImage->setInternalTextureFormat(GL_RG32F);
    memset(_clusterImage->data(), 0, _clusterImage->getTotalSizeInBytes());
    _clusterBuffer->setImage(_clusterImage.get());
    _clusterBuffer->dirtyTextureObject();  // same reason as the index buffer above
    _clusterCapacity = cap;
}

bool LightCluster::build(const osg::Matrix& projMatrix, const osg::Vec2s& viewport,
                         float nearZ, float farZ, const std::vector<Light>& lights)
{
    if (!_enabled || !_indexBuffer.valid() || !_clusterBuffer.valid()) return false;

    int viewW = viewport.x(), viewH = viewport.y();
    if (viewW <= 0 || viewH <= 0) return false;

    // The (0,0) and (1,1) terms of the projection define the frustum shape; the (3,2) term
    // tells a perspective projection from an orthographic one
    double p00 = projMatrix(0, 0), p11 = projMatrix(1, 1);
    if (p00 == 0.0 || p11 == 0.0) return false;
    bool perspective = (fabs(projMatrix(3, 2)) > 1e-6);
    if (farZ <= nearZ) farZ = nearZ * 1000.0f;

    // Nothing the assignment depends on (frustum shape, viewport, grid and lights) changed since
    // the last build: the buffers still hold the right data, so skip both the CPU work and the
    // upload. A still frame is the common case for an editor or a paused application
    unsigned long long signature = 1469598103934665603ULL;
    hashFloat(signature, (float)p00); hashFloat(signature, (float)p11);
    hashFloat(signature, perspective ? 1.0f : 0.0f);
    hashFloat(signature, nearZ); hashFloat(signature, farZ);
    hashInt(signature, viewW); hashInt(signature, viewH);
    hashInt(signature, _tileSize); hashInt(signature, _numSlices);
    hashInt(signature, (int)lights.size());
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const Light& l = lights[i];
        hashInt(signature, l.index);
        hashFloat(signature, l.eyePosition.x());
        hashFloat(signature, l.eyePosition.y());
        hashFloat(signature, l.eyePosition.z());
        hashFloat(signature, l.range);
        hashInt(signature, l.directional ? 1 : 0);
    }
    if (signature == _lastSignature && _clusterImage.valid()) return true;
    _lastSignature = signature;

    // 1) Cluster grid
    int tilesX = (viewW + _tileSize - 1) / _tileSize;
    int tilesY = (viewH + _tileSize - 1) / _tileSize;
    int slices = _numSlices, numClusters = tilesX * tilesY * slices;

    _gridSize.set(tilesX, tilesY, slices);
    _tilePixels.set((float)_tileSize, (float)_tileSize);
    _nearFar.set(nearZ, farZ);
    ensureClusterCapacity(numClusters);

    // 2) Split the lights: the unbounded ones (directional, or range <= 0) reach every cluster
    // and are kept in a small list prepended to all of them; the bounded ones go to the octree
    std::vector<int> globalIndices;
    osgVerse::BoundsOctree<BoundedLight> octree(
        osg::Vec3d(0.0, 0.0, 0.0), 64.0f, 1.0f, 1.0f, 8);
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const Light& l = lights[i];
        if (l.directional || !(l.range > 0.0f)) { globalIndices.push_back(l.index); continue; }

        osg::Vec3d c(l.eyePosition.x(), l.eyePosition.y(), l.eyePosition.z());
        osg::Vec3d ext(l.range, l.range, l.range);
        octree.add(new BoundedLight(l.index), osg::BoundingBoxd(c - ext, c + ext));
    }

    // 3) Walk the clusters and collect the affecting lights of each one. The light lists are
    // compacted into a flat array; each cluster only stores the (offset, count) of its range
    _clusterScratch.assign((size_t)numClusters * 2, 0.0f);
    _indexScratch.clear();
    int numGlobals = (int)globalIndices.size(), numDropped = 0;
    std::vector<osg::ref_ptr<BoundedLight>> hits;
    double sliceRatio = log((double)farZ / (double)nearZ);

    for (int sz = 0; sz < slices; ++sz)
    {
        double z0 = nearZ * exp(sliceRatio * (double)sz / (double)slices);
        double z1 = nearZ * exp(sliceRatio * (double)(sz + 1) / (double)slices);
        for (int ty = 0; ty < tilesY; ++ty)
        {
            float py0 = (float)(ty * _tileSize);
            float py1 = (float)osg::minimum((ty + 1) * _tileSize, viewH);
            float ny0 = py0 / viewH * 2.0f - 1.0f, ny1 = py1 / viewH * 2.0f - 1.0f;
            for (int tx = 0; tx < tilesX; ++tx)
            {
                float px0 = (float)(tx * _tileSize);
                float px1 = (float)osg::minimum((tx + 1) * _tileSize, viewW);
                float nx0 = px0 / viewW * 2.0f - 1.0f, nx1 = px1 / viewW * 2.0f - 1.0f;

                // View-space AABB of the cluster: 4 NDC corners at both slice depths. The AABB
                // is slightly enlarged so that a fragment on a cluster border (where the shader
                // and this code round differently) never loses the lights it should see
                osg::BoundingBoxd bb;
                for (int ci = 0; ci < 4; ++ci)
                {
                    float nx = (ci & 1) ? nx1 : nx0, ny = (ci & 2) ? ny1 : ny0;
                    for (int d = 0; d < 2; ++d)
                    {
                        double depth = (d == 0) ? z0 : z1;
                        float x = perspective ? (float)(nx * depth / p00) : (float)(nx / p00);
                        float y = perspective ? (float)(ny * depth / p11) : (float)(ny / p11);
                        bb.expandBy(osg::Vec3d(x, y, -depth));
                    }
                }

                osg::Vec3d c = bb.center(), ext = (bb._max - bb._min) * 0.5;
                bb._min = c - ext * 1.05; bb._max = c + ext * 1.05;

                int clusterID = (sz * tilesY + ty) * tilesX + tx;
                int offset = (int)_indexScratch.size(), count = 0;
                for (int g = 0; g < numGlobals; ++g)
                {
                    if ((int)_indexScratch.size() >= MaxIndexEntries) { numDropped++; break; }
                    _indexScratch.push_back((float)globalIndices[g]); count++;
                }

                hits.clear();
                octree.getRoot().getColliding(bb, hits);
                for (size_t h = 0; h < hits.size(); ++h)
                {
                    if ((int)_indexScratch.size() >= MaxIndexEntries) { numDropped++; break; }
                    _indexScratch.push_back((float)hits[h]->index); count++;
                }
                _clusterScratch[clusterID * 2 + 0] = (float)offset;
                _clusterScratch[clusterID * 2 + 1] = (float)osg::minimum(count, (int)MaxLightsPerCluster);
            }
        }
    }

    if (numDropped > 0 && !_capacityWarned)
    {
        OSG_WARN << "[LightCluster] Light-index list reached its upper bound ("
                 << (int)MaxIndexEntries << " entries); " << numDropped << " assignments were "
                 << "dropped. Consider a larger cluster tile size or fewer lights." << std::endl;
        _capacityWarned = true;
    }

    // 4) Upload: the images are resized only when the workload grows, and only the used part
    // of the index list is copied (the tail of the buffer is never read by the shader)
    int numIndexEntries = (int)_indexScratch.size();
    ensureIndexCapacity(numIndexEntries);
    if (numIndexEntries > 0)
        memcpy(_indexImage->data(), &_indexScratch[0], sizeof(float) * numIndexEntries);
    memcpy(_clusterImage->data(), &_clusterScratch[0], sizeof(float) * _clusterScratch.size());

    _numIndexEntries = numIndexEntries;
    _indexImage->dirty(); _clusterImage->dirty();
    return true;
}
