#ifndef MANA_AI_ONNXRUNTIMEENGINE
#define MANA_AI_ONNXRUNTIMEENGINE

#include <osg/Version>
#include <osg/Geometry>
#include <osg/Image>
#include <string>
#include <vector>
#include <functional>

namespace osgVerse
{

    class OnnxInferencer : public osg::Referenced
    {
    public:
        enum DeviceType { CPU, CUDA };
        enum DataType { UnknownData, FloatData, UCharData, CharData, UShortData, ShortData, IntData, LongData,
                        StringData, BoolData, HalfData, DoubleData, UIntData, ULongData, Complex64, Complex128 };
        typedef std::function<void (bool)> FinishedCallback;

        OnnxInferencer(const std::wstring& modelPath, DeviceType type, int deviceID = 0);

        std::string getModelDescription() const;
        std::vector<std::string> getModelLayerNames(bool inputLayer) const;
        std::vector<int64_t> getModelShapes(bool inputLayer, const std::string& name) const;
        DataType getModelDataType(bool inputLayer, const std::string& name) const;

        bool addInput(const std::vector<std::vector<float>>& values, const std::string& inName);
        bool addInput(const std::vector<std::vector<unsigned char>>& values, const std::string& inName);
        bool addInput(const std::vector<std::vector<char>>& values, const std::string& inName);
        bool addInput(const std::vector<std::vector<unsigned short>>& values, const std::string& inName);
        bool addInput(const std::vector<std::vector<short>>& values, const std::string& inName);
        bool addInput(const std::vector<std::vector<unsigned int>>& values, const std::string& inName);
        bool addInput(const std::vector<std::vector<int>>& values, const std::string& inName);
        bool addInput(const std::vector<osg::Image*>& images, const std::string& inName);

        bool run(FinishedCallback cb = NULL);
        bool getOutput(std::vector<float>& values, unsigned int index = 0) const;
        bool getOutput(std::vector<unsigned char>& values, unsigned int index = 0) const;
        bool getOutput(std::vector<char>& values, unsigned int index = 0) const;
        bool getOutput(std::vector<unsigned short>& values, unsigned int index = 0) const;
        bool getOutput(std::vector<short>& values, unsigned int index = 0) const;
        bool getOutput(std::vector<unsigned int>& values, unsigned int index = 0) const;
        bool getOutput(std::vector<int>& values, unsigned int index = 0) const;

        static osg::Image* convertImage(osg::Image* in, DataType type, const std::vector<int64_t>& shapes);

    protected:
        virtual ~OnnxInferencer();
        void* _handle;
    };

}

#endif
