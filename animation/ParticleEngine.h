#ifndef MANA_ANIM_PARTICLEENGINE_HPP
#define MANA_ANIM_PARTICLEENGINE_HPP

#include <osg/Version>
#include <osg/Drawable>
#include <osg/MatrixTransform>
#include <map>

namespace Effekseer
{
    class Manager;
}

namespace osgVerse
{

    class ParticleDrawable : public osg::Drawable
    {
    public:
        ParticleDrawable(int maxInstances = 8000);
        ParticleDrawable(const ParticleDrawable& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        Effekseer::Manager* getManager() const;

#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
        virtual osg::BoundingBox computeBoundingBox() const;
        virtual osg::BoundingSphere computeBound() const;
#else
        virtual osg::BoundingBox computeBound() const;
#endif
        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;

    protected:
        virtual ~ParticleDrawable();

        osg::ref_ptr<osg::Referenced> _data;
    };

}

#endif
