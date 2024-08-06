#include "LoftModeler.h"
#include <osg/io_utils>
#include <iostream>
using namespace osgVerse;

LoftModeler::LoftModeler()
{
    _segments = 0;
    _wirframeColor.set(1.0f, 0.0f, 0.0f, 1.0f);
    _texCoordRange.set(1.0f, 1.0f);
    _vertices = new osg::Vec3Array;
    _normals = new osg::Vec3Array;
    _texCoords = new osg::Vec2Array;

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    _solidGeom = new osg::Geometry;
    _solidGeom->setUseDisplayList(false);
    _solidGeom->setUseVertexBufferObjects(true);
    _solidGeom->setVertexArray(_vertices.get());
    _solidGeom->setTexCoordArray(0, _texCoords.get());
    _solidGeom->setNormalArray(_normals.get());
    _solidGeom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    _solidGeom->setColorArray(colors);
    _solidGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

    _wireframeGeom = new osg::Geometry;
    _wireframeGeom->setUseDisplayList(false);
    _wireframeGeom->setUseVertexBufferObjects(true);
    _wireframeGeom->setVertexArray(_vertices.get());
    //_wireframeGeom->setTexCoordArray( 0, _texCoords.get() );
    _wireframeGeom->setNormalArray(_normals.get());
    _wireframeGeom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
}

LoftModeler::~LoftModeler()
{
}

void LoftModeler::setWireframeColor(const osg::Vec4& color)
{
    osg::ref_ptr<osg::Vec4Array> colors = static_cast<osg::Vec4Array*>(_wireframeGeom->getColorArray());
    if (!colors)
    {
        colors = new osg::Vec4Array;
        _wireframeGeom->setColorArray(colors.get());
    }

    colors->clear();
    colors->resize(_wireframeGeom->getNumPrimitiveSets(), color);
    colors->dirty();

    _wireframeGeom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    _wirframeColor = color;
}

void LoftModeler::selectOnWireframe(unsigned int pos, const osg::Vec4& color)
{
    osg::ref_ptr<osg::Vec4Array> colors = static_cast<osg::Vec4Array*>(_wireframeGeom->getColorArray());
    if (colors && pos < _shapes.size())
    {
        // Primitives are: lstrip(shape0), lines, lstrip(shape1), lines, lstrip(shape2), ...
        unsigned int primitiveIndex = pos * 2;
        if (primitiveIndex < colors->size()) (*colors)[primitiveIndex] = color;
        colors->dirty();
    }
}

void LoftModeler::addSections(const VertexList& va, const VertexList& centers)
{
    for (unsigned int i = 0; i < centers.size(); ++i)
    {
        const osg::Vec3& center = centers[i]; osg::Vec3 dir;
        if (i > 0) dir += center - centers[i - 1];
        if (i < centers.size() - 1) dir += centers[i + 1] - center;
        addSection(va, center, dir);
    }
}

void LoftModeler::addSection(const VertexList& va, const osg::Vec3& center,
                             const osg::Vec3& direction, double planarRot)
{
    if (!_segments) _segments = va.size();
    ShapeData data(va, center, direction, planarRot);
    _shapes.push_back(data);
    insertGeoemtryData(_shapes.size(), data);
}

void LoftModeler::addSection(const osg::Vec3& center, const osg::Vec3& direction, double planarRot)
{
    if (_shapes.size() > 0)
    {
        ShapeData data(_shapes.back()._section, center, direction, planarRot);
        _shapes.push_back(data);
        insertGeoemtryData(_shapes.size(), data);
    }
}

void LoftModeler::setSection(unsigned int pos, const VertexList& va)
{
    if (pos < _shapes.size())
    {
        _shapes[pos]._section = va;
        setGeoemtryData(pos, _shapes[pos]);
    }
}

void LoftModeler::setCenter(unsigned int pos, const osg::Vec3& center, const osg::Vec3& direction)
{
    if (pos < _shapes.size())
    {
        _shapes[pos]._center = center;
        _shapes[pos]._direction = direction;
        setGeoemtryData(pos, _shapes[pos]);
    }
}

void LoftModeler::setPlanarRotation(unsigned int pos, double planarRot)
{
    if (pos < _shapes.size())
    {
        _shapes[pos]._planarRotation = planarRot;
        setGeoemtryData(pos, _shapes[pos]);
    }
}

void LoftModeler::insertSection(unsigned int pos, const VertexList& va, const osg::Vec3& center,
                                const osg::Vec3& direction, double planarRot)
{
    if (pos < _shapes.size())
    {
        if (!_segments) _segments = va.size();
        ShapeData data(va, center, direction, planarRot);
        _shapes.insert(_shapes.begin() + pos, data);
        insertGeoemtryData(pos, data);
    }
}

void LoftModeler::removeSection(unsigned int pos)
{
    if (pos < _shapes.size())
    {
        _shapes.erase(_shapes.begin() + pos);
        removeGeoemtryData(pos);
        if (!_shapes.size()) _segments = 0;
    }
}

unsigned int LoftModeler::findShapeIndex(const osg::Vec3& point)
{
    unsigned int index = 0;
    double minLength2 = FLT_MAX, length2 = 0.0;
    for (unsigned int i = 0; i < _vertices->size(); ++i)
    {
        length2 = (point - (*_vertices)[i]).length2();
        if (length2 < minLength2) { minLength2 = length2; index = i / _segments; }
    }
    return index;
}

void LoftModeler::setGeoemtryData(unsigned int pos, const LoftModeler::ShapeData& data)
{
    updateVertices(pos, data);
    updateRatios(); updateBound();
}

void LoftModeler::insertGeoemtryData(unsigned int pos, const LoftModeler::ShapeData& data)
{
    if (pos < _shapes.size())
    {
        unsigned int start = pos * _segments;
        _vertices->insert(_vertices->begin() + start, _segments, osg::Vec3());
        _texCoords->insert(_texCoords->begin() + start, _segments, osg::Vec2());
        _normals->insert(_normals->begin() + start, _segments, osg::Vec3());
        updateVertices(pos, data);
    }
    else
    {
        _vertices->resize(_vertices->size() + _segments);
        _texCoords->resize(_vertices->size());
        _normals->resize(_vertices->size());
        updateVertices(pos - 1, data);
    }

    updateRatios();
    updatePrimitiveSets();
    updateBound();
}

void LoftModeler::removeGeoemtryData(unsigned int pos)
{
    unsigned int start = pos * _segments;
    if (start < _vertices->size())
    {
        unsigned int end = start + _segments;
        if (end < _vertices->size())
            _vertices->erase(_vertices->begin() + start, _vertices->begin() + end);
        else
            _vertices->erase(_vertices->begin() + start, _vertices->end());
    }

    updateRatios();
    updatePrimitiveSets();
    updateBound();
}

void LoftModeler::updateRatios()
{
    double totalDistance = 0.0;
    std::vector<double> distances;
    distances.push_back(0.0);

    unsigned int size = _shapes.size();
    for (unsigned int i = 0; i < size - 1; ++i)
    {
        totalDistance += (_shapes[i + 1]._center - _shapes[i]._center).length();
        distances.push_back(totalDistance);
    }

    for (unsigned int i = 0; i < size; ++i)
    {
        float tx = _texCoordRange.x() * distances[i] / totalDistance;
        for (unsigned int j = 0; j < _segments; ++j)
        {
            float ty = _texCoordRange.y() * (float)j / (float)(_segments - 1);
            (*_texCoords)[j + i * _segments] = osg::Vec2(tx, ty);
        }
    }
    _texCoords->dirty();
}

void LoftModeler::updateVertices(unsigned int pos, const LoftModeler::ShapeData& data)
{
    int start = pos * _segments;

    // Reset the shape segments
    const VertexList& va = data._section;
    VertexList shape(va.begin(), va.end());
    shape.resize(_segments, va.size() > 0 ? va.back() : osg::Vec3());

    // Alter rotation to get most suitable result
    osg::Matrix rotation = osg::Matrix::rotate(osg::Z_AXIS, data._direction);
    if (pos > 0)
    {
        osg::Vec3 auxLine = data._center - _shapes[pos - 1]._center;
        auxLine.normalize();

        osg::Vec3 lastFirstNormal = (*_normals)[start - _segments];
        osg::Vec3 newFirstPtPlane = lastFirstNormal ^ auxLine;
        newFirstPtPlane.normalize();
        osg::Vec3 newFirstNormal = data._direction ^ newFirstPtPlane;
        newFirstNormal.normalize();

        int maxLoopTime = 100;
        do
        {
            osg::Vec3 currFirstPt = shape.front() * rotation + data._center;
            osg::Vec3 currFirstNormal = currFirstPt - data._center;
            currFirstNormal.normalize();

            // This will adjust the new shape according to previous one to get good looking
            osg::Vec3 axis = currFirstNormal ^ newFirstNormal;
            float angle = atan2(axis.length(), currFirstNormal * newFirstNormal);
            if (angle < 0.01f) break;
            if (angle != 0.0f && !osg::isNaN(angle))
                rotation.preMult(osg::Matrix::rotate(angle, osg::Z_AXIS));
            maxLoopTime--;
        } while (maxLoopTime > 0);
    }

    // Add user-defined planar rotation here
    if (data._planarRotation != 0.0)
        rotation.preMult(osg::Matrix::rotate(data._planarRotation, osg::Z_AXIS));

    // Apply to  other vertices and make it dirty
    for (unsigned int i = 0; i < _segments; ++i)
    {
        unsigned int index = start + i;
        (*_vertices)[index] = shape[i] * rotation + data._center;
        (*_normals)[index] = (*_vertices)[index] - data._center;
        (*_normals)[index].normalize();
    }
    _vertices->dirty(); _normals->dirty();
}

void LoftModeler::updatePrimitiveSets()
{
    _solidGeom->removePrimitiveSet(0, _solidGeom->getNumPrimitiveSets());
    _wireframeGeom->removePrimitiveSet(0, _wireframeGeom->getNumPrimitiveSets());

    unsigned int maxIndex = _shapes.size() > 0 ? _shapes.size() - 1 : 0;
    for (unsigned int i = 0; i < maxIndex; ++i)
    {
        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_QUAD_STRIP);
        int start1 = i * _segments, start2 = (i + 1) * _segments;
        for (unsigned int j = 0; j < _segments; ++j) { de->push_back(start1 + j); de->push_back(start2 + j); }
        _solidGeom->addPrimitiveSet(de.get());

        de = new osg::DrawElementsUShort(GL_LINES);
        for (unsigned int j = 0; j < _segments; ++j)
        {
            de->push_back(start1 + j);
            de->push_back(start2 + j);
        }
        _wireframeGeom->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, start1, _segments));
        _wireframeGeom->addPrimitiveSet(de.get());
    }

    _wireframeGeom->addPrimitiveSet(
        new osg::DrawArrays(GL_LINE_STRIP, maxIndex * _segments, _segments));
    setWireframeColor(_wirframeColor);
}

void LoftModeler::updateBound()
{
    _solidGeom->dirtyBound();
    _wireframeGeom->dirtyBound();
}
