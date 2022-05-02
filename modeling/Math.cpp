#include <algorithm>
#include <functional>
#include <iostream>
#include <cstdio>

#include <exprtk.hpp>
#include <nanoflann.hpp>
#include <wykobi/wykobi.hpp>
#include <wykobi/wykobi_algorithm.hpp>
#include "Math.h"
const float ZERO_TOLERANCE = float(1e-5);

namespace wykobiUtil
{

    static osg::Vec2 convToVec2(const wykobi::point2d<wykobi::Float>& pt)
    { return osg::Vec2(pt.x, pt.y); }

    static wykobi::point2d<wykobi::Float> convToPoint2D(const osg::Vec2& pt)
    { wykobi::point2d<wykobi::Float> p; p.x = pt[0]; p.y = pt[1]; return p; }

    static osg::Vec3 convToVec3(const wykobi::point3d<wykobi::Float>& pt)
    { return osg::Vec3(pt.x, pt.y, pt.z); }

    static wykobi::point3d<wykobi::Float> convToPoint3D(const osg::Vec3& pt)
    { wykobi::point3d<wykobi::Float> p; p.x = pt[0]; p.y = pt[1]; p.z = pt[2]; return p; }

}

namespace osgVerse
{

    osg::Vec3d computeHPRFromQuat(const osg::Quat& quat)
    {
        // From: http://guardian.curtin.edu.au/cga/faq/angles.html 
        // Except OSG exchanges pitch & roll from what is listed on that page 
        double qx = quat.x(), qy = quat.y(), qz = quat.z(), qw = quat.w();
        double sqx = qx * qx, sqy = qy * qy, sqz = qz * qz, sqw = qw * qw;

        double term1 = 2.0 * (qx * qy + qw * qz);
        double term2 = sqw + sqx - sqy - sqz;
        double term3 = -2.0 * (qx * qz - qw * qy);
        double term4 = 2.0 * (qw * qx + qy * qz);
        double term5 = sqw - sqx - sqy + sqz;

        double heading = atan2(term1, term2), pitch = atan2(term4, term5), roll = asin(term3);
        return osg::Vec3d(heading, pitch, roll);
    }

    int computePowerOfTwo(int s, bool findNearest)
    {
        int powerOfTwo = 1;
        if (findNearest)
        {
            int lastPowerOfTwo = 1;
            while (powerOfTwo < s)
            {
                lastPowerOfTwo = powerOfTwo;
                powerOfTwo <<= 1;
            }
            return (s - lastPowerOfTwo < powerOfTwo - s) ? lastPowerOfTwo : powerOfTwo;
        }
        else
        {
            while (powerOfTwo < s) powerOfTwo <<= 1;
            return powerOfTwo;
        }
    }

    bool createRoundCorner(PointList3D& va, unsigned int pos, float radius, unsigned int samples)
    {
        if (pos == 0 || pos >= va.size() - 1)  // Vertex at the end
            return false;

        osg::Vec3 p0 = va[pos];
        osg::Vec3 dir1 = va[pos - 1] - p0; dir1.normalize();
        osg::Vec3 dir2 = va[pos + 1] - p0; dir2.normalize();
        osg::Vec3 dir0 = dir1 + dir2; dir0.normalize();

        // Compute the end points of the arc (pa & pb), and the circle center (po)
        osg::Vec3 axis = dir1 ^ dir2;
        float angle = atan2(axis.length(), dir1 * dir2);
        if (angle<0.01 || angle>osg::PI - 0.01)  // Invalid crease angle
            return false;

        float roundAngle = osg::PI - angle;
        float lenToRoundEnd = radius * tanf(roundAngle * 0.5f);
        float lenToRoundCenter = radius / cosf(roundAngle * 0.5f);
        osg::Vec3 pa = p0 + dir1 * lenToRoundEnd, pb = p0 + dir2 * lenToRoundEnd;
        osg::Vec3 po = p0 + dir0 * lenToRoundCenter;

        // Now use the endpoints and center to compute points on the arc
        PointList3D newVertices;
        newVertices.push_back(pa);
        dir1 = pa - po; dir1.normalize();
        dir2 = pb - po; dir2.normalize();
        axis = dir1 ^ dir2; axis.normalize();

        float delta = roundAngle / (float)samples;
        for (unsigned int i = 1; i < samples; ++i)
        {
            osg::Vec3 vec = osg::Quat(delta * (float)i, axis) * dir1;
            newVertices.push_back(vec * radius + po);
        }
        newVertices.push_back(pb);
        va.erase(va.begin() + pos);
        va.insert(va.begin() + pos, newVertices.begin(), newVertices.end());
        return true;
    }

    float computeRotationAngle(const osg::Vec3& v1, const osg::Vec3& v2, osg::Vec3& axis)
    {
        axis = v1 ^ v2;
        float angle = atan2(axis.length(), v1 * v2);
        axis.normalize();
        return angle;
    }

    float computeArea(const PointList3D& points, const osg::Vec3& normal)
    {
        float area = 0.0f;
        float an, ax, ay, az;
        int coord, i, j, k;
        int size = (int)points.size();

        // Select largest abs coordinate to ignore for projection
        ax = fabs(normal.x());
        ay = fabs(normal.y());
        az = fabs(normal.z());
        coord = 3;
        if (ax > ay) { if (ax > az) coord = 1; }
        else if (ay > az) coord = 2;

        // Compute area of the 2D projection
        for (i = 1, j = 2, k = 0; i <= size; ++i, ++j, ++k)
        {
            switch (coord)
            {
            case 1:
                area += points[i%size].y() * (points[j%size].z() - points[k%size].z());
                break;
            case 2:
                area += points[i%size].x() * (points[j%size].z() - points[k%size].z());
                break;
            case 3:
                area += points[i%size].x() * (points[j%size].y() - points[k%size].y());
                break;
            }
        }

        // Scale to get area before projection
        an = sqrt(ax*ax + ay * ay + az * az);
        switch (coord)
        {
        case 1: area *= (an / (2 * ax)); break;
        case 2: area *= (an / (2 * ay)); break;
        case 3: area *= (an / (2 * az)); break;
        }
        return area;
    }

    float computeTriangleArea(float a, float b, float c)
    {
        float s = (a + b + c) * 0.5f;
        return sqrt(s * (s - a) * (s - b) * (s - c));
    }

    float computeStandardDeviation(const std::vector<float>& values)
    {
        float deviation = 0.0f, avg = 0.0f, sizeF = (float)values.size();
        for (unsigned int i = 0; i < values.size(); ++i) avg += values[i];
        avg = avg / sizeF;

        for (unsigned int i = 0; i < values.size(); ++i)
            deviation += (values[i] - avg) * (values[i] - avg);
        return sqrt(deviation / sizeF);
    }

    osg::Matrix computePerspectiveMatrix(double hfov, double vfov, double zn, double zf)
    {
        double fov1 = osg::DegreesToRadians(hfov) * 0.5;
        double fov2 = osg::DegreesToRadians(vfov) * 0.5;
        //double aspectRatio = tan(fov1) / tan(fov2);
        double l = -zn * tan(fov1), r = zn * tan(fov1);
        double b = -zn * tan(fov2), t = zn * tan(fov2);
        return osg::Matrix::frustum(l, r, b, t, zn, zf);
    };

    osg::Matrix computeInfiniteMatrix(const osg::Matrix& proj, double zn)
    {
        const static double nudge = 1.0 - 1.0 / double(1 << 23);
        osg::Matrix infiniteProj(proj);
        infiniteProj(2, 2) = -1.0 * nudge;
        infiniteProj(2, 3) = -2.0 * zn * nudge;
        infiniteProj(3, 2) = -1.0;
        infiniteProj(3, 3) = 0.0;
        return infiniteProj;
    }

    void retrieveNearAndFar(const osg::Matrix& projectionMatrix, double& znear, double& zfar)
    {
        bool orthographicViewFrustum = projectionMatrix(0, 3) == 0.0 &&
            projectionMatrix(1, 3) == 0.0 && projectionMatrix(2, 3) == 0.0;
        if (orthographicViewFrustum)
        {
            znear = (projectionMatrix(3, 2) + 1.0) / projectionMatrix(2, 2);
            zfar = (projectionMatrix(3, 2) - 1.0) / projectionMatrix(2, 2);
        }
        else
        {
            znear = projectionMatrix(3, 2) / (projectionMatrix(2, 2) - 1.0);
            zfar = projectionMatrix(3, 2) / (1.0 + projectionMatrix(2, 2));
        }
    }

    struct MathExpressionPrivate
    {
        exprtk::symbol_table<double> symbolTable;
        exprtk::expression<double> expression;
        exprtk::parser<double> parser;
    };

}

using namespace osgVerse;

/* MathExpression */

MathExpression::MathExpression(const std::string& exp)
    : _expressionString(exp), _compiled(false)
{
    _private = new MathExpressionPrivate;
    _private->symbolTable.add_constants();
}

MathExpression::~MathExpression()
{
    delete _private;
}

void MathExpression::setVariable(const std::string& name, double& value)
{
    if (_private->symbolTable.symbol_exists(name))
        _private->symbolTable.remove_variable(name);
    _private->symbolTable.add_variable(name, value);
    _compiled = false;
}

void MathExpression::setVariable(const std::string& name, const double& value)
{
    if (_private->symbolTable.symbol_exists(name))
        _private->symbolTable.remove_variable(name);
    _private->symbolTable.add_constant(name, value);
    _compiled = false;
}

double MathExpression::evaluate(bool* ok)
{
    if (!_compiled)
    {
        _private->expression.register_symbol_table(_private->symbolTable);
        _compiled = _private->parser.compile(_expressionString, _private->expression);
        if (ok) *ok = _compiled;
        if (!_compiled)
        {
            OSG_NOTICE << "[MathExpression] expression " << _expressionString << " is invalid: "
                       << _private->parser.error() << std::endl;
            return 0.0;
        }
    }
    return _private->expression.value();
}

/* PointCloudQuery */

struct PointCloudData
{
    std::vector<PointCloudQuery::PointData> points;

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return points.size(); }

    // Returns the distance between the vector "p1[0:size-1]" and
    // the data point with index "idx_p2" stored in the class
    inline float kdtree_distance(const float* p1, const size_t idx_p2, size_t size) const
    {
        const osg::Vec3& p = points[idx_p2].first;
        const float d0 = p1[0] - p[0];
        const float d1 = p1[1] - p[1];
        const float d2 = p1[2] - p[2];
        return d0 * d0 + d1 * d1 + d2 * d2;
    }

    // Returns the dim'th component of the idx'th point in the class
    inline float kdtree_get_pt(const size_t idx, int dim) const
    {
        if (dim == 0) return points[idx].first.x();
        else if (dim == 1) return points[idx].first.y();
        else return points[idx].first.z();
    }

    // Optional bounding-box computation: return false to default to a standard bbox computation loop
    template <class BBOX> bool kdtree_get_bbox(BBOX& bb) const { return false; }
};

typedef nanoflann::L2_Simple_Adaptor<float, PointCloudData> AdaptorType;
typedef nanoflann::KDTreeSingleIndexAdaptor<AdaptorType, PointCloudData, 3> KdTreeType;

PointCloudQuery::PointCloudQuery()
{
    _queryData = new PointCloudData;
    _index = NULL;
}

PointCloudQuery::~PointCloudQuery()
{
    PointCloudData* pcd = (PointCloudData*)_queryData;
    pcd->points.clear();
    delete _queryData;
    _queryData = NULL;

    delete _index;
    _index = NULL;
}

void PointCloudQuery::addPoint(const osg::Vec3& pt, osg::Referenced* userData)
{
    PointCloudData* pcd = (PointCloudData*)_queryData;
    pcd->points.push_back(PointData(pt, userData));
}

void PointCloudQuery::setPoints(const std::vector<PointData>& data)
{
    PointCloudData* pcd = (PointCloudData*)_queryData;
    pcd->points = data;
}

unsigned int PointCloudQuery::getNumPoints() const
{
    PointCloudData* pcd = (PointCloudData*)_queryData;
    return pcd->points.size();
}

const std::vector<PointCloudQuery::PointData>& PointCloudQuery::getPoints() const
{
    PointCloudData* pcd = (PointCloudData*)_queryData;
    return pcd->points;
}

void PointCloudQuery::buildIndex(int maxLeafSize)
{
    PointCloudData* pcd = (PointCloudData*)_queryData;
    if (_index != NULL) delete _index;
    KdTreeType* kdtree = new KdTreeType(3, *pcd, nanoflann::KDTreeSingleIndexAdaptorParams(maxLeafSize));
    kdtree->buildIndex();
    _index = kdtree;
}

void PointCloudQuery::findNearest(const osg::Vec3& pt, std::vector<uint32_t>& resultIndices,
    unsigned int maxResults)
{
    KdTreeType* kdtree = (KdTreeType*)_index;
    std::vector<float> resultDistance2(maxResults);
    resultIndices.resize(maxResults);

    float queryPt[3] = { pt[0], pt[1], pt[2] };
    kdtree->knnSearch(&queryPt[0], maxResults, &(resultIndices[0]), &(resultDistance2[0]));
}

int PointCloudQuery::findInRadius(const osg::Vec3& pt, float radius,
    std::vector<IndexAndDistancePair>& resultIndices)
{
    KdTreeType* kdtree = (KdTreeType*)_index;
    nanoflann::SearchParams params;
    params.sorted = false;

    float queryPt[3] = { pt[0], pt[1], pt[2] };
    return kdtree->radiusSearch(&queryPt[0], radius, resultIndices, params);
}

/* Spline */

PointList2D Spline::createQuadraticBezier(const PointList2D& controls, unsigned int samples)
{
    PointList2D result;
    if (controls.size() < 3) return controls;

    wykobi::quadratic_bezier<wykobi::Float, 2> bezier;
    for (int i = 0; i < 3; ++i) bezier[i] = wykobiUtil::convToPoint2D(controls[i]);

    std::vector< wykobi::point2d<wykobi::Float> > points;
    wykobi::generate_bezier(bezier, std::back_inserter(points), samples);
    std::transform(points.begin(), points.end(), std::back_inserter(result), wykobiUtil::convToVec2);
    return result;
}

PointList3D Spline::createQuadraticBezier(const PointList3D& controls, unsigned int samples)
{
    PointList3D result;
    if (controls.size() < 3) return controls;

    wykobi::quadratic_bezier<wykobi::Float, 3> bezier;
    for (int i = 0; i < 3; ++i) bezier[i] = wykobiUtil::convToPoint3D(controls[i]);

    std::vector< wykobi::point3d<wykobi::Float> > points;
    wykobi::generate_bezier(bezier, std::back_inserter(points), samples);
    std::transform(points.begin(), points.end(), std::back_inserter(result), wykobiUtil::convToVec3);
    return result;
}

PointList2D Spline::createCubicBezier(const PointList2D& controls, unsigned int samples)
{
    PointList2D result;
    if (controls.size() < 4) return controls;

    wykobi::cubic_bezier<wykobi::Float, 2> bezier;
    for (int i = 0; i < 4; ++i) bezier[i] = wykobiUtil::convToPoint2D(controls[i]);

    std::vector< wykobi::point2d<wykobi::Float> > points;
    wykobi::generate_bezier(bezier, std::back_inserter(points), samples);
    std::transform(points.begin(), points.end(), std::back_inserter(result), wykobiUtil::convToVec2);
    return result;
}

PointList3D Spline::createCubicBezier(const PointList3D& controls, unsigned int samples)
{
    PointList3D result;
    if (controls.size() < 4) return controls;

    wykobi::cubic_bezier<wykobi::Float, 3> bezier;
    for (int i = 0; i < 4; ++i) bezier[i] = wykobiUtil::convToPoint3D(controls[i]);

    std::vector< wykobi::point3d<wykobi::Float> > points;
    wykobi::generate_bezier(bezier, std::back_inserter(points), samples);
    std::transform(points.begin(), points.end(), std::back_inserter(result), wykobiUtil::convToVec3);
    return result;
}

/* Intersection */

bool Intersection::lineWithLine2D(const LineType2D& line1, const LineType2D& line2, osg::Vec2& result)
{
    wykobi::segment<wykobi::Float, 2> segment1;
    segment1[0] = wykobiUtil::convToPoint2D(line1.first);
    segment1[1] = wykobiUtil::convToPoint2D(line1.second);

    wykobi::segment<wykobi::Float, 2> segment2;
    segment2[0] = wykobiUtil::convToPoint2D(line2.first);
    segment2[1] = wykobiUtil::convToPoint2D(line2.second);

    wykobi::point2d<wykobi::Float> resultPoint;
    bool ok = wykobi::intersect(segment1, segment2, resultPoint);
    if (ok) result = wykobiUtil::convToVec2(resultPoint);
    return ok;
}

bool Intersection::lineWithPlane3D(const LineType3D& line, const osg::Plane& plane, osg::Vec3& result)
{
    osg::Vec3 dir = line.second - line.first;
    float lineLength = dir.normalize();

    float dot = dir * plane.getNormal();
    float distance = plane.distance(line.first);
    if (fabs(dot) > ZERO_TOLERANCE)
    {
        // The line is not parallel to the plane
        float t = -distance / dot;
        result = line.first + dir * t;
        return fabs(t) <= lineLength;
    }

    if (fabs(distance) <= ZERO_TOLERANCE)
    {
        // The line is coincident with the plane
        result = line.first;
        return true;
    }
    return false;
}

/* GeometryAlgorithm */

bool GeometryAlgorithm::pointInPolygon2D(const osg::Vec2& p, const PointList2D& polygon, bool isConvex)
{
    unsigned int numPoints = polygon.size();
    if (isConvex)
    {
        for (unsigned int i = 0, j = numPoints - 1; i < numPoints; j = i++)
        {
            float nx = polygon[i].y() - polygon[j].y();
            float ny = polygon[j].x() - polygon[i].x();
            float dx = p.x() - polygon[j].x();
            float dy = p.y() - polygon[j].y();
            if ((nx*dx + ny * dy) > 0.0f) return false;
        }
        return true;
    }

    bool inside = false;
    for (unsigned int i = 0, j = numPoints - 1; i < numPoints; j = i++)
    {
        const osg::Vec2& u0 = polygon[i];
        const osg::Vec2& u1 = polygon[j];
        float lhs = 0.0f, rhs = 0.0f;

        if (p.y() < u1.y())  // U1 above ray
        {
            if (u0.y() <= p.y())  // U0 on or below ray
            {
                lhs = (p.y() - u0.y()) * (u1.x() - u0.x());
                rhs = (p.x() - u0.x()) * (u1.y() - u0.y());
                if (lhs > rhs) inside = !inside;
            }
        }
        else if (p.y() < u0.y())  // U1 on or below ray, U0 above ray
        {
            lhs = (p.y() - u0.y()) * (u1.x() - u0.x());
            rhs = (p.x() - u0.x()) * (u1.y() - u0.y());
            if (lhs < rhs) inside = !inside;
        }
    }
    return inside;
}

std::vector<LineType2D> GeometryAlgorithm::decomposePolygon2D(const PointList2D& polygon)
{
    std::vector<LineType2D> result, temp1, temp2;
    unsigned int numDiags = INT_MAX;

#define SAFT_POLYGON_INDEX(i) polygon[(i) < 0 ? ((i) % size + size) : ((i) % size)]
#define TRIANGLE_AREA(a, b, c) \
        (((b.x() - a.x()) * (c.y() - a.y())) - ((c.x() - a.x()) * (b.y() - a.y())))

    int size = (int)polygon.size();
    for (int i = 0; i < size; ++i)
    {
        const osg::Vec2& a = SAFT_POLYGON_INDEX(i - 1);
        const osg::Vec2& b = SAFT_POLYGON_INDEX(i);
        const osg::Vec2& c = SAFT_POLYGON_INDEX(i + 1);
        if (TRIANGLE_AREA(a, b, c) >= 0.0f) continue;

        for (int j = 0; j < size; ++j)
        {
            const osg::Vec2& d = SAFT_POLYGON_INDEX(j);
            if (TRIANGLE_AREA(c, b, d) >= 0.0f && TRIANGLE_AREA(a, b, d) <= 0.0f) continue;

            // Check for each edge
            bool accepted = true;
            float distance = (b - d).length2();
            for (int k = 0; k < size; ++k)
            {
                const osg::Vec2& e = SAFT_POLYGON_INDEX(k);
                const osg::Vec2& f = SAFT_POLYGON_INDEX(k + 1);
                if (((k + 1) % size) == i || k == i) continue;  // ignore incident edges

                if (TRIANGLE_AREA(b, d, f) >= 0.0f && TRIANGLE_AREA(b, d, e) <= 0.0f)
                {   // if diag intersects an edge
                    // Compute line intersection: (b, d) - (e, f)
                    float a1 = d.y() - b.y(), b1 = b.x() - d.x();
                    float c1 = a1 * b.x() + b1 * b.y();
                    float a2 = f.y() - e.y(), b2 = e.x() - f.x();
                    float c2 = a2 * e.x() + b2 * e.y();
                    float det = a1 * b2 - a2 * b1;
                    if (fabs(det) > 0.0001f)
                    {
                        osg::Vec2 ip((b2 * c1 - b1 * c2) / det,
                            (a1 * c2 - a2 * c1) / det);
                        float newDistance = (b - ip).length2() + 0.0001f;
                        if (newDistance < distance) { accepted = false; break; }
                    }
                }
            }
            if (!accepted) continue;

            std::vector<osg::Vec2> p1, p2;
            if (i < j)
            {
                p1.insert(p1.begin(), polygon.begin() + i, polygon.begin() + j + 1);
                p2.insert(p2.begin(), polygon.begin() + j, polygon.end());
                p2.insert(p2.end(), polygon.begin(), polygon.begin() + i + 1);
            }
            else
            {
                p1.insert(p1.begin(), polygon.begin() + i, polygon.end());
                p1.insert(p1.end(), polygon.begin(), polygon.begin() + j + 1);
                p2.insert(p2.begin(), polygon.begin() + j, polygon.begin() + i + 1);
            }

            temp1 = GeometryAlgorithm::decomposePolygon2D(p1);
            temp2 = GeometryAlgorithm::decomposePolygon2D(p2);
            temp1.insert(temp1.end(), temp2.begin(), temp2.end());
            if (temp1.size() < numDiags)
            {
                numDiags = temp1.size();
                result = temp1;
                result.push_back(LineType2D(b, d));
            }
        }
    }
    return result;
}

osg::Vec2 GeometryAlgorithm::closestPointOnLine2D(const osg::Vec2& p, const LineType2D& line)
{
    wykobi::segment<wykobi::Float, 2> segment;
    segment[0] = wykobiUtil::convToPoint2D(line.first);
    segment[1] = wykobiUtil::convToPoint2D(line.second);

    wykobi::point2d<wykobi::Float> closest =
        wykobi::closest_point_on_segment_from_point(segment, wykobiUtil::convToPoint2D(p));
    return wykobiUtil::convToVec2(closest);
}

osg::Vec3 GeometryAlgorithm::closestPointOnLine3D(const osg::Vec3& p, const LineType3D& line)
{
    wykobi::segment<wykobi::Float, 3> segment;
    segment[0] = wykobiUtil::convToPoint3D(line.first);
    segment[1] = wykobiUtil::convToPoint3D(line.second);

    wykobi::point3d<wykobi::Float> closest =
        wykobi::closest_point_on_segment_from_point(segment, wykobiUtil::convToPoint3D(p));
    return wykobiUtil::convToVec3(closest);
}

osg::Vec3 GeometryAlgorithm::closestPointOnBoxAABB(const osg::Vec3& p,
                                                   const osg::Vec3& boxMin, const osg::Vec3& boxMax)
{
    wykobi::box<wykobi::Float, 3> box;
    box[0] = wykobiUtil::convToPoint3D(boxMin);
    box[1] = wykobiUtil::convToPoint3D(boxMax);

    wykobi::point3d<wykobi::Float> closest =
        wykobi::closest_point_on_box_from_point(box, wykobiUtil::convToPoint3D(p));
    return wykobiUtil::convToVec3(closest);
}

void GeometryAlgorithm::mirror2D(const PointList2D& points, PointList2D& output,
                                 const LineType2D& axis, float ratio)
{
    wykobi::polygon<wykobi::Float, 2> polygon(points.size());
    for (unsigned int i = 0; i < points.size(); ++i)
        polygon[i] = wykobiUtil::convToPoint2D(points[i]);

    wykobi::line<wykobi::Float, 2> line;
    line[0] = wykobiUtil::convToPoint2D(axis.first);
    line[1] = wykobiUtil::convToPoint2D(axis.second);
    wykobi::polygon<wykobi::Float, 2> resultPolygon = wykobi::nonsymmetric_mirror(
        polygon, wykobi::Float(ratio), line);

    output.resize(resultPolygon.size());
    for (unsigned int i = 0; i < resultPolygon.size(); ++i)
        output[i] = wykobiUtil::convToVec2(resultPolygon[i]);
}

void GeometryAlgorithm::mirror3D(const PointList3D& points, PointList3D& output,
                                 const osg::Plane& plane, float ratio)
{
    wykobi::polygon<wykobi::Float, 3> polygon(points.size());
    for (unsigned int i = 0; i < points.size(); ++i)
        polygon[i] = wykobiUtil::convToPoint3D(points[i]);

    wykobi::plane<wykobi::Float, 3> refPlane;
    refPlane.constant = plane[3];
    refPlane.normal = wykobi::make_vector(wykobiUtil::convToPoint3D(plane.getNormal()));
    wykobi::polygon<wykobi::Float, 3> resultPolygon = wykobi::nonsymmetric_mirror(
        polygon, wykobi::Float(ratio), refPlane);

    output.resize(resultPolygon.size());
    for (unsigned int i = 0; i < resultPolygon.size(); ++i)
        output[i] = wykobiUtil::convToVec3(resultPolygon[i]);
}

bool GeometryAlgorithm::convexHull2D(const PointList2D& points, std::vector<int>& result,
                                     GeometryAlgorithm::ConvexHullMethod method)
{
    std::vector< wykobi::point2d<wykobi::Float> > internal_points, convex_hull;
    std::transform(points.begin(), points.end(), std::back_inserter(internal_points),
        wykobiUtil::convToPoint2D);

    switch (method)
    {
    case GRAHAM_SCAN:
        wykobi::algorithm::convex_hull_graham_scan< wykobi::point2d<wykobi::Float> >(
            internal_points.begin(), internal_points.end(), std::back_inserter(convex_hull));
        break;
    case JARVIS_MARCH:
        wykobi::algorithm::convex_hull_jarvis_march< wykobi::point2d<wykobi::Float> >(
            internal_points.begin(), internal_points.end(), std::back_inserter(convex_hull));
        break;
    case MELKMAN:
        wykobi::algorithm::convex_hull_melkman< wykobi::point2d<wykobi::Float> >(
            internal_points.begin(), internal_points.end(), std::back_inserter(convex_hull));
        break;
    default: return false;
    }

    PointList2D resultPoints;
    std::transform(convex_hull.begin(), convex_hull.end(), std::back_inserter(resultPoints),
        wykobiUtil::convToVec2);
    for (unsigned int i = 0; i < resultPoints.size(); ++i)
    {
        // FIXME: not a effective way to construct the indices
        PointList2D::const_iterator itr =
            std::find(points.begin(), points.end(), resultPoints[i]);
        if (itr != points.end()) result.push_back((int)(itr - points.begin()));
    }
    return true;
}

bool GeometryAlgorithm::segmentIntersection2D(const PointList2D& points, PointList2D& result)
{
    std::vector< wykobi::segment<wykobi::Float, 2> > segment_list;
    for (unsigned int i = 0; i < points.size(); i += 2)
    {
        const osg::Vec2& v1 = points[i];
        const osg::Vec2& v2 = points[i + 1];
        wykobi::segment<wykobi::Float, 2> segment;
        segment[0] = wykobi::pointnd<wykobi::Float, 2>(v1[0], v1[1]);
        segment[1] = wykobi::pointnd<wykobi::Float, 2>(v2[0], v2[1]);
        segment_list.push_back(segment);
    }

    std::vector< wykobi::point2d<wykobi::Float> > intersection_list;
    wykobi::algorithm::naive_group_intersections< wykobi::segment<wykobi::Float, 2> >(
        segment_list.begin(), segment_list.end(), std::back_inserter(intersection_list));
    std::transform(intersection_list.begin(), intersection_list.end(), result.begin(),
        wykobiUtil::convToVec2);
    return true;
}

bool GeometryAlgorithm::segmentIntersection3D(const PointList3D& points, PointList3D& result)
{
    std::vector< wykobi::segment<wykobi::Float, 3> > segment_list;
    for (unsigned int i = 0; i < points.size(); i += 2)
    {
        const osg::Vec3& v1 = points[i];
        const osg::Vec3& v2 = points[i + 1];
        wykobi::segment<wykobi::Float, 3> segment;
        segment[0] = wykobi::pointnd<wykobi::Float, 3>(v1[0], v1[1], v1[2]);
        segment[1] = wykobi::pointnd<wykobi::Float, 3>(v2[0], v2[1], v2[2]);
        segment_list.push_back(segment);
    }

    std::vector< wykobi::point3d<wykobi::Float> > intersection_list;
    wykobi::algorithm::naive_group_intersections< wykobi::segment<wykobi::Float, 3> >(
        segment_list.begin(), segment_list.end(), std::back_inserter(intersection_list));
    std::transform(intersection_list.begin(), intersection_list.end(), result.begin(),
        wykobiUtil::convToVec3);
    return true;
}

bool GeometryAlgorithm::clockwise2D(const PointList2D& points)
{
    unsigned int num = points.size();
    if (num < 3) return false;

    int count = 0;
    for (unsigned int i = 0; i < num; ++i)
    {
        unsigned int j = (i + 1) % num;
        unsigned int k = (i + 2) % num;
        float z = (points[j].x() - points[i].x()) * (points[k].y() - points[j].y())
            - (points[j].y() - points[i].y()) * (points[k].x() - points[j].x());
        if (z < 0.0f) count--;
        else if (z > 0.0f) count++;
    }
    return count < 0;
}

bool GeometryAlgorithm::reorderPolygon2D(PointList2D& points)
{
    std::vector< wykobi::point2d<wykobi::Float> > internal_points, convex_hull;
    std::transform(points.begin(), points.end(), std::back_inserter(internal_points),
        wykobiUtil::convToPoint2D);
    wykobi::algorithm::ordered_polygon< wykobi::point2d<wykobi::Float> >(
        internal_points.begin(), internal_points.end(), std::back_inserter(convex_hull));

    points.clear();
    std::transform(convex_hull.begin(), convex_hull.end(), std::back_inserter(points),
        wykobiUtil::convToVec2);
    return true;
}
