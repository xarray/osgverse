#include "OnnxRuntimeEngine.h"
#include <onnxruntime_cxx_api.h>
#include <iostream>
using namespace osgVerse;

namespace
{
    class OnnxWrapper
    {
    public:
        OnnxWrapper(const std::wstring& modelPath, OnnxInferencer::DeviceType type, int deviceID)
            : _env(ORT_LOGGING_LEVEL_WARNING, "OnnxInferencer"), _session(nullptr)
        {
            Ort::SessionOptions session_options;
            session_options.SetIntraOpNumThreads(1);
            session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef VERSE_USE_CUDA
            if (type == OnnxInferencer::CUDA)
                Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, deviceID));
#endif
            //else if (type == OnnxInferencer::OneDNN)
            //    Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_Dnnl(session_options, deviceID));
            //else if (type == OnnxInferencer::TensorRT)
            //    Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_Tensorrt(session_options, deviceID));

            _session = Ort::Session(_env, modelPath.c_str(), session_options);
            initializeModelInformation();
        }

        struct ModelInformation
        {
            std::vector<std::string> inputNames, outputNames;
            std::map<std::string, std::vector<int64_t>> inputShapes, outputShapes;
            std::map<std::string, ONNXTensorElementDataType> inputTypes, outputTypes;
            std::string description;
        };
        const ModelInformation& getModelInformation() const { return _modelInfo; }

    protected:
        void initializeModelInformation()
        {
            Ort::AllocatorWithDefaultOptions allocator;
            Ort::ModelMetadata metadata = _session.GetModelMetadata();
            Ort::AllocatedStringPtr descr_ptr = metadata.GetDescriptionAllocated(allocator);
            if (descr_ptr) _modelInfo.description = std::string(descr_ptr.get());

            size_t num_inputs = _session.GetInputCount();
            for (size_t i = 0; i < num_inputs; ++i)
            {
                Ort::AllocatedStringPtr name_ptr = _session.GetInputNameAllocated(i, allocator);
                Ort::TypeInfo type_info = _session.GetInputTypeInfo(i);
                Ort::ConstTensorTypeAndShapeInfo tensor_info = type_info.GetTensorTypeAndShapeInfo();
                
                std::string name(name_ptr.get()); _modelInfo.inputNames.push_back(name);
                _modelInfo.inputShapes[name] = tensor_info.GetShape();
                _modelInfo.inputTypes[name] = tensor_info.GetElementType();
            }

            size_t num_outputs = _session.GetOutputCount();
            for (size_t i = 0; i < num_outputs; ++i)
            {
                Ort::AllocatedStringPtr name_ptr = _session.GetOutputNameAllocated(i, allocator);
                Ort::TypeInfo type_info = _session.GetOutputTypeInfo(i);
                Ort::ConstTensorTypeAndShapeInfo tensor_info = type_info.GetTensorTypeAndShapeInfo();

                std::string name(name_ptr.get()); _modelInfo.outputNames.push_back(name);
                _modelInfo.outputShapes[name] = tensor_info.GetShape();
                _modelInfo.outputTypes[name] = tensor_info.GetElementType();
            }
        }

        Ort::Env _env;
        Ort::Session _session;
        ModelInformation _modelInfo;
    };
}

OnnxInferencer::OnnxInferencer(const std::wstring& modelPath, DeviceType type, int deviceID)
    : _handle(NULL)
{
    try { _handle = new OnnxWrapper(modelPath, type, deviceID); }
    catch (std::exception& e) { OSG_WARN << "[OnnxInferencer] Failed to load model\n"; }
}

OnnxInferencer::~OnnxInferencer()
{ if (_handle) delete _handle; }

std::string OnnxInferencer::getModelDescription() const
{
    OnnxWrapper* w = (OnnxWrapper*)_handle;
    if (!w) return ""; else return w->getModelInformation().description;
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
