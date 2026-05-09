#ifndef MANA_MODELING_ANNOTATIONMAKER_HPP
#define MANA_MODELING_ANNOTATIONMAKER_HPP

#include <osg/Geode>
#include <osg/Geometry>
#include <map>
#include <vector>
#include <iostream>

namespace osgVerse
{

struct AnnotationData
{
    osg::Vec3d box[8];
    std::string name;
};

/** Create semantic annotations and bounding boxes in an interactive way */
class AnnotationMaker : public osg::Referenced
{
public:
    AnnotationMaker();
    osg::Geode* getOrCreateGeode();
    osg::Geode* getOrCreateTextGeode();
    void dirtyGeode(bool onlyCurrent = false);

    bool load(std::istream& in, bool eraseCurrent);
    bool save(std::ostream& out) const;

    int addAnnotation(const std::string& name);
    void removeAnnotation(int id);

    bool setAnnotation(int id, AnnotationData& ad);
    bool getAnnotation(int id, AnnotationData& ad) const;

    void setCurrentEditing(int id) { _currentID = id; dirtyGeode(); }
    int getCurrentEditing() const { return _currentID; }

    std::map<int, AnnotationData>& getAnnotations() { return _annotations; }
    const std::map<int, AnnotationData>& getAnnotations() const { return _annotations; }

protected:
    std::map<int, AnnotationData> _annotations;
    osg::ref_ptr<osg::Geode> _geode, _textGeode;
    int _currentID;
};

}

#endif
