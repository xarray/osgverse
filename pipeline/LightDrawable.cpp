#include <osg/PolygonMode>
#include <osgUtil/CullVisitor>
#include "LightDrawable.h"
using namespace osgVerse;

bool LightCullCallback::cull(osg::NodeVisitor*, osg::Drawable* drawable, osg::State* state) const
{
    LightDrawable* ld = static_cast<LightDrawable*>(drawable);
    if (ld != NULL)
    {
        // TODO
        // cull this drawable first
        // if not culled, add parameters to global light manager
        // sort lights by distance to eye
        // save all lights to a parameter texture
        // use the texture in deferred shader
    }
    return !ld->getDebugShow();
}

LightDrawable::LightDrawable()
:   osg::ShapeDrawable(), _debugShow(false)
{
    _lightColor.set(1.0f, 1.0f, 1.0f, 1.0f);
    _position.set(0.0f, 0.0f, 1.0f, 0.0f);
    _direction.set(0.0f, 0.0f, -1.0f);
    _attenuationRange.set(0.0f, 0.0f);
    _spotExponent = 0.0f; _spotCutoff = 180.0f;
    _callback = new LightCullCallback;
    setCullCallback(_callback.get());
    recreate();
}

LightDrawable::LightDrawable(const LightDrawable& copy, const osg::CopyOp& copyop)
:   osg::ShapeDrawable(copy, copyop), _callback(copy._callback), _lightColor(copy._lightColor),
    _position(copy._position), _direction(copy._direction), _attenuationRange(copy._attenuationRange),
    _spotExponent(copy._spotExponent), _spotCutoff(copy._spotCutoff), _debugShow(copy._debugShow) {}

LightDrawable::Type LightDrawable::getType(bool& unlimited) const
{
    unlimited = true;
    if (_position.w() > 0.0f)
    {
        if (_spotExponent > 0.0f) { unlimited = false; return SpotLight; }
        else { unlimited = _attenuationRange[0] < _attenuationRange[1]; return PointLight; }
    }
    return Directional;
}

void LightDrawable::recreate()
{
    bool unlimited = false;
    osg::ref_ptr<osg::Shape> shape;
    switch (getType(unlimited))
    {
    case Directional:
        break;
    case PointLight:
        break;
    case SpotLight:
        break;
    }
    setShape(shape.get());
    dirtyBound(); dirtyDisplayList();
}
