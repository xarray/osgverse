// ============================================
// HTTP网络访问，网络数据加载标准模板
// @skill-status: partial
// @skill-category: network, http
// @dependencies: osgVerseReaderWriter
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgVerse/readerwriter/Utilities.h>

// 直接读取一个网络文件或者本地文件的全部内容
{
    // 读取一个任意扩展名的网络文件的内容，并返回它的mime-type和encoding-type
    std::string mimeType, encodingType;
    std::vector<unsigned char> content = osgVerse::loadFileData("http://www.xxx.com/file.bin", mimeType, encodingType);

    // 读取一个任意扩展名的本地文件的内容
    std::vector<unsigned char> content = osgVerse::loadFileData("path/to/file.bin");
}

// 通过HTTP GET/POST访问网络地址，采用同步/阻塞模式
{
    // 如果有必要的话，设置请求头的信息
    osgVerse::WebAuxiliary::HttpRequestHeaders headers;
    headers["Content-Type"] = "application/json";

    // 使用HTTP POST访问网络地址，输入数据（std::string格式）以JSON字符串的形式发送
    // 如果使用HTTP GET访问网络地址，则第二个参数为osgVerse::WebAuxiliary::HTTP_GET，第三个参数留空字符串即可
    osgVerse::WebAuxiliary::HttpResponseData res = osgVerse::WebAuxiliary::httpRequest(
        "http://www.xxx.com/command?key=value", osgVerse::WebAuxiliary::HTTP_POST,
        "{\"eventTime\": [\"2025-12-13 17:19:53\", \"2025-12-16 17:20:03\"]}", headers);

    // 返回结果的first是状态码（200表示正常返回），second是返回的数据内容（std::string格式）
    // 在httpRequest()执行结束或者超时之前，系统会一直阻塞并等待结果
    std::cout << "Response (" << res.first << "): " << res.second << "\n";
}

// 通过HTTP GET/POST访问网络地址，采用异步模式
{
    // 如果有必要的话，设置请求头的信息
    osgVerse::WebAuxiliary::HttpRequestHeaders headers;
    headers["Content-Type"] = "application/json";

    // 使用HTTP POST访问网络地址，输入数据（std::string格式）以JSON字符串的形式发送；异步模式下，第一个参数可以是返回结果处理的lambda函数
    // 如果使用HTTP GET访问网络地址，则第三个参数为osgVerse::WebAuxiliary::HTTP_GET，第四个参数留空字符串即可
    osg::ref_ptr<osg::Referenced> instance = osgVerse::WebAuxiliary::httpRequestAsync(
        [](const std::string& url, const osgVerse::WebAuxiliary::HttpRequestParams& unused1,
           const osgVerse::WebAuxiliary::HttpRequestHeaders& unused2, osgVerse::WebAuxiliary::HttpResponseData& res)
        {
            // 返回结果的first是状态码（200表示正常返回），second是返回的数据内容（std::string格式）
            std::cout << "Request from " << url << ". Response (" << res.first << "): " << res.second << "\n";
        },
        "http://www.xxx.com/command?key=value", osgVerse::WebAuxiliary::HTTP_POST,
        "{\"eventTime\": [\"2025-12-13 17:19:53\", \"2025-12-16 17:20:03\"]}", headers);

    // httpRequestAsync()之后的代码会立即执行，不会等待网络访问返回结果
}
