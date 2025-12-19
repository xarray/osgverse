#include "3rdparty/libhv/all/client/requests.h"
#include "3rdparty/libhv/all/client/WebSocketClient.h"
#include "3rdparty/libhv/all/server/HttpServer.h"
#include "3rdparty/libhv/all/server/WebSocketServer.h"
#include "3rdparty/libhv/all/TcpClient.h"
#include "3rdparty/libhv/all/TcpServer.h"
#include "3rdparty/libhv/all/UdpClient.h"
#include "3rdparty/libhv/all/UdpServer.h"
#include "3rdparty/libhv/all/hasync.h"

#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/Geode>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Multisample>
#include <osg/Material>
#include <osg/PolygonOffset>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <sstream>
#include <iomanip>
#include <cctype>

#include <ghc/filesystem.hpp>
#include <nanoid/nanoid.h>
#include <libhv/all/client/requests.h>
#include <libhv/all/base64.h>
#include "modeling/Utilities.h"
#include "LoadTextureKTX.h"
#include "Utilities.h"
using namespace osgVerse;

static int char_from_hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A'; return 0;
}

class ModeChecker : public osg::Referenced
{
public:
    static ModeChecker* instance()
    {
        static osg::ref_ptr<ModeChecker> s_instance = new ModeChecker;
        return s_instance.get();
    }

    bool check(osg::StateSet& ss)
    {
        if (modeSet.empty()) return true;
        osg::StateSet::ModeList modeList = ss.getModeList();
        for (osg::StateSet::ModeList::iterator it = modeList.begin(); it != modeList.end(); ++it)
        { if (!check(it->first)) ss.removeMode(it->first); } return true;
    }

    bool check(GLenum mode)
    {
        if (modeSet.empty()) return true;
        return modeSet.find(mode) != modeSet.end();
    }

protected:
    ModeChecker()
    {
#if defined(OSG_GL3_AVAILABLE)
        // https://docs.gl/gl4/glEnable
        modeSet.insert(GL_BLEND);
        //modeSet.insert(GL_CLIP_DISTANCE);
        modeSet.insert(GL_COLOR_LOGIC_OP);
        modeSet.insert(GL_CULL_FACE);
        modeSet.insert(GL_DEBUG_OUTPUT);
        modeSet.insert(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        modeSet.insert(GL_DEPTH_CLAMP);
        modeSet.insert(GL_DEPTH_TEST);
        modeSet.insert(GL_DITHER);
        modeSet.insert(GL_FRAMEBUFFER_SRGB);
        modeSet.insert(GL_LINE_SMOOTH);
        modeSet.insert(GL_MULTISAMPLE_ARB);
        modeSet.insert(GL_POLYGON_OFFSET_FILL);
        modeSet.insert(GL_POLYGON_OFFSET_LINE);
        modeSet.insert(GL_POLYGON_OFFSET_POINT);
        modeSet.insert(GL_POLYGON_SMOOTH);
        modeSet.insert(GL_PRIMITIVE_RESTART);
        //modeSet.insert(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        modeSet.insert(GL_RASTERIZER_DISCARD);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_ONE_ARB);
        modeSet.insert(GL_SAMPLE_COVERAGE_ARB);
        //modeSet.insert(GL_SAMPLE_SHADING);
        //modeSet.insert(GL_SAMPLE_MASK);
        modeSet.insert(GL_SCISSOR_TEST);
        modeSet.insert(GL_STENCIL_TEST);
        modeSet.insert(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        modeSet.insert(GL_PROGRAM_POINT_SIZE);
#elif defined(OSG_GLES3_AVAILABLE)
        // https://docs.gl/es3/glEnable
        modeSet.insert(GL_BLEND);
        modeSet.insert(GL_CULL_FACE);
        modeSet.insert(GL_DEPTH_TEST);
        modeSet.insert(GL_DITHER);
        modeSet.insert(GL_POLYGON_OFFSET_FILL);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
        modeSet.insert(GL_SAMPLE_COVERAGE_ARB);
        modeSet.insert(GL_SCISSOR_TEST);
        modeSet.insert(GL_STENCIL_TEST);
        // https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/enable
        modeSet.insert(GL_RASTERIZER_DISCARD);
#  if !defined(VERSE_EMBEDDED_GLES3)
        //modeSet.insert(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        //modeSet.insert(GL_SAMPLE_MASK);
#  endif
#elif defined(OSG_GLES2_AVAILABLE)
        // https://docs.gl/es2/glEnable
        // https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/enable
        modeSet.insert(GL_BLEND);
        modeSet.insert(GL_CULL_FACE);
        modeSet.insert(GL_DEPTH_TEST);
        modeSet.insert(GL_DITHER);
        modeSet.insert(GL_POLYGON_OFFSET_FILL);
        modeSet.insert(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
        modeSet.insert(GL_SAMPLE_COVERAGE_ARB);
        modeSet.insert(GL_SCISSOR_TEST);
        modeSet.insert(GL_STENCIL_TEST);
#endif
    }

    std::set<GLenum> modeSet;
};

#ifdef __EMSCRIPTEN__
// Reference: https://github.com/emscripten-core/emscripten/issues/9574
EM_JS(void, emscripten_sleep_using_raf, (),
{
    Asyncify.handleSleep(wakeUp =>
    { requestAnimationFrame(wakeUp); });
});

void emscripten_advance()
{
#if 1
    emscripten_sleep_using_raf();
#else
    emscripten_sleep(10);
#endif
}
#endif

FixedFunctionOptimizer::~FixedFunctionOptimizer()
{
    for (std::set<osg::ref_ptr<osg::StateSet>>::iterator it = _materialSets.begin();
         it != _materialSets.end(); ++it) (*it)->removeAttribute(osg::StateAttribute::MATERIAL);
}

void FixedFunctionOptimizer::apply(osg::Geometry& geom)
{
    bool added = removeUnusedStateAttributes(geom.getStateSet());
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
    if (va && !va->empty() && !_materialStack.empty())
    {
        // Change material to color array
        osg::Material* mtl = static_cast<osg::Material*>(_materialStack.back().get());
        osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
        ca->assign(va->size(), mtl->getDiffuse(osg::Material::FRONT));
        geom.setColorArray(ca.get()); geom.setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    optimizeIndices(geom);
    geom.setUseDisplayList(false);
    geom.setUseVertexBufferObjects(true);
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    traverse(geom);
#endif
    if (added) _materialStack.pop_back();
}

void FixedFunctionOptimizer::apply(osg::Geode& geode)
{
    bool added = removeUnusedStateAttributes(geode.getStateSet());
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
    {
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
        if (geom) apply(*geom);
    }
#endif
    NodeVisitor::apply(geode);
    if (added) _materialStack.pop_back();
}

void FixedFunctionOptimizer::apply(osg::Node& node)
{
    bool added = removeUnusedStateAttributes(node.getStateSet());
    NodeVisitor::apply(node);
    if (added) _materialStack.pop_back();
}

bool FixedFunctionOptimizer::removeUnusedStateAttributes(osg::StateSet* ssPtr)
{
    if (ssPtr == NULL) return false;
    osg::StateSet& ss = *ssPtr;

    osg::StateAttribute* sa = ss.getAttribute(osg::StateAttribute::MATERIAL);
    if (sa != NULL) { _materialStack.push_back(sa); _materialSets.insert(ssPtr); }

    ss.removeAttribute(osg::StateAttribute::ALPHAFUNC);
    ss.removeAttribute(osg::StateAttribute::CLIPPLANE);
    ss.removeAttribute(osg::StateAttribute::COLORMATRIX);
    ss.removeAttribute(osg::StateAttribute::FOG);
    ss.removeAttribute(osg::StateAttribute::LIGHT);
    ss.removeAttribute(osg::StateAttribute::LIGHTMODEL);
    ss.removeAttribute(osg::StateAttribute::LINESTIPPLE);
    ss.removeAttribute(osg::StateAttribute::LOGICOP);
    //ss.removeAttribute(osg::StateAttribute::MATERIAL);  // don't remove here, considering shared statesets
    ss.removeAttribute(osg::StateAttribute::POINT);
    ss.removeAttribute(osg::StateAttribute::POLYGONSTIPPLE);
    ss.removeAttribute(osg::StateAttribute::SHADEMODEL);
    if (_toRemoveShaders) ss.removeAttribute(osg::StateAttribute::PROGRAM);

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    osg::StateSet::TextureAttributeList texAttrs = ss.getTextureAttributeList();
    for (size_t i = 0; i < texAttrs.size(); ++i)
    {
        osg::Texture* tex = static_cast<osg::Texture*>(
            ss.getTextureAttribute(i, osg::StateAttribute::TEXTURE));
        if (tex && tex->getNumImages() > 0)
        {
            // Try to fix some old and wrong internal formats
            for (size_t j = 0; j < tex->getNumImages(); ++j)
            {
                osg::Image* img = tex->getImage(j); if (!img) continue;
                if (img->isCompressed() && !img->isMipmap())
                {
#  if defined(VERSE_EMBEDDED_GLES2) || defined(VERSE_EMBEDDED_GLES3)
                    // No mipmapping for compressed format
                    switch (tex->getFilter(osg::Texture::MIN_FILTER))
                    {
                    case osg::Texture::NEAREST_MIPMAP_LINEAR: case osg::Texture::NEAREST_MIPMAP_NEAREST:
                        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST); break;
                    case osg::Texture::LINEAR_MIPMAP_LINEAR: case osg::Texture::LINEAR_MIPMAP_NEAREST:
                        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR); break;
                    default: break;
                    }
                    tex->setUseHardwareMipMapGeneration(false);
#  endif
                }

#  if !defined(VERSE_EMBEDDED_GLES2)
                GLenum internalFmt = img->getInternalTextureFormat();
                switch (internalFmt)
                {
                case GL_ALPHA: img->setInternalTextureFormat(GL_ALPHA8); break;
                case GL_LUMINANCE: img->setInternalTextureFormat(GL_LUMINANCE8); break;
                case GL_RGB: img->setInternalTextureFormat(GL_RGB8); break;
                case GL_RGBA: img->setInternalTextureFormat(GL_RGBA8); break;
                }
#  endif
            }
        }
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXENV);
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXGEN);
        ss.removeTextureAttribute(i, osg::StateAttribute::TEXMAT);
    }

    // Remove texture modes as they are not needed by glEnable() in GLES 2.0/3.x and GL3/4
    // https://docs.gl/es2/glEnable  // https://docs.gl/gl4/glEnable
    osg::StateSet::TextureModeList texModes = ss.getTextureModeList();
    for (size_t i = 0; i < texModes.size(); ++i)
    {
        osg::StateSet::ModeList modes = texModes[i];
        for (osg::StateSet::ModeList::const_iterator itr = modes.begin();
            itr != modes.end(); ++itr) ss.removeTextureMode(i, itr->first);
    }

    // Remove unused GL modes
    ModeChecker::instance()->check(ss);
#endif  // defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    return (sa != NULL);
}

TextureOptimizer::TextureOptimizer(bool inlineFile, const std::string& newTexFolder)
:   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
    if (inlineFile) osgDB::makeDirectory(newTexFolder);
    _textureFolder = newTexFolder;
    _saveAsInlineFile = inlineFile;
    _generateMipmaps = false;
    _ktxOptions = new osgDB::Options("UseBASISU=1");
}

TextureOptimizer::~TextureOptimizer()
{
}

void TextureOptimizer::deleteSavedTextures()
{
    try
    {
        for (size_t i = 0; i < _savedTextures.size(); ++i)
        {
            ghc::filesystem::path path = _savedTextures[i];
            ghc::filesystem::remove_all(path);
        }
        ghc::filesystem::remove_all(_textureFolder);
    }
    catch (std::runtime_error& err)
    {
        OSG_WARN << "[TextureOptimizer] deleteSavedTextures(): " << err.what() << std::endl;
    }
}

void TextureOptimizer::apply(osg::Drawable& drawable)
{
    applyTextureAttributes(drawable.getStateSet());
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    traverse(drawable);
#endif
}

void TextureOptimizer::apply(osg::Geode& geode)
{
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
    for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        applyTextureAttributes(geode.getDrawable(i)->getStateSet());
#endif
    applyTextureAttributes(geode.getStateSet());
    NodeVisitor::apply(geode);
}

void TextureOptimizer::apply(osg::Node& node)
{
    applyTextureAttributes(node.getStateSet());
    NodeVisitor::apply(node);
}

void TextureOptimizer::applyTextureAttributes(osg::StateSet* ssPtr)
{
    if (ssPtr == NULL) return;
    osg::StateSet& ss = *ssPtr;

    const osg::StateSet::TextureAttributeList& texAttrs = ss.getTextureAttributeList();
    for (size_t i = 0; i < texAttrs.size(); ++i)
    {
        const osg::StateSet::AttributeList& attrs = texAttrs[i];
        for (osg::StateSet::AttributeList::const_iterator itr = attrs.begin();
             itr != attrs.end(); ++itr)
        {
            if (itr->first.first == osg::StateAttribute::TEXTURE)
                applyTexture(static_cast<osg::Texture*>(itr->second.first.get()), i);
        }
    }
}

void TextureOptimizer::applyTexture(osg::Texture* tex, unsigned int unit)
{
    osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(tex);
    if (tex2D && tex2D->getImage())
    {
        // Copy to original image as it may be shared by other textures
        osg::ref_ptr<osg::Image> image0 = tex2D->getImage();
        osg::ref_ptr<osg::Image> image1 = compressImage(tex, image0.get(), !_saveAsInlineFile);
        if (!image1 || (image1.valid() && !image1->valid())) return;
        image0->allocateImage(image1->s(), image1->t(), image1->r(),
                              image1->getPixelFormat(), image1->getDataType(),
                              image1->getPacking());
        image0->setInternalTextureFormat(image1->getInternalTextureFormat());
        memcpy(image0->data(), image1->data(), image1->getTotalSizeInBytes());
    }
}

osg::Image* TextureOptimizer::compressImage(osg::Texture* tex, osg::Image* img, bool toLoad)
{
    std::stringstream ss; if (!img->valid()) return NULL;
    if (img->isCompressed()) return NULL;
    if (img->getFileName().empty()) img->setFileName(nanoid::generate(8) + ".png");
    if (img->getFileName().find("verse_ktx") != std::string::npos) return NULL;
    if ((img->s() < 4 || img->t() < 4)) return NULL;

    if (_generateMipmaps && !img->isMipmap())
    {
        img->ensureValidSizeForTexturing(2048);
        osgVerse::generateMipmaps(*img, false);
    }

    int w = osg::Image::computeNearestPowerOfTwo(img->s());
    int h = osg::Image::computeNearestPowerOfTwo(img->t());
    if (w != img->s() || h != img->t()) img->scaleImage(w, h, 1);

    switch (img->getInternalTextureFormat())
    {
    case GL_LUMINANCE: case 1: img->setInternalTextureFormat(GL_R8); break;
    case GL_LUMINANCE_ALPHA: case 2: img->setInternalTextureFormat(GL_RG8); break;
    case GL_RGB: case 3: img->setInternalTextureFormat(GL_RGB8); break;
    case GL_RGBA: case 4: img->setInternalTextureFormat(GL_RGBA8); break;
    default: break;
    }

    std::vector<osg::Image*> images; images.push_back(img);
    if (!saveKtx2(ss, false, _ktxOptions.get(), images)) return NULL;
    else OSG_NOTICE << "[TextureOptimizer] Compressed: " << img->getFileName()
                    << " (" << img->s() << " x " << img->t() << ")" << std::endl;

    if (!toLoad)
    {
        std::string fileName = img->getFileName(), id = "__" + nanoid::generate(8);
        if (fileName.empty()) fileName = "temp" + id + ".ktx";
        else fileName = osgDB::getStrippedName(fileName) + id + ".ktx";
        fileName = _textureFolder + osgDB::getNativePathSeparator() + fileName;
        img->setFileName(fileName + ".verse_ktx");

        std::ofstream out(fileName.c_str(), std::ios::out | std::ios::binary);
        out.write(ss.str().data(), ss.str().size());
        _savedTextures.push_back(fileName); return NULL;
    }
    else
    {
        std::vector<osg::ref_ptr<osg::Image>> outImages = loadKtx2(ss, _ktxOptions.get());
        return outImages.empty() ? NULL : outImages[0].release();
    }
}

EncodedFrameObject::EncodedFrameObject()
:   _framestamp(0), _duration(0), _width(0), _height(0), _type(FRAME_CUSTOMIZED) {}

EncodedFrameObject::EncodedFrameObject(ImageType t, int w, int h, unsigned long long dts)
:   _framestamp(dts), _duration(0), _width(w), _height(h), _type(t) {}

EncodedFrameObject::EncodedFrameObject(const EncodedFrameObject& obj, const osg::CopyOp& op)
:   osg::Object(obj, op), _data(obj._data), _framestamp(obj._framestamp), _duration(obj._duration),
    _width(obj._width), _height(obj._height), _type(obj._type) {}

/// WebAuxiliary ///
namespace
{
    std::istream& getline_ex(std::istream& is, std::string& str)
    {
        str.clear(); char c = 0;
        while (is.get(c))
        {
            if (c == '\\' || c == '/') break;
            str += c;
        }
        return is;
    }

    struct HttpClientInstance : public osg::Referenced
    {
        hv::HttpClient client;
    };

    struct HttpServerInstance : public osg::Referenced
    {
        bool withWebsockets;
        hv::HttpServer server;
        hv::WebSocketServer serverEx;
        hv::HttpService router;
        hv::WebSocketService ws;
        std::map<std::string, WebSocketChannelPtr> wsChannels;
        HttpServerInstance(bool w) { withWebsockets = w; }

        virtual ~HttpServerInstance()
        {
            if (withWebsockets) serverEx.stop(); else server.stop();
            hv::async::cleanup();
        }
    };

    struct SocketInstance : public osg::Referenced
    {
        WebAuxiliary::SocketMethod method;
        hv::TcpClient tcpClient; hv::TcpServer tcpServer;
        hv::UdpClient udpClient; hv::UdpServer udpServer;
        hv::WebSocketClient wsClient;
        SocketInstance(WebAuxiliary::SocketMethod m) { method = m; }

        virtual ~SocketInstance()
        {
            switch (method)
            {
            case WebAuxiliary::TCP_CLIENT: tcpClient.stop(); tcpClient.closesocket(); break;
            case WebAuxiliary::TCP_SERVER: tcpServer.stop(); tcpServer.closesocket(); break;
            case WebAuxiliary::UDP_CLIENT: udpClient.stop(); udpClient.closesocket(); break;
            case WebAuxiliary::UDP_SERVER: udpServer.stop(); udpServer.closesocket(); break;
            case WebAuxiliary::WEBSOCKET_CLIENT: wsClient.stop(); wsClient.closesocket(); break;
            }
        }
    };
}

std::string WebAuxiliary::encodeBase64(const std::vector<unsigned char>& buffer)
{ return buffer.empty() ? "" : hv::Base64Encode(&buffer[0], buffer.size()); }

std::vector<unsigned char> WebAuxiliary::decodeBase64(const std::string& data)
{
    std::string result = hv::Base64Decode(data.data(), data.size());
    std::vector<unsigned char> out(result.size()); if (result.empty()) return out;
    memcpy(&out[0], result.data(), result.size()); return out;
}

std::string WebAuxiliary::urlEncode(const std::string& str)
{
    std::ostringstream oss;
    for (size_t i = 0; i < str.length(); ++i)
    {
        char c = str[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~')
        { oss << c; }
        else
        {
            oss << std::uppercase << '%' << std::setw(2) << std::setfill('0')
                << std::hex << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return oss.str();
}

std::string WebAuxiliary::urlDecode(const std::string& str)
{
    std::string decoded;
    for (size_t i = 0; i < str.length(); ++i)
    {
        char c = str[i];
        if (c == '%' && i + 2 < str.length())
        {
            char hex1 = std::toupper(str[i + 1]);
            char hex2 = std::toupper(str[i + 2]);
            if ((hex1 >= '0' && hex1 <= '9') || (hex1 >= 'A' && hex1 <= 'F') &&
                (hex2 >= '0' && hex2 <= '9') || (hex2 >= 'A' && hex2 <= 'F'))
            {
                char code = char_from_hex(hex1) * 16 + char_from_hex(hex2);
                decoded += code; i += 2;
            }
            else decoded += c;
        }
        else decoded += c;
    }
    return decoded;
}

std::string WebAuxiliary::normalizeUrl(const std::string& url, const std::string& sep)
{
    size_t pathStart = url.find("://");
    if (pathStart == std::string::npos) pathStart = 0;
    else pathStart += 3;

    std::string path = url.substr(pathStart), part;
    std::istringstream iss(path); std::vector<std::string> parts;
    while (getline_ex(iss, part)) { if (!part.empty()) parts.push_back(part); }
    if (!part.empty()) parts.push_back(part);

    std::vector<std::string> normalizedParts;
    for (const auto& p : parts)
    {
        if (p == "..") { if (!normalizedParts.empty()) normalizedParts.pop_back(); }
        else if (p != ".") normalizedParts.push_back(p);
    }

    std::ostringstream oss;
    for (size_t i = 0; i < normalizedParts.size(); ++i)
    { if (i > 0) oss << sep; oss << normalizedParts[i]; }
    return url.substr(0, pathStart) + oss.str();
}

WebAuxiliary::HttpResponseData WebAuxiliary::httpRequest(const std::string& url, HttpMethod method, const std::string& body,
                                                         const HttpRequestHeaders& headers, int timeout)
{
    HttpRequestPtr req = std::make_shared<HttpRequest>();
    req->method = (http_method)method; req->url = url;
    req->body = body; if (timeout > 0) req->timeout = timeout;
    for (HttpRequestHeaders::const_iterator it = headers.begin(); it != headers.end(); ++it)
        req->headers[it->first] = it->second;

    hv::HttpClient client; HttpResponse res;
    int ret = client.send(req.get(), &res);
    if (ret != 0) return HttpResponseData(-1, "[WebAuxiliary] HTTP request failed");
    else return HttpResponseData(res.status_code, res.body);
}

osg::Referenced* WebAuxiliary::httpRequestAsync(HttpCallback cb, const std::string& url, HttpMethod method,
                                                const std::string& body, const HttpRequestHeaders& headers, int timeout)
{
    HttpRequestPtr req = std::make_shared<HttpRequest>();
    req->method = (http_method)method; req->url = url;
    req->body = body; if (timeout > 0) req->timeout = timeout;
    for (HttpRequestHeaders::const_iterator it = headers.begin(); it != headers.end(); ++it)
        req->headers[it->first] = it->second;

    osg::ref_ptr<HttpClientInstance> instance = new HttpClientInstance;
    instance->client.sendAsync(req, [cb, url](const HttpResponsePtr& res)
    {
        if (!res)
        {
            HttpResponseData data(-1, "[WebAuxiliary] HTTP request failed");
            cb(url, HttpRequestParams(), HttpRequestHeaders(), data);
        }
        else
        {
            HttpRequestHeaders resHeaders;
            for (http_headers::iterator it = res->headers.begin(); it != res->headers.end(); ++it)
                resHeaders[it->first] = it->second;
            HttpResponseData data(res->status_code, res->body);
            cb(url, HttpRequestParams(), resHeaders, data);
        }
    });
    return instance.release();
}

osg::Referenced* WebAuxiliary::httpServer(const std::map<std::string, WebAuxiliary::HttpCallback>& getEntries,
                                          const std::map<std::string, WebAuxiliary::HttpCallback>& postEntries,
                                          int port, const std::string& rootDir, bool allowCORS)
{ return httpServerEx(getEntries, postEntries, port, rootDir, allowCORS, false, NULL, NULL); }

osg::Referenced* WebAuxiliary::httpServerEx(const std::map<std::string, HttpCallback>& getEntries,
                                            const std::map<std::string, HttpCallback>& postEntries,
                                            int port, const std::string& rootDir, bool allowCORS,
                                            bool withWebsockets, SocketCallback readCB, SocketCallback joinCB)
{
    osg::ref_ptr<HttpServerInstance> instance = new HttpServerInstance(withWebsockets);
    instance->router.Static("/", rootDir.c_str());
    for (std::map<std::string, WebAuxiliary::HttpCallback>::const_iterator it = getEntries.begin();
         it != getEntries.end(); ++it)
    {
        const std::string& url = it->first; const WebAuxiliary::HttpCallback& cb = it->second;
        instance->router.GET(url.c_str(), [cb](HttpRequest* req, HttpResponse* res)
        {
            HttpResponseData resData; HttpRequestHeaders headers;
            headers.insert(req->headers.begin(), req->headers.end());
            cb(req->url, req->query_params, headers, resData); return res->String(resData.second);
        });
    }

    for (std::map<std::string, WebAuxiliary::HttpCallback>::const_iterator it = postEntries.begin();
         it != postEntries.end(); ++it)
    {
        const std::string& url = it->first; const WebAuxiliary::HttpCallback& cb = it->second;
        instance->router.POST(url.c_str(), [cb](HttpRequest* req, HttpResponse* res)
        {
            HttpResponseData resData; HttpRequestParams bodyData; HttpRequestHeaders headers;
            bodyData.insert(req->query_params.begin(), req->query_params.end()); bodyData[""] = req->body;
            headers.insert(req->headers.begin(), req->headers.end());
            cb(req->url, bodyData, headers, resData); return res->String(resData.second);
        });
    }

    if (withWebsockets)
    {
        instance->ws.onopen = [instance, joinCB](const WebSocketChannelPtr& ch, const HttpRequestPtr& req)
        {
            std::vector<unsigned char> empty; std::string addr = ch->peeraddr();
            if (joinCB != NULL) joinCB(addr, UNCONNECTED, empty);
            instance->wsChannels[addr] = ch;
        };
        instance->ws.onclose = [instance, joinCB](const WebSocketChannelPtr& ch)
        {
            std::vector<unsigned char> empty; std::string addr = ch->peeraddr();
            if (joinCB != NULL) joinCB(addr, CONNECTED, empty);

            std::map<std::string, WebSocketChannelPtr>::iterator it = instance->wsChannels.find(addr);
            if (it != instance->wsChannels.end()) instance->wsChannels.erase(it);
        };
        instance->ws.onmessage = [readCB](const WebSocketChannelPtr& ch, const std::string& msg)
        {
            const char* s = msg.data(); std::vector<unsigned char> data(s, s + msg.size());
            if (readCB != NULL) readCB(ch->peeraddr(), SocketState(WS_CONTINUE + ch->opcode), data);
        };

        instance->serverEx.ws = &(instance->ws);
        instance->serverEx.service = &(instance->router);
        instance->serverEx.port = port; instance->serverEx.start();
    }
    else
    {
        instance->server.service = &(instance->router);
        instance->server.port = port; instance->server.start();
    }
    return instance.release();
}

osg::Referenced* WebAuxiliary::socketListener(const std::string& host, int port, SocketMethod method,
                                              SocketCallback readCB, SocketCallback joinCB, const HttpRequestHeaders& wsHeaders)
{
#define CREATESOCKET(socket, H, P) \
    int fd = H.empty() ? (socket).createsocket(P) : (socket).createsocket(P, H.c_str()); \
    if (fd < 0) { OSG_WARN << "[WebAuxiliary] Failed to create socket: " << H << ":" << P << "\n"; return NULL; }

    osg::ref_ptr<SocketInstance> instance = new SocketInstance(method);
    if (method == TCP_CLIENT)
    {
        CREATESOCKET(instance->tcpClient, host, port);
        instance->tcpClient.onConnection = [joinCB](const hv::SocketChannelPtr& ch)
        {
            std::vector<unsigned char> empty; if (joinCB == NULL) return;
            if (ch->isConnected()) joinCB(ch->peeraddr(), CONNECTED, empty);
            else joinCB(ch->peeraddr(), UNCONNECTED, empty);
        };
        instance->tcpClient.onMessage = [readCB](const hv::SocketChannelPtr& ch, hv::Buffer* buf)
        {
            char* s = (char*)buf->data(); std::vector<unsigned char> data(s, s + buf->size());
            if (readCB != NULL) readCB(ch->peeraddr(), RECEIVED, data);
        };

        reconn_setting_t reconn; reconn_setting_init(&reconn);
        reconn.min_delay = 1000; reconn.max_delay = 10000; reconn.delay_policy = 2;
        instance->tcpClient.setReconnect(&reconn); instance->tcpClient.start();
    }
    else if (method == TCP_SERVER)
    {
        CREATESOCKET(instance->tcpServer, host, port);
        instance->tcpServer.onConnection = [joinCB](const hv::SocketChannelPtr& ch)
        {
            std::vector<unsigned char> empty; if (joinCB == NULL) return;
            if (ch->isConnected()) joinCB(ch->peeraddr(), CONNECTED, empty);
            else joinCB(ch->peeraddr(), UNCONNECTED, empty);
        };
        instance->tcpServer.onMessage = [readCB](const hv::SocketChannelPtr& ch, hv::Buffer* buf)
        {
            char* s = (char*)buf->data(); std::vector<unsigned char> data(s, s + buf->size());
            if (readCB != NULL) readCB(ch->peeraddr(), RECEIVED, data);
        };

        instance->tcpServer.setThreadNum(4);
        instance->tcpServer.setLoadBalance(LB_LeastConnections);
        instance->tcpServer.start();
    }
    else if (method == UDP_CLIENT)
    {
        CREATESOCKET(instance->udpClient, host, port);
        instance->udpClient.onMessage = [readCB](const hv::SocketChannelPtr& ch, hv::Buffer* buf)
        {
            char* s = (char*)buf->data(); std::vector<unsigned char> data(s, s + buf->size());
            if (readCB != NULL) readCB(ch->peeraddr(), RECEIVED, data);
        };
        instance->udpClient.start();
    }
    else if (method == UDP_SERVER)
    {
        CREATESOCKET(instance->udpServer, host, port);
        instance->udpServer.onMessage = [readCB](const hv::SocketChannelPtr& ch, hv::Buffer* buf)
        {
            char* s = (char*)buf->data(); std::vector<unsigned char> data(s, s + buf->size());
            if (readCB != NULL) readCB(ch->peeraddr(), RECEIVED, data);
        };
        instance->udpServer.start();
    }
    else if (method == WEBSOCKET_CLIENT)
    {
        instance->wsClient.onopen = [host, joinCB]()
        { std::vector<unsigned char> empty; if (joinCB != NULL) joinCB(host, CONNECTED, empty); };
        instance->wsClient.onclose = [host, joinCB]()
        { std::vector<unsigned char> empty; if (joinCB != NULL) joinCB(host, UNCONNECTED, empty); };
        instance->wsClient.onmessage = [instance, host, readCB](const std::string& msg)
        {
            const char* s = msg.data(); std::vector<unsigned char> data(s, s + msg.size());
            if (readCB != NULL) readCB(host, SocketState(WS_CONTINUE + instance->wsClient.opcode()), data);
        };

        reconn_setting_t reconn; reconn_setting_init(&reconn);
        reconn.min_delay = 1000; reconn.max_delay = 10000; reconn.delay_policy = 2;
        instance->wsClient.setReconnect(&reconn);

        http_headers headers; headers.insert(wsHeaders.begin(), wsHeaders.end());
        instance->wsClient.open(host.c_str(), headers);
    }
    return instance.release();
}

int WebAuxiliary::socketWriter(osg::Referenced* socket, const std::string& target,
                               const std::vector<unsigned char>& data)
{
    int result = -1; if (!socket || data.empty()) return result;
    SocketInstance* instance = dynamic_cast<SocketInstance*>(socket);
    if (!instance)
    {
        HttpServerInstance* httpInstance = dynamic_cast<HttpServerInstance*>(socket);
        if (httpInstance && httpInstance->withWebsockets)
        {
            for (std::map<std::string, WebSocketChannelPtr>::iterator it = httpInstance->wsChannels.begin();
                 it != httpInstance->wsChannels.end(); ++it)
            {
                if (it->first.find(target) != std::string::npos && it->second.get())
                    result += it->second->send((char*)data.data(), data.size(), WS_OPCODE_BINARY);
            }
        }
        return result;
    }

    if (instance->method == TCP_CLIENT)
    {
        if (!instance->tcpClient.isConnected()) { OSG_WARN << "[WebAuxiliary] Socket not connected\n"; return 0; }
        return instance->tcpClient.send(data.data(), data.size());
    }
    else if (instance->method == TCP_SERVER)
    {
        int result = 0; int* resultPtr = &result;
        if (target.empty()) return instance->tcpServer.broadcast(data.data(), data.size());
        instance->tcpServer.foreachChannel([target, data, resultPtr](const hv::SocketChannelPtr& ch)
        {
            std::string peerAddr = ch->peeraddr();
            if (peerAddr.find(target) != std::string::npos)
                *resultPtr += ch->write(data.data(), data.size());
        }); return result;
    }
    else if (instance->method == UDP_CLIENT)
    {
        if (target.empty()) return instance->udpClient.sendto(data.data(), data.size());
        sockaddr_u peerAddr; if (ResolveAddr(target.c_str(), &peerAddr) != 0) return 0;
        return instance->udpClient.sendto(data.data(), data.size(), &(peerAddr.sa));
    }
    else if (instance->method == UDP_SERVER)
    {
        if (target.empty()) return instance->udpServer.sendto(data.data(), data.size());
        sockaddr_u peerAddr; if (ResolveAddr(target.c_str(), &peerAddr) != 0) return 0;
        return instance->udpServer.sendto(data.data(), data.size(), &(peerAddr.sa));
    }
    else if (instance->method == WEBSOCKET_CLIENT)
    {
        if (!instance->wsClient.isConnected()) { OSG_WARN << "[WebAuxiliary] Socket not connected\n"; return 0; }
        return instance->wsClient.send((char*)data.data(), data.size(), WS_OPCODE_BINARY);
    }
    return -1;
}
