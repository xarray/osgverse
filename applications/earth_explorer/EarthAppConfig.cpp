#include "EarthAppConfig.h"
#include <picojson.h>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <pipeline/Global.h>
#include <osg/Notify>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>

namespace earthee
{
    namespace
    {
        // 内置默认:朋友版 earth_main.cpp 中验证过的在线公共图源。
        // Terrarium 高程走 ".png.verse_terrain" 伪扩展 → osgdb_terrain 插件解码。
        const char* kDefaultOrthophoto =
            "https://mt1.google.com/vt/lyrs%3ds&x%3d{x}&y%3d{y}&z%3d{z}";
        const char* kDefaultElevation =
            "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{z}/{x}/{y}.png.verse_terrain";

        // ---- JSON 取值小助手:字段缺失/类型不符时返回默认值并在控制台告警 ----
        std::string getString(const picojson::object& obj, const char* key,
                              const std::string& defVal, const char* ctx)
        {
            picojson::object::const_iterator it = obj.find(key);
            if (it == obj.end()) return defVal;
            if (!it->second.is<std::string>())
            { OSG_WARN << "[EarthConfig] " << u8"配置项 " << ctx << "." << key
                       << u8" 类型应为字符串,已忽略\n"; return defVal; }
            return it->second.get<std::string>();
        }

        double getNumber(const picojson::object& obj, const char* key,
                         double defVal, const char* ctx)
        {
            picojson::object::const_iterator it = obj.find(key);
            if (it == obj.end()) return defVal;
            if (!it->second.is<double>())
            { OSG_WARN << "[EarthConfig] " << u8"配置项 " << ctx << "." << key
                       << u8" 类型应为数字,已忽略\n"; return defVal; }
            return it->second.get<double>();
        }

        bool getBool(const picojson::object& obj, const char* key,
                     bool defVal, const char* ctx)
        {
            picojson::object::const_iterator it = obj.find(key);
            if (it == obj.end()) return defVal;
            if (!it->second.is<bool>())
            { OSG_WARN << "[EarthConfig] " << u8"配置项 " << ctx << "." << key
                       << u8" 类型应为布尔,已忽略\n"; return defVal; }
            return it->second.get<bool>();
        }

        const picojson::object* getObject(const picojson::object& obj, const char* key)
        {
            picojson::object::const_iterator it = obj.find(key);
            if (it == obj.end() || !it->second.is<picojson::object>()) return NULL;
            return &it->second.get<picojson::object>();
        }

        SourceConfig parseSource(const picojson::object& s)
        {
            SourceConfig sc;
            sc.id = getString(s, "id", "", "data_sources[]");
            sc.type = getString(s, "type", sc.id, "data_sources[]");
            sc.displayName = getString(s, "name", sc.id, "data_sources[]");
            sc.group = getString(s, "group", u8"实时数据 / Live", "data_sources[]");
            sc.subtitle = getString(s, "subtitle", "", "data_sources[]");
            sc.url = getString(s, "url", "", "data_sources[]");
            sc.keyEnv = getString(s, "key_env", "", "data_sources[]");
            sc.keyConfig = getString(s, "key_config", "", "data_sources[]");
            sc.refreshSeconds = (int)getNumber(s, "refresh_seconds", 600.0, "data_sources[]");
            sc.timeoutSeconds = (int)getNumber(s, "timeout_seconds", 15.0, "data_sources[]");
            sc.liftMeters = getNumber(s, "lift_meters", 3000.0, "data_sources[]");
            sc.enabled = getBool(s, "enabled", false, "data_sources[]");
            if (sc.refreshSeconds <= 0) sc.refreshSeconds = 600;
            if (sc.timeoutSeconds <= 0) sc.timeoutSeconds = 15;
            return sc;
        }
    }

    std::vector<AIProviderConfig> defaultAIProviderConfigs()
    {
        std::vector<AIProviderConfig> list;

        // DeepSeek:OpenAI 兼容协议,国内直连、性价比高,作为默认首选
        AIProviderConfig deepseek;
        deepseek.name = "deepseek";
        deepseek.type = "openai_compatible";
        deepseek.baseUrl = "https://api.deepseek.com/v1";
        deepseek.model = "deepseek-chat";
        deepseek.keyEnv = "EARTH_DEEPSEEK_KEY";
        deepseek.keyConfig = "deepseek";
        list.push_back(deepseek);

        // Gemini:Google generateContent 协议(朋友版遗留,海外网络可达时可用)
        AIProviderConfig gemini;
        gemini.name = "gemini";
        gemini.type = "gemini";
        gemini.model = "gemini-2.5-flash";
        gemini.keyEnv = "EARTH_GEMINI_KEY";
        gemini.keyConfig = "gemini";
        list.push_back(gemini);
        return list;
    }

    std::string resolveAIKey(const AppConfig& cfg, const AIProviderConfig& p)
    {
        if (!p.needsKey()) return "";
        if (!p.keyEnv.empty())
        {
            const char* v = getenv(p.keyEnv.c_str());
            if (v && *v) return v;
        }
        if (!p.keyConfig.empty())
        {
            std::map<std::string, std::string>::const_iterator it =
                cfg.apiKeys.find(p.keyConfig);
            if (it != cfg.apiKeys.end() && !it->second.empty()) return it->second;
        }
        static std::set<std::string> s_reported;
        if (s_reported.insert(p.name).second)
        {
            OSG_WARN << "[EarthConfig] " << u8"AI 供应商 " << p.name
                     << u8" 缺少 API Key(环境变量 "
                     << (p.keyEnv.empty() ? "-" : p.keyEnv) << u8" / 配置 api_keys."
                     << (p.keyConfig.empty() ? "-" : p.keyConfig)
                     << u8" 均未设置),该供应商将禁用\n";
        }
        return "";
    }

    std::vector<SourceConfig> defaultSourceConfigs()
    {
        std::vector<SourceConfig> list;

        SourceConfig gdacs;
        gdacs.id = "gdacs"; gdacs.type = "gdacs";
        gdacs.displayName = u8"灾害预警 (GDACS)";
        gdacs.group = u8"实时数据 / Live";
        gdacs.subtitle = u8"GDACS · UN 全球灾害预警";
        gdacs.url = "https://www.gdacs.org/gdacsapi/api/events/geteventlist/MAP";
        gdacs.refreshSeconds = 600;
        list.push_back(gdacs);

        SourceConfig usgs;
        usgs.id = "quakes"; usgs.type = "usgs";
        usgs.displayName = u8"地震 (USGS)";
        usgs.group = u8"实时数据 / Live";
        usgs.subtitle = u8"USGS · 近24小时 M2.5+ 地震";
        usgs.url = "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_day.geojson";
        usgs.refreshSeconds = 300;
        list.push_back(usgs);

        SourceConfig eonet;
        eonet.id = "eonet"; eonet.type = "eonet";
        eonet.displayName = u8"自然事件 (NASA EONET)";
        eonet.group = u8"实时数据 / Live";
        eonet.subtitle = u8"NASA EONET · 活跃自然事件";
        eonet.url = "https://eonet.gsfc.nasa.gov/api/v3/events?status=open&limit=200";
        eonet.refreshSeconds = 900;
        list.push_back(eonet);
        return list;
    }

    AppConfig loadAppConfig(osg::ArgumentParser& arguments)
    {
        AppConfig cfg;
        std::string explicitPath;
        if (arguments.read("--config", explicitPath))
        {
            if (!osgDB::fileExists(explicitPath))
                OSG_WARN << "[EarthConfig] --config 指定的文件不存在: " << explicitPath
                         << ",将尝试默认搜索路径\n";
        }

        std::string candidates[3] = { explicitPath,
                                      BASE_DIR + "/misc/earth_explorer.json",
                                      "earth_explorer.json" };
        std::string content, pathUsed;
        for (int i = 0; i < 3; ++i)
        {
            if (candidates[i].empty() || !osgDB::fileExists(candidates[i])) continue;
            std::ifstream in(candidates[i].c_str());
            if (!in) continue;
            std::stringstream ss; ss << in.rdbuf(); content = ss.str();
            pathUsed = candidates[i];
            break;
        }

        if (content.empty())
        {
            OSG_WARN << "[EarthConfig] " << u8"未找到 earth_explorer.json(搜索过 --config / "
                     << BASE_DIR << "/misc/ / 当前目录)," << u8"使用内置默认配置:\n"
                     << "[EarthConfig]   " << u8"底图 = Google 卫星图, 高程 = AWS Terrarium, "
                        u8"数据源 = GDACS / USGS / EONET\n";
            cfg.sources = defaultSourceConfigs();
            cfg.ai.providers = defaultAIProviderConfigs();
            return cfg;
        }

        picojson::value root;
        std::string err = picojson::parse(root, content);
        if (!err.empty() || !root.is<picojson::object>())
        {
            OSG_WARN << "[EarthConfig] " << u8"配置文件解析失败(" << pathUsed << "): "
                     << (err.empty() ? u8"根节点不是对象" : err) << u8",使用内置默认配置\n";
            cfg.sources = defaultSourceConfigs();
            cfg.ai.providers = defaultAIProviderConfigs();
            return cfg;
        }

        cfg.loaded = true; cfg.path = pathUsed;
        const picojson::object& rootObj = root.get<picojson::object>();
        OSG_NOTICE << "[EarthConfig] 已加载配置: " << pathUsed << "\n";

        // ---- globe 段 ----
        if (const picojson::object* globe = getObject(rootObj, "globe"))
        {
            cfg.orthophotoTemplate = getString(*globe, "orthophoto", kDefaultOrthophoto, "globe");
            cfg.elevationTemplate = getString(*globe, "elevation", kDefaultElevation, "globe");
            cfg.oceanMaskTemplate = getString(*globe, "ocean_mask", "", "globe");
            cfg.skirtRatio = getNumber(*globe, "skirt_ratio", 0.05, "globe");
            cfg.elevationScale = getNumber(*globe, "elevation_scale", 1.0, "globe");
            cfg.useWebMercator = getBool(*globe, "web_mercator", true, "globe");
            cfg.originBottomLeft = getBool(*globe, "origin_bottom_left", false, "globe");
        }
        else
        {
            OSG_WARN << "[EarthConfig] " << u8"缺少 globe 段,底图/高程使用内置默认\n";
            cfg.orthophotoTemplate = kDefaultOrthophoto;
            cfg.elevationTemplate = kDefaultElevation;
        }

        // ---- city 段 ----
        if (const picojson::object* city = getObject(rootObj, "city"))
        {
            cfg.cityEnabled = getBool(*city, "enabled", false, "city");
            cfg.cityFolder = getString(*city, "folder", "", "city");
        }

        // ---- ui 段 ----
        if (const picojson::object* ui = getObject(rootObj, "ui"))
            cfg.fontPath = getString(*ui, "font", "", "ui");

        // ---- api_keys 段 ----
        if (const picojson::object* keys = getObject(rootObj, "api_keys"))
        {
            for (picojson::object::const_iterator it = keys->begin(); it != keys->end(); ++it)
            {
                if (it->second.is<std::string>())
                    cfg.apiKeys[it->first] = it->second.get<std::string>();
            }
        }

        // ---- data_sources 段 ----
        picojson::object::const_iterator dsIt = rootObj.find("data_sources");
        if (dsIt != rootObj.end() && dsIt->second.is<picojson::array>())
        {
            const picojson::array& arr = dsIt->second.get<picojson::array>();
            for (size_t i = 0; i < arr.size(); ++i)
            {
                if (!arr[i].is<picojson::object>())
                { OSG_WARN << "[EarthConfig] data_sources[" << i << "] " << u8"不是对象,已跳过\n"; continue; }
                SourceConfig sc = parseSource(arr[i].get<picojson::object>());
                if (sc.id.empty() || sc.url.empty())
                { OSG_WARN << "[EarthConfig] data_sources[" << i << "] " << u8"缺少 id 或 url,已跳过\n"; continue; }
                cfg.sources.push_back(sc);
            }
            if (cfg.sources.empty())
                OSG_WARN << "[EarthConfig] data_sources " << u8"为空,界面数据源面板将全部不可用\n";
        }
        else
        {
            OSG_WARN << "[EarthConfig] " << u8"缺少 data_sources 段,使用内置默认数据源"
                        "(GDACS / USGS / EONET)\n";
            cfg.sources = defaultSourceConfigs();
        }

        // ---- ai 段(AI 助手;缺失 = 内置默认供应商,缺 key 只禁用对应家)----
        if (const picojson::object* ai = getObject(rootObj, "ai"))
        {
            cfg.ai.enabled = getBool(*ai, "enabled", true, "ai");
            cfg.ai.active = getString(*ai, "active", "", "ai");
            cfg.ai.maxRounds = (int)getNumber(*ai, "max_rounds", 30.0, "ai");
            cfg.ai.systemPrompt = getString(*ai, "system_prompt", "", "ai");
            if (cfg.ai.maxRounds <= 0) cfg.ai.maxRounds = 30;

            if (const picojson::object* providers = getObject(*ai, "providers"))
            {
                for (picojson::object::const_iterator it = providers->begin();
                     it != providers->end(); ++it)
                {
                    if (!it->second.is<picojson::object>())
                    {
                        OSG_WARN << "[EarthConfig] ai.providers." << it->first
                                 << u8" 不是对象,已跳过\n";
                        continue;
                    }
                    const picojson::object& po = it->second.get<picojson::object>();
                    std::string ctx = "ai.providers." + it->first;
                    AIProviderConfig pc;
                    pc.name = it->first;
                    pc.type = getString(po, "type", "", ctx.c_str());
                    pc.baseUrl = getString(po, "base_url", "https://api.deepseek.com/v1", ctx.c_str());
                    pc.model = getString(po, "model", "deepseek-chat", ctx.c_str());
                    pc.keyEnv = getString(po, "key_env", "", ctx.c_str());
                    pc.keyConfig = getString(po, "key_config", pc.name, ctx.c_str());
                    pc.scriptFile = getString(po, "script", "", ctx.c_str());
                    pc.timeoutSeconds = (int)getNumber(po, "timeout_seconds", 60.0, ctx.c_str());
                    if (pc.type.empty())
                    {
                        OSG_WARN << "[EarthConfig] " << ctx << " " << u8"缺少 type"
                                    "(openai_compatible / gemini / fake),已跳过\n";
                        continue;
                    }
                    if (pc.type != "fake" && pc.model.empty())
                    { OSG_WARN << "[EarthConfig] " << ctx << " " << u8"缺少 model,已跳过\n"; continue; }
                    if (pc.timeoutSeconds <= 0) pc.timeoutSeconds = 60;
                    cfg.ai.providers.push_back(pc);
                }
                if (cfg.ai.providers.empty())
                    OSG_WARN << "[EarthConfig] ai.providers " << u8"为空,AI 助手将不可用\n";
            }
            else
            {
                OSG_WARN << "[EarthConfig] " << u8"缺少 ai.providers 段,使用内置默认"
                            "(deepseek / gemini)\n";
                cfg.ai.providers = defaultAIProviderConfigs();
            }
        }
        else
        {
            cfg.ai.providers = defaultAIProviderConfigs();
            OSG_NOTICE << "[EarthConfig] " << u8"未配置 ai 段,使用内置默认 AI 供应商"
                          "(deepseek / gemini,缺 key 自动禁用)\n";
        }
        return cfg;
    }

    std::string resolveKey(const AppConfig& cfg, const SourceConfig& src)
    {
        if (!src.needsKey()) return "";
        if (!src.keyEnv.empty())
        {
            const char* v = getenv(src.keyEnv.c_str());
            if (v && *v) return v;
        }
        if (!src.keyConfig.empty())
        {
            std::map<std::string, std::string>::const_iterator it = cfg.apiKeys.find(src.keyConfig);
            if (it != cfg.apiKeys.end() && !it->second.empty()) return it->second;
        }
        static std::set<std::string> s_reported;  // 每个 key 只告警一次
        if (s_reported.insert(src.id).second)
        {
            OSG_WARN << "[EarthConfig] " << u8"数据源 " << src.id << u8" 缺少 API Key(环境变量 "
                     << (src.keyEnv.empty() ? "-" : src.keyEnv) << u8" / 配置 api_keys."
                     << (src.keyConfig.empty() ? "-" : src.keyConfig)
                     << u8" 均未设置),该源将被禁用\n";
        }
        return "";
    }
}
