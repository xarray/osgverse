#if defined(__AVX__)
#   include <immintrin.h>
#elif defined(__SSE2__)
#   include <emmintrin.h>
#endif

#include <algorithm>
#include <functional>
#include <iostream>
#include <cstdio>
#include <climits>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osgUtil/Tessellator>

#include "3rdparty/exprtk.hpp"
#include "3rdparty/nanoflann.hpp"
#include "3rdparty/cdt/CDT.h"
#include "3rdparty/mapbox/polylabel.hpp"
#ifndef VERSE_MSVC14
#   include "3rdparty/mapbox/supercluster.hpp"
#endif
#include "3rdparty/clipper2/clipper.h"
#include "Math.h"
const float ZERO_TOLERANCE = float(1e-5);

namespace osgVerse
{

    osg::Vec3d computeHPRFromQuat(const osg::Quat& q)
    {
        // https://github.com/brztitouan/euler-angles-quaternions-library-conversion/blob/master/src/euler.cpp
        const double w2 = q.w() * q.w(), x2 = q.x() * q.x();
        const double y2 = q.y() * q.y(), z2 = q.z() * q.z();
        const double unitLength = w2 + x2 + y2 + z2;  // Normalised == 1, otherwise correction divisor.
        const double abcd = q.w() * q.x() + q.y() * q.z(), eps = 1e-7;
        if (abcd > (0.5 - eps) * unitLength)
            return osg::Vec3d(osg::RadiansToDegrees(2.0 * std::atan2(q.y(), q.w())), 180.0, 0.0);
        else if (abcd < (-0.5 + eps) * unitLength)
            return osg::Vec3d(osg::RadiansToDegrees(-2.0 * std::atan2(q.y(), q.w())), -180.0, 0.0);
        else
        {
            const double adbc = q.w() * q.z() - q.x() * q.y();
            const double acbd = q.w() * q.y() - q.x() * q.z();
            return osg::Vec3d(osg::RadiansToDegrees(std::atan2(2.0 * adbc, 1.0 - (z2 + x2) * 2.0)),
                              osg::RadiansToDegrees(std::asin(2.0 * abcd / unitLength)),
                              osg::RadiansToDegrees(std::atan2(2.0 * acbd, 1.0 - (y2 + x2) * 2.0)));
        }
    }

    osg::Vec3d computeHPRFromMatrix(const osg::Matrix& rotation)
    {
        double sy = sqrt(rotation(0, 0) * rotation(0, 0) + rotation(1, 0) * rotation(1, 0));
        if (!(sy < 1e-6))  // singular
        {
            return osg::Vec3d(osg::RadiansToDegrees(std::atan2(rotation(2, 1), rotation(2, 2))),
                              osg::RadiansToDegrees(std::atan2(-rotation(2, 0), sy)),
                              osg::RadiansToDegrees(std::atan2(rotation(1, 0), rotation(0, 0))));
        }
        else
        {
            return osg::Vec3d(osg::RadiansToDegrees(std::atan2(-rotation(1, 2), rotation(1, 1))),
                              osg::RadiansToDegrees(std::atan2(-rotation(2, 0), sy)), 0.0);
        }
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

    float computeTriangleArea(const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2)
    {
        osg::Vec3 edge1 = v1 - v0, edge2 = v2 - v0;
        osg::Vec3 crossProduct = edge1 ^ edge2;
        return crossProduct.length() * 0.5f;
    }

    float computeTriangleUVArea(const osg::Vec2& v0, const osg::Vec2& v1, const osg::Vec2& v2)
    {
        osg::Vec2 uv[3] = { v0, v1, v2 };
        float area = (uv[1][0] - uv[0][0]) * (uv[2][1] - uv[0][1]) -
                     (uv[2][0] - uv[0][0]) * (uv[1][1] - uv[0][1]);
        return std::abs(area * 0.5f);
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

    osg::Quat computeParentRotation(const osg::Vec3& parentDirection, const osg::Quat& localRot)
    {
        osg::Vec3 forward(parentDirection), up(osg::Z_AXIS);
        osg::Vec3 right = up ^ forward;
        if (right.normalize() == 0.0f) right = osg::Y_AXIS;
        up = forward ^ right;

        osg::Matrix w2l(right[0], up[0], forward[0], 0.0f, right[1], up[1], forward[1], 0.0f,
                        right[2], up[2], forward[2], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        osg::Quat parentQuat = w2l.getRotate();
        return parentQuat * localRot * parentQuat.inverse();
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

    osg::Matrix computePerspectiveMatrix(double focalX, double focalY,
                                         double centerX, double centerY, double zn, double zf)
    {
        osg::Matrix matrix = osg::Matrix::frustum(-1.0, 1.0, -1.0, 1.0, zn, zf);
        matrix(0, 0) = focalX / centerX; matrix(1, 1) = focalY / centerY; return matrix;
    }

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

    bool isEqual(const osg::Matrix& m0, const osg::Matrix& m1)
    {
#if true  // any platform compatible?
        return std::memcmp(m0.ptr(), m1.ptr(), 16 * sizeof(double)) == 0;
#endif
    }

    struct MathExpressionPrivate
    {
        exprtk::symbol_table<double> symbolTable;
        exprtk::expression<double> expression;
        exprtk::parser<double> parser;
    };

}

using namespace osgVerse;

/* Coordinate */

Coordinate::WGS84::WGS84(double radiusE, double radiusP)
{
    double flattening = (radiusE - radiusP) / radiusE;
    eccentricitySq = 2 * flattening - flattening * flattening;
    radiusEquator = radiusE; radiusPolar = radiusP;
}

Coordinate::CGCS2000::CGCS2000(const osg::Vec3d& T, const osg::Vec3d& R, double K)
{ for (int i = 0; i < 3; ++i) {paramT[i] = T[i]; paramR[i] = R[i];} paramK = K; }

double Coordinate::UTM::clenshaw(const double* a, int size, double real)
{
    // Compute the real clenshaw summation.
    // Also computes Gaussian latitude for some B as clens(a, len(a), 2 * B) + B
    const double* p; double hr, hr1, hr2;
    for (p = a + size, hr2 = 0., hr1 = *(--p), hr = 0.; a - p;
         hr2 = hr1, hr1 = hr) { hr = -hr2 + (2. * hr1 * std::cos(real)) + *(--p); }
    return std::sin(real) * hr;
}

double Coordinate::UTM::clenshaw2(const double* a, int size, double real, double imag, double& R, double& I)
{
    // Compute the complex clenshaw summation.
    const double* p; double hr, hr1, hr2, hi, hi1, hi2;
    for (p = a + size, hr2 = 0., hi2 = 0., hi1 = 0., hr1 = *(--p), hi1 = 0.,
         hr = 0., hi = 0.; a - p; hr2 = hr1, hi2 = hi1, hr1 = hr, hi1 = hi)
    {
        hr = -hr2 + (2. * hr1 * std::cos(real) * std::cosh(imag)) -
            (-2. * hi1 * std::sin(real) * std::sinh(imag)) + *(--p);
        hi = -hi2 + (-2. * hr1 * std::sin(real) * std::sinh(imag)) +
            (2. * hi1 * std::cos(real) * std::cosh(imag));
    }

    // Bad practice - Should *either* modify R in-place *or* return R, not both.
    // I is modified, but not returned. Since R and I are tied, we should either
    // return a pair<,>(,) or modify in-place, not mix the strategies
    R = (std::sin(real) * std::cosh(imag) * hr) - (std::cos(real) * std::sinh(imag) * hi);
    I = (std::sin(real) * std::cosh(imag) * hi) + (std::cos(real) * std::sinh(imag) * hr);
    return R;
}

Coordinate::UTM::UTM(int code, const WGS84& wgs84)
{
    if ((code > 32600) && (code <= 32660)) { zone = code - 32600; isNorth = true; }
    else if ((code > 32700) && (code <= 32760)) { zone = code - 32700; isNorth = false; }
    else throw std::invalid_argument("Invalid EPSG Code for UTM Projection");
    lon0 = ((zone - 0.5) * (osg::PI / 30.)) - osg::PI;

    // We have fixed k0 = 0.9996 here. This is standard for WGS84 zones.
    double f = wgs84.eccentricitySq / (1.0 + std::sqrt(1.0 - wgs84.eccentricitySq)); double n = f / (2.0 - f);
    Qn = (0.9996 / (1. + n)) * (1. + n * n * ((1. / 4.) + n * n * ((1. / 64.) + ((n * n) / 256.))));

    // Gaussian -> Geodetic == cgb; Geodetic -> Gaussian == cbg
    cgb[0] = n * (2 + n * ((-2. / 3.) + n * (-2 + n * ((116. / 45.) +
             n * ((26. / 45.) + n * (-2854. / 675.))))));
    cbg[0] = n * (-2 + n * ((2. / 3.) + n * ((4. / 3.) + n * ((-82. / 45.) +
             n * ((32. / 45.) + n * (4642. / 4725.))))));
    cgb[1] = std::pow(n, 2) * ((7. / 3.) + n * ((-8. / 5.) +
             n * ((-227. / 45.) + n * ((2704. / 315.) + n * (2323. / 945.)))));
    cbg[1] = std::pow(n, 2) * ((5. / 3.) + n * ((-16. / 15.) +
             n * ((-13. / 9.) + n * ((904. / 315.) + n * (-1522. / 945.)))));
    cgb[2] = std::pow(n, 3) * ((56. / 15.) + n * ((-136. / 35.) +
             n * ((-1262. / 105.) + n * (73814. / 2835.))));
    cbg[2] = std::pow(n, 3) * ((-26. / 15.) + n * ((34. / 21.) +
             n * ((8. / 5.) + n * (-12686. / 2835.))));
    cgb[3] = std::pow(n, 4) * ((4279. / 630.) + n * ((-332. / 35.) + n * (-399572 / 14175.)));
    cbg[3] = std::pow(n, 4) * ((1237. / 630.) + n * ((-12. / 5.) + n * (-24832. / 14175.)));
    cgb[4] = std::pow(n, 5) * ((4174. / 315.) + n * (-144838. / 6237.));
    cbg[4] = std::pow(n, 5) * ((-734. / 315.) + n * (109598. / 31185.));
    cgb[5] = std::pow(n, 6) * (601676. / 22275.);
    cbg[5] = std::pow(n, 6) * (444337. / 155925.);

    // Elliptical N,E -> Spherical N,E == utg; Spherical N,E -> Elliptical N,E == gtu
    utg[0] = n * (-.5 + n * ((2. / 3.) + n * ((-37. / 96.) + n * ((1. / 360.) +
             n * ((81. / 512.) + n * (-96199. / 604800.))))));
    gtu[0] = n * (.5 + n * ((-2. / 3.) + n * ((5. / 16.) + n * ((41. / 180.) +
             n * ((-127. / 288.) + n * (7891. / 37800.))))));
    utg[1] = std::pow(n, 2) * ((-1. / 48.) + n * ((-1. / 15.) + n * ((437. / 1440.) +
             n * ((-46. / 105.) + n * (1118711. / 3870720.)))));
    gtu[1] = std::pow(n, 2) * ((13. / 48.) + n * ((-3. / 5.) + n * ((557. / 1440.) +
             n * ((281. / 630.) + n * (-1983433. / 1935360.)))));
    utg[2] = std::pow(n, 3) * ((-17. / 480.) + n * ((37. / 840.) +
             n * ((209. / 4480.) + n * (-5569. / 90720.))));
    gtu[2] = std::pow(n, 3) * ((61. / 240.) + n * ((-103. / 140.) +
             n * ((15061. / 26880.) + n * (167603. / 181440.))));
    utg[3] = std::pow(n, 4) * ((-4397. / 161280.) + n * ((11. / 504.) + n * (830251. / 7257600.)));
    gtu[3] = std::pow(n, 4) * ((49561. / 161280.) + n * ((-179. / 168.) + n * (6601661. / 7257600.)));
    utg[4] = std::pow(n, 5) * ((-4583. / 161280.) + n * (108847. / 3991680.));
    gtu[4] = std::pow(n, 5) * ((34729. / 80640.) + n * (-3418889. / 1995840.));
    utg[5] = std::pow(n, 6) * (-20648693. / 638668800.);
    gtu[5] = std::pow(n, 6) * (212378941. / 319334400.);

    // Gaussian latitude of origin latitude
    double Z = clenshaw(cbg, 6, 0.); Zb = -Qn * (Z + clenshaw(gtu, 6, 2 * Z));
}

osg::Vec3d Coordinate::convertLLAtoECEF(const osg::Vec3d& lla, const WGS84& wgs84)
{
    // for details on maths see https://en.wikipedia.org/wiki/ECEF
    const double latitude = lla[0], longitude = lla[1], height = lla[2];
    double sin_latitude = sin(latitude), cos_latitude = cos(latitude), polarThreshold = osg::inDegrees(85.05);
    double N = wgs84.radiusEquator / sqrt(1.0 - wgs84.eccentricitySq * sin_latitude * sin_latitude);
    if (latitude > polarThreshold) return osg::Vec3d(0.0, 0.0, wgs84.radiusPolar + height);
    else if (latitude < -polarThreshold) return osg::Vec3d(0.0, 0.0, -(wgs84.radiusPolar + height));
    return osg::Vec3d((N + height) * cos_latitude * cos(longitude),
                      (N + height) * cos_latitude * sin(longitude),
                      (N * (1 - wgs84.eccentricitySq) + height) * sin_latitude);
}

osg::Vec3d Coordinate::convertECEFtoLLA(const osg::Vec3d& ecef, const WGS84& wgs84)
{
    double latitude = 0.0, longitude = 0.0, height = 0.0;
    if (ecef.x() != 0.0)
        longitude = atan2(ecef.y(), ecef.x());
    else
    {
        if (ecef.y() > 0.0) longitude = osg::PI_2;
        else if (ecef.y() < 0.0) longitude = -osg::PI_2;
        else
        {   // at pole or at center of the earth
            if (ecef.z() > 0.0)
                { latitude = osg::PI_2; height = ecef.z() - wgs84.radiusPolar; }  // north pole.
            else if (ecef.z() < 0.0)
                { latitude = -osg::PI_2; height = -ecef.z() - wgs84.radiusPolar; }   // south pole.
            else
                { latitude = osg::PI_2; height = -wgs84.radiusPolar; }  // center of earth.
            longitude = 0.0; return osg::Vec3d(latitude, longitude, height);
        }
    }

    // http://www.colorado.edu/geography/gcraft/notes/datum/gif/xyzllh.gif
    double p = sqrt(ecef.x() * ecef.x() + ecef.y() * ecef.y());
    double re2 = wgs84.radiusEquator* wgs84.radiusEquator, rp2 = wgs84.radiusPolar * wgs84.radiusPolar;
    double theta = atan2(ecef.z() * wgs84.radiusEquator, (p * wgs84.radiusPolar));
    double eDashSquared = (re2 - rp2) / rp2, sin_theta = sin(theta), cos_theta = cos(theta);
    latitude = atan((ecef.z() + eDashSquared * wgs84.radiusPolar * sin_theta * sin_theta * sin_theta) /
               (p - wgs84.eccentricitySq * wgs84.radiusEquator * cos_theta * cos_theta * cos_theta));

    double sin_latitude = sin(latitude);
    double N = wgs84.radiusEquator / sqrt(1.0 - wgs84.eccentricitySq * sin_latitude * sin_latitude);
    height = p / cos(latitude) - N; return osg::Vec3d(latitude, longitude, height);
}

osg::Vec3d Coordinate::convertECEFtoCGCS2000(const osg::Vec3d& ecef, const CGCS2000& c2k)
{
    double rx = c2k.paramR[0] * osg::PI / 180.0;
    double ry = c2k.paramR[1] * osg::PI / 180.0;
    double rz = c2k.paramR[2] * osg::PI / 180.0;
    double cos_rx = std::cos(rx), sin_rx = std::sin(rx);
    double cos_ry = std::cos(ry), sin_ry = std::sin(ry);
    double cos_rz = std::cos(rz), sin_rz = std::sin(rz);

    double r11 = cos_ry * cos_rz, r12 = cos_rx * sin_rz + sin_rx * sin_ry * cos_rz,
           r13 = sin_rx * sin_rz - cos_rx * sin_ry * cos_rz;
    double r21 = -cos_ry * sin_rz, r22 = cos_rx * cos_rz - sin_rx * sin_ry * sin_rz,
           r23 = sin_rx * cos_rz + cos_rx * sin_ry * sin_rz;
    double r31 = sin_ry, r32 = -sin_rx * cos_ry, r33 = cos_rx * cos_ry;
    double x_rot = r11 * ecef.x() + r12 * ecef.y() + r13 * ecef.z();
    double y_rot = r21 * ecef.x() + r22 * ecef.y() + r23 * ecef.z();
    double z_rot = r31 * ecef.x() + r32 * ecef.y() + r33 * ecef.z();
    return osg::Vec3d(c2k.paramK * x_rot + c2k.paramT[0], c2k.paramK * y_rot + c2k.paramT[1],
                      c2k.paramK * z_rot + c2k.paramT[2]);
}

osg::Vec3d Coordinate::convertCGCS2000toECEF(const osg::Vec3d& coord, const CGCS2000& c2k)
{
    double rx = -c2k.paramR[0] * osg::PI / 180.0;
    double ry = -c2k.paramR[1] * osg::PI / 180.0;
    double rz = -c2k.paramR[2] * osg::PI / 180.0;
    double cos_rx = std::cos(rx), sin_rx = std::sin(rx);
    double cos_ry = std::cos(ry), sin_ry = std::sin(ry);
    double cos_rz = std::cos(rz), sin_rz = std::sin(rz);

    double r11 = cos_ry * cos_rz, r12 = -cos_rx * sin_rz + sin_rx * sin_ry * cos_rz,
           r13 = sin_rx * sin_rz + cos_rx * sin_ry * cos_rz;
    double r21 = cos_ry * sin_rz, r22 = cos_rx * cos_rz + sin_rx * sin_ry * sin_rz,
           r23 = -sin_rx * cos_rz + cos_rx * sin_ry * sin_rz;
    double r31 = -sin_ry, r32 = sin_rx * cos_ry, r33 = cos_rx * cos_ry;
    double x_rot = r11 * coord.x() + r21 * coord.y() + r31 * coord.z();
    double y_rot = r12 * coord.x() + r22 * coord.y() + r32 * coord.z();
    double z_rot = r13 * coord.x() + r23 * coord.y() + r33 * coord.z();
    return osg::Vec3d(c2k.paramK * x_rot + c2k.paramT[0], c2k.paramK * y_rot + c2k.paramT[1],
                      c2k.paramK * z_rot + c2k.paramT[2]);
}

osg::Vec3d Coordinate::convertLLAtoWebMercator(const osg::Vec3d& lla, const WGS84& wgs84)
{
    double norm = wgs84.radiusEquator * 1.0;
    return osg::Vec3d(std::log(std::tan(osg::PI_4 + lla[0] / 2.0)) * norm, lla[1] * norm, lla[2]);
}

osg::Vec3d Coordinate::convertWebMercatorToLLA(const osg::Vec3d& yxz, const WGS84& wgs84)
{
    double norm = wgs84.radiusEquator * 1.0;
    return osg::Vec3d(2.0 * std::atan(std::exp(yxz[0] / norm)) - osg::PI_2, yxz[1] / norm, yxz[2]);
}

osg::Vec3d Coordinate::convertLLAtoUTM(const osg::Vec3d& lla, const UTM& utm, const WGS84& wgs84)
{
    // Elliptical Lat, Lon -> Gaussian Lat, Lon
    double gauss = UTM::clenshaw(utm.cbg, 6, 2. * lla[1]) + lla[1];
    double lam = lla[0] - utm.lon0;  // Adjust longitude for zone offset

    // Account for longitude and get Spherical N,E
    double Cn = std::atan2(std::sin(gauss), std::cos(lam) * std::cos(gauss));
    double Ce = std::atan2(std::sin(lam) * std::cos(gauss),
                           std::hypot(std::sin(gauss), std::cos(gauss) * std::cos(lam)));

    // Spherical N,E to Elliptical N,E
    double dCn = 0.0, dCe = 0.0; Ce = asinh(tan(Ce));
    Cn += UTM::clenshaw2(utm.gtu, 6, 2 * Cn, 2 * Ce, dCn, dCe); Ce += dCe;
    if (std::fabs(Ce) <= 2.623395162778)
        return osg::Vec3d((utm.Qn * Ce * wgs84.radiusEquator) + 500000.0,
                          (((utm.Qn * Cn) + utm.Zb) * wgs84.radiusEquator) +
                          (utm.isNorth ? 0. : 10000000.), lla[2]);
    return osg::Vec3d();
}

osg::Vec3d Coordinate::convertUTMtoLLA(const osg::Vec3d& coord, const UTM& utm, const WGS84& wgs84)
{
    double Cn = (coord[1] - (utm.isNorth ? 0. : 10000000.0)) / wgs84.radiusEquator;
    double Ce = (coord[0] - 500000.0) / wgs84.radiusEquator;
    Cn = (Cn - utm.Zb) / utm.Qn; Ce /= utm.Qn;  // Normalize N,E to Spherical N,E
    if (std::fabs(Ce) <= 2.623395162778)
    {   // N,E to Spherical Lat, Lon
        double dCn, dCe; Cn += UTM::clenshaw2(utm.utg, 6, 2 * Cn, 2 * Ce, dCn, dCe);
        Ce = std::atan(std::sinh(Ce + dCe));

        // Spherical Lat, Lon to Gaussian Lat, Lon
        double sinCe = std::sin(Ce), cosCe = std::cos(Ce);
        Ce = std::atan2(sinCe, cosCe * std::cos(Cn));
        Cn = std::atan2(std::sin(Cn) * cosCe, std::hypot(sinCe, cosCe * std::cos(Cn)));
        return osg::Vec3d(Ce + utm.lon0, UTM::clenshaw(utm.cgb, 6, 2 * Cn) + Cn, coord[2]);
    }
    return osg::Vec3d();
}

osg::Matrix Coordinate::convertLLAtoENU(const osg::Vec3d& lla, const WGS84& wgs84)
{
    const double latitude = lla[0], longitude = lla[1];
    osg::Vec3d up(cos(longitude) * cos(latitude), sin(longitude) * cos(latitude), sin(latitude));
    osg::Vec3d east(-sin(longitude), cos(longitude), 0.0); osg::Vec3d north = up ^ east;
    osg::Matrix m; for (int i = 0; i < 3; ++i)
    { m(0, i) = east[i]; m(1, i) = north[i]; m(2, i) = up[i]; }
    m.setTrans(convertLLAtoECEF(lla, wgs84)); return m;
}

osg::Matrix Coordinate::convertLLAtoNED(const osg::Vec3d& lla, const WGS84& wgs84)
{
    const double latitude = lla[0], longitude = lla[1];
    osg::Vec3d up(cos(longitude) * cos(latitude), sin(longitude) * cos(latitude), sin(latitude));
    osg::Vec3d east(-sin(longitude), cos(longitude), 0.0); osg::Vec3d north = up ^ east;
    osg::Matrix m; for (int i = 0; i < 3; ++i)
    { m(0, i) = north[i]; m(1, i) = east[i]; m(2, i) = -up[i]; }
    m.setTrans(convertLLAtoECEF(lla, wgs84)); return m;
}

struct GCJ02_Helper_Degrees
{
    static bool outOfChina(double lat, double lng)
    {
        if (lat > 0.8293 && lat < 55.8271 && lng > 72.004 && lng < 137.8437) return false;
        return true;
    }

    static double transformLatitude(double lat, double lng)
    {
        double ret = -100 + 2.0 * lng + 3.0 * lat + 0.2 * lat * lat + 0.1 * lng * lat + 0.2 * sqrt(fabs(lng));
        ret += (20.0 * sin(6.0 * lng * osg::PI) + 20.0 * sin(2.0 * lng * osg::PI)) * 2.0 / 3.0;
        ret += (20.0 * sin(lat * osg::PI) + 40.0 * sin(lat / 3.0 * osg::PI)) * 2.0 / 3.0;
        ret += (160.0 * sin(lat * osg::PI / 12.0) + 320.0 * sin(lat * osg::PI / 30.0)) * 2.0 / 3.0;
        return ret;
    }

    static double transformLongitude(double lat, double lng)
    {
        double ret = 300.0 + lng + 2.0 * lat + 0.1 * lng * lng + 0.1 * lng * lat + 0.1 * sqrt(fabs(lng));
        ret += (20.0 * sin(6.0 * lng * osg::PI) + 20.0 * sin(2.0 * lng * osg::PI)) * 2.0 / 3.0;
        ret += (20.0 * sin(lng * osg::PI) + 40.0 * sin(lng / 3.0 * osg::PI)) * 2.0 / 3.0;
        ret += (150.0 * sin(lng * osg::PI / 12.0) + 300.0 * sin(lng * osg::PI / 30.0)) * 2.0 / 3.0;
        return ret;
    }

    static void WGS84toGCJ02(double lat0, double lng0, double& lat, double& lng, double A, double EE)
    {
        lat = lat0; lng = lng0; if (outOfChina(lat0, lng0)) return;
        double dlat = transformLatitude(lat0 - 35.0, lng0 - 105.0);
        double dlng = transformLongitude(lat0 - 35.0, lng0 - 105.0);
        double radlat = lat0 / 180.0 * osg::PI;
        double magic = sin(radlat); magic = 1.0 - EE * magic * magic;
        double sqrtmagic = sqrt(magic);
        dlat = (dlat * 180.0) / ((A * (1.0 - EE)) / (magic * sqrtmagic) * osg::PI);
        dlng = (dlng * 180.0) / (A / sqrtmagic * cos(radlat) * osg::PI);
        lat = lat0 + dlat; lng = lng0 + dlng;
    }
};

osg::Vec3d Coordinate::convertWGS84toGCJ02(const osg::Vec3d& lla, const WGS84& wgs84)
{
    double lat0 = osg::RadiansToDegrees(lla[0]), lat = 0.0;
    double lng0 = osg::RadiansToDegrees(lla[1]), lng = 0.0;
    GCJ02_Helper_Degrees::WGS84toGCJ02(lat0, lng0, lat, lng, wgs84.radiusEquator, wgs84.eccentricitySq);
    return osg::Vec3d(osg::inDegrees(lat), osg::inDegrees(lng), lla[2]);
}

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
    inline size_t kdtree_get_point_count() const { return points.size(); }

    // Returns the distance between the vector "p1[0:size-1]" and
    // the data point with index "idx_p2" stored in the class
    inline float kdtree_distance(const float* p1, const size_t idx_p2, size_t size) const
    {
        const osg::Vec3& p = points[idx_p2].first;
        const float d0 = p1[0] - p[0], d1 = p1[1] - p[1], d2 = p1[2] - p[2];
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
{ _queryData = new PointCloudData; _index = NULL; }

PointCloudQuery::~PointCloudQuery()
{
    PointCloudData* pcd = (PointCloudData*)_queryData;
    pcd->points.clear(); delete pcd; _queryData = NULL;
    KdTreeType* kdtree = (KdTreeType*)_index; delete kdtree; _index = NULL;
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
    if (_index != NULL) { KdTreeType* kd0 = (KdTreeType*)_index; delete kd0; }
    KdTreeType* kdtree = new KdTreeType(3, *pcd, nanoflann::KDTreeSingleIndexAdaptorParams(maxLeafSize));
    kdtree->buildIndex(); _index = kdtree;
}

float PointCloudQuery::findNearest(const osg::Vec3& pt, std::vector<uint32_t>& resultIndices,
                                   unsigned int maxResults)
{
    KdTreeType* kdtree = (KdTreeType*)_index;
    std::vector<float> resultDistance2(maxResults);
    resultIndices.resize(maxResults);

    float queryPt[3] = { pt[0], pt[1], pt[2] };
    kdtree->knnSearch(&queryPt[0], maxResults, &(resultIndices[0]), &(resultDistance2[0]));
    std::sort(resultDistance2.begin(), resultDistance2.end(), std::less<float>());
    return resultDistance2[0];
}

int PointCloudQuery::findInRadius(const osg::Vec3& pt, float radius,
                                  std::vector<IndexAndDistancePair>& resultIndices)
{
    KdTreeType* kdtree = (KdTreeType*)_index;
    nanoflann::SearchParams params; params.sorted = false;

    float queryPt[3] = { pt[0], pt[1], pt[2] };
    return kdtree->radiusSearch(&queryPt[0], radius, resultIndices, params);
}

/* GeometryAlgorithm */

osg::Matrix GeometryAlgorithm::project(const PointList3D& pIn, const osg::Vec3d& planeNormal,
                                       const osg::Vec3d& planeUp, PointList2D& proj)
{
    size_t ptr = 2, size = pIn.size();
    if (!size) return osg::Matrix(); proj.resize(size);

    osg::Vec3d norm = planeNormal, p0 = pIn[0], v0 = planeUp, v1;
    if (norm.length2() == 0.0 || !norm.valid())
    {
        if (size < 3) return osg::Matrix();
        v0 = pIn[1] - pIn[0]; v0.normalize();
        v1 = pIn[2] - pIn[1]; v1.normalize(); norm = v0 ^ v1;
        while (norm.length2() == 0.0 || !norm.valid())
        {
            v1 = pIn[ptr] - pIn[ptr - 1]; v1.normalize();
            norm = v0 ^ v1; ptr++; if (ptr >= size) return osg::Matrix();
        }
    }

    osg::Matrix m = osg::Matrix::lookAt(p0 + norm, p0, v0);
    for (size_t i = 0; i < proj.size(); ++i)
    {
        osg::Vec3d pt = pIn[i] * m; proj[i].second = i;
        proj[i].first = osg::Vec2d(pt[0], pt[1]);
    }
    return m;
}

EdgeList GeometryAlgorithm::project(const std::vector<LineType3D>& edges, const osg::Vec3d& normal,
                                    PointList3D& points, PointList2D& points2D)
{
    osg::Matrix matrix;
    if (normal.length2() > 0.0)
    {
        osg::Vec3d center; size_t centerN = 0;
        for (size_t i = 0; i < edges.size(); ++i)
        { center += edges[i].first; center += edges[i].second; centerN += 2; }
        center /= (double)centerN;

        osg::Vec3d up = (normal.z() > 0.8) ? osg::Y_AXIS : osg::Z_AXIS;
        matrix = osg::Matrix::lookAt(center + normal, center, up);
    }

    std::map<osg::Vec3d, osgVerse::PointType2D> projected;
    for (size_t i = 0; i < edges.size(); ++i)
    {
        const osg::Vec3d& e0 = edges[i].first, e1 = edges[i].second;
        osg::Vec3d p0 = e0 * matrix, p1 = e1 * matrix;
        if (projected.find(e0) == projected.end())
            projected[e0] = osgVerse::PointType2D(osg::Vec2(p0[0], p0[1]), projected.size());
        if (projected.find(e1) == projected.end())
            projected[e1] = osgVerse::PointType2D(osg::Vec2(p1[0], p1[1]), projected.size());
    }

    points.resize(projected.size()); points2D.resize(projected.size());
    for (std::map<osg::Vec3d, osgVerse::PointType2D>::iterator vitr = projected.begin();
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

bool GeometryAlgorithm::pointInPolygon2D(const osg::Vec2d& p, const PointList2D& polygon, bool isConvex)
{
    unsigned int numPoints = polygon.size();
    if (isConvex)
    {
        for (unsigned int i = 0, j = numPoints - 1; i < numPoints; j = i++)
        {
            double nx = polygon[i].first.y() - polygon[j].first.y();
            double ny = polygon[j].first.x() - polygon[i].first.x();
            double dx = p.x() - polygon[j].first.x();
            double dy = p.y() - polygon[j].first.y();
            if ((nx * dx + ny * dy) > 0.0) return false;
        }
        return true;
    }

    bool inside = false;
    for (unsigned int i = 0, j = numPoints - 1; i < numPoints; j = i++)
    {
        const osg::Vec2d& u0 = polygon[i].first;
        const osg::Vec2d& u1 = polygon[j].first;
        double lhs = 0.0, rhs = 0.0;

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

struct IntersectionHelper2D
{
    static osg::Vec2d intersect(const osg::Vec2d& A, const osg::Vec2d& B,
                                const osg::Vec2d& C, const osg::Vec2d& D)
    {
        // Line AB represented as a1x + b1y = c1
        double a1 = B.y() - A.y(), b1 = A.x() - B.x();
        double c1 = a1 * A.x() + b1 * A.y();

        // Line CD represented as a2x + b2y = c2
        double a2 = D.y() - C.y(), b2 = C.x() - D.x();
        double c2 = a2 * C.x() + b2 * C.y();

        double determinant = a1 * b2 - a2 * b1;
        if (determinant == 0)
            return osg::Vec2d(FLT_MAX, FLT_MAX);
        else
        {
            double x = (b2 * c1 - b1 * c2) / determinant;
            double y = (a1 * c2 - a2 * c1) / determinant;
            return osg::Vec2d(x, y);
        }
    }

    static bool isWithinSegment(const osg::Vec2d& p, const osg::Vec2d& a, const osg::Vec2d& b)
    {
        double p0 = p.x(), p1 = p.y(), a0 = a.x(), a1 = a.y(), b0 = b.x(), b1 = b.y();
        return (osg::minimum(a0, b0) <= p0 && p0 <= osg::maximum(a0, b0)) &&
               (osg::minimum(a1, b1) <= p1 && p1 <= osg::maximum(a1, b1));
    }
};

PointList2D GeometryAlgorithm::intersectionWithLine2D(const LineType2D& l0, const LineType2D& l1)
{
    PointList2D result; osg::Vec2d s = l0.first, e = l0.second, p0 = l1.first, p1 = l1.second;
#ifdef false
    Clipper2Lib::PointD rr, l0A(s[0], s[1]), l0B(e[0], e[1]), l1A(p0[0], p0[1]), l1B(p1[0], p1[1]);
    if (Clipper2Lib::GetSegmentIntersectPt(l1A, l1B, l0A, l0B, rr))
    { result.push_back(PointType2D(osg::Vec2d(rr.x, rr.y), 0)); }
#else
    osg::Vec2d intersection = IntersectionHelper2D::intersect(p0, p1, s, e);
    if (IntersectionHelper2D::isWithinSegment(intersection, p0, p1) &&
        IntersectionHelper2D::isWithinSegment(intersection, s, e))
    { result.push_back(PointType2D(intersection, 0)); }
#endif
    return result;
}

PointList2D GeometryAlgorithm::intersectionWithPolygon2D(const LineType2D& line, const PointList2D& polygon)
{
    PointList2D result; size_t num = polygon.size();
    osg::Vec2d s = line.first, e = line.second;
    for (size_t i = 0; i < num; ++i)
    {
        osg::Vec2d p0 = polygon[i].first, p1 = polygon[(i + 1) % num].first;
#ifdef false
        Clipper2Lib::PointD rr, l0A(s[0], s[1]), l0B(e[0], e[1]), l1A(p0[0], p0[1]), l1B(p1[0], p1[1]);
        if (Clipper2Lib::GetSegmentIntersectPt(l1A, l1B, l0A, l0B, rr))
        { result.push_back(PointType2D(osg::Vec2d(rr.x, rr.y), i)); }
#else
        osg::Vec2d intersection = IntersectionHelper2D::intersect(p0, p1, s, e);
        if (IntersectionHelper2D::isWithinSegment(intersection, p0, p1) &&
            IntersectionHelper2D::isWithinSegment(intersection, s, e))
        { result.push_back(PointType2D(intersection, i)); }
#endif
    }
    return result;
}

std::vector<PointList2D> GeometryAlgorithm::clipPolygon2D(const std::vector<PointList2D>& subjects,
                                                          const std::vector<PointList2D>& clips,
                                                          BooleanOperator op, bool evenOdd)
{
    Clipper2Lib::PathsD subsD, clipsD, resultsD;
    Clipper2Lib::ClipType type = Clipper2Lib::ClipType::None;
    switch (op)
    {
    case BooleanOperator::BOOL_Intersection: type = Clipper2Lib::ClipType::Intersection; break;
    case BooleanOperator::BOOL_Union: type = Clipper2Lib::ClipType::Union; break;
    case BooleanOperator::BOOL_Difference: type = Clipper2Lib::ClipType::Difference; break;
    case BooleanOperator::BOOL_Xor: type = Clipper2Lib::ClipType::Xor; break;
    default: break;
    }

    for (size_t i = 0; i < subjects.size(); ++i)
    {
        Clipper2Lib::PathD pathD; const PointList2D& path = subjects[i];
        for (size_t j = 0; j < path.size(); ++j)
        {
            const PointType2D& v = path[j];
            pathD.push_back(Clipper2Lib::PointD(v.first[0], v.first[1], v.second));
        }
        subsD.push_back(pathD);
    }

    for (size_t i = 0; i < clips.size(); ++i)
    {
        Clipper2Lib::PathD pathD; const PointList2D& path = clips[i];
        for (size_t j = 0; j < path.size(); ++j)
        {
            const PointType2D& v = path[j];
            pathD.push_back(Clipper2Lib::PointD(v.first[0], v.first[1], v.second));
        }
        clipsD.push_back(pathD);
    }

    Clipper2Lib::ClipperD clipper; std::vector<PointList2D> results;
    clipper.AddSubject(subsD); clipper.AddClip(clipsD);
    if (clipper.Execute(type,
        evenOdd ? Clipper2Lib::FillRule::EvenOdd : Clipper2Lib::FillRule::NonZero, resultsD))
    {
        for (size_t i = 0; i < resultsD.size(); ++i)
        {
            const Clipper2Lib::PathD& pathD = resultsD[i]; PointList2D path;
            for (size_t j = 0; j < pathD.size(); ++j)
            {
                const Clipper2Lib::PointD& pt = pathD[j];
                path.push_back(PointType2D(osg::Vec2d(pt.x, pt.y), pt.z));
            }
            results.push_back(path);
        }
    }
    return results;
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
        const osg::Vec2d& a = SAFT_POLYGON_INDEX(i - 1);
        const osg::Vec2d& b = SAFT_POLYGON_INDEX(i);
        const osg::Vec2d& c = SAFT_POLYGON_INDEX(i + 1);
        if (TRIANGLE_AREA(a, b, c) >= 0.0) continue;

        for (int j = 0; j < size; ++j)
        {
            const osg::Vec2d& d = SAFT_POLYGON_INDEX(j);
            if (TRIANGLE_AREA(c, b, d) >= 0.0 && TRIANGLE_AREA(a, b, d) <= 0.0) continue;

            // Check for each edge
            bool accepted = true;
            double distance = (b - d).length2();
            for (int k = 0; k < size; ++k)
            {
                const osg::Vec2d& e = SAFT_POLYGON_INDEX(k);
                const osg::Vec2d& f = SAFT_POLYGON_INDEX(k + 1);
                if (((k + 1) % size) == i || k == i) continue;  // ignore incident edges

                if (TRIANGLE_AREA(b, d, f) >= 0.0 && TRIANGLE_AREA(b, d, e) <= 0.0)
                {   // if diag intersects an edge
                    // Compute line intersection: (b, d) - (e, f)
                    double a1 = d.y() - b.y(), b1 = b.x() - d.x();
                    double c1 = a1 * b.x() + b1 * b.y();
                    double a2 = f.y() - e.y(), b2 = e.x() - f.x();
                    double c2 = a2 * e.x() + b2 * e.y();
                    double det = a1 * b2 - a2 * b1;
                    if (fabs(det) > 0.0001)
                    {
                        osg::Vec2d ip((b2 * c1 - b1 * c2) / det,
                                      (a1 * c2 - a2 * c1) / det);
                        double newDistance = (b - ip).length2() + 0.0001;
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
                numDiags = temp1.size(); result = temp1;
                result.push_back(LineType2D(b, d));
            }
        }
    }
    return result;
}

osg::Vec2d GeometryAlgorithm::getPoleOfInaccessibility(const PointList2D& polygon, double precision)
{
    mapbox::geometry::linear_ring<double> ring;
    for (size_t i = 0; i < polygon.size(); ++i)
    {
        const osg::Vec2d& pt = polygon[i].first;
        ring.push_back(mapbox::geometry::point<double>(pt[0], pt[1]));
    }

    mapbox::geometry::polygon<double> mbPolygon; mbPolygon.push_back(ring);
    mapbox::geometry::point<double> result = mapbox::polylabel(mbPolygon, precision);
    return osg::Vec2d(result.x, result.y);
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
        double z = (points[j].first.x() - points[i].first.x()) *
                   (points[k].first.y() - points[j].first.y())
                 - (points[j].first.y() - points[i].first.y()) *
                   (points[k].first.x() - points[j].first.x());
        if (z < 0.0) count--;
        else if (z > 0.0) count++;
    }
    return count < 0;
}

std::vector<PointList2D> GeometryAlgorithm::expandPolygon2D(const PointList2D& polygon, double offset, double scale)
{
    Clipper2Lib::Path64 path; double invScale = 1.0 / scale;
    for (size_t i = 0; i < polygon.size(); ++i)
    {
        const PointType2D& v = polygon[i];
        path.push_back(Clipper2Lib::Point64(
            (int64_t)(v.first[0] * scale), (int64_t)(v.first[1] * scale), v.second));
    }

    Clipper2Lib::ClipperOffset offseter; Clipper2Lib::Paths64 pathsOut;
    offseter.AddPath(path, Clipper2Lib::JoinType::Round, Clipper2Lib::EndType::Polygon);
    offseter.Execute(offset * scale, pathsOut);

    std::vector<PointList2D> result;
    for (size_t i = 0; i < pathsOut.size(); ++i)
    {
        const Clipper2Lib::Path64& pathOut = pathsOut[i]; PointList2D rData;
        for (size_t j = 0; j < pathOut.size(); ++j)
        {
            const Clipper2Lib::Point64& pt = pathOut[j];
            double v0 = (double)pt.x, v1 = (double)pt.y;
            rData.push_back(PointType2D(osg::Vec2d(v0 * invScale, v1 * invScale), pt.z));
        }
        result.push_back(rData);
    }
    return result;
}

struct ResortHelper
{
    static PointType2D centroid;
    static double to_angle(const PointType2D& p, const PointType2D& o)
    { return atan2(p.first.y() - o.first.y(), p.first.x() - o.first.x()); }

    static void find_centroid(PointType2D& c, const PointType2D* pts, int n_pts)
    {
        double x = 0, y = 0;
        for (int i = 0; i < n_pts; i++) { x += pts[i].first.x(); y += pts[i].first.y(); }
        c.first.x() = x / n_pts; c.first.y() = y / n_pts;
    }

    static void find_center_of_mass(PointType2D& c, const PointType2D* pts, int n_pts)
    {
        double x = 0, y = 0, area = 0;
        for (int i = 0; i < n_pts - 1; i++)
        {
            double subArea = (pts[i].first.x() * pts[i + 1].first.y())
                           - (pts[i + 1].first.x() * pts[i].first.y());
            x += (pts[i].first.x() + pts[i + 1].first.x()) * subArea;
            y += (pts[i].first.y() + pts[i + 1].first.y()) * subArea;
            area += subArea;
        }
        c.first.x() = x / (area * 6.0); c.first.y() = y / (area * 6.0);
    }

    static int by_polar_angle(const void* va, const void* vb)
    {
        double theta_a = to_angle(*(PointType2D*)va, centroid);
        double theta_b = to_angle(*(PointType2D*)vb, centroid);
        return theta_a < theta_b ? -1 : theta_a > theta_b ? 1 : 0;
    }
};
PointType2D ResortHelper::centroid;

bool GeometryAlgorithm::reorderPointsInPlane(PointList2D& proj, bool usePoleOfInaccessibility,
                                             const std::vector<osgVerse::EdgeType>& edges0)
{
    if (usePoleOfInaccessibility)
        ResortHelper::centroid.first = getPoleOfInaccessibility(proj);
    else
        ResortHelper::find_centroid(ResortHelper::centroid, &proj[0], proj.size());
    qsort(&proj[0], proj.size(), sizeof(PointType2D), ResortHelper::by_polar_angle);

    if (!edges0.empty())
    {
        // First find 2 vertices in pre-ordered list
        size_t pt0 = 0, pt1 = 0; std::map<size_t, PointType2D> idMap;
        for (size_t i = 0; i < proj.size(); ++i) idMap[proj[i].second] = proj[i];

        std::vector<osgVerse::EdgeType> edges = edges0;
        std::vector<osgVerse::EdgeType>::iterator itr0 = edges.end();
        for (size_t i = 1; i < proj.size(); ++i)
        {
            osgVerse::EdgeType e0(proj[i - 1].second, proj[i].second);
            osgVerse::EdgeType e1(proj[i].second, proj[i - 1].second);
            itr0 = std::find(edges.begin(), edges.end(), e0);
            if (itr0 != edges.end())
                { pt0 = e0.first; pt1 = e0.second; break; }
            else
            {
                itr0 = std::find(edges.begin(), edges.end(), e1);
                if (itr0 != edges.end()) { pt0 = e1.first; pt1 = e1.second; break; }
            }
        }
        if (itr0 == edges.end()) return false; else edges.erase(itr0);

        // Find all other vertices along the edges, instead of finding by centroid and angles
        PointType2D &vertex0 = idMap[pt0], &vertex1 = idMap[pt1];
        PointList2D projNew; bool canContinue = true;
        projNew.push_back(vertex0); projNew.push_back(vertex1);
        while (canContinue)
        {
            std::vector<osgVerse::EdgeType>::iterator itr1 = edges.end(); canContinue = false;
            for (itr1 = edges.begin(); itr1 != edges.end(); ++itr1)
            {
                if (itr1->first == pt1 && itr1->second != pt0)
                    { pt0 = pt1; pt1 = itr1->second; canContinue = true; }
                else if (itr1->second == pt1 && itr1->first != pt0)
                    { pt0 = pt1; pt1 = itr1->first; canContinue = true; }
                if (canContinue) { projNew.push_back(idMap[pt1]); break; }
            }
            if (itr1 != edges.end()) edges.erase(itr1);
        }

        if (projNew.front() == projNew.back()) projNew.pop_back();
        if (projNew.size() != proj.size())
        {
            OSG_WARN << "[GeometryAlgorithm] Reordered polygon has different points than original? "
                     << projNew.size() << " != " << proj.size() << std::endl;
        }
        proj.assign(projNew.begin(), projNew.end());
    }
    return true;
}

osg::Vec2d GeometryAlgorithm::getCentroid(const PointList2D& proj, bool centerOfMass)
{
    PointType2D centroidOfMass;
    if (centerOfMass)
        ResortHelper::find_center_of_mass(centroidOfMass, &proj[0], proj.size());
    else
        ResortHelper::find_centroid(centroidOfMass, &proj[0], proj.size());
    return osg::Vec2d(centroidOfMass.first);
}

std::vector<size_t> GeometryAlgorithm::delaunayTriangulation(
        const PointList2D& points, const EdgeList& edges, bool allowEdgeIntersection)
{
    if (points.size() < 3) return std::vector<size_t>();
    CDT::Triangulation<double> cdt(
        CDT::VertexInsertionOrder::Auto,
        allowEdgeIntersection ? CDT::IntersectingConstraintEdges::TryResolve
                              : CDT::IntersectingConstraintEdges::NotAllowed, 0.0);
    try
    {
        cdt.insertVertices(
            points.begin(), points.end(),
            [](const PointType2D& p) { return p.first[0]; },
            [](const PointType2D& p) { return p.first[1]; });
    }
    catch (const CDT::DuplicateVertexError& err)
    {
        OSG_WARN << "[GeometryAlgorithm] Duplicated: " << err.description() << std::endl;
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
            OSG_NOTICE << "[GeometryAlgorithm] Intersected: " << err.description() << std::endl;
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

namespace
{
    struct TriangleCollector
    {
        std::vector<size_t> triangles;
        void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
        { triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3); }
    };
}

std::vector<size_t> GeometryAlgorithm::delaunayTriangulation(const std::vector<PointList2D>& polygons,
                                                             PointList2D& addedPoints)
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(va.get());

    for (size_t i = 0; i < polygons.size(); ++i)
    {
        const PointList2D& poly = polygons[i]; size_t start = va->size();
        for (size_t j = 0; j < poly.size(); ++j) va->push_back(osg::Vec3(poly[j].first, 0.0f));
        geom->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, start, poly.size()));
    }

    osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
    tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
    tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
    tscx->setTessellationNormal(osg::Z_AXIS);

    size_t vSize0 = va->size(), vSize1 = 0;
    tscx->retessellatePolygons(*geom); vSize1 = va->size();
    for (size_t i = vSize0; i < vSize1; ++i)
    {
        const osg::Vec3& v = (*va)[i];
        addedPoints.push_back(PointType2D(osg::Vec2(v[0], v[1]), i));
    }
    osg::TriangleIndexFunctor<TriangleCollector> f;
    geom->accept(f);
    return f.triangles;
}
