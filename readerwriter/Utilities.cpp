#include <osg/Geode>
#include "Utilities.h"
using namespace osgVerse;

void FixedFunctionOptimizer::apply(osg::Geode& geode)
{
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
    {
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
        if (geom)
        {
            removeUnusedStateAttributes(geom->getStateSet());
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
        }
    }
    removeUnusedStateAttributes(geode.getStateSet());
    NodeVisitor::apply(geode);
}

void FixedFunctionOptimizer::apply(osg::Group& node)
{
    removeUnusedStateAttributes(node.getStateSet());
    NodeVisitor::apply(node);
}

void FixedFunctionOptimizer::removeUnusedStateAttributes(osg::StateSet* ssPtr)
{
    if (ssPtr == NULL) return;
    osg::StateSet& ss = *ssPtr;

    ss.removeAttribute(osg::StateAttribute::ALPHAFUNC);
    ss.removeAttribute(osg::StateAttribute::CLIPPLANE);
    ss.removeAttribute(osg::StateAttribute::COLORMATRIX);
    ss.removeAttribute(osg::StateAttribute::FOG);
    ss.removeAttribute(osg::StateAttribute::LIGHT);
    ss.removeAttribute(osg::StateAttribute::LIGHTMODEL);
    ss.removeAttribute(osg::StateAttribute::LINESTIPPLE);
    ss.removeAttribute(osg::StateAttribute::LOGICOP);
    ss.removeAttribute(osg::StateAttribute::MATERIAL);
    ss.removeAttribute(osg::StateAttribute::POINT);
    ss.removeAttribute(osg::StateAttribute::POLYGONSTIPPLE);
    ss.removeAttribute(osg::StateAttribute::SHADEMODEL);

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    // Remove texture modes as they are not needed by glEnable() in GLES 2.0/3.x
    // https://docs.gl/es2/glEnable
    const osg::StateSet::TextureModeList& texModes = ss.getTextureModeList();
    for (size_t i = 0; i < texModes.size(); ++i)
    {
        osg::StateSet::ModeList modes = texModes[i];
        for (osg::StateSet::ModeList::const_iterator itr = modes.begin();
            itr != modes.end(); ++itr) ss.removeTextureMode(i, itr->first);
    }
#endif
}
