#include <osg/FrameBufferObject>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/ContextData>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/Geode>
#include <iostream>
#include <mikktspace.h>
#include <normalmap/normalmapgenerator.h>
#include <normalmap/specularmapgenerator.h>
#include "Utilities.h"

/// MikkTSpace visitor utilities
struct MikkTSpaceHelper
{
    std::vector<osg::Vec3ui> _faceList;
    osg::Vec3Array *tangents, *binormals;
    osg::Geometry* _geometry;

    bool initialize(SMikkTSpaceContext* sc, osg::Geometry* g)
    {
        sc->m_pInterface->m_getNumFaces = MikkTSpaceHelper::mikk_getNumFaces;
        sc->m_pInterface->m_getNumVerticesOfFace = MikkTSpaceHelper::mikk_getNumVerticesOfFace;
        sc->m_pInterface->m_getPosition = MikkTSpaceHelper::mikk_getPosition;
        sc->m_pInterface->m_getNormal = MikkTSpaceHelper::mikk_getNormal;
        sc->m_pInterface->m_getTexCoord = MikkTSpaceHelper::mikk_getTexCoord;
        sc->m_pInterface->m_setTSpaceBasic = MikkTSpaceHelper::mikk_setTSpaceBasic;
        sc->m_pInterface->m_setTSpace = NULL; sc->m_pUserData = this; _geometry = g;

        osg::Vec3Array* va = vArray(); osg::Vec3Array* na = nArray();
        osg::Vec2Array* ta = tArray();
        if (!va || !na || !ta) return false;
        if (va->size() != na->size() || va->size() != ta->size()) return false;

        tangents = new osg::Vec3Array(va->size()); binormals = new osg::Vec3Array(va->size());
        g->setVertexAttribArray(6, tangents); g->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
        g->setVertexAttribArray(7, binormals); g->setVertexAttribBinding(7, osg::Geometry::BIND_PER_VERTEX);
        return true;
    }

    osg::Vec3Array* vArray() { return static_cast<osg::Vec3Array*>(_geometry->getVertexArray()); }
    osg::Vec3Array* nArray() { return static_cast<osg::Vec3Array*>(_geometry->getNormalArray()); }
    osg::Vec2Array* tArray() { return static_cast<osg::Vec2Array*>(_geometry->getTexCoordArray(0)); }

    void operator()(unsigned int i0, unsigned int i1, unsigned int i2)
    { _faceList.push_back(osg::Vec3ui(i0, i1, i2)); }

    static MikkTSpaceHelper* me(const SMikkTSpaceContext* pContext)
    { return static_cast<MikkTSpaceHelper*>(pContext->m_pUserData); }

    static int mikk_getNumFaces(const SMikkTSpaceContext* pContext) { return (int)me(pContext)->_faceList.size(); }
    static int mikk_getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace) { return 3; }

    static void mikk_getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[],
                                 const int iFace, const int iVert)
    {
        osg::Vec3Array* vArray = me(pContext)->vArray();
        const osg::Vec3& v = vArray->at(me(pContext)->_faceList[iFace][iVert]);
        for (int i = 0; i < 3; ++i) fvPosOut[i] = v[i];
    }

    static void mikk_getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[],
                               const int iFace, const int iVert)
    {
        osg::Vec3Array* nArray = me(pContext)->nArray();
        const osg::Vec3& v = nArray->at(me(pContext)->_faceList[iFace][iVert]);
        for (int i = 0; i < 3; ++i) fvNormOut[i] = v[i];
    }

    static void mikk_getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[],
                                 const int iFace, const int iVert)
    {
        osg::Vec2Array* tArray = me(pContext)->tArray();
        const osg::Vec2& v = tArray->at(me(pContext)->_faceList[iFace][iVert]);
        for (int i = 0; i < 2; ++i) fvTexcOut[i] = v[i];
    }

    static void mikk_setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[],
                                    const float fSign, const int iFace, const int iVert)
    {
        MikkTSpaceHelper* self = me(pContext); unsigned int vIndex = self->_faceList[iFace][iVert];
        osg::Vec3 T(fvTangent[0], fvTangent[1], fvTangent[2]);
        osg::Vec3 N = self->nArray()->at(vIndex);  osg::Vec3 B = (N ^ T) * fSign;
        (*self->tangents)[vIndex] = T; (*self->binormals)[vIndex] = B;
    }
};

namespace osgVerse
{
    osg::Texture2D* createDefaultTexture(const osg::Vec4& color)
    {
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA);

        osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
        *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
        tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        tex2D->setImage(image.get()); return tex2D.release();
    }

    osg::Texture2D* createTexture2D(osg::Image* image, osg::Texture::WrapMode mode)
    {
        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setWrap(osg::Texture2D::WRAP_S, mode);
        tex2D->setWrap(osg::Texture2D::WRAP_T, mode);
        tex2D->setResizeNonPowerOfTwoHint(false);
        tex2D->setImage(image); return tex2D.release();
    }

    osg::Geode* createScreenQuad(const osg::Vec3& corner, float width, float height, const osg::Vec4& uvRange)
    {
        osg::Geometry* geom = osg::createTexturedQuadGeometry(
            corner, osg::Vec3(width, 0.0f, 0.0f), osg::Vec3(0.0f, height, 0.0f),
            uvRange[0], uvRange[1], uvRange[2], uvRange[3]);
        osg::ref_ptr<osg::Geode> quad = new osg::Geode;
        quad->addDrawable(geom);

        int values = osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED;
        quad->getOrCreateStateSet()->setAttribute(
            new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL), values);
        quad->getOrCreateStateSet()->setMode(GL_LIGHTING, values);
        return quad.release();
    }

    osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture* tex,
        osg::GraphicsContext* gc, bool screenSpaced)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setDrawBuffer(GL_FRONT);
        camera->setReadBuffer(GL_FRONT);
        camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setGraphicsContext(gc);
        if (tex)
        {
            tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
            tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
            camera->attach(buffer, tex);
        }

        if (screenSpaced)
        {
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
            camera->setViewMatrix(osg::Matrix::identity());
            camera->addChild(createScreenQuad(osg::Vec3(), 1.0f, 1.0f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f)));
        }
        return camera.release();
    }

    osg::Camera* createHUDCamera(osg::GraphicsContext* gc, int w, int h, const osg::Vec3& quadPt,
        float quadW, float quadH, bool screenSpaced)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setAllowEventFocus(false);
        camera->setGraphicsContext(gc);
        camera->setViewport(0, 0, w, h);
        camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        if (screenSpaced)
        {
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
            camera->setViewMatrix(osg::Matrix::identity());
            camera->addChild(createScreenQuad(quadPt, quadW, quadH, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f)));
        }
        return camera.release();
    }

    TangentSpaceVisitor::TangentSpaceVisitor(const float threshold)
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _angularThreshold(threshold)
    {
        _mikkiTSpace = new SMikkTSpaceContext;
        _mikkiTSpace->m_pInterface = new SMikkTSpaceInterface;
        _mikkiTSpace->m_pUserData = NULL;
    }

    TangentSpaceVisitor::~TangentSpaceVisitor()
    {
        if (_mikkiTSpace != NULL)
        {
            if (_mikkiTSpace->m_pInterface) delete _mikkiTSpace->m_pInterface;
            delete _mikkiTSpace; _mikkiTSpace = NULL;
        }
    }

    void TangentSpaceVisitor::apply(osg::Geode& node)
    {
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (!geom || (geom && geom->getNormalArray() == NULL)) continue;
            if (geom->getNormalBinding() != osg::Geometry::BIND_PER_VERTEX) continue;
            
            osg::TriangleIndexFunctor<MikkTSpaceHelper> functor;
            geom->accept(functor);
            if (functor.initialize(_mikkiTSpace, geom))
                genTangSpace(_mikkiTSpace, _angularThreshold);
        }
        traverse(node);
    }

    NormalMapGenerator::NormalMapGenerator(double nStrength, double spScale, double spContrast, bool nInvert)
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _nStrength(nStrength), _spScale(spScale), _spContrast(spContrast),
        _normalMapUnit(1), _specMapUnit(2), _nInvert(nInvert) {}

    void NormalMapGenerator::apply(osg::Node& node)
    {
        if (node.getStateSet()) apply(*node.getStateSet());
        traverse(node);
    }

    void NormalMapGenerator::apply(osg::Geode& node)
    {
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Drawable* drawable = node.getDrawable(i);
            if (drawable && drawable->getStateSet())
                apply(*drawable->getStateSet());
        }

        if (node.getStateSet()) apply(*node.getStateSet());
        traverse(node);
    }

    void NormalMapGenerator::apply(osg::StateSet& ss)
    {
        osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(
            ss.getTextureAttribute(0, osg::StateAttribute::TEXTURE));
        if (!tex2D) return;

        osg::Image* image = tex2D->getImage();
        if (!image || (image && !image->valid())) return;
        if ((image->getPixelFormat() != GL_RGBA && image->getPixelFormat() != GL_RGB) ||
            image->getDataType() != GL_UNSIGNED_BYTE)
        {
            OSG_NOTICE << "[NormalMapGenerator] Only support Vec3ub/Vec4ub pixels, mismatched with "
                       << image->getFileName() << std::endl; return;
        }

        double invPixel = 1.0 / 255.0;
        if (_normalMapUnit > 0)
        {
            NormalmapGenerator ng(IntensityMap::AVERAGE, invPixel, invPixel, invPixel, invPixel);
            osg::ref_ptr<osg::Image> nMap = ng.calculateNormalmap(
                image, NormalmapGenerator::PREWITT, _nStrength, _nInvert);
            if (nMap.valid() && nMap->valid())
                ss.setTextureAttributeAndModes(_normalMapUnit, createTexture2D(nMap.get()));
        }

        if (_specMapUnit > 0)
        {
            SpecularmapGenerator spg(IntensityMap::AVERAGE, invPixel, invPixel, invPixel, invPixel);
            osg::ref_ptr<osg::Image> spMap = spg.calculateSpecmap(image, _spScale, _spContrast);
            if (spMap.valid() && spMap->valid())
                ss.setTextureAttributeAndModes(_specMapUnit, createTexture2D(spMap.get()));
        }
        OSG_NOTICE << "Normal-map generation for " << image->getFileName() << " finished" << std::endl;
    }

    void Frustum::create(const osg::Matrix& modelview, const osg::Matrix& originProj,
                         float preferredNear, float preferredFar)
    {
        double znear = 0.0, zfar = 0.0, epsilon = 1e-6;
        osg::Matrixd proj = originProj;
        if (preferredNear < preferredFar)
        {
            if (fabs(proj(0, 3)) < epsilon  && fabs(proj(1, 3)) < epsilon  && fabs(proj(2, 3)) < epsilon)
            {
                double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0;
                proj.getOrtho(left, right, bottom, top, znear, zfar);
                proj = osg::Matrix::ortho(
                    left, right, bottom, top, (preferredNear >= 0.0f) ? preferredNear : znear, preferredFar);
            }
            else
            {
                double ratio = 0.0, fovy = 0.0;
                proj.getPerspective(fovy, ratio, znear, zfar);
                proj = osg::Matrix::perspective(
                    fovy, ratio, (preferredNear > 0.0f) ? preferredNear : znear, preferredFar);
            }
        }

        osg::Matrixd clipToWorld;
        clipToWorld.invert(modelview * proj);
        corners[0].set(-1.0, -1.0, -1.0); corners[1].set(1.0, -1.0, -1.0);
        corners[2].set(1.0, 1.0, -1.0); corners[3].set(-1.0, 1.0, -1.0);
        corners[4].set(-1.0, -1.0, 1.0); corners[5].set(1.0, -1.0, 1.0);
        corners[6].set(1.0, 1.0, 1.0); corners[7].set(-1.0, 1.0, 1.0);
        for (int i = 0; i < 8; ++i) corners[i] = corners[i] * clipToWorld;

        centerNearPlane = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25;
        centerFarPlane = (corners[4] + corners[5] + corners[6] + corners[7]) * 0.25;
        center = (centerNearPlane + centerFarPlane) * 0.5;
        frustumDir = centerFarPlane - centerNearPlane;
        frustumDir.normalize();
    }

    osg::BoundingBox Frustum::createShadowBound(const std::vector<osg::Vec3>& refPoints,
                                                const osg::Matrix& worldToLocal)
    {
        osg::BoundingBox lightSpaceBB0, lightSpaceBB1;
        for (int i = 0; i < 8; ++i) lightSpaceBB0.expandBy(corners[i] * worldToLocal);
        if (refPoints.empty()) return lightSpaceBB0;

        for (size_t i = 0; i < refPoints.size(); ++i)
            lightSpaceBB1.expandBy(refPoints[i] * worldToLocal);
        if (lightSpaceBB1._min[0] > lightSpaceBB0._min[0]) lightSpaceBB0._min[0] = lightSpaceBB1._min[0];
        if (lightSpaceBB1._min[1] > lightSpaceBB0._min[1]) lightSpaceBB0._min[1] = lightSpaceBB1._min[1];
        if (lightSpaceBB1._max[0] < lightSpaceBB0._max[0]) lightSpaceBB0._max[0] = lightSpaceBB1._max[0];
        if (lightSpaceBB1._max[1] < lightSpaceBB0._max[1]) lightSpaceBB0._max[1] = lightSpaceBB1._max[1];
        return lightSpaceBB0;
    }
}
