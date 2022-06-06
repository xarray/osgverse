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

    typedef std::pair<osg::Vec2, osg::Vec2> LineType2D;
    typedef std::pair<osg::Vec3, osg::Vec3> LineType3D;
    typedef std::vector<osg::Vec2> PointList2D;
    typedef std::vector<osg::Vec3> PointList3D;
    typedef std::vector<osg::Plane> PlaneList;

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

    /** Spline helpers struct */
    struct Spline
    {
        /** Create 2D quadratic Bezier */
        static PointList2D createQuadraticBezier(const PointList2D& controls, unsigned int samples = 32);

        /** Create 3D quadratic Bezier */
        static PointList3D createQuadraticBezier(const PointList3D& controls, unsigned int samples = 32);

        /** Create 2D cubic Bezier */
        static PointList2D createCubicBezier(const PointList2D& controls, unsigned int samples = 32);

        /** Create 3D cubic Bezier */
        static PointList3D createCubicBezier(const PointList3D& controls, unsigned int samples = 32);
    };

    /** Intersection computations */
    struct Intersection
    {
        /** 2D line & 2D line */
        static bool lineWithLine2D(const LineType2D& line1, const LineType2D& line2, osg::Vec2& result);

        /** 3D line & 3D plane */
        static bool lineWithPlane3D(const LineType3D& line, const osg::Plane& plane, osg::Vec3& result);
    };

    /** Computational geometry helpers struct */
    struct GeometryAlgorithm
    {
        /** Containment computations */
        static bool pointInPolygon2D(const osg::Vec2& p, const PointList2D& polygon, bool isConvex);

        /** Decompose a concave polygon into multiple convex polygons and return splitting edges */
        static std::vector<LineType2D> decomposePolygon2D(const PointList2D& polygon);

        /** Closest point computations */
        static osg::Vec2 closestPointOnLine2D(const osg::Vec2& p, const LineType2D& line);
        static osg::Vec3 closestPointOnLine3D(const osg::Vec3& p, const LineType3D& line);
        static osg::Vec3 closestPointOnBoxAABB(const osg::Vec3& p, const osg::Vec3& boxMin,
                                               const osg::Vec3& boxMax);

        /** Mirror polygon along specified axis */
        static void mirror2D(const PointList2D& points, PointList2D& output,
                             const LineType2D& axis, float ratio = 1.0f);
        static void mirror3D(const PointList3D& points, PointList3D& output,
                             const osg::Plane& plane, float ratio = 1.0f);

        enum ConvexHullMethod
        {
            GRAHAM_SCAN /* O(nlogn) */, JARVIS_MARCH /* O(nh) */, MELKMAN /* O(n), needs ordered points */
        };

        /** Compute 2D convex hull (indices) of a list of points */
        static bool convexHull2D(const PointList2D& points, std::vector<int>& result,
                                 ConvexHullMethod method = JARVIS_MARCH);

        /** Compute 3D convex hull (indices of all triangles) of a list of points */
        //TODO: static bool convexHull3D( const PointList3D& points, std::vector<int>& result );

        /** Compute 2D intersections of a list of segments (start/end points) */
        static bool segmentIntersection2D(const PointList2D& points, PointList2D& result);

        /** Compute 3D intersections of a list of segments (start/end points) */
        static bool segmentIntersection3D(const PointList3D& points, PointList3D& result);

        /** Check for clockwise/counter-clockwise */
        static bool clockwise2D(const PointList2D& points);

        /** Reset points as an ordered polygon */
        static bool reorderPolygon2D(PointList2D& points);
    };

}

#endif
