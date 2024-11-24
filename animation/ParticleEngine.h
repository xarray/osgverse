#ifndef MANA_ANIM_PARTICLEENGINE_HPP
#define MANA_ANIM_PARTICLEENGINE_HPP

#include <osg/Version>
#include <osg/Drawable>
#include <osg/MatrixTransform>
#include <map>

namespace Effekseer
{
    class Manager;
    class Effect;
}

namespace osgVerse
{

    class ParticleDrawable : public osg::Drawable
    {
    public:
        ParticleDrawable(int maxInstances = 8000);
        ParticleDrawable(const ParticleDrawable& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
        virtual const char* libraryName() const { return "osgVerse"; }
        virtual const char* className() const { return "ParticleDrawable"; }

        virtual Object* cloneType() const { return new ParticleDrawable; }
        virtual Object* clone(const osg::CopyOp& copyop) const { return new ParticleDrawable(*this, copyop); }
        virtual bool isSameKindAs(const osg::Object* obj) const
        { return dynamic_cast<const ParticleDrawable*>(obj) != NULL; }

        enum PlayingState
        { INVALID = -1, STOPPED = 0, PLAYING = 1, PAUSED = 2 };

        Effekseer::Effect* createEffect(const std::string& name, const std::string& fileName);
        void destroyEffect(const std::string& name);
        bool playEffect(const std::string& name, PlayingState state);

        PlayingState getEffectState(const std::string& name) const;
        Effekseer::Effect* getEffect(const std::string& name) const;
        Effekseer::Manager* getManager() const;

#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
        virtual osg::BoundingBox computeBoundingBox() const;
        virtual osg::BoundingSphere computeBound() const;
#else
        virtual osg::BoundingBox computeBound() const;
#endif
        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
        virtual void releaseGLObjects(osg::State* state) const;

    protected:
        virtual ~ParticleDrawable();

        osg::ref_ptr<osg::Referenced> _data;
    };

}

#endif
