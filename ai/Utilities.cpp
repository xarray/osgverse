#include <osg/io_utils>
#include <osg/Version>
#include <osgDB/ConvertUTF>
#include "3rdparty/avir/avir.h"

#if defined(VERSE_WITH_BYTETRACK)
#   include "3rdparty/ByteTrack/BYTETracker.h"
#endif

#if defined(VERSE_WITH_ONNXRUNTIME)
#   include "OnnxRuntimeEngine.h"
#endif

#include "Utilities.h"
#include <iostream>
#include <sstream>
using namespace osgVerse;

namespace
{
#if defined(VERSE_WITH_BYTETRACK)
    struct ByteTrackerData : public osg::Referenced
    {
        std::vector<byte_track::Object> inputs;
        std::vector<std::shared_ptr<byte_track::STrack>> results;
        byte_track::BYTETracker* tracker;
    };
#endif
}

ObjectTracker::ObjectTracker(int fps, int track_buffer, float track_th, float high_th, float match_th)
{
#if defined(VERSE_WITH_BYTETRACK)
    ByteTrackerData* btd = new ByteTrackerData; _internal = btd;
    btd->tracker = new byte_track::BYTETracker(fps, track_buffer, track_th, high_th, match_th);
#else
    OSG_WARN << "[ObjectTracker] ByteTrack not compiled, so tracker is disabled\n";
#endif
}

ObjectTracker::~ObjectTracker()
{
#if defined(VERSE_WITH_BYTETRACK)
    ByteTrackerData* btd = (ByteTrackerData*)_internal.get();
    delete btd->tracker; _internal = NULL;
#endif
}

void ObjectTracker::addObject(const osg::Vec4& rect, int label, float prob)
{
#if defined(VERSE_WITH_BYTETRACK)
    ByteTrackerData* btd = (ByteTrackerData*)_internal.get();
    btd->inputs.push_back(
        byte_track::Object(byte_track::Rect<float>(rect[0], rect[1], rect[2], rect[3]), label, prob));
#endif
}

void ObjectTracker::clearObjects()
{
#if defined(VERSE_WITH_BYTETRACK)
    ByteTrackerData* btd = (ByteTrackerData*)_internal.get(); btd->inputs.clear();
#endif
}

void ObjectTracker::clearResults()
{
#if defined(VERSE_WITH_BYTETRACK)
    ByteTrackerData* btd = (ByteTrackerData*)_internal.get(); btd->results.clear();
#endif
}

unsigned int ObjectTracker::update()
{
#if defined(VERSE_WITH_BYTETRACK)
    ByteTrackerData* btd = (ByteTrackerData*)_internal.get();
    btd->results = btd->tracker->update(btd->inputs);
    return btd->results.size();
#else
    OSG_NOTICE << "[ObjectTracker] ByteTrack not compiled, so tracker is disabled\n";
    return 0;
#endif
}

ObjectTracker::Result ObjectTracker::getResult(unsigned int id) const
{
#if defined(VERSE_WITH_BYTETRACK)
    const ByteTrackerData* btd = (const ByteTrackerData*)_internal.get();
    if (id < btd->results.size())
    {
        const byte_track::STrack* s = btd->results[id].get(); if (!s) return { Result::Inactive };
        Result::State state = s->isActivated() ? (Result::State)s->getSTrackState() : Result::Inactive;
        const byte_track::Rect<float>& r = s->getRect(); osg::Vec4 rect(r.x(), r.y(), r.width(), r.height());
        return { state, s->getTrackId(), s->getFrameId(), s->getStartFrameId(),
                 s->getTrackletLength(), s->getScore(), rect };
    }
#endif
    return { Result::Inactive };
}

DepthEstimator::DepthEstimator(const std::string& modelFile) : _inputDataType(0)
{
#if defined(VERSE_WITH_ONNXRUNTIME)
    osg::ref_ptr<OnnxInferencer> inferencer =
        new OnnxInferencer(osgDB::convertUTF8toUTF16(modelFile), OnnxInferencer::CUDA);
    _inferencer = inferencer;

    _inputLayerName = inferencer->getModelLayerNames(true).front();
    _inputShapes = inferencer->getModelShapes(true, _inputLayerName);
    _inputDataType = (int)inferencer->getModelDataType(true, _inputLayerName);
#else
    OSG_WARN << "[DepthEstimator] OnnxRuntime not compiled, so estimator is disabled\n";
#endif

    _depthImage = new osg::Image;
    _depthImage->setInternalTextureFormat(GL_LUMINANCE8);
}

osg::Image* DepthEstimator::convert(osg::Image* rgb, int reqW, int reqH)
{
#if defined(VERSE_WITH_ONNXRUNTIME)
    OnnxInferencer* inferencer = static_cast<OnnxInferencer*>(_inferencer.get());
    if (rgb && rgb->valid())
    {
        osg::ref_ptr<osg::Image> image = rgb;
        if (reqW > 0 && reqH > 0)
        {
            int depth = (image->getDataType() == GL_UNSIGNED_BYTE) ? 8
                      : ((image->getDataType() == GL_UNSIGNED_SHORT) ? 16 : 0);
            int comp = osg::Image::computeNumComponents(image->getPixelFormat());
            int bufferSize = reqW * reqH * comp;
            if (_resizeBuffer.size() != bufferSize) _resizeBuffer.resize(bufferSize);

            avir::CImageResizer<> resizer(8, depth);
            resizer.resizeImage(image->data(), image->s(), image->t(), 0,
                                _resizeBuffer.data(), reqW, reqH, comp, 0.0);
            image->setImage(reqW, reqH, 1, image->getInternalTextureFormat(), image->getPixelFormat(),
                            GL_UNSIGNED_BYTE, _resizeBuffer.data(), osg::Image::NO_DELETE);
        }

        int imgW = image->s(), imgH = image->t();
        if (imgW != _depthImage->s() || imgH != _depthImage->t())
            _depthImage->allocateImage(imgW, imgH, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE);

        // FIXME: improve image converting?
        image = OnnxInferencer::convertImage(image.get(), (OnnxInferencer::DataType)_inputDataType, _inputShapes);
        inferencer->addInput(std::vector<osg::Image*>{ image.get() }, _inputLayerName);
        inferencer->run([this, &inferencer](size_t numOutputs, bool success)
            {
                if (!success) return; else _depthImage->dirty();
                std::vector<float> values; inferencer->getOutput(values);

                std::vector<float>::iterator it0 = std::min_element(values.begin(), values.end());
                std::vector<float>::iterator it1 = std::max_element(values.begin(), values.end());
                float minV = *it0, range = (*it1 - *it0); int size = (int)values.size();
                unsigned char* ptr = _depthImage->data();
#pragma omp parallel for
                for (int i = 0; i < size; ++i) *(ptr + i) = (unsigned char)((values[i] - minV) * 255.0f / range);
            });
    }
#else
    OSG_NOTICE << "[DepthEstimator] OnnxRuntime not compiled, so estimator is disabled\n";
#endif
    return _depthImage.get();
}

void DepthEstimator::findClosestImageSize(int& W, int& H, int divisor)
{
    int original_h = H, original_w = W;
    int new_h = ((original_h + divisor - 1) / divisor) * divisor;
    int new_w = ((original_w + divisor - 1) / divisor) * divisor;
    int new_h_down = (original_h / divisor) * divisor;
    int new_w_down = (original_w / divisor) * divisor;

    int change_up = std::abs(new_h - original_h) + std::abs(new_w - original_w);
    int change_down = std::abs(new_h_down - original_h) + std::abs(new_w_down - original_w);
    if (change_up < change_down) { W = new_w; H = new_h; }
    else { W = new_w_down; H = new_h_down; }
}
