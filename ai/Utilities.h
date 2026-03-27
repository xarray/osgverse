#ifndef MANA_AI_UTILITIES_HPP
#define MANA_AI_UTILITIES_HPP

#include <osg/Geometry>
#include <osg/Texture2D>

namespace osgVerse
{
    class OnnxInferencer;

    /** Multiple object tracker using ByteTrack */
    class ObjectTracker : public osg::Referenced
    {
    public:
        ObjectTracker(int frame_rate = 30, int track_buffer = 30, float track_threshold = 0.5,
                      float high_threshold = 0.6, float match_threshold = 0.8);

        void addObject(const osg::Vec4& rect, int label, float probability);
        unsigned int update();

        struct Result
        {
            enum State { Inactive = -1, New = 0, Tracked = 1, Lost = 2, Removed = 3 } state;
            size_t trackID, frameID, startFrame, tracklet; float score; osg::Vec4 rect;
        };
        Result getResult(unsigned int id) const;

        void clearObjects();
        void clearResults();

    protected:
        virtual ~ObjectTracker();

        osg::ref_ptr<osg::Referenced> _internal;
    };

    /** Real-time depth estimator using DepthAnything and OnnxRuntime */
    class DepthEstimator : public osg::Referenced
    {
    public:
        DepthEstimator(const std::string& modelFile);
        osg::Image* convert(osg::Image* rgb, int reqW = 0, int reqH = 0);

        // DepthAnything dynamic model signature： H mod 14 == 0, W mod 14 == 0
        static void findClosestImageSize(int& W, int& H, int divisor = 14);

    protected:
        osg::ref_ptr<osg::Referenced> _inferencer;
        osg::ref_ptr<osg::Image> _depthImage;

        std::vector<unsigned char> _resizeBuffer;
        std::vector<int64_t> _inputShapes;
        std::string _inputLayerName;
        int _inputDataType;
    };
}

#endif
