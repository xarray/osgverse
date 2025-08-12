#include <osg/io_utils>
#include <osg/Texture2D>
#include <osgViewer/View>
#include <iostream>
#include <fstream>

#ifdef NV_ENCODER
#   include <nvEncodeAPI.h>
#   include <cuda.h>
#endif
#define RECORD_FILE 0

class CaptureCallback : public osg::Camera::DrawCallback
{
public:
    CaptureCallback(const std::string& url, int w, int h)
        : _encoder(NULL), _streamURL(url), _width(w), _height(h), _frameNumber(0)
    {
#ifdef NV_ENCODER
        int numGpu = 0, idGpu = 0; _cuContext = NULL;
        cuInit(0); cuDeviceGetCount(&numGpu);
        if (idGpu < 0 || idGpu >= numGpu) return;
        createCudaContext(&_cuContext, idGpu, CU_CTX_SCHED_BLOCKING_SYNC);

        _encodingManager = initializeNVENC();
        _initParams = setupEncoder(_encodingManager, w, h);

        NV_ENC_CREATE_INPUT_BUFFER createInputBufferParams = { 0 };
        createInputBufferParams.version = NV_ENC_CREATE_INPUT_BUFFER_VER;
        createInputBufferParams.width = w; createInputBufferParams.height = h;
        createInputBufferParams.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
        createInputBufferParams.bufferFmt = NV_ENC_BUFFER_FORMAT_ABGR;
        if (_encodingManager->nvEncCreateInputBuffer(_encoder, &createInputBufferParams) != NV_ENC_SUCCESS)
        {
            OSG_WARN << "Failed to create input: " << _encodingManager->nvEncGetLastErrorString(_encoder) << std::endl;
        }

        NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBufferParams = { 0 };
        createBitstreamBufferParams.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
        createBitstreamBufferParams.size = 2 * 2048 * 1024;
        createBitstreamBufferParams.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
        if (_encodingManager->nvEncCreateBitstreamBuffer(_encoder, &createBitstreamBufferParams) != NV_ENC_SUCCESS)
        {
            OSG_WARN << "Failed to create output: " << _encodingManager->nvEncGetLastErrorString(_encoder) << std::endl;
        }

        _inputBuffer = createInputBufferParams.inputBuffer;
        _outputBuffer = createBitstreamBufferParams.bitstreamBuffer;
#endif
        _msWriter = osgDB::Registry::instance()->getReaderWriterForExtension("verse_ms");

#if RECORD_FILE
        _streamFile = new std::ofstream("../record.h265", std::ios::out | std::ios::binary);
#endif
    }

    virtual ~CaptureCallback()
    {
#if RECORD_FILE
        _streamFile->close(); delete _streamFile;
#endif

#ifdef NV_ENCODER
        if (_inputBuffer) _encodingManager->nvEncDestroyInputBuffer(_encoder, _inputBuffer);
        if (_outputBuffer) _encodingManager->nvEncDestroyBitstreamBuffer(_encoder, _outputBuffer);
        _encodingManager->nvEncDestroyEncoder(_encoder);
        delete _encodingManager; cuCtxDestroy(_cuContext);
#endif
    }

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
        glReadBuffer(GL_BACK);  // read from back buffer (gc must be double-buffered)
#endif
        _frameNumber++;

#ifdef NV_ENCODER
        if (_encoder)
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->readPixels(0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_BYTE);

            //std::vector<std::vector<unsigned char>> yuvResult = osgVerse::convertRGBtoYUV(image.get());
            //if (yuvResult.size() != 3) return;

            // Lock input buffer
            NV_ENC_LOCK_INPUT_BUFFER lockInputBufferParams = { 0 };
            lockInputBufferParams.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
            lockInputBufferParams.inputBuffer = _inputBuffer;

            uint8_t* lockedInputBuffer = nullptr;
            NVENCSTATUS status = _encodingManager->nvEncLockInputBuffer(_encoder, &lockInputBufferParams);
            if (status != NV_ENC_SUCCESS) { OSG_WARN << "Failed to lock input buffer" << std::endl; return; }
            lockedInputBuffer = (uint8_t*)lockInputBufferParams.bufferDataPtr;

            // Copy to input buffer
            //YV12_to_YUV444(yuvResult[0].data(), yuvResult[1].data(), yuvResult[2].data(), _width, _height, lockedInputBuffer);
            memcpy(lockedInputBuffer, image->data(), image->getTotalSizeInBytes());
            status = _encodingManager->nvEncUnlockInputBuffer(_encoder, _inputBuffer);
            if (status != NV_ENC_SUCCESS) { OSG_WARN << "Failed to unlock input buffer" << std::endl; return; }

            // Encode buffer data
            NV_ENC_PIC_PARAMS picParams = { 0 };
            picParams.version = NV_ENC_PIC_PARAMS_VER;
            picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
            picParams.bufferFmt = NV_ENC_BUFFER_FORMAT_ABGR;
            picParams.inputBuffer = _inputBuffer;
            picParams.outputBitstream = _outputBuffer;
            picParams.inputWidth = _width; picParams.inputHeight = _height;
            picParams.inputPitch = picParams.inputWidth;
            picParams.completionEvent = 0; picParams.encodePicFlags = 0;
            picParams.inputTimeStamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            picParams.inputDuration = 1000000.0 / 30.0;
            status = _encodingManager->nvEncEncodePicture(_encoder, &picParams);
            if (status != NV_ENC_SUCCESS && status != NV_ENC_ERR_NEED_MORE_INPUT)
            {
                OSG_WARN << "Failed to encode picture: " << status << ", "
                    << _encodingManager->nvEncGetLastErrorString(_encoder) << std::endl; return;
            }

            // Lock output buffer
            NV_ENC_LOCK_BITSTREAM lockBitstreamParams = { 0 };
            lockBitstreamParams.version = NV_ENC_LOCK_BITSTREAM_VER;
            lockBitstreamParams.outputBitstream = _outputBuffer;
            status = _encodingManager->nvEncLockBitstream(_encoder, &lockBitstreamParams);
            if (status != NV_ENC_SUCCESS) { OSG_WARN << "Failed to lock bitstream" << std::endl; return; }

            // Handle encoded data
#if RECORD_FILE
            std::cout << "Encoded frame: " << lockBitstreamParams.bitstreamSizeInBytes << " bytes, PTR = "
                      << lockBitstreamParams.bitstreamBufferPtr << std::endl;
            _streamFile->write((char*)lockBitstreamParams.bitstreamBufferPtr, lockBitstreamParams.bitstreamSizeInBytes);
#endif
            status = _encodingManager->nvEncUnlockBitstream(_encoder, _outputBuffer);
            if (status != NV_ENC_SUCCESS) { OSG_WARN << "Failed to unlock bitstream" << std::endl; return; }
        }
#else
        if (_msWriter.valid() && _frameNumber > 1)
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->readPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE);
            image->flipVertical();  // low-performance, just for example here
            _msWriter->writeImage(*image, _streamURL);
        }
        else
            OSG_WARN << "Invalid readerwriter verse_ms?\n";
#endif
    }

#ifdef NV_ENCODER
    static NV_ENCODE_API_FUNCTION_LIST* initializeNVENC()
    {
        NV_ENCODE_API_FUNCTION_LIST* pNvEnc = new NV_ENCODE_API_FUNCTION_LIST;
        memset(pNvEnc, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
        pNvEnc->version = NV_ENCODE_API_FUNCTION_LIST_VER;
        if (NvEncodeAPICreateInstance(pNvEnc) != NV_ENC_SUCCESS)
        {
            OSG_FATAL << "Failed to create NVENC instance" << std::endl;
            delete pNvEnc; return nullptr;
        }
        return pNvEnc;
    }

    NV_ENC_INITIALIZE_PARAMS setupEncoder(NV_ENCODE_API_FUNCTION_LIST* pNvEnc, int width, int height)
    {
        NV_ENC_INITIALIZE_PARAMS initParams = { 0 };
        initParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
        initParams.encodeGUID = NV_ENC_CODEC_HEVC_GUID;
        initParams.presetGUID = NV_ENC_PRESET_P3_GUID;
        initParams.bufferFormat = NV_ENC_BUFFER_FORMAT_ABGR;
        initParams.tuningInfo = NV_ENC_TUNING_INFO_HIGH_QUALITY;
        initParams.encodeWidth = width; initParams.encodeHeight = height;
        initParams.darWidth = width; initParams.darHeight = height;
        initParams.maxEncodeWidth = width; initParams.maxEncodeHeight = height;
        initParams.frameRateNum = 30; initParams.frameRateDen = 1;
        initParams.enableOutputInVidmem = 0; initParams.enablePTD = 1;

        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS sessionParams = { 0 };
        sessionParams.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        sessionParams.apiVersion = NVENCAPI_VERSION;
        sessionParams.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
        sessionParams.device = _cuContext;

        NVENCSTATUS status = pNvEnc->nvEncOpenEncodeSessionEx(&sessionParams, &_encoder);
        if (status != NV_ENC_SUCCESS)
        {
            OSG_WARN << "Failed to open encode session: " << status << std::endl;
            return initParams;
        }

        NV_ENC_PRESET_CONFIG presetConfig = { 0 };
        presetConfig.version = NV_ENC_PRESET_CONFIG_VER;
        presetConfig.presetCfg.version = NV_ENC_CONFIG_VER;
        pNvEnc->nvEncGetEncodePresetConfigEx(
            _encoder, initParams.encodeGUID, initParams.presetGUID, initParams.tuningInfo, &presetConfig);

        NV_ENC_CONFIG encodeConfig = { 0 };
        encodeConfig.version = NV_ENC_CONFIG_VER;
        memcpy(&encodeConfig, &presetConfig.presetCfg, sizeof(NV_ENC_CONFIG));
        encodeConfig.profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
        encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
        encodeConfig.encodeCodecConfig.h264Config.chromaFormatIDC = 3;
        encodeConfig.encodeCodecConfig.h264Config.idrPeriod = encodeConfig.gopLength;
        encodeConfig.encodeCodecConfig.hevcConfig.chromaFormatIDC = 3;
        encodeConfig.encodeCodecConfig.hevcConfig.idrPeriod = encodeConfig.gopLength;
        initParams.encodeConfig = &encodeConfig;

        status = pNvEnc->nvEncInitializeEncoder(_encoder, &initParams);
        if (status != NV_ENC_SUCCESS)
        {
            OSG_WARN << "Failed to initialize encoder: " << status << ", "
                << pNvEnc->nvEncGetLastErrorString(_encoder) << std::endl;
        }
        return initParams;
    }

    static void createCudaContext(CUcontext* cuContext, int iGpu, unsigned int flags)
    {
        CUdevice cuDevice = 0; char deviceName[80];
        cuDeviceGet(&cuDevice, iGpu);
        cuDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
        cuCtxCreate(cuContext, flags, cuDevice);
        OSG_NOTICE << "GPU in use: " << deviceName << std::endl;
    }
#endif

protected:
#ifdef NV_ENCODER
    static uint8_t bilinear_interpolate(const uint8_t* src, int src_w, int src_h, float x, float y)
    {
        int x1 = static_cast<int>(x), y1 = static_cast<int>(y);
        int x2 = std::min(x1 + 1, src_w - 1), y2 = std::min(y1 + 1, src_h - 1);
        float dx = x - x1, dy = y - y1;
        uint8_t f11 = src[y1 * src_w + x1]; uint8_t f21 = src[y1 * src_w + x2];
        uint8_t f12 = src[y2 * src_w + x1]; uint8_t f22 = src[y2 * src_w + x2];
        float value = (1 - dx) * (1 - dy) * f11 + dx * (1 - dy) * f21 + (1 - dx) * dy * f12 + dx * dy * f22;
        return static_cast<uint8_t>(value + 0.5f);
    }

    static void YV12_to_YUV444(const uint8_t* Y, const uint8_t* U, const uint8_t* V, int width, int height, uint8_t* yuv444)
    {
        std::copy(Y, Y + width * height, yuv444);
        uint8_t* U_dst = yuv444 + width * height;
        uint8_t* V_dst = U_dst + width * height;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
            {
                float src_x = x / 2.0f, src_y = y / 2.0f;
                U_dst[y * width + x] = bilinear_interpolate(U, width / 2, height / 2, src_x, src_y);
                V_dst[y * width + x] = bilinear_interpolate(V, width / 2, height / 2, src_x, src_y);
            }
    }

    NV_ENCODE_API_FUNCTION_LIST* _encodingManager;
    NV_ENC_INITIALIZE_PARAMS _initParams;
    NV_ENC_INPUT_PTR _inputBuffer;
    NV_ENC_OUTPUT_PTR _outputBuffer;
    void* _encoder;
    CUcontext _cuContext;
#endif
    osg::ref_ptr<osgDB::ReaderWriter> _msWriter;
    std::ofstream* _streamFile;
    std::string _streamURL;
    int _width, _height;
    mutable int _frameNumber;
};
