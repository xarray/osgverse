#include "Utilities.h"
#include "MultiEffectNode.h"
using namespace osgVerse;

void MultiEffectContainer::traverse(osg::Node& node, osgUtil::CullVisitor& cv)
{
    for (size_t i = 0; i < _statesets.size(); ++i)
    {
        osg::StateSet* ss = _statesets[i].get();
        if (!ss) continue; else cv.pushStateSet(ss);
        node.accept(cv); cv.popStateSet();
    }
}

void MultiEffectGeode::traverse(osg::NodeVisitor& nv)
{
    if (_container.valid() && _container->getEnabled())
    {
        osgUtil::CullVisitor* cv = nv.asCullVisitor();
        if (cv) { _container->traverse(*this, *cv); return; }
    }
    osg::Geode::traverse(nv);
}

void MultiEffectGroup::traverse(osg::NodeVisitor& nv)
{
    if (_container.valid() && _container->getEnabled())
    {
        osgUtil::CullVisitor* cv = nv.asCullVisitor();
        if (cv) { _container->traverse(*this, *cv); return; }
    }
    osg::Group::traverse(nv);
}
