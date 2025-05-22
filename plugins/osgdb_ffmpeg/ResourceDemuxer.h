#pragma once
#include "Utils/FFmpegDemuxer.h"
#include "pipeline/CudaTexture2D.h"
#include <osg/ref_ptr>

namespace osgVerse
{

    class FFmpegResourceDemuxer : public osgVerse::CudaResourceReaderBase::Demuxer
    {
    public:
        FFmpegResourceDemuxer(const std::string& fileName, long long timeScale = 1000ll)
        { _subDemuxer = new FFmpegDemuxer(fileName.c_str(), timeScale); }

        virtual osgVerse::VideoCodecType getVideoCodec()
        {
            switch (_subDemuxer->GetVideoCodec())
            {
            case AV_CODEC_ID_MPEG1VIDEO: return osgVerse::CODEC_MPEG1;
            case AV_CODEC_ID_MPEG2VIDEO: return osgVerse::CODEC_MPEG2;
            case AV_CODEC_ID_MPEG4: return osgVerse::CODEC_MPEG4;
            case AV_CODEC_ID_WMV3:
            case AV_CODEC_ID_VC1: return osgVerse::CODEC_VC1;
            case AV_CODEC_ID_H264: return osgVerse::CODEC_H264;
            case AV_CODEC_ID_HEVC: return osgVerse::CODEC_HEVC;
            case AV_CODEC_ID_VP8: return osgVerse::CODEC_VP8;
            case AV_CODEC_ID_VP9: return osgVerse::CODEC_VP9;
            case AV_CODEC_ID_MJPEG: return osgVerse::CODEC_JPEG;
            case AV_CODEC_ID_AV1: return osgVerse::CODEC_AV1;
            default: return osgVerse::CODEC_INVALID;
            }
        }

        virtual int getWidth() { return _subDemuxer->GetWidth(); }
        virtual int getHeight() { return _subDemuxer->GetHeight(); }

        virtual bool demux(unsigned char** videoData, int* videoBytes, long long* pts)
        { return _subDemuxer->Demux(videoData, videoBytes, pts); }

    protected:
        FFmpegDemuxer* _subDemuxer;
    };
}
