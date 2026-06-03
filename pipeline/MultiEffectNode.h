#ifndef MANA_PP_MULTIEFFECT_NODE_HPP
#define MANA_PP_MULTIEFFECT_NODE_HPP

#include <osg/Geode>
#include <osg/Group>
#include <osg/StateSet>
#include <osgUtil/CullVisitor>

namespace osgVerse
{

    class MultiEffectContainer : public osg::Referenced
    {
    public:
        MultiEffectContainer() : _enabled(true) {}
        virtual void traverse(osg::Node& node, osgUtil::CullVisitor& cv);

        void addPass(osg::StateSet* ss) { _statesets.push_back(ss); }
        void removePass(unsigned int id) { _statesets.erase(_statesets.begin() + id); }
        unsigned int getNumPasses() const { return _statesets.size(); }

        osg::StateSet* getPass(unsigned int id) { return _statesets[id].get(); }
        const osg::StateSet* getPass(unsigned int id) const { return _statesets[id].get(); }
        std::vector<osg::ref_ptr<osg::StateSet>>& getPasses() { return _statesets; }
        const std::vector<osg::ref_ptr<osg::StateSet>>& getPasses() const { return _statesets; }

        void setEnabled(bool b) { _enabled = b; }
        bool getEnabled() const { return _enabled && !_statesets.empty(); }

    protected:
        virtual ~MultiEffectContainer() {}
        std::vector<osg::ref_ptr<osg::StateSet>> _statesets;
        bool _enabled;
    };

    class MultiEffectGeode : public osg::Geode
    {
    public:
        MultiEffectGeode() {}
        virtual void traverse(osg::NodeVisitor& nv);

        void setEffect(MultiEffectContainer* e) { _container = e; }
        MultiEffectContainer* getEffect() { return _container.get(); }
        const MultiEffectContainer* getEffect() const { return _container.get(); }
    
    protected:
        virtual ~MultiEffectGeode() {}
        osg::ref_ptr<MultiEffectContainer> _container;
    };

    class MultiEffectGroup : public osg::Group
    {
    public:
        MultiEffectGroup() {}
        virtual void traverse(osg::NodeVisitor& nv);

        void setEffect(MultiEffectContainer* e) { _container = e; }
        MultiEffectContainer* getEffect() { return _container.get(); }
        const MultiEffectContainer* getEffect() const { return _container.get(); }
    
    protected:
        virtual ~MultiEffectGroup() {}
        osg::ref_ptr<MultiEffectContainer> _container;
    };
}

#endif
