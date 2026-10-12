#ifndef MANA_READERWRITER_TEXTURESIZEADVISOR_HPP
#define MANA_READERWRITER_TEXTURESIZEADVISOR_HPP

#include <osg/Image>
#include "Export.h"
#include <string>

namespace osgVerse
{
    /** Texture size advisor: judge whether a texture really deserves its (often
        oversized) resolution, and shrink it when the reduction cannot be seen.

        The core assumption is expressed by Options::targetScreenSize: the texture
        is expected to be displayed on screen inside a targetScreenSize x
        targetScreenSize pixel area (1000 by default). If reducing the texture
        below that size produces no visible difference, the reduction is treated
        as safe.

        Three cheap statistical metrics are combined to describe how "simple"
        (i.e. low-frequency) an image is:
          - Shannon entropy of the luminance histogram
          - number of distinct colors
          - standard deviation of the pixel intensity
        A "simple" image (e.g. a solid color or a near-solid one) is the fast
        path that can be shrunk aggressively. For the precise boundary the
        advisor relies on structural similarity (SSIM, see pipeline/Utilities.h):
        the on-screen reference (original resized down to the target screen size)
        is compared against successive half-resolution levels up-scaled back to
        the same size. The smallest level whose SSIM is still above
        Options::ssimThreshold is returned as the suggested size.

        The suggested size always keeps the original aspect ratio and a power-of-two
        longer side (so it stays friendly for texture compression and mipmapping);
        the shorter side is derived from the aspect ratio and aligned to
        Options::blockAlign, which for normally power-of-two textures is also a
        power of two. The longer side is never reduced below Options::minSize.

        Note: this works on uncompressed 8-bit images (compressed ones are
        decoded best-effort). It is an offline processing helper and does not
        touch any OpenGL state. */
    class OSGVERSE_RW_EXPORT TextureSizeAdvisor
    {
    public:
        struct Options
        {
            /** Screen size (longer side, in pixels) the texture is displayed in.
                This is the main parameter of the whole algorithm. */
            int targetScreenSize = 1000;

            /** Never suggest a size whose longer side is smaller than this */
            int minSize = 4;

            /** Align suggested sizes to this block size. Compressed formats (BC/BC7/ETC/ASTC)
                work on 4x4 blocks, so 4 is a safe default. */
            int blockAlign = 4;

            /** Cap of the internal analysis resolution to bound the cost */
            int maxAnalysisSize = 2048;

            /** Minimum SSIM (0..1) for a reduction to count as invisible */
            float ssimThreshold = 0.985f;

            /** Entropy (bits/pixel, luminance) below which an image is "simple" */
            float entropyThreshold = 3.0f;

            /** Distinct color count below which an image is "simple" */
            int colorCountThreshold = 256;

            /** Intensity standard deviation (0..255) below which an image is "flat" */
            float stdDevThreshold = 12.0f;

            /** Only apply a reduction when it saves at least this ratio of the size */
            float minReductionRatio = 0.9f;

            /** Use SSIM for the precise size search (metrics only when disabled) */
            bool useSSIM = true;

            /** Print extra information during analysis */
            bool verbose = false;
        };

        struct Metrics
        {
            float entropy = 0.0f;     // Shannon entropy of luminance, bits
            float stdDev = 0.0f;      // standard deviation of luminance, 0..255
            int uniqueColors = 0;     // distinct colors, -1 if more than the threshold
            float complexity = 0.0f;  // rough complexity 0..1 combined from the above
            bool isSimple = false;    // all the metrics are below their thresholds
        };

        struct Result
        {
            int originalWidth = 0, originalHeight = 0;
            int suggestedWidth = 0, suggestedHeight = 0;
            int referenceWidth = 0, referenceHeight = 0;  // on-screen reference size
            float achievedSSIM = 1.0f;  // SSIM at the suggested size, -1 if not evaluated
            bool reduced = false;       // suggested size is smaller than the original
            bool simple = false;        // whether the image is considered simple
            Metrics metrics;
        };

        /** Compute the statistical metrics of the given image. It works on an
            internal on-screen reference (see Options::targetScreenSize). */
        static Metrics computeMetrics(const osg::Image& image);
        static Metrics computeMetrics(const osg::Image& image, const Options& opt);

        /** Analyze the image and return the suggested size and related data */
        static Result analyze(const osg::Image& image);
        static Result analyze(const osg::Image& image, const Options& opt);

        /** Shrink the image in place if the advisor thinks it is safe. It never
            enlarges the image. Returns true if the image was modified. */
        static bool optimizeSize(osg::Image& image);
        static bool optimizeSize(osg::Image& image, const Options& opt, Result* out = NULL);

        /** Check if the current image size is meaningful, i.e. a reduction is
            not recommended (convenience wrapper of analyze()) */
        static bool isSizeWorthKeeping(const osg::Image& image);
        static bool isSizeWorthKeeping(const osg::Image& image, const Options& opt);

        /** Format the metrics / result as a readable string (for logging) */
        static std::string toString(const Metrics& m);
        static std::string toString(const Result& r);
    };
}

#endif
