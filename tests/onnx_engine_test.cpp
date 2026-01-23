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
#include <algorithm>
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
    if (arguments.read("--mobilenet"))
    {
        // https://github.com/onnx/models/tree/main/validated/vision/classification/mobilenet
        /* MoileNetv2 model (object detection from an image):
           -  IN: input/Float/-1/3/224/224 => dynamic batch / RGB image (3-comp) / 224x224 size
           - OUT: output/Float/-1/1000 => dynamic batch / 1000 classes
        */
        std::string inputFile; if (!arguments.read("--image", inputFile)) return 1;
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(inputFile);
        image = osgVerse::OnnxInferencer::convertImage(
            image.get(), inferencer->getModelDataType(true, inputName), inferencer->getModelShapes(true, inputName));

        inferencer->addInput(std::vector<osg::Image*>{ image.get() }, inputName);
        inferencer->run([&inferencer](bool success)
        {
            if (!success) std::cout << "Failed to run task\n"; else std::cout << "Results (size = ";
            std::vector<float> values; inferencer->getOutput(values); std::cout << values.size() << "): ";
            std::vector<float> probabilities = osgVerse::OnnxInferencer::computeSoftmax(values);

            std::vector<float>::iterator it = std::max_element(probabilities.begin(), probabilities.end());
            std::cout << "Maximum index " << (it - probabilities.begin()) << ", Percent = " << *it << "\n";
        });
    }
    else if (arguments.read("--yolo4"))
    {
        // https://github.com/onnx/models/blob/main/validated/vision/object_detection_segmentation/yolov4
        /* YOLOv4 model (object detection from an image):
           -  IN: input_1:0/Float/-1/416/416/3 => dynamic batch / 416x416 size / RGB image (3-comp)
           - OUT: Identity:0/Float/-1/-1/-1/3/85 => dynamic batch / dynamic size / 3 anchor boxes /
                                                    85 values (4 for rect, 1 for confidence, 80 for classes)
                  Identity_1:0/Float/-1/-1/-1/3/85 => same
                  Identity_2:0/Float/-1/-1/-1/3/85 => same
        */

        // TODO
    }
    else if (arguments.read("--upscale"))
    {
        // https://github.com/onnx/models/blob/main/validated/vision/super_resolution/sub_pixel_cnn_2016
        /* Super Resolution (upscale an input image):
           -  IN: input/Float/-1/1/224/224 => dynamic batch / gray image (1-comp) / 224x224 size
           - OUT: output/Float/-1/1/672/672 => dynamic batch / gray image (1-comp) / 672x672 size
        */

        // TODO
    }
    else if (arguments.read("--ultraface"))
    {
        // https://github.com/onnx/models/blob/main/validated/vision/body_analysis/ultraface
        /* UltraFace (face detection from an input image):
           -  IN: input/Float/1/3/480/640 => one batch / RGB image (3-comp) / 640x480 size
           - OUT: scores/Float/1/17640/2 => one batch / 17640 ancher boxes / 2 confidence values (yes/no)
                  boxes/Float/1/17640/4 => one batch / 17640 ancher boxes / 4 coordinates (x/y/w/h)
        */

        // TODO
    }
    else
    {
        // https://github.com/onnx/models/blob/main/validated/vision/classification/mnist
        /* MNIST model (handwriting of 0 - 9 numbers):
           -  IN: Input3/Float/1/1/28/28 => one batch / gray image (1-comp) / 28x28 size
           - OUT: Plus214_Output_0/Float/1/10 => one batch / 10 confidence values (for each number)
        */
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(BASE_DIR + "/misc/mnist_test.jpg");
        image = osgVerse::OnnxInferencer::convertImage(
            image.get(), inferencer->getModelDataType(true, inputName), inferencer->getModelShapes(true, inputName));

        inferencer->addInput(std::vector<osg::Image*>{ image.get() }, inputName);
        inferencer->run([&inferencer](bool success)
        {
            if (!success) std::cout << "Failed to run task\n"; else std::cout << "Results (size = ";
            std::vector<float> values; inferencer->getOutput(values); std::cout << values.size() << "): ";
            std::vector<float> probabilities = osgVerse::OnnxInferencer::computeSoftmax(values);
            for (size_t i = 0; i < probabilities.size(); ++i) std::cout << probabilities[i] << " ";
        });
    }

    /*osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewInWindow(50, 50, 960, 600);
    viewer.realize();

    while (!viewer.done())
    {
        viewer.frame();
    }*/
    return 0;
}
