#include <osg/io_utils>
#include <osg/Texture2D>
#include <osgDB/Archive>
#include <osgViewer/View>
#include <iostream>
#include <fstream>

#if defined(NV_ENCODER)
#   include <nvEncodeAPI.h>
#   include <cuda.h>
#elif defined(MUSA_ENCODER)
#   include <mtEncodeAPI.h>
#endif
#define RECORD_FILE 0
#define MEDIA_SERVER 1

#if MEDIA_SERVER
class HttpApiCallback : public osgVerse::UserCallback
{
public:
    HttpApiCallback(const std::string& name) : osgVerse::UserCallback(name) {}
    virtual bool run(osg::Object* object, Parameters& in, Parameters& out) const
    {
        if (in.empty()) return false;
        osgVerse::StringObject* so = static_cast<osgVerse::StringObject*>(in[0].get());
        if (!so || (so && so->values.size() < 2)) return false;

        // TODO
        std::cout << so->values[0] << ", " << so->values[1] << "\n";
        return true;
    }
};
#endif

#ifdef MUSA_ENCODER
struct MTEncInputFrame
{
    void* inputPtr = nullptr;
    uint32_t chromaOffsets[2];
    uint32_t numChromaPlanes;
    uint32_t pitch, chromaPitch;
    MT_ENC_BUFFER_FORMAT bufferFormat;
    MT_ENC_INPUT_RESOURCE_TYPE resourceType;
};
#endif

class CaptureCallback : public osg::Camera::DrawCallback
{
public:
    CaptureCallback(const std::string& url, int w, int h)
        : _streamURL(url), _width(w), _height(h), _frameNumber(0)
    {
#if defined(NV_ENCODER)
        int numGpu = 0, idGpu = 0; _cuContext = NULL;
        cuInit(0); cuDeviceGetCount(&numGpu);
        if (idGpu < 0 || idGpu >= numGpu) return;
        createCudaContext(&_cuContext, idGpu, CU_CTX_SCHED_BLOCKING_SYNC);

        _encodingManager = initializeNVENC(); _encoder = NULL;
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
#elif defined(MUSA_ENCODER)
        _encodingManager = initializeMTENC(); _encoder = NULL;
        _initParams = setupEncoder(_encodingManager, w, h);

        memset(&_inputFrame, 0, sizeof(MTEncInputFrame));
        _inputFrame.inputPtr = malloc(4 * w * h);  // frame size of RGBA
        _inputFrame.chromaOffsets[0] = 0; _inputFrame.chromaOffsets[1] = 0;
        _inputFrame.numChromaPlanes = 0; _inputFrame.chromaPitch = 0; _inputFrame.pitch = 0;
        _inputFrame.bufferFormat = MT_ENC_BUFFER_FORMAT_RGBA;
        _inputFrame.resourceType = MT_ENC_INPUT_RESOURCE_TYPE_OPENGL_TEX;
        
        MT_ENC_CREATE_OUTPUT_BUFFER createOutputBufferParams = { 0 };
        createOutputBufferParams.version = MTENCAPI_VERSION;
        if (_encodingManager->mtEncCreateOutputBuffer(_encoder, &createOutputBufferParams) != MT_ENC_SUCCESS)
        { OSG_WARN << "Failed to create output" << std::endl; }
        else _outputBuffer = createOutputBufferParams.outputBuffer;
#endif
        _msWriter = osgDB::Registry::instance()->getReaderWriterForExtension("verse_ms");

#if MEDIA_SERVER
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
        options->setPluginStringData("http", "80");
        options->setPluginStringData("rtsp", "554");
        options->setPluginStringData("rtmp", "1935");
        options->setPluginStringData("rtc", "8000");  // set RTC port: 8000
        _msServer = _msWriter->openArchive(
            "TestServer", osgDB::ReaderWriter::CREATE, 4096, options.get()).getArchive();
        _msServer->getOrCreateUserDataContainer()->addUserObject(new HttpApiCallback("HttpAPI"));
#endif
#if RECORD_FILE
        _streamFile = new std::ofstream("../record.h265", std::ios::out | std::ios::binary);
#endif
    }

    virtual ~CaptureCallback()
    {
#if RECORD_FILE
        _streamFile->close(); delete _streamFile;
#endif

#if defined(NV_ENCODER)
        if (_inputBuffer) _encodingManager->nvEncDestroyInputBuffer(_encoder, _inputBuffer);
        if (_outputBuffer) _encodingManager->nvEncDestroyBitstreamBuffer(_encoder, _outputBuffer);
        _encodingManager->nvEncDestroyEncoder(_encoder);
        delete _encodingManager; cuCtxDestroy(_cuContext);
#elif defined(MUSA_ENCODER)
        if (_inputFrame.inputPtr != nullptr) free(_inputFrame.inputPtr);
        if (_outputBuffer) _encodingManager->mtEncReleaseOutputBuffer(_encoder, _outputBuffer);
        _encodingManager->mtEncReleaseEncoder(_encoder);
        delete _encodingManager;
#endif

#if MEDIA_SERVER
        if (_msServer.valid()) _msServer->close();
#endif
    }

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
        glReadBuffer(GL_BACK);  // read from back buffer (gc must be double-buffered)
#endif
        _frameNumber++;

        if (_encoder)
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->readPixels(0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_BYTE);

#if defined(NV_ENCODER)
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
            unsigned char* ptr = (unsigned char*)lockBitstreamParams.bitstreamBufferPtr;
            if (_msWriter.valid())
            {
                osg::ref_ptr<osgVerse::EncodedFrameObject> frame = new osgVerse::EncodedFrameObject(
                    osgVerse::EncodedFrameObject::FRAME_H264, _width, _height, lockBitstreamParams.outputTimeStamp);
                frame->getData().assign(ptr, ptr + lockBitstreamParams.bitstreamSizeInBytes);
                _msWriter->writeObject(*frame, _streamURL);
            }
#if RECORD_FILE
            std::cout << "Encoded frame: " << lockBitstreamParams.bitstreamSizeInBytes << " bytes" << std::endl;
            _streamFile->write((char*)ptr, lockBitstreamParams.bitstreamSizeInBytes);
#endif
            status = _encodingManager->nvEncUnlockBitstream(_encoder, _outputBuffer);
            if (status != NV_ENC_SUCCESS) { OSG_WARN << "Failed to unlock bitstream" << std::endl; return; }
#elif defined(MUSA_ENCODER)
            // Copy to input buffer
            memcpy(_inputFrame.inputPtr, image->data(), image->getTotalSizeInBytes());

            MT_ENC_MAP_RESOURCE mapInputResource = { 0 };
            mapInputResource.version = MTENCAPI_VERSION;
            mapInputResource.resourceType = _inputFrame.resourceType;
            mapInputResource.resourceToMap = _inputFrame.inputPtr;
            _encodingManager->mtEncMapResource(_encoder, &mapInputResource);
            
            MT_ENC_INPUT_PTR inputBuffer = mapInputResource.mappedResource;
            MT_ENC_PIC_PARAMS picParams = { 0 };
            picParams.version = MTENCAPI_VERSION;
            picParams.bufferFmt = MT_ENC_BUFFER_FORMAT_RGBA;
            picParams.inputWidth = _width; picParams.inputHeight = _height;
            picParams.inputPitch = picParams.inputWidth;
            picParams.inputBufferType = MT_ENC_BUFFER_TYPE_SYSTEM_MEMORY;
            picParams.inputBuffer = inputBuffer;
            picParams.outputBuffer = _outputBuffer;
            picParams.completionEvent = 0;

            MTENCSTATUS status = _encodingManager->mtEncEncodeFrame(_encoder, &picParams);
            if (status != MT_ENC_SUCCESS) { OSG_WARN << "Failed to encode image" << std::endl; }

            MT_ENC_LOCK_BUFFER lockBuffer = { 0 };
            lockBuffer.version = MTENCAPI_VERSION;
            _encodingManager->mtEncLockOutputBuffer(_encoder, &lockBuffer);

            uint8_t* ptr = (uint8_t*)lockBuffer.outputBufferPtr;
            if (_msWriter.valid())
            {
                osg::ref_ptr<osgVerse::EncodedFrameObject> frame = new osgVerse::EncodedFrameObject(
                    osgVerse::EncodedFrameObject::FRAME_H264, _width, _height, 0);
                frame->getData().assign(ptr, ptr + lockBuffer.outputBufferSizeInBytes);
                _msWriter->writeObject(*frame, _streamURL);
            }
            _encodingManager->mtEncUnlockOutputBuffer(_encoder, lockBuffer.lockedOutputBuffer);
            _encodingManager->mtEncUnmapResource(_encoder, inputBuffer);
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
    }

#if defined(NV_ENCODER)
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
        initParams.encodeGUID = NV_ENC_CODEC_H264_GUID;
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
        encodeConfig.profileGUID = NV_ENC_H264_PROFILE_HIGH_444_GUID;
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
#elif defined(MUSA_ENCODER)
    static MT_ENCODE_API_FUNCTION_LIST* initializeMTENC()
    {
#if defined(_WIN64)
        HMODULE hModule = LoadLibrary(TEXT("mtencodeapi64.dll"));
#elif defined(_WIN32)
        HMODULE hModule = LoadLibrary(TEXT("mtencodeapi32.dll"));
#else
        void* hModule = dlopen("libencode_musa.so", RTLD_LAZY);
#endif
        if (hModule == NULL)
        {
            OSG_FATAL << "Failed to find MTENC library" << std::endl;
            return nullptr;
        }
#if defined(_WIN32)
        typedef MTENCSTATUS(MTENCAPI* MTEncodeAPICreateInstance_Type)(MT_ENCODE_API_FUNCTION_LIST*);
        MTEncodeAPICreateInstance_Type pfCreateInstance =
            (MTEncodeAPICreateInstance_Type)GetProcAddress(hModule, "MTEncodeAPICreateInstance");
#else
        typedef MTENCSTATUS(MTENCAPI* MTEncodeAPICreateInstance_Type)(MT_ENCODE_API_FUNCTION_LIST*);
        MTEncodeAPICreateInstance_Type pfCreateInstance =
            (MTEncodeAPICreateInstance_Type)dlsym(hModule, "MTEncodeAPICreateInstance");
#endif
        if (!pfCreateInstance)
        {
            OSG_FATAL << "Failed to create MTENC instance" << std::endl;
            return nullptr;
        }

        MT_ENCODE_API_FUNCTION_LIST* pMtEnc = new MT_ENCODE_API_FUNCTION_LIST;
        memset(pMtEnc, 0, sizeof(MT_ENCODE_API_FUNCTION_LIST));
        pMtEnc->version = MTENCAPI_VERSION;
        if (pfCreateInstance(pMtEnc) != MT_ENC_SUCCESS)
        {
            OSG_FATAL << "Failed to create MTENC instance" << std::endl;
            delete pMtEnc; return nullptr;
        }
        return pMtEnc;
    }

    MT_ENC_INIT_PARAMS setupEncoder(MT_ENCODE_API_FUNCTION_LIST* pMtEnc, int width, int height)
    {
        MT_ENC_INIT_PARAMS initParams = { 0 };
        initParams.version = MTENCAPI_VERSION;
        initParams.encodeID = MT_ENC_CODEC_ID_H264;
        initParams.presetID = MT_ENC_PRESET_ID_DEFAULT;
        initParams.encodeWidth = width; initParams.encodeHeight = height;
        initParams.maxEncodeWidth = width; initParams.maxEncodeHeight = height;
        initParams.frameRateNum = 30; initParams.frameRateDen = 1;

        MT_ENC_CREATE_ENCODER_PARAMS sessionParams = { 0 };
        sessionParams.version = MTENCAPI_VERSION;
        sessionParams.deviceType = MT_ENC_DEVICE_TYPE_OPENGL;
        sessionParams.device = NULL;

        MTENCSTATUS status = pMtEnc->mtEncCreateEncoder(&sessionParams, &_encoder);
        if (status != MT_ENC_SUCCESS)
        {
            OSG_WARN << "Failed to open encode session: " << status << std::endl;
            return initParams;
        }

        MT_ENC_PRESET_CONFIG presetConfig = { 0 };
        presetConfig.version = MTENCAPI_VERSION;
        presetConfig.presetCfg.version = MTENCAPI_VERSION;
        pMtEnc->mtEncGetPresetConfig(
            _encoder, initParams.encodeID, initParams.presetID, &presetConfig);

        MT_ENC_CONFIG encodeConfig = { 0 };
        encodeConfig.version = MTENCAPI_VERSION;
        memcpy(&encodeConfig, &presetConfig.presetCfg, sizeof(MT_ENC_CONFIG));
        encodeConfig.profileID = MT_ENC_CODEC_PROFILE_ID_BASELINE_H264;
        encodeConfig.rcParams.rateControlMode = MT_ENC_PARAMS_RC_CONSTQP;
        encodeConfig.encodeCodecConfig.h264Config.idrPeriod = encodeConfig.gopLength;
        encodeConfig.encodeCodecConfig.hevcConfig.idrPeriod = encodeConfig.gopLength;
        initParams.encodeConfig = &encodeConfig;

        status = pMtEnc->mtEncInitEncoder(_encoder, &initParams);
        if (status != MT_ENC_SUCCESS) { OSG_WARN << "Failed to initialize encoder: " << status << std::endl; }
        return initParams;
    }
#endif

protected:
#if defined(NV_ENCODER)
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
#elif defined(MUSA_ENCODER)
    MT_ENCODE_API_FUNCTION_LIST* _encodingManager;
    MT_ENC_INIT_PARAMS _initParams;
    MT_ENC_OUTPUT_PTR _outputBuffer;
    MTEncInputFrame _inputFrame;
    void* _encoder;
#endif

#if MEDIA_SERVER
    osg::ref_ptr<osgDB::Archive> _msServer;
#endif
    osg::ref_ptr<osgDB::ReaderWriter> _msWriter;
    std::ofstream* _streamFile;
    std::string _streamURL;
    int _width, _height;
    mutable int _frameNumber;
};
