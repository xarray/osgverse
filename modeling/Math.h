#ifndef MANA_MODELING_MATH_HPP
#define MANA_MODELING_MATH_HPP

#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <array>

#include <osg/io_utils>
#include <osg/Math>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Quat>
#include <osg/Plane>
#include <osg/Matrix>
#include <osg/Node>
#include <osg/Shape>

namespace osgVerse
{

    typedef std::pair<osg::Vec2d, osg::Vec2d> LineType2D;
    typedef std::pair<osg::Vec3d, osg::Vec3d> LineType3D;
    typedef std::pair<osg::Vec2d, size_t> PointType2D;
    typedef std::pair<size_t, size_t> EdgeType;
    typedef std::vector<PointType2D> PointList2D;
    typedef std::vector<osg::Vec3d> PointList3D;
    typedef std::vector<osg::Plane> PlaneList;
    typedef std::vector<EdgeType> EdgeList;
    struct MathExpressionPrivate;

    template <class T>
    inline T interpolate(const T& start, const T& end, float percent)
    { return static_cast<T>(start + (end - start) * percent); }

    /** Get euler angles in HPR order from a quaternion */
    extern osg::Vec3d computeHPRFromQuat(const osg::Quat& quat);

    /** Compute a power-of-two value according to current one */
    extern int computePowerOfTwo(int s, bool findNearest);

    /** Create round corner at specified pos of the input vector list,
        adding some points (defined by samples) */
    extern bool createRoundCorner(PointList3D& va, unsigned int pos, float radius,
                                  unsigned int samples = 12);

    /** Compute rotation angle and axis from one vector to another */
    extern float computeRotationAngle(const osg::Vec3& v1, const osg::Vec3& v2, osg::Vec3& axis);

    /** Compute area of a 3D polygon composited of points */
    extern float computeArea(const PointList3D& points, const osg::Vec3& normal);

    /** Compute area of any triangle using Heron's Formula */
    extern float computeTriangleArea(float a, float b, float c);

    /** Compute standard deviation */
    extern float computeStandardDeviation(const std::vector<float>& values);

    /** Compute perspective matrix from horizontal and vertical FOVs */
    extern osg::Matrix computePerspectiveMatrix(double hfov, double vfov, double zn, double zf);

    /** Compute perspective matrix from OpenCV intrinsic camera matrix
        See: http://www.info.hiroshima-cu.ac.jp/~miyazaki/knowledge/teche0092.html
    */
    extern osg::Matrix computePerspectiveMatrix(double focalX, double focalY,
                                                double centerX, double centerY, double zn, double zf);

    /** Change an existing perspective matrix to an infinite one (not for displaying use) */
    extern osg::Matrix computeInfiniteMatrix(const osg::Matrix& proj, double zn);

    /** Obtain near/far value from a specified projection matrix */
    extern void retrieveNearAndFar(const osg::Matrix& projectionMatrix, double& znear, double& zfar);

    /** Compute result of a numeric expression */
    class MathExpression
    {
        friend struct MathExpressionPrivate;
    public:
        MathExpression(const std::string& exp);
        ~MathExpression();

        void setVariable(const std::string& name, double& value);
        void setVariable(const std::string& name, const double& value);
        double evaluate(bool* ok = NULL);

    protected:
        MathExpressionPrivate* _private;
        std::string _expressionString;
        bool _compiled;
    };

    /** Computational geometry helpers struct */
    struct GeometryAlgorithm
    {
        /** Project a list of 3D points on a plane to 2D and return the transform matrix */
        static osg::Matrix project(const PointList3D& points, const osg::Vec3d& planeNormal,
                                   const osg::Vec3d& planeUp, PointList2D& pointsOut);

        /** Convenient method to convert edges to 3D vertices, 2D projections and edge indices */
        static EdgeList project(const std::vector<LineType3D>& edges, const osg::Vec3d& planeNormal,
                                PointList3D& points, PointList2D& points2D);
        
        /** Containment computations */
        static bool pointInPolygon2D(const osg::Vec2d& p, const PointList2D& polygon, bool isConvex);

        /** Compute intersections of a 2D line and a 2D polygon */
        static PointList2D intersectionWithPolygon2D(const LineType2D& l, const PointList2D& polygon);

        /** Decompose a concave polygon into multiple convex polygons and return splitting edges */
        static std::vector<LineType2D> decomposePolygon2D(const PointList2D& polygon);

        /** Compute the pole of inaccessibility coordinate of a polygon.
            It is the most distant internal point from the polygon outline (not centroid) */
        static osg::Vec2d getPoleOfInaccessibility(const PointList2D& polygon, double precision = 1.0);

        /** Check for clockwise/counter-clockwise */
        static bool clockwise2D(const PointList2D& points);

        /** Reorder a list of 2D hull points on a plane */
        static bool reorderPointsInPlane(PointList2D& points);

        /** Delaunay triangulation (with/without auto-detected boundaries and holes) */
        static std::vector<size_t> delaunayTriangulation(
                const PointList2D& points, const EdgeList& edges, bool allowEdgeIntersection = false);
    };

}

#endif
