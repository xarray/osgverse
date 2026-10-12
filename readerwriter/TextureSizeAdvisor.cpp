#include <osg/Math>
#include <osg/Image>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include "pipeline/Utilities.h"
#include "Utilities.h"
#include "TextureSizeAdvisor.h"

namespace osgVerse
{
namespace
{
    inline int toByteValue(float v)
    { return osg::clampBetween<int>((int)(v * 255.0f + 0.5f), 0, 255); }

    inline int luminanceOf(int r, int g, int b)
    { return (r * 77 + g * 150 + b * 29) >> 8; }  // ~0.299/0.587/0.114

    inline bool isPowerOfTwo(int x)
    { return (x > 0) && ((x & (x - 1)) == 0); }

    inline int floorPowerOfTwo(int x)
    { int p = 1; while (p <= x / 2) p <<= 1; return p; }

    inline int alignDown(int x, int a)
    { if (a <= 1) return x; int r = (x / a) * a; return (r < a) ? a : r; }

    // Convert an image to an uncompressed copy that resizeImage() can handle. Compressed
    // images are decoded best-effort to RGBA8 so the advisor also works on DDS/KTX inputs.
    osg::Image* cloneUncompressed(const osg::Image& src)
    {
        if (!src.valid() || src.s() <= 0 || src.t() <= 0) return NULL;
        osg::ref_ptr<osg::Image> dst = new osg::Image;
        dst->setOrigin(src.getOrigin());

        if (src.isCompressed())
        {
            int w = src.s(), h = src.t();
            unsigned char* data = new unsigned char[w * h * 4];
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                {
                    osg::Vec4 c = src.getColor(x, y); int idx = (y * w + x) * 4;
                    data[idx + 0] = (unsigned char)toByteValue(c[0]);
                    data[idx + 1] = (unsigned char)toByteValue(c[1]);
                    data[idx + 2] = (unsigned char)toByteValue(c[2]);
                    data[idx + 3] = (unsigned char)toByteValue(c[3]);
                }
            dst->setImage(w, h, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
            return dst.release();
        }

        dst->allocateImage(src.s(), src.t(), src.r(), src.getPixelFormat(), src.getDataType(), src.getPacking());
        dst->setInternalTextureFormat(src.getInternalTextureFormat());
        if (dst->data() && src.data()) memcpy(dst->data(), src.data(), src.getTotalSizeInBytes());
        return dst.release();
    }

    // Clone + resize to the given size, returning a new image (never modifies the source)
    osg::Image* resizeCopy(const osg::Image& src, int w, int h)
    {
        if (w <= 0 || h <= 0) return NULL;
        osg::ref_ptr<osg::Image> copy = cloneUncompressed(src);
        if (!copy.valid()) return NULL;
        if (!osgVerse::resizeImage(*copy, w, h, false)) copy->scaleImage(w, h, 1);
        return copy.release();
    }

    // Compute entropy / color count / intensity standard deviation on the given image
    void computeMetricsInternal(const osg::Image& img, const TextureSizeAdvisor::Options& opt,
                                TextureSizeAdvisor::Metrics& m)
    {
        int w = img.s(), h = img.t();
        long long hist[256] = { 0 }; long long n = 0;
        double sum = 0.0, sumSq = 0.0;
        std::unordered_set<unsigned int> colors;
        size_t colorLimit = (size_t)osg::maximum(1, opt.colorCountThreshold);
        bool saturated = false;

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                osg::Vec4 c = img.getColor(x, y);
                int r = toByteValue(c[0]), g = toByteValue(c[1]);
                int b = toByteValue(c[2]), a = toByteValue(c[3]);
                if (!saturated)
                {   // Stop counting once there are clearly many colors
                    colors.insert((unsigned int)r | ((unsigned int)g << 8) |
                                  ((unsigned int)b << 16) | ((unsigned int)a << 24));
                    if (colors.size() > colorLimit) { saturated = true; colors.clear(); }
                }
                int lum = luminanceOf(r, g, b); hist[lum]++; ++n;
                sum += lum; sumSq += (double)lum * lum;
            }

        m.entropy = 0.0f;
        if (n > 0)
            for (int i = 0; i < 256; ++i)
                if (hist[i] > 0)
                { double p = (double)hist[i] / (double)n; m.entropy -= (float)(p * std::log2(p)); }

        m.stdDev = 0.0f;
        if (n > 0)
        {
            double mean = sum / (double)n, var = sumSq / (double)n - mean * mean;
            m.stdDev = (float)(var > 0.0 ? std::sqrt(var) : 0.0);
        }
        m.uniqueColors = saturated ? -1 : (int)colors.size();

        float colorNorm = saturated ? 1.0f
            : osg::clampBetween<float>((float)(std::log2((double)colors.size() + 1.0) / 16.0), 0.0f, 1.0f);
        float entropyNorm = osg::clampBetween<float>(m.entropy / 8.0f, 0.0f, 1.0f);
        float stdNorm = osg::clampBetween<float>(m.stdDev / 128.0f, 0.0f, 1.0f);
        m.complexity = osg::maximum(osg::maximum(entropyNorm, colorNorm), stdNorm);

        bool fewColors = saturated ? false : (m.uniqueColors < opt.colorCountThreshold);
        m.isSimple = (m.entropy < opt.entropyThreshold) && fewColors && (m.stdDev < opt.stdDevThreshold);
    }

    // Longer side of the on-screen reference: the power of two closest to the target screen
    // size, never up-scaling beyond the original image and never exceeding the analysis cap.
    int computeReferenceLongSide(int srcW, int srcH, const TextureSizeAdvisor::Options& opt)
    {
        int longer = osg::maximum(srcW, srcH);
        int target = (opt.targetScreenSize > 0) ? opt.targetScreenSize : longer;
        int cap = osg::minimum(longer, target);
        if (opt.maxAnalysisSize > 0) cap = osg::minimum(cap, opt.maxAnalysisSize);

        int refLong = osg::Image::computeNearestPowerOfTwo(cap);
        if (refLong > longer) refLong = floorPowerOfTwo(longer);
        return osg::maximum(refLong, osg::minimum(osg::maximum(1, opt.minSize), longer));
    }

    // Size of the "j-th" candidate: the reference longer side halved j times (so it stays a
    // power of two and keeps the aspect ratio), the shorter side derived from the original
    // aspect ratio and aligned to the compression block size. Returns false once too small.
    bool computeLevelSize(int refLong, int longer, int shorter, bool landscape,
                          int minSize, int blockAlign, int j, int& w, int& h)
    {
        int cl = refLong >> j;
        if (cl < osg::maximum(1, minSize)) return false;

        int cs = (int)std::llround((double)cl * (double)shorter / (double)longer);
        cs = alignDown(osg::maximum(cs, osg::maximum(1, blockAlign)), osg::maximum(1, blockAlign));
        if (landscape) { w = cl; h = cs; } else { w = cs; h = cl; }
        return true;
    }
}

TextureSizeAdvisor::Metrics TextureSizeAdvisor::computeMetrics(const osg::Image& image)
{ return computeMetrics(image, Options()); }

TextureSizeAdvisor::Metrics TextureSizeAdvisor::computeMetrics(const osg::Image& image, const Options& opt)
{
    Metrics m;
    if (!image.valid() || image.s() <= 0 || image.t() <= 0) return m;

    int longer = osg::maximum(image.s(), image.t());
    int shorter = osg::minimum(image.s(), image.t());
    bool landscape = image.s() >= image.t();
    int refLong = computeReferenceLongSide(image.s(), image.t(), opt);
    int refW = 0, refH = 0;
    if (!computeLevelSize(refLong, longer, shorter, landscape, opt.minSize, opt.blockAlign, 0, refW, refH))
        return m;

    osg::ref_ptr<osg::Image> ref = resizeCopy(image, refW, refH);
    if (ref.valid() && ref->valid()) computeMetricsInternal(*ref, opt, m);
    return m;
}

TextureSizeAdvisor::Result TextureSizeAdvisor::analyze(const osg::Image& image)
{ return analyze(image, Options()); }

TextureSizeAdvisor::Result TextureSizeAdvisor::analyze(const osg::Image& image, const Options& opt)
{
    Result r; r.originalWidth = image.s(); r.originalHeight = image.t();
    r.suggestedWidth = image.s(); r.suggestedHeight = image.t(); r.achievedSSIM = 1.0f;
    if (!image.valid() || image.s() <= 0 || image.t() <= 0) return r;

    int longer = osg::maximum(image.s(), image.t());
    int shorter = osg::minimum(image.s(), image.t());
    bool landscape = image.s() >= image.t();
    if (longer <= osg::maximum(1, opt.minSize)) return r;  // already too small to bother

    int refLong = computeReferenceLongSide(image.s(), image.t(), opt);
    int refW = 0, refH = 0;
    if (!computeLevelSize(refLong, longer, shorter, landscape, opt.minSize, opt.blockAlign, 0, refW, refH))
        return r;

    osg::ref_ptr<osg::Image> ref = resizeCopy(image, refW, refH);
    if (!ref.valid() || !ref->valid()) return r;
    r.referenceWidth = refW; r.referenceHeight = refH;

    computeMetricsInternal(*ref, opt, r.metrics);
    r.simple = r.metrics.isSimple;

    int bestW = refW, bestH = refH;
    bool evaluated = false; float bestSSIM = 1.0f;
    bool reducible = image.isCompressed() || image.getDataType() == GL_UNSIGNED_BYTE;

    if (reducible && opt.useSSIM)
    {   // Halve the reference level by level, up-scale back and compare with SSIM. The first
        // level that falls below the threshold stops the search (down-scaling is monotonic).
        osg::ref_ptr<osg::Image> prev = ref;
        for (int j = 1; ; ++j)
        {
            int cw = 0, ch = 0;
            if (!computeLevelSize(refLong, longer, shorter, landscape, opt.minSize, opt.blockAlign, j, cw, ch))
                break;

            osg::ref_ptr<osg::Image> cand = resizeCopy(*prev, cw, ch);
            if (!cand.valid()) break;
            osg::ref_ptr<osg::Image> up = resizeCopy(*cand, refW, refH);
            if (!up.valid()) break;

            float sim = SSIM::compute(*ref, *up).ssim; evaluated = true;
            if (sim < 0.0f || sim < opt.ssimThreshold) break;
            bestW = cw; bestH = ch; bestSSIM = sim; prev = cand;
        }
    }
    else if (reducible && r.simple)
    {   // Metrics-only mode: a simple image can go straight to the smallest allowed level
        for (int j = 1; ; ++j)
        {
            int cw = 0, ch = 0;
            if (!computeLevelSize(refLong, longer, shorter, landscape, opt.minSize, opt.blockAlign, j, cw, ch))
                break;
            bestW = cw; bestH = ch; bestSSIM = -1.0f;
        }
    }

    int oLong = longer, sLong = osg::maximum(bestW, bestH);
    bool smaller = (bestW < r.originalWidth || bestH < r.originalHeight);
    r.reduced = smaller && (sLong <= (int)std::floor(oLong * opt.minReductionRatio));
    if (r.reduced)
    { r.suggestedWidth = bestW; r.suggestedHeight = bestH; r.achievedSSIM = evaluated ? bestSSIM : -1.0f; }
    else
    { r.suggestedWidth = r.originalWidth; r.suggestedHeight = r.originalHeight; r.achievedSSIM = 1.0f; }

    if (opt.verbose)
        OSG_NOTICE << "[TextureSizeAdvisor] " << toString(r) << std::endl;
    return r;
}

bool TextureSizeAdvisor::optimizeSize(osg::Image& image)
{ return optimizeSize(image, Options(), NULL); }

bool TextureSizeAdvisor::optimizeSize(osg::Image& image, const Options& opt, Result* out)
{
    Result r = analyze(image, opt);
    if (out) *out = r;
    if (!r.reduced || r.suggestedWidth <= 0 || r.suggestedHeight <= 0) return false;

    osg::ref_ptr<osg::Image> work = cloneUncompressed(image);
    if (!work.valid()) return false;
    if (!osgVerse::resizeImage(*work, r.suggestedWidth, r.suggestedHeight, false))
        work->scaleImage(r.suggestedWidth, r.suggestedHeight, 1);

    image.allocateImage(work->s(), work->t(), work->r(), work->getPixelFormat(),
                        work->getDataType(), work->getPacking());
    image.setInternalTextureFormat(work->getInternalTextureFormat());
    memcpy(image.data(), work->data(), work->getTotalSizeInBytes());
    return true;
}

bool TextureSizeAdvisor::isSizeWorthKeeping(const osg::Image& image)
{ return !analyze(image, Options()).reduced; }

bool TextureSizeAdvisor::isSizeWorthKeeping(const osg::Image& image, const Options& opt)
{ return !analyze(image, opt).reduced; }

std::string TextureSizeAdvisor::toString(const Metrics& m)
{
    std::stringstream ss; ss << std::fixed << std::setprecision(3);
    ss << "entropy=" << m.entropy << ", stddev=" << m.stdDev << ", colors=" << m.uniqueColors
       << ", complexity=" << m.complexity << (m.isSimple ? ", simple=yes" : ", simple=no");
    return ss.str();
}

std::string TextureSizeAdvisor::toString(const Result& r)
{
    std::stringstream ss; ss << std::fixed << std::setprecision(4);
    ss << r.originalWidth << "x" << r.originalHeight << " -> "
       << r.suggestedWidth << "x" << r.suggestedHeight
       << " (ref=" << r.referenceWidth << "x" << r.referenceHeight
       << ", ssim=" << r.achievedSSIM << (r.reduced ? ", reduced" : ", kept") << ") | "
       << toString(r.metrics);
    return ss.str();
}

}  // namespace osgVerse
