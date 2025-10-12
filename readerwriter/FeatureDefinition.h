#ifndef MANA_READERWRITER_FEATUREDEFINITION_HPP
#define MANA_READERWRITER_FEATUREDEFINITION_HPP

#include <osg/Object>
#include <osg/BoundingBox>

namespace osgVerse
{

    class Feature : public osg::Object
    {
    public:
        Feature(osg::Object* parent = NULL, GLenum t = GL_NONE)
            : _parent(parent), _type(t) { if (parent) parent->setUserData(this); }
        Feature(const Feature& f, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : _parent(f._parent), _bound(f._bound), _type(f._type) {}
        META_Object(osgVerse, Feature)

        osg::Object* getParent() { return _parent; }
        const osg::Object* getParent() const { return _parent; }

        void addPointsToBound(osg::Vec3Array* va)
        { for (size_t i = 0; i < va->size(); ++i) _bound.expandBy((*va)[i]); }
        void addPointToBound(const osg::Vec3& pt) { _bound.expandBy(pt); }
        void resetBound() { _bound = osg::BoundingBox(); }
        const osg::BoundingBox& getBound() const { return _bound; }

        void setType(GLenum t) { _type = t; }
        GLenum getType() const { return _type; }

    protected:
        osg::Object* _parent;
        osg::BoundingBox _bound;
        GLenum _type;
    };

}

#endif
