#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ConvertUTF>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <VerseCommon.h>
#include <ai/OnnxRuntimeEngine.h>
#include <modeling/Utilities.h>
#include <readerwriter/Utilities.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

    std::string modelName = BASE_DIR + "/misc/mnist-12.onnx"; arguments.read("--model", modelName);
    osg::ref_ptr<osgVerse::OnnxInferencer> inferencer =
        new osgVerse::OnnxInferencer(osgDB::convertUTF8toUTF16(modelName), osgVerse::OnnxInferencer::CUDA);
    std::cout << modelName << ": " << inferencer->getModelDescription();

    std::string inputName = inferencer->getModelLayerNames(true).front();
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(BASE_DIR + "/misc/mnist_test.jpg");
    image = osgVerse::OnnxInferencer::convertImage(
        image.get(), inferencer->getModelDataType(true, inputName), inferencer->getModelShapes(true, inputName));

    inferencer->addInput(std::vector<osg::Image*>{ image.get() }, inputName);
    inferencer->run([&inferencer](bool success)
    {
        if (!success) std::cout << "Failed to run task\n"; else std::cout << "Results: ";
        std::vector<float> values; inferencer->getOutput(values);
        for (size_t i = 0; i < values.size(); ++i) std::cout << values[i] << " ";
    });

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewInWindow(50, 50, 960, 600);
    viewer.realize();

    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
