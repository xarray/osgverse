#include <osg/io_utils>
#include <osg/UserDataContainer>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/ImageStream>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/Archive>

#include "pipeline/Global.h"
#include "pipeline/CudaTexture2D.h"
#include "readerwriter/Utilities.h"
#include <mk_mediakit.h>
#include <chrono>
#include <queue>

/* To make MediaServer automatically forward messages from the RCTP data channel to pushers,
*  we can also try to modify ZLMediaKit source code:
*  See https://github.com/ZLMediaKit/ZLMediaKit/issues/3963 for details:
*  webrtc/WebRtcPusher.h
     #ifdef ENABLE_SCTP
     void OnSctpAssociationMessageReceived(RTC::SctpAssociation *sctpAssociation, uint16_t streamId, uint32_t ppid,
                                           const uint8_t *msg, size_t len) override;
     #endif
* webrtc/WebRtcPusher.cpp
     #ifdef ENABLE_SCTP
     void WebRtcPusher::OnSctpAssociationMessageReceived(RTC::SctpAssociation *sctpAssociation, uint16_t streamId,
                                                         uint32_t ppid, const uint8_t *msg, size_t len) {
        if (_push_src) {
           toolkit::Any any; Buffer::Ptr buffer = std::make_shared<BufferLikeString>(std::string((char *)msg, len));
           any.set(std::move(buffer)); _push_src->getRing()->sendMessage(any); }
     }
     #endif
*/

// for webrtc answer sdp
static char* escape_string(const char* ptr)
{
    char* escaped = (char*)malloc(2 * strlen(ptr));
    char* ptr_escaped = escaped;
    while (1)
    {
        switch (*ptr)
        {
        case '\r':
            *(ptr_escaped++) = '\\';
            *(ptr_escaped++) = 'r';
            break;
        case '\n':
            *(ptr_escaped++) = '\\';
            *(ptr_escaped++) = 'n';
            break;
        case '\t':
            *(ptr_escaped++) = '\\';
            *(ptr_escaped++) = 't';
            break;
        default:
            *(ptr_escaped++) = *ptr;
            if (!*ptr) return escaped;
            break;
        }
        ++ptr;
    }
    return NULL;
}

class ZLMediaServerArchive;
osg::observer_ptr<ZLMediaServerArchive> g_server;

class ZLMediaResourceDemuxer : public osgVerse::CudaResourceReaderBase::Demuxer
{
public:
    ZLMediaResourceDemuxer() : _width(0), _height(0), _type(osgVerse::CODEC_INVALID) {}

    void setTrackInfo(int codec, int w, int h)
    {
        if (codec == MKCodecH264) _type = osgVerse::CODEC_H264;
        else if (codec == MKCodecH265) _type = osgVerse::CODEC_HEVC;
        else if (codec == MKCodecVP8) _type = osgVerse::CODEC_VP8;
        else if (codec == MKCodecVP9) _type = osgVerse::CODEC_VP9;
        else if (codec == MKCodecAV1) _type = osgVerse::CODEC_AV1;
        else if (codec == MKCodecJPEG) _type = osgVerse::CODEC_JPEG;
        _width = w; _height = h;
    }

    void addFrame(mk_frame frame)
    {
        const char* data = mk_frame_get_data(frame);
        size_t prefix = mk_frame_get_data_prefix_size(frame);
        size_t size = mk_frame_get_data_size(frame);
        long long pts = mk_frame_get_pts(frame);

        _mutex.lock();
        _frames.push(std::vector<unsigned char>(data, data + size));
        _frameTimes.push(pts);
        _mutex.unlock();
    }

    virtual osgVerse::VideoCodecType getVideoCodec() { return _type; }
    virtual int getWidth() { return _width; }
    virtual int getHeight() { return _height; }

    virtual bool demux(unsigned char** videoData, int* videoBytes, long long* pts)
    {
        bool hasData = false;
        _mutex.lock();
        hasData = !_frames.empty();
        if (hasData)
        {
            if (pts) *pts = _frameTimes.front(); _frameTimes.pop();
            _lastFrame = _frames.front(); _frames.pop();
        }
        _mutex.unlock();

        *videoData = _lastFrame.data(); *videoBytes = (int)_lastFrame.size();
        return hasData;
    }

protected:
    std::queue<std::vector<unsigned char>> _frames;
    std::queue<long long> _frameTimes;
    std::vector<unsigned char> _lastFrame;

    int _width, _height;
    osgVerse::VideoCodecType _type;
    std::mutex _mutex;
};

class ZLMediaServerArchive : public osgDB::Archive
{
public:
    ZLMediaServerArchive(const osgDB::Options* options);
    virtual ~ZLMediaServerArchive() { close(); g_server = NULL; }

    virtual const char* libraryName() const { return "osgVerse"; }
    virtual const char* className() const { return "ZLMediaServerArchive"; }
    virtual bool acceptsExtension(const std::string& /*ext*/) const { return true; }

    virtual void close() { mk_stop_all_server(); }
    virtual bool fileExists(const std::string& filename) const { return false; }
    virtual std::string getArchiveFileName() const { return std::string(); }
    virtual std::string getMasterFileName() const { return std::string(); }

    virtual osgDB::FileType getFileType(const std::string& filename) const { return osgDB::FILE_NOT_FOUND; }
    virtual bool getFileNames(osgDB::DirectoryContents& fileNames) const { return false; }
    //virtual osgDB::DirectoryContents getDirectoryContents(const std::string& dirName) const;

    virtual ReadResult readObject(const std::string& file, const osgDB::Options* o = NULL) const
    {
        std::vector<std::string> messages;
        if (file == "rtcp")
        {
            g_server->_rtcpMutex.lock();
            g_server->_rtcpMessages.swap(messages);
            g_server->_rtcpMutex.unlock();
        }

        if (!messages.empty())
        {
            osg::ref_ptr<osgVerse::StringObject> msgObject = new osgVerse::StringObject;
            msgObject->values.swap(messages); return msgObject;
        }
        return ReadResult::FILE_NOT_FOUND;
    }

    virtual ReadResult readImage(
        const std::string& f, const osgDB::Options* o = NULL) const { return ReadResult::NOT_IMPLEMENTED; }
    virtual ReadResult readHeightField(
        const std::string& f, const osgDB::Options* o = NULL) const { return ReadResult::NOT_IMPLEMENTED; }
    virtual ReadResult readNode(
        const std::string& f, const osgDB::Options* o = NULL) const { return ReadResult::NOT_IMPLEMENTED; }
    virtual ReadResult readShader(
        const std::string& f, const osgDB::Options* o = NULL) const { return ReadResult::NOT_IMPLEMENTED; }

    // Use mk_rtc_send_datachannel() to send messages in data channel?
    virtual WriteResult writeObject(const osg::Object& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return WriteResult::NOT_IMPLEMENTED; }
    virtual WriteResult writeImage(const osg::Image& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return WriteResult::NOT_IMPLEMENTED; }
    virtual WriteResult writeHeightField(const osg::HeightField& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return WriteResult::NOT_IMPLEMENTED; }
    virtual WriteResult writeNode(const osg::Node& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return WriteResult::NOT_IMPLEMENTED; }
    virtual WriteResult writeShader(const osg::Shader& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return WriteResult::NOT_IMPLEMENTED; }

    static void API_CALL onMkMediaChanged(int regist, const mk_media_source sender)
    {
        const char* schema = mk_media_source_get_schema(sender);
        const char* vhost = mk_media_source_get_vhost(sender);
        const char* app = mk_media_source_get_app(sender);
        const char* stream = mk_media_source_get_stream(sender);
        OSG_NOTICE << "[ZLMediaServerArchive] Media registry changed: " << schema << "://" << vhost
                   << "/"  << app << "/" << stream << ", registered to " << regist << std::endl;
    }

    static void API_CALL onMkMediaPublish(const mk_media_info url_info,
                                          const mk_publish_auth_invoker invoker, const mk_sock_info sender)
    {
        char ip[64];
        const char* schema = mk_media_info_get_schema(url_info);
        const char* vhost = mk_media_info_get_vhost(url_info);
        const char* app = mk_media_info_get_app(url_info);
        const char* stream = mk_media_info_get_stream(url_info);
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] Media publishing: " << schema << "://" << vhost
                   << "/" <<  app << "/" << stream << "; Local:" << localIP << ", Peer:" << peerIP << std::endl;
        mk_publish_auth_invoker_do(invoker, NULL, 1, 1);
    }

    static void API_CALL onMkMediaPlay(const mk_media_info url_info,
                                       const mk_auth_invoker invoker, const mk_sock_info sender)
    {
        char ip[64];
        const char* schema = mk_media_info_get_schema(url_info);
        const char* vhost = mk_media_info_get_vhost(url_info);
        const char* app = mk_media_info_get_app(url_info);
        const char* stream = mk_media_info_get_stream(url_info);
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] Media playing: "  << schema << "://" << vhost
                   << "/" <<  app << "/" << stream << "; Local:" << localIP << ", Peer:" << peerIP << std::endl;
        mk_auth_invoker_do(invoker, NULL);
    }

    static int API_CALL onMkMediaNotFound(const mk_media_info url_info, const mk_sock_info sender)
    {
        char ip[64];
        const char* schema = mk_media_info_get_schema(url_info);
        const char* vhost = mk_media_info_get_vhost(url_info);
        const char* app = mk_media_info_get_app(url_info);
        const char* stream = mk_media_info_get_stream(url_info);
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] Media not found: " << schema << "://" << vhost
                   << "/" << app << "/" << stream << "; Local:" << localIP << ", Peer:" << peerIP << std::endl;
        return 0;  // 0: wait for registry; 1: close now
    }

    static void API_CALL onMkMediaNoReader(const mk_media_source sender)
    {
        const char* schema = mk_media_source_get_schema(sender);
        const char* vhost = mk_media_source_get_vhost(sender);
        const char* app = mk_media_source_get_app(sender);
        const char* stream = mk_media_source_get_stream(sender);
        OSG_NOTICE << "[ZLMediaServerArchive] Media registry status: " << schema << "://" << vhost
                   << "/" << app << "/" << stream << ", currently has no readers "<< std::endl;
    }

    static void onMkWebrtcGetAnswerSdp(void* userData, const char* answer, const char* err)
    {
        const char* response_header[] = { "Content-Type", "application/json",
                                          "Access-Control-Allow-Origin", "*" , NULL };
        if (answer) answer = escape_string(answer);

        size_t len = answer ? 2 * strlen(answer) : 1024;
        char* response_content = (char*)malloc(len);
        if (answer)
        {
            snprintf(response_content, len,
                    "{\"sdp\":\"%s\", \"type\":\"answer\", \"code\":0}", answer);
        }
        else
            snprintf(response_content, len, "{\"msg\":\"%s\", \"code\":-1}", err);

        mk_http_response_invoker invoker = (mk_http_response_invoker)userData;
        mk_http_response_invoker_do_string(invoker, 200, response_header, response_content);
        mk_http_response_invoker_clone_release(invoker);
        free(response_content);
        if (answer) free((void*)answer);
    }

    static void API_CALL onMkHttpRequest(const mk_parser parser, const mk_http_response_invoker invoker,
                                         int* consumed, const mk_sock_info sender)
    {
        const char* url = mk_parser_get_url(parser);
        const char* params = mk_parser_get_url_params(parser);
        std::string urlString = (url != NULL) ? std::string(url) : "";
        std::string paramsString = (params != NULL) ? std::string(params) : "";
        *consumed = 1;  // set to 1 to handle this request

        if (urlString == "/api/test")
        {
            const char* response_header[] = { "Content-Type", "text/plain", NULL };
            const char* reply = "osgVerse.ZLMediaServerArchive.api.test";
            mk_http_body body = mk_http_body_from_string(reply, 0);
            mk_http_response_invoker_do(invoker, 200, response_header, body);
            mk_http_body_release(body);
        }
        else if (urlString == "/index/api/webrtc")
        {
            char rtc_url[1024];
            snprintf(rtc_url, sizeof(rtc_url), "rtc://%s/%s/%s?%s",
                     mk_parser_get_header(parser, "Host"), mk_parser_get_url_param(parser, "app"),
                     mk_parser_get_url_param(parser, "stream"), paramsString.c_str());
            mk_webrtc_get_answer_sdp(
                mk_http_response_invoker_clone(invoker), ZLMediaServerArchive::onMkWebrtcGetAnswerSdp,
                mk_parser_get_url_param(parser, "type"), mk_parser_get_content(parser, NULL), rtc_url);
        }
        else if (g_server.valid())
        {
            osg::ref_ptr<osgVerse::StringObject> so = new osgVerse::StringObject;
            so->values.push_back(urlString); so->values.push_back(paramsString);
            
            osgVerse::UserCallback::Parameters in, out; in.push_back(so.get());
            osgVerse::UserCallback* cb = g_server->getHttpAPI();
            if (cb && cb->run(g_server.get(), in, out))
            {
                std::string reply;
                osgVerse::StringObject* so = out.empty() ? NULL
                                           : static_cast<osgVerse::StringObject*>(out[0].get());
                if (so && so->values.size() > 1)
                {
                    reply = so->values[1] + "/" + so->values[0];
                    if (!osgDB::findDataFile(reply).empty())
                    {
                        const char* response_header[] = { "Content-Type", "text/html", NULL };
                        mk_http_body body = mk_http_body_from_file(reply.c_str());
                        mk_http_response_invoker_do(invoker, 200, response_header, body);
                        mk_http_body_release(body); *consumed = 1; return;
                    }
                }
            }
            *consumed = 0;
        }
    }

    static void API_CALL onMkHttpAccess(const mk_parser parser, const char* path, int is_dir,
                                        const mk_http_access_path_invoker invoker, const mk_sock_info sender)
    {
        char ip[64];
        const char* url = mk_parser_get_url(parser);
        const char* params = mk_parser_get_url_params(parser);
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] HTTP access request: " << url << "?" << params
                   << ", Path = " << path << "(DIR = " << is_dir << ")"
                   << ", Local IP:" << localIP << ", Peer IP:" << peerIP << std::endl;
        mk_http_access_path_invoker_do(invoker, NULL, NULL, 0);
    }

    static void API_CALL onMkHttpPreAccess(const mk_parser parser, char* path, const mk_sock_info sender)
    {
        // Do sth here if you want to redirect the path
    }

    static void API_CALL onMkRtspGetRealm(const mk_media_info url_info,
                                          const mk_rtsp_get_realm_invoker invoker, const mk_sock_info sender)
    {
        char ip[64];
        const char* app = mk_media_info_get_app(url_info);
        const char* stream = mk_media_info_get_stream(url_info);
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] RTSP is getting realm: " << app << "/" << stream
                   << ", Local IP:" << localIP << ", Peer IP:" << peerIP << std::endl;
        mk_rtsp_get_realm_invoker_do(invoker, "zlmediakit");
    }

    static void API_CALL onMkRtspAuthorize(const mk_media_info url_info,
                                           const char* realm, const char* user_name, int must_no_encrypt,
                                           const mk_rtsp_auth_invoker invoker, const mk_sock_info sender)
    {
        char ip[64];
        const char* app = mk_media_info_get_app(url_info);
        const char* stream = mk_media_info_get_stream(url_info);
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] RTSP is authorizing: " << app << "/" << stream
                   << ", Realm = " << realm << ", User = " << user_name
                   << ", Local IP:" << localIP << ", Peer IP:" << peerIP << std::endl;
        mk_rtsp_auth_invoker_do(invoker, 0, user_name);
    }

    static void API_CALL onMkRecordVideo(const mk_mp4_info mp4)
    {
    }

    static void API_CALL onMkShellLogin(const char* user_name, const char* passwd,
                                        const mk_auth_invoker invoker, const mk_sock_info sender)
    {
        char ip[64];
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] Shell login: User = " << user_name
                   << ", Local IP:" << localIP << ", Peer IP:" << peerIP << std::endl;
        mk_auth_invoker_do(invoker, NULL);
    }

    static void API_CALL onMkFlowReport(const mk_media_info url_info, size_t total_bytes,
                                        size_t total_seconds, int is_player, const mk_sock_info sender)
    {
        char ip[64];
        const char* app = mk_media_info_get_app(url_info);
        const char* stream = mk_media_info_get_stream(url_info);
        const char* localIP = mk_sock_info_local_ip(sender, ip);
        const char* peerIP = mk_sock_info_peer_ip(sender, ip + 32);
        OSG_NOTICE << "[ZLMediaServerArchive] Flow report: " << app << "/" << stream
                   << ", Total bytes = " << total_bytes << ", Total seconds = " << total_seconds
                   << ", Local IP:" << localIP << ", Peer IP:" << peerIP << std::endl;
    }

    static void API_CALL onMkGetSCTP(mk_rtc_transport rtc_transport, uint16_t streamId,
                                     uint32_t ppid, const uint8_t* msg, size_t len)
    {
        //OSG_NOTICE << "[ZLMediaServerArchive] SCTP Data: stream-" << streamId << ":"
        //           << std::string((char*)msg, (char*)msg + len) << std::endl;
        if (g_server.valid())
        {
            std::stringstream ss; ss << streamId << "/" << ppid << ":";
            ss << std::string((char*)msg, (char*)msg + len) << std::endl;
            g_server->_rtcpMutex.lock();
            g_server->_rtcpMessages.push_back(ss.str());
            g_server->_rtcpMutex.unlock();
        }
    }

protected:
    osgVerse::UserCallback* getHttpAPI()
    {
        if (!_httpApiCallback)
        {
            osg::UserDataContainer* udc = getUserDataContainer(); if (!udc) return NULL;
            _httpApiCallback = dynamic_cast<osgVerse::UserCallback*>(udc->getUserObject("HttpAPI"));
        }
        return _httpApiCallback.get();
    }

    osg::observer_ptr<osgVerse::UserCallback> _httpApiCallback;
    std::vector<std::string> _rtcpMessages;
    std::mutex _rtcpMutex;
};

class ZLMediaPlayer : public osg::ImageStream, public OpenThreads::Thread
{
public:
    ZLMediaPlayer() { _done = false; }
    ZLMediaPlayer(const ZLMediaPlayer& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
    :   osg::ImageStream(copy, copyop), OpenThreads::Thread(),
        _reader(copy._reader), _name(copy._name), _done(copy._done) {}

    META_Object(osgVerse, ZLMediaPlayer);
    virtual void play() { _status = PLAYING; }
    virtual void pause() { _status = PAUSED; }
    virtual void rewind() { _status = REWINDING; }

    void open(osgDB::ReaderWriter* rw, const std::string name)
    {
        allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        ((osg::Vec4ub*)data())->set(255, 255, 255, 255);
        _reader = rw; _name = name; _status = PAUSED;
        start(); // start thread
    }

    virtual void quit(bool waitForThreadToExit = true)
    { _done = true; if (isRunning() && waitForThreadToExit) join(); }

protected:
    virtual ~ZLMediaPlayer() { quit(true); }
    void updateImage();

    virtual void run()
    {
        _done = false;
        while (!_done)
        {
            if (_status == PLAYING) updateImage();
            else if (_status == REWINDING) _status = PLAYING;
            YieldCurrentThread();
        }
    }

    osg::observer_ptr<osgDB::ReaderWriter> _reader;
    std::string _name; bool _done;
};

// Pushing: rw->writeImage(img, "rtsp://push_url")
// Pulling: rw->readImage("rtsp://pull_url")
class ReaderWriterZLMedia : public osgDB::ReaderWriter
{
public:
    ReaderWriterZLMedia() : _mkEnvCreated(false)
    {
        supportsProtocol("rtsp", "Support Real-Time Streaming Protocol.");
        supportsProtocol("rtmp", "Support Real-Time Messaging Protocol.");

        supportsExtension("verse_ms", "Pseudo file extension, used to select media streaming plugin.");
        supportsExtension("*", "Passes all read files to other plugins to handle actual image pulling/pushing.");
    }

    virtual ~ReaderWriterZLMedia()
    {
        for (std::map<std::string, PusherContext*>::iterator itr = _pushers.begin();
             itr != _pushers.end(); ++itr)
        { itr->second->destroy(); delete itr->second; }

        for (std::map<std::string, PlayerContext*>::iterator itr = _players.begin();
             itr != _players.end(); ++itr)
        { itr->second->destroy(); delete itr->second; }
    }

    virtual const char* className() const
    { return "[osgVerse] Media streaming plugin supporting image data pulling/pushing"; }

    virtual ReadResult openArchive(const std::string& fullFileName, ArchiveStatus status,
                                   unsigned int, const Options* options) const
    {
        // Create media server as archive
        if (!_mkEnvCreated) initialize(options);
        std::string msName = osgDB::getServerAddress(fullFileName);
        return new ZLMediaServerArchive(options);
    }

    virtual ReadResult readObject(const std::string& fullFileName, const Options* options = NULL) const
    {
        std::string fileName(fullFileName);
        std::string ext = osgDB::getFileExtension(fullFileName);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        bool usePseudo = (ext == "verse_ms");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getFileExtension(fileName);
        }

        PlayerContext* ctx = NULL;
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
        if (!acceptsProtocol(scheme)) return ReadResult::FILE_NOT_HANDLED;
#endif
        if (!_mkEnvCreated) initialize(options);

        ReaderWriterZLMedia* nonconst = const_cast<ReaderWriterZLMedia*>(this);
        if (_players.find(fileName) == _players.end())
        {
            ctx = PlayerContext::create();
            mk_player_play(ctx->player, fileName.c_str());
            nonconst->_players[fileName] = ctx;
        }
        else
            ctx = nonconst->_players[fileName];
        osg::ref_ptr<ZLMediaResourceDemuxer> demuxer = new ZLMediaResourceDemuxer;
        ctx->setDemuxer(demuxer.get());

        osgVerse::CudaResourceDemuxerMuxerContainer* container =
            new osgVerse::CudaResourceDemuxerMuxerContainer;
        container->setDemuxer(demuxer.get()); return container;
    }

    virtual ReadResult readImage(const std::string& fullFileName, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
        if (!acceptsProtocol(scheme)) return ReadResult::FILE_NOT_HANDLED;
#endif
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;
        if (!_mkEnvCreated) initialize(options);

        ReaderWriterZLMedia* nonconst = const_cast<ReaderWriterZLMedia*>(this);
        if (_players.find(fileName) == _players.end())
        {
            PlayerContext* ctx = PlayerContext::create();
            mk_player_play(ctx->player, fileName.c_str());
            nonconst->_players[fileName] = ctx;
        }

        ZLMediaPlayer* player = new ZLMediaPlayer;
        player->open(nonconst, fileName);
        return player;
    }

    virtual WriteResult writeObject(const osg::Object& obj, const std::string& fullFileName,
                                    const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
        if (!acceptsProtocol(scheme)) return WriteResult::FILE_NOT_HANDLED;
#endif
        if (fileName.empty()) return WriteResult::FILE_NOT_HANDLED;
        if (!_mkEnvCreated) initialize(options);

        ReaderWriterZLMedia* nonconst = const_cast<ReaderWriterZLMedia*>(this);
        const osgVerse::EncodedFrameObject* frame = static_cast<const osgVerse::EncodedFrameObject*>(&obj);
        if (!frame) return WriteResult::NOT_IMPLEMENTED;
        if (_pushers.find(fileName) == _pushers.end())
        {
            PusherContext* ctx = NULL;
            if (frame->getImageType() == osgVerse::EncodedFrameObject::FRAME_H264)
                ctx = PusherContext::create(fileName, frame->getFrameWidth(), frame->getFrameHeight());
            else if (frame->getImageType() == osgVerse::EncodedFrameObject::FRAME_H265)
                ctx = PusherContext::createHEVC(fileName, frame->getFrameWidth(), frame->getFrameHeight());
            if (!ctx) return WriteResult::NOT_IMPLEMENTED; else nonconst->_pushers[fileName] = ctx;
        }

        PusherContext* ctx = nonconst->_pushers[fileName];
        return ctx->pushNewFrame(const_cast<osgVerse::EncodedFrameObject*>(frame));
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& fullFileName,
                                   const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
        if (!acceptsProtocol(scheme)) return WriteResult::FILE_NOT_HANDLED;
#endif
        if (fileName.empty()) return WriteResult::FILE_NOT_HANDLED;
        if (!_mkEnvCreated) initialize(options);

        ReaderWriterZLMedia* nonconst = const_cast<ReaderWriterZLMedia*>(this);
        if (_pushers.find(fileName) == _pushers.end())
        {
            PusherContext* ctx = PusherContext::create(fileName, image.s(), image.t());
            //mk_media_start_send_rtp(ctx->media, "127.0.0.1", 30443,
            //    stream_name, true, ReaderWriterZLMedia::onMkMediaSourceSendRtp, ctx);
            nonconst->_pushers[fileName] = ctx;
        }

        PusherContext* ctx = nonconst->_pushers[fileName];
        return ctx->pushNewFrame(&image);
    }

    osg::Image* getPlayerImage(const std::string& fileName, long long* pts)
    {
        PlayerContext* ctx = _players[fileName];
        return ctx ? ctx->pullFromImageList(pts) : NULL;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_ms");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    class BaseContext
    {
    protected:
        typedef std::pair<osg::ref_ptr<osg::Image>, long long> ImagePair;
        std::list<ImagePair> _images;
        OpenThreads::Mutex _mutex;
        unsigned int _maxImages;

    public:
        void clear(unsigned int m)
        { _maxImages = m; _images.clear(); }

        void pushToImageList(osg::Image* img, long long pts)
        {
            _mutex.lock();
            _images.push_back(ImagePair(img, pts));
            if (_maxImages < _images.size()) _images.pop_front();
            _mutex.unlock();
        }

        osg::Image* pullFromImageList(long long* pts = NULL)
        {
            osg::ref_ptr<osg::Image> image; _mutex.lock();
            if (!_images.empty())
            {
                ImagePair& pair = _images.front();
                image = (osg::Image*)pair.first->clone(osg::CopyOp::DEEP_COPY_ALL);
                if (pts) *pts = pair.second; _images.pop_front();
            }
            _mutex.unlock();
            return image.release();
        }
    };
    
    class PusherContext : public BaseContext
    {
    public:
        PusherContext() : BaseContext() {}
        mk_media media;
        mk_pusher pusher;
        std::string pushUrl;

        static PusherContext* create(const std::string& url, int w, int h, int fps = 25, int bitRate = 0,
                                     const char* app = "live", const char* stream = "stream")
        {
            PusherContext* ctx = new PusherContext; ctx->pusher = NULL; ctx->pushUrl = url;
            ctx->media = mk_media_create("__defaultVhost__", app, stream, 0, 0, 0);
            mk_media_init_video(ctx->media, MKCodecH264, w, h, fps, bitRate);
            mk_media_set_on_regist(ctx->media, ReaderWriterZLMedia::onMkRegisterMediaSource, ctx);
            OSG_NOTICE << "[ReaderWriterZLMedia] Created H264 media source: " << url << "\n";
            ctx->clear(1); return ctx;
        }

        static PusherContext* createHEVC(const std::string& url, int w, int h, int fps = 25, int bitRate = 0,
                                         const char* app = "live", const char* stream = "stream")
        {
            PusherContext* ctx = new PusherContext; ctx->pusher = NULL; ctx->pushUrl = url;
            ctx->media = mk_media_create("__defaultVhost__", app, stream, 0, 0, 0);
            mk_media_init_video(ctx->media, MKCodecH265, w, h, fps, bitRate);
            mk_media_set_on_regist(ctx->media, ReaderWriterZLMedia::onMkRegisterMediaSource, ctx);
            OSG_NOTICE << "[ReaderWriterZLMedia] Created H265 media source: " << url << "\n";
            ctx->clear(1); return ctx;
        }

        void destroy()
        {
            if (pusher) mk_pusher_release(pusher);
            if (media) mk_media_release(media);
        }

        WriteResult pushNewFrame(osgVerse::EncodedFrameObject* frame)
        {
            if (!media) return WriteResult::FILE_SAVED;  // not prepared
            long long pts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            long long dts = frame->getFrameStamp(); if (!dts) dts = pts;

            static unsigned char header[4] = { 0, 0, 0, 1 };
            if (frame->getFrameWidth() > 0 && frame->getFrameHeight() > 0)
            {
                std::vector<unsigned char>& data = frame->getData();
                data.insert(data.begin(), header, header + 4);
                if (frame->getImageType() == osgVerse::EncodedFrameObject::FRAME_H264)
                { mk_media_input_h264(media, data.data(), data.size(), dts, pts); return WriteResult::FILE_SAVED; }
                else if (frame->getImageType() == osgVerse::EncodedFrameObject::FRAME_H265)
                { mk_media_input_h265(media, data.data(), data.size(), dts, pts); return WriteResult::FILE_SAVED; }
            }
            return WriteResult::ERROR_IN_WRITING_FILE;
        }

        WriteResult pushNewFrame(const osg::Image* img)
        {
            long long pts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            pushToImageList(const_cast<osg::Image*>(img), pts);
            if (!media) return WriteResult::FILE_SAVED;  // not prepared

            osg::ref_ptr<osg::Image> image = pullFromImageList(&pts);
            if (image.valid() && image->s() > 0 && image->t() > 0)
            {
                if (osg::Image::computeNumComponents(image->getPixelFormat()) < 3 ||
                    image->getDataType() != GL_UNSIGNED_BYTE)
                {
                    OSG_NOTICE << "[ReaderWriterZLMedia] Unsupported image type" << std::endl;
                    return WriteResult::NOT_IMPLEMENTED;
                }

                std::vector<std::vector<unsigned char>> yuvResult = osgVerse::convertRGBtoYUV(image.get(), osgVerse::YV12);
                if (yuvResult.size() == 3)
                {
                    char* yuvData[3] = { (char*)yuvResult[0].data(), (char*)yuvResult[1].data(), (char*)yuvResult[2].data() };
                    int linesize[3] = { image->s(), image->s() / 2, image->s() / 2 };
                    mk_media_input_yuv(media, (const char**)yuvData, linesize, pts);
                    return WriteResult::FILE_SAVED;
                }
            }
            return WriteResult::ERROR_IN_WRITING_FILE;
        }
    };

    class PlayerContext : public BaseContext
    {
    public:
        PlayerContext() : BaseContext() {}
        void setDemuxer(ZLMediaResourceDemuxer* d) { _demuxer = d; }
        ZLMediaResourceDemuxer* getDemuxer() { return _demuxer.get(); }

        mk_player player;
        mk_decoder decoder;
        mk_swscale swscale;

        static PlayerContext* create(int pixelFormat = 3/*AV_PIX_FMT_BGR24*/)
        {
            PlayerContext* ctx = new PlayerContext;
            ctx->player = mk_player_create(); ctx->decoder = NULL;
            ctx->swscale = mk_swscale_create(pixelFormat, 0, 0);
            mk_player_set_on_result(ctx->player, ReaderWriterZLMedia::onMkPlayerEvent, ctx);
            mk_player_set_on_shutdown(ctx->player, ReaderWriterZLMedia::onMkShutdown, ctx);
            ctx->clear(1); return ctx;
        }

        void destroy()
        {
            if (player) mk_player_release(player);
            if (decoder) mk_decoder_release(decoder, 1);
            if (swscale) mk_swscale_release(swscale);
        }

    protected:
        osg::observer_ptr<ZLMediaResourceDemuxer> _demuxer;
    };

    static void API_CALL onMkRegisterMediaSource(void* userData, mk_media_source sender, int regist)
    {
        PusherContext* ctx = (PusherContext*)userData;
        const char* schema = mk_media_source_get_schema(sender);
        if (!schema || ctx->pushUrl.empty()) return;

        std::string protocol = osgDB::getServerProtocol(ctx->pushUrl);
        if (protocol == schema)
        {
            if (ctx->pusher) mk_pusher_release(ctx->pusher);
            if (regist)
            {
                ctx->pusher = mk_pusher_create_src(sender);
                mk_pusher_set_on_result(ctx->pusher, ReaderWriterZLMedia::onMkPusherEvent, ctx);
                mk_pusher_set_on_shutdown(ctx->pusher, ReaderWriterZLMedia::onMkPusherEvent, ctx);
                mk_pusher_publish(ctx->pusher, ctx->pushUrl.c_str());
            }
            else
                OSG_NOTICE << "[ReaderWriterZLMedia] Pusher is stopped" << std::endl;
        }
    }

    static void API_CALL onMkPusherEvent(void* userData, int errCode, const char* errMsg)
    {
        PusherContext* ctx = (PusherContext*)userData;
        if (errCode == 0)
        {
            OSG_NOTICE << "[ReaderWriterZLMedia] Pusher is running successfully" << std::endl;
        }
        else
        {
            OSG_WARN << "[ReaderWriterZLMedia] Pusher error: (" << errCode << ") " << errMsg << std::endl;
            if (ctx->pusher) { mk_pusher_release(ctx->pusher); ctx->pusher = NULL; }
        }
    }

    static void API_CALL onMkPlayerEvent(void* userData, int errCode, const char* errMsg,
                                         mk_track tracks[], int trackCount)
    {
        if (errCode != 0)
        {
            OSG_WARN << "[ReaderWriterZLMedia] Player error: (" << errCode << ") " << errMsg << std::endl;
        }
        else
        {
            PlayerContext* ctx = (PlayerContext*)userData;
            for (int i = 0; i < trackCount; ++i)
            {
                if (mk_track_is_video(tracks[i]) == 0) continue;
                if (ctx->decoder) mk_decoder_release(ctx->decoder, 1);

                mk_track& track = tracks[i];
                if (ctx->getDemuxer())
                {
                    ctx->getDemuxer()->setTrackInfo(
                        mk_track_codec_id(track), mk_track_video_width(track), mk_track_video_height(track));
                }
                else
                {
                    ctx->decoder = mk_decoder_create(track, 0);
                    mk_decoder_set_cb(ctx->decoder, ReaderWriterZLMedia::onMkFrameDecoded, userData);
                }
                mk_track_add_delegate(track, ReaderWriterZLMedia::onMkTrackFrameOut, userData);
            }
        }
    }

    static void API_CALL onMkTrackFrameOut(void* userData, mk_frame frame)
    {
        PlayerContext* ctx = (PlayerContext*)userData;
        if (ctx->getDemuxer())
            ctx->getDemuxer()->addFrame(frame);
        else
            mk_decoder_decode(ctx->decoder, frame, 1, 1);
    }

    static void API_CALL onMkFrameDecoded(void* userData, mk_frame_pix frame)
    {
        AVFrame* frameData = mk_frame_pix_get_av_frame(frame);
        int w = mk_get_av_frame_width(frameData), h = mk_get_av_frame_height(frameData);
        long long pts = mk_get_av_frame_pts(frameData);

        osg::Image* img = new osg::Image;
        img->allocateImage(w, h, 1, GL_BGR, GL_UNSIGNED_BYTE);
        img->setInternalTextureFormat(GL_RGB8);
        img->setFileName(std::to_string(pts));

        PlayerContext* ctx = (PlayerContext*)userData;
        mk_swscale_input_frame(ctx->swscale, frame, img->data());
        ctx->pushToImageList(img, pts);
    }

    static void API_CALL onMkShutdown(void* userData, int errCode, const char* errMsg,
                                      mk_track tracks[], int trackCount)
    {
        if (errCode != 0)
        {
            OSG_WARN << "[ReaderWriterZLMedia] Player interrupted: ("
                     << errCode << ") " << errMsg << std::endl;
        }
    }
    
    void initialize(const osgDB::Options* options) const
    {
        mk_config config = { 0 };
        config.ini = NULL,
        config.ini_is_path = 1;
        config.log_level = 0;
        config.log_mask = LOG_FILE;
        config.ssl = NULL;
        config.ssl_is_path = 1;
        config.ssl_pwd = NULL;
        config.thread_num = 0;

        std::string ini, ssl, pwd;
        if (options != NULL)
        {
            ini = options->getPluginStringData("ini_file");
            ssl = options->getPluginStringData("ssl_file");
            pwd = options->getPluginStringData("ssl_pwd");
            if (!ini.empty()) config.ini = ini.c_str();
            if (!ssl.empty()) config.ssl = ssl.c_str();
            if (!pwd.empty()) config.ssl_pwd = pwd.c_str();
        }
        mk_env_init(&config);
    }

    std::map<std::string, PusherContext*> _pushers;
    std::map<std::string, PlayerContext*> _players;
    bool _mkEnvCreated;
};

ZLMediaServerArchive::ZLMediaServerArchive(const osgDB::Options* options)
{
    int createdServer = 0;
    if (options)
    {
        const std::string http = options->getPluginStringData("http");
        const std::string rtsp = options->getPluginStringData("rtsp");
        const std::string rtmp = options->getPluginStringData("rtmp");
        const std::string shell = options->getPluginStringData("shell");
        const std::string rtp = options->getPluginStringData("rtp");
        const std::string rtc = options->getPluginStringData("rtc");
        const std::string srt = options->getPluginStringData("srt");

#define MK_SRV(p, t) \
    if (p == 0) { result << "[ZLMediaServerArchive] Failed to start " << t << " server\n"; } \
    else { result << "[ZLMediaServerArchive] " << t << " server started at " << p << "\n"; createdServer++; }
        std::stringstream result; uint16_t p = 0;
        if (!http.empty()) { p = mk_http_server_start(atoi(http.c_str()), 0); MK_SRV(p, "HTTP"); }
        if (!rtsp.empty()) { p = mk_rtsp_server_start(atoi(rtsp.c_str()), 0); MK_SRV(p, "RTSP"); }
        if (!rtmp.empty()) { p = mk_rtmp_server_start(atoi(rtmp.c_str()), 0); MK_SRV(p, "RTMP"); }
        if (!shell.empty()) { p = mk_shell_server_start(atoi(shell.c_str())); MK_SRV(p, "Shell"); }
        if (!rtp.empty()) { p = mk_rtp_server_start(atoi(rtp.c_str())); MK_SRV(p, "RTP"); }
        if (!rtc.empty()) { p = mk_rtc_server_start(atoi(rtc.c_str())); MK_SRV(p, "RTC"); }
        if (!srt.empty()) { p = mk_srt_server_start(atoi(srt.c_str())); MK_SRV(p, "SRT"); }
        OSG_NOTICE << "[ZLMediaServerArchive] Created servers: " << createdServer << "\n" << result.str();
    }

    if (createdServer == 0)
    {
        mk_http_server_start(443, 0);
        mk_rtsp_server_start(554, 0);
        mk_rtmp_server_start(1935, 0);
    }
    g_server = this;

    mk_events events = { 0 };
    events.on_mk_media_changed = ZLMediaServerArchive::onMkMediaChanged;
    events.on_mk_media_publish = ZLMediaServerArchive::onMkMediaPublish;
    events.on_mk_media_play = ZLMediaServerArchive::onMkMediaPlay;
    events.on_mk_media_not_found = ZLMediaServerArchive::onMkMediaNotFound;
    events.on_mk_media_no_reader = ZLMediaServerArchive::onMkMediaNoReader;
    events.on_mk_http_request = ZLMediaServerArchive::onMkHttpRequest;
    events.on_mk_http_access = ZLMediaServerArchive::onMkHttpAccess;
    events.on_mk_http_before_access = ZLMediaServerArchive::onMkHttpPreAccess;
    events.on_mk_rtsp_get_realm = ZLMediaServerArchive::onMkRtspGetRealm;
    events.on_mk_rtsp_auth = ZLMediaServerArchive::onMkRtspAuthorize;
    events.on_mk_record_mp4 = ZLMediaServerArchive::onMkRecordVideo;
    events.on_mk_shell_login = ZLMediaServerArchive::onMkShellLogin;
    events.on_mk_flow_report = ZLMediaServerArchive::onMkFlowReport;
    events.on_mk_rtc_sctp_received = ZLMediaServerArchive::onMkGetSCTP;
    mk_events_listen(&events);
}

void ZLMediaPlayer::updateImage()
{
    ReaderWriterZLMedia* rw = static_cast<ReaderWriterZLMedia*>(_reader.get());
    if (!rw || _name.empty()) return;

    long long pts = 0;
    osg::ref_ptr<osg::Image> img = rw->getPlayerImage(_name, &pts);
    if (img.valid())
    {
        if (s() != img->s() || t() != img->t())
        {
            allocateImage(img->s(), img->t(), 1, img->getPixelFormat(), img->getDataType());
            setInternalTextureFormat(img->getInternalTextureFormat());
        }
        memcpy(data(), img->data(), img->getTotalSizeInBytes()); dirty();
    }
}

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_ms, ReaderWriterZLMedia)
