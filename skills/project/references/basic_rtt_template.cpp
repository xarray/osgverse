// ============================================
// 子场景“渲染到纹理”过程的标准模板
// @skill-status: partial
// @skill-category: deferred rendering, render-to-texture
// @dependencies: osgVersePipeline
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgVerse/pipeline/Pipeline.h>
#include <osgVerse/pipeline/Utilities.h>

// 创建一个将子场景“渲染到纹理”的相机对象，以及一个用于保存结果的GPU纹理
{
    int w = ..., h = ...;  // 结果纹理对象的宽度和高度
    osg::Node* node = ...;  // 准备渲染到纹理的子场景节点

    // 创建一个用于保存渲染结果的GPU纹理，它的像素格式是RGBA，数据格式是INT8（unsigned byte）
    // 其它常见的像素/数据格式取值包括：
    //   - R_INT8：像素格式是R（单通道），数据格式是unsigned byte
    //   - R_FLOAT32：像素格式是R（单通道），数据格式是float
    //   - RG_INT8：像素格式是RG（双通道），数据格式是unsigned byte
    //   - RG_FLOAT32：像素格式是RG（双通道），数据格式是float
    //   - RGB_INT8：像素格式是RGB，数据格式是unsigned byte
    //   - RGB_FLOAT32：像素格式是RGB，数据格式是float
    //   - RGBA_FLOAT32：像素格式是RGBA，数据格式是float
    //   - DEPTH24_STENCIL8：像素格式是 深度+模板 的压缩格式，专用于深度缓存的保存
    //   - DEPTH32：像素格式是32位深度值，专用于深度缓存的保存
    osg::ref_ptr<osg::Texture> buffer = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);

    // 创建一个“渲染到纹理”的相机对象，渲染目标为COLOR_BUFFER0，结果保存到buffer对象
    // 最后一个参数为false，此时可以自己添加rttCamera的子场景对象，将它们作为被渲染的场景；同时要注意设置rttCamera的观察矩阵以确保子场景可见
    osg::Camera* rttCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, buffer.get(), NULL, false);
    rttCamera->addChild(node);  // 加入被渲染的场景对象

    // 最后一个参数为true，则自动创建一个全屏幕大小的四边形作为被渲染的场景，并设置对应的观察矩阵；此即延迟渲染管线的中间环节
    // 如果需要给这个中间环节设置输入的纹理，着色器代码，或者Uniform变量，可以直接对rttCamera2的getOrCreateStateSet()操作。它们会自动赋予子节点全屏幕四边形，从而实现一些中间环节的渲染操作
    osg::Camera* rttCamera2 = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, buffer.get(), NULL, true);
}

// 创建一个将子场景“渲染到纹理”的相机对象，以及多个用于保存结果的GPU纹理。支持MRT（多重渲染目标）的方式
{
    int w = ..., h = ...;  // 结果纹理对象的宽度和高度
    osg::Node* node = ...;  // 准备渲染到纹理的子场景节点

    // 创建一个用于保存渲染结果的GPU纹理，它的像素格式是RGBA，数据格式是INT8（unsigned byte）；其它常用格式参见前文
    osg::ref_ptr<osg::Texture> buffer0 = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);

    // 创建第二个GPU纹理，它的格式是DEPTH24_STENCIL8，可以用来保存深度缓存的信息
    osg::ref_ptr<osg::Texture> buffer1 = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::DEPTH24_STENCIL8, w, h);

    // 创建一个“渲染到纹理”的相机对象，渲染目标为COLOR_BUFFER0，结果保存到buffer0对象；再添加一个新的渲染目标DEPTH_BUFFER，结果保存到buffer1对象
    // 如果继续添加新的渲染目标如COLOR_BUFFER1，COLOR_BUFFER2等，则需要编写对应的着色器代码以实现MRT（多重渲染目标）的输出
    osg::Camera* rttCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, buffer0.get(), NULL, false);
    rttCamera->attach(osg::Camera::DEPTH_BUFFER, buffer1.get());
    rttCamera->addChild(node);  // 加入被渲染的场景对象
}

// 创建一个将子场景“渲染到纹理”的相机对象，以及一个用于保存结果的图片对象，可以在CPU端读取
{
    int w = ..., h = ...;  // 结果图片对象的宽度和高度
    osg::Node* node = ...;  // 准备渲染到纹理的子场景节点

    // 创建保存渲染结果的图片对象
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);

    // 创建一个“渲染到纹理”的相机对象，渲染目标为COLOR_BUFFER0，结果保存到image图片，子场景为node
    osg::Camera* rttCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, image.get(), node);

    // 继续之后的代码
    
    // 主渲染循环经过了至少2帧之后，可以从image中获取渲染到纹理的结果，并通过image->data()访问其中具体的像素值
}
