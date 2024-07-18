#include <osg/Version>
#include <osg/io_utils>
#include <osg/ImageUtils>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgUtil/SmoothingVisitor>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <iostream>

#define STB_RECT_PACK_IMPLEMENTATION
#define ENABLE_VHACD_IMPLEMENTATION 1
#include <stb/stb_rect_pack.h>
#include "3rdparty/VHACD.h"
#include "3rdparty/ApproxMVBB/ComputeApproxMVBB.hpp"
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

struct CollectVertexOperator
{
    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (vertices && vertices->size() <= baseIndex)
        {
            std::vector<bool> vertexAddingList;
            for (size_t i = 0; i < inputV->size(); ++i)
            {
                osg::Vec3 v = (*inputV)[i] * matrix;
                if (vertexMap)
                {
                    if (vertexMap->find(v) == vertexMap->end())
                    {
                        (*vertexMap)[v] = vertices->size();
                        vertexAddingList.push_back(true);
                        vertices->push_back(v);
                    }
                    else
                        vertexAddingList.push_back(false);
                    indexMap[baseIndex + i] = (*vertexMap)[v] - baseIndex;
                }
                else vertices->push_back(v);
            }

            if (attributes)
            {
                std::vector<osg::Vec4>& na = (*attributes)[MeshCollector::NormalAttr];
                std::vector<osg::Vec4>& ca = (*attributes)[MeshCollector::ColorAttr];
                std::vector<osg::Vec4>& ta = (*attributes)[MeshCollector::UvAttr];
                for (size_t i = 0; i < inputV->size(); ++i)
                {
                    if (vertexMap && !vertexAddingList[i]) continue;
                    if (inputN)
                    {
                        osg::Vec3 n = osg::Matrix::transform3x3(osg::Matrix::inverse(matrix), (*inputN)[i]);
                        na.push_back(osg::Vec4(n, 0.0));
                    }
                    if (inputC) ca.push_back((*inputC)[i]);
                    if (inputT) ta.push_back(osg::Vec4((*inputT)[i].x(),
                                                       (*inputT)[i].y(), 0.0f, 1.0));
                }
            }
        }

        if (indices)
        {
            if (vertexMap)
            {
                i1 = indexMap[baseIndex + i1]; i2 = indexMap[baseIndex + i2];
                i3 = indexMap[baseIndex + i3];
            }

            if (i1 == i2 || i2 == i3 || i1 == i3) return;
            indices->push_back(baseIndex + i1); indices->push_back(baseIndex + i2);
            indices->push_back(baseIndex + i3);
        }
    }

    CollectVertexOperator()
    :   inputV(NULL), inputN(NULL), inputT(NULL), inputC(NULL), vertexMap(NULL),
        vertices(NULL), indices(NULL), attributes(NULL), baseIndex(0) {}
    osg::Vec3Array *inputV, *inputN;
    osg::Vec2Array *inputT; osg::Vec4Array *inputC;
    std::map<osg::Vec3, unsigned int, Vec3MapComparer>* vertexMap;
    std::map<unsigned int, unsigned int> indexMap;

    std::vector<osg::Vec3>* vertices; std::vector<unsigned int>* indices;
    std::map<MeshCollector::VertexAttribute, std::vector<osg::Vec4>>* attributes;
    osg::Matrix matrix; unsigned int baseIndex;
};

MeshCollector::MeshCollector()
:   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _weldVertices(false), _globalVertices(false),
    _loadedFineLevels(false), _onlyVertexAndIndices(false) {}

void MeshCollector::reset()
{
    _matrixStack.clear(); _boundingBox.init();
    _vertexMap.clear(); _attributes.clear();
    _vertices.clear(); _indices.clear();
}

void MeshCollector::apply(osg::Node& node)
{
    if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());
    traverse(node);
}

void MeshCollector::apply(osg::PagedLOD& node)
{
    if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());
    if (_loadedFineLevels)
    {
        if (node.getNumFileNames() > 0 && _traversalMode == TRAVERSE_ALL_CHILDREN)
        {
            float targetRangeValue = 0.0f;
            if (node.getRangeMode() == osg::LOD::DISTANCE_FROM_EYE_POINT) targetRangeValue = 1e6;
            else targetRangeValue = 0;

            const osg::LOD::RangeList rl = node.getRangeList();
            for (osg::LOD::RangeList::const_iterator rit = rl.begin(); rit != rl.end(); rit++)
            {
                if (node.getRangeMode() == osg::LOD::DISTANCE_FROM_EYE_POINT)
                { if (rit->first < targetRangeValue) targetRangeValue = rit->first; }
                else
                { if (rit->first > targetRangeValue) targetRangeValue = rit->first; }
            }

            unsigned int childIndex = 0;
            for (osg::LOD::RangeList::const_iterator rit = rl.begin();
                 rit != rl.end(); rit++, childIndex++)
            {
                osg::ref_ptr<osg::Node> child;
                if (rit->first != targetRangeValue) continue;
                if (childIndex < node.getNumChildren()) child = node.getChild(childIndex);

                if (!child)
                {   // Child is NULL; attempt to load it
                    unsigned int validIndex(childIndex);
                    if (node.getNumFileNames() <= childIndex) validIndex = node.getNumFileNames() - 1;
                    child = osgDB::readNodeFile(node.getDatabasePath() + node.getFileName(validIndex));
                }

                if (!child && node.getNumChildren() > 0)
                    child = node.getChild(node.getNumChildren() - 1);
                if (child.valid()) child->accept(*this);
            }
            return;
        }
    }
    traverse(node);
}

void MeshCollector::apply(osg::ProxyNode& node)
{
    if (node.getStateSet()) apply(&node, NULL, *node.getStateSet()); traverse(node);
    if (!_loadedFineLevels || _traversalMode != TRAVERSE_ALL_CHILDREN) return;

    for (unsigned int i = node.getNumChildren(); i < node.getNumFileNames(); ++i)
    {
        osg::ref_ptr<osg::Node> child = osgDB::readNodeFile(
            node.getDatabasePath() + node.getFileName(i));
        OSG_NOTICE << "[MeshCollector] Loaded " << node.getFileName(i) << std::endl;
        if (child.valid()) child->accept(*this);
    }
}

void MeshCollector::apply(osg::Transform& node)
{
    osg::Matrix matrix;
    if (!_matrixStack.empty()) matrix = _matrixStack.back();
    node.computeLocalToWorldMatrix(matrix, this);
    if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());

    pushMatrix(matrix);
    traverse(node);
    popMatrix();
}

void MeshCollector::apply(osg::Geode& node)
{
    if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
    for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
    {
        osg::Geometry* geom = node.getDrawable(i)->asGeometry();
        if (geom != NULL) apply(*geom);
    }
#endif

    osg::Matrix matrix;
    if (_matrixStack.size() > 0) matrix = _matrixStack.back();
    for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
    {
        const osg::BoundingBox& bbox = node.getDrawable(i)->getBoundingBox();
        _boundingBox.expandBy(bbox.corner(0) * matrix);
        _boundingBox.expandBy(bbox.corner(1) * matrix);
        _boundingBox.expandBy(bbox.corner(2) * matrix);
        _boundingBox.expandBy(bbox.corner(3) * matrix);
        _boundingBox.expandBy(bbox.corner(4) * matrix);
        _boundingBox.expandBy(bbox.corner(5) * matrix);
        _boundingBox.expandBy(bbox.corner(6) * matrix);
        _boundingBox.expandBy(bbox.corner(7) * matrix);
    }
    traverse(node);
}

void MeshCollector::apply(osg::Geometry& geom)
{
    osg::Matrix matrix;
    if (_matrixStack.size() > 0) matrix = _matrixStack.back();

    osg::StateSet* ss = geom.getStateSet();
    if (ss) apply(geom.getNumParents() > 0 ? geom.getParent(0) : NULL, &geom, *ss);

    osg::TriangleIndexFunctor<CollectVertexOperator> functor;
    functor.inputV = static_cast<osg::Vec3Array*>(geom.getVertexArray());
    if (!_onlyVertexAndIndices)
        functor.inputT = static_cast<osg::Vec2Array*>(geom.getTexCoordArray(0));
    if (!_onlyVertexAndIndices && geom.getNormalBinding() == osg::Geometry::BIND_PER_VERTEX)
        functor.inputN = static_cast<osg::Vec3Array*>(geom.getNormalArray());
    if (!_onlyVertexAndIndices && geom.getColorBinding() == osg::Geometry::BIND_PER_VERTEX)
        functor.inputC = static_cast<osg::Vec4Array*>(geom.getColorArray());

    functor.vertices = &_vertices; functor.attributes = &_attributes;
    functor.indices = &_indices; functor.matrix = matrix;
    functor.baseIndex = _vertices.size();
    if (_weldVertices)
    {
        if (!_globalVertices) _vertexMap.clear();
        functor.vertexMap = &_vertexMap;
    }
    geom.accept(functor);
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    traverse(geom);
#endif
}

void MeshCollector::apply(osg::Node* n, osg::Drawable* d, osg::StateSet& ss)
{
    osg::StateSet::TextureAttributeList& texAttrList = ss.getTextureAttributeList();
    for (size_t i = 0; i < texAttrList.size(); ++i)
    {
        osg::StateSet::AttributeList& attr = texAttrList[0];
        for (osg::StateSet::AttributeList::iterator itr = attr.begin();
             itr != attr.end(); ++itr)
        {
            osg::StateAttribute::Type t = itr->first.first;
            if (t == osg::StateAttribute::TEXTURE)
                apply(n ,d, static_cast<osg::Texture*>(itr->second.first.get()), i);
        }
    }
}

osg::Geometry* BoundingVolumeVisitor::computeVHACD(bool findBestPlane, bool shrinkWrap,
                                                   int resolution, int maxConvexHulls, float maxError)
{
    VHACD::IVHACD::Parameters params;
    params.m_findBestPlane = findBestPlane;
    params.m_shrinkWrap = shrinkWrap;
    if (resolution > 0) params.m_resolution = resolution;
    if (maxConvexHulls > 0) params.m_maxConvexHulls = maxConvexHulls;
    if (maxError > 0.0f) params.m_minimumVolumePercentErrorAllowed = maxError;
    // params.m_maxRecursionDepth; params.m_fillMode; params.m_maxNumVerticesPerCH; params.m_minEdgeLength
    if (_vertices.empty() || _indices.empty()) return NULL;

    std::vector<double> points(_vertices.size() * 3);
    for (size_t i = 0; i < _vertices.size(); ++i)
    {
        size_t index = i * 3; const osg::Vec3& pt = _vertices[i];
        points[index] = pt[0]; points[index + 1] = pt[1]; points[index + 2] = pt[2];
    }

    VHACD::IVHACD* iface = VHACD::CreateVHACD();
    iface->Compute(&points[0], _vertices.size(), &_indices[0], _indices.size() / 3, params);
    if (iface->GetNConvexHulls() == 0) return NULL;

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
    for (uint32_t i = 0; i < iface->GetNConvexHulls(); ++i)
    {
        VHACD::IVHACD::ConvexHull ch; iface->GetConvexHull(i, ch);
        unsigned int baseIndex = va->size();
        for (size_t v = 0; v < ch.m_points.size(); ++v)
        {
            const VHACD::Vertex& pt = ch.m_points[v];
            va->push_back(osg::Vec3(pt.mX, pt.mY, pt.mZ));
        }

        for (size_t v = 0; v < ch.m_triangles.size(); ++v)
        {
            const VHACD::Triangle& t = ch.m_triangles[v]; de->push_back(baseIndex + t.mI0);
            de->push_back(baseIndex + t.mI1); de->push_back(baseIndex + t.mI2);
        }
    }

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get()); geom->addPrimitiveSet(de.get());
    iface->Release(); return geom.release();
}

osg::BoundingBox BoundingVolumeVisitor::computeOBB(osg::Quat& rotation, float relativeExtent, int numSamples)
{
    ApproxMVBB::Matrix3Dyn points(3, _vertices.size());
    for (size_t i = 0; i < _vertices.size(); ++i)
    {
        const osg::Vec3& v = _vertices[i];
        points.col(i) << v[0], v[1], v[2];
    }

    ApproxMVBB::OOBB oobb = ApproxMVBB::approximateMVBB(points, 0.001, numSamples);
    oobb.expandToMinExtentRelative(relativeExtent);
    rotation.set(oobb.m_q_KI.x(), oobb.m_q_KI.y(), oobb.m_q_KI.z(), oobb.m_q_KI.w());
    return osg::BoundingBox(osg::Vec3(oobb.m_minPoint.x(), oobb.m_minPoint.y(), oobb.m_minPoint.z()),
                            osg::Vec3(oobb.m_maxPoint.x(), oobb.m_maxPoint.y(), oobb.m_maxPoint.z()));
}

void MeshTopologyVisitor::apply(osg::Node* n, osg::Drawable* d, osg::StateSet& ss)
{
    if (!_stateset) _stateset = new osg::StateSet;
    if (_stateset->getName().empty()) _stateset->setName(ss.getName());
    else _stateset->setName(_stateset->getName() + "+" + ss.getName());
    _stateset->merge(ss);
}

MeshTopology* MeshTopologyVisitor::generate()
{
    osg::ref_ptr<MeshTopology> mesh = new MeshTopology;
    mesh->generate(this); return mesh.release();
}

/// TexturePacker ///

void TexturePacker::clear()
{ _input.clear(); _result.clear(); _dictIndex = 0; }

size_t TexturePacker::addElement(osg::Image* image)
{ _input[++_dictIndex] = InputPair(image, osg::Vec4()); return _dictIndex; }

size_t TexturePacker::addElement(int w, int h)
{ _input[++_dictIndex] = InputPair(NULL, osg::Vec4(0, 0, w, h)); return _dictIndex; }

void TexturePacker::removeElement(size_t id)
{ if (_input.find(id) != _input.end()) _input.erase(_input.find(id)); }

osg::Image* TexturePacker::pack(size_t& numImages, bool generateResult, bool stopIfFailed)
{
    stbrp_context context; int ptr = 0, totalW = 0, totalH = 0;
    int maxSize = osg::maximum(_maxWidth, _maxHeight) * 2;
    stbrp_node* nodes = (stbrp_node*)malloc(sizeof(stbrp_node) * maxSize);
    if (nodes) memset(nodes, 0, sizeof(stbrp_node) * maxSize);

    stbrp_rect* rects = (stbrp_rect*)malloc(sizeof(stbrp_rect) * _input.size());
    for (std::map<size_t, InputPair>::iterator itr = _input.begin();
         itr != _input.end(); ++itr, ++ptr)
    {
        stbrp_rect& r = rects[ptr];
        InputPair& pair = itr->second;
        r.id = itr->first; r.was_packed = 0;
        r.x = 0; r.w = pair.first.valid() ? pair.first->s() : pair.second[2];
        r.y = 0; r.h = pair.first.valid() ? pair.first->t() : pair.second[3];
    }

    stbrp_init_target(&context, _maxWidth, _maxHeight, nodes, maxSize);
    stbrp_pack_rects(&context, rects, _input.size());
    free(nodes); _result.clear(); ptr = 0;

    osg::observer_ptr<osg::Image> validChild;
    for (std::map<size_t, InputPair>::iterator itr = _input.begin();
         itr != _input.end(); ++itr, ++ptr)
    {
        InputPair& pair = itr->second;
        stbrp_rect& r = rects[ptr];
        osg::Vec4 v(r.x, r.y, r.w, r.h);

        if (r.id != itr->first || !r.was_packed)
        {
            OSG_NOTICE << "[TexturePacker] Bad packing element: " << pair.first->getFileName()
                       << ", order = " << ptr << "/" << _input.size() << ", rect = " << v
                       << ", packed = " << r.was_packed << std::endl;
            if (stopIfFailed) return NULL; else continue;
        }

        if (totalW < (r.x + r.w)) totalW = r.x + r.w;
        if (totalH < (r.y + r.h)) totalH = r.y + r.h;
        _result[itr->first] = InputPair(pair.first, v);
        if (pair.first.valid()) validChild = pair.first;
    }
    free(rects); numImages = _result.size();
    if (!generateResult) return NULL;

    osg::ref_ptr<osg::Image> total = new osg::Image;
    if (validChild.valid())
    {
        total->allocateImage(totalW, totalH, 1,
            validChild->getPixelFormat(), validChild->getDataType());
        total->setInternalTextureFormat(validChild->getInternalTextureFormat());
    }
    else
        total->allocateImage(totalW, totalH, 1, GL_RGBA, GL_UNSIGNED_BYTE);

    for (std::map<size_t, InputPair>::iterator itr = _result.begin();
         itr != _result.end(); ++itr)
    {
        InputPair& pair = itr->second;
        const osg::Vec4& r = itr->second.second;
        if (!pair.first.valid()) continue;

        if (!osg::copyImage(pair.first.get(), 0, 0, 0, r[2], r[3], 1,
                            total.get(), r[0], r[1], 0))
        { OSG_WARN << "[TexturePacker] Failed to copy image " << itr->first << std::endl; }
    }
    return total.release();
}

bool TexturePacker::getPackingData(size_t id, int& x, int& y, int& w, int& h)
{
    if (_result.find(id) != _result.end())
    {
        const osg::Vec4& rect = _result[id].second;
        x = rect[0]; y = rect[1]; w = rect[2]; h = rect[3];
        return true;
    }
    return false;
}

namespace osgVerse
{

    osg::Geometry* createGeometry(osg::Vec3Array* va, osg::Vec3Array* na,
                                  osg::Vec2Array* ta, osg::PrimitiveSet* p,
                                  bool autoNormals, bool useVBO)
    {
        if (!va || !p)
        {
            OSG_NOTICE << "createGeometry: invalid parameters" << std::endl;
            return NULL;
        }

        osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array(1);
        (*ca)[0] = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setVertexArray(va);
        geom->setTexCoordArray(0, ta);
        geom->setColorArray(ca.get());
        geom->setColorBinding(osg::Geometry::BIND_OVERALL);
        geom->addPrimitiveSet(p);
        if (useVBO)
        {
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
        }

        if (na)
        {
            unsigned int normalSize = na->size();
            if (normalSize == va->size())
            {
                geom->setNormalArray(na);
                geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
            }
            else if (normalSize == 1)
            {
                geom->setNormalArray(na);
                geom->setNormalBinding(osg::Geometry::BIND_OVERALL);
            }
        }
        else if (autoNormals)
            osgUtil::SmoothingVisitor::smooth(*geom);
        return geom.release();
    }

    osg::Geometry* createGeometry(osg::Vec3Array* va, osg::Vec3Array* na, const osg::Vec4& color,
                                  osg::PrimitiveSet* p, bool autoNormals, bool useVBO)
    {
        osg::Geometry* geom = createGeometry(va, na, NULL, p, autoNormals, useVBO);
        osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(geom->getColorArray());
        if (ca && ca->size() > 0) ca->front() = color;
        return geom;
    }

    osg::Geometry* createEllipsoid(const osg::Vec3& center, float radius1, float radius2,
                                   float radius3, int samples)
    {
        if (samples < 2 || radius1 <= 0.0f || radius2 <= 0.0f || radius3 <= 0.0f)
        {
            OSG_NOTICE << "createEllipsoid: invalid parameters" << std::endl;
            return NULL;
        }

        int halfSamples = samples / 2;
        float samplesF = (float)samples;

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        for (int j = 0; j < halfSamples; ++j)
        {
            float theta1 = 2.0f * (float)j * osg::PI / samplesF - osg::PI_2;
            float theta2 = 2.0f * (float)(j + 1) * osg::PI / samplesF - osg::PI_2;
            for (int i = 0; i <= samples; ++i)
            {
                float theta3 = 2.0f * (float)i * osg::PI / samplesF;
                osg::Vec3 e;
                e.x() = cosf(theta1) * cosf(theta3) * radius1;
                e.y() = sinf(theta1) * radius2;
                e.z() = cosf(theta1) * sinf(theta3) * radius3;

                va->push_back(center + e);
                ta->push_back(osg::Vec2((float)i / samplesF, 2.0f * (float)j / samplesF));

                e.x() = cosf(theta2) * cosf(theta3) * radius1;
                e.y() = sinf(theta2) * radius2;
                e.z() = cosf(theta2) * sinf(theta3) * radius3;

                va->push_back(center + e);
                ta->push_back(osg::Vec2((float)i / samplesF, 2.0f * (float)(j + 1) / samplesF));
            }
        }

        osg::ref_ptr<osg::DrawArrays> de = new osg::DrawArrays(GL_QUAD_STRIP, 0, va->size());
        return createGeometry(va.get(), NULL, ta.get(), de.get());
    }

    osg::Geometry* createSuperEllipsoid(const osg::Vec3& center, float radius, float power1,
                                        float power2, int samples)
    {
        struct EvalSuperEllipsoid
        {
            static void eval(float t1, float t2, float p1, float p2, osg::Vec3& pt)
            {
                float ct1 = cosf(t1), ct2 = cosf(t2);
                float st1 = sinf(t1), st2 = sinf(t2);
                float tmp = osg::sign(ct1) * powf(fabs(ct1), p1);
                pt.x() = tmp * osg::sign(ct2) * pow(fabs(ct2), p2);
                pt.y() = osg::sign(st1) * pow(fabs(st1), p1);
                pt.z() = tmp * osg::sign(st2) * pow(fabs(st2), p2);
            }
        };

        if (samples < 4 || (power1 > 10.0f && power2 > 10.0f))
        {
            OSG_NOTICE << "createSuperEllipsoid: invalid parameters" << std::endl;
            return NULL;
        }

        int halfSamples = samples / 2;
        float samplesF = (float)samples;
        float delta = 0.02 * osg::PI / samplesF;

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        for (int j = 0; j < halfSamples; ++j)
        {
            float theta1 = 2.0f * (float)j * osg::PI / samplesF - osg::PI_2;
            float theta2 = 2.0f * (float)(j + 1) * osg::PI / samplesF - osg::PI_2;
            for (int i = 0; i <= samples; ++i)
            {
                float theta3 = ((i == 0 || i == samples) ? 0.0f : 2.0f * (float)i * osg::PI / samplesF);
                osg::Vec3 pt;
                EvalSuperEllipsoid::eval(theta1, theta3, power1, power2, pt);
                va->push_back(pt * radius);
                ta->push_back(osg::Vec2((float)i / samplesF, 2.0f * (float)(j + 1) / samplesF));

                EvalSuperEllipsoid::eval(theta2, theta3, power1, power2, pt);
                va->push_back(pt * radius);
                ta->push_back(osg::Vec2((float)i / samplesF, 2.0f * (float)j / samplesF));
            }
        }

        osg::ref_ptr<osg::DrawArrays> de = new osg::DrawArrays(GL_QUAD_STRIP, 0, va->size());
        return createGeometry(va.get(), NULL, ta.get(), de.get());
    }

    osg::Geometry* createPrism(const osg::Vec3& centerBottom, float radiusBottom, float radiusTop,
                               float height, int n, bool capped)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        float samplesF = (float)n;
        for (int i = 0; i <= n; ++i)
        {
            float theta = 2.0f * (float)i * osg::PI / samplesF + osg::PI_4;
            osg::Vec3 r(radiusTop * cosf(theta), radiusTop * sinf(theta), height);
            va->push_back(centerBottom + r);
            ta->push_back(osg::Vec2((float)i / samplesF, 1.0f));

            r.x() = radiusBottom * cosf(theta);
            r.y() = radiusBottom * sinf(theta);
            r.z() = 0.0f;
            va->push_back(centerBottom + r);
            ta->push_back(osg::Vec2((float)i / samplesF, 0.0f));
        }

        osg::ref_ptr<osg::DrawArrays> de = new osg::DrawArrays(GL_QUAD_STRIP, 0, va->size());
        if (capped)  // Add center points
        {
            va->push_back(centerBottom + osg::Vec3(0.0f, 0.0f, height));
            ta->push_back(osg::Vec2(1.0f, 1.0f));
            va->push_back(centerBottom);
            ta->push_back(osg::Vec2(0.0f, 0.0f));
        }

        osg::Geometry* geom = createGeometry(va.get(), NULL, ta.get(), de.get(), !capped);
        if (geom && capped)
        {
            osg::ref_ptr<osg::DrawElementsUByte> top = new osg::DrawElementsUByte(GL_TRIANGLE_FAN);
            osg::ref_ptr<osg::DrawElementsUByte> bottom = new osg::DrawElementsUByte(GL_TRIANGLE_FAN);
            top->push_back(va->size() - 2);
            for (int i = 0; i <= n; ++i)
            {
                top->push_back(i * 2);
                bottom->insert(bottom->begin(), i * 2 + 1);
            }
            bottom->insert(bottom->begin(), va->size() - 1);

            geom->addPrimitiveSet(top.get());
            geom->addPrimitiveSet(bottom.get());
            osgUtil::SmoothingVisitor::smooth(*geom);
        }
        return geom;
    }

    osg::Geometry* createPyramid(const osg::Vec3& centerBottom, float radius, float height,
                                 int n, bool capped)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        va->push_back(centerBottom + osg::Vec3(0.0f, 0.0f, height));
        ta->push_back(osg::Vec2(1.0f, 1.0f));

        float samplesF = (float)n;
        for (int i = 0; i <= n; ++i)
        {
            float theta = 2.0f * (float)i * osg::PI / samplesF;
            osg::Vec3 r;
            r.x() = radius * cosf(theta);
            r.y() = radius * sinf(theta);
            va->push_back(centerBottom + r);
            ta->push_back(osg::Vec2((float)i / samplesF, 0.0f));
        }

        osg::ref_ptr<osg::DrawArrays> de = new osg::DrawArrays(GL_TRIANGLE_FAN, 0, va->size());
        if (capped)  // Add center points
        {
            va->push_back(centerBottom);
            ta->push_back(osg::Vec2(0.0f, 0.0f));
        }

        osg::Geometry* geom = createGeometry(va.get(), NULL, ta.get(), de.get(), !capped);
        if (geom && capped)
        {
            osg::ref_ptr<osg::DrawElementsUByte> bottom = new osg::DrawElementsUByte(GL_TRIANGLE_FAN);
            for (int i = 0; i <= n; ++i)
            {
                bottom->insert(bottom->begin(), i + 1);
            }
            bottom->insert(bottom->begin(), va->size() - 1);

            geom->addPrimitiveSet(bottom.get());
            osgUtil::SmoothingVisitor::smooth(*geom);
        }
        return geom;
    }

    osg::Geometry* createViewFrustumGeometry(const osg::Matrix& view, const osg::Matrix& proj)
    {
        osg::Matrix invMVP = osg::Matrix::inverse(view * proj);
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(8);
        (*va)[0] = osg::Vec3(-1.0f, -1.0f, -1.0f) * invMVP;
        (*va)[1] = osg::Vec3(1.0f, -1.0f, -1.0f) * invMVP;
        (*va)[2] = osg::Vec3(1.0f, 1.0f, -1.0f) * invMVP;
        (*va)[3] = osg::Vec3(-1.0f, 1.0f, -1.0f) * invMVP;
        (*va)[4] = osg::Vec3(-1.0f, -1.0f, 1.0f) * invMVP;
        (*va)[5] = osg::Vec3(1.0f, -1.0f, 1.0f) * invMVP;
        (*va)[6] = osg::Vec3(1.0f, 1.0f, 1.0f) * invMVP;
        (*va)[7] = osg::Vec3(-1.0f, 1.0f, 1.0f) * invMVP;

        osg::ref_ptr<osg::DrawElementsUByte> de = new osg::DrawElementsUByte(GL_QUADS);
        de->push_back(0); de->push_back(1); de->push_back(2); de->push_back(3);
        de->push_back(4); de->push_back(5); de->push_back(6); de->push_back(7);
        de->push_back(0); de->push_back(4); de->push_back(5); de->push_back(1);
        de->push_back(1); de->push_back(5); de->push_back(6); de->push_back(2);
        de->push_back(2); de->push_back(6); de->push_back(7); de->push_back(3);
        de->push_back(3); de->push_back(7); de->push_back(4); de->push_back(0);
        return createGeometry(va.get(), NULL, NULL, de.get(), true);
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

    osg::Geometry* createBoundingBoxGeometry(const osg::BoundingBox& bb)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(8);
        for (int i = 0; i < 8; ++i) (*va)[i] = bb.corner(i);

        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(8);
        (*ta)[0] = osg::Vec2(0.0f, 0.0f); (*ta)[1] = osg::Vec2(0.33f, 0.0f);
        (*ta)[3] = osg::Vec2(0.67f, 0.0f); (*ta)[2] = osg::Vec2(1.0f, 0.0f);
        (*ta)[4] = osg::Vec2(0.0f, 1.0f); (*ta)[5] = osg::Vec2(0.33f, 1.0f);
        (*ta)[7] = osg::Vec2(0.67f, 1.0f); (*ta)[6] = osg::Vec2(1.0f, 1.0f);

        osg::ref_ptr<osg::DrawElementsUByte> de = new osg::DrawElementsUByte(GL_QUADS);
        de->push_back(0); de->push_back(1); de->push_back(3); de->push_back(2);
        de->push_back(4); de->push_back(5); de->push_back(7); de->push_back(6);
        de->push_back(0); de->push_back(1); de->push_back(5); de->push_back(4);
        de->push_back(1); de->push_back(3); de->push_back(7); de->push_back(5);
        de->push_back(3); de->push_back(2); de->push_back(6); de->push_back(7);
        de->push_back(2); de->push_back(0); de->push_back(4); de->push_back(6);
        return createGeometry(va.get(), NULL, ta.get(), de.get());
    }

    osg::Geometry* createBoundingSphereGeometry(const osg::BoundingSphere& bs)
    { return createEllipsoid(bs.center(), bs.radius(), bs.radius(), bs.radius()); }

}

