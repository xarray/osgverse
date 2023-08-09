#ifndef MANA_READERWRITER_UTILITIES_HPP
#define MANA_READERWRITER_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>
#ifdef __EMSCRIPTEN__
#   include <emscripten/fetch.h>
#   include <emscripten.h>
#endif
#include "Export.h"

#ifndef GL_ARB_texture_rg
#define GL_RG                             0x8227
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
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
        TextureOptimizer(bool preparingForInlineFile = false,
                         const std::string& newTexFolder = "optimized_tex");
        virtual ~TextureOptimizer();

        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Node& node);

    protected:
        void applyTextureAttributes(osg::StateSet* ssPtr);
        void applyTexture(osg::Texture* tex, unsigned int unit);
        osg::Image* compressImage(osg::Texture* tex, osg::Image* img, bool toLoad);

        std::string _textureFolder;
        bool _preparingForInlineFile;
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
