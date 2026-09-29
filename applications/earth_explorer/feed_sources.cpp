// 内置数据源实现:GDACS 灾害 / USGS 地震 / NASA EONET 自然事件。
// 每个源 = HttpJsonSource 子类(只实现 parsePoints),经 DataSourceFactory 注册——
// 配置文件 data_sources[].type 据此实例化,新增源零侵入。
// 解析逻辑移植自朋友版 feeds/gdacs_feed.cpp 等(真实响应形状已核实)。
#include "DataSource.h"
#include "marker_style.h"
#include <cstdio>
#include <ctime>

namespace earthds
{
    namespace
    {
        // ISO8601(如 "2026-06-29T11:35:33",UTC)→ Unix 秒;失败返回 0
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

        // ===== GDACS(UN 全球灾害预警)=====
        // 响应是 FeatureCollection,但每个事件的多边形/连线等几何各占一条 Feature,
        // 只有 Class == "Point_Centroid" 是代表点;Green(非紧急)级别过滤不显示。
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
                    if (strField(props, "Class") != "Point_Centroid") continue;  // 去重

                    const picojson::value& coords = geom.get("coordinates");
                    if (!coords.is<picojson::array>()) continue;
                    const picojson::array& c = coords.get<picojson::array>();
                    if (c.size() < 2 || !c[0].is<double>() || !c[1].is<double>()) continue;

                    std::string lvl = strField(props, "alertlevel");
                    osg::Vec4 color; float sizePx;
                    if (lvl == "Red") { color = osg::Vec4(1.0f, 0.25f, 0.2f, 1.0f); sizePx = 16.0f; }
                    else if (lvl == "Orange") { color = osg::Vec4(1.0f, 0.65f, 0.15f, 1.0f); sizePx = 12.0f; }
                    else continue;  // Green / 未知级别:不显示

                    std::string etype = strField(props, "eventtype");
                    std::string country = strField(props, "country");
                    std::string fromdate = strField(props, "fromdate");
                    std::string sevText;
                    const picojson::value& sev = props.get("severitydata");
                    if (sev.is<picojson::object>()) sevText = strField(sev, "severitytext");

                    DataPoint p;
                    p.lon = c[0].get<double>(); p.lat = c[1].get<double>();
                    p.color = color; p.sizePx = sizePx;
                    p.title = std::string(eventTypeText(etype)) + u8" \xc2\xb7 "
                            + (country.empty() ? u8"未知" : country);
                    p.detail = u8"等级 Level: " + lvl + u8"\n时间 Time: " + fromdate;
                    if (!sevText.empty()) p.detail += u8"\n影响 Impact: " + sevText;
                    pts.push_back(p);
                }
                return true;
            }

        private:
            static const char* eventTypeText(const std::string& t)
            {
                if (t == "EQ") return u8"地震"; if (t == "TC") return u8"热带气旋";
                if (t == "FL") return u8"洪水"; if (t == "VO") return u8"火山";
                if (t == "WF") return u8"野火"; if (t == "DR") return u8"干旱";
                return u8"灾害";
            }
        };

        // ===== USGS 地震(M2.5+,最近 24 小时)=====
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

                    // 震级 → 大小/颜色(紫环,震级越高越亮越大)
                    float magF = (float)osg::clampBetween(mag, 2.5, 9.5);
                    DataPoint p;
                    p.lon = c[0].get<double>(); p.lat = c[1].get<double>();
                    p.sizePx = 6.0f + (magF - 2.5f) * 2.2f;
                    float t = (magF - 2.5f) / 7.0f;
                    p.color = osg::Vec4(0.45f + 0.35f * t, 0.25f + 0.25f * t, 1.0f, 1.0f);
                    char buf[64]; snprintf(buf, sizeof(buf), "M%.1f", mag);
                    p.title = std::string(buf) + u8" \xc2\xb7 "
                            + (place.empty() ? u8"未知" : place);
                    p.detail = u8"震级 Magnitude: " + std::string(buf) + u8"\n深度 Depth: "
                              + std::to_string((int)depthKm) + u8" km\n时间 Time: "
                              + timeText(unixMs / 1000.0);
                    p.url = url;
                    pts.push_back(p);
                }
                return true;
            }
        };

        // ===== NASA EONET(活跃自然事件:火山/野火/风暴/冰山等)=====
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
                    p.detail = u8"类别 Category: " + (category.empty() ? "-" : category);
                    if (!link.empty()) p.detail += u8"\n来源 Source: " + link;
                    p.url = link;
                    pts.push_back(p);
                }
                return true;
            }
        };

        // ===== 工厂注册 =====
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
