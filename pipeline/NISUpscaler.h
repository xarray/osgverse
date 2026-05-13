#ifndef MANA_PP_NISUPSCALER_HPP
#define MANA_PP_NISUPSCALER_HPP

#include <osg/Referenced>
#include <osg/Texture2D>
#include <osg/Shader>
#include <osg/Program>
#include <osg/Camera>
#include <osg/RenderInfo>
#include <osg/StateSet>

struct NISOptimizer;
struct NISConfig;

namespace osgVerse
{
    /**
     * NVIDIA Image Scaling processor for osgVerse.
     *
     * Encapsulates NIS compute shader resources: coefficient textures,
     * uniform buffer object (UBO), and shader program.
     *
     * Typical integration in osgVerse Pipeline:
     *   1. Create NISUpscaler with desired mode (SCALER or SHARPEN)
     *   2. Call initialize() with input/output dimensions
     *   3. Wire input texture from previous pipeline stage (post-tone-mapping)
     *   4. Wire output texture to display stage
     *   5. Use NISDrawCallback on a stage camera to auto-dispatch compute
     */
    class NISUpscaler : public osg::Referenced
    {
    public:
        enum Mode
        {
            SCALER = 0,   /**< Upscale + Sharpen (input < output, 0.5 <= scale <= 1.0) */
            SHARPEN = 1   /**< Sharpen only (input == output, no scaling) */
        };

        enum HDRMode
        {
            HDR_NONE = 0,
            HDR_LINEAR = 1,
            HDR_PQ = 2
        };

        enum GPUArchitecture
        {
            NVIDIA_Generic = 0,
            AMD_Generic = 1,
            Intel_Generic = 2,
            NVIDIA_Generic_fp16 = 3
        };

        /**
         * Create NIS processor.
         * @param mode    SCALER for upscale+sharpen, SHARPEN for sharpen-only
         * @param gpuArch Target GPU architecture for thread optimization
         */
        NISUpscaler(Mode mode = SCALER, GPUArchitecture gpuArch = GPUArchitecture::NVIDIA_Generic);

        /**
         * Initialize OpenGL resources.
         *
         * Must be called before first use. Creates textures, coefficient data,
         * compiles compute shader, and uploads initial config to GPU.
         *
         * @param inputWidth    Source render resolution width (e.g. 1920)
         * @param inputHeight   Source render resolution height (e.g. 1080)
         * @param outputWidth   Target display resolution width (e.g. 2560)
         * @param outputHeight  Target display resolution height (e.g. 1440)
         * @param sharpness     Sharpness [0.0, 1.0], default 0.5
         * @param hdr           HDR mode for tone-mapped HDR input
         * @param computeGLSL   Compute shader code, empty to use default code
         * @return true if all GL resources created successfully
         */
        bool initialize(int inputWidth, int inputHeight,
                        int outputWidth, int outputHeight,
                        float sharpness = 0.5f, HDRMode hdr = HDR_NONE,
                        const std::string& computeGLSL = std::string());

        /**
         * Reconfigure sharpness/HDR. Cheaper than full reinitialize.
         * Call when sharpness slider changes at runtime.
         * @return true if config valid and UBO updated
         */
        bool updateConfig(float sharpness, HDRMode hdr = HDR_NONE);

        /**
         * Dispatch compute shader. Binds textures/UBO and calls glDispatchCompute.
         *
         * Call this from a draw callback (see NISDrawCallback) or manually
         * at the correct pipeline point (after tone-mapping, before display).
         *
         * @param state Current OSG GL state (provides context ID and extensions)
         */
        void dispatch(osg::State* state);

        /**
         * Convenience: set input texture from previous pipeline stage's output.
         * The texture should contain tone-mapped, gamma-corrected color.
         */
        void setInputTexture(osg::Texture2D* tex) { _inputTexture = tex; }

        /**
         * Convenience: set output texture (will be written by compute shader).
         * This texture should be bound to the next stage (display) as input.
         */
        void setOutputTexture(osg::Texture2D* tex) { _outputTexture = tex; }

        // Direct accessors for pipeline wiring --------------------------------
        osg::Texture2D* getInputTexture() const  { return _inputTexture.get(); }
        osg::Texture2D* getOutputTexture() const { return _outputTexture.get(); }
        osg::Program*   getProgram() const       { return _program.get(); }

        Mode     getMode() const  { return _mode; }
        int      getInputWidth() const   { return _inputWidth; }
        int      getInputHeight() const  { return _inputHeight; }
        int      getOutputWidth() const  { return _outputWidth; }
        int      getOutputHeight() const { return _outputHeight; }
        uint32_t getBlockWidth() const   { return _blockWidth; }
        uint32_t getBlockHeight() const  { return _blockHeight; }
        uint32_t getThreadGroupSize() const { return _threadGroupSize; }
        bool     isInitialized() const   { return _initialized; }

        const char* getModeName() const
        { return (_mode == SCALER) ? "NVScaler" : "NVSharpen"; }

        /**
         * Apply shader to StateSet (for pipeline stage setup).
         * Note: actual GL bindings happen in dispatch().
         */
        void applyShader(osg::StateSet* ss) const;

    protected:
        virtual ~NISUpscaler();

        bool createTextures(int w, int h);
        bool createOutputTexture(int w, int h);
        bool createCoefficientTextures();
        bool createShaderProgram(const std::string& computeGLSL);
        void updateUniformBuffer();
        void destroyGLObjects();

        NISOptimizer* _optimizer;
        NISConfig* _config;
        Mode _mode;
        HDRMode _hdrMode;

        int _inputWidth, _inputHeight;
        int _outputWidth, _outputHeight;
        float _sharpness;

        uint32_t _blockWidth, _blockHeight, _threadGroupSize;
        unsigned int _uboId;       // OpenGL UBO handle
        bool _initialized;

        osg::ref_ptr<osg::Texture2D> _inputTexture;
        osg::ref_ptr<osg::Texture2D> _outputTexture;
        osg::ref_ptr<osg::Texture2D> _coefScalerTexture;
        osg::ref_ptr<osg::Texture2D> _coefUSMTexture;
        osg::ref_ptr<osg::Program> _program;
    };

    /**
     * Camera draw callback that auto-dispatches NIS compute shader.
     *
     * Usage in osgVerse Pipeline:
     *   stage->camera->setPreDrawCallback(new NISDrawCallback(nis));
     *
     * Or on any RTT camera where you want NIS to run before that camera renders:
     *   rttCamera->setPreDrawCallback(new NISDrawCallback(nis));
     */
    class NISDrawCallback : public osg::Camera::DrawCallback
    {
    public:
        explicit NISDrawCallback(NISUpscaler* upscaler) : _upscaler(upscaler) {}

        virtual void operator()(osg::RenderInfo& renderInfo) const
        { if (_upscaler.valid()) _upscaler->dispatch(renderInfo.getState()); }

    protected:
        osg::observer_ptr<NISUpscaler> _upscaler;
    };
}

#endif
