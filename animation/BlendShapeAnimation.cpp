#include <osg/io_utils>
#include "BlendShapeAnimation.h"
using namespace osgVerse;

BlendShapeAnimation::BlendShapeAnimation()
{
}

void BlendShapeAnimation::apply(const std::vector<std::string>& names,
                                const std::vector<double>& weights)
{
    size_t num = osg::minimum(names.size(), _blendshapes.size());
    size_t wNum = weights.size();
    for (size_t i = 0; i < num; ++i)
    {
        BlendShapeData* bsd = _blendshapes[i].get();
        if (i < wNum) bsd->weight = weights[i];
        bsd->name = names[i]; _blendshapeMap[bsd->name] = bsd;
    }
}

void BlendShapeAnimation::update(osg::NodeVisitor* nv, osg::Drawable* drawable)
{

}
