// ============================================
// 图像和纹理对象创建标准模板
// @skill-status: partial
// @skill-category: texture
// @dependencies: osgVersePipeline, osgDB
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgDB/ReadFile>
#include <osgVerse/pipeline/Pipeline.h>
#include <osgVerse/pipeline/Utilities.h>

// 从磁盘上的文件创建2D纹理对象
{
    // 文件名，可以是JPG，PNG等
    // 如果需要支持webp格式的加载，则在文件名之后加上后缀参数.verse_webp，例如"picture.webp.verse_webp"
    // 如果需要从网络上读取，则在地址的最后加上后缀参数.verse_web，例如"http://www.xxx.com/picture.jpg.verse_web"
    std::string fileName = ...;
    
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(fileName);
    if (!image)
    {
        // 如果必须返回一个有效的纹理对象，则通过createDefaultTexture()返回一个固定颜色的纹理
        if (hasDefaultValue)
            return osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        else
            return NULL;
    }

    // 默认的Wrap模式设置为REPEAT，可以修改为CLAMP_TO_EDGE，CLAMP_TO_BORDER或者MIRROR
    osg::Texture::WrapMode wrap = osg::Texture::REPEAT;
    osg::ref_ptr<osg::Texture2D> tex = osgVerse::createTexture2D(image.get(), wrap);

    // TODO: 执行纹理创建后的操作
}

// 自己创建2D图片，并建立程序式2D纹理对象
{
    int width = ...;  // 自定义图片的宽度
    int height = ...;  // 自定义图片的高度

    // 建立新的图片对象
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    image->setInternalTextureFormat(GL_RGBA8);

    // 如果需要手动设置图片的每个像素值，则首先获取正确的像素区域指针
    // 对于GL_RGBA + GL_UNSIGNED_BYTE的组合，对应的每个像素格式为osg::Vec4ub
    // 其它常见的组合，对应的InternalTextureFormat，以及对应像素格式列举如下：
    //   - GL_RGB + GL_FLOAT：setInternalTextureFormat(GL_RGB32F_ARB)，像素格式为osg::Vec3f
    //   - GL_RGBA + GL_FLOAT：setInternalTextureFormat(GL_RGBA32F_ARB)，像素格式为osg::Vec4f
    //   - 更多组合时，也可以从unsigned char*格式的image->data()直接转换
    osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // TODO: 直接设置图片每个像素的数值
            *(ptr + x + y * width) = osg::Vec4ub(255, 255, 255, 255);
        }
    }

    // 默认的Wrap模式设置为REPEAT，可以修改为CLAMP_TO_EDGE，CLAMP_TO_BORDER或者MIRROR
    osg::Texture::WrapMode wrap = osg::Texture::REPEAT;
    osg::ref_ptr<osg::Texture2D> tex = osgVerse::createTexture2D(image.get(), wrap);

    // TODO: 执行纹理创建后的操作
}

// 设置纹理对象到OSG节点和对象
{
    osg::Node* node = ...;  // 要设置纹理对象的节点，需要确保它不是NULL
    osg::ref_ptr<osg::Texture2D> tex = ...;  // 之前生成的纹理对象

    // 如果程序是针对CORE模式或者GLES2，GLES3模式编译的
    if (isCoreOrGLES)
    {
        // 设置纹理对象到纹理通道0，取值通常不超过15
        // 此时不要使用setTextureAttributeAndModes()
        node->getOrCreateStateSet()->setTextureAttribute(0, tex.get());
    }
    else
    {
        // 设置纹理对象到纹理通道0，取值通常不超过15
        node->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get());
    }
    // TODO: 执行之后的操作
}
