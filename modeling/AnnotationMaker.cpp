#include "AnnotationMaker.h"
#include "Utilities.h"
#include <picojson.h>
using namespace osgVerse;

namespace
{
    template <typename T>
    inline void applyBoundingBoxIndices(T& indices0, int startV, int startI)
    {
        indices0[startI++] = startV + 0; indices0[startI++] = startV + 1;
        indices0[startI++] = startV + 1; indices0[startI++] = startV + 2;
        indices0[startI++] = startV + 2; indices0[startI++] = startV + 3;
        indices0[startI++] = startV + 3; indices0[startI++] = startV + 0;
        indices0[startI++] = startV + 4; indices0[startI++] = startV + 5;
        indices0[startI++] = startV + 5; indices0[startI++] = startV + 6;
        indices0[startI++] = startV + 6; indices0[startI++] = startV + 7;
        indices0[startI++] = startV + 7; indices0[startI++] = startV + 4;
        indices0[startI++] = startV + 0; indices0[startI++] = startV + 4;
        indices0[startI++] = startV + 1; indices0[startI++] = startV + 5;
        indices0[startI++] = startV + 2; indices0[startI++] = startV + 6;
        indices0[startI++] = startV + 3; indices0[startI++] = startV + 7;
    }
}

AnnotationMaker::AnnotationMaker()
: _currentID(-1) { getOrCreateGeode(); }

osg::Geode* AnnotationMaker::getOrCreateGeode()
{
    if (!_geode)
    {
        _geode = new osg::Geode; dirtyGeode();
        _geode->setName("AnnotationRoot");
    }
    return _geode.get();
}

void AnnotationMaker::dirtyGeode(bool onlyCurrent)
{
    osg::ref_ptr<osg::Geometry> boxes0, boxes1;
    if (_geode->getNumDrawables() != 2)
    {
        osg::Vec4 color0(1.0f, 1.0f, 1.0f, 1.0f), color1(1.0f, 1.0f, 0.0f, 1.0f);
        boxes0 = createGeometry(new osg::Vec3Array(2), NULL, color0, new osg::DrawElementsUInt(GL_LINES));
        boxes1 = createGeometry(new osg::Vec3Array(2), NULL, color1, new osg::DrawElementsUByte(GL_LINES));
        boxes0->setName("AllAnnotations"); boxes1->setName("CurrentAnnotation");

        _geode->removeDrawables(0, _geode->getNumDrawables());
        _geode->addDrawable(boxes0.get()); _geode->addDrawable(boxes1.get());
    }
    
    osg::DrawElementsUInt* de0 = NULL; osg::DrawElementsUByte* de1 = NULL;
    boxes0 = static_cast<osg::Geometry*>(_geode->getDrawable(0));
    boxes1 = static_cast<osg::Geometry*>(_geode->getDrawable(1));
    if (!boxes0 || !boxes1 || boxes0->getNumPrimitiveSets() == 0 || boxes1->getNumPrimitiveSets() == 0)
    { _geode->removeDrawables(0, _geode->getNumDrawables()); dirtyGeode(); return; }

    // Update all annotations
    if (!onlyCurrent)
    {
        std::vector<osg::Vec3> positions((_annotations.size() - (_currentID < 0 ? 0 : 1)) * 8);
        std::vector<unsigned int> indices0(positions.size() * 3); int ptr = 0;
        for (std::map<int, AnnotationData>::iterator it = _annotations.begin();
            it != _annotations.end(); ++it, ++ptr)
        {
            AnnotationData& aData = it->second; int startV = ptr * 8, startI = ptr * 24;
            for (int i = 0; i < 8; ++i) positions[startV + i] = aData.box[i];
            applyBoundingBoxIndices(indices0, startV, startI);
        }

        osg::Vec3Array* va0 = static_cast<osg::Vec3Array*>(boxes0->getVertexArray());
        de0 = static_cast<osg::DrawElementsUInt*>(boxes0->getPrimitiveSet(0));
        va0->assign(positions.begin(), positions.end()); de0->assign(indices0.begin(), indices0.end());
        va0->dirty(); de0->dirty(); boxes0->dirtyBound();
    }

    // Update current editing annotation
    osg::Vec3Array* va1 = static_cast<osg::Vec3Array*>(boxes1->getVertexArray());
    de1 = static_cast<osg::DrawElementsUByte*>(boxes1->getPrimitiveSet(0));
    if (_currentID >= 0)
    {
        AnnotationData aData; getAnnotation(_currentID, aData);
        va1->resize(8); for (int i = 0; i < 8; ++i) (*va1)[i] = aData.box[i];
        de1->resize(24); applyBoundingBoxIndices(*de1, 0, 0);
    }
    else
        de1->clear();
    va1->dirty(); de1->dirty(); boxes1->dirtyBound();
}

bool AnnotationMaker::load(std::istream& in, bool eraseCurrent)
{
    picojson::value document;
    std::string err = picojson::parse(document, in);
    if (err.empty() && document.is<picojson::array>())
    {
        if (eraseCurrent) { _annotations.clear(); _currentID = -1; }
        try
        {
            picojson::array& boxList = document.get<picojson::array>();
            for (size_t i = 0; i < boxList.size(); ++i)
            {
                picojson::value& box = boxList[i];
                std::string idTag = box.contains("ins_id") ? "ins_id" : "id";
                std::string nTag = box.contains("label") ? "label" : "name";

                std::string name = box.contains(nTag) ? box.get(nTag).get<std::string>() : "";
                int id = _annotations.size() + 1;
                if (box.contains(idTag))
                {
                    picojson::value& idValue = box.get(idTag);
                    if (idValue.is<std::string>()) id = atoi(idValue.get<std::string>().c_str());
                    else if (idValue.is<double>()) id = (int)idValue.get<double>();
                }
                
                AnnotationData aData; aData.name = name;
                if (box.contains("bounding_box"))
                {
                    picojson::value& bbox = box.get("bounding_box");
                    if (bbox.is<picojson::array>())
                    {
                        picojson::array& ptList = bbox.get<picojson::array>();
                        for (size_t j = 0; j < ptList.size(); ++j)
                        {
                            picojson::value& pt = ptList[j]; if (j > 7) break;
                            aData.box[j] = osg::Vec3d(pt.contains("x") ? pt.get("x").get<double>() : 0.0,
                                                    pt.contains("y") ? pt.get("y").get<double>() : 0.0,
                                                    pt.contains("z") ? pt.get("z").get<double>() : 0.0);
                        }
                        _annotations[id] = aData;
                    }
                }
                else
                {}  // TODO: room_box?
            }
            dirtyGeode(); return true;
        }
        catch (const std::exception& e)
            { OSG_NOTICE << "[AnnotationMaker] Failed to parse annotations: " << e.what() << "\n"; }
    }
    else
        { OSG_NOTICE << "[AnnotationMaker] Failed to load annotations from json\n"; }
    return false;
}

bool AnnotationMaker::save(std::ostream& out) const
{
    // TODO
    return false;
}

int AnnotationMaker::addAnnotation(const std::string& name)
{
    int count = _annotations.size() + 1;
    AnnotationData data; data.name = name;

    std::map<int, AnnotationData>::iterator it = _annotations.find(count);
    if (it != _annotations.end()) return -1; else _currentID = count;
    _annotations[count] = data; return count;
}

void AnnotationMaker::removeAnnotation(int id)
{
    std::map<int, AnnotationData>::iterator it = _annotations.find(id);
    if (it != _annotations.end())
    {
        if (id == _currentID) _currentID = -1;
        _annotations.erase(it); dirtyGeode();
    }
}

bool AnnotationMaker::setAnnotation(int id, AnnotationData& ad)
{
    std::map<int, AnnotationData>::iterator it = _annotations.find(id);
    if (it != _annotations.end()) { it->second = ad; return true; }
    else return false;
}

bool AnnotationMaker::getAnnotation(int id, AnnotationData& ad) const
{
    std::map<int, AnnotationData>::const_iterator it = _annotations.find(id);
    if (it != _annotations.end()) { ad = it->second; return true; }
    else return false;
}
