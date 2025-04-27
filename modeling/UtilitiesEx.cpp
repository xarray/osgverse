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

const unsigned int MIN_NUM_ROWS = 3;
const unsigned int MIN_NUM_SEGMENTS = 5;

ShapeGeometryVisitor::ShapeGeometryVisitor(osg::Geometry* geometry, const osg::TessellationHints* hints)
    : _geometry(geometry), _hints(hints)
{
    bool requiresClearOfPrimitiveSets = false;
    _vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    _normals = dynamic_cast<osg::Vec3Array*>(geometry->getNormalArray());
    _texcoords = dynamic_cast<osg::Vec2Array*>(geometry->getTexCoordArray(0));

    if (!_vertices || _vertices->getBinding() != osg::Array::BIND_PER_VERTEX)
    {
        requiresClearOfPrimitiveSets = true;
        _vertices = new osg::Vec3Array(osg::Array::BIND_PER_VERTEX);
        _geometry->setVertexArray(_vertices.get());
    }
    if (!_normals || (_normals->getBinding() != osg::Array::BIND_PER_VERTEX || _vertices->size() != _normals->size()))
    {
        requiresClearOfPrimitiveSets = true;
        _normals = new osg::Vec3Array(osg::Array::BIND_PER_VERTEX);
        _geometry->setNormalArray(_normals.get());
    }
    if (!_texcoords || (_texcoords->getBinding() != osg::Array::BIND_PER_VERTEX || _vertices->size() != _texcoords->size()))
    {
        requiresClearOfPrimitiveSets = true;
        _texcoords = new osg::Vec2Array(osg::Array::BIND_PER_VERTEX);
        _geometry->setTexCoordArray(0, _texcoords.get());
    }

    if (requiresClearOfPrimitiveSets && !_geometry->getPrimitiveSetList().empty())
    {
        OSG_NOTICE << "Warning: ShapeGeometryVisitor() Geometry contains compatible arrays, "
                   << "resetting before shape build." << std::endl;
        _geometry->getPrimitiveSetList().clear();
    }
    _mode = 0; _start_index = 0;
}

void ShapeGeometryVisitor::setMatrix(const osg::Matrixd& m)
{
    _matrix = m; _inverse.invert(m);
    _inverse.setTrans(0.0, 0.0, 0.0);
}

void ShapeGeometryVisitor::Vertex(const osg::Vec3f& v)
{
    _vertices->push_back(v);
    if (_normals.valid() && _normals->size() < _vertices->size())
    { while (_normals->size() < _vertices->size()) _normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f)); }
    if (_texcoords.valid() && _texcoords->size() < _vertices->size())
    { while (_texcoords->size() < _vertices->size()) _texcoords->push_back(osg::Vec2(0.0f, 0.0f)); }
}

void ShapeGeometryVisitor::Begin(GLenum mode)
{ _mode = mode; _start_index = _vertices->size(); }

void ShapeGeometryVisitor::End()
{
    if (_start_index >= _vertices->size()) return;
    bool smallPrimitiveSet = _vertices->size() < 65536;
    if (_mode == GL_QUADS)
    {
        osg::ref_ptr<osg::DrawElements> primitives = smallPrimitiveSet ?
            static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
            static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));
        _geometry->addPrimitiveSet(primitives.get());
        for (unsigned int i = _start_index; i < _vertices->size(); i += 4)
        {
            unsigned int p0 = i, p1 = i + 1, p2 = i + 2, p3 = i + 3;
            primitives->addElement(p0); primitives->addElement(p1); primitives->addElement(p3);
            primitives->addElement(p1); primitives->addElement(p2); primitives->addElement(p3);
        }
    }
    else if (_mode == GL_QUAD_STRIP)
    {
        osg::ref_ptr<osg::DrawElements> primitives = smallPrimitiveSet ?
            static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
            static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));
        _geometry->addPrimitiveSet(primitives.get());
        for (unsigned int i = _start_index; i < _vertices->size() - 2; i += 2)
        {
            unsigned int p0 = i, p1 = i + 1, p2 = i + 2, p3 = i + 3;
            primitives->addElement(p0); primitives->addElement(p1); primitives->addElement(p2);
            primitives->addElement(p1); primitives->addElement(p3); primitives->addElement(p2);
        }
    }
    else
        _geometry->addPrimitiveSet(new osg::DrawArrays(_mode, _start_index, _vertices->size() - _start_index));

    for (unsigned int i = _start_index; i < _vertices->size(); ++i)
    {
        osg::Vec3& v = (*_vertices)[i]; v = v * _matrix;
        osg::Vec3& n = (*_normals)[i]; n = _inverse * n; n.normalize();
    }
    _vertices->dirty(); _normals->dirty(); _texcoords->dirty();
    _geometry->dirtyGLObjects(); _start_index = _vertices->size();
}

void ShapeGeometryVisitor::drawCylinderBody(unsigned int numSegments, float radius, float height)
{
    const float angleDelta = 2.0f * osg::PI / (float)numSegments;
    const float texCoordDelta = 1.0f / (float)numSegments;
    const float r = radius, h = height;
    float basez = -h * 0.5f, topz = h * 0.5f;
    float angle = 0.0f, texCoord = 0.0f;
    bool drawFrontFace = _hints ? _hints->getCreateFrontFace() : true;
    bool drawBackFace = _hints ? _hints->getCreateBackFace() : false;

    Begin(GL_QUAD_STRIP);
    if (drawFrontFace)
    {
        for (unsigned int bodyi = 0; bodyi < numSegments;
             ++bodyi, angle += angleDelta, texCoord += texCoordDelta)
        {
            float c = cosf(angle), s = sinf(angle); osg::Vec3 n(c, s, 0.0f);
            Normal(n); TexCoord2f(texCoord, 1.0f); Vertex3f(c * r, s * r, topz);
            Normal(n); TexCoord2f(texCoord, 0.0f); Vertex3f(c * r, s * r, basez);
        }

        osg::Vec3 n(1.0f, 0.0f, 0.0f);  // do last point by hand to ensure no round off errors.
        Normal(n); TexCoord2f(1.0f, 1.0f); Vertex3f(r, 0.0f, topz);
        Normal(n); TexCoord2f(1.0f, 0.0f); Vertex3f(r, 0.0f, basez);
    }
    if (drawBackFace)
    {
        for (unsigned int bodyi = 0; bodyi < numSegments;
             ++bodyi, angle += angleDelta, texCoord += texCoordDelta)
        {
            float c = cosf(angle), s = sinf(angle); osg::Vec3 n(-c, -s, 0.0f);
            Normal(n); TexCoord2f(texCoord, 0.0f); Vertex3f(c * r, s * r, basez);
            Normal(n); TexCoord2f(texCoord, 1.0f); Vertex3f(c * r, s * r, topz);
        }

        osg::Vec3 n(-1.0f, 0.0f, 0.0f);  // do last point by hand to ensure no round off errors.
        Normal(n); TexCoord2f(1.0f, 0.0f); Vertex3f(r, 0.0f, basez);
        Normal(n); TexCoord2f(1.0f, 1.0f); Vertex3f(r, 0.0f, topz);
    }
    End();
}

void ShapeGeometryVisitor::drawHalfSphere(unsigned int numSegments, unsigned int numRows,
                                               float radius, SphereHalf which, float zOffset)
{
    float lDelta = osg::PI / (float)numRows, vDelta = 1.0f / (float)numRows;
    bool top = (which == SphereTopHalf);
    bool drawFrontFace = _hints ? _hints->getCreateFrontFace() : true;
    bool drawBackFace = _hints ? _hints->getCreateBackFace() : false;
    float angleDelta = osg::PI * 2.0f / (float)numSegments;
    float texCoordHorzDelta = 1.0f / (float)numSegments;
    float lBase = -osg::PI * 0.5f + (top ? (lDelta * (numRows / 2)) : 0.0f);
    float rBase = (top ? (cosf(lBase) * radius) : 0.0f);
    float zBase = (top ? (sinf(lBase) * radius) : -radius);
    float vBase = (top ? (vDelta * (numRows / 2)) : 0.0f);
    float nzBase = (top ? (sinf(lBase)) : -1.0f);
    float nRatioBase = (top ? (cosf(lBase)) : 0.0f);
    unsigned int rowbegin = top ? numRows / 2 : 0;
    unsigned int rowend = top ? numRows : numRows / 2;

    for (unsigned int rowi = rowbegin; rowi < rowend; ++rowi)
    {
        float lTop = lBase + lDelta, vTop = vBase + vDelta;
        float rTop = cosf(lTop) * radius, zTop = sinf(lTop) * radius;
        float nzTop = sinf(lTop), nRatioTop = cosf(lTop);
        float angle = 0.0f, texCoord = 0.0f;

        Begin(GL_QUAD_STRIP);
        if (drawFrontFace)
        {
            for (unsigned int topi = 0; topi < numSegments;
                 ++topi, angle += angleDelta, texCoord += texCoordHorzDelta)
            {
                float c = cosf(angle), s = sinf(angle);
                Normal3f(c * nRatioTop, s * nRatioTop, nzTop);
                TexCoord2f(texCoord, vTop); Vertex3f(c * rTop, s * rTop, zTop + zOffset);
                Normal3f(c * nRatioBase, s * nRatioBase, nzBase);
                TexCoord2f(texCoord, vBase); Vertex3f(c * rBase, s * rBase, zBase + zOffset);
            }

            // do last point by hand to ensure no round off errors.
            Normal3f(nRatioTop, 0.0f, nzTop);
            TexCoord2f(1.0f, vTop); Vertex3f(rTop, 0.0f, zTop + zOffset);
            Normal3f(nRatioBase, 0.0f, nzBase);
            TexCoord2f(1.0f, vBase); Vertex3f(rBase, 0.0f, zBase + zOffset);
        }

        if (drawBackFace)
        {
            for (unsigned int topi = 0; topi < numSegments;
                 ++topi, angle += angleDelta, texCoord += texCoordHorzDelta)
            {
                float c = cosf(angle), s = sinf(angle);
                Normal3f(-c * nRatioBase, -s * nRatioBase, -nzBase);
                TexCoord2f(texCoord, vBase); Vertex3f(c * rBase, s * rBase, zBase + zOffset);
                Normal3f(-c * nRatioTop, -s * nRatioTop, -nzTop);
                TexCoord2f(texCoord, vTop); Vertex3f(c * rTop, s * rTop, zTop + zOffset);
            }

            // do last point by hand to ensure no round off errors.
            Normal3f(-nRatioBase, 0.0f, -nzBase);
            TexCoord2f(1.0f, vBase); Vertex3f(rBase, 0.0f, zBase + zOffset);
            Normal3f(-nRatioTop, 0.0f, -nzTop);
            TexCoord2f(1.0f, vTop); Vertex3f(rTop, 0.0f, zTop + zOffset);
        }
        End();

        lBase = lTop; rBase = rTop; zBase = zTop;
        vBase = vTop; nzBase = nzTop; nRatioBase = nRatioTop;
    }
}

void ShapeGeometryVisitor::apply(const osg::Sphere& sphere)
{
    bool drawFrontFace = _hints ? _hints->getCreateFrontFace() : true;
    bool drawBackFace = _hints ? _hints->getCreateBackFace() : false;
    setMatrix(osg::Matrixd::translate(sphere.getCenter().x(), sphere.getCenter().y(), sphere.getCenter().z()));

    unsigned int numSegments = 40, numRows = 20;
    float ratio = (_hints ? _hints->getDetailRatio() : 1.0f);
    if (ratio > 0.0f && ratio != 1.0f)
    {
        numRows = (unsigned int)(numRows * ratio);
        if (numRows < MIN_NUM_ROWS) numRows = MIN_NUM_ROWS;
        numSegments = (unsigned int)(numSegments * ratio);
        if (numSegments < MIN_NUM_SEGMENTS) numSegments = MIN_NUM_SEGMENTS;
    }

    float lDelta = osg::PI / (float)numRows, vDelta = 1.0f / (float)numRows;
    float angleDelta = osg::PI * 2.0f / (float)numSegments;
    float texCoordHorzDelta = 1.0f / (float)numSegments;
    if (drawBackFace)
    {
        float lBase = -osg::PI * 0.5f, rBase = 0.0f, zBase = -sphere.getRadius();
        float vBase = 0.0f, nzBase = -1.0f, nRatioBase = 0.0f;
        for (unsigned int rowi = 0; rowi < numRows; ++rowi)
        {
            float lTop = lBase + lDelta, vTop = vBase + vDelta;
            float rTop = cosf(lTop) * sphere.getRadius();
            float zTop = sinf(lTop) * sphere.getRadius();
            float nzTop = sinf(lTop), nRatioTop = cosf(lTop);
            float angle = 0.0f, texCoord = 0.0f;

            Begin(GL_QUAD_STRIP);
            for (unsigned int topi = 0; topi < numSegments;
                 ++topi, angle += angleDelta, texCoord += texCoordHorzDelta)
            {
                float c = cosf(angle), s = sinf(angle);
                Normal3f(-c * nRatioBase, -s * nRatioBase, -nzBase);
                TexCoord2f(texCoord, vBase); Vertex3f(c * rBase, s * rBase, zBase);
                Normal3f(-c * nRatioTop, -s * nRatioTop, -nzTop);
                TexCoord2f(texCoord, vTop); Vertex3f(c * rTop, s * rTop, zTop);
            }

            // do last point by hand to ensure no round off errors.
            Normal3f(-nRatioBase, 0.0f, -nzBase);
            TexCoord2f(1.0f, vBase); Vertex3f(rBase, 0.0f, zBase);
            Normal3f(-nRatioTop, 0.0f, -nzTop);
            TexCoord2f(1.0f, vTop); Vertex3f(rTop, 0.0f, zTop);
            End();

            lBase = lTop; rBase = rTop; zBase = zTop;
            vBase = vTop; nzBase = nzTop; nRatioBase = nRatioTop;
        }
    }

    if (drawFrontFace)
    {
        float lBase = -osg::PI * 0.5f, rBase = 0.0f, zBase = -sphere.getRadius();
        float vBase = 0.0f, nzBase = -1.0f, nRatioBase = 0.0f;
        for (unsigned int rowi = 0; rowi < numRows; ++rowi)
        {
            float lTop = lBase + lDelta, vTop = vBase + vDelta;
            float rTop = cosf(lTop) * sphere.getRadius();
            float zTop = sinf(lTop) * sphere.getRadius();
            float nzTop = sinf(lTop), nRatioTop = cosf(lTop);
            float angle = 0.0f, texCoord = 0.0f;

            Begin(GL_QUAD_STRIP);
            for (unsigned int topi = 0; topi < numSegments;
                 ++topi, angle += angleDelta, texCoord += texCoordHorzDelta)
            {
                float c = cosf(angle), s = sinf(angle);
                Normal3f(c * nRatioTop, s * nRatioTop, nzTop);
                TexCoord2f(texCoord, vTop); Vertex3f(c * rTop, s * rTop, zTop);
                Normal3f(c * nRatioBase, s * nRatioBase, nzBase);
                TexCoord2f(texCoord, vBase); Vertex3f(c * rBase, s * rBase, zBase);
            }

            // do last point by hand to ensure no round off errors.
            Normal3f(nRatioTop, 0.0f, nzTop); TexCoord2f(1.0f, vTop); Vertex3f(rTop, 0.0f, zTop);
            Normal3f(nRatioBase, 0.0f, nzBase); TexCoord2f(1.0f, vBase); Vertex3f(rBase, 0.0f, zBase);
            End();
            lBase = lTop; rBase = rTop; zBase = zTop;
            vBase = vTop; nzBase = nzTop; nRatioBase = nRatioTop;
        }
    }
}

void ShapeGeometryVisitor::apply(const osg::Box& box)
{
    bool createBody = (_hints ? _hints->getCreateBody() : true);
    bool createTop = (_hints ? _hints->getCreateTop() : true);
    bool createBottom = (_hints ? _hints->getCreateBottom() : true);
    float dx = box.getHalfLengths().x(), dy = box.getHalfLengths().y(), dz = box.getHalfLengths().z();
    setMatrix(box.computeRotationMatrix() * osg::Matrixd::translate(box.getCenter()));

    Begin(GL_QUADS);
    if (createBody)
    {   // -ve y plane
        Normal3f(0.0f, -1.0f, 0.0f); TexCoord2f(0.0f, 1.0f); Vertex3f(-dx, -dy, dz);
        Normal3f(0.0f, -1.0f, 0.0f); TexCoord2f(0.0f, 0.0f); Vertex3f(-dx, -dy, -dz);
        Normal3f(0.0f, -1.0f, 0.0f); TexCoord2f(1.0f, 0.0f); Vertex3f(dx, -dy, -dz);
        Normal3f(0.0f, -1.0f, 0.0f); TexCoord2f(1.0f, 1.0f); Vertex3f(dx, -dy, dz);
        // +ve y plane
        Normal3f(0.0f, 1.0f, 0.0f); TexCoord2f(0.0f, 1.0f); Vertex3f(dx, dy, dz);
        Normal3f(0.0f, 1.0f, 0.0f); TexCoord2f(0.0f, 0.0f); Vertex3f(dx, dy, -dz);
        Normal3f(0.0f, 1.0f, 0.0f); TexCoord2f(1.0f, 0.0f); Vertex3f(-dx, dy, -dz);
        Normal3f(0.0f, 1.0f, 0.0f); TexCoord2f(1.0f, 1.0f); Vertex3f(-dx, dy, dz);
        // +ve x plane
        Normal3f(1.0f, 0.0f, 0.0f); TexCoord2f(0.0f, 1.0f); Vertex3f(dx, -dy, dz);
        Normal3f(1.0f, 0.0f, 0.0f); TexCoord2f(0.0f, 0.0f); Vertex3f(dx, -dy, -dz);
        Normal3f(1.0f, 0.0f, 0.0f); TexCoord2f(1.0f, 0.0f); Vertex3f(dx, dy, -dz);
        Normal3f(1.0f, 0.0f, 0.0f); TexCoord2f(1.0f, 1.0f); Vertex3f(dx, dy, dz);
        // -ve x plane
        Normal3f(-1.0f, 0.0f, 0.0f); TexCoord2f(0.0f, 1.0f); Vertex3f(-dx, dy, dz);
        Normal3f(-1.0f, 0.0f, 0.0f); TexCoord2f(0.0f, 0.0f); Vertex3f(-dx, dy, -dz);
        Normal3f(-1.0f, 0.0f, 0.0f); TexCoord2f(1.0f, 0.0f); Vertex3f(-dx, -dy, -dz);
        Normal3f(-1.0f, 0.0f, 0.0f); TexCoord2f(1.0f, 1.0f); Vertex3f(-dx, -dy, dz);
    }

    if (createTop)
    {   // +ve z plane
        Normal3f(0.0f, 0.0f, 1.0f); TexCoord2f(0.0f, 1.0f); Vertex3f(-dx, dy, dz);
        Normal3f(0.0f, 0.0f, 1.0f); TexCoord2f(0.0f, 0.0f); Vertex3f(-dx, -dy, dz);
        Normal3f(0.0f, 0.0f, 1.0f); TexCoord2f(1.0f, 0.0f); Vertex3f(dx, -dy, dz);
        Normal3f(0.0f, 0.0f, 1.0f); TexCoord2f(1.0f, 1.0f); Vertex3f(dx, dy, dz);
    }
    if (createBottom)
    {   // -ve z plane
        Normal3f(0.0f, 0.0f, -1.0f); TexCoord2f(0.0f, 1.0f); Vertex3f(dx, dy, -dz);
        Normal3f(0.0f, 0.0f, -1.0f); TexCoord2f(0.0f, 0.0f); Vertex3f(dx, -dy, -dz);
        Normal3f(0.0f, 0.0f, -1.0f); TexCoord2f(1.0f, 0.0f); Vertex3f(-dx, -dy, -dz);
        Normal3f(0.0f, 0.0f, -1.0f); TexCoord2f(1.0f, 1.0f); Vertex3f(-dx, dy, -dz);
    }
    End();
}

void ShapeGeometryVisitor::apply(const osg::Cone& cone)
{
    setMatrix(cone.computeRotationMatrix() * osg::Matrixd::translate(cone.getCenter()));
    bool createBody = (_hints ? _hints->getCreateBody() : true);
    bool createBottom = (_hints ? _hints->getCreateBottom() : true);

    unsigned int numSegments = 40, numRows = 10;
    float ratio = (_hints ? _hints->getDetailRatio() : 1.0f);
    if (ratio > 0.0f && ratio != 1.0f)
    {
        numRows = (unsigned int)(numRows * ratio);
        if (numRows < MIN_NUM_ROWS) numRows = MIN_NUM_ROWS;
        numSegments = (unsigned int)(numSegments * ratio);
        if (numSegments < MIN_NUM_SEGMENTS) numSegments = MIN_NUM_SEGMENTS;
    }

    float r = cone.getRadius(), h = cone.getHeight();
    float normalz = r / (sqrt(r * r + h * h));
    float normalRatio = 1.0f / (sqrt(1.0f + normalz * normalz));
    normalz *= normalRatio;
    float angleDelta = 2.0f * osg::PI / (float)numSegments;
    float texCoordHorzDelta = 1.0 / (float)numSegments;
    float texCoordRowDelta = 1.0 / (float)numRows;
    float hDelta = cone.getHeight() / (float)numRows;
    float rDelta = cone.getRadius() / (float)numRows;
    float topz = cone.getHeight() + cone.getBaseOffset();
    float topr = 0.0f, topv = 1.0f;
    float basez = topz - hDelta, baser = rDelta;
    float basev = topv - texCoordRowDelta;
    float angle, texCoord;
    if (createBody)
    {
        for (unsigned int rowi = 0; rowi < numRows;
             ++rowi, topz = basez, basez -= hDelta, topr = baser, baser += rDelta, topv = basev, basev -= texCoordRowDelta)
        {
            Begin(GL_QUAD_STRIP);
            angle = 0.0f; texCoord = 0.0f;
            for (unsigned int topi = 0; topi < numSegments;
                 ++topi, angle += angleDelta, texCoord += texCoordHorzDelta)
            {
                float c = cos(angle), s = sin(angle);
                Normal3f(c * normalRatio, s * normalRatio, normalz);
                TexCoord2f(texCoord, topv); Vertex3f(c * topr, s * topr, topz);
                Normal3f(c * normalRatio, s * normalRatio, normalz);
                TexCoord2f(texCoord, basev); Vertex3f(c * baser, s * baser, basez);
            }

            // do last point by hand to ensure no round off errors.
            Normal3f(normalRatio, 0.0f, normalz); TexCoord2f(1.0f, topv); Vertex3f(topr, 0.0f, topz);
            Normal3f(normalRatio, 0.0f, normalz); TexCoord2f(1.0f, basev); Vertex3f(baser, 0.0f, basez);
            End();
        }
    }

    if (createBottom)
    {
        Begin(GL_TRIANGLE_FAN);
        angle = osg::PI * 2.0f; texCoord = 1.0f; basez = cone.getBaseOffset();
        Normal3f(0.0f, 0.0f, -1.0f); TexCoord2f(0.5f, 0.5f); Vertex3f(0.0f, 0.0f, basez);
        for (unsigned int bottomi = 0; bottomi < numSegments;
             ++bottomi, angle -= angleDelta, texCoord -= texCoordHorzDelta)
        {
            float c = cos(angle), s = sin(angle);
            Normal3f(0.0f, 0.0f, -1.0f);
            TexCoord2f(c * 0.5f + 0.5f, s * 0.5f + 0.5f);
            Vertex3f(c * r, s * r, basez);
        }
        Normal3f(0.0f, 0.0f, -1.0f);
        TexCoord2f(1.0f, 0.0f); Vertex3f(r, 0.0f, basez);
        End();
    }
}

void ShapeGeometryVisitor::apply(const osg::Cylinder& cylinder)
{
    setMatrix(cylinder.computeRotationMatrix() * osg::Matrixd::translate(cylinder.getCenter()));
    bool createBody = (_hints ? _hints->getCreateBody() : true);
    bool createTop = (_hints ? _hints->getCreateTop() : true);
    bool createBottom = (_hints ? _hints->getCreateBottom() : true);
    unsigned int numSegments = 40;
    float ratio = (_hints ? _hints->getDetailRatio() : 1.0f);
    if (ratio > 0.0f && ratio != 1.0f)
    {
        numSegments = (unsigned int)(numSegments * ratio);
        if (numSegments < MIN_NUM_SEGMENTS) numSegments = MIN_NUM_SEGMENTS;
    }

    if (createBody) drawCylinderBody(numSegments, cylinder.getRadius(), cylinder.getHeight());
    float angleDelta = 2.0f * osg::PI / (float)numSegments;
    float texCoordDelta = 1.0f / (float)numSegments;
    float r = cylinder.getRadius(), h = cylinder.getHeight();
    float basez = -h * 0.5f, topz = h * 0.5f;
    float angle = 0.0f, texCoord = 0.0f;

    if (createTop)
    {
        Begin(GL_TRIANGLE_FAN);
        Normal3f(0.0f, 0.0f, 1.0f);
        TexCoord2f(0.5f, 0.5f); Vertex3f(0.0f, 0.0f, topz);
        angle = 0.0f; texCoord = 0.0f;
        for (unsigned int topi = 0; topi < numSegments;
             ++topi, angle += angleDelta, texCoord += texCoordDelta)
        {
            float c = cos(angle), s = sin(angle);
            Normal3f(0.0f, 0.0f, 1.0f);
            TexCoord2f(c * 0.5f + 0.5f, s * 0.5f + 0.5f); Vertex3f(c * r, s * r, topz);
        }
        Normal3f(0.0f, 0.0f, 1.0f); TexCoord2f(1.0f, 0.5f); Vertex3f(r, 0.0f, topz);
        End();
    }

    if (createBottom)
    {
        Begin(GL_TRIANGLE_FAN);
        Normal3f(0.0f, 0.0f, -1.0f);
        TexCoord2f(0.5f, 0.5f); Vertex3f(0.0f, 0.0f, basez);
        angle = osg::PI * 2.0f; texCoord = 1.0f;
        for (unsigned int bottomi = 0; bottomi < numSegments;
             ++bottomi, angle -= angleDelta, texCoord -= texCoordDelta)
        {
            float c = cos(angle), s = sin(angle);
            Normal3f(0.0f, 0.0f, -1.0f);
            TexCoord2f(c * 0.5f + 0.5f, s * 0.5f + 0.5f);
            Vertex3f(c * r, s * r, basez);
        }
        Normal3f(0.0f, 0.0f, -1.0f);
        TexCoord2f(1.0f, 0.5f); Vertex3f(r, 0.0f, basez);
        End();
    }
}

void ShapeGeometryVisitor::apply(const osg::Capsule& capsule)
{
    setMatrix(capsule.computeRotationMatrix() * osg::Matrixd::translate(capsule.getCenter()));
    bool createBody = (_hints ? _hints->getCreateBody() : true);
    bool createTop = (_hints ? _hints->getCreateTop() : true);
    bool createBottom = (_hints ? _hints->getCreateBottom() : true);
    unsigned int numSegments = 40, numRows = 20;
    float ratio = (_hints ? _hints->getDetailRatio() : 1.0f);
    if (ratio > 0.0f && ratio != 1.0f)
    {
        numSegments = (unsigned int)(numSegments * ratio);
        if (numSegments < MIN_NUM_SEGMENTS) numSegments = MIN_NUM_SEGMENTS;
        numRows = (unsigned int)(numRows * ratio);
        if (numRows < MIN_NUM_ROWS) numRows = MIN_NUM_ROWS;
    }

    if ((numRows % 2) != 0) ++numRows;
    if (createBody) drawCylinderBody(numSegments, capsule.getRadius(), capsule.getHeight());
    if (createTop) drawHalfSphere(numSegments, numRows, capsule.getRadius(), SphereTopHalf, capsule.getHeight() / 2.0f);
    if (createBottom) drawHalfSphere(numSegments, numRows, capsule.getRadius(), SphereBottomHalf, -capsule.getHeight() / 2.0f);
}

void ShapeGeometryVisitor::apply(const osg::InfinitePlane&)
{ OSG_NOTICE << "Warning: ShapeGeometryVisitor::apply(const InfinitePlane& plane) not yet implemented. " << std::endl; }

void ShapeGeometryVisitor::apply(const osg::TriangleMesh& mesh)
{
    const osg::Vec3Array* vertices = mesh.getVertices();
    const osg::IndexArray* indices = mesh.getIndices();
    if (vertices && indices)
    {
        Begin(GL_TRIANGLES);
        for (unsigned int i = 0; i + 2 < indices->getNumElements(); i += 3)
        {
            const osg::Vec3& v1 = (*vertices)[indices->index(i)];
            const osg::Vec3& v2 = (*vertices)[indices->index(i + 1)];
            const osg::Vec3& v3 = (*vertices)[indices->index(i + 2)];
            osg::Vec3 normal = (v2 - v1) ^ (v3 - v2); normal.normalize();
            Normal(normal); Vertex(v1);
            Normal(normal); Vertex(v2);
            Normal(normal); Vertex(v3);
        }
        End();
    }
}

void ShapeGeometryVisitor::apply(const osg::ConvexHull& hull)
{ apply((const osg::TriangleMesh&)hull); }

void ShapeGeometryVisitor::apply(const osg::HeightField& field)
{
    if (field.getNumColumns() == 0 || field.getNumRows() == 0) return;
    setMatrix(field.computeRotationMatrix() * osg::Matrixd::translate(field.getOrigin()));
    float dx = field.getXInterval(), dy = field.getYInterval();
    float du = 1.0f / ((float)field.getNumColumns() - 1.0f);
    float dv = 1.0f / ((float)field.getNumRows() - 1.0f), vBase = 0.0f;
    osg::Vec3 vertTop, normTop, vertBase, normBase;

    if (field.getSkirtHeight() != 0.0f)
    {
        Begin(GL_QUAD_STRIP);
        float u = 0.0f;

        // draw bottom skirt
        unsigned int col; vertTop.y() = 0.0f;
        for (col = 0; col < field.getNumColumns(); ++col, u += du)
        {
            vertTop.x() = dx * (float)col; vertTop.z() = field.getHeight(col, 0);
            normTop.set(field.getNormal(col, 0));
            TexCoord2f(u, 0.0f); Normal(normTop); Vertex(vertTop);

            vertTop.z() -= field.getSkirtHeight();
            TexCoord2f(u, 0.0f); Normal(normTop); Vertex(vertTop);
        }
        End();

        // draw top skirt
        Begin(GL_QUAD_STRIP);
        unsigned int row = field.getNumRows() - 1;
        u = 0.0f; vertTop.y() = dy * (float)(row);
        for (col = 0; col < field.getNumColumns(); ++col, u += du)
        {
            vertTop.x() = dx * (float)col; vertTop.z() = field.getHeight(col, row);
            normTop.set(field.getNormal(col, row));
            TexCoord2f(u, 1.0f); Normal(normTop);
            Vertex3f(vertTop.x(), vertTop.y(), vertTop.z() - field.getSkirtHeight());
            TexCoord2f(u, 1.0f); Normal(normTop); Vertex(vertTop);
        }
        End();
    }

    // draw each row of HeightField
    for (unsigned int row = 0; row < field.getNumRows() - 1; ++row, vBase += dv)
    {
        float vTop = vBase + dv, u = 0.0f;
        Begin(GL_QUAD_STRIP);
        if (field.getSkirtHeight() != 0.0f)
        {
            vertTop.set(0.0f, dy * (float)(row + 1), field.getHeight(0, row + 1) - field.getSkirtHeight());
            normTop.set(field.getNormal(0, row + 1));
            vertBase.set(0.0f, dy * (float)row, field.getHeight(0, row) - field.getSkirtHeight());
            normBase.set(field.getNormal(0, row));
            TexCoord2f(u, vTop); Normal(normTop); Vertex(vertTop);
            TexCoord2f(u, vBase); Normal(normBase); Vertex(vertBase);
        }

        for (unsigned int col = 0; col < field.getNumColumns(); ++col, u += du)
        {
            vertTop.set(dx * (float)col, dy * (float)(row + 1), field.getHeight(col, row + 1));
            normTop.set(field.getNormal(col, row + 1));
            vertBase.set(dx * (float)col, dy * (float)row, field.getHeight(col, row));
            normBase.set(field.getNormal(col, row));
            TexCoord2f(u, vTop); Normal(normTop); Vertex(vertTop);
            TexCoord2f(u, vBase); Normal(normBase); Vertex(vertBase);
        }

        if (field.getSkirtHeight() != 0.0f)
        {
            vertBase.z() -= field.getSkirtHeight(); vertTop.z() -= field.getSkirtHeight();
            TexCoord2f(u, vTop); Normal(normTop); Vertex(vertTop);
            TexCoord2f(u, vBase); Normal(normBase); Vertex(vertBase);
        }
        End();
    }
}

void ShapeGeometryVisitor::apply(const osg::CompositeShape& group)
{
    for (unsigned int i = 0; i < group.getNumChildren(); ++i)
        group.getChild(i)->accept(*this);
}

static osg::Vec3 computeMidpointOnSphere(const osg::Vec3& a, const osg::Vec3& b,
                                         const osg::Vec3& center, float radius)
{
    osg::Vec3 unitRadial = (a + b) * 0.5f - center; unitRadial.normalize();
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
        { de.push_back(c); de.push_back(b); de.push_back(a); }
    else  // subdivide recursively
    {
        // Find edge midpoints
        unsigned int ab = va.size(); va.push_back(computeMidpointOnSphere(v1, v2, center, radius));
        unsigned int bc = va.size(); va.push_back(computeMidpointOnSphere(v2, v3, center, radius));
        unsigned int ca = va.size(); va.push_back(computeMidpointOnSphere(v3, v1, center, radius));

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
