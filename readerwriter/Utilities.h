#ifndef MANA_READERWRITER_UTILITIES_HPP
#define MANA_READERWRITER_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>
#ifdef __EMSCRIPTEN__
#   include <emscripten/fetch.h>
#   include <emscripten.h>
#endif

namespace osgVerse
{
    class FixedFunctionOptimizer : public osg::NodeVisitor
    {
    public:
        FixedFunctionOptimizer()
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

        virtual void apply(osg::Geode& geode)
        {
            for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
            {
                osg::Geometry *geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
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

        virtual void apply(osg::Group& node)
        {
            removeUnusedStateAttributes(node.getStateSet());
            NodeVisitor::apply(node);
        }

    protected:
        void removeUnusedStateAttributes(osg::StateSet* ssPtr)
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
        }   
    };

#ifdef __EMSCRIPTEN__
    struct WebFetcher : public osg::Referenced
    {
        WebFetcher() : fetch(NULL), done(false) {}
        emscripten_fetch_t* fetch; bool done;
        std::vector<char> buffer;

        bool httpGet(const std::string& uri)
        {
            // https://emscripten.org/docs/api_reference/fetch.html
            emscripten_fetch_attr_t attr;
            emscripten_fetch_attr_init(&attr);
            strcpy(attr.requestMethod, "GET");
            attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
            attr.onsuccess = WebFetcher::downloadSuccess;
            attr.onerror = WebFetcher::downloadFailure;
            attr.userData = this;

            emscripten_fetch(&attr, uri.c_str());
            while (!done) emscripten_sleep(10);
            return !buffer.empty();
        }

        static void downloadSuccess(emscripten_fetch_t* f)
        {
            WebFetcher* fr = (WebFetcher*)f->userData;
            char* ptr = (char*)&f->data[0]; fr->done = true;
            fr->buffer.assign(ptr, ptr + f->numBytes);
            emscripten_fetch_close(f);
        }

        static void downloadFailure(emscripten_fetch_t* f)
        {
            WebFetcher* fr = (WebFetcher*)f->userData;
            fr->done = true; fr->buffer.clear();
            emscripten_fetch_close(f);
        }
    };
#endif

}

#endif
