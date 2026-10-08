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

// Config-driven "world data source" framework (a trimmed port of the friend's feed_layer):
// - DataSource: fetch thread + main-thread sync rebuild of GL_POINTS + picking + detail data;
// - HttpJsonSource: an intermediate base class for GET (osgVerse::loadFileData, reusing the
//   core library HTTP client) + picojson parsing; concrete sources only implement parsePoints();
// - DataSourceFactory: registry from type name -> constructor; the config file's
//   data_sources[].type instantiates accordingly, so adding a source needs no framework change;
// - DataSourceManager: holds all sources, registers the global pick handler, reports status to
//   LayerRegistry.
namespace earthds
{
    // Register built-in data source types (feed_sources.cpp; called once in main before initialize)
    void registerBuiltinSourceTypes();

    struct DataPoint
    {
        double lat = 0.0, lon = 0.0;   // degrees
        float sizePx = 8.0f;
        osg::Vec4 color = osg::Vec4(1, 1, 1, 1);
        std::string title, detail;     // detail card title/body (pre-formatted multi-line)
        std::string url;               // nullable: link for the detail card "Open" button
    };

    // Currently selected feature across all sources (written by pick handler, read by UI, locked internally)
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

        // Threading contract: enabled is written by the UI (draw thread) and read by the fetch
        // thread -> atomic
        void setEnabled(bool on);
        bool isEnabled() const { return _enabled; }

        // 0=not fetched yet, 1=success with features, 2=failed (see lastError), 3=success but 0 features
        int fetchState() const { return _fetchState; }
        std::string lastError() const;
        size_t pointCount() const;

        // Copy the first maxN feature titles (used as samples by the AI assistant's
        // get_data_summary; locked copy)
        void sampleTitles(std::vector<std::string>& out, size_t maxN) const;

        // Root node attached to the scene (contains the fetch thread, joined on node destruction)
        osg::Group* root() { return _root; }
        void pickAt(osg::Camera* cam, float mx, float my);  // main/event thread
        void syncIfDirty();                                 // main thread (update callback)
        void pollFetch();                                   // fetch thread (poll body)

    protected:
        virtual ~DataSource();
        // Called by the fetch thread: fetch + parse. Returns false and fills err on failure.
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
        mutable OpenThreads::Mutex _mutex;  // guards _records/_pending
        mutable OpenThreads::Mutex _errMutex;
        std::string _lastErrorText;
        std::atomic<bool> _enabled;
        std::atomic<int> _fetchState;       // 0/1/2/3
        bool _dirty;
        int _tick;                        // poll tick counter of the fetch thread
        OpenThreads::Thread* _thread;
    };

    // Common base class for HTTP GET + JSON parsing: "{key}" in the URL is replaced by the
    // result of resolveKey
    class HttpJsonSource : public DataSource
    {
    protected:
        HttpJsonSource(const earthee::SourceConfig& cfg, const earthee::AppConfig& app);
        virtual bool fetch(std::vector<DataPoint>& points, std::string& err);
        // Parse the JSON root object into features; returns false = parse failure (data format mismatch)
        virtual bool parsePoints(const picojson::value& root, std::vector<DataPoint>& points) = 0;
    private:
        std::string _resolvedKey;
    };

    // Type registry: data_sources[].type -> Creator. registerBuiltinSourceTypes()
    // registers the built-in sources; extending with a new source = register in a new file, zero intrusion.
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

    // Manager of all data sources (owned by the application, destroyed with the scene)
    class DataSourceManager
    {
    public:
        // Instantiate all sources from the config, register layer entries and the pick handler;
        // returns the root group attached to the scene camera.
        // Any source creation failure is only a console warning + skip, affecting no other source.
        osg::Group* initialize(const earthee::AppConfig& cfg, osgViewer::Viewer& viewer,
                               LayerRegistry& layers);

        // Status carries "n/m": number of sources enabled and successfully fetched in the last
        // round / number of enabled sources
        void health(int& okCount, int& enabledCount) const;
        const std::vector<osg::ref_ptr<DataSource>>& sources() const { return _sources; }
        void clearSelectionIfSource(const std::string& sourceId);

    private:
        std::vector<osg::ref_ptr<DataSource>> _sources;
    };
}

#endif
