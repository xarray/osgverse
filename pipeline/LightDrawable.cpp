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

        // If not culled, add parameters to global light manager. Note the light is recorded in
        // its own local space: it may well have been culled for a camera which is not the one
        // finally shading the scene (a shadow or a sky camera, and only the last of them would
        // have its matrix stored), so the view transform is applied later on by the light
        // module, once it knows the camera it is building the light table for
        LightGlobalManager::LightData lData;
        lData.light = ld; lData.frameNo = cv->getFrameStamp()->getFrameNumber();
        LightGlobalManager::instance()->add(lData);
        return !ld->getDebugShow();
    }
    return osg::Drawable::CullCallback::cull(nv, drawable, state);
}

LightDrawable::LightDrawable()
:   osg::ShapeDrawable(), _debugSizeScale(0.1f), _eyeSpace(false), _debugShow(false)
{
    setUseDisplayList(false); setUseVertexBufferObjects(true);
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
    _spotCutoff(copy._spotCutoff), _debugSizeScale(copy._debugSizeScale),
    _eyeSpace(copy._eyeSpace), _directional(copy._directional), _debugShow(copy._debugShow),
    _lightBound(copy._lightBound) {}

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

    // An unlimited light has no range, so a nominal size keeps its volume from collapsing to a
    // point; the drawn marker of a bounded light is scaled down by _debugSizeScale, so that a
    // light whose attenuation reaches far (a hundred times the size of a room, for instance) does
    // not fill the whole view with a huge wireframe volume
    if (length <= 0.0f) length = 1.0f;
    float markerLength = osg::maximum(length * _debugSizeScale, length * 0.001f);

    // The volume the light influence reaches (used for culling, see computeBoundingBox) is a
    // sphere of the attenuation range around the position: the shader falloff is purely radial
    switch (getType(unlimited))
    {
    case Directional:
        {   // A long cuboid along the light direction, so a directional light is easy to tell from
            // the sphere of a point light and the cone of a spot light in the debug view
            osg::Box* box = new osg::Box(_position, markerLength, markerLength * 0.5f, markerLength * 0.5f);
            box->setRotation(q); shape = box;
        }
        break;
    case SpotLight:
        if (_spotCutoff < osg::PI_2)
        {
            float radius = markerLength * atan(_spotCutoff);
            osg::Cone* cone = new osg::Cone(_position, radius, markerLength);
            cone->setRotation(q); shape = cone;
        }
        else
            shape = new osg::Sphere(_position, markerLength);
        break;
    default:
        shape = new osg::Sphere(_position, markerLength);
        break;
    }

    _lightBound.set(_position, length);
    setShape(shape.get());
    setComputeBoundingBoxCallback(
        unlimited ? new osgVerse::DisableBoundingBoxCallback : NULL);
    dirtyBound();
}

osg::BoundingBox LightDrawable::computeBoundingBox() const
{
    // Only reached for a bounded light (unlimited ones install DisableBoundingBoxCallback, so the
    // cull visitor never drops them): the influence of the light has to stay cullable even though
    // the drawn shape is only a small marker
    osg::BoundingBox bb; bb.expandBy(_lightBound); return bb;
}
