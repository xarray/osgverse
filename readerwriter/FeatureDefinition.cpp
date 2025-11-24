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

    void addFeatureToGeometry(Feature& f, osg::Geometry* geom, bool asNewPrimitiveSet)
    {
        const std::vector<osg::ref_ptr<osg::Vec3Array>>& ptList = f.getPointList();
        // TODO
    }
}
