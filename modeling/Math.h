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
#include <osg/CoordinateSystemNode>

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

    /** Get euler angles in HPR order from direction and up vectors */
    extern osg::Vec3d computeHPRFromMatrix(const osg::Matrix& rotation);

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

    /** Point cloud querying manager, used for finding closest points */
    class PointCloudQuery
    {
    public:
        typedef std::pair<uint32_t, float> IndexAndDistancePair;
        typedef std::pair<osg::Vec3, osg::ref_ptr<osg::Referenced>> PointData;

        PointCloudQuery();
        ~PointCloudQuery();

        void addPoint(const osg::Vec3& pt, osg::Referenced* userData);
        void setPoints(const std::vector<PointData>& data);

        unsigned int getNumPoints() const;
        const std::vector<PointData>& getPoints() const;

        /** Build the KDTree index for point cloud */
        void buildIndex(int maxLeafSize = 10);

        /** Find nearest neighbors of specific point */
        float findNearest(const osg::Vec3& pt, std::vector<uint32_t>& resultIndices,
                          unsigned int maxResults = 1000);

        /** Find points inside the radius of specific point */
        int findInRadius(const osg::Vec3& pt, float radius, std::vector<IndexAndDistancePair>& resultIndices);

    protected:
        void* _queryData;
        void* _index;
    };

    /** A set of transformation functions between coordinate systems
        Information about spatial reference systems
        - [EPSG:4326] Geographic coordinate system (LLA / geodetic)
        - [EPSG:4978] Geocentric coordinate system (Earth-centered Earth-fixed, ECEF)
        - [EPSG:3857] Web Mercator / Spherical Mercator
        - [EPSG:32601-32660] for UTM Northern, [EPSG:32701-32760] for UTM Southern
    */
    struct Coordinate
    {
        inline osg::Vec3d translateRHtoLH(const osg::Vec3d& v) { return osg::Vec3d(-v[1], v[2], v[0]); }
        inline osg::Vec3d translateLHtoRH(const osg::Vec3d& v) { return osg::Vec3d(v[2], -v[0], v[1]); }
        inline osg::Vec3d scaleRHtoLH(const osg::Vec3d& v) { return osg::Vec3d(v[1], v[2], v[0]); }
        inline osg::Vec3d scaleLHtoRH(const osg::Vec3d& v) { return osg::Vec3d(v[2], v[0], v[1]); }
        inline osg::Quat rotateRHtoLH(const osg::Quat& q) { return osg::Quat(q[1], -q[2], -q[0], q[3]); }
        inline osg::Quat rotateLHtoRH(const osg::Quat& q) { return osg::Quat(-q[2], q[0], -q[1], q[3]); }

        struct WGS84
        {
            double radiusEquator, radiusPolar, eccentricitySq;
            WGS84(double radiusE = osg::WGS_84_RADIUS_EQUATOR, double radiusP = osg::WGS_84_RADIUS_POLAR);
        };

        struct UTM
        {
            // https://github.com/isce-framework/isce3/blob/develop/cxx/isce3/core/Projections.cpp
            double cgb[6], cbg[6], utg[6], gtu[6], lon0, Qn, Zb;
            int zone; bool isNorth; UTM(int code, const WGS84& wgs84 = WGS84());
            static double clenshaw(const double* a, int size, double real);
            static double clenshaw2(const double* a, int size, double real, double imag, double& R, double& I);
        };

        /// Geodetic: latitude and longitude in radius, altitude in metres; ECEF: coords in metres
        static osg::Vec3d convertLLAtoECEF(const osg::Vec3d& lla, const WGS84& wgs84 = WGS84());

        /// Geodetic: latitude and longitude in radius, altitude in metres; ECEF: coords in metres
        static osg::Vec3d convertECEFtoLLA(const osg::Vec3d& ecef, const WGS84& wgs84 = WGS84());

        /// Geodetic: latitude and longitude in radius, altitude in metres; Web Mercator: coords in metres
        static osg::Vec3d convertLLAtoWebMercator(const osg::Vec3d& lla, const WGS84& wgs84 = WGS84());

        /// Geodetic: latitude and longitude in radius, altitude in metres; Web Mercator: coords in metres
        static osg::Vec3d convertWebMercatorToLLA(const osg::Vec3d& yxz, const WGS84& wgs84 = WGS84());

        /// Geodetic: latitude and longitude in radius, altitude in metres; UTM: coords in metres
        static osg::Vec3d convertLLAtoUTM(const osg::Vec3d& lla,
                                          const UTM& utm, const WGS84& wgs84 = WGS84());

        /// Geodetic: latitude and longitude in radius, altitude in metres; UTM: coords in metres
        static osg::Vec3d convertUTMtoLLA(const osg::Vec3d& coord,
                                          const UTM& utm, const WGS84& wgs84 = WGS84());

        /// Geodetic: latitude and longitude in radius, altitude in metres; ENU: east-north-up
        static osg::Matrix convertLLAtoENU(const osg::Vec3d& lla, const WGS84& wgs84 = WGS84());

        /// Geodetic: latitude and longitude in radius, altitude in metres; NED: north-east-down
        static osg::Matrix convertLLAtoNED(const osg::Vec3d& lla, const WGS84& wgs84 = WGS84());
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

        /** Compute intersections of a 2D line and another */
        static PointList2D intersectionWithLine2D(const LineType2D& l0, const LineType2D& l1);

        /** Compute intersections of a 2D line and a 2D polygon */
        static PointList2D intersectionWithPolygon2D(const LineType2D& l, const PointList2D& polygon);

        /** Decompose a concave polygon into multiple convex polygons and return splitting edges */
        static std::vector<LineType2D> decomposePolygon2D(const PointList2D& polygon);

        /** Compute the pole of inaccessibility coordinate of a polygon.
            It is the most distant internal point from the polygon outline (not centroid) */
        static osg::Vec2d getPoleOfInaccessibility(const PointList2D& polygon, double precision = 1.0);

        /** Compute center of geometry / mass of a polygon */
        static osg::Vec2d getCentroid(const PointList2D& polygon, bool centerOfMass);

        /** Check for clockwise/counter-clockwise */
        static bool clockwise2D(const PointList2D& points);

        /** Reorder a list of 2D hull points on a plane */
        static bool reorderPointsInPlane(PointList2D& points, bool usePoleOfInaccessibility = true,
                                         const std::vector<osgVerse::EdgeType>& edges = {});

        /** Delaunay triangulation (with/without auto-detected boundaries and holes) */
        static std::vector<size_t> delaunayTriangulation(
                const PointList2D& points, const EdgeList& edges, bool allowEdgeIntersection = false);
    };

}

#endif
