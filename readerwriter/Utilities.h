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
#include <functional>

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

struct ma_device;
struct ma_decoder;
struct AudioPlayingMixer;

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
        virtual ~FixedFunctionOptimizer();
        void setRemovingOriginalShaders(bool b) { _toRemoveShaders = b; }

        virtual void apply(osg::Geometry& geom);
        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Node& node);

    protected:
        bool removeUnusedStateAttributes(osg::StateSet* ssPtr);
        std::vector<osg::ref_ptr<osg::StateAttribute>> _materialStack;
        std::set<osg::ref_ptr<osg::StateSet>> _materialSets;
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
    OSGVERSE_RW_EXPORT osg::Texture* constructOcclusionRoughnessMetallic(osg::Texture* origin, osg::Texture* input,
                                                                         int chO, int chR, int chM);

    /** Convert image to compressed texture format */
    OSGVERSE_RW_EXPORT osg::Image* compressImage(osg::Image& img, osgDB::ReaderWriter* rw = NULL, bool forceDXT1 = false);

    /** Resize image using AVIR resizing algorithm */
    OSGVERSE_RW_EXPORT bool resizeImage(osg::Image& img, int rWidth, int rHeight, bool autoCompress = true);

    /** Generate mipmaps of given image */
    OSGVERSE_RW_EXPORT bool generateMipmaps(osg::Image& image, bool useKaiser);

    /** Convert RGB image to YUV */
    enum YUVFormat { YU12 = 0/*IYUV*/, YV12, NV12, NV21 };
    OSGVERSE_RW_EXPORT std::vector<std::vector<unsigned char>> convertRGBtoYUV(osg::Image* image, YUVFormat f = YV12);

    /** Some web-related helper functions and algorithms */
    struct OSGVERSE_RW_EXPORT WebAuxiliary
    {
        /** Encode data to base64 */
        static std::string encodeBase64(const std::vector<unsigned char>& buffer);

        /** Decode base64 to data */
        static std::vector<unsigned char> decodeBase64(const std::string& data);

        /** Encode string to URL style (e.g., '+' to %2B) */
        static std::string urlEncode(const std::string& str);

        /** Decode string from URL style */
        static std::string urlDecode(const std::string& str);

        /** Normalize input URL to replace ./ and ../ substrings to absolute paths */
        static std::string normalizeUrl(const std::string& url, const std::string& sep = "/");

        /* HTTP related enum and typedefs */
        enum HttpMethod { HTTP_DELETE = 0, HTTP_GET = 1, HTTP_HEAD = 2, HTTP_POST = 3, HTTP_PUT = 4 };
        typedef std::map<std::string, std::string> HttpRequestParams;
        typedef std::map<std::string, std::string> HttpRequestHeaders;
        typedef std::pair<int, std::string> HttpResponseData;
        typedef std::function<void (const std::string& /*url*/, const HttpRequestParams& /*paramsOrBody*/,
                                    const HttpRequestHeaders&, HttpResponseData&)> HttpCallback;

        /* TCP/UDP related enum and typedefs */
        enum SocketMethod { UDP_CLIENT = 0, UDP_SERVER, TCP_CLIENT, TCP_SERVER, WEBSOCKET_CLIENT };
        enum SocketState { UNCONNECTED = 0, CONNECTED, RECEIVED, WS_CONTINUE, WS_TEXT, WS_BINARY };
        typedef std::function<void (const std::string& /*ip*/, SocketState /*state*/,
                                    const std::vector<unsigned char>&)> SocketCallback;

        /** Set an HTTP client request (e.g., GET / POST) */
        static HttpResponseData httpRequest(const std::string& url, HttpMethod m, const std::string& body,
                                            const HttpRequestHeaders& headers = HttpRequestHeaders(), int timeout = 0);
        static osg::Referenced* httpRequestAsync(HttpCallback cb, const std::string& url, HttpMethod m, const std::string& body,
                                                 const HttpRequestHeaders& headers = HttpRequestHeaders(), int timeout = 0);

        /** Set an HTTP server */
        static osg::Referenced* httpServer(const std::map<std::string, HttpCallback>& getEntries,
                                           const std::map<std::string, HttpCallback>& postEntries,
                                           int port, const std::string& rootDir = "./", bool allowCORS = true);
        
        /** Set an HTTP + websocket server */
        static osg::Referenced* httpServerEx(const std::map<std::string, HttpCallback>& getEntries,
                                             const std::map<std::string, HttpCallback>& postEntries,
                                             int port, const std::string& rootDir, bool allowCORS, bool withWebsockets,
                                             SocketCallback readCB, SocketCallback joinCB);

        /** Set a TCP/UDP/WS socket to listen to messages */
        static osg::Referenced* socketListener(const std::string& host, int port, SocketMethod method,
                                               SocketCallback readCB, SocketCallback joinCB,
                                               const HttpRequestHeaders& wsHeaders = HttpRequestHeaders());

        /** Set a TCP/UDP/WS socket or websocket server for sending out messages */
        static int socketWriter(osg::Referenced* socketListenerOrWsServer, const std::string& target,
                                const std::vector<unsigned char>& data);
    };

    /** Compression helper functions and algorithms */
    struct OSGVERSE_RW_EXPORT CompressAuxiliary
    {
        enum CompressorType { ZIP };

        /** Create the archive handle */
        static osg::Referenced* createHandle(CompressorType type, std::istream& fin);

        /** Destroy the archive handle */
        static void destroyHandle(osg::Referenced* handle);

        /** List all files in the archive */
        static std::vector<std::string> listContents(osg::Referenced* handle);

        /** Extract specified file data from the archive */
        static std::vector<unsigned char> extract(osg::Referenced* handle, const std::string& fileName);
    };

    /** Audio playback interface */
    class OSGVERSE_RW_EXPORT AudioPlayer : public osg::Referenced
    {
    public:
        struct OSGVERSE_RW_EXPORT Clip : public osg::Referenced
        {
            enum State { STOPPED = 0, PLAYING, PAUSED } state;
            float volume; bool looping; struct ma_decoder* decoder;
            Clip() : state(STOPPED), volume(1.0f), looping(false), decoder(NULL) {}
        };
        static AudioPlayer* instance();

        bool addFile(const std::string& file, bool autoPlay, bool looping);
        bool removeFile(const std::string& file);
        Clip* getClip(const std::string& file);
        const Clip* getClip(const std::string& file) const;

        std::map<std::string, osg::ref_ptr<Clip>>& getClips() { return _clips; }
        const std::map<std::string, osg::ref_ptr<Clip>>& getClips() const { return _clips; }

    protected:
        AudioPlayer();
        virtual ~AudioPlayer();

        std::map<std::string, osg::ref_ptr<Clip>> _clips;
        struct ma_device* _device;
        struct AudioPlayingMixer* _mixer;
    };

    /** Load content from local file or network protocol */
    OSGVERSE_RW_EXPORT std::vector<unsigned char> loadFileData(
            const std::string& url, std::string& mimeType, std::string& encodingType,
            const std::vector<std::string>& reqHeaders = std::vector<std::string>());

    /** Get mimetype and extension map data */
    OSGVERSE_RW_EXPORT std::map<std::string, std::string> createMimeTypeMapper();

    /** Get OpenGL enum name and corresponding group/value pairs */
    OSGVERSE_RW_EXPORT std::map<std::string, std::pair<std::string, GLenum>> createGLEnumMapper();

    /** Setup KTX trancoding flags */
    enum ReadingKtxFlag { ReadKtx_ToRGBA, ReadKtx_NoDXT };
    OSGVERSE_RW_EXPORT void setReadingKtxFlag(ReadingKtxFlag flag, int value);
}

#endif
