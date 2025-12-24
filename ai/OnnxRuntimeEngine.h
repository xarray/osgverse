#ifndef MANA_AI_ONNXRUNTIMEENGINE
#define MANA_AI_ONNXRUNTIMEENGINE

#include <osg/Version>
#include <osg/Geometry>
#include <osg/Image>
#include <string>
#include <vector>

namespace osgVerse
{

    class OnnxInferencer : public osg::Referenced
    {
    public:
        enum DeviceType { CPU, CUDA };
        enum DataType { UnknownData, FloatData, UCharData, CharData, UShortData, ShortData, IntData, LongData,
                        StringData, BoolData, HalfData, DoubleData, UIntData, ULongData, Complex64, Complex128 };
        OnnxInferencer(const std::wstring& modelPath, DeviceType type, int deviceID = 0);

        std::string getModelDescription() const;
        std::vector<std::string> getModelLayerNames(bool inputLayer) const;
        std::vector<int64_t> getModelShapes(bool inputLayer, const std::string& name) const;
        DataType getModelDataType(bool inputLayer, const std::string& name) const;

        bool addInput(const std::vector<osg::Image*>& images, const std::string& inName);

    protected:
        virtual ~OnnxInferencer();
        void* _handle;
    };

}

#endif
