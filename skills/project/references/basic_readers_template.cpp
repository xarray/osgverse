// ============================================
// 多三维地球数据瓦片，高斯泼溅数据，3dtiles数据格式加载标准模板
// @skill-status: partial
// @skill-category: earth, gaussian splatting, 3dtiles, WMS, TMS
// @dependencies: osgVerseReaderWriter, osgDB
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgDB/ReadFile>
#include <osgVerse/readerwriter/Utilities.h>

// 从高德地图的TMS影像服务构建三维地球（UseEarth3D=1）或者平面地图（UseEarth3D=0），影像地址注意URL编码。
// 重要参数设置：瓦片图像原点在左上角（OriginBottomLeft=0），采用Web墨卡托投影（UseWebMercator=1）
{
    std::string options = "Orthophoto=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} "
                          "OriginBottomLeft=0 UseWebMercator=1 UseEarth3D=1";
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("0-0-0.verse_tms", new osgDB::Options(options));
}

// 从本地的.mbtiles影像和地形瓦片文件构建三维地球（UseEarth3D=1）或者平面地图（UseEarth3D=0）
// 这里的E:/satellite-2017-z13.mbtiles和E:/elevation-google-z8.mbtiles是本地文件名（注意Windows下也需要使用/分隔符），mbtiles://是前缀协议名，{z}-{x}-{y}.jpg是读取瓦片的匹配模式
{
    std::string options = "Orthophoto=mbtiles://E:/satellite-2017-z13.mbtiles/{z}-{x}-{y}.jpg "
                          "Elevation=mbtiles://E:/elevation-google-z8.mbtiles/{z}-{x}-{y}.tif "
                          "OriginBottomLeft=0 UseWebMercator=1 UseEarth3D=1";
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("0-0-0.verse_tms", new osgDB::Options(options));
}

// 从本地的TMS结构瓦片文件夹中加载影像数据，构建三维地球（UseEarth3D=1）或者平面地图（UseEarth3D=0）
// 这里假设本地图片的图像原点在左下角（OriginBottomLeft=1）
// 这里假设本地瓦片文件夹采用Web经纬度方式投影（UseWebMercator=0），即第0级文件夹中有两个根瓦片数据（0和1），它们分别构成各自的四叉树金字塔结构
{
    std::string options = "Orthophoto=G:/DOM_DEM/dom/{z}/{x}/{y}.jpg "
                          "OriginBottomLeft=0 UseWebMercator=1 UseEarth3D=1";
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("0-0-x.verse_tms", new osgDB::Options(options));
}

// 加载本地的osgb倾斜摄影数据，在metadata.xml文件名之后添加后缀.verse_tiles
{
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("path/to/root_folder/metadata.xml.verse_tiles");
}

// 加载本地的3dtiles倾斜摄影数据，在tileset.json文件名之后添加后缀.verse_tiles
{
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("path/to/root_folder/tileset.json.verse_tiles");
}

// 加载网络上的3dtiles倾斜摄影数据，在tileset.json文件名之后添加后缀.verse_web，同时在Options中指定读取瓦片文件的插件为verse_tiles
{
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("http://www.xxx.com/tileset.json.verse_web", new osgDB::Options("Extension=verse_tiles"));
}

// 加载三维高斯泼溅数据，支持PLY，SPLAT，LCC，SOG格式的文件
// 如果是3dtiles形式的三维高斯泼溅数据，则直接使用之前的方法，在tileset.json文件名之后添加后缀.verse_tiles即可
{
    // 主函数main启动时，必须进行osgVerse全局初始化
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());

    // 之后的初始化操作

    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("path/to/file.ply.verse_3dgs");

    // 之后的场景操作
}

