#ifndef MANA_PP_LIGHTDRAWABLE_HPP
#define MANA_PP_LIGHTDRAWABLE_HPP

#include <osg/ShapeDrawable>
#include <osg/Transform>

namespace osgVerse
{
    class LightCullCallback : public osg::Drawable::CullCallback
    {
    public:
        virtual bool cull(osg::NodeVisitor*, osg::Drawable* drawable, osg::State* state) const;
    };

    /** The light drawable which is used in deferred pipeline. */
    class LightDrawable : public osg::ShapeDrawable
    {
    public:
        LightDrawable();
        LightDrawable(const LightDrawable& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        enum Type { Invalid = 0, Directional, PointLight, SpotLight };
        Type getType(bool& unlimited) const;

        /** Set the color & power of the light. */
        inline void setColor(const osg::Vec3& color) { _lightColor = color; }

        /** Get the color & power of the light. */
        inline const osg::Vec3& getColor() const { return _lightColor; }

        /** Set the position of the light. */
        inline void setPosition(const osg::Vec3& position) { _position = position; recreate(); }

        /** Get the position of the light. */
        inline const osg::Vec3& getPosition() const { return _position; }

        /** Set the direction of the light. */
        inline void setDirection(const osg::Vec3& direction) { _direction = direction; recreate(); }

        /** Get the direction of the light. */
        inline const osg::Vec3& getDirection() const { return _direction; }

        /** Set if light is directional */
        inline void setDirectional(bool b) { _directional = b; recreate(); }

        /** Set if light is directional */
        inline bool getDirectional() const { return _directional; }

        /** Set the attenuation range of the light. */
        inline void setMaxRange(float r) { _attenuationRange = osg::Vec2(0.0f, r); recreate(); }
        inline void setRange(const osg::Vec2& r) { _attenuationRange = r; recreate(); }

        /** Get the attenuation range of the light. */
        inline osg::Vec2 getRange() const { return _attenuationRange; }

        /** Set the spot exponent of the light. */
        inline void setSpotExponent(float se) { _spotExponent = se; }

        /** Get the spot exponent of the light. */
        inline float getSpotExponent() const { return _spotExponent; }

        /** Set the spot cutoff (in radians) of the light. */
        inline void setSpotCutoff(float sc) { _spotCutoff = sc; recreate(); }

        /** Get the spot cutoff (in radians) of the light. */
        inline float getSpotCutoff() const { return _spotCutoff; }

        /** Set if light should be treated as in eye-space, which can follow the viewer */
        inline void setEyeSpace(bool b) { _eyeSpace = b; }

        /** Get if light should be treated as in eye-space */
        inline bool getEyeSpace() const { return _eyeSpace; }

        /** Set if show debug wireframe model of the light. */
        void setDebugShow(bool b) { _debugShow = b; }

        /** Get if show debug wireframe model of the light. */
        bool getDebugShow() const { return _debugShow; }

    protected:
        virtual ~LightDrawable();
        void recreate();

        osg::Vec3 _position, _direction, _lightColor;
        osg::Vec2 _attenuationRange;
        float _spotExponent, _spotCutoff;
        bool _eyeSpace, _directional, _debugShow;
    };
}

#endif
