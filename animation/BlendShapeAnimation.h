#ifndef MANA_ANIM_BLENDSHAPEANIMATION_HPP
#define MANA_ANIM_BLENDSHAPEANIMATION_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>

namespace osgVerse
{

    /** The blendshape animation support class */
    class BlendShapeAnimation : public osg::Drawable::UpdateCallback
    {
    public:
        BlendShapeAnimation();
        void apply(const std::vector<std::string>& names, const std::vector<double>& weights);
        virtual void update(osg::NodeVisitor* nv, osg::Drawable* drawable);

        struct BlendShapeData : public osg::Referenced
        {
            std::string name; double weight;
            osg::ref_ptr<osg::Vec3Array> vertices, normals;
            osg::ref_ptr<osg::Vec4Array> tangents;
            BlendShapeData() : weight(0.0) {}
        };

        void addBlendShapeData(BlendShapeData* bd) { _blendshapes.push_back(bd); }
        BlendShapeData* getBlendShapeData(unsigned int i) { return _blendshapes[i].get(); }
        unsigned int getNumBlendShapes() const { return _blendshapes.size(); }

        std::vector<osg::ref_ptr<BlendShapeData>>& getAllBlendShapes() { return _blendshapes; }
        const std::vector<osg::ref_ptr<BlendShapeData>>& getAllBlendShapes() const { return _blendshapes; }

    protected:
        std::vector<osg::ref_ptr<BlendShapeData>> _blendshapes;
        std::map<std::string, osg::observer_ptr<BlendShapeData>> _blendshapeMap;
    };

}

#endif
