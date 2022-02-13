#include <osg/FrameBufferObject>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/ContextData>
#include <osg/PolygonMode>
#include <osg/Geode>
#include <iostream>
#include "Utilities.h"

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
        tex2D->setImage(image.get());
        return tex2D.release();
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
