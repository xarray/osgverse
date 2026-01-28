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
#include <pipeline/Drawer2D.h>
#include <readerwriter/Utilities.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

struct YoloDetection
{
    float x, y, w, h;
    float confidence;
    int class_id;

    static float sigmoid(float x)
    { return 1.0f / (1.0f + std::exp(-x)); }

    static float calculateIOU(const YoloDetection& box1, const YoloDetection& box2)
    {
        float inter_x1 = osg::maximum(box1.x, box2.x);
        float inter_y1 = osg::maximum(box1.y, box2.y);
        float inter_x2 = osg::minimum(box1.x + box1.w, box2.x + box2.w);
        float inter_y2 = osg::minimum(box1.y + box1.h, box2.y + box2.h);
        float inter_area = osg::maximum(0.0f, inter_x2 - inter_x1) * osg::maximum(0.0f, inter_y2 - inter_y1);
        float union_area = box1.w * box1.h + box2.w * box2.h - inter_area;
        return inter_area / (union_area + 1e-6f);
    }

    static std::vector<YoloDetection> applyNMS(std::vector<YoloDetection>& detections, float iou_threshold = 0.5f)
    {
        std::vector<YoloDetection> final_detections;
        std::sort(detections.begin(), detections.end(), [](const YoloDetection& a, const YoloDetection& b)
        { return a.confidence > b.confidence; });

        std::vector<bool> suppressed(detections.size(), false);
        for (size_t i = 0; i < detections.size(); ++i)
        {
            if (suppressed[i]) continue;
            final_detections.push_back(detections[i]);
            for (size_t j = i + 1; j < detections.size(); ++j)
            {
                if (suppressed[j]) continue;
                if (detections[i].class_id != detections[j].class_id) continue;
                float iou = calculateIOU(detections[i], detections[j]);
                if (iou > iou_threshold) suppressed[j] = true;
            }
        }
        return final_detections;
    }
};

std::vector<YoloDetection> decodeYOLOv4Output(
        const float* output_data, int grid_size, int num_anchors, int num_classes, int stride,
        const std::vector<std::pair<float, float>>& anchors, float nms_threshold = 0.5f,
        float conf_threshold = 0.5f, int input_width = 416, int input_height = 416)
{
    std::vector<YoloDetection> detections;
    int num_predictions_per_anchor = 5 + num_classes; // tx, ty, tw, th, conf, classes
    for (int grid_y = 0; grid_y < grid_size; ++grid_y)
    {
        for (int grid_x = 0; grid_x < grid_size; ++grid_x)
        {
            for (int anchor_idx = 0; anchor_idx < num_anchors; ++anchor_idx)
            {
                int base_idx = (grid_y * grid_size * num_anchors * num_predictions_per_anchor) +
                               (grid_x * num_anchors * num_predictions_per_anchor) +
                               (anchor_idx * num_predictions_per_anchor);
                float tx = output_data[base_idx + 0], ty = output_data[base_idx + 1];
                float tw = output_data[base_idx + 2], th = output_data[base_idx + 3];
                float objectness = output_data[base_idx + 4];

                float sig_objectness = YoloDetection::sigmoid(objectness);
                float max_class_prob = 0.0f; int class_id = -1;
                for (int c = 0; c < num_classes; ++c)
                {
                    float class_score = output_data[base_idx + 5 + c];
                    float sig_class_score = YoloDetection::sigmoid(class_score);
                    if (sig_class_score > max_class_prob)
                    { max_class_prob = sig_class_score; class_id = c; }
                }

                // Check confidence
                float confidence = sig_objectness * max_class_prob;
                if (confidence < conf_threshold) continue;

                // Get coordinates
                float bx = (YoloDetection::sigmoid(tx) + grid_x) / grid_size;
                float by = (YoloDetection::sigmoid(ty) + grid_y) / grid_size;
                float bw = std::exp(tw) * anchors[anchor_idx].first / input_width;
                float bh = std::exp(th) * anchors[anchor_idx].second / input_height;
                float x_center = bx, y_center = by, box_width = bw, box_height = bh;
                float x = x_center - box_width / 2.0f; x = osg::maximum(0.0f, osg::minimum(1.0f, x));
                float y = y_center - box_height / 2.0f; y = osg::maximum(0.0f, osg::minimum(1.0f, y));
                box_width = osg::maximum(0.0f, osg::minimum(1.0f - x, box_width));
                box_height = osg::maximum(0.0f, osg::minimum(1.0f - y, box_height));
                detections.push_back(YoloDetection { x, y, box_width, box_height, confidence, class_id });
            }
        }
    }
    return YoloDetection::applyNMS(detections, nms_threshold);
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osgVerse::updateOsgBinaryWrappers();
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

    // Create a drawer for displaying final result
    osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
    drawer->allocateImage(512, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    drawer->loadFont("default", MISC_DIR + "LXGWFasmartGothic.otf");
    drawer->start(false);
    drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

    // Load onnx model
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
        inferencer->run([&inferencer](size_t numOutputs, bool success)
        {
            if (!success) std::cout << "Failed to run task\n"; else std::cout << "Results (outputs = " << numOutputs;
            std::vector<float> values; inferencer->getOutput(values); std::cout << ", size0 = " << values.size() << "): ";
            std::vector<float> probabilities = osgVerse::OnnxInferencer::computeSoftmax(values);

            // Check result at https://github.com/onnx/models/blob/main/validated/vision/classification/synset.txt
            std::vector<float>::iterator it = std::max_element(probabilities.begin(), probabilities.end());
            std::cout << "Maximum index " << (it - probabilities.begin()) + 1 << ", Confidence = " << *it << "\n";
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
        std::string inputFile; if (!arguments.read("--image", inputFile)) return 1;
        inferencer->setModelDataLayout(true, inputName, osgVerse::OnnxInferencer::ImageNHWC);

        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(inputFile);
        image = osgVerse::OnnxInferencer::convertImage(
            image.get(), inferencer->getModelDataType(true, inputName),
            inferencer->getModelShapes(true, inputName), osgVerse::OnnxInferencer::ImageNHWC);

        inferencer->addInput(std::vector<osg::Image*>{ image.get() }, inputName);
        inferencer->run([&drawer, &inferencer](size_t numOutputs, bool success)
        {
            if (!success) std::cout << "Failed to run task\n"; else std::cout << "Results (outputs = " << numOutputs;
            std::vector<float> values; inferencer->getOutput(values); std::cout << ", size0 = " << values.size() << ")\n";

            std::vector<std::pair<float, float>> anchors;
            anchors.push_back(std::pair<float, float>(12, 16));
            anchors.push_back(std::pair<float, float>(19, 36));
            anchors.push_back(std::pair<float, float>(40, 28));

            // Size of output 'Identity' (batch * grid_x * grid_y * anchor_boxes * classes)
            // It should be 689520 here: 1 * (416/8) * (416/8) * 3 * 80
            std::vector<YoloDetection> detections = decodeYOLOv4Output(values.data(), 52, 3, 80, 8, anchors, 0.5f, 0.4f);

            // Check result at https://github.com/hunglc007/tensorflow-yolov4-tflite/blob/master/data/classes/coco.names
            osgVerse::Drawer2D::StyleData style1(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
            osgVerse::Drawer2D::StyleData style2(osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f), true);
            for (size_t i = 0; i < detections.size(); ++i)
            {
                const YoloDetection& d = detections[i];
                drawer->drawRectangle(osg::Vec4(d.x, d.y, d.w, d.h) * drawer->s(), 0.0f, 0.0f, style1);
                drawer->drawUtf8Text(osg::Vec2(d.x, d.y) * drawer->s(), 30.0f, std::to_string(d.class_id + 1), "", style2);
            }
            drawer->finish();
        });

        // Show detection results in scene graph
        osg::Geometry* geom0 = osg::createTexturedQuadGeometry(
            osg::Vec3(0.0f, 0.0f, 0.0f), osg::X_AXIS, osg::Y_AXIS, 0.0f, 1.0f, 1.0f, 0.0f);
        geom0->getOrCreateStateSet()->setTextureAttributeAndModes(0, osgVerse::createTexture2D(image.get()));

        osg::Geometry* geom1 = osg::createTexturedQuadGeometry(
            osg::Vec3(0.0f, 0.0f, 0.01f), osg::X_AXIS, osg::Y_AXIS, 0.0f, 1.0f, 1.0f, 0.0f);
        geom1->getOrCreateStateSet()->setTextureAttributeAndModes(0, osgVerse::createTexture2D(drawer.get()));
        geom1->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        geom1->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(geom0); geode->addDrawable(geom1);
        geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        root->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS)); root->addChild(geode);
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
        inferencer->run([&inferencer](size_t numOutputs, bool success)
        {
            if (!success) std::cout << "Failed to run task\n"; else std::cout << "Results (outputs = " << numOutputs;
            std::vector<float> values; inferencer->getOutput(values); std::cout << ", size0 = " << values.size() << "): ";
            std::vector<float> probabilities = osgVerse::OnnxInferencer::computeSoftmax(values);

            std::vector<float>::iterator it = std::max_element(probabilities.begin(), probabilities.end());
            std::cout << "Maximum index " << (it - probabilities.begin()) + 1 << ", Confidence = " << *it << "\n";
        });
    }

    if (root->getNumChildren() > 0)
    {
        osgViewer::Viewer viewer;
        viewer.addEventHandler(new osgViewer::StatsHandler);
        viewer.addEventHandler(new osgViewer::WindowSizeHandler);
        viewer.setCameraManipulator(new osgGA::TrackballManipulator);
        viewer.setSceneData(root.get());
        viewer.setUpViewInWindow(50, 50, 960, 600);
        return viewer.run();
    }
    return 0;
}
