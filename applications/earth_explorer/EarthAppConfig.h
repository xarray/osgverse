#ifndef EARTH_APP_CONFIG_H
#define EARTH_APP_CONFIG_H

#include <map>
#include <string>
#include <vector>
#include <osg/ArgumentParser>

// EarthExplorer 统一应用配置。设计目标:
// 1. 外部 API 地址 / 凭证 / 刷新周期等全部收拢到一个 JSON 配置文件
//    (assets/misc/earth_explorer.json),源码不再散落硬编码 URL;
// 2. 任何配置缺失都只是"降级 + 控制台告警",绝不允许崩溃或退出——
//    没有配置文件时程序照常显示地球与 UI(数据源面板为不可用状态)。
namespace earthee
{
    // 单个外部数据源在配置文件中的描述
    struct SourceConfig
    {
        std::string id;            // 稳定 id,如 "gdacs"(标记形状/图层检索都用它)
        std::string type;          // DataSourceFactory 注册的类型名,如 "gdacs"
        std::string displayName;   // UI 显示名(中文)
        std::string group;         // UI 分组,如 "实时数据 / Live"
        std::string subtitle;      // 图层条目下方的小字(数据来源署名)
        std::string url;           // API 地址;其中的 "{key}" 占位符会被解析后的凭证替换
        std::string keyEnv;        // 凭证环境变量名(优先),可空
        std::string keyConfig;     // 配置文件 api_keys 段中的键名,可空
        int refreshSeconds = 600;  // 轮询间隔(秒);<=0 视为 600
        int timeoutSeconds = 15;   // 单次抓取超时(秒)
        double liftMeters = 3000.0;// 标记抬离椭球的高度(米),防沉入地形
        bool enabled = false;      // 启动时是否默认开启
        bool needsKey() const { return !keyEnv.empty() || !keyConfig.empty(); }
    };

    // AI 助手的单个供应商描述(配置文件 ai.providers 段)
    struct AIProviderConfig
    {
        std::string name;         // 配置键名,如 "deepseek"
        std::string type;         // openai_compatible / gemini / fake
        std::string baseUrl;      // openai_compatible 的 API 根地址
        std::string model;        // 模型名,如 deepseek-chat / gemini-2.5-flash
        std::string keyEnv;       // 凭证环境变量名(优先),可空
        std::string keyConfig;    // 配置文件 api_keys 段中的键名,可空
        std::string scriptFile;   // fake 类型的回放脚本路径
        int timeoutSeconds = 60;
        bool needsKey() const { return type != "fake"; }
    };

    // AI 助手配置(全部字段有默认;缺配置 = AI 面板灰显,不影响其它功能)
    struct AIConfig
    {
        bool enabled = true;           // false = 完全不创建 AI(不显示面板)
        std::string active;            // 启用的供应商标识;空 = 取 providers 第一个
        int maxRounds = 30;            // 单次指令的工具调用轮次上限
        std::string systemPrompt;      // 空 = 内置默认提示词
        std::vector<AIProviderConfig> providers;  // 空 = 内置 deepseek+gemini 兜底
    };

    // 应用级配置。所有字段都有内置默认值(内置默认 = 在线公共图源,无需 key)。
    struct AppConfig
    {
        bool loaded = false;       // 是否读到了磁盘配置文件(没读到只是告警,不退出)
        std::string path;          // 实际加载的文件路径(空 = 使用内置默认)

        // ---- 地球底图(URL 模板,{z}/{x}/{y} 占位;URL 中的 '=' 需写成 %3d)----
        std::string orthophotoTemplate;  // 影像底图,空 = 内置 Google 卫星图
        std::string elevationTemplate;   // 高程,空 = 无高程(纯椭球)
        std::string oceanMaskTemplate;   // 海洋掩膜,空 = 关闭海洋着色效果
        double skirtRatio = 0.05;
        double elevationScale = 1.0;
        bool useWebMercator = true;
        bool originBottomLeft = false;

        // ---- 城市数据(本地资产,默认关闭)----
        bool cityEnabled = false;
        std::string cityFolder;    // 空 = BASE_DIR/models

        // ---- 界面 ----
        std::string fontPath;      // 中文矢量字体,空 = 自动找 MISC_DIR 下的内置字体

        // ---- 外部凭证表(env 优先级更高;源码不硬编码任何 key 值)----
        std::map<std::string, std::string> apiKeys;

        // ---- 外部数据源列表(空 = 使用 defaultSourceConfigs() 兜底)----
        std::vector<SourceConfig> sources;

        // ---- AI 助手 ----
        AIConfig ai;
    };

    // 加载配置。搜索顺序:--config <file> > BASE_DIR/misc/earth_explorer.json >
    // ./earth_explorer.json。文件缺失/字段缺失/类型不对都只打 OSG_WARN,返回可用配置。
    AppConfig loadAppConfig(osg::ArgumentParser& arguments);

    // 解析某个数据源的凭证:环境变量(如 EARTH_FIRMS_KEY)优先,回退配置文件 api_keys。
    // 都没有时返回空串(调用方应禁用该源并在 UI 上提示"缺少 Key"),控制台告警只打一次。
    std::string resolveKey(const AppConfig& cfg, const SourceConfig& src);

    // 内置默认数据源(GDACS 灾害 / USGS 地震 / NASA EONET,全部免 key)
    std::vector<SourceConfig> defaultSourceConfigs();

    // 内置默认 AI 供应商(DeepSeek OpenAI 兼容 + Gemini;均需 key,缺 key 时该家禁用)
    std::vector<AIProviderConfig> defaultAIProviderConfigs();

    // 解析 AI 供应商凭证:环境变量优先,回退 api_keys;都没有时告警一次并返回空串
    std::string resolveAIKey(const AppConfig& cfg, const AIProviderConfig& p);
}

#endif
