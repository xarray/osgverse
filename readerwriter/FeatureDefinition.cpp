#include <modeling/Utilities.h>
#include <pipeline/Utilities.h>
#include <pipeline/Drawer2D.h>
#include <osg/Geometry>
#include <osg/TriangleIndexFunctor>
#include <osgUtil/Tessellator>
#include "FeatureDefinition.h"

struct TriangleCollector
{
    std::vector<unsigned int> triangles;
    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    { triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3); }
};

static osg::PrimitiveSet* createDelaunayTriangulation(
        osg::Geometry& geom, const std::vector<osg::ref_ptr<osg::DrawArrays>>& primitives)
{
    geom.removePrimitiveSet(0, geom.getNumPrimitiveSets());
    for (size_t i = 0; i < primitives.size(); ++i)
        geom.addPrimitiveSet(primitives[i].get());

    osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
    tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
    tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
    tscx->setTessellationNormal(osg::Z_AXIS);
    tscx->retessellatePolygons(geom);

    osg::TriangleIndexFunctor<TriangleCollector> f; geom.accept(f);
    if (f.triangles.size() < 65535)
        return new osg::DrawElementsUShort(GL_TRIANGLES, f.triangles.begin(), f.triangles.end());
    else
        return new osg::DrawElementsUInt(GL_TRIANGLES, f.triangles.begin(), f.triangles.end());
}

static void findAndAddToPrimitiveSet(osg::Geometry& geom, osg::PrimitiveSet& p, size_t vStart, bool asNewPrimitiveSet)
{
    // TODO: reorder vertex indices according to vStart
    // TODO: find a suitable existing primitive-set and add to it, or create a new one
}

namespace osgVerse
{
    void drawFeatureToImage(Feature& f, Drawer2D* drawer, DrawerStyleData* style0)
    {
        const std::vector<osg::ref_ptr<osg::Vec3Array>>& ptList = f.getPointList();
        DrawerStyleData style = (style0 == NULL) ? DrawerStyleData() : (*style0);

        switch (f.getType())
        {
        case GL_POINTS:
            for (unsigned int i = 0; i < ptList.size(); ++i)
            {
                osg::Vec3Array* va = ptList[i].get();
                for (unsigned int j = 0; j < va->size(); ++j)
                {
                    osg::Vec2 v0((*va)[j].x(), (*va)[j].y());
                    drawer->drawCircle(v0, 1.0f, 1.0f, style);
                }
            }
            break;
        case GL_LINES:
            for (unsigned int i = 0; i < ptList.size(); ++i)
            {
                osg::Vec3Array* va = ptList[i].get();
                for (unsigned int j = 0; j < va->size(); j += 2)
                {
                    osg::Vec2 v0((*va)[j].x(), (*va)[j].y());
                    osg::Vec2 v1((*va)[j + 1].x(), (*va)[j + 1].y());
                    drawer->drawLine(v0, v1, style);
                }
            }
            break;
        case GL_LINE_STRIP: case GL_LINE_LOOP:
            if (!ptList.empty())
            {
                bool closed = (f.getType() == GL_LINE_LOOP);
                for (unsigned int i = 0; i < ptList.size(); ++i)
                {
                    std::vector<osg::Vec2f> input; osg::Vec3Array* va = ptList[i].get();
                    for (unsigned int j = 0; j < va->size(); ++j)
                        input.push_back(osg::Vec2((*va)[j].x(), (*va)[j].y()));
                    drawer->drawPolyline(input, closed, style);
                }
            }
            break;
        case GL_POLYGON:
            if (!ptList.empty())
            {
                std::vector<std::vector<osg::Vec2f>> polygon;
                for (unsigned int i = 0; i < ptList.size(); ++i)
                {
                    std::vector<osg::Vec2f> input; osg::Vec3Array* va = ptList[i].get();
                    for (unsigned int j = 0; j < va->size(); ++j)
                        input.push_back(osg::Vec2((*va)[j].x(), (*va)[j].y()));
                    polygon.push_back(input);
                }
                drawer->drawPolygon(polygon, style);
            }
            break;
        default: break;
        }
    }

    void addFeatureToGeometry(Feature& f, osg::Geometry* geom, bool asNewPrimitiveSet, const osg::Vec4& color)
    {
        osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec4Array* ca = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
        if (!va) { va = new osg::Vec3Array; geom->setVertexArray(va); }

        size_t vStart = va->size(); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        if (!ca) { ca = new osg::Vec4Array; geom->setColorArray(ca); }
        if (ca->size() != vStart) ca->resize(vStart);
        
        const std::vector<osg::ref_ptr<osg::Vec3Array>>& ptList = f.getPointList();
        switch (f.getType())
        {
        case GL_POLYGON:
            if (!ptList.empty())
            {
                osg::ref_ptr<osg::Geometry> geomToTess = new osg::Geometry;
                osg::ref_ptr<osg::Vec3Array> vaToTess = new osg::Vec3Array;
                geomToTess->setVertexArray(vaToTess.get());

                std::vector<osg::ref_ptr<osg::DrawArrays>> polygonsToTess;
                for (unsigned int i = 0; i < ptList.size(); ++i)
                {
                    osg::Vec3Array* subV = ptList[i].get(); size_t s0 = vaToTess->size();
                    vaToTess->insert(vaToTess->end(), subV->begin(), subV->end());
                    polygonsToTess.push_back(new osg::DrawArrays(GL_POLYGON, s0, vaToTess->size() - s0));
                }

                osg::ref_ptr<osg::PrimitiveSet> p = createDelaunayTriangulation(*geomToTess, polygonsToTess);
                va->insert(va->end(), vaToTess->begin(), vaToTess->end());
                ca->insert(ca->end(), vaToTess->size(), color);
                if (p.valid()) findAndAddToPrimitiveSet(*geom, *p, vStart, asNewPrimitiveSet);
            }
            break;
        default:
            for (unsigned int i = 0; i < ptList.size(); ++i)
            {
                osg::Vec3Array* subV = ptList[i].get();
                va->insert(va->end(), subV->begin(), subV->end());
                ca->insert(ca->end(), subV->size(), color);
            }

            osg::ref_ptr<osg::PrimitiveSet> p = new osg::DrawArrays(f.getType(), 0, va->size() - vStart);
            findAndAddToPrimitiveSet(*geom, *p, vStart, asNewPrimitiveSet); break;
        }
    }
}
