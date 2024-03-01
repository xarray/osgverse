#include <algorithm>
#include <functional>
#include <iostream>
#include <cstdio>
#include <climits>

#include "3rdparty/exprtk.hpp"
#include "3rdparty/cdt/CDT.h"
#include "Math.h"
const float ZERO_TOLERANCE = float(1e-5);

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

/* GeometryAlgorithm */

bool GeometryAlgorithm::project(const PointList3D& pIn, PointList2D& proj)
{
    size_t ptr = 2, size = pIn.size();
    if (size < 3) return false; else proj.resize(size);

    osg::Vec3 v0 = pIn[1] - pIn[0]; v0.normalize();
    osg::Vec3 v1 = pIn[2] - pIn[1]; v1.normalize();
    osg::Vec3 norm = v0 ^ v1, p0(pIn[0]);
    while (norm.length2() == 0.0f || !norm.valid())
    {
        v1 = pIn[ptr] - pIn[ptr - 1]; v1.normalize();
        norm = v0 ^ v1; ptr++; if (ptr >= size) return false;
    }

    osg::Matrix m = osg::Matrix::lookAt(p0 + norm, p0, v0);
    for (size_t i = 0; i < proj.size(); ++i)
    {
        osg::Vec3 pt = pIn[i] * m; proj[i].second = i;
        proj[i].first = osg::Vec2(pt[0], pt[1]);
    }
    return true;
}

EdgeList GeometryAlgorithm::project(const std::vector<LineType3D>& edges, const osg::Vec3& normal,
                                    PointList3D& points, PointList2D& points2D)
{
    osg::Matrix matrix;
    if (normal.length2() > 0.0f)
    {
        osg::Vec3 center; size_t centerN = 0;
        for (size_t i = 0; i < edges.size(); ++i)
        { center += edges[i].first; center += edges[i].second; centerN += 2; }
        center /= (float)centerN;

        osg::Vec3 up = (normal.z() > 0.8f) ? osg::Y_AXIS : osg::Z_AXIS;
        matrix = osg::Matrix::lookAt(center + normal, center, up);
    }

    std::map<osg::Vec3, osgVerse::PointType2D> projected;
    for (size_t i = 0; i < edges.size(); ++i)
    {
        const osg::Vec3& e0 = edges[i].first, e1 = edges[i].second;
        osg::Vec3 p0 = e0 * matrix, p1 = e1 * matrix;
        if (projected.find(e0) == projected.end())
            projected[e0] = osgVerse::PointType2D(osg::Vec2(p0[0], p0[1]), projected.size());
        if (projected.find(e1) == projected.end())
            projected[e1] = osgVerse::PointType2D(osg::Vec2(p1[0], p1[1]), projected.size());
    }

    points.resize(projected.size()); points2D.resize(projected.size());
    for (std::map<osg::Vec3, osgVerse::PointType2D>::iterator vitr = projected.begin();
         vitr != projected.end(); ++vitr)
    {
        points[vitr->second.second] = vitr->first;
        points2D[vitr->second.second] = vitr->second;
    }

    EdgeList edgeIndices;
    for (size_t i = 0; i < edges.size(); ++i)
    {
        size_t e0 = projected[edges[i].first].second;
        size_t e1 = projected[edges[i].second].second;
        edgeIndices.push_back(EdgeType(e0, e1));
    }
    return edgeIndices;
}

bool GeometryAlgorithm::pointInPolygon2D(const osg::Vec2& p, const PointList2D& polygon, bool isConvex)
{
    unsigned int numPoints = polygon.size();
    if (isConvex)
    {
        for (unsigned int i = 0, j = numPoints - 1; i < numPoints; j = i++)
        {
            float nx = polygon[i].first.y() - polygon[j].first.y();
            float ny = polygon[j].first.x() - polygon[i].first.x();
            float dx = p.x() - polygon[j].first.x();
            float dy = p.y() - polygon[j].first.y();
            if ((nx * dx + ny * dy) > 0.0f) return false;
        }
        return true;
    }

    bool inside = false;
    for (unsigned int i = 0, j = numPoints - 1; i < numPoints; j = i++)
    {
        const osg::Vec2& u0 = polygon[i].first;
        const osg::Vec2& u1 = polygon[j].first;
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

#define SAFT_POLYGON_INDEX(i) polygon[(i) < 0 ? ((i) % size + size) : ((i) % size)].first
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

            std::vector<PointType2D> p1, p2;
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

bool GeometryAlgorithm::clockwise2D(const PointList2D& points)
{
    unsigned int num = points.size();
    if (num < 3) return false;

    int count = 0;
    for (unsigned int i = 0; i < num; ++i)
    {
        unsigned int j = (i + 1) % num;
        unsigned int k = (i + 2) % num;
        float z = (points[j].first.x() - points[i].first.x()) *
                  (points[k].first.y() - points[j].first.y())
                - (points[j].first.y() - points[i].first.y()) *
                  (points[k].first.x() - points[j].first.x());
        if (z < 0.0f) count--;
        else if (z > 0.0f) count++;
    }
    return count < 0;
}

struct ResortHelper
{
    static PointType2D centroid;
    static double to_angle(const PointType2D& p, const PointType2D& o)
    { return atan2(p.first.y() - o.first.y(), p.first.x() - o.first.x()); }

    static void find_centroid(PointType2D& c, PointType2D* pts, int n_pts)
    {
        double x = 0, y = 0;
        for (int i = 0; i < n_pts; i++) { x += pts[i].first.x(); y += pts[i].first.y(); }
        c.first.x() = x / n_pts; c.first.y() = y / n_pts;
    }

    static int by_polar_angle(const void* va, const void* vb)
    {
        double theta_a = to_angle(*(PointType2D*)va, centroid);
        double theta_b = to_angle(*(PointType2D*)vb, centroid);
        return theta_a < theta_b ? -1 : theta_a > theta_b ? 1 : 0;
    }
};
PointType2D ResortHelper::centroid;

bool GeometryAlgorithm::reorderPointsInPlane(PointList2D& proj)
{
    ResortHelper::find_centroid(ResortHelper::centroid, &proj[0], proj.size());
    qsort(&proj[0], proj.size(), sizeof(PointType2D), ResortHelper::by_polar_angle);
    return true;
}

std::vector<size_t> GeometryAlgorithm::delaunayTriangulation(
        const PointList2D& points, const EdgeList& edges)
{
    if (points.size() < 3) return std::vector<size_t>();
    CDT::Triangulation<double> cdt;
    try
    {
        cdt.insertVertices(
            points.begin(), points.end(),
            [](const PointType2D& p) { return p.first[0]; },
            [](const PointType2D& p) { return p.first[1]; });
    }
    catch (const CDT::DuplicateVertexError& err)
    {
        OSG_WARN << "[GeometryAlgorithm] Exception: " << err.description() << std::endl;
        return std::vector<size_t>();
    }

    if (!edges.empty())
    {
        try
        {
            cdt.insertEdges(
                edges.begin(), edges.end(),
                [](const EdgeType& e) { return e.first; },
                [](const EdgeType& e) { return e.second; });
            cdt.eraseOuterTrianglesAndHoles();
        }
        catch (const CDT::IntersectingConstraintsError& err)
        {
            OSG_WARN << "[GeometryAlgorithm] Exception: " << err.description() << std::endl;
            return std::vector<size_t>();
        }
    }
    else
        cdt.eraseSuperTriangle();

    std::vector<size_t> indices;
    for (size_t i = 0; i < cdt.triangles.size(); ++i)
    {
        CDT::VerticesArr3 idx = cdt.triangles[i].vertices;
        indices.push_back(idx[0]); indices.push_back(idx[1]); indices.push_back(idx[2]);
    }
    return indices;
}
