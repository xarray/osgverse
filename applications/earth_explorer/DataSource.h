#ifndef EARTH_DATA_SOURCE_H
#define EARTH_DATA_SOURCE_H

#include <osg/Geode>
#include <osg/Group>
#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>
#include <OpenThreads/Mutex>
#include <OpenThreads/Thread>
#include <picojson.h>
#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "EarthAppConfig.h"
#include "LayerRegistry.h"

// 配置驱动的"世界数据源"框架(精简移植自朋友版 feed_layer):
// - DataSource: 抓取线程 + 主线程同步重建 GL_POINTS + 拾取 + 详情数据;
// - HttpJsonSource: GET(osgVerse::loadFileData,复用核心库 HTTP 客户端)+
//   picojson 解析的中间基类,具体源只需实现 parsePoints();
// - DataSourceFactory: 类型名 → 构造函数的注册表,配置文件的 data_sources[].type
//   据此实例化,新增数据源不需要改框架代码;
// - DataSourceManager: 持有全部源、注册全局拾取处理器、向 LayerRegistry 报告状态。
namespace earthds
{
    // 注册内置数据源类型(feed_sources.cpp;main 里在 initialize 之前调用一次)
    void registerBuiltinSourceTypes();

    struct DataPoint
    {
        double lat = 0.0, lon = 0.0;   // 度
        float sizePx = 8.0f;
        osg::Vec4 color = osg::Vec4(1, 1, 1, 1);
        std::string title, detail;     // 详情卡标题/正文(已格式化多行)
        std::string url;               // 可空:详情卡"打开"按钮的链接
    };

    // 跨全部源的当前选中要素(拾取处理器写、UI 读,内部有锁)
    struct Selection
    {
        bool valid = false;
        std::string title, detail, url, sourceId;
    };
    Selection currentSelection();
    void clearSelection();

    class DataSource : public osg::Referenced
    {
    public:
        DataSource(const earthee::SourceConfig& cfg);
        const earthee::SourceConfig& config() const { return _cfg; }

        // 线程约定:enabled 由 UI(draw 线程)写、抓取线程读 → atomic
        void setEnabled(bool on);
        bool isEnabled() const { return _enabled; }

        // 0=未抓取,1=成功有要素,2=失败(见 lastError),3=成功但 0 要素
        int fetchState() const { return _fetchState; }
        std::string lastError() const;
        size_t pointCount() const;

        // 取前 maxN 条要素标题(AI 助手的 get_data_summary 样本用;加锁拷贝)
        void sampleTitles(std::vector<std::string>& out, size_t maxN) const;

        // 挂到场景的根节点(内含抓取线程,随节点析构而 join)
        osg::Group* root() { return _root; }
        void pickAt(osg::Camera* cam, float mx, float my);  // 主/事件线程
        void syncIfDirty();                                 // 主线程(更新回调)
        void pollFetch();                                   // 抓取线程(轮询体)

    protected:
        virtual ~DataSource();
        // 抓取线程调用:取数 + 解析。失败返回 false 并填写 err。
        virtual bool fetch(std::vector<DataPoint>& points, std::string& err) = 0;

        const earthee::SourceConfig _cfg;

    private:
        struct Record { DataPoint pt; osg::Vec3d ecef; };
        struct Snapshot { std::vector<Record> records; };

        void postSnapshot(Snapshot snap);
        osg::Geode* buildGeodeFrom(const std::vector<Record>& recs);
        void startFetch();
        void stopFetch();

        int _shapeId;                       // earthmark::visualForLayer(id).shape
        osg::Group* _root;
        osg::ref_ptr<osg::Geode> _geode;
        osg::ref_ptr<osg::StateSet> _ss;
        std::vector<Record> _records;
        Snapshot _pending;
        mutable OpenThreads::Mutex _mutex;  // 守护 _records/_pending
        mutable OpenThreads::Mutex _errMutex;
        std::string _lastErrorText;
        std::atomic<bool> _enabled;
        std::atomic<int> _fetchState;       // 0/1/2/3
        bool _dirty;
        int _tick;                        // 抓取线程的轮询节拍计数
        OpenThreads::Thread* _thread;
    };

    // HTTP GET + JSON 解析的公共基类:URL 中 "{key}" 会被 resolveKey 的结果替换
    class HttpJsonSource : public DataSource
    {
    protected:
        HttpJsonSource(const earthee::SourceConfig& cfg, const earthee::AppConfig& app);
        virtual bool fetch(std::vector<DataPoint>& points, std::string& err);
        // 解析 JSON 根对象产出要素;返回 false = 解析失败(数据格式不符)
        virtual bool parsePoints(const picojson::value& root, std::vector<DataPoint>& points) = 0;
    private:
        std::string _resolvedKey;
    };

    // 类型注册表:data_sources[].type → Creator。registerBuiltinSourceTypes()
    // 注册内置源;扩展新源 = 新文件里 REGISTRY 注册,零侵入。
    class DataSourceFactory
    {
    public:
        typedef DataSource* (*Creator)(const earthee::SourceConfig&, const earthee::AppConfig&);
        static DataSourceFactory& instance()
        { static DataSourceFactory s; return s; }

        void registerType(const std::string& type, Creator c) { _creators[type] = c; }
        DataSource* create(const earthee::SourceConfig& cfg, const earthee::AppConfig& app) const
        {
            std::map<std::string, Creator>::const_iterator it = _creators.find(cfg.type);
            if (it == _creators.end()) return NULL;
            return it->second(cfg, app);
        }
        bool hasType(const std::string& type) const { return _creators.count(type) > 0; }
    private:
        std::map<std::string, Creator> _creators;
    };

    // 全部数据源的管理器(应用持有,随场景销毁)
    class DataSourceManager
    {
    public:
        // 按配置实例化全部源,注册图层条目与拾取处理器;返回挂到场景相机的根组。
        // 任何源创建失败都只是控制台告警 + 跳过,不影响其余源。
        osg::Group* initialize(const earthee::AppConfig& cfg, osgViewer::Viewer& viewer,
                               LayerRegistry& layers);

        // 状态带 "n/m":开启且最近一轮抓取成功的源数 / 开启的源数
        void health(int& okCount, int& enabledCount) const;
        const std::vector<osg::ref_ptr<DataSource>>& sources() const { return _sources; }
        void clearSelectionIfSource(const std::string& sourceId);

    private:
        std::vector<osg::ref_ptr<DataSource>> _sources;
    };
}

#endif
