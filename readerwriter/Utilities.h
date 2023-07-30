#ifndef MANA_READERWRITER_UTILITIES_HPP
#define MANA_READERWRITER_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>
#ifdef __EMSCRIPTEN__
#   include <emscripten/fetch.h>
#   include <emscripten.h>
#endif

#if defined(VERSE_STATIC_BUILD)
#  define OSGVERSE_RW_EXPORT extern
#elif defined(VERSE_WINDOWS)
#  if defined(VERSE_RW_LIBRARY)
#    define OSGVERSE_RW_EXPORT   __declspec(dllexport)
#  else
#    define OSGVERSE_RW_EXPORT   __declspec(dllimport)
#  endif
#else
#  define OSGVERSE_RW_EXPORT extern
#endif

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT FixedFunctionOptimizer : public osg::NodeVisitor
    {
    public:
        FixedFunctionOptimizer()
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Node& node);

    protected:
        void removeUnusedStateAttributes(osg::StateSet* ssPtr);
    };

    class OSGVERSE_RW_EXPORT TextureOptimizer : public osg::NodeVisitor
    {
    public:
        TextureOptimizer()
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Node& node);

    protected:
        void applyTextureAttributes(osg::StateSet* ssPtr);
        void applyTexture(osg::Texture* tex, unsigned int unit);
        osg::Image* compressImage(osg::Texture* tex, osg::Image* img);
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
