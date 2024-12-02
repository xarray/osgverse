#include <osg/Version>
#include <osg/io_utils>
#include <osg/ImageUtils>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/MeshOptimizers>
#include <osgUtil/Tessellator>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <iostream>

#include "3rdparty/tinyspline/tinyspline.h"
#include "MeshTopology.h"
#include "Utilities.h"
using namespace osgVerse;

static osg::Vec3 computeMidpointOnSphere(const osg::Vec3& a, const osg::Vec3& b,
                                         const osg::Vec3& center, float radius)
{
    osg::Vec3 unitRadial = (a + b) * 0.5f - center;
    unitRadial.normalize();
    return center + (unitRadial * radius);
}

static void createMeshedTriangleOnSphere(unsigned int a, unsigned int b, unsigned int c,
                                         osg::Vec3Array& va, osg::DrawElementsUShort& de,
                                         const osg::Vec3& center, float radius, int iterations)
{
    const osg::Vec3& v1 = va[a];
    const osg::Vec3& v2 = va[b];
    const osg::Vec3& v3 = va[c];
    if (iterations <= 0)
    {
        de.push_back(c);
        de.push_back(b);
        de.push_back(a);
    }
    else  // subdivide recursively
    {
        // Find edge midpoints
        unsigned int ab = va.size();
        va.push_back(computeMidpointOnSphere(v1, v2, center, radius));
        unsigned int bc = va.size();
        va.push_back(computeMidpointOnSphere(v2, v3, center, radius));
        unsigned int ca = va.size();
        va.push_back(computeMidpointOnSphere(v3, v1, center, radius));

        // Continue draw four sub-triangles
        createMeshedTriangleOnSphere(a, ab, ca, va, de, center, radius, iterations - 1);
        createMeshedTriangleOnSphere(ab, b, bc, va, de, center, radius, iterations - 1);
        createMeshedTriangleOnSphere(ca, bc, c, va, de, center, radius, iterations - 1);
        createMeshedTriangleOnSphere(ab, bc, ca, va, de, center, radius, iterations - 1);
    }
}


static void createPentagonTriangles(unsigned int a, unsigned int b, unsigned int c, unsigned int d,
                                    unsigned int e, osg::DrawElementsUShort& de)
{
    de.push_back(a); de.push_back(b); de.push_back(e);
    de.push_back(b); de.push_back(d); de.push_back(e);
    de.push_back(b); de.push_back(c); de.push_back(d);
}

static void createHexagonTriangles(unsigned int a, unsigned int b, unsigned int c, unsigned int d,
                                   unsigned int e, unsigned int f, osg::DrawElementsUShort& de)
{
    de.push_back(a); de.push_back(b); de.push_back(f);
    de.push_back(b); de.push_back(e); de.push_back(f);
    de.push_back(b); de.push_back(c); de.push_back(e);
    de.push_back(c); de.push_back(d); de.push_back(e);
}

static void tessellateGeometry(osg::Geometry& geom, const osg::Vec3& axis)
{
    osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
    tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
    tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
    if (axis.length2() > 0.1f) tscx->setTessellationNormal(axis);
    tscx->retessellatePolygons(geom);
}

namespace osgVerse
{

    PointList3D createBSpline(const PointList3D& ctrl, int numToCreate, int dim)
    {
        tsBSpline* tb = new tsBSpline; std::vector<tsReal> pointsT;
        for (size_t i = 0; i < ctrl.size(); ++i)
        {
            const osg::Vec3& v = ctrl[i];
            for (int d = 0; d < dim; ++d) pointsT.push_back(v[d]);
        }

        std::vector<osg::Vec3d> result;
        tsError err = ts_bspline_interpolate_cubic_natural(&pointsT[0], ctrl.size(), dim, tb, NULL);
        if (err != TS_SUCCESS) { delete tb; return result; }

        tsReal* pointsT1 = new tsReal[numToCreate * dim]; size_t count = 0;
        err = ts_bspline_sample(tb, numToCreate, &pointsT1, &count, NULL);
        if (err == TS_SUCCESS)
            for (size_t i = 0; i < count; ++i)
            {
                osg::Vec3d pt; size_t idx = i * dim;
                for (int d = 0; d < dim; ++d) pt[d] = *(pointsT1 + idx + d);
                result.push_back(pt);
            }
        delete tb; delete[] pointsT1;
        return result;
    }

    osg::Geometry* createLatheGeometry(const PointList3D& ctrlPoints, const osg::Vec3& axis,
                                       int segments, bool withSplinePoints, bool withCaps)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        PointList3D path =
            withSplinePoints ? osgVerse::createBSpline(ctrlPoints, ctrlPoints.size() * 4) : ctrlPoints;

        size_t pSize = path.size();
        float step = 1.0f / (float)(segments - 1), stepY = 1.0f / (float)(pSize - 1);
        for (int i = 0; i < segments; ++i)
        {
            osg::Quat q(osg::PI * 2.0f * step * (float)i, axis);
            for (size_t n = 0; n < pSize; ++n)
            {
                va->push_back(q * path[n]);
                ta->push_back(osg::Vec2d(step * (float)i, stepY * (float)n));
            }
        }

        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_QUADS);
        for (int i = 0; i < segments; ++i)
            for (size_t n = 0; n < pSize - 1; ++n)
            {
                size_t i1 = (i + 1) % segments, n1 = (n + 1) % pSize;
                de->push_back(i * pSize + n); de->push_back(i * pSize + n1);
                de->push_back(i1 * pSize + n1); de->push_back(i1 * pSize + n);
            }

        osg::ref_ptr<osg::Geometry> geom = createGeometry(va.get(), NULL, ta.get(), de.get());
        if (withCaps)
        {
            osg::ref_ptr<osg::DrawElementsUInt> deCap0 = new osg::DrawElementsUInt(GL_POLYGON);
            osg::ref_ptr<osg::DrawElementsUInt> deCap1 = new osg::DrawElementsUInt(GL_POLYGON);
            for (int i = 0; i < segments; ++i)
            {
                deCap0->push_back(i * pSize);
                deCap1->push_back(i * pSize + pSize - 1);
            }
            geom->addPrimitiveSet(deCap0.get()); geom->addPrimitiveSet(deCap1.get());
            tessellateGeometry(*geom, axis);
        }
        return geom.release();
    }

    osg::Geometry* createExtrusionGeometry(const PointList3D& outer, const std::vector<PointList3D>& inners,
                                           const osg::Vec3& height, bool withSplinePoints, bool withCaps)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        PointList3D pathEx = withSplinePoints ? osgVerse::createBSpline(outer, outer.size() * 4) : outer;
        std::vector<PointList3D> pathIn = inners;
        if (withSplinePoints)
            for (size_t i = 0; i < pathIn.size(); ++i)
                pathIn[i] = osgVerse::createBSpline(pathIn[i], pathIn[i].size() * 4);

        osg::ref_ptr<osg::DrawElementsUInt> deWall = new osg::DrawElementsUInt(GL_QUADS);
        bool closed = (pathEx.front() == pathEx.back() || !inners.empty());
        if (closed && pathEx.front() == pathEx.back()) pathEx.pop_back();

        size_t eSize = pathEx.size(); float eStep = 1.0f / (float)eSize;
        for (size_t i = 0; i <= eSize; ++i)
        {   // outer walls
            if (!closed && i == eSize) continue;
            va->push_back(pathEx[i % eSize]); ta->push_back(osg::Vec2((float)i * eStep, 0.0f));
            va->push_back(pathEx[i % eSize] + height); ta->push_back(osg::Vec2((float)i * eStep, 1.0f));
            if (i > 0)
            {
                deWall->push_back(2 * (i - 1) + 1); deWall->push_back(2 * (i - 1));
                deWall->push_back(2 * i); deWall->push_back(2 * i + 1);
            }
        }

        std::vector<size_t> vStartList;
        for (size_t j = 0; j < pathIn.size(); ++j)
        {   // inner walls
            const PointList3D& path0 = pathIn[j]; size_t vStart = va->size(), iSize = path0.size();
            float iStep = 1.0f / (float)iSize; vStartList.push_back(vStart);
            for (size_t i = 0; i <= iSize; ++i)
            {
                va->push_back(path0[i % iSize]); ta->push_back(osg::Vec2((float)i * iStep, 0.0f));
                va->push_back(path0[i % iSize] + height); ta->push_back(osg::Vec2((float)i * iStep, 1.0f));
                if (i > 0)
                {
                    deWall->push_back(vStart + 2 * (i - 1)); deWall->push_back(vStart + 2 * (i - 1) + 1);
                    deWall->push_back(vStart + 2 * i + 1); deWall->push_back(vStart + 2 * i);
                }
            }
        }

        osg::ref_ptr<osg::Geometry> geom = createGeometry(va.get(), NULL, ta.get(), deWall.get());
        if (withCaps)
        {
            osg::ref_ptr<osg::DrawElementsUInt> deCap0 = new osg::DrawElementsUInt(GL_POLYGON);
            osg::ref_ptr<osg::DrawElementsUInt> deCap1 = new osg::DrawElementsUInt(GL_POLYGON);
            for (size_t i = 0; i <= eSize; ++i)
            {
                if (!closed && i == eSize) continue;
                deCap0->insert(deCap0->begin(), 2 * i); deCap1->push_back(2 * i + 1);
            }

            for (size_t j = 0; j < pathIn.size(); ++j)
            {
                size_t vStart = vStartList[j], iSize = pathIn[j].size();
                for (size_t i = 0; i <= iSize; ++i)
                {
                    deCap0->push_back(vStart + 2 * (iSize - i));
                    deCap1->push_back(vStart + 2 * (iSize - i) + 1);
                }
            }
            geom->addPrimitiveSet(deCap0.get()); geom->addPrimitiveSet(deCap1.get());
            tessellateGeometry(*geom, height);
        }
        return geom.release();
    }

    osg::Geometry* createLoftGeometry(const PointList3D& path, const std::vector<PointList3D>& sections,
                                      bool closed, bool withSplinePoints, bool withCaps)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        PointList3D pathEx = withSplinePoints ? osgVerse::createBSpline(path, path.size() * 4) : path;

        size_t pSize = pathEx.size(), secSize = sections.size(), numInSection = 0;
        for (size_t j = 0; j < secSize; ++j)
        { size_t num = sections[j].size(); if (numInSection < num) numInSection = num; }

        PointList3D normals; std::vector<float> distances;
        for (size_t j = 0; j < pSize; ++j)
        {
            osg::Vec3 dir0 = (j > 0) ? osg::Vec3(pathEx[j] - pathEx[j - 1]) : osg::Vec3();
            osg::Vec3 dir1 = (j < pSize - 1) ? osg::Vec3(pathEx[j + 1] - pathEx[j]) : osg::Vec3();
            osg::Vec3 N = dir0 + dir1; N.normalize(); normals.push_back(N);

            if (j > 0) distances.push_back(distances.back() + dir0.length());
            else distances.push_back(0.0f);
        }

        for (size_t j = 0; j < pSize; ++j)
        {
            const PointList3D& sec = sections[osg::minimum(j, secSize - 1)];
            const osg::Vec3d& pt = pathEx[j]; float step = 1.0f / (float)(sec.size() - 1);
            osg::Quat quat; quat.makeRotate(osg::Vec3d(osg::Z_AXIS), normals[j]);
            for (size_t i = 0; i < sec.size(); ++i)
            {
                va->push_back(pt + quat * sec[i]);
                ta->push_back(osg::Vec2((float)i * step, distances[j] / distances.back()));
            }
            if (sec.size() < numInSection)
            {
                va->insert(va->end(), numInSection - sec.size(), va->back());
                ta->insert(ta->end(), numInSection - sec.size(), ta->back());
            }
        }

        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_QUADS);
        for (size_t j = 1; j < pSize; ++j)
        {
            for (size_t i = 0; i < numInSection; ++i)
            {
                if (i == 0 && !closed) continue; size_t i0 = (i - 1) % numInSection;
                de->push_back((j - 1) * numInSection + i0); de->push_back((j - 1) * numInSection + i);
                de->push_back(j * numInSection + i); de->push_back(j * numInSection + i0);
            }
        }

        osg::ref_ptr<osg::Geometry> geom = createGeometry(va.get(), NULL, ta.get(), de.get());
        if (withCaps)
        {
            osg::ref_ptr<osg::DrawElementsUInt> deCap0 = new osg::DrawElementsUInt(GL_POLYGON);
            osg::ref_ptr<osg::DrawElementsUInt> deCap1 = new osg::DrawElementsUInt(GL_POLYGON);
            for (size_t i = 0; i < numInSection; ++i)
            {
                deCap0->push_back(0 * numInSection + i);
                deCap1->push_back((pSize - 1) * numInSection + i);
            }
            geom->addPrimitiveSet(deCap0.get()); geom->addPrimitiveSet(deCap1.get());
            tessellateGeometry(*geom, osg::Vec3());
        }
        return geom.release();
    }

    osg::Geometry* createGeodesicSphere(const osg::Vec3& center, float radius, int iterations)
    {
        // Reference: http://paulbourke.net/geometry/platonic/
        if (iterations < 0 || radius <= 0.0f)
        {
            OSG_NOTICE << "createGeodesicSphere: invalid parameters" << std::endl;
            return NULL;
        }

        static const float sqrt5 = sqrt(5.0f);
        static const float phi = (1.0f + sqrt5) * 0.5f; // "golden ratio"
        static const float ratio = sqrt(10.0f + (2.0f * sqrt5)) / (4.0f * phi);
        static const float a = (radius / ratio) * 0.5;
        static const float b = (radius / ratio) / (2.0f * phi);

        // Define the icosahedron's 12 vertices:
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        va->push_back(center + osg::Vec3(0.0f, b, -a));
        va->push_back(center + osg::Vec3(b, a, 0.0f));
        va->push_back(center + osg::Vec3(-b, a, 0.0f));
        va->push_back(center + osg::Vec3(0.0f, b, a));
        va->push_back(center + osg::Vec3(0.0f, -b, a));
        va->push_back(center + osg::Vec3(-a, 0.0f, b));
        va->push_back(center + osg::Vec3(0.0f, -b, -a));
        va->push_back(center + osg::Vec3(a, 0.0f, -b));
        va->push_back(center + osg::Vec3(a, 0.0f, b));
        va->push_back(center + osg::Vec3(-a, 0.0f, -b));
        va->push_back(center + osg::Vec3(b, -a, 0.0f));
        va->push_back(center + osg::Vec3(-b, -a, 0.0f));

        // Draw the icosahedron's 20 triangular faces
        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
        createMeshedTriangleOnSphere(0, 1, 2, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(3, 2, 1, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(3, 4, 5, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(3, 8, 4, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(0, 6, 7, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(0, 9, 6, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(4, 10, 11, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(6, 11, 10, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(2, 5, 9, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(11, 9, 5, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(1, 7, 8, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(10, 8, 7, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(3, 5, 2, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(3, 1, 8, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(0, 2, 9, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(0, 7, 1, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(6, 9, 11, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(6, 10, 7, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(4, 11, 5, *va, *de, center, radius, iterations);
        createMeshedTriangleOnSphere(4, 8, 10, *va, *de, center, radius, iterations);
        return createGeometry(va.get(), NULL, NULL, de.get());
    }

    osg::Geometry* createSoccer(const osg::Vec3& center, float radius)
    {
        if (radius <= 0.0f)
        {
            OSG_NOTICE << "createSoccer: invalid parameters" << std::endl;
            return NULL;
        }

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        va->push_back(center + osg::Vec3(0.0f, 0.0f, 1.021f) * radius);
        va->push_back(center + osg::Vec3(0.4035482f, 0.0f, 0.9378643f) * radius);
        va->push_back(center + osg::Vec3(-0.2274644f, 0.3333333f, 0.9378643f) * radius);
        va->push_back(center + osg::Vec3(-0.1471226f, -0.375774f, 0.9378643f) * radius);
        va->push_back(center + osg::Vec3(0.579632f, 0.3333333f, 0.7715933f) * radius);
        va->push_back(center + osg::Vec3(0.5058321f, -0.375774f, 0.8033483f) * radius);
        va->push_back(center + osg::Vec3(-0.6020514f, 0.2908927f, 0.7715933f) * radius);
        va->push_back(center + osg::Vec3(-0.05138057f, 0.6666667f, 0.7715933f) * radius);
        va->push_back(center + osg::Vec3(0.1654988f, -0.6080151f, 0.8033483f) * radius);
        va->push_back(center + osg::Vec3(-0.5217096f, -0.4182147f, 0.7715933f) * radius);
        va->push_back(center + osg::Vec3(0.8579998f, 0.2908927f, 0.4708062f) * radius);
        va->push_back(center + osg::Vec3(0.3521676f, 0.6666667f, 0.6884578f) * radius);
        va->push_back(center + osg::Vec3(0.7841999f, -0.4182147f, 0.5025612f) * radius);
        va->push_back(center + osg::Vec3(-0.657475f, 0.5979962f, 0.5025612f) * radius);
        va->push_back(center + osg::Vec3(-0.749174f, -0.08488134f, 0.6884578f) * radius);
        va->push_back(center + osg::Vec3(-0.3171418f, 0.8302373f, 0.5025612f) * radius);
        va->push_back(center + osg::Vec3(0.1035333f, -0.8826969f, 0.5025612f) * radius);
        va->push_back(center + osg::Vec3(-0.5836751f, -0.6928964f, 0.4708062f) * radius);
        va->push_back(center + osg::Vec3(0.8025761f, 0.5979962f, 0.2017741f) * radius);
        va->push_back(center + osg::Vec3(0.9602837f, -0.08488134f, 0.3362902f) * radius);
        va->push_back(center + osg::Vec3(0.4899547f, 0.8302373f, 0.3362902f) * radius);
        va->push_back(center + osg::Vec3(0.7222343f, -0.6928964f, 0.2017741f) * radius);
        va->push_back(center + osg::Vec3(-0.8600213f, 0.5293258f, 0.1503935f) * radius);
        va->push_back(center + osg::Vec3(-0.9517203f, -0.1535518f, 0.3362902f) * radius);
        va->push_back(center + osg::Vec3(-0.1793548f, 0.993808f, 0.1503935f) * radius);
        va->push_back(center + osg::Vec3(0.381901f, -0.9251375f, 0.2017741f) * radius);
        va->push_back(center + osg::Vec3(-0.2710537f, -0.9251375f, 0.3362902f) * radius);
        va->push_back(center + osg::Vec3(-0.8494363f, -0.5293258f, 0.2017741f) * radius);
        va->push_back(center + osg::Vec3(0.8494363f, 0.5293258f, -0.2017741f) * radius);
        va->push_back(center + osg::Vec3(1.007144f, -0.1535518f, -0.06725804f) * radius);
        va->push_back(center + osg::Vec3(0.2241935f, 0.993808f, 0.06725804f) * radius);
        va->push_back(center + osg::Vec3(0.8600213f, -0.5293258f, -0.1503935f) * radius);
        va->push_back(center + osg::Vec3(-0.7222343f, 0.6928964f, -0.2017741f) * radius);
        va->push_back(center + osg::Vec3(-1.007144f, 0.1535518f, 0.06725804f) * radius);
        va->push_back(center + osg::Vec3(-0.381901f, 0.9251375f, -0.2017741f) * radius);
        va->push_back(center + osg::Vec3(0.1793548f, -0.993808f, -0.1503935f) * radius);
        va->push_back(center + osg::Vec3(-0.2241935f, -0.993808f, -0.06725804f) * radius);
        va->push_back(center + osg::Vec3(-0.8025761f, -0.5979962f, -0.2017741f) * radius);
        va->push_back(center + osg::Vec3(0.5836751f, 0.6928964f, -0.4708062f) * radius);
        va->push_back(center + osg::Vec3(0.9517203f, 0.1535518f, -0.3362902f) * radius);
        va->push_back(center + osg::Vec3(0.2710537f, 0.9251375f, -0.3362902f) * radius);
        va->push_back(center + osg::Vec3(0.657475f, -0.5979962f, -0.5025612f) * radius);
        va->push_back(center + osg::Vec3(-0.7841999f, 0.4182147f, -0.5025612f) * radius);
        va->push_back(center + osg::Vec3(-0.9602837f, 0.08488134f, -0.3362902f) * radius);
        va->push_back(center + osg::Vec3(-0.1035333f, 0.8826969f, -0.5025612f) * radius);
        va->push_back(center + osg::Vec3(0.3171418f, -0.8302373f, -0.5025612f) * radius);
        va->push_back(center + osg::Vec3(-0.4899547f, -0.8302373f, -0.3362902f) * radius);
        va->push_back(center + osg::Vec3(-0.8579998f, -0.2908927f, -0.4708062f) * radius);
        va->push_back(center + osg::Vec3(0.5217096f, 0.4182147f, -0.7715933f) * radius);
        va->push_back(center + osg::Vec3(0.749174f, 0.08488134f, -0.6884578f) * radius);
        va->push_back(center + osg::Vec3(0.6020514f, -0.2908927f, -0.7715933f) * radius);
        va->push_back(center + osg::Vec3(-0.5058321f, 0.375774f, -0.8033483f) * radius);
        va->push_back(center + osg::Vec3(-0.1654988f, 0.6080151f, -0.8033483f) * radius);
        va->push_back(center + osg::Vec3(0.05138057f, -0.6666667f, -0.7715933f) * radius);
        va->push_back(center + osg::Vec3(-0.3521676f, -0.6666667f, -0.6884578f) * radius);
        va->push_back(center + osg::Vec3(-0.579632f, -0.3333333f, -0.7715933f) * radius);
        va->push_back(center + osg::Vec3(0.1471226f, 0.375774f, -0.9378643f) * radius);
        va->push_back(center + osg::Vec3(0.2274644f, -0.3333333f, -0.9378643f) * radius);
        va->push_back(center + osg::Vec3(-0.4035482f, 0.0f, -0.9378643f) * radius);
        va->push_back(center + osg::Vec3(0.0f, 0.0f, -1.021f) * radius);

        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
        createPentagonTriangles(0, 3, 8, 5, 1, *de);
        createPentagonTriangles(2, 7, 15, 13, 6, *de);
        createPentagonTriangles(4, 10, 18, 20, 11, *de);
        createPentagonTriangles(9, 14, 23, 27, 17, *de);
        createPentagonTriangles(12, 21, 31, 29, 19, *de);
        createPentagonTriangles(16, 26, 36, 35, 25, *de);
        createPentagonTriangles(22, 32, 42, 43, 33, *de);
        createPentagonTriangles(24, 30, 40, 44, 34, *de);
        createPentagonTriangles(28, 39, 49, 48, 38, *de);
        createPentagonTriangles(37, 47, 55, 54, 46, *de);
        createPentagonTriangles(41, 45, 53, 57, 50, *de);
        createPentagonTriangles(51, 52, 56, 59, 58, *de);
        createHexagonTriangles(0, 1, 4, 11, 7, 2, *de);
        createHexagonTriangles(0, 2, 6, 14, 9, 3, *de);
        createHexagonTriangles(1, 5, 12, 19, 10, 4, *de);
        createHexagonTriangles(3, 9, 17, 26, 16, 8, *de);
        createHexagonTriangles(5, 8, 16, 25, 21, 12, *de);
        createHexagonTriangles(6, 13, 22, 33, 23, 14, *de);
        createHexagonTriangles(7, 11, 20, 30, 24, 15, *de);
        createHexagonTriangles(10, 19, 29, 39, 28, 18, *de);
        createHexagonTriangles(13, 15, 24, 34, 32, 22, *de);
        createHexagonTriangles(17, 27, 37, 46, 36, 26, *de);
        createHexagonTriangles(18, 28, 38, 40, 30, 20, *de);
        createHexagonTriangles(21, 25, 35, 45, 41, 31, *de);
        createHexagonTriangles(23, 33, 43, 47, 37, 27, *de);
        createHexagonTriangles(29, 31, 41, 50, 49, 39, *de);
        createHexagonTriangles(32, 34, 44, 52, 51, 42, *de);
        createHexagonTriangles(35, 36, 46, 54, 53, 45, *de);
        createHexagonTriangles(38, 48, 56, 52, 44, 40, *de);
        createHexagonTriangles(42, 51, 58, 55, 47, 43, *de);
        createHexagonTriangles(48, 49, 50, 57, 59, 56, *de);
        createHexagonTriangles(53, 54, 55, 58, 59, 57, *de);
        return createGeometry(va.get(), NULL, NULL, de.get());
    }

    osg::Geometry* createPanoramaSphere(int subdivs)
    {
        static float radius = 1.0f / sqrt(1.0f + osg::PI * osg::PI);
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        va->push_back(osg::Vec3(-1.0f, osg::PI, 0.0f) * radius);
        va->push_back(osg::Vec3(1.0f, osg::PI, 0.0f) * radius);
        va->push_back(osg::Vec3(-1.0f, -osg::PI, 0.0f) * radius);
        va->push_back(osg::Vec3(1.0f, -osg::PI, 0.0f) * radius);
        va->push_back(osg::Vec3(0.0f, -1.0f, osg::PI) * radius);
        va->push_back(osg::Vec3(0.0f, 1.0f, osg::PI) * radius);
        va->push_back(osg::Vec3(0.0f, -1.0f, -osg::PI) * radius);
        va->push_back(osg::Vec3(0.0f, 1.0f, -osg::PI) * radius);
        va->push_back(osg::Vec3(osg::PI, 0.0f, -1.0f) * radius);
        va->push_back(osg::Vec3(osg::PI, 0.0f, 1.0f) * radius);
        va->push_back(osg::Vec3(-osg::PI, 0.0f, -1.0f) * radius);
        va->push_back(osg::Vec3(-osg::PI, 0.0f, 1.0f) * radius);

        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
        de->push_back(0); de->push_back(11); de->push_back(5);
        de->push_back(0); de->push_back(5); de->push_back(1);
        de->push_back(0); de->push_back(1); de->push_back(7);
        de->push_back(0); de->push_back(7); de->push_back(10);
        de->push_back(0); de->push_back(10); de->push_back(11);
        de->push_back(1); de->push_back(5); de->push_back(9);
        de->push_back(5); de->push_back(11); de->push_back(4);
        de->push_back(11); de->push_back(10); de->push_back(2);
        de->push_back(10); de->push_back(7); de->push_back(6);
        de->push_back(7); de->push_back(1); de->push_back(8);
        de->push_back(3); de->push_back(9); de->push_back(4);
        de->push_back(3); de->push_back(4); de->push_back(2);
        de->push_back(3); de->push_back(2); de->push_back(6);
        de->push_back(3); de->push_back(6); de->push_back(8);
        de->push_back(3); de->push_back(8); de->push_back(9);
        de->push_back(4); de->push_back(9); de->push_back(5);
        de->push_back(2); de->push_back(4); de->push_back(11);
        de->push_back(6); de->push_back(2); de->push_back(10);
        de->push_back(8); de->push_back(6); de->push_back(7);
        de->push_back(9); de->push_back(8); de->push_back(1);

        for (int i = 0; i < subdivs; ++i)
        {
            unsigned int numIndices = de->size();
            for (unsigned int n = 0; n < numIndices; n += 3)
            {
                unsigned short n1 = (*de)[n], n2 = (*de)[n + 1], n3 = (*de)[n + 2];
                unsigned short n12 = 0, n23 = 0, n13 = 0;
                va->push_back((*va)[n1] + (*va)[n2]); va->back().normalize(); n12 = va->size() - 1;
                va->push_back((*va)[n2] + (*va)[n3]); va->back().normalize(); n23 = va->size() - 1;
                va->push_back((*va)[n1] + (*va)[n3]); va->back().normalize(); n13 = va->size() - 1;

                (*de)[n] = n1; (*de)[n + 1] = n12; (*de)[n + 2] = n13;
                de->push_back(n2); de->push_back(n23); de->push_back(n12);
                de->push_back(n3); de->push_back(n13); de->push_back(n23);
                de->push_back(n12); de->push_back(n23); de->push_back(n13);
            }
        }

        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        for (unsigned int i = 0; i < va->size(); ++i)
        {
            const osg::Vec3& v = (*va)[i];
            ta->push_back(osg::Vec2((1.0f + atan2(v.y(), v.x()) / osg::PI) * 0.5f,
                (1.0f - asin(v.z()) * 2.0f / osg::PI) * 0.5f));
        }
        return createGeometry(va.get(), NULL, ta.get(), de.get());
    }

    osg::Geometry* createPointListGeometry(const PointList2D& points, const osg::Vec4& color, bool asPolygon,
                                           bool closed, const std::vector<EdgeType>& edges)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        for (size_t i = 0; i < points.size(); ++i)
            va->push_back(osg::Vec3(points[i].first.x(), points[i].first.y(), 0.0f));

        if (asPolygon)
        {
            osg::ref_ptr<osg::DrawArrays> da = new osg::DrawArrays(GL_POLYGON, 0, va->size());
            osg::ref_ptr<osg::Geometry> geom = createGeometry(va.get(), NULL, color, da.get());
            tessellateGeometry(*geom, osg::Z_AXIS); return geom.release();
        }
        else if (edges.empty())
        {
            osg::ref_ptr<osg::DrawArrays> da =
                new osg::DrawArrays(closed ? GL_LINE_LOOP : GL_LINE_STRIP, 0, va->size());
            return createGeometry(va.get(), NULL, color, da.get());
        }
        else
        {
            osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_LINES);
            for (size_t i = 0; i < edges.size(); ++i)
                { de->push_back(edges[i].first); de->push_back(edges[i].second); }
            return createGeometry(va.get(), NULL, color, de.get());
        }
    }

}
