#ifndef EARTH_APP_CONFIG_H
#define EARTH_APP_CONFIG_H

#include <map>
#include <string>
#include <vector>
#include <osg/ArgumentParser>

// Unified application config for EarthExplorer. Design goals:
// 1. External API endpoints / credentials / refresh intervals are all collected
//    into a single JSON file (assets/misc/earth_explorer.json); the source code
//    no longer scatters hard-coded URLs;
// 2. Any missing config only causes a "degrade + console warning", never a crash
//    or an exit -- without a config file the program still shows the earth and
//    the UI (the data source panel is simply unavailable).
namespace earthee
{
    // Description of a single external data source in the config file
    struct SourceConfig
    {
        std::string id;            // Stable id, e.g. "gdacs" (marker shape/layer lookup use it)
        std::string type;          // Registered DataSourceFactory type name, e.g. "gdacs"
        std::string displayName;   // Name shown in the UI
        std::string group;         // UI group, e.g. "Live Data / Live"
        std::string subtitle;      // Small text under the layer entry (data source attribution)
        std::string url;           // API address; the "{key}" placeholder is replaced by the resolved credential
        std::string keyEnv;        // Credential environment variable name (preferred), may be empty
        std::string keyConfig;     // Key name inside the config api_keys section, may be empty
        int refreshSeconds = 600;  // Polling interval (seconds); <=0 is treated as 600
        int timeoutSeconds = 15;   // Single fetch timeout (seconds)
        double liftMeters = 3000.0;// Height to lift markers above the ellipsoid (meters), prevents sinking into terrain
        bool enabled = false;      // Whether it is enabled by default at startup
        bool needsKey() const { return !keyEnv.empty() || !keyConfig.empty(); }
    };

    // Description of a single AI assistant provider (config ai.providers section)
    struct AIProviderConfig
    {
        std::string name;         // Config key name, e.g. "deepseek"
        std::string type;         // openai_compatible / gemini / fake
        std::string baseUrl;      // API root address for openai_compatible
        std::string model;        // Model name, e.g. deepseek-chat / gemini-2.5-flash
        std::string keyEnv;       // Credential environment variable name (preferred), may be empty
        std::string keyConfig;    // Key name inside the config api_keys section, may be empty
        std::string scriptFile;   // Replay script path for the fake type
        int timeoutSeconds = 60;
        bool needsKey() const { return type != "fake"; }
    };

    // AI assistant config (every field has a default; missing config = AI panel greyed out, other features unaffected)
    struct AIConfig
    {
        bool enabled = true;           // false = do not create AI at all (panel hidden)
        std::string active;            // Identifier of the enabled provider; empty = take the first of providers
        int maxRounds = 30;            // Max tool-call rounds for a single instruction
        std::string systemPrompt;      // Empty = built-in default prompt
        std::vector<AIProviderConfig> providers;  // Empty = built-in deepseek+gemini fallback
    };

    // Application-level config. Every field has a built-in default (the built-in
    // default uses public online tile sources and needs no key).
    struct AppConfig
    {
        bool loaded = false;       // Whether the on-disk config file was read (not read = warning only, no exit)
        std::string path;          // Actually loaded file path (empty = built-in defaults in use)

        // ---- Earth imagery (URL templates, {z}/{x}/{y} placeholders; '=' in a URL must be written as %3d) ----
        std::string orthophotoTemplate;  // Imagery, empty = built-in AutoNavi (GaoDe) satellite
        std::string elevationTemplate;   // Elevation, empty = no elevation (pure ellipsoid)
        std::string oceanMaskTemplate;   // Ocean mask, empty = ocean shading effect disabled
        double skirtRatio = 0.05;
        double elevationScale = 1.0;
        bool useWebMercator = true;
        bool originBottomLeft = false;

        // ---- City data (local assets, disabled by default) ----
        bool cityEnabled = false;
        std::string cityFolder;    // Empty = BASE_DIR/models

        // ---- Interface ----
        std::string fontPath;      // Vector font, empty = auto-detect the built-in font under MISC_DIR

        // ---- External credential table (env takes priority; the source never hard-codes any key value) ----
        std::map<std::string, std::string> apiKeys;

        // ---- External data source list (empty = fall back to defaultSourceConfigs()) ----
        std::vector<SourceConfig> sources;

        // ---- AI assistant ----
        AIConfig ai;
    };

    // Load the config. Search order: --config <file> > BASE_DIR/misc/earth_explorer.json >
    // ./earth_explorer.json. A missing file / missing field / wrong type only prints an
    // OSG_WARN and returns a usable config.
    AppConfig loadAppConfig(osg::ArgumentParser& arguments);

    // Resolve the credential of a data source: environment variable (e.g. EARTH_FIRMS_KEY)
    // first, then fall back to the config api_keys. Returns an empty string when neither is
    // set (the caller should disable the source and show "missing Key" in the UI); the console
    // warning is printed only once.
    std::string resolveKey(const AppConfig& cfg, const SourceConfig& src);

    // Built-in default data sources (GDACS disasters / USGS earthquakes / NASA EONET, all key-free)
    std::vector<SourceConfig> defaultSourceConfigs();

    // Built-in default AI providers (DeepSeek OpenAI-compatible + Gemini; both need a key and are disabled when missing)
    std::vector<AIProviderConfig> defaultAIProviderConfigs();

    // Resolve an AI provider credential: environment variable first, then api_keys; warn once and return empty when neither is set
    std::string resolveAIKey(const AppConfig& cfg, const AIProviderConfig& p);
}

#endif
