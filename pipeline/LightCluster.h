#ifndef MANA_PP_LIGHT_CLUSTER_HPP
#define MANA_PP_LIGHT_CLUSTER_HPP

#include <osg/Referenced>
#include <osg/Matrix>
#include <osg/Vec2>
#include <osg/Vec2s>
#include <osg/Vec3i>
#include <osg/TextureBuffer>
#include <osg/Viewport>
#include <vector>
#include "Pipeline.h"

namespace osgVerse
{
    /** CPU-side clustered light culling (the "light cluster" management of the deferred pipeline).

        The view frustum is divided into a 3D grid of clusters: a 2D grid of screen tiles times a
        set of exponentially distributed depth slices. Every frame, each light is turned into a
        view-space bounding box and assigned to the clusters it may affect, so that the deferred
        lighting shader only has to loop over the lights of the cluster a pixel belongs to instead
        of walking the whole light table.

        The cluster assignment is accelerated with modeling/Octree.h: the local (bounded) lights
        are inserted into a BoundsOctree in view space, and every cluster queries it with its own
        view-space AABB. Lights without a finite range (directional lights, and point/spot lights
        whose attenuation range is 0) can not be bounded this way, so they are kept in a small
        "global" list which is prepended to every cluster.

        The result is uploaded to two texture buffers (the TBO mode of modeling/GaussianGeometry):
        - a light-index buffer: a flat list of indices into the light parameter table, grouped by
          cluster (a float per entry, as it is read with texelFetch())
        - a cluster buffer: (offset, count) of the index range of each cluster, as a float pair

        Both texture buffer objects are created once, as the pipeline binds them to a stage state
        set at stage-creation time: replacing the texture object later would leave the state set
        pointing at the old one. Their images, however, are (re)allocated on demand - the size one
        needs depends on how many lights touch how many clusters, which is a property of the scene
        - so the memory follows the workload instead of a worst case guess */
    class LightCluster : public osg::Referenced
    {
    public:
        enum { MaxLightsPerCluster = 256,   // must match MAX_CLUSTER_LIGHTS in the shader
               MaxIndexEntries = 1 << 21 }; // hard upper bound of the index list (8MB)

        /** One light to be clustered. eyePosition is in view space (the same space the light
            parameter table uses), range <= 0 means the light reaches everywhere */
        struct Light
        {
            int index;              // index in the light parameter table
            osg::Vec3 eyePosition;
            float range;
            bool directional;

            Light() : index(-1), range(0.0f), directional(false) {}
            Light(int i, const osg::Vec3& p, float r, bool d = false)
                : index(i), eyePosition(p), range(r), directional(d) {}
        };

        LightCluster(int tileSize = 64, int numSlices = 24);

        /** Screen tile size in pixels (only square tiles are supported) */
        void setTileSize(int pixels) { _tileSize = osg::maximum(4, pixels); }
        int getTileSize() const { return _tileSize; }

        /** Wanted number of depth slices; the real number may be reduced to fit the buffers */
        void setNumSlices(int n) { _numSlices = osg::maximum(1, n); }
        int getNumSlices() const { return _numSlices; }

        bool getEnabled() const { return _enabled; }
        void setEnabled(bool b) { _enabled = b; }

        /** Rebuild the cluster assignment for the current frame.
            - projMatrix: projection matrix used by the G-buffer (only its (0,0) and (1,1) terms
                          are read, as they define the frustum shape)
            - viewport:   screen size the shader uses for gl_FragCoord
            - nearZ/farZ: depth range the slices are distributed over; the same values must be
                          given to the shader (see getNearFar()) so both sides agree
            - lights:     all lights of the frame, in table order (index is the table index)
            Returns true if the assignment was built (false keeps the previous contents) */
        bool build(const osg::Matrix& projMatrix, const osg::Vec2s& viewport,
                   float nearZ, float farZ, const std::vector<Light>& lights);

        /** GPU buffers and their uniforms, valid from construction on */
        osg::TextureBuffer* getIndexBuffer() { return _indexBuffer.get(); }
        osg::TextureBuffer* getClusterBuffer() { return _clusterBuffer.get(); }
        const osg::TextureBuffer* getIndexBuffer() const { return _indexBuffer.get(); }
        const osg::TextureBuffer* getClusterBuffer() const { return _clusterBuffer.get(); }

        const osg::Vec3i& getGridSize() const { return _gridSize; }   // (tilesX, tilesY, slices)
        const osg::Vec2& getTilePixels() const { return _tilePixels; }
        const osg::Vec2& getNearFar() const { return _nearFar; }

        int getNumClusters() const { return _gridSize.x() * _gridSize.y() * _gridSize.z(); }
        int getNumIndexEntries() const { return _numIndexEntries; }

    protected:
        virtual ~LightCluster();
        void ensureIndexCapacity(int needed);
        void ensureClusterCapacity(int needed);

        osg::ref_ptr<osg::TextureBuffer> _indexBuffer, _clusterBuffer;
        osg::ref_ptr<osg::Image> _indexImage, _clusterImage;
        std::vector<float> _indexScratch, _clusterScratch;
        osg::Vec3i _gridSize; osg::Vec2 _tilePixels, _nearFar;
        int _tileSize, _numSlices, _numIndexEntries;
        int _indexCapacity, _clusterCapacity;
        unsigned long long _lastSignature;  // inputs of the last build (skip it when unchanged)
        bool _enabled, _capacityWarned;
    };
}

#endif
