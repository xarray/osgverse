#include <osgUtil/Tessellator>
#include "DynamicGeometry.h"

using namespace osgVerse;

/* DynamicGeometry */

DynamicGeometry::DynamicGeometry() : _mode(GL_LINES), _editing(true)
{
    setDataVariance(osg::Object::DYNAMIC);
    setUseDisplayList(false);
    setUseVertexBufferObjects(true);
    getOrCreateColorArray();
}

DynamicGeometry::DynamicGeometry(const DynamicGeometry& copy, const osg::CopyOp& copyop)
    : osg::Geometry(copy, copyop),
    _mode(copy._mode), _editing(copy._editing)
{
}

DynamicGeometry::~DynamicGeometry()
{
}

void DynamicGeometry::addPoint(const osg::Vec3& p)
{
    if (!isEditing()) return;
    osg::Vec3Array* va = getOrCreateVertexArray();
    va->push_back(p); updatePrimitiveSet(va);
}

void DynamicGeometry::popPoint()
{
    if (!isEditing()) return;
    osg::Vec3Array* va = getOrCreateVertexArray();
    if (va->size() > 0) { va->pop_back(); updatePrimitiveSet(va); }
}

void DynamicGeometry::setPoint(int pos, const osg::Vec3& p)
{
    if (!isEditing()) return;
    osg::Vec3Array* va = getOrCreateVertexArray();
    unsigned int index = pos;

    if (pos < 0) index = rescaleNegativeIndex(pos, va->size());
    if (index < va->size()) { (*va)[index] = p; updatePrimitiveSet(va); }
}

void DynamicGeometry::insertPoint(int pos, const osg::Vec3& p)
{
    if (!isEditing()) return;
    osg::Vec3Array* va = getOrCreateVertexArray();
    unsigned int index = pos;

    if (pos < 0) index = rescaleNegativeIndex(pos, va->size());
    if (index < va->size())
    {
        va->insert(va->begin() + index, p);
        updatePrimitiveSet(va);
    }
}

void DynamicGeometry::insertPoints(int pos, Vertices& pts)
{
    if (!isEditing()) return;
    osg::Vec3Array* va = getOrCreateVertexArray();
    unsigned int index = pos;

    if (pos < 0) index = rescaleNegativeIndex(pos, va->size());
    if (index < va->size())
    {
        va->insert(va->begin() + index, pts.begin(), pts.end());
        updatePrimitiveSet(va);
    }
}

void DynamicGeometry::removePoints(int pos, unsigned int numToRemove)
{
    if (!isEditing()) return;
    osg::Vec3Array* va = getOrCreateVertexArray();
    unsigned int index = pos;

    if (pos < 0) index = rescaleNegativeIndex(pos, va->size());
    if (index < va->size())
    {
        unsigned int endOfRemoveRange = index + numToRemove;
        if (endOfRemoveRange > va->size()) endOfRemoveRange = va->size();
        va->erase(va->begin() + index, va->begin() + endOfRemoveRange);
        updatePrimitiveSet(va);
    }
}

unsigned int DynamicGeometry::size() const
{
    const osg::Vec3Array* va = dynamic_cast<const osg::Vec3Array*>(getVertexArray());
    return (va ? va->size() : 0);
}

unsigned int DynamicGeometry::getPointList(DynamicGeometry::Vertices& pts)
{
    const osg::Vec3Array* va = dynamic_cast<const osg::Vec3Array*>(getVertexArray());
    if (!va) return 0;

    unsigned int size = va->size();
    for (unsigned int i = 0; i < size; ++i) pts.push_back((*va)[i]);
    return size;
}

bool DynamicGeometry::getPoint(int pos, osg::Vec3& p)
{
    osg::Vec3Array* va = getOrCreateVertexArray();
    unsigned int index = pos;

    if (pos < 0) index = rescaleNegativeIndex(pos, va->size());
    if (index < va->size()) { p = (*va)[index]; return true; }
    return false;
}

bool DynamicGeometry::getCenter(osg::Vec3& p, osg::Vec3& n)
{
    osg::Vec3Array* va = getOrCreateVertexArray();
    if (va->size() > 0)
    {
        unsigned int size = va->size();
        for (unsigned int i = 0; i < size; ++i)
        {
            const osg::Vec3& p1 = (*va)[(i + 1) % size];
            const osg::Vec3& p2 = (*va)[(i + 2) % size];
            p += (*va)[i]; n += (p1 - (*va)[i]) ^ (p2 - p1);
        }
        p *= 1.0f / (float)size;
        n.normalize(); return true;
    }
    return false;
}

void DynamicGeometry::updatePrimitiveSet(osg::Vec3Array* va, unsigned int start)
{
    va->dirty(); dirtyBound();
}

void DynamicGeometry::edit()
{
    _editing = true;
    setUseDisplayList(false);
    setUseVertexBufferObjects(true);
}

void DynamicGeometry::finish()
{
    _editing = false;
    //setUseDisplayList(true);
    //setUseVertexBufferObjects(false);
}

void DynamicGeometry::setColor(const osg::Vec4& color, unsigned int index)
{
    osg::Vec4Array* ca = getOrCreateColorArray();
    if (index < ca->size()) ca->at(index) = color;
    if (getUseDisplayList()) dirtyDisplayList(); else ca->dirty();
}

osg::Vec4 DynamicGeometry::getColor(unsigned int index) const
{
    const osg::Vec4Array* ca = dynamic_cast<const osg::Vec4Array*>(getColorArray());
    return (ca != NULL && index < ca->size() ? ca->at(index) : osg::Vec4());
}

/* DynamicPointLine */

DynamicPointLine::DynamicPointLine(bool usePoint) : DynamicGeometry()
{ setPointMode(usePoint); }

DynamicPointLine::DynamicPointLine(const DynamicPointLine& copy, const osg::CopyOp& copyop)
    : DynamicGeometry(copy, copyop)
{
}

DynamicPointLine::~DynamicPointLine()
{
}

void DynamicPointLine::updatePrimitiveSet(osg::Vec3Array* va, unsigned int start)
{
    if (getNumPrimitiveSets() > 0 && !start)
    {
        osg::DrawArrays* drawArrays = static_cast<osg::DrawArrays*>(getPrimitiveSetList().back().get());
        drawArrays->setCount(va->size() - drawArrays->getFirst());
        drawArrays->dirty();
    }
    else
    {
        addPrimitiveSet(new osg::DrawArrays(_mode, start, va->size() - start));
        inheritColor(getNumPrimitiveSets());
    }
    DynamicGeometry::updatePrimitiveSet(va);
}

/* DynamicPolyline */

DynamicPolyline::DynamicPolyline(bool loopMode) : DynamicGeometry()
{ setLoopMode(loopMode); }

DynamicPolyline::DynamicPolyline(const DynamicPolyline& copy, const osg::CopyOp& copyop)
    : DynamicGeometry(copy, copyop)
{
}

DynamicPolyline::~DynamicPolyline()
{
}

void DynamicPolyline::finish()
{ DynamicGeometry::finish(); }

void DynamicPolyline::updatePrimitiveSet(osg::Vec3Array* va, unsigned int start)
{
    if (getNumPrimitiveSets() > 0 && !start)
    {
        osg::DrawArrays* drawArrays = static_cast<osg::DrawArrays*>(getPrimitiveSetList().back().get());
        drawArrays->setCount(va->size() - drawArrays->getFirst());
        drawArrays->dirty();
    }
    else
    {
        addPrimitiveSet(new osg::DrawArrays(_mode, start, va->size() - start));
        inheritColor(getNumPrimitiveSets());
    }
    DynamicGeometry::updatePrimitiveSet(va);
}

/* DynamicPolygon */

DynamicPolygon::DynamicPolygon() : DynamicGeometry()
{ _mode = GL_LINE_LOOP; }

DynamicPolygon::DynamicPolygon(const DynamicPolygon& copy, const osg::CopyOp& copyop)
    : DynamicGeometry(copy, copyop)
{
}

DynamicPolygon::~DynamicPolygon()
{
}

void DynamicPolygon::finish()
{
    // FIXME!
    osg::Geometry* tessellatedGeom = new osg::Geometry;
    //copyToAndOptimize( *tessellatedGeom );
    //setInternalOptimizedGeometry( tessellatedGeom );

    osgUtil::Tessellator tsl;
    tsl.setTessellationType(osgUtil::Tessellator::TESS_TYPE_GEOMETRY);
    tsl.setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
    tsl.setBoundaryOnly(false);
    tsl.retessellatePolygons(*tessellatedGeom);
    DynamicGeometry::finish();
}

void DynamicPolygon::updatePrimitiveSet(osg::Vec3Array* va, unsigned int start)
{
    if (getNumPrimitiveSets() > 0 && !start)
    {
        osg::DrawArrays* drawArrays = static_cast<osg::DrawArrays*>(getPrimitiveSetList().back().get());
        drawArrays->setCount(va->size() - drawArrays->getFirst());
        drawArrays->dirty();
    }
    else
    {
        addPrimitiveSet(new osg::DrawArrays(_mode, start, va->size() - start));
        inheritColor(getNumPrimitiveSets());
    }
    DynamicGeometry::updatePrimitiveSet(va);
}
