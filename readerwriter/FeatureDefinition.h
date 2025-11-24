#ifndef MANA_READERWRITER_FEATUREDEFINITION_HPP
#define MANA_READERWRITER_FEATUREDEFINITION_HPP

#include <osg/Geometry>
#include <osg/BoundingBox>
#include "Export.h"

namespace osgVerse
{
    class Drawer2D;
    struct DrawerStyleData;

    class Feature : public osg::Object
    {
    public:
        Feature(GLenum t = GL_NONE, osg::Object* parent = NULL)
            : _parent(parent), _type(t) { if (parent) parent->setUserData(this); }
        Feature(const Feature& f, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : _ptList(f._ptList), _parent(f._parent), _bound(f._bound), _type(f._type) {}
        META_Object(osgVerse, Feature)

        void addPoints(osg::Vec3Array* va)
        {
            if (!va) return; else  _ptList.push_back(va);
            for (size_t i = 0; i < va->size(); ++i) _bound.expandBy((*va)[i]);
        }

        void addPoint(const osg::Vec3& pt, unsigned int index = 0)
        {
            if (_ptList.size() < index) _ptList.resize(index);
            osg::Vec3Array* va = _ptList[index].get();
            if (!va) { va = new osg::Vec3Array; _ptList[index] = va; }
            va->push_back(pt); _bound.expandBy(pt);
        }

        void removePoints(unsigned int i) { if (i < _ptList.size()) _ptList.erase(_ptList.begin() + i); }
        void clearAllPoints() { _ptList.clear(); }

        osg::Vec3Array* getPoints(unsigned int i = 0) { return (i < _ptList.size()) ? _ptList[i].get() : NULL; }
        const osg::Vec3Array* getPoints(unsigned int i = 0) const { return (i < _ptList.size()) ? _ptList[i].get() : NULL; }

        std::vector<osg::ref_ptr<osg::Vec3Array>>& getPointList() { return _ptList; }
        const std::vector<osg::ref_ptr<osg::Vec3Array>>& getPointList() const { return _ptList; }

        osg::Object* getParent() { return _parent; }
        const osg::Object* getParent() const { return _parent; }

        void resetBound() { _bound = osg::BoundingBox(); }
        const osg::BoundingBox& getBound() const { return _bound; }

        void setType(GLenum t) { _type = t; }
        GLenum getType() const { return _type; }

    protected:
        std::vector<osg::ref_ptr<osg::Vec3Array>> _ptList;
        osg::Object* _parent;
        osg::BoundingBox _bound;
        GLenum _type;  // GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_POLYGON
    };

    /** Render the feature on a 2D drawer image */
    OSGVERSE_RW_EXPORT void drawFeatureToImage(Feature& f, Drawer2D* drawer, DrawerStyleData* style = NULL);

    /** Add the feature to an existing geometry */
    OSGVERSE_RW_EXPORT void addFeatureToGeometry(Feature& f, osg::Geometry* geom, bool asNewPrimitiveSet);
}

#endif
