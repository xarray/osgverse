#pragma once
#include "Utils/FFmpegDemuxer.h"
#include "pipeline/ExternalTexture2D.h"
#include <osg/ref_ptr>

namespace osgVerse
{

    class FFmpegResourceDemuxer : public osgVerse::GpuResourceReaderBase::Demuxer
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

        virtual int getWidth() const { return _subDemuxer->GetWidth(); }
        virtual int getHeight() const { return _subDemuxer->GetHeight(); }
        virtual double getFrameRate() const { return _subDemuxer->GetFPS(); }

        virtual bool demux(unsigned char** dataData, int* dataBytes, long long* pts)
        {
            std::vector<float> buffer; std::string name = _subDemuxer->GetMediaName();
            int numSamples = 0, channels = 0; bool isVideo = false, ok = false;
            AudioPlayer* audio = static_cast<AudioPlayer*>(getAudioContainer());

            ok = _subDemuxer->Demux(isVideo, dataData, dataBytes, pts);
            while (ok && !isVideo)
            {
                if (audio != NULL)
                {
                    channels = _subDemuxer->GetAudioFrame(buffer, numSamples);
                    while (channels > 0)
                    {
                        // Check and add new PCM frame to global osgVerse::AudioPlayer
                        AudioPlayer::Clip* clip = audio->getClip(name);
                        if (!clip) { audio->addQueue(name, true, false); clip = audio->getClip(name); }

                        if (clip && clip->decodeData.valid())
                        {
                            AudioPlayer::PcmQueue* q = static_cast<AudioPlayer::PcmQueue*>(clip->decodeData.get());
                            q->push(AudioPlayer::PcmFrame{ numSamples, channels, buffer });
                        }
                        channels = _subDemuxer->GetAudioFrame(buffer, numSamples);
                    }
                }
                else
                    { OSG_NOTICE << "[FFmpegResourceDemuxer] Audio data found but no audio-container set\n"; }

                // Continue till a video frame comes
                ok = _subDemuxer->Demux(isVideo, dataData, dataBytes, pts);
            }
            return isVideo && ok;
        }

    protected:
        virtual ~FFmpegResourceDemuxer()
        {
            std::string name = _subDemuxer->GetMediaName();
            AudioPlayer* audio = static_cast<AudioPlayer*>(getAudioContainer());
            if (audio && !name.empty()) audio->removeFile(name);
        }

        FFmpegDemuxer* _subDemuxer;
    };
}
