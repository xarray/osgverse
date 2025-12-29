// Prevent GLES2/gl2.h to redefine gl* functions
#define GL_GLES_PROTOTYPES 0
#include <GL/glew.h>
#include <gl_radix_sort/RadixSort.hpp>
#include <parallel_radix_sort.h>

#include <algorithm>
#include <iostream>
#include <osg/io_utils>
#include <osg/BufferIndexBinding>
#include <osg/BufferObject>
#include <osgUtil/CullVisitor>
#include "Math.h"
#include "GaussianGeometry.h"
using namespace osgVerse;

namespace
{
    static osg::Matrix transpose(const osg::Matrix& m)
    {
        return osg::Matrix(m(0, 0), m(1, 0), m(2, 0), m(3, 0), m(0, 1), m(1, 1), m(2, 1), m(3, 1),
            m(0, 2), m(1, 2), m(2, 2), m(3, 2), m(0, 3), m(1, 3), m(2, 3), m(3, 3));
    }

    static std::pair<int, int> calculateTextureDim(int count)
    {
        int w = 1; while (w * w < count) w <<= 1;
        int h = 1; while (w * h < count) h <<= 1;
        return std::pair<int, int>(w, h);
    }

    class GaussianUniformCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
            if (cv && cv->getCurrentCamera())
            {
                osg::StateSet* ss = node->getOrCreateStateSet();
                osg::Uniform* invScreen = ss->getUniform("InvScreenResolution"), * nearFar = ss->getUniform("NearFarPlanes");
                if (!invScreen)
                {
                    invScreen = ss->getOrCreateUniform("InvScreenResolution", osg::Uniform::FLOAT_VEC2);
                    invScreen->setDataVariance(osg::Object::DYNAMIC);
                }
                if (!nearFar)
                {
                    nearFar = ss->getOrCreateUniform("NearFarPlanes", osg::Uniform::FLOAT_VEC2);
                    nearFar->setDataVariance(osg::Object::DYNAMIC);
                }

                const osg::Viewport* vp = cv->getCurrentCamera()->getViewport();
                if (vp) invScreen->set(osg::Vec2(1.0f / vp->width(), 1.0f / vp->height()));

                double fov = 0.0, aspect = 0.0, znear = 0.0, zfar = 0.0;
                cv->getCurrentCamera()->getProjectionMatrixAsPerspective(fov, aspect, znear, zfar);
                nearFar->set(osg::Vec2(znear, zfar));
            }
            traverse(node, nv);
        }
    };

    struct TextureLookUpTable
    {
        static osg::Texture2DArray* create(int w, int h, int layers, bool useVec3, bool useHalf)
        {
            osg::ref_ptr<osg::Texture2DArray> tex = new osg::Texture2DArray;
            tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            tex->setWrap(osg::Texture::WRAP_S, osg::Texture::MIRROR);
            tex->setWrap(osg::Texture::WRAP_T, osg::Texture::MIRROR);

            GLenum pf = (useVec3 ? GL_RGB : GL_RGBA);
            GLenum tf = (useVec3 ? (useHalf ? GL_RGB16F_ARB : GL_RGB32F_ARB)
                                 : (useHalf ? GL_RGBA16F_ARB : GL_RGBA32F_ARB));
            for (int i = 0; i < layers; ++i)
            {
                osg::ref_ptr<osg::Image> image = new osg::Image;
                image->allocateImage(w, h, 1, pf, useHalf ? GL_HALF_FLOAT : GL_FLOAT);
                image->setInternalTextureFormat(tf); tex->setImage(i, image.get());
            }
            return tex.release();
        }

        static osg::Image* getLayer(osg::Texture2DArray* tex, int layer, bool asVec3, bool asHalf)
        {
            if (tex && layer < (int)tex->getNumImages())
            {
                osg::Image* image = tex->getImage(layer);
                bool useVec3 = (image->getPixelFormat() == GL_RGB);
                bool useHalf = (image->getDataType() == GL_HALF_FLOAT);
                if (asVec3 == useVec3 && asHalf == useHalf) return image;
            }
            OSG_WARN << "[GaussianGeometry] Failed to get lookup table: " << layer << "\n";
            return NULL;
        }

        static void setLayerData(osg::Image* image, void* src, int elemSize, int num)
        {
            if (!image) return; else if (!image->data()) return; else image->dirty();
            if (num < (image->s() * image->t())) memcpy(image->data(), src, elemSize * num);
            else { OSG_WARN << "[GaussianGeometry] Failed to set lookup table\n"; }
        }

        static osg::Vec3* getFloat3(osg::Texture2DArray* t, int d)
        { osg::Image* im = getLayer(t, d, true, false); return im ? (osg::Vec3*)im->data() : NULL; }
        static osg::Vec4* getFloat4(osg::Texture2DArray* t, int d)
        { osg::Image* im = getLayer(t, d, false, false); return im ? (osg::Vec4*)im->data() : NULL; }
        static osg::Vec3us* getHalf3(osg::Texture2DArray* t, int d)
        { osg::Image* im = getLayer(t, d, true, true); return im ? (osg::Vec3us*)im->data() : NULL; }
        static osg::Vec4us* getHalf4(osg::Texture2DArray* t, int d)
        { osg::Image* im = getLayer(t, d, false, true); return im ? (osg::Vec4us*)im->data() : NULL; }

        static void setFloat3(osg::Texture2DArray* t, int d, std::vector<osg::Vec3>* ptr)
        { if (!ptr->empty()) setLayerData(getLayer(t, d, true, false), &(*ptr)[0], sizeof(osg::Vec3), ptr->size()); }
        static void setFloat4(osg::Texture2DArray* t, int d, std::vector<osg::Vec4>* ptr)
        { if (!ptr->empty()) setLayerData(getLayer(t, d, false, false), &(*ptr)[0], sizeof(osg::Vec4), ptr->size()); }
        static void setHalf3(osg::Texture2DArray* t, int d, std::vector<osg::Vec3us>* ptr)
        { if (!ptr->empty()) setLayerData(getLayer(t, d, true, true), &(*ptr)[0], sizeof(osg::Vec3us), ptr->size()); }
        static void setHalf4(osg::Texture2DArray* t, int d, std::vector<osg::Vec4us>* ptr)
        { if (!ptr->empty()) setLayerData(getLayer(t, d, false, true), &(*ptr)[0], sizeof(osg::Vec4us), ptr->size()); }
    };
}

GaussianGeometry::GaussianGeometry(RenderMethod m)
:   osg::Geometry(), _method(m), _degrees(0), _numSplats(0)
{
    setUseDisplayList(false); setUseVertexBufferObjects(true);
    if (_method == INSTANCING)
    {
        _coreBuffer = new osg::FloatArray;
        _shcoefBuffer = new osg::FloatArray;
    }
}

GaussianGeometry::GaussianGeometry(const GaussianGeometry& copy, const osg::CopyOp& copyop)
:   osg::Geometry(copy, copyop), _preDataMap(copy._preDataMap), _preDataMap2(copy._preDataMap2),
    _coreBuffer(copy._coreBuffer), _shcoefBuffer(copy._shcoefBuffer), _core(copy._core), _shcoef(copy._shcoef),
    _method(copy._method), _degrees(copy._degrees), _numSplats(copy._numSplats) {}

osg::Program* GaussianGeometry::createProgram(osg::Shader* vs, osg::Shader* gs, osg::Shader* fs, RenderMethod m)
{
    osg::Program* program = new osg::Program;
    program->addShader(vs); program->addShader(fs);
    if (m == GEOMETRY_SHADER)
    {
        program->addShader(gs);
        program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 4);
        program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
        program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
        program->addBindAttribLocation("osg_Covariance0", 1);
        program->addBindAttribLocation("osg_Covariance1", 2);
        program->addBindAttribLocation("osg_Covariance2", 3);
        program->addBindAttribLocation("osg_R_SH0", 4);
        program->addBindAttribLocation("osg_G_SH0", 5);
        program->addBindAttribLocation("osg_B_SH0", 6);
        program->addBindAttribLocation("osg_R_SH1", 7);
        program->addBindAttribLocation("osg_G_SH1", 8);
        program->addBindAttribLocation("osg_B_SH1", 9);
        program->addBindAttribLocation("osg_R_SH2", 10);
        program->addBindAttribLocation("osg_G_SH2", 11);
        program->addBindAttribLocation("osg_B_SH2", 12);
        program->addBindAttribLocation("osg_R_SH3", 13);
        program->addBindAttribLocation("osg_G_SH3", 14);
        program->addBindAttribLocation("osg_B_SH3", 15);
    }
    else
    {
        program->addBindAttribLocation("osg_UserIndex", 1);
    }
    return program;
}

void GaussianGeometry::checkShaderFlag()
{
#if OSG_VERSION_GREATER_THAN(3, 3, 6)
    if (_degrees > 0) getOrCreateStateSet()->setDefine("FULL_SH");
    else getOrCreateStateSet()->removeDefine("FULL_SH");
#endif
    if (_method != GEOMETRY_SHADER)
    {
        getOrCreateStateSet()->setDefine("USE_INSTANCING");
        if (_method == INSTANCING_TEXTURE) getOrCreateStateSet()->setDefine("USE_INSTANCING_TEXARRAY");
    }
    else
    {
        getOrCreateStateSet()->removeDefine("USE_INSTANCING");
        getOrCreateStateSet()->removeDefine("USE_INSTANCING_TEXARRAY");
    }
}

#define GET_POS4(v) osg::Vec4* v = const_cast<GaussianGeometry*>(this)->getPosition4();
osg::BoundingBox GaussianGeometry::getBounding(osg::Vec4* va) const
{
    osg::BoundingBox bbox;
    for (size_t i = 0; i < _numSplats; ++i)
    { const osg::Vec4& v = va[i]; bbox.expandBy(osg::Vec3(v[0], v[1], v[2])); }
    return bbox;
}

#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
osg::BoundingSphere GaussianGeometry::computeBound() const
{
    if (_method != GEOMETRY_SHADER) { GET_POS4(v); return getBounding(v); }
    return osg::Geometry::computeBound();
}

osg::BoundingBox GaussianGeometry::computeBoundingBox() const
{
    if (_method != GEOMETRY_SHADER) { GET_POS4(v); return getBounding(v); }
    return osg::Geometry::computeBoundingBox();
}
#else
osg::BoundingBox GaussianGeometry::computeBound() const
{
    if (_method != GEOMETRY_SHADER) { GET_POS4(v); return getBounding(v); }
    return osg::Geometry::computeBound();
}
#endif

osg::NodeCallback* GaussianGeometry::createUniformCallback()
{ return new GaussianUniformCallback; }

bool GaussianGeometry::finalize()
{
    osg::StateSet* ss = getOrCreateStateSet();
    if (_method != GEOMETRY_SHADER)
    {
        std::pair<int, int> res = calculateTextureDim(_numSplats);
        ss->addUniform(new osg::Uniform("TextureSize", osg::Vec2(res.first, res.second)));

        // Apply core attributes
        size_t blockSize = _numSplats * sizeof(osg::Vec4);
        if (_coreBuffer.valid())
        {
            osg::ShaderStorageBufferObject* ssbo = new osg::ShaderStorageBufferObject; ssbo->setUsage(GL_STATIC_DRAW);
            _coreBuffer->resize(_numSplats * 16, 0.0f); _coreBuffer->setBufferObject(ssbo);
            ss->setAttributeAndModes(new osg::ShaderStorageBufferBinding(0, _coreBuffer.get(), 0, blockSize));
            ss->setAttributeAndModes(new osg::ShaderStorageBufferBinding(1, _coreBuffer.get(), blockSize, blockSize * 2));
            ss->setAttributeAndModes(new osg::ShaderStorageBufferBinding(2, _coreBuffer.get(), blockSize * 2, blockSize * 3));
            ss->setAttributeAndModes(new osg::ShaderStorageBufferBinding(3, _coreBuffer.get(), blockSize * 3, blockSize * 4));
        }
        else if (!_core)
        {
            _core = TextureLookUpTable::create(res.first, res.second, 4, false, false);
            ss->setTextureAttributeAndModes(0, _core.get());
            ss->addUniform(new osg::Uniform("CoreParameters", (int)0));
        }

        size_t total = 0; char* ptr = _coreBuffer.valid() ? (char*)_coreBuffer->getDataPointer() : NULL;
        for (int i = 0; i < 4; ++i)
        {
            std::vector<osg::Vec4>& src = _preDataMap["Layer" + std::to_string(i)];
            if (_coreBuffer.valid())
                { memcpy(ptr + total, src.data(), src.size() * sizeof(osg::Vec4)); total += blockSize; }
            else
                TextureLookUpTable::setFloat4(_core.get(), i, &src);
        }
        _preDataMap.clear();  // clear host prepared data

        // Apply shcoef attributes
        size_t shDataSize = _preDataMap2.size();
        if (_degrees > 0 && shDataSize > 10)
        {
            size_t blockSize = _numSplats * sizeof(osg::Vec4) * 15;  // rgb4 * 15
            if (_shcoefBuffer.valid())
            {
                osg::ShaderStorageBufferObject* ssbo = new osg::ShaderStorageBufferObject; ssbo->setUsage(GL_STATIC_DRAW);
                _shcoefBuffer->resize(_numSplats * 60, 0.0f); _shcoefBuffer->setBufferObject(ssbo);
                ss->setAttributeAndModes(new osg::ShaderStorageBufferBinding(4, _shcoefBuffer.get(), 0, blockSize));
            }
            else if (!_shcoef)
            {
                _shcoef = TextureLookUpTable::create(res.first, res.second, 15, true, false);
                ss->setTextureAttributeAndModes(1, _shcoef.get());
                ss->addUniform(new osg::Uniform("ShParameters", (int)1));
            }

            ptr = _shcoefBuffer.valid() ? (char*)_shcoefBuffer->getDataPointer() : NULL;
            for (size_t i = 0; i < shDataSize; ++i)
            {
                std::vector<osg::Vec3>& src = _preDataMap2["Layer" + std::to_string(i + 1)];
                if (_shcoefBuffer.valid())
                {
                    for (size_t j = 0; j < src.size(); ++j)
                        *((osg::Vec4*)ptr + (j * 15 + i)) = osg::Vec4(src[j], 0.0f);
                }
                else
                    TextureLookUpTable::setFloat3(_core.get(), i, &src);
            }
        }
        _preDataMap2.clear();  // clear host prepared data

        // Create default quad and instanced primitive set
        osg::ref_ptr<osg::Vec3Array> quad = new osg::Vec3Array(4);
        (*quad)[0].set(1.0f, 1.0f, 0.0f); (*quad)[1].set(-1.0f, 1.0f, 0.0f);
        (*quad)[2].set(1.0f, -1.0f, 0.0f); (*quad)[3].set(-1.0f, -1.0f, 0.0f);
        setVertexArray(quad.get());

        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
        de->push_back(0); de->push_back(1); de->push_back(2);
        de->push_back(1); de->push_back(3); de->push_back(2);
        de->setNumInstances(_numSplats);

        // Add an index array for sorting
        osg::ref_ptr<osg::UIntArray> indices = new osg::UIntArray(_numSplats);
        for (int i = 0; i < _numSplats; ++i) (*indices)[i] = i;
        setVertexAttribArray(1, indices); setVertexAttribNormalize(1, GL_FALSE);
        setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
#if OSG_VERSION_GREATER_THAN(3, 3, 3)
        getOrCreateStateSet()->setAttributeAndModes(new osg::VertexAttribDivisor(1, 1));
#endif
        return addPrimitiveSet(de.get());
    }
    else
    {
        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_POINTS);
        for (size_t i = 0; i < _numSplats; ++i) de->push_back(i);
        return addPrimitiveSet(de.get());
    }
}

void GaussianGeometry::setPosition(osg::Vec3Array* v)
{
    if (v) _numSplats = v->size(); else return;
    if (_method != GEOMETRY_SHADER)
    {
        std::vector<osg::Vec4>& dst = _preDataMap["Layer0"]; dst.resize(v->size());
        for (size_t i = 0; i < v->size(); ++i)
            dst[i] = osg::Vec4((*v)[i].x(), (*v)[i].y(), (*v)[i].z(), dst[i].a());
#if OSG_VERSION_GREATER_THAN(3, 3, 3)
        getOrCreateStateSet()->setAttributeAndModes(new osg::VertexAttribDivisor(1, 1));
#endif
    }
    else
        setVertexArray(v);
}

void GaussianGeometry::setScaleAndRotation(osg::Vec3Array* vArray, osg::Vec4Array* qArray,
                                           osg::FloatArray* alphas)
{
    if (!vArray || !qArray) return; if (vArray->size() != qArray->size()) return;
    osg::Vec4Array* cov0 = new osg::Vec4Array(vArray->size());
    osg::Vec4Array* cov1 = new osg::Vec4Array(vArray->size());
    osg::Vec4Array* cov2 = new osg::Vec4Array(vArray->size());

    for (size_t i = 0; i < vArray->size(); ++i)
    {
        float a = (alphas == NULL) ? 1.0f : alphas->at(i);
        const osg::Vec3& scale = (*vArray)[i]; osg::Quat quat((*qArray)[i]);
        if (!quat.zeroRotation()) { double l = quat.length(); if (l > 0.0) quat = quat / l; }
        
        osg::Matrix R(quat), S = osg::Matrix::scale(scale);
        osg::Matrix cov = transpose(R) * S * S * R;
        (*cov0)[i] = osg::Vec4(cov(0, 0), cov(1, 0), cov(2, 0), a);
        (*cov1)[i] = osg::Vec4(cov(0, 1), cov(1, 1), cov(2, 1), 1.0f);
        (*cov2)[i] = osg::Vec4(cov(0, 2), cov(1, 2), cov(2, 2), 1.0f);
    }

    if (_method != GEOMETRY_SHADER)
    {
        std::vector<osg::Vec4>& dst0 = _preDataMap["Layer0"]; dst0.resize(vArray->size());
        std::vector<osg::Vec4>& dst1 = _preDataMap["Layer1"]; dst1.resize(vArray->size());
        std::vector<osg::Vec4>& dst2 = _preDataMap["Layer2"]; dst2.resize(vArray->size());
        std::vector<osg::Vec4>& dst3 = _preDataMap["Layer3"]; dst3.resize(vArray->size());
        for (size_t i = 0; i < vArray->size(); ++i)
        {
            dst0[i].a() = (*cov0)[i].a();
            dst1[i] = osg::Vec4((*cov0)[i].x(), (*cov0)[i].y(), (*cov0)[i].z(), dst1[i].a());
            dst2[i] = osg::Vec4((*cov1)[i].x(), (*cov1)[i].y(), (*cov1)[i].z(), dst2[i].a());
            dst3[i] = osg::Vec4((*cov2)[i].x(), (*cov2)[i].y(), (*cov2)[i].z(), dst3[i].a());
        }
    }
    else
    {
        setVertexAttribArray(1, cov0); setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
        setVertexAttribArray(2, cov1); setVertexAttribBinding(2, osg::Geometry::BIND_PER_VERTEX);
        setVertexAttribArray(3, cov2); setVertexAttribBinding(3, osg::Geometry::BIND_PER_VERTEX);
    }
}

#define SET_SHCOEF_DATA(col, pos, i, v) \
    std::vector<osg::Vec3>& dst1 = _preDataMap2["Layer" + std::to_string(i * 4 + 1)]; dst1.resize(v->size()); \
    std::vector<osg::Vec3>& dst2 = _preDataMap2["Layer" + std::to_string(i * 4 + 2)]; dst2.resize(v->size()); \
    std::vector<osg::Vec3>& dst3 = _preDataMap2["Layer" + std::to_string(i * 4 + 3)]; dst3.resize(v->size()); \
    if (i == 0) { \
        std::vector<osg::Vec4>& dstR = _preDataMap["Layer" #col]; dstR.resize(v->size()); \
        for (size_t i = 0; i < v->size(); ++i) { \
            dstR[i][3] = (*v)[i].x(); dst1[i][pos] = (*v)[i].y(); dst2[i][pos] = (*v)[i].z(); dst3[i][pos] = (*v)[i].w(); \
        } \
    } else { \
        std::vector<osg::Vec3>& dst0 = _preDataMap2["Layer" + std::to_string(i * 4 + 0)]; dst0.resize(v->size()); \
        for (size_t i = 0; i < v->size(); ++i) { \
            dst0[i][pos] = (*v)[i].x(); dst1[i][pos] = (*v)[i].y(); dst2[i][pos] = (*v)[i].z(); dst3[i][pos] = (*v)[i].w(); \
        } }

void GaussianGeometry::setShRed(int i, osg::Vec4Array* v)
{
    if (_method != GEOMETRY_SHADER)
        { SET_SHCOEF_DATA(1, 0, i, v); }
    else
        { setVertexAttribArray(4 + i * 3, v); setVertexAttribBinding(4 + i * 3, osg::Geometry::BIND_PER_VERTEX); }
}

void GaussianGeometry::setShGreen(int i, osg::Vec4Array* v)
{
    if (_method != GEOMETRY_SHADER)
        { SET_SHCOEF_DATA(2, 1, i, v); }
    else
        { setVertexAttribArray(5 + i * 3, v); setVertexAttribBinding(5 + i * 3, osg::Geometry::BIND_PER_VERTEX); }
}

void GaussianGeometry::setShBlue(int i, osg::Vec4Array* v)
{
    if (_method != GEOMETRY_SHADER)
        { SET_SHCOEF_DATA(3, 2, i, v); }
    else
        { setVertexAttribArray(6 + i * 3, v); setVertexAttribBinding(6 + i * 3, osg::Geometry::BIND_PER_VERTEX); }
}

osg::Vec3* GaussianGeometry::getPosition3()
{
    if (_method != GEOMETRY_SHADER) return NULL;  // not supported
    else return (osg::Vec3*)getVertexArray()->getDataPointer();
}

osg::Vec4* GaussianGeometry::getPosition4()
{
    if (_method != GEOMETRY_SHADER)
    {
        if (!_preDataMap.empty())
        {
            std::vector<osg::Vec4>& dst = _preDataMap["Layer0"];
            if (!dst.empty()) return dst.data();
        }
        else if (_coreBuffer.valid())
            return (osg::Vec4*)_coreBuffer->getDataPointer();
        else if (_core.valid())
            return TextureLookUpTable::getFloat4(_core.get(), 0);
    }
    return NULL;
}

osg::ref_ptr<osg::Vec3Array> GaussianGeometry::getCovariance0()
{
    if (_method != GEOMETRY_SHADER)
    {
        return NULL;  // TODO: not implemented
    }
    else
        return static_cast<osg::Vec3Array*>(getVertexAttribArray(1));
}

osg::ref_ptr<osg::Vec3Array> GaussianGeometry::getCovariance1()
{
    if (_method != GEOMETRY_SHADER)
    {
        return NULL;  // TODO: not implemented
    }
    else
        return static_cast<osg::Vec3Array*>(getVertexAttribArray(2));
}

osg::ref_ptr<osg::Vec3Array> GaussianGeometry::getCovariance2()
{
    if (_method != GEOMETRY_SHADER)
    {
        return NULL;  // TODO: not implemented
    }
    else
        return static_cast<osg::Vec3Array*>(getVertexAttribArray(3));
}

#define GET_SHCOEF_DATA(ra, col, pos, i) \
    const std::vector<osg::Vec3>& src1 = _preDataMap2["Layer" + std::to_string(i * 4 + 1)]; \
    const std::vector<osg::Vec3>& src2 = _preDataMap2["Layer" + std::to_string(i * 4 + 2)]; \
    const std::vector<osg::Vec3>& src3 = _preDataMap2["Layer" + std::to_string(i * 4 + 3)]; \
    if (i == 0) { \
        const std::vector<osg::Vec4>& srcR = _preDataMap["Layer" #col]; \
        ra = new osg::Vec4Array(srcR.size()); \
        for (size_t i = 0; i < srcR.size(); ++i) \
            (*ra)[i] = osg::Vec4(srcR[i][3], src1[i][pos], src2[i][pos], src3[i][pos]); \
    } else { \
        const std::vector<osg::Vec3>& src0 = _preDataMap2["Layer" + std::to_string(i * 4 + 0)]; \
        ra = new osg::Vec4Array(src0.size()); \
        for (size_t i = 0; i < src0.size(); ++i) \
            (*ra)[i] = osg::Vec4(src0[i][col], src1[i][pos], src2[i][pos], src3[i][pos]); \
    }

osg::ref_ptr<osg::Vec4Array> GaussianGeometry::getShRed(int index)
{
    if (_method != GEOMETRY_SHADER)
    {
        osg::ref_ptr<osg::Vec4Array> ra;  // FIXME: will fail after if finalize() done
        GET_SHCOEF_DATA(ra, 1, 0, index); return ra;
    }
    else
        return static_cast<osg::Vec4Array*>(getVertexAttribArray(4 + index * 3));
}

osg::ref_ptr<osg::Vec4Array> GaussianGeometry::getShGreen(int index)
{
    if (_method != GEOMETRY_SHADER)
    {
        osg::ref_ptr<osg::Vec4Array> ra;  // FIXME: will fail after if finalize() done
        GET_SHCOEF_DATA(ra, 2, 1, index); return ra;
    }
    else
        return static_cast<osg::Vec4Array*>(getVertexAttribArray(5 + index * 3));
}

osg::ref_ptr<osg::Vec4Array> GaussianGeometry::getShBlue(int index)
{
    if (_method != GEOMETRY_SHADER)
    {
        osg::ref_ptr<osg::Vec4Array> ra;  // FIXME: will fail after if finalize() done
        GET_SHCOEF_DATA(ra, 3, 2, index); return ra;
    }
    else
        return static_cast<osg::Vec4Array*>(getVertexAttribArray(6 + index * 3));
}

///////////////////////// GaussianSorter /////////////////////////
class GaussianSortThread : public OpenThreads::Thread
{
public:
    virtual int cancel()
    { _running = false; return OpenThreads::Thread::cancel(); }

    virtual void run()
    {
        _running = true;
        while (_running)
        {
            std::map<osg::VectorGLuint*, Task> tempTasks;
            _taskLock.lock();
            tempTasks.swap(_sortTasks);
            _taskLock.unlock();

            for (std::map<osg::VectorGLuint*, Task>::iterator it = tempTasks.begin();
                 it != tempTasks.end(); ++it)
            {
                Task& task = it->second; size_t numCulled = 0;
                if (task.indices.empty()) continue;

                std::vector<GLuint> keys(task.indices.size());
                if (task.positions3)
                {
                    for (size_t i = 0; i < task.indices.size(); ++i)
                    {   // comparing floating-point numbers as integers
                        float d = (task.positions3[task.indices[i]] * task.localToEye).z();
                        union { float f; uint32_t u; } un = { (d > 0.0f ? 0.0f : (-d)) };
                        keys[i] = (GLuint)un.u; if (d > 0.0f) numCulled++;
                    }
                }
                else if (task.positions4)
                {
                    for (size_t i = 0; i < task.indices.size(); ++i)
                    {   // comparing floating-point numbers as integers
                        const osg::Vec4& v = task.positions4[task.indices[i]];
                        float d = (osg::Vec3(v[0], v[1], v[2]) * task.localToEye).z();
                        union { float f; uint32_t u; } un = { (d > 0.0f ? 0.0f : (-d)) };
                        keys[i] = (GLuint)un.u; if (d > 0.0f) numCulled++;
                    }
                }

                OpenThreads::Thread::YieldCurrentThread();
                parallel_radix_sort::SortPairs(&keys[0], &(task.indices)[0], keys.size());

                _resultLock.lock();
                ResultIndices& ri = _sortResults[it->first]; ri.second = numCulled;
                ri.first.assign(task.indices.rbegin(), task.indices.rend());
                _resultLock.unlock();

                _taskLock.lock();
                _sortTaskFlags.erase(it->first);
                _taskLock.unlock();
            }
            OpenThreads::Thread::YieldCurrentThread();
        }
    }

    size_t addTask(osg::Vec3* va, osg::Vec4* va2, osg::VectorGLuint* de, const osg::Matrix& matrix)
    {
        size_t num = 0; _taskLock.lock();
        if (_sortTaskFlags.find(de) == _sortTaskFlags.end())
        {
            _sortTasks[de] = Task{ va, va2, std::vector<GLuint>(de->begin(), de->end()), matrix };
            _sortTaskFlags.insert(de);
        }
        num = _sortTasks.size();
        _taskLock.unlock(); return num;
    }

    bool applyResult(osg::VectorGLuint* de, size_t& numCulled)
    {
        bool taskDone = false; _resultLock.lock();
        std::map<osg::VectorGLuint*, ResultIndices>::iterator it = _sortResults.find(de);
        if (it != _sortResults.end()) { numCulled = it->second.second; de->swap(it->second.first);
                                        _sortResults.erase(it); taskDone = true; }
        _resultLock.unlock(); return taskDone;
    }

protected:
    struct Task
    {
        osg::Vec3* positions3;
        osg::Vec4* positions4;
        std::vector<GLuint> indices;
        osg::Matrix localToEye;
    };

    typedef std::pair<std::vector<GLuint>, size_t> ResultIndices;
    std::map<osg::VectorGLuint*, ResultIndices> _sortResults;
    std::set<osg::VectorGLuint*> _sortTaskFlags;
    std::map<osg::VectorGLuint*, Task> _sortTasks;
    OpenThreads::Mutex _taskLock, _resultLock;
    bool _running;
};

void GaussianSorter::configureThreads(int numThreads)
{
    size_t currentNum = _sortThreads.size();
    if (numThreads < currentNum)
    {
        for (size_t i = numThreads; i < currentNum; ++i)
        {
            OpenThreads::Thread* thread = _sortThreads[i]; thread->cancel();
            while (thread->isRunning()) OpenThreads::Thread::YieldCurrentThread();
            thread->join(); delete thread; _sortThreads[i] = NULL;
        }
    }

    if (numThreads > 0) _sortThreads.resize(numThreads); else _sortThreads.clear();
    for (size_t i = currentNum; i < numThreads; ++i)
    {
        GaussianSortThread* thread = new GaussianSortThread;
        thread->start(); _sortThreads[i] = thread;
    }
}

void GaussianSorter::addGeometry(GaussianGeometry* geom)
{ _geometries.insert(geom); }

void GaussianSorter::removeGeometry(GaussianGeometry* geom)
{
    std::set<osg::ref_ptr<GaussianGeometry>>::iterator it = _geometries.find(geom);
    if (it != _geometries.end())
    {
        std::map<GaussianGeometry*, osg::Matrix>::iterator it2 = _geometryMatrices.find(geom);
        if (it2 != _geometryMatrices.end()) _geometryMatrices.erase(it2);
        _geometries.erase(it);
    }
}

void GaussianSorter::cull(const osg::Matrix& view)
{
    std::vector<osg::ref_ptr<GaussianGeometry>> _geometriesToSort;
    for (std::set<osg::ref_ptr<GaussianGeometry>>::iterator it = _geometries.begin();
         it != _geometries.end();)
    {
        GaussianGeometry* gs = (*it).get();  // remove those only ref-ed by the sorter
        if (!gs || (gs && gs->referenceCount() < 2)) { it = _geometries.erase(it); continue; }

        osg::MatrixList matrices = gs->getWorldMatrices();
        if (matrices.empty()) { it = _geometries.erase(it); continue; }
        cull(gs, matrices[0], view); ++it;
    }
}

void GaussianSorter::cull(GaussianGeometry* geom, const osg::Matrix& model, const osg::Matrix& view)
{
    osg::VectorGLuint* indices = NULL; osg::BufferData* indexBuffer = NULL;
    osg::Vec3* pos = geom->getPosition3(); osg::Vec4* pos2 = geom->getPosition4();
    int numSplats = geom->getNumSplats(); if ((!pos && !pos2) || !numSplats) return;
    
    if (geom->getRenderMethod() != GaussianGeometry::GEOMETRY_SHADER)
    {
        osg::UIntArray* vaa = static_cast<osg::UIntArray*>(geom->getVertexAttribArray(1));
        if (!vaa)
        {
            vaa = new osg::UIntArray(numSplats);
            for (int i = 0; i < numSplats; ++i) (*vaa)[i] = i;
            geom->setVertexAttribArray(1, vaa); geom->setVertexAttribNormalize(1, GL_FALSE);
            geom->setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
#if OSG_VERSION_GREATER_THAN(3, 3, 3)
            geom->getOrCreateStateSet()->setAttributeAndModes(new osg::VertexAttribDivisor(1, 1));
#endif
        }
        indices = vaa; indexBuffer = vaa;
    }
    else
    {
        osg::DrawElementsUInt* de = (geom->getNumPrimitiveSets() > 0)
                                  ? static_cast<osg::DrawElementsUInt*>(geom->getPrimitiveSet(0)) : NULL;
        if (!de)
        {
            de = new osg::DrawElementsUInt(GL_POINTS); de->resize(numSplats); geom->addPrimitiveSet(de);
            for (int i = 0; i < numSplats; ++i) (*de)[i] = i;
        }
        indices = de; indexBuffer = de;
    }

    osg::Matrix localToEye = model * view; size_t numCulled = 0; bool toSort = true;
    if (_onDemand)
    {
        osg::Matrix& matrix = _geometryMatrices[geom];
        if (isEqual(matrix, localToEye)) toSort = false; else matrix = localToEye;
    }

    switch (_method)
    {
    case CPU_SORT:
        if (!_sortThreads.empty())
        {
            // FIXME: use different threads to share the burden
            GaussianSortThread* thread = static_cast<GaussianSortThread*>(_sortThreads[0]);
            if (toSort) thread->addTask(pos, pos2, indices, localToEye);
            if (thread->applyResult(indices, numCulled))
            {
                if (geom->getRenderMethod() != GaussianGeometry::GEOMETRY_SHADER)
                {
                    osg::DrawElementsUShort* de = (geom->getNumPrimitiveSets() > 0)
                                                ? static_cast<osg::DrawElementsUShort*>(geom->getPrimitiveSet(0)) : NULL;
                    de->setNumInstances(numSplats - numCulled); de->dirty();
                }
                indexBuffer->dirty();
            }
        }
        break;
    case GL46_RADIX_SORT:
        if (toSort && pos && !indices->empty())
        {
            std::vector<GLuint> values(indices->begin(), indices->end());
            std::vector<GLuint> keys(values.size());
            for (size_t i = 0; i < indices->size(); ++i)
            {
                float d = (pos[(*indices)[i]] * localToEye).z();
                union { float f; uint32_t u; } un = { (d > 0.0f ? 0.0f : (-d)) };
                keys[i] = (GLuint)un.u;  // comparing floating-point numbers as integers
            }

            if (_firstFrame) { glewInit(); _firstFrame = false; }
            glu::ShaderStorageBuffer val_buffer(values);
            glu::ShaderStorageBuffer key_buffer(keys);
            glu::RadixSort radix_sort;
            radix_sort(key_buffer.handle(), val_buffer.handle(), keys.size());
            //val_buffer.write_data(indices->getDataPointer(), indices->size() * sizeof(GLuint));

            std::vector<GLuint> sorted_vals = val_buffer.get_data<GLuint>();
            indices->assign(sorted_vals.rbegin(), sorted_vals.rend()); indexBuffer->dirty();
        }
        break;
    default:
        if (toSort && _sortCallback.valid())
        {
            if (_sortCallback->sort(indices, pos, numSplats, model, view)) indexBuffer->dirty();
            else if (_sortCallback->sort(indices, pos2, numSplats, model, view)) indexBuffer->dirty();
        } break;
    }
}
