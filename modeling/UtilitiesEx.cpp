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

static void tessellateGeometry(osg::Geometry& geom, const osg::Vec3& axis)
{
    osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
    tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
    tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
    tscx->setTessellationNormal(axis); tscx->retessellatePolygons(geom);
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

    osg::Geometry* createExtrusionGeometry(const PointList3D& contours, const std::vector<PointList3D>& inners,
                                           const osg::Vec3& height, bool withSplinePoints, bool withCaps)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        PointList3D pathEx =
            withSplinePoints ? osgVerse::createBSpline(contours, contours.size() * 4) : contours;
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

    osg::Geometry* createLoftGeometry(const PointList3D& path, const std::vector<SectionAndLength>& sections,
                                      bool withSplinePoints, bool withCaps)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        PointList3D pathEx =
            withSplinePoints ? osgVerse::createBSpline(path, path.size() * 4) : path;

        osg::ref_ptr<osg::Geometry> geom;// = createGeometry(va.get(), NULL, ta.get(), deWall.get());
        return geom.release();
    }

}
