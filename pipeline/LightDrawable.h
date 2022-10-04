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

        enum Type { Directional, PointLight, SpotLight };
        Type getType(bool& unlimited) const;

        /** Set the color & power of the light. */
        inline void setColorAndPower(const osg::Vec4& color) { _lightColor = color; }

        /** Get the color & power of the light. */
        inline const osg::Vec4& getColorAndPower() const { return _lightColor; }

        /** Set the position of the light. */
        inline void setPosition(const osg::Vec4& position) { _position = position; recreate(); }

        /** Get the position of the light. */
        inline const osg::Vec4& getPosition() const { return _position; }

        /** Set the spot direction of the light. */
        inline void setSpotDirection(const osg::Vec3& direction) { _direction = direction; recreate(); }

        /** Get the spot direction of the light. */
        inline const osg::Vec3& getSpotDirection() const { return _direction; }

        /** Set the attenuation range of the light. */
        inline void setRange(const osg::Vec2& r) { _attenuationRange = r; recreate(); }

        /** Get the attenuation range of the light. */
        inline osg::Vec2 getRange() const { return _attenuationRange; }

        /** Set the spot exponent of the light. */
        inline void setSpotExponent(float se) { _spotExponent = se; }

        /** Get the spot exponent of the light. */
        inline float getSpotExponent() const { return _spotExponent; }

        /** Set the spot cutoff of the light. */
        inline void setSpotCutoff(float sc) { _spotCutoff = sc; recreate(); }

        /** Get the spot cutoff of the light. */
        inline float getSpotCutoff() const { return _spotCutoff; }

        /** Set if show debug wireframe model of the light. */
        void setDebugShow(bool b) { _debugShow = b; }

        /** Get if show debug wireframe model of the light. */
        bool getDebugShow() const { return _debugShow; }

    protected:
        virtual ~LightDrawable() {}
        void recreate();
        osg::ref_ptr<LightCullCallback> _callback;

        osg::Vec4 _lightColor, _position;
        osg::Vec3 _direction;
        osg::Vec2 _attenuationRange;
        float _spotExponent, _spotCutoff;
        bool _debugShow;
    };
}

#endif
