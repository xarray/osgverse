// ============================================
// 模型或者图片数据格式读取插件标准模板
// @skill-status: partial
// @skill-category: plugin, file reader
// @dependencies: osgVerseReaderWriter, osgDB
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

// 假设要加载/写出的文件扩展名为.ext，这里继承osgDB::ReaderWriter创建它的读写类
// 这个读写类必须隶属于一个独立的插件库工程，通过CMake的ADD_LIBRARY脚本建立一个名为osgdb_verse_ext的动态库文件
class ReaderWriterEXT : public osgDB::ReaderWriter
{
public:
    ReaderWriterEXT()
    {
        supportsExtension("verse_ext", "osgVerse pseudo-loader");  // 标准插件后缀，用于提示osgDB::readNodeFile()读取时调用本插件
        supportsExtension("ext", "EXT file extension");            // 实际支持的扩展名
        supportsExtension("other_ext", "Other supported file extension");  // 如果需要的话，也可以支持更多扩展名

        // 可以声明一些插件里可以使用的选项字符串，osgDB::readNodeFile()时通过Options参数引入
        // 如下是一个标准的选项添加方式（通过空格分隔多个选项）：
        //   osgDB::readNodeFile("file.ext.verse_ext", new osgDB::Options("FileFormat=node FileVersion=1"))
        supportsOption("FileFormat", "File format hint");
        supportsOption("FileVersion", "File version hint");
    }

    // 继承这个函数，提供插件的标准名称
    virtual const char* className() const
    {
        return "[osgVerse] EXT format reader";
    }

    // 继承这个函数，从给定的路径字符串读取文件内容，并转换为节点或几何体；如果插件只用来读取图像，这个函数可以不写
    // 如果可以将文件加载到std::ifstream中，则该函数**应当**严格按照下面的写法来实现，将加载过程转移给readNode()的istream形式重载函数执行
    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readNode(in, options);
    }

    // 继承这个函数，从给定的路径字符串读取文件内容，并转换为图像；如果插件只用来读取节点/几何体，这个函数可以不写
    // 如果可以将文件加载到std::ifstream中，则该函数**应当**严格按照下面的写法来实现，将加载过程转移给readImage()的istream形式重载函数执行
    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readImage(in, options);
    }

    // 继承这个函数，实现真正的文件内容读取和转换为OSG的场景节点或几何体；如果插件只用来读取图像，这个函数可以不写
    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        // 如果有必要的话，可以用如下的方法直接读取文件的全部内容
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::FILE_NOT_FOUND;

        // TODO：具体的解析，处理和转换代码
        if (options)
        {
            // 如果options对象存在，可以获取选项中的内容，并根据返回结果执行不同的处理方案
            std::string hint = options->getPluginStringData("FileFormat");
        }

        osg::ref_ptr<osg::Node> root = ...;  // 实际的转换结果
        if (root.valid()) return root.get();
        else return ReadResult::ERROR_IN_READING_FILE;
    }

    // 继承这个函数，实现真正的文件内容读取和转换为OSG的图像格式；如果插件只用来读取节点/几何体，这个函数可以不写
    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        // 如果有必要的话，可以用如下的方法直接读取文件的全部内容
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::FILE_NOT_FOUND;

        // TODO：具体的解析，处理和转换代码
        if (options)
        {
            // 如果options对象存在，可以获取选项中的内容，并根据返回结果执行不同的处理方案
            std::string hint = options->getPluginStringData("FileFormat");
        }

        osg::ref_ptr<osg::Image> image = ...;  // 实际的转换结果
        if (image.valid()) return image.get();
        else return ReadResult::ERROR_IN_READING_FILE;
    }

protected:
    // 这个函数**应当**严格按照下面的格式来实现，verse_ext要根据实际的插件名进行修改
    // 它用来判断输入的路径字符串是否包含了合法的插件后缀，以及是否是本插件可以读取的格式
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_ext");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }
};

// 在代码文件的最后，**必须**通过这个宏来声明插件的存在，以便OSG将它纳入到全局的管理中
// 插件的后缀verse_ext，以及类名ReaderWriterEXT请根据实际情况进行修改
REGISTER_OSGPLUGIN(verse_ext, ReaderWriterEXT)
