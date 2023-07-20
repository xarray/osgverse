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

        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Group& node);

    protected:
        void removeUnusedStateAttributes(osg::StateSet* ssPtr);
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
