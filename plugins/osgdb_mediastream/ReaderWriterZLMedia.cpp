#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <xxYUV/rgb2yuv.h>
#include <mk_mediakit.h>
#include <chrono>
#define ALIGN(v, a) ((v) + ((a) - 1) & ~((a) - 1))

// Pushing: rw->writeImage(img, "rtsp://push_url")
// Pulling: rw->readImage("rtsp://pull_url")
class ReaderWriterZLMedia : public osgDB::ReaderWriter
{
public:
    ReaderWriterZLMedia()
    {
        initialize();
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

    virtual ReadResult readImage(const std::string& fullFileName, const Options* options) const
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

        ReaderWriterZLMedia* nonconst = const_cast<ReaderWriterZLMedia*>(this);
        if (_players.find(fileName) == _players.end())
        {
            PlayerContext* ctx = PlayerContext::create();
            mk_player_play(ctx->player, fileName.c_str());
            nonconst->_players[fileName] = ctx;
        }

        PlayerContext* ctx = nonconst->_players[fileName];
        return ctx->pullFromImageList();
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& fullFileName,
                                   const Options* options) const
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

        ReaderWriterZLMedia* nonconst = const_cast<ReaderWriterZLMedia*>(this);
        if (_pushers.find(fileName) == _pushers.end())
        {
            PusherContext* ctx = PusherContext::create();
            nonconst->_pushers[fileName] = ctx;
        }

        PusherContext* ctx = nonconst->_pushers[fileName];
        return ctx->pushNewFrame(&image);
    }

protected:
    struct BaseContext
    {
        typedef std::pair<osg::ref_ptr<osg::Image>, long long> ImagePair;
        std::list<ImagePair> _images;
        OpenThreads::Mutex _mutex;
        unsigned int _maxImages;

        void clear(unsigned int m)
        { _maxImages = m; _images.clear(); }

        void pushToImageList(osg::Image* img, long long pts)
        {
            _mutex.lock(); _images.push_back(ImagePair(img, pts));
            if (_maxImages < _images.size()) _images.pop_front();
            _mutex.unlock();
        }

        osg::Image* pullFromImageList(long long* pts = NULL)
        {
            osg::ref_ptr<osg::Image> image;
            _mutex.lock();
            if (!_images.empty())
            {
                ImagePair& pair = _images.front(); image = pair.first;
                if (pts) *pts = pair.second; _images.pop_front();
            }
            _mutex.unlock();
            return image.release();
        }
    };
    
    struct PusherContext : public BaseContext
    {
        mk_media media;
        mk_pusher pusher;
        std::string pushUrl;

        static PusherContext* create(const char* app = "live", const char* stream = "stream")
        {
            PusherContext* ctx = new PusherContext;
            memset(ctx, 0, sizeof(PusherContext));
            ctx->media = mk_media_create("__defaultVhost__", app, stream, 0, 0, 0);
            ctx->pusher = NULL;
            {
                codec_args v_args = { 0 };
                mk_track v_track = mk_track_create(MKCodecH264, &v_args);
                mk_media_init_track(ctx->media, v_track);
                mk_media_init_complete(ctx->media);
                mk_media_set_on_regist(ctx->media, ReaderWriterZLMedia::onMkRegisterMediaSource, &ctx);
                mk_track_unref(v_track);
            }
            ctx->clear(10); return ctx;
        }

        void destroy()
        {
            if (pusher) mk_pusher_release(pusher);
            if (media) mk_media_release(media);
        }

        WriteResult pushNewFrame(const osg::Image* img)
        {
            long long pts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            pushToImageList(const_cast<osg::Image*>(img), pts);
            if (!media || !pusher) return WriteResult::FILE_SAVED;  // not prepared

            osg::ref_ptr<osg::Image> image = pullFromImageList(&pts);
            if (image.valid() && image->s() > 0 && image->t() > 0)
            {
                if (osg::Image::computeNumComponents(image->getPixelFormat()) != 3 ||
                    image->getDataType() != GL_UNSIGNED_BYTE)
                { return WriteResult::NOT_IMPLEMENTED; }

                rgb2yuv_parameter rgb2yuv;
                memset(&rgb2yuv, 0, sizeof(rgb2yuv_parameter));
                rgb2yuv.width = image->s(); rgb2yuv.height = image->t();
                rgb2yuv.rgb = image->data(); rgb2yuv.componentRGB = 3;
                rgb2yuv.strideRGB = 0; rgb2yuv.swizzleRGB = false;
                rgb2yuv.alignWidth = 16; rgb2yuv.alignHeight = 1;
                rgb2yuv.alignSize = 1; rgb2yuv.videoRange = false;
                rgb2yuv_yv12(&rgb2yuv);

                int strideY = ALIGN(rgb2yuv.width, rgb2yuv.alignWidth);
                int strideU = strideY / 2, strideV = strideY / 2;
                int sizeY = ALIGN(strideY * ALIGN(rgb2yuv.height, rgb2yuv.alignHeight), rgb2yuv.alignSize);
                int sizeU = ALIGN(strideU * ALIGN(rgb2yuv.height, rgb2yuv.alignHeight) / 2, rgb2yuv.alignSize);
                
                char* yuvData[3] = { (char*)rgb2yuv.y, (char*)rgb2yuv.u, (char*)rgb2yuv.v };
                int linesize[3] = { sizeY, sizeU, sizeU };
                mk_media_input_yuv(media, (const char**)yuvData, linesize, pts);
                return WriteResult::FILE_SAVED;
            }
            return WriteResult::ERROR_IN_WRITING_FILE;
        }
    };

    struct PlayerContext : public BaseContext
    {
        mk_player player;
        mk_decoder decoder;
        mk_swscale swscale;

        static PlayerContext* create(int pixelFormat = 3/*AV_PIX_FMT_BGR24*/)
        {
            PlayerContext* ctx = new PlayerContext;
            memset(ctx, 0, sizeof(PlayerContext));
            ctx->player = mk_player_create();
            ctx->swscale = mk_swscale_create(pixelFormat, 0, 0);
            mk_player_set_on_result(ctx->player, ReaderWriterZLMedia::onMkPlayerEvent, &ctx);
            mk_player_set_on_shutdown(ctx->player, ReaderWriterZLMedia::onMkShutdown, &ctx);
            ctx->clear(10); return ctx;
        }

        void destroy()
        {
            if (player) mk_player_release(player);
            if (decoder) mk_decoder_release(decoder, 1);
            if (swscale) mk_swscale_release(swscale);
        }
    };

    static void API_CALL onMkRegisterMediaSource(void* userData, mk_media_source sender, int regist)
    {
        PusherContext* ctx = (PusherContext*)userData;
        const char* schema = mk_media_source_get_schema(sender);

        if (strncmp(schema, ctx->pushUrl.c_str(), strlen(schema)) == 0)
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
                OSG_NOTICE << "[ReaderWriterZLMedia] pusher is stopped" << std::endl;
        }
    }

    static void API_CALL onMkPusherEvent(void* userData, int errCode, const char* errMsg)
    {
        PusherContext* ctx = (PusherContext*)userData;
        if (errCode == 0)
        {
            OSG_NOTICE << "[ReaderWriterZLMedia] pusher is running successfully" << std::endl;
        }
        else
        {
            OSG_WARN << "[ReaderWriterZLMedia] pusher error: ("
                     << errCode << ") " << errMsg << std::endl;
            if (ctx->pusher) { mk_pusher_release(ctx->pusher); ctx->pusher = NULL; }
        }
    }

    static void API_CALL onMkPlayerEvent(void* userData, int errCode, const char* errMsg,
                                         mk_track tracks[], int trackCount)
    {
        if (errCode != 0)
        {
            OSG_WARN << "[ReaderWriterZLMedia] player error: ("
                     << errCode << ") " << errMsg << std::endl;
        }
        else
        {
            PlayerContext* ctx = (PlayerContext*)userData;
            for (int i = 0; i < trackCount; ++i)
            {
                if (!mk_track_is_video(tracks[i])) continue;
                if (ctx->decoder) mk_decoder_release(ctx->decoder, 1);
                ctx->decoder = mk_decoder_create(tracks[i], 0);

                mk_decoder_set_cb(ctx->decoder, ReaderWriterZLMedia::onMkFrameDecoded, userData);
                mk_track_add_delegate(tracks[i], ReaderWriterZLMedia::onMkTrackFrameOut, userData);
            }
        }
    }

    static void API_CALL onMkTrackFrameOut(void* userData, mk_frame frame)
    {
        PlayerContext* ctx = (PlayerContext*)userData;
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
            OSG_WARN << "[ReaderWriterZLMedia] player interrupted: ("
                     << errCode << ") " << errMsg << std::endl;
        }
    }
    
    void initialize()
    {
        mk_config config;
        config.ini = NULL,
        config.ini_is_path = 0,
        config.log_level = 0,
        config.log_mask = LOG_CONSOLE,
        config.ssl = NULL,
        config.ssl_is_path = 1,
        config.ssl_pwd = NULL,
        config.thread_num = 0;
        mk_env_init(&config);
    }

    std::map<std::string, PusherContext*> _pushers;
    std::map<std::string, PlayerContext*> _players;
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_ms, ReaderWriterZLMedia)
