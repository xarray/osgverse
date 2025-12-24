#include "OnnxRuntimeEngine.h"
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <sstream>
using namespace osgVerse;

namespace
{
    class OnnxWrapper
    {
    public:
        OnnxWrapper(const std::wstring& modelPath, OnnxInferencer::DeviceType type, int deviceID)
            : _env(ORT_LOGGING_LEVEL_WARNING, "OnnxInferencer"), _session(nullptr)
        {
            Ort::SessionOptions session_options; Ort::Status status;
            session_options.SetIntraOpNumThreads(1);
            session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef VERSE_USE_CUDA
            if (type == OnnxInferencer::CUDA)
                status = Ort::Status(OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, deviceID));
#endif
            //else if (type == OnnxInferencer::OneDNN)
            //    status = Ort::Status(OrtSessionOptionsAppendExecutionProvider_Dnnl(session_options, deviceID));
            //else if (type == OnnxInferencer::TensorRT)
            //    status = Ort::Status(OrtSessionOptionsAppendExecutionProvider_Tensorrt(session_options, deviceID));

            if (!status.IsOK())
                { OSG_NOTICE << "[OnnxInferencer] Failed to load provider: " << status.GetErrorMessage() << "\n"; }
            _session = Ort::Session(_env, modelPath.c_str(), session_options);
            initializeModelInformation();
        }

        std::string getTensorInformation(const std::string& name, bool asInput) const
        {
            std::map<std::string, std::vector<int64_t>>::const_iterator it0 =
                asInput ? _modelInfo.inputShapes.find(name) : _modelInfo.outputShapes.find(name);
            std::map<std::string, ONNXTensorElementDataType>::const_iterator it1 =
                asInput ? _modelInfo.inputTypes.find(name) : _modelInfo.outputTypes.find(name);

            std::string info(name);
            if (it1 != _modelInfo.inputTypes.end() || it1 != _modelInfo.outputTypes.end())
                info += std::string("/") + getDataTypeName(it1->second);
            if (it0 != _modelInfo.inputShapes.end() || it0 != _modelInfo.outputShapes.end())
            for (size_t i = 0; i < it0->second.size(); ++i)
                info += "/" + std::to_string(it0->second[i]);
            return info;
        }

        struct ModelInformation
        {
            std::vector<std::string> inputNames, outputNames;
            std::map<std::string, std::vector<int64_t>> inputShapes, outputShapes;
            std::map<std::string, ONNXTensorElementDataType> inputTypes, outputTypes;
            std::string description;
        };
        const ModelInformation& getModelInformation() const { return _modelInfo; }

        static const char* getDataTypeName(ONNXTensorElementDataType type)
        {
            switch (type)
            {
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED: return "Undefined";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: return "Float";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: return "UInt8";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8: return "Int8";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16: return "UInt16";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16: return "Int16";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32: return "Int32";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: return "Int64";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING: return "String";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL: return "Boolean";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16: return "HalfFloat";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE: return "Double";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32: return "UInt32";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64: return "UInt64";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_COMPLEX64: return "Complex64";
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_COMPLEX128: return "Complex128";
            }
            return "Unknown";
        }

        static void runCallback(void* userData, OrtValue** outputs, size_t numOutputs, OrtStatus* status)
        {
            // TODO
        }

        Ort::Value createInput(const std::vector<osg::Image*>& images, const std::string& checkInName)
        {
            Ort::Value tensor; osg::Image* firstImage = NULL;
            if (images.empty()) return tensor; else firstImage = images.front();

            int64_t channels = osg::Image::computeNumComponents(firstImage->getPixelFormat());
            std::vector<int64_t> shapes = { (int64_t)images.size(), channels, firstImage->t(), firstImage->s() };
            switch (firstImage->getDataType())
            {
            case GL_UNSIGNED_BYTE:
                if (checkValidation(checkInName, shapes, ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8, true))
                {
                    tensor = Ort::Value::CreateTensor(_allocator, shapes.data(), shapes.size(),
                                                      ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8);
                    unsigned char* ptr = tensor.GetTensorMutableData<unsigned char>();
                    // TODO
                } break;
            case GL_FLOAT:
                if (checkValidation(checkInName, shapes, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, true))
                {
                    tensor = Ort::Value::CreateTensor(_allocator, shapes.data(), shapes.size(),
                                                      ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
                    float* ptr = tensor.GetTensorMutableData<float>();
                    // TODO
                } break;
            default:
                OSG_NOTICE << "[OnnxInferencer] Input image type unsupported: " << std::hex
                           << firstImage->getDataType() << "\n"; break;
            }
            return tensor;
        }

        bool checkValidation(const std::string& n, const std::vector<int64_t>& shapes,
                             ONNXTensorElementDataType type, bool asInput)
        {
            const std::vector<std::string>& names = asInput ? _modelInfo.inputNames : _modelInfo.outputNames;
            if (std::find(names.begin(), names.end(), n) == names.end()) return false;

            std::vector<int64_t>& shapes0 = asInput ? _modelInfo.inputShapes[n] : _modelInfo.outputShapes[n];
            ONNXTensorElementDataType type0 = asInput ? _modelInfo.inputTypes[n] : _modelInfo.outputTypes[n];
            if (type0 == type && shapes0.size() == shapes.size())
            {
                bool diff = false;
                for (size_t i = 0; i < shapes.size(); ++i)
                { if (shapes0[i] != shapes[i]) diff = true; }
                if (!diff) return true;
            }

            std::string info; for (size_t i = 0; i < shapes.size(); ++i) info += "/" + std::to_string(shapes[i]);
            OSG_NOTICE << "[OnnxInferencer] Failed to check validation of tensor: Expected = "
                       << getTensorInformation(n, asInput) << "; Provided = " << n << "/"
                       << getDataTypeName(type) << info << std::endl; return false;
        }

        void runAsync(const std::vector<std::string>& inNameStrings, const std::vector<Ort::Value>& inTensors)
        {
            std::vector<const char*> inNames, outNames; std::vector<Ort::Value> outTensors;
            const std::vector<std::string>& inputNames = inNameStrings.empty() ? _modelInfo.inputNames : inNameStrings;
            for (size_t i = 0; i < inputNames.size(); ++i) inNames.push_back(inputNames[i].c_str());
            for (size_t i = 0; i < _modelInfo.outputNames.size(); ++i) outNames.push_back(_modelInfo.outputNames[i].c_str());

            size_t inTensorSize = osg::minimum(inTensors.size(), inNames.size());
            Ort::AllocatorWithDefaultOptions allocator;
            for (size_t i = 0; i < outNames.size(); ++i)
            {
                const char* name = outNames[i]; size_t elementCount = 1;
                ONNXTensorElementDataType type = _modelInfo.outputTypes[name];
                std::vector<int64_t> shapes = _modelInfo.outputShapes[name];
                for (size_t s = 0; s < shapes.size(); ++s) { if (shapes[s] > 0) elementCount *= shapes[s]; }

                Ort::Value outTensor = Ort::Value::CreateTensor(allocator, shapes.data(), shapes.size(), type);
                outTensors.push_back(std::move(outTensor));
            }

            Ort::RunOptions options;
            _session.RunAsync(options, inNames.data(), inTensors.data(), inTensorSize,
                outNames.data(), outTensors.data(), outTensors.size(), &runCallback, this);
        }

    protected:
        void initializeModelInformation()
        {
            Ort::ModelMetadata metadata = _session.GetModelMetadata();
            Ort::AllocatedStringPtr descr_ptr = metadata.GetDescriptionAllocated(_allocator);
            if (descr_ptr) _modelInfo.description = std::string(descr_ptr.get());

            size_t num_inputs = _session.GetInputCount();
            for (size_t i = 0; i < num_inputs; ++i)
            {
                Ort::AllocatedStringPtr name_ptr = _session.GetInputNameAllocated(i, _allocator);
                Ort::TypeInfo type_info = _session.GetInputTypeInfo(i);
                Ort::ConstTensorTypeAndShapeInfo tensor_info = type_info.GetTensorTypeAndShapeInfo();
                
                std::string name(name_ptr.get()); _modelInfo.inputNames.push_back(name);
                _modelInfo.inputShapes[name] = tensor_info.GetShape();
                _modelInfo.inputTypes[name] = tensor_info.GetElementType();
            }

            size_t num_outputs = _session.GetOutputCount();
            for (size_t i = 0; i < num_outputs; ++i)
            {
                Ort::AllocatedStringPtr name_ptr = _session.GetOutputNameAllocated(i, _allocator);
                Ort::TypeInfo type_info = _session.GetOutputTypeInfo(i);
                Ort::ConstTensorTypeAndShapeInfo tensor_info = type_info.GetTensorTypeAndShapeInfo();

                std::string name(name_ptr.get()); _modelInfo.outputNames.push_back(name);
                _modelInfo.outputShapes[name] = tensor_info.GetShape();
                _modelInfo.outputTypes[name] = tensor_info.GetElementType();
            }
        }

        Ort::Env _env;
        Ort::Session _session;
        Ort::AllocatorWithDefaultOptions _allocator;
        ModelInformation _modelInfo;
    };
}

OnnxInferencer::OnnxInferencer(const std::wstring& modelPath, DeviceType type, int deviceID)
    : _handle(NULL)
{
    try { _handle = new OnnxWrapper(modelPath, type, deviceID); }
    catch (std::exception& e) { OSG_WARN << "[OnnxInferencer] Failed to load model: " << e.what(); }
}

OnnxInferencer::~OnnxInferencer()
{ if (_handle) delete _handle; }

std::string OnnxInferencer::getModelDescription() const
{
    OnnxWrapper* w = (OnnxWrapper*)_handle; if (!w) return "";
    const OnnxWrapper::ModelInformation& info = w->getModelInformation();
    std::map<std::string, ONNXTensorElementDataType>::const_iterator it2;
    std::stringstream ss; ss << info.description;

    ss << std::endl << "Input layers: ";
    for (size_t i = 0; i < info.inputNames.size(); ++i)
        ss << w->getTensorInformation(info.inputNames[i], true) << "; ";
    ss << std::endl << "Output layers: ";
    for (size_t i = 0; i < info.outputNames.size(); ++i)
        ss << w->getTensorInformation(info.outputNames[i], false) << "; ";
    ss << std::endl; return ss.str();
}

std::vector<std::string> OnnxInferencer::getModelLayerNames(bool in) const
{
    std::vector<std::string> s; OnnxWrapper* w = (OnnxWrapper*)_handle;
    if (w) s = in ? w->getModelInformation().inputNames : w->getModelInformation().outputNames; return s;
}

std::vector<int64_t> OnnxInferencer::getModelShapes(bool in, const std::string& name) const
{
    std::vector<int64_t> s; OnnxWrapper* w = (OnnxWrapper*)_handle;
    if (w)
    {
        const OnnxWrapper::ModelInformation& info = w->getModelInformation();
        std::map<std::string, std::vector<int64_t>>::const_iterator itr;
        if (in) { itr = info.inputShapes.find(name); if (itr != info.inputShapes.end()) return itr->second; }
        else { itr = info.outputShapes.find(name); if (itr != info.outputShapes.end()) return itr->second; }
    }
    return s;
}

OnnxInferencer::DataType OnnxInferencer::getModelDataType(bool in, const std::string& name) const
{
    OnnxWrapper* w = (OnnxWrapper*)_handle;
    if (w)
    {
        const OnnxWrapper::ModelInformation& info = w->getModelInformation();
        std::map<std::string, ONNXTensorElementDataType>::const_iterator itr;
        if (in) { itr = info.inputTypes.find(name); if (itr != info.inputTypes.end()) return (DataType)itr->second; }
        else { itr = info.outputTypes.find(name); if (itr != info.outputTypes.end()) return (DataType)itr->second; }
    }
    return UnknownData;
}

bool OnnxInferencer::addInput(const std::vector<osg::Image*>& images, const std::string& inName)
{
    OnnxWrapper* w = (OnnxWrapper*)_handle; if (!w) return false;
    Ort::Value value = w->createInput(images, inName);
    // TODO
    return true;
}
