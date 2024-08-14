#include <osg/io_utils>
#include <osg/PolygonMode>
#include <osgUtil/CullVisitor>
#include "LightDrawable.h"
#include "LightModule.h"
#include <iostream>
using namespace osgVerse;

bool LightCullCallback::cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::State* state) const
{
    osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
    LightDrawable* ld = static_cast<LightDrawable*>(drawable);
    if (ld != NULL && cv != NULL)
    {
        // Cull this drawable first
        bool unlimited = false; LightDrawable::Type t = ld->getType(unlimited);
#if OSG_VERSION_GREATER_THAN(3, 2, 3)
        if (!unlimited && cv->isCulled(ld->getBoundingBox())) return true;
#else
        if (!unlimited && cv->isCulled(ld->getBound())) return true;
#endif

        // If not culled, add parameters to global light manager
        LightGlobalManager::LightData lData;
        lData.light = ld; lData.frameNo = cv->getFrameStamp()->getFrameNumber();
        lData.matrix = ld->getEyeSpace() ? osg::Matrix() : (*cv->getModelViewMatrix());
        LightGlobalManager::instance()->add(lData);
        return !ld->getDebugShow();
    }
    return osg::Drawable::CullCallback::cull(nv, drawable, state);
}

LightDrawable::LightDrawable()
:   osg::ShapeDrawable(), _eyeSpace(false), _debugShow(false)
{
    setCullCallback(LightGlobalManager::instance()->getCallback());
    _lightColor.set(1.0f, 1.0f, 1.0f);
    _position.set(0.0f, 0.0f, 1.0f);
    _direction.set(1.0f, 0.0f, 0.0f);
    _attenuationRange = 0.0f; _spotCutoff = 0.0f;
    _directional = false; recreate();
}

LightDrawable::LightDrawable(const LightDrawable& copy, const osg::CopyOp& copyop)
:   osg::ShapeDrawable(copy, copyop), _lightColor(copy._lightColor),
    _position(copy._position), _direction(copy._direction), _attenuationRange(copy._attenuationRange),
    _spotCutoff(copy._spotCutoff), _eyeSpace(copy._eyeSpace),
    _directional(copy._directional), _debugShow(copy._debugShow) {}

LightDrawable::~LightDrawable()
{
    LightGlobalManager* instance = LightGlobalManager::instance();
    if (instance != NULL) instance->remove(this);
}

LightDrawable::Type LightDrawable::getType(bool& unlimited) const
{
    if (!_directional)
    {
        if (_spotCutoff > 0.0f) { unlimited = false; return SpotLight; }
        else { unlimited = !(_attenuationRange > 0.0f); return PointLight; }
    }
    else
    {
        unlimited = !(_attenuationRange > 0.0f);
        return Directional;
    }
}

void LightDrawable::recreate()
{
    bool unlimited = false;
    osg::ref_ptr<osg::Shape> shape;
    osg::Quat q; q.makeRotate(osg::X_AXIS, _direction);
    float length = _attenuationRange;

    switch (getType(unlimited))
    {
    case Directional:
        {
            osg::Cylinder* cylinder = new osg::Cylinder(_position, length * 0.5f, length);
            cylinder->setRotation(q); shape = cylinder;
        }
        break;
    case SpotLight:
        if (_spotCutoff < osg::PI_2)
        {
            float radius = length * atan(_spotCutoff);
            osg::Cone* cone = new osg::Cone(_position, radius, length);
            cone->setRotation(q); shape = cone;
        }
        else
            shape = new osg::Sphere(_position, length);
        break;
    default:
        shape = new osg::Sphere(_position, length);
        break;
    }

    setShape(shape.get());
    setComputeBoundingBoxCallback(
        unlimited ? new osgVerse::DisableBoundingBoxCallback : NULL);
    dirtyBound(); dirtyDisplayList();
}
