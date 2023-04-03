#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <mk_mediakit.h>

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

        // TODO
        return WriteResult();
    }

protected:
    struct BaseContext
    {
        std::list<osg::ref_ptr<osg::Image>> _images;
        OpenThreads::Mutex _mutex;
        unsigned int _maxImages;

        void clear(unsigned int m)
        { _maxImages = m; _images.clear(); }

        void pushToImageList(osg::Image* img)
        {
            _mutex.lock(); _images.push_back(img);
            if (_maxImages < _images.size()) _images.pop_front();
            _mutex.unlock();
        }

        osg::Image* pullFromImageList()
        {
            osg::ref_ptr<osg::Image> image;
            _mutex.lock();
            if (!_images.empty()) { image = _images.front(); _images.pop_front(); }
            _mutex.unlock();
            return image.release();
        }
    };
    
    struct PusherContext : public BaseContext
    {
        mk_media media;
        mk_pusher pusher;
        std::string pushUrl;

        static PusherContext* create()
        {
            PusherContext* ctx = new PusherContext;
            memset(ctx, 0, sizeof(PusherContext));
            ctx->clear(10); return ctx;
        }

        void destroy()
        {
            if (pusher) mk_pusher_release(pusher);
            if (media) mk_media_release(media);
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

        osg::Image* img = new osg::Image;
        img->allocateImage(w, h, 1, GL_BGR, GL_UNSIGNED_BYTE);
        img->setInternalTextureFormat(GL_RGB8);

        PlayerContext* ctx = (PlayerContext*)userData;
        mk_swscale_input_frame(ctx->swscale, frame, img->data());
        ctx->pushToImageList(img);
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
