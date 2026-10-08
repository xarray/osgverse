// Built-in data source implementations: GDACS disasters / USGS earthquakes / NASA EONET natural events.
// Each source = an HttpJsonSource subclass (implementing only parsePoints), registered through
// DataSourceFactory — the config file's data_sources[].type instantiates accordingly, so adding a
// source is zero-intrusion.
// The parsing logic is ported from the friend's feeds/gdacs_feed.cpp etc. (real response shapes verified).
#include "DataSource.h"
#include "marker_style.h"
#include <cstdio>
#include <ctime>

namespace earthds
{
    namespace
    {
        // ISO8601 (e.g. "2026-06-29T11:35:33", UTC) -> Unix seconds; returns 0 on failure
        long long parseIso8601(const std::string& s)
        {
            struct tm t = tm();
            if (sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday,
                       &t.tm_hour, &t.tm_min, &t.tm_sec) != 6) return 0;
            t.tm_year -= 1900; t.tm_mon -= 1;
#ifdef _WIN32
            return (long long)_mkgmtime(&t);
#else
            return (long long)timegm(&t);
#endif
        }

        std::string timeText(double unixSeconds)
        {
            if (unixSeconds <= 0) return "";
            time_t tt = (time_t)unixSeconds;
            char buf[64] = ""; struct tm t = tm();
#ifdef _WIN32
            gmtime_s(&t, &tt);
#else
            gmtime_r(&tt, &t);
#endif
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M UTC", &t);
            return buf;
        }

        std::string strField(const picojson::value& obj, const char* key)
        {
            if (!obj.is<picojson::object>()) return "";
            const picojson::value& v = obj.get(key);
            return v.is<std::string>() ? v.get<std::string>() : std::string();
        }

        // ===== GDACS (UN global disaster alerts) =====
        // The response is a FeatureCollection, but each event's polygon/polyline geometries each
        // take one Feature; only Class == "Point_Centroid" is the representative point; the Green
        // (non-urgent) level is filtered out and not shown.
        class GdacsSource : public HttpJsonSource
        {
        public:
            GdacsSource(const earthee::SourceConfig& cfg, const earthee::AppConfig& app)
            : HttpJsonSource(cfg, app) {}

        protected:
            virtual bool parsePoints(const picojson::value& root, std::vector<DataPoint>& pts)
            {
                if (!root.is<picojson::object>()) return false;
                const picojson::value& feats = root.get("features");
                if (!feats.is<picojson::array>()) return false;
                const picojson::array& arr = feats.get<picojson::array>();
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    const picojson::value& f = arr[i];
                    if (!f.is<picojson::object>()) continue;
                    const picojson::value& geom = f.get("geometry");
                    const picojson::value& props = f.get("properties");
                    if (!geom.is<picojson::object>() || !props.is<picojson::object>()) continue;
                    if (strField(props, "Class") != "Point_Centroid") continue;  // deduplicate

                    const picojson::value& coords = geom.get("coordinates");
                    if (!coords.is<picojson::array>()) continue;
                    const picojson::array& c = coords.get<picojson::array>();
                    if (c.size() < 2 || !c[0].is<double>() || !c[1].is<double>()) continue;

                    std::string lvl = strField(props, "alertlevel");
                    osg::Vec4 color; float sizePx;
                    if (lvl == "Red") { color = osg::Vec4(1.0f, 0.25f, 0.2f, 1.0f); sizePx = 16.0f; }
                    else if (lvl == "Orange") { color = osg::Vec4(1.0f, 0.65f, 0.15f, 1.0f); sizePx = 12.0f; }
                    else continue;  // Green / unknown level: do not show

                    std::string etype = strField(props, "eventtype");
                    std::string country = strField(props, "country");
                    std::string fromdate = strField(props, "fromdate");
                    std::string sevText;
                    const picojson::value& sev = props.get("severitydata");
                    if (sev.is<picojson::object>()) sevText = strField(sev, "severitytext");

                    DataPoint p;
                    p.lon = c[0].get<double>(); p.lat = c[1].get<double>();
                    p.color = color; p.sizePx = sizePx;
                    p.title = std::string(eventTypeText(etype)) + u8" - "
                            + (country.empty() ? u8"Unknown" : country);
                    p.detail = u8"Level: " + lvl + u8"\nTime: " + fromdate;
                    if (!sevText.empty()) p.detail += u8"\nImpact: " + sevText;
                    pts.push_back(p);
                }
                return true;
            }

        private:
            static const char* eventTypeText(const std::string& t)
            {
                if (t == "EQ") return u8"Earthquake"; if (t == "TC") return u8"Tropical Cyclone";
                if (t == "FL") return u8"Flood"; if (t == "VO") return u8"Volcano";
                if (t == "WF") return u8"Wildfire"; if (t == "DR") return u8"Drought";
                return u8"Disaster";
            }
        };

        // ===== USGS earthquakes (M2.5+, last 24 hours) =====
        class UsgsSource : public HttpJsonSource
        {
        public:
            UsgsSource(const earthee::SourceConfig& cfg, const earthee::AppConfig& app)
            : HttpJsonSource(cfg, app) {}

        protected:
            virtual bool parsePoints(const picojson::value& root, std::vector<DataPoint>& pts)
            {
                if (!root.is<picojson::object>()) return false;
                const picojson::value& feats = root.get("features");
                if (!feats.is<picojson::array>()) return false;
                const picojson::array& arr = feats.get<picojson::array>();
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    const picojson::value& f = arr[i];
                    if (!f.is<picojson::object>()) continue;
                    const picojson::value& geom = f.get("geometry");
                    const picojson::value& props = f.get("properties");
                    if (!geom.is<picojson::object>() || !props.is<picojson::object>()) continue;
                    const picojson::value& coords = geom.get("coordinates");
                    if (!coords.is<picojson::array>()) continue;
                    const picojson::array& c = coords.get<picojson::array>();
                    if (c.size() < 3 || !c[0].is<double>() || !c[1].is<double>()) continue;

                    const picojson::value& magV = props.get("mag");
                    double mag = magV.is<double>() ? magV.get<double>() : 0.0;
                    std::string place = strField(props, "place");
                    std::string url = strField(props, "url");
                    double unixMs = 0.0;
                    const picojson::value& timeV = props.get("time");
                    if (timeV.is<double>()) unixMs = timeV.get<double>();
                    double depthKm = c[2].is<double>() ? c[2].get<double>() : 0.0;

                    // magnitude -> size/color (purple ring, higher magnitude = brighter and larger)
                    float magF = (float)osg::clampBetween(mag, 2.5, 9.5);
                    DataPoint p;
                    p.lon = c[0].get<double>(); p.lat = c[1].get<double>();
                    p.sizePx = 6.0f + (magF - 2.5f) * 2.2f;
                    float t = (magF - 2.5f) / 7.0f;
                    p.color = osg::Vec4(0.45f + 0.35f * t, 0.25f + 0.25f * t, 1.0f, 1.0f);
                    char buf[64]; snprintf(buf, sizeof(buf), "M%.1f", mag);
                    p.title = std::string(buf) + u8" - "
                            + (place.empty() ? u8"Unknown" : place);
                    p.detail = u8"Magnitude: " + std::string(buf) + u8"\nDepth: "
                              + std::to_string((int)depthKm) + u8" km\nTime: "
                              + timeText(unixMs / 1000.0);
                    p.url = url;
                    pts.push_back(p);
                }
                return true;
            }
        };

        // ===== NASA EONET (active natural events: volcanoes/wildfires/storms/icebergs etc.) =====
        class EonetSource : public HttpJsonSource
        {
        public:
            EonetSource(const earthee::SourceConfig& cfg, const earthee::AppConfig& app)
            : HttpJsonSource(cfg, app) {}

        protected:
            virtual bool parsePoints(const picojson::value& root, std::vector<DataPoint>& pts)
            {
                if (!root.is<picojson::object>()) return false;
                const picojson::value& events = root.get("events");
                if (!events.is<picojson::array>()) return false;
                const picojson::array& arr = events.get<picojson::array>();
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    const picojson::value& ev = arr[i];
                    if (!ev.is<picojson::object>()) continue;
                    const picojson::value& geom = ev.get("geometry");
                    if (!geom.is<picojson::array>() || geom.get<picojson::array>().empty()) continue;
                    const picojson::value& g0 = geom.get<picojson::array>()[0];
                    if (!g0.is<picojson::object>()) continue;
                    const picojson::value& coords = g0.get("coordinates");
                    if (!coords.is<picojson::array>()) continue;
                    const picojson::array& c = coords.get<picojson::array>();
                    if (c.size() < 2 || !c[0].is<double>() || !c[1].is<double>()) continue;

                    std::string title = strField(ev, "title");
                    std::string link = strField(ev, "link");
                    std::string category;
                    const picojson::value& cats = ev.get("categories");
                    if (cats.is<picojson::array>() && !cats.get<picojson::array>().empty())
                        category = strField(cats.get<picojson::array>()[0], "title");

                    DataPoint p;
                    p.lon = c[0].get<double>(); p.lat = c[1].get<double>();
                    p.sizePx = 10.0f;
                    p.color = osg::Vec4(0.29f, 0.85f, 0.42f, 1.0f);
                    p.title = title;
                    p.detail = u8"Category: " + (category.empty() ? "-" : category);
                    if (!link.empty()) p.detail += u8"\nSource: " + link;
                    p.url = link;
                    pts.push_back(p);
                }
                return true;
            }
        };

        // ===== factory registration =====
        DataSource* createGdacs(const earthee::SourceConfig& c, const earthee::AppConfig& a)
        { return new GdacsSource(c, a); }
        DataSource* createUsgs(const earthee::SourceConfig& c, const earthee::AppConfig& a)
        { return new UsgsSource(c, a); }
        DataSource* createEonet(const earthee::SourceConfig& c, const earthee::AppConfig& a)
        { return new EonetSource(c, a); }
    }

    void registerBuiltinSourceTypes()
    {
        DataSourceFactory& f = DataSourceFactory::instance();
        f.registerType("gdacs", &createGdacs);
        f.registerType("usgs", &createUsgs);
        f.registerType("eonet", &createEonet);
    }
}
