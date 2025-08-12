#ifndef MANA_READERWRITER_UTILITIES_HPP
#define MANA_READERWRITER_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>
#include <osgDB/ReaderWriter>
#ifdef __EMSCRIPTEN__
#   include <emscripten/fetch.h>
#   include <emscripten.h>
extern void emscripten_advance();
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
    class OSGVERSE_RW_EXPORT EncodedFrameObject : public osg::Object
    {
    public:
        enum ImageType { FRAME_CUSTOMIZED = 0, FRAME_H264, FRAME_H265 };

        EncodedFrameObject();
        EncodedFrameObject(ImageType t, int w, int h, unsigned long long dts);
        EncodedFrameObject(const EncodedFrameObject& obj, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
        META_Object(osgVerse, EncodedFrameObject)

        void setImageType(ImageType t) { _type = t; }
        ImageType getImageType() const { return _type; }

        void setData(const std::vector<unsigned char>& d) { _data = d; }
        const std::vector<unsigned char>& getData() const { return _data; }
        std::vector<unsigned char>& getData() { return _data; }

        void setFrameWidth(unsigned int w) { _width = w; }
        void setFrameHeight(unsigned int h) { _height = h; }
        unsigned int getFrameWidth() const { return _width; }
        unsigned int getFrameHeight() const { return _height; }

        void setFrameStamp(unsigned long long s) { _framestamp = s; }
        void setDuration(unsigned long long s) { _duration = s; }
        unsigned long long getFrameStamp() const { return _framestamp; }
        unsigned long long getDuration() const { return _duration; }

    protected:
        std::vector<unsigned char> _data;
        unsigned long long _framestamp, _duration;
        unsigned int _width, _height;
        ImageType _type;
    };

    class OSGVERSE_RW_EXPORT FixedFunctionOptimizer : public osg::NodeVisitor
    {
    public:
        FixedFunctionOptimizer()
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _toRemoveShaders(false) {}
        void setRemovingOriginalShaders(bool b) { _toRemoveShaders = b; }

        virtual void apply(osg::Geometry& geom);
        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Node& node);

    protected:
        bool removeUnusedStateAttributes(osg::StateSet* ssPtr);
        std::vector<osg::ref_ptr<osg::StateAttribute>> _materialStack;
        bool _toRemoveShaders;
    };

    class OSGVERSE_RW_EXPORT TextureOptimizer : public osg::NodeVisitor
    {
    public:
        TextureOptimizer(bool saveAsInlineFile = false,
                         const std::string& newTexFolder = "optimized_tex");
        virtual ~TextureOptimizer();
        void deleteSavedTextures();

        void setOptions(osgDB::Options* op) { _ktxOptions = op; }
        osgDB::Options* getOptions() { return _ktxOptions.get(); }

        void setGeneratingMipmaps(bool b) { _generateMipmaps = b; }
        bool getGeneratingMipmaps() const { return _generateMipmaps; }

        virtual void apply(osg::Drawable& drawable);
        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Node& node);
        void applyTextureAttributes(osg::StateSet* ssPtr);

    protected:
        virtual void applyTexture(osg::Texture* tex, unsigned int unit);
        osg::Image* compressImage(osg::Texture* tex, osg::Image* img, bool toLoad);

        osg::ref_ptr<osgDB::Options> _ktxOptions;
        std::vector<std::string> _savedTextures;
        std::string _textureFolder;
        bool _saveAsInlineFile, _generateMipmaps;
    };

#ifdef __EMSCRIPTEN__
    struct OSGVERSE_RW_EXPORT WebFetcher : public osg::Referenced
    {
        WebFetcher() : status(0), done(false) {}
        std::vector<std::string> resHeaders;
        std::vector<char> buffer;
        int status; bool done;

        bool httpGet(const std::string& uri, const char* userName = NULL, const char* password = NULL,
                     const char* mimeType = NULL, const std::vector<std::string> requestHeaders = std::vector<std::string>())
        {
            // https://emscripten.org/docs/api_reference/fetch.html
            emscripten_fetch_attr_t attr;
            emscripten_fetch_attr_init(&attr); strcpy(attr.requestMethod, "GET");
            if (userName != NULL) attr.userName = userName;
            if (password != NULL) attr.password = password;
            if (mimeType != NULL) attr.overriddenMimeType = mimeType;
            if (!requestHeaders.empty())
            {
                std::vector<const char*> cRequestHeaders;
                cRequestHeaders.reserve(requestHeaders.size());
                for(size_t i = 0; i < requestHeaders.size(); ++i)
                    cRequestHeaders.push_back(requestHeaders[i].c_str());
                attr.requestHeaders = &cRequestHeaders[0];
            }

            attr.userData = this;
            attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
            attr.onreadystatechange = WebFetcher::stateChanged;
            attr.onsuccess = WebFetcher::downloadSuccess;
            attr.onerror = WebFetcher::downloadFailure;
            attr.onprogress = WebFetcher::emptyCallback;

            emscripten_fetch_t* f = emscripten_fetch(&attr, uri.c_str());
            while (!done) emscripten_advance();
            emscripten_fetch_close(f);
            return !buffer.empty();
        }

        static std::vector<std::string> getResponseHeaders(emscripten_fetch_t* f)
        {
            int headerSize = (int)emscripten_fetch_get_response_headers_length(f);
            std::string headerData; headerData.resize(headerSize + 1);
            if (headerSize <= 0) return std::vector<std::string>();
            emscripten_fetch_get_response_headers(f, (char*)headerData.data(), headerData.size());

            char** cHeaders = emscripten_fetch_unpack_response_headers(headerData.data());
            std::vector<std::string> headers; int ptr = 0; char* hValue = cHeaders[ptr];
            while (hValue != NULL) { headers.push_back(hValue); hValue = cHeaders[++ptr]; }
            emscripten_fetch_free_unpacked_response_headers(cHeaders); return headers;
        }

        static void downloadSuccess(emscripten_fetch_t* f)
        {
            WebFetcher* fr = (WebFetcher*)f->userData;
            char* ptr = (char*)&f->data[0]; fr->buffer.assign(ptr, ptr + f->numBytes);
            fr->status = f->status; fr->done = true;
        }

        static void downloadFailure(emscripten_fetch_t* f)
        {
            WebFetcher* fr = (WebFetcher*)f->userData;
            fr->status = f->status; fr->done = true; fr->buffer.clear();
        }

        static void stateChanged(emscripten_fetch_t* f)
        {
            WebFetcher* fr = (WebFetcher*)f->userData;
            if (f->readyState == /*HEADERS_RECEIVED*/2) fr->resHeaders = getResponseHeaders(f);
        }

        static void emptyCallback(emscripten_fetch_t* f) {}
    };
#endif

    /** Copy certain channel of an image to another image */
    OSGVERSE_RW_EXPORT bool copyImageChannel(osg::Image& src, int srcChannel, osg::Image& dst, int dstChannel);

    /** Handle ORM (Occlusion-Roughness-Metallic) texture creation */
    OSGVERSE_RW_EXPORT osg::Texture* constrctOcclusionRoughnessMetallic(osg::Texture* origin, osg::Texture* input,
                                                                        int chO, int chR, int chM);

    /** Generate mipmaps of given image */
    OSGVERSE_RW_EXPORT bool generateMipmaps(osg::Image& image, bool useKaiser);

    /** Convert RGB image to YUV */
    enum YUVFormat { YU12 = 0/*IYUV*/, YV12, NV12, NV21 };
    OSGVERSE_RW_EXPORT std::vector<std::vector<unsigned char>> convertRGBtoYUV(osg::Image* image, YUVFormat f = YV12);

    /** Encode data to base64 */
    OSGVERSE_RW_EXPORT std::string encodeBase64(const std::vector<unsigned char>& buffer);

    /** Decode base64 to data */
    OSGVERSE_RW_EXPORT std::vector<unsigned char> decodeBase64(const std::string& data);

    /** Encode string to URL style (e.g., '+' to %2B) */
    OSGVERSE_RW_EXPORT std::string urlEncode(const std::string& str);

    /** Decode string from URL style */
    OSGVERSE_RW_EXPORT std::string urlDecode(const std::string& str);

    /** Setup KTX trancoding flags */
    enum ReadingKtxFlag { ReadKtx_ToRGBA, ReadKtx_NoDXT };
    OSGVERSE_RW_EXPORT void setReadingKtxFlag(ReadingKtxFlag flag, int value);
}

#endif
