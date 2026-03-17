// ============================================
// GLSL着色器对象创建标准模板
// @skill-status: partial
// @skill-category: shader, glsl
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

// 从磁盘文件建立标准的着色器对象，并设置给输入的渲染状态集
{
    std::string vertex_file = ...;      // 顶点着色器文件名，后缀可以是.glsl或者.vert
    std::string fragment_file = ...;    // 片元着色器文件名，后缀可以是.glsl或者.frag

    // 从磁盘文件读取GLSL着色器代码并建立Shader对象
    osg::Shader* vs = osgDB::readShaderFile(osg::Shader::VERTEX, vertex_file);
    osg::Shader* fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, fragment_file);

    // 猜测OpenGL Context和GLSL的版本，并且通过它来修正着色器中的内容，以适配多种不同的接口平台（早期GL2，GL3/GLCore，GLES等）
    int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);

    // 建立新的着色器程序对象，给Shader对象设置一个名称以便调试时追溯，然后将它设置给程序对象
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("Function_VS"); program->addShader(vs);
    fs->setName("Function_FS"); program->addShader(fs);

    // 从节点或者几何体对象获取渲染状态集，应用着色器程序对象
    osg::Node* node = ...;
    node->getOrCreateStateSet()->setAttribute(program.get());

    // TODO: 执行之后的操作
}

// 自己编写着色器代码，添加Uniform变量和纹理对象，并设置给输入的渲染状态集
{
    // 顶点着色器中，输入的顶点属性均以osg_为前缀，包括osg_Vertex，osg_Normal，osg_Color，osg_MultiTexCoord0（以及1-7），不需要C++代码额外再做定义
    // 自定义的输入变量以VERSE_VS_IN为前缀，自定义的输出变量以VERSE_VS_OUT为前缀，一致变量仍然以uniform为前缀
    // osgVerse预设一些矩阵相关的一致变量，包括VERSE_MATRIX_MVP，VERSE_MATRIX_MV，VERSE_MATRIX_P，不需要C++代码额外再做定义
    // OSG也有一些预设的一致变量，包括osg_ViewMatrixInverse，osg_FrameNumber等，可以直接使用
    const char* vertCode = {
        "uniform mat4 osg_ViewMatrixInverse; \n"
        "VERSE_VS_OUT vec4 worldPos, texCoord; \n"
        "void main() {\n"
        "    texCoord = osg_MultiTexCoord0; \n"
        "    worldPos = osg_ViewMatrixInverse * osg_ModelViewMatrix * osg_Vertex; \n"
        "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
        "}\n"
    };

    // 片元着色器中，自定义的输入变量以VERSE_FS_IN为前缀，输出的目标数值以VERSE_FS_OUT为前缀，一致变量仍然以uniform为前缀
    // osgVerse预设一些纹理读取相关的函数，包括VERSE_TEX1D，VERSE_TEX2D，VERSE_TEX3D，VERSE_TEXCUBE，不需要C++代码额外再做定义
    // 为了适配早期的GL2版本着色器代码，需要额外在main函数的末尾添加一行VERSE_FS_FINAL()，用来说明最终输出的颜色值
    // 对于MRT（多重渲染目标）的情况，可以直接设置多个VERSE_FS_OUT变量，并且无需考虑VERSE_FS_FINAL()
    const char* fragCode = {
        "uniform sampler2D baseTexture; \n"
        "uniform vec4 extraColor; \n"
        "VERSE_FS_IN vec4 worldPos, texCoord; \n"
        "VERSE_FS_OUT vec4 fragColor; \n"
        "void main() {\n"
        "    fragColor = VERSE_TEX2D(baseTexture, texCoord.st) * extraColor; \n"
        "    VERSE_FS_FINAL(fragColor);\n"
        "}\n"
    };

    // 通过GLSL着色器代码建立Shader对象
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, vertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, fragCode);

    // 猜测OpenGL Context和GLSL的版本，并且通过它来修正着色器中的内容，以适配多种不同的接口平台（早期GL2，GL3/GLCore，GLES等）
    int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);

    // 建立新的着色器程序对象，给Shader对象设置一个名称以便调试时追溯，然后将它设置给程序对象
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("Function_VS"); program->addShader(vs);
    fs->setName("Function_FS"); program->addShader(fs);

    // 从节点或者几何体对象获取渲染状态集，应用着色器程序对象
    osg::Node* node = ...;
    node->getOrCreateStateSet()->setAttribute(program.get());

    // 添加纹理对象
    osg::Texture2D* tex = ...;
    node->getOrCreateStateSet()->setTextureAttribute(0, tex);  // 纹理对象绑定到0通道

    // 添加Uniform变量
    node->getOrCreateStateSet()->addUniform(new osg::Uniform("baseTexture", (int)0));  // 纹理对象在0通道
    node->getOrCreateStateSet()->addUniform(new osg::Uniform("extraColor", osg::Vec4(1.0f, 1.0f,1.0f, 1.0f)));

    // TODO: 执行之后的操作
}
