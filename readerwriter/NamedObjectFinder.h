#ifndef MANA_READERWRITER_NAMEDOBJECT_FINDER_HPP
#define MANA_READERWRITER_NAMEDOBJECT_FINDER_HPP

#include <osg/NodeVisitor>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Drawable>
#include <osg/StateSet>
#include <osg/Program>
#include <osg/Texture>

namespace osgVerse
{
    /** The helper for finding an object in the scene graph with the specified name */
    template<typename T>
    class NamedObjectFinder : public osg::NodeVisitor
    {
    public:
        /** Set how to compare the source name string and targets in scene graph */
        enum CompareMode { EQUAL, CONTAIN_SOURCE, CONTAIN_TARGET };

        /** Set the type of objects to search */
        enum Type
        {
            FIND_NODES = 0x1, FIND_DRAWABLES = 0x2,
            FIND_STATESETS = 0x4, FIND_ATTRIBUTES = 0x8,
            FIND_SHADERS = 0x10, FIND_UNIFORMS = 0x20
        };

        /** Set extra properties during the searching process */
        enum Characteristic
        { IGNORE_CHILDREN_WHEN_FOUND = 0x1 };

        NamedObjectFinder(CompareMode cm, int ch = 0)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), _compareMode(cm), _characteristics(ch)
            , _types(0) {}

        /** Convenient static method for finding named nodes */
        static inline std::vector<osg::Node*> findNamedNodes(
            osg::Node* root, const std::string& name, bool containsName = true)
        {
            typedef NamedObjectFinder<osg::Node> NodeFinder;
            NodeFinder finder(containsName ? NodeFinder::CONTAIN_SOURCE : NodeFinder::EQUAL,
                              NodeFinder::IGNORE_CHILDREN_WHEN_FOUND);
            finder.addNameCondition(NodeFinder::FIND_NODES, name);
            root->accept(finder);
            return finder.getResults(NodeFinder::FIND_NODES);
        }

        /** Convenient static method for finding named drawables */
        static inline std::vector<osg::Drawable*> findNamedDrawables(
            osg::Node* root, const std::string& name, bool containsName = true)
        {
            typedef NamedObjectFinder<osg::Drawable> DrawableFinder;
            DrawableFinder finder(containsName ? DrawableFinder::CONTAIN_SOURCE : DrawableFinder::EQUAL,
                                  DrawableFinder::IGNORE_CHILDREN_WHEN_FOUND);
            finder.addNameCondition(DrawableFinder::FIND_NODES, "");
            finder.addNameCondition(DrawableFinder::FIND_DRAWABLES, name);
            root->accept(finder);
            return finder.getResults(DrawableFinder::FIND_DRAWABLES);
        }

        /** Convenient static method for finding named textures */
        static inline std::vector<osg::Texture*> findNamedTextures(
            osg::Node* root, const std::string& name, bool containsName = true)
        {
            typedef NamedObjectFinder<osg::Texture> TexFinder;
            TexFinder finder(containsName ? TexFinder::CONTAIN_SOURCE : TexFinder::EQUAL,
                             TexFinder::IGNORE_CHILDREN_WHEN_FOUND);
            finder.addNameCondition(TexFinder::FIND_NODES, "");
            finder.addNameCondition(TexFinder::FIND_DRAWABLES, "");
            finder.addNameCondition(TexFinder::FIND_STATESETS, "");
            finder.addNameCondition(TexFinder::FIND_ATTRIBUTES, name);
            root->accept(finder);
            return finder.getResults(TexFinder::FIND_ATTRIBUTES);
        }

        /** Add a new condition for finding specified type of objects with specified name */
        void addNameCondition(Type t, const std::string& n) { _types |= t; _nameMap[t] = n; }
        std::vector<T*> getResults(Type t) { return _results[t]; }

        virtual void apply(osg::Node& node)
        {
            if (_types & FIND_NODES)
            {
                if (checkNode(&node) && (_characteristics & IGNORE_CHILDREN_WHEN_FOUND))
                    return;
            }
            traverse(node);
        }

        virtual void apply(osg::Geode& node)
        {
            if (_types & FIND_DRAWABLES)
            {
                for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
                {
                    osg::Drawable* drawable = node.getDrawable(i);
                    checkDrawable(drawable);
                }
            }

            if (_types & FIND_NODES)
            {
                if (checkNode(&node) && (_characteristics & IGNORE_CHILDREN_WHEN_FOUND))
                    return;
            }
            traverse(node);
        }

    protected:
        bool checkNode(osg::Node* node)
        {
            bool found = false;
            if (_types & FIND_STATESETS && node->getStateSet())
                found = checkStateSet(node->getStateSet());

            if (isNameMatched(FIND_NODES, node->getName()))
            {
                T* obj = dynamic_cast<T*>(node);
                if (obj) { _results[FIND_NODES].push_back(obj); found = true; }
            }
            return found;
        }

        bool checkDrawable(osg::Drawable* drawable)
        {
            bool found = false;
            if (_types & FIND_STATESETS && drawable->getStateSet())
                found = checkStateSet(drawable->getStateSet());

            if (isNameMatched(FIND_DRAWABLES, drawable->getName()))
            {
                T* obj = dynamic_cast<T*>(drawable);
                if (obj) { _results[FIND_DRAWABLES].push_back(obj); found = true; }
            }
            return found;
        }

        bool checkStateSet(osg::StateSet* ss)
        {
            bool found = false;
            if (isNameMatched(FIND_STATESETS, ss->getName()))
            {
                T* obj = dynamic_cast<T*>(ss);
                if (obj) { _results[FIND_STATESETS].push_back(obj); found = true; }
            }

            if (_types & FIND_ATTRIBUTES)
            {
                osg::StateSet::AttributeList& attributes = ss->getAttributeList();
                for (osg::StateSet::AttributeList::iterator itr = attributes.begin();
                    itr != attributes.end(); ++itr)
                {
                    osg::StateAttribute* sa = itr->second.first.get();
                    if (!isNameMatched(FIND_ATTRIBUTES, sa->getName())) continue;

                    T* obj = dynamic_cast<T*>(sa);
                    if (obj) { _results[FIND_ATTRIBUTES].push_back(obj); found = true; }
                }

                osg::StateSet::TextureAttributeList& texAttributes = ss->getTextureAttributeList();
                for (unsigned int n = 0; n < texAttributes.size(); ++n)
                {
                    for (osg::StateSet::AttributeList::iterator itr = texAttributes[n].begin();
                        itr != texAttributes[n].end(); ++itr)
                    {
                        osg::StateAttribute* sa = itr->second.first.get();
                        if (!isNameMatched(FIND_ATTRIBUTES, sa->getName()))
                        {
                            osg::Texture* tex = dynamic_cast<osg::Texture*>(sa);
                            if (!tex) continue;

                            bool hasMatchedImage = false;
                            for (unsigned int t = 0; t < tex->getNumImages(); ++t)
                            {
                                osg::Image* img = tex->getImage(t); if (!img) continue;
                                hasMatchedImage |= isNameMatched(FIND_ATTRIBUTES, img->getName());
                                hasMatchedImage |= isNameMatched(FIND_ATTRIBUTES, img->getFileName());
                            }
                            if (!hasMatchedImage) continue;
                        }

                        T* obj = dynamic_cast<T*>(sa);
                        if (obj) { _results[FIND_ATTRIBUTES].push_back(obj); found = true; }
                    }
                }
            }

            if (_types & FIND_SHADERS)
            {
                osg::Program* program = dynamic_cast<osg::Program*>(ss->getAttribute(osg::StateAttribute::PROGRAM));
                if (program && program->getNumShaders() > 0)
                {
                    for (unsigned int i = 0; i < program->getNumShaders(); ++i)
                    {
                        osg::Shader* shader = program->getShader(i);
                        if (!isNameMatched(FIND_SHADERS, shader->getName())) continue;

                        T* obj = dynamic_cast<T*>(shader);
                        if (obj) { _results[FIND_SHADERS].push_back(obj); found = true; }
                    }
                }
            }

            if (_types & FIND_UNIFORMS)
            {
                osg::StateSet::UniformList& uniforms = ss->getUniformList();
                for (osg::StateSet::UniformList::iterator itr = uniforms.begin();
                    itr != uniforms.end(); ++itr)
                {
                    if (!isNameMatched(FIND_UNIFORMS, itr->first)) continue;
                    T* obj = dynamic_cast<T*>(itr->second.first.get());
                    if (obj) { _results[FIND_UNIFORMS].push_back(obj); found = true; }
                }
            }
            return found;
        }

        bool isNameMatched(Type t, const std::string& name)
        {
            const std::string& src = _nameMap[t];
            switch (_compareMode)
            {
            case CONTAIN_SOURCE: return name.find(src) != std::string::npos;
            case CONTAIN_TARGET: return src.find(name) != std::string::npos;
            default: return name == src;
            }
        }

        CompareMode _compareMode;
        int _types, _characteristics;
        std::map<Type, std::string> _nameMap;
        std::map<Type, std::vector<T*>> _results;
    };
}

#endif
