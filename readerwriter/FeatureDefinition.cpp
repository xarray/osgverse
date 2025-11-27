#include <modeling/Utilities.h>
#include <pipeline/Utilities.h>
#include <pipeline/Drawer2D.h>
#include <osg/Geometry>
#include <osg/TriangleIndexFunctor>
#include <osgUtil/Tessellator>
#include "FeatureDefinition.h"

namespace
{
    struct TriangleCollector
    {
        std::vector<unsigned int> triangles;
        void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
        { triangles.push_back(i1); triangles.push_back(i2); triangles.push_back(i3); }
    };
}

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

static void findAndAddPrimitiveSet(osg::Geometry& geom, osg::PrimitiveSet& p, size_t vStart, bool asNewPrimitiveSet)
{
    // Reorder vertex indices according to vStart
    osg::TriangleIndexFunctor<TriangleCollector> f; p.accept(f);
    for (size_t i = 0; i < f.triangles.size(); ++i) f.triangles[i] += vStart;

    // Find a suitable existing primitive-set and add to it, or create a new one
    size_t newNumTriangles = f.triangles.size(); bool applied = false;
    if (!asNewPrimitiveSet)
    {
        for (size_t i = 0; i < geom.getNumPrimitiveSets(); ++i)
        {
            if (newNumTriangles < 65535)
            {
                osg::DrawElementsUShort* de0 = static_cast<osg::DrawElementsUShort*>(geom.getPrimitiveSet(i));
                if (de0 && de0->getMode() == GL_TRIANGLES)
                { de0->insert(de0->end(), f.triangles.begin(), f.triangles.end()); applied = true; break; }
            }

            osg::DrawElementsUInt* de = static_cast<osg::DrawElementsUInt*>(geom.getPrimitiveSet(i));
            if (de && de->getMode() == GL_TRIANGLES)
            { de->insert(de->end(), f.triangles.begin(), f.triangles.end()); applied = true; break; }
        }
    }
    
    if (!applied)
    {
        if (newNumTriangles < 65535)
            geom.addPrimitiveSet(new osg::DrawElementsUShort(GL_TRIANGLES, f.triangles.begin(), f.triangles.end()));
        else
            geom.addPrimitiveSet(new osg::DrawElementsUInt(GL_TRIANGLES, f.triangles.begin(), f.triangles.end()));
    }
}

namespace osgVerse
{
    void drawFeatureToImage(Feature& f, Drawer2D* drawer, const osg::Vec2& off,
                            const osg::Vec2& scale, DrawerStyleData* style0)
    {
        const std::vector<osg::ref_ptr<osg::Vec3Array>>& ptList = f.getPointList();
        DrawerStyleData style = (style0 == NULL) ? DrawerStyleData() : (*style0);
#define NEW_VERTEX(v) osg::Vec2(((v)[0] + off[0]) * scale[0], ((v)[1] + off[1]) * scale[1])

        switch (f.getType())
        {
        case GL_POINTS:
            for (unsigned int i = 0; i < ptList.size(); ++i)
            {
                osg::Vec3Array* va = ptList[i].get();
                for (unsigned int j = 0; j < va->size(); ++j)
                {
                    osg::Vec2 v0((*va)[j].x(), (*va)[j].y());
                    drawer->drawCircle(NEW_VERTEX(v0), 1.0f, 1.0f, style);
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
                    drawer->drawLine(NEW_VERTEX(v0), NEW_VERTEX(v1), style);
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
                    {
                        osg::Vec2 v((*va)[j].x(), (*va)[j].y());
                        input.push_back(NEW_VERTEX(v));
                    }
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
                    {
                        osg::Vec2 v((*va)[j].x(), (*va)[j].y());
                        input.push_back(NEW_VERTEX(v));
                    }
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

        size_t vStart = va->size();
        if (!ca) { ca = new osg::Vec4Array; geom->setColorArray(ca); }
        if (ca->size() != vStart) ca->resize(vStart);
        geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        
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
                if (p.valid()) findAndAddPrimitiveSet(*geom, *p, vStart, asNewPrimitiveSet);
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
            findAndAddPrimitiveSet(*geom, *p, vStart, asNewPrimitiveSet); break;
        }
    }
}
