#ifndef MANA_MODELING_UTILITIES_HPP
#define MANA_MODELING_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>

namespace osgVerse
{
    
    struct ConvexHull
    {
        std::vector<osg::Vec3> points;
        std::vector<unsigned int> triangles;
    };
    
    class MeshCollector : public osg::NodeVisitor
    {
    public:
        MeshCollector();
        inline void pushMatrix(osg::Matrix& matrix) { _matrixStack.push_back(matrix); }
        inline void popMatrix() { _matrixStack.pop_back(); }

        virtual void reset();
        virtual void apply(osg::Transform& transform);
        virtual void apply(osg::Geode& node);

        virtual void apply(osg::Node& node) { traverse(node); }
        virtual void apply(osg::Drawable& node) {}  // do nothing
        virtual void apply(osg::Geometry& geometry) {}  // do nothing
    
        const std::vector<osg::Vec3>& getVertices() const { return _vertices; }
        const std::vector<unsigned int>& getTriangles() const { return _indices; }

    protected:
        typedef std::vector<osg::Matrix> MatrixStack;
        MatrixStack _matrixStack;
        std::vector<osg::Vec3> _vertices;
        std::vector<unsigned int> _indices;
    };

    class BoundingVolumeVisitor : public MeshCollector
    {
    public:
        BoundingVolumeVisitor() : MeshCollector() {}
        
        /** Returned value is in OBB coordinates, using rotation to convert it */
        osg::BoundingBox computeOBB(osg::Quat& rotation, float relativeExtent = 0.1f, int numSamples = 500);

        /** Get a list of convex hulls to contain this node */
        bool computeKDop(std::vector<ConvexHull>& hulls, int maxConvexHulls = 24);
    };
    
    /** Create a geometry with specified arrays */
    extern osg::Geometry* createGeometry(osg::Vec3Array* va, osg::Vec3Array* na, osg::Vec2Array* ta,
                                         osg::PrimitiveSet* p, bool autoNormals = true, bool useVBO = false);

    extern osg::Geometry* createGeometry(osg::Vec3Array* va, osg::Vec3Array* na, const osg::Vec4& color,
                                         osg::PrimitiveSet* p, bool autoNormals = true, bool useVBO = false);

    /** Create a polar sphere (r1 = r2 = r3) or ellipsoid */
    extern osg::Geometry* createEllipsoid(const osg::Vec3& center, float radius1, float radius2,
                                          float radius3, int samples = 32);

    /** Create a superellipsoid (see http://paulbourke.net/geometry/spherical/) */
    extern osg::Geometry* createSuperEllipsoid(const osg::Vec3& center, float radius, float power1,
                                               float power2, int samples = 32);

    /** Create a prism (n > 3) or cylinder (n is large enough) */
    extern osg::Geometry* createPrism(const osg::Vec3& centerBottom, float radiusBottom, float radiusTop,
                                      float height, int n = 4, bool capped = true);

    /** Create a pyramid (n > 3) or cone (n is large enough) */
    extern osg::Geometry* createPyramid(const osg::Vec3& centerBottom, float radius, float height,
                                        int n = 4, bool capped = false);

    /** Create a view frustum geometry corresponding to given matrices */
    extern osg::Geometry* createViewFrustumGeometry(const osg::Matrix& view, const osg::Matrix& proj);

    /** Create a geodesic sphere which has well-distributed facets */
    extern osg::Geometry* createGeodesicSphere(const osg::Vec3& center, float radius, int iterations = 4);

    /** Create a soccer-like geometry named truncated icosahedron */
    extern osg::Geometry* createSoccer(const osg::Vec3& center, float radius);

    /** Create a textured icosahedron for panorama use */
    extern osg::Geometry* createPanoramaSphere(int subdivs = 2);

}

#endif
