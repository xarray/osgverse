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
        // Built-in defaults: public online tile sources reachable without a key.
        // AutoNavi (GaoDe) satellite imagery is used by default because it is
        // directly reachable in mainland China (Google's endpoints are not).
        // Elevation is left empty by default so the globe is a clean ellipsoid
        // and startup never waits on a terrain endpoint; set globe.elevation in
        // the config to enable terrain.
        const char* kDefaultOrthophoto =
            "https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z}";
        const char* kDefaultElevation = "";

        // ---- Small JSON accessors: on missing/mismatched fields return the
        //      default value and print a console warning ----
        std::string getString(const picojson::object& obj, const char* key,
                              const std::string& defVal, const char* ctx)
        {
            picojson::object::const_iterator it = obj.find(key);
            if (it == obj.end()) return defVal;
            if (!it->second.is<std::string>())
            { OSG_WARN << "[EarthConfig] config item " << ctx << "." << key
                       << " should be a string, ignored\n"; return defVal; }
            return it->second.get<std::string>();
        }

        double getNumber(const picojson::object& obj, const char* key,
                         double defVal, const char* ctx)
        {
            picojson::object::const_iterator it = obj.find(key);
            if (it == obj.end()) return defVal;
            if (!it->second.is<double>())
            { OSG_WARN << "[EarthConfig] config item " << ctx << "." << key
                       << " should be a number, ignored\n"; return defVal; }
            return it->second.get<double>();
        }

        bool getBool(const picojson::object& obj, const char* key,
                     bool defVal, const char* ctx)
        {
            picojson::object::const_iterator it = obj.find(key);
            if (it == obj.end()) return defVal;
            if (!it->second.is<bool>())
            { OSG_WARN << "[EarthConfig] config item " << ctx << "." << key
                       << " should be a boolean, ignored\n"; return defVal; }
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
            sc.group = getString(s, "group", "Live Data / Live", "data_sources[]");
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

        // DeepSeek: OpenAI-compatible protocol, directly reachable in China and
        // cost effective, so it is the default first choice.
        AIProviderConfig deepseek;
        deepseek.name = "deepseek";
        deepseek.type = "openai_compatible";
        deepseek.baseUrl = "https://api.deepseek.com/v1";
        deepseek.model = "deepseek-chat";
        deepseek.keyEnv = "EARTH_DEEPSEEK_KEY";
        deepseek.keyConfig = "deepseek";
        list.push_back(deepseek);

        // Gemini: Google generateContent protocol (legacy from the reference
        // version; usable when an overseas network is available).
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
            OSG_WARN << "[EarthConfig] AI provider " << p.name
                     << " is missing its API Key (environment variable "
                     << (p.keyEnv.empty() ? "-" : p.keyEnv) << " / config api_keys."
                     << (p.keyConfig.empty() ? "-" : p.keyConfig)
                     << " are both unset), this provider will be disabled\n";
        }
        return "";
    }

    std::vector<SourceConfig> defaultSourceConfigs()
    {
        std::vector<SourceConfig> list;

        SourceConfig gdacs;
        gdacs.id = "gdacs"; gdacs.type = "gdacs";
        gdacs.displayName = "Disaster Alerts (GDACS)";
        gdacs.group = "Live Data / Live";
        gdacs.subtitle = "GDACS - UN global disaster alerts";
        gdacs.url = "https://www.gdacs.org/gdacsapi/api/events/geteventlist/MAP";
        gdacs.refreshSeconds = 600;
        list.push_back(gdacs);

        SourceConfig usgs;
        usgs.id = "quakes"; usgs.type = "usgs";
        usgs.displayName = "Earthquakes (USGS)";
        usgs.group = "Live Data / Live";
        usgs.subtitle = "USGS - M2.5+ earthquakes in the last 24 hours";
        usgs.url = "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_day.geojson";
        usgs.refreshSeconds = 300;
        list.push_back(usgs);

        SourceConfig eonet;
        eonet.id = "eonet"; eonet.type = "eonet";
        eonet.displayName = "Natural Events (NASA EONET)";
        eonet.group = "Live Data / Live";
        eonet.subtitle = "NASA EONET - active natural events";
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
                OSG_WARN << "[EarthConfig] file given by --config does not exist: " << explicitPath
                         << ", falling back to the default search path\n";
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
            OSG_WARN << "[EarthConfig] earth_explorer.json not found (searched --config / "
                     << BASE_DIR << "/misc/ / current directory), using built-in defaults:\n"
                     << "[EarthConfig]   imagery = AutoNavi (GaoDe) satellite, elevation = none, "
                        "data sources = GDACS / USGS / EONET\n";
            cfg.sources = defaultSourceConfigs();
            cfg.ai.providers = defaultAIProviderConfigs();
            return cfg;
        }

        picojson::value root;
        std::string err = picojson::parse(root, content);
        if (!err.empty() || !root.is<picojson::object>())
        {
            OSG_WARN << "[EarthConfig] failed to parse config file (" << pathUsed << "): "
                     << (err.empty() ? "root node is not an object" : err) << ", using built-in defaults\n";
            cfg.sources = defaultSourceConfigs();
            cfg.ai.providers = defaultAIProviderConfigs();
            return cfg;
        }

        cfg.loaded = true; cfg.path = pathUsed;
        const picojson::object& rootObj = root.get<picojson::object>();
        OSG_NOTICE << "[EarthConfig] loaded config: " << pathUsed << "\n";

        // ---- globe section ----
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
            OSG_WARN << "[EarthConfig] missing globe section, using built-in imagery/elevation defaults\n";
            cfg.orthophotoTemplate = kDefaultOrthophoto;
            cfg.elevationTemplate = kDefaultElevation;
        }

        // ---- city section ----
        if (const picojson::object* city = getObject(rootObj, "city"))
        {
            cfg.cityEnabled = getBool(*city, "enabled", false, "city");
            cfg.cityFolder = getString(*city, "folder", "", "city");
        }

        // ---- ui section ----
        if (const picojson::object* ui = getObject(rootObj, "ui"))
            cfg.fontPath = getString(*ui, "font", "", "ui");

        // ---- api_keys section ----
        if (const picojson::object* keys = getObject(rootObj, "api_keys"))
        {
            for (picojson::object::const_iterator it = keys->begin(); it != keys->end(); ++it)
            {
                if (it->second.is<std::string>())
                    cfg.apiKeys[it->first] = it->second.get<std::string>();
            }
        }

        // ---- data_sources section ----
        picojson::object::const_iterator dsIt = rootObj.find("data_sources");
        if (dsIt != rootObj.end() && dsIt->second.is<picojson::array>())
        {
            const picojson::array& arr = dsIt->second.get<picojson::array>();
            for (size_t i = 0; i < arr.size(); ++i)
            {
                if (!arr[i].is<picojson::object>())
                { OSG_WARN << "[EarthConfig] data_sources[" << i << "] is not an object, skipped\n"; continue; }
                SourceConfig sc = parseSource(arr[i].get<picojson::object>());
                if (sc.id.empty() || sc.url.empty())
                { OSG_WARN << "[EarthConfig] data_sources[" << i << "] is missing id or url, skipped\n"; continue; }
                cfg.sources.push_back(sc);
            }
            if (cfg.sources.empty())
                OSG_WARN << "[EarthConfig] data_sources is empty, the data source panel will be entirely unavailable\n";
        }
        else
        {
            OSG_WARN << "[EarthConfig] missing data_sources section, using built-in defaults"
                        "(GDACS / USGS / EONET)\n";
            cfg.sources = defaultSourceConfigs();
        }

        // ---- ai section (AI assistant; missing = built-in providers, a missing key only disables that provider) ----
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
                                 << " is not an object, skipped\n";
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
                        OSG_WARN << "[EarthConfig] " << ctx << " is missing type"
                                    "(openai_compatible / gemini / fake), skipped\n";
                        continue;
                    }
                    if (pc.type != "fake" && pc.model.empty())
                    { OSG_WARN << "[EarthConfig] " << ctx << " is missing model, skipped\n"; continue; }
                    if (pc.timeoutSeconds <= 0) pc.timeoutSeconds = 60;
                    cfg.ai.providers.push_back(pc);
                }
                if (cfg.ai.providers.empty())
                    OSG_WARN << "[EarthConfig] ai.providers is empty, the AI assistant will be unavailable\n";
            }
            else
            {
                OSG_WARN << "[EarthConfig] missing ai.providers section, using built-in defaults"
                            "(deepseek / gemini)\n";
                cfg.ai.providers = defaultAIProviderConfigs();
            }
        }
        else
        {
            cfg.ai.providers = defaultAIProviderConfigs();
            OSG_NOTICE << "[EarthConfig] ai section not configured, using built-in AI providers"
                          "(deepseek / gemini, auto-disabled when the key is missing)\n";
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
        static std::set<std::string> s_reported;  // warn only once per key
        if (s_reported.insert(src.id).second)
        {
            OSG_WARN << "[EarthConfig] data source " << src.id << " is missing its API Key (environment variable "
                     << (src.keyEnv.empty() ? "-" : src.keyEnv) << " / config api_keys."
                     << (src.keyConfig.empty() ? "-" : src.keyConfig)
                     << " are both unset), this source will be disabled\n";
        }
        return "";
    }
}
