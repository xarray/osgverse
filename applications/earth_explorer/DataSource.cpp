// DataSource framework implementation: fetch thread / main-thread sync rebuild of GL_POINTS /
// point sprite shaders / picking.
// The shader and picking math are ported from the friend's feed_layer.cpp (battle-tested), while
// the HTTP layer reuses the core library osgVerse::loadFileData (connection reuse + 8/15s timeout)
// instead of depending on libhv directly.
#include "DataSource.h"
#include <osg/Geometry>
#include <osg/Point>
#include <osg/NodeCallback>
#include <osg/Timer>
#include <osgDB/FileUtils>
#include <modeling/Math.h>
#include <pipeline/Pipeline.h>
#include <readerwriter/Utilities.h>
#include <ui/ImGuiComponents.h>   // ImGui:: namespace (WantCaptureMouse check)
#include <algorithm>
#include <cmath>
#include <iostream>

namespace earthds
{
    namespace
    {
        const float kDragThreshPx2 = 25.0f;     // square of the 5px drag threshold
        const float kPickExtraRadiusPx = 10.0f; // pick tolerance: pixel margin outside the point radius

        // Point sprite vertex shader: color/size come from DataPoint; front hemisphere culling
        // (replacing the depth test) — the point vertex is an ECEF world coordinate, and after
        // transforming to camera space the earth center Cv=MV*origin, camera at origin, the
        // visibility criterion (eye·P>|P|²) is equivalent to dot(Pv-Cv, Pv)<0 in camera space.
        // Back-facing points are moved out of clip space, and with the depth test disabled this
        // eliminates z-fight flicker from insufficient depth precision at far viewpoints.
        const char* feedVertCode = {
            "VERSE_VS_OUT vec4 pointColor;\n"
            "VERSE_VS_OUT float vShapeId;\n"
            "void main() {\n"
            "    pointColor = osg_Color;\n"
            "    gl_PointSize = osg_MultiTexCoord0.x;\n"
            "    vShapeId = osg_MultiTexCoord0.y;\n"
            "    vec4 Pv = VERSE_MATRIX_MV * osg_Vertex;\n"
            "    vec3 Cv = (VERSE_MATRIX_MV * vec4(0.0, 0.0, 0.0, 1.0)).xyz;\n"
            "    if (dot(Pv.xyz - Cv, Pv.xyz) >= 0.0) { gl_Position = vec4(2.0, 2.0, 2.0, 1.0); return; }\n"
            "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n"
            "}\n"
        };

        // Fragment entry: the shape drawing lives in the shared earthmark::markerShapeGLSL()
        // library (concatenated in buildScene)
        const char* feedFragMain = {
            "VERSE_FS_IN vec4 pointColor;\n"
            "VERSE_FS_IN float vShapeId;\n"
            "#ifdef VERSE_GLES3\n"
            "layout(location = 0) VERSE_FS_OUT vec4 fragColor;\n"
            "layout(location = 1) VERSE_FS_OUT vec4 fragOrigin;\n"
            "#endif\n"
            "void main() {\n"
            "    float cov = markerCoverage(int(vShapeId + 0.5), gl_PointCoord, 0.0);\n"
            "    if (cov <= 0.0) discard;\n"
            "    vec3 rgb = mix(pointColor.rgb * 0.7, pointColor.rgb, cov);\n"
            "#ifdef VERSE_GLES3\n"
            "    fragColor = vec4(rgb, 1.0); fragOrigin = vec4(1.0);\n"
            "#else\n"
            "    gl_FragData[0] = vec4(rgb, 1.0); gl_FragData[1] = vec4(1.0);\n"
            "#endif\n"
            "}\n"
        };

#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif

        // Global selection state (written only by the main/event thread, read by the UI draw
        // thread, with a lock)
        OpenThreads::Mutex& globalSelMutex() { static OpenThreads::Mutex m; return m; }
        Selection& globalSelStorage() { static Selection s; return s; }

        // HTTP GET (with connection-layer retry): a non-empty body means success. loadFileData
        // itself carries an 8s connect / 15s total timeout and a persistent connection per
        // (thread,host); here we only add "retry on no response".
        bool httpGetWithRetry(const std::string& url, int retries, std::string& body)
        {
            for (int attempt = 0; attempt <= retries; ++attempt)
            {
                try
                {
                    std::string mimeType, encoding;
                    std::vector<unsigned char> data = osgVerse::loadFileData(url, mimeType, encoding);
                    if (!data.empty())
                    { body.assign(data.begin(), data.end()); return true; }
                }
                catch (...) {}

                if (attempt < retries)
                    OpenThreads::Thread::microSleep((attempt + 1) * 500 * 1000);
            }
            return false;
        }
    }

    Selection currentSelection()
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lk(globalSelMutex());
        return globalSelStorage();
    }

    void clearSelection()
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lk(globalSelMutex());
        globalSelStorage() = Selection();
    }

    // ===== fetch thread =====
    namespace
    {
        class FetchThread : public OpenThreads::Thread
        {
        public:
            FetchThread(DataSource* o) : _owner(o), _done(false) {}
            virtual int cancel() { _done = true; return OpenThreads::Thread::cancel(); }
            virtual void run()
            {
                const int kTickMs = 100;  // poll tick; when disabled tick resets to zero (fetch immediately when re-enabled)
                while (!_done)
                {
                    _owner->pollFetch();
                    OpenThreads::Thread::microSleep(kTickMs * 1000);
                }
                _done = true;
            }
        protected:
            DataSource* _owner;
            std::atomic<bool> _done;  // read/written by the fetch thread, also written by the main thread (destructor) -> atomic
        };

        class SyncCallback : public osg::NodeCallback
        {
        public:
            SyncCallback(DataSource* o) : _owner(o) {}
            virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
        protected:
            DataSource* _owner;
        };
    }

    // ===== DataSource =====
    DataSource::DataSource(const earthee::SourceConfig& cfg)
    :   _cfg(cfg), _shapeId((int)earthmark::visualForLayer(cfg.id).shape), _root(NULL),
        _enabled(false), _fetchState(0), _dirty(false), _tick(0), _thread(NULL)
    {
        // Point sprite shader + StateSet: depth test off (renderBin 11 ensures drawing after the surface)
        osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, feedVertCode);
        std::string fsSrc = std::string(earthmark::markerShapeGLSL()) + feedFragMain;
        osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, fsSrc);
        vs->setName(("Feed_" + cfg.id + "_VS").c_str());
        fs->setName(("Feed_" + cfg.id + "_FS").c_str());
        osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
        osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);
        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs);
        _ss = new osg::StateSet;
        _ss->setAttributeAndModes(prog.get(), osg::StateAttribute::ON);
        _ss->setMode(GL_PROGRAM_POINT_SIZE, osg::StateAttribute::ON);
        _ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        _ss->setRenderBinDetails(11, "RenderBin");

        _root = new osg::Group;
        _root->setNodeMask(0);  // disabled by default
        _root->setName("DataSource_" + cfg.id);
        _root->addUpdateCallback(new SyncCallback(this));
    }

    DataSource::~DataSource()
    {
        stopFetch();
    }

    void DataSource::setEnabled(bool on)
    {
        _enabled = on;
        if (_root) _root->setNodeMask(on ? ~0u : 0u);
        if (on) startFetch();
        if (!on)
        {   // when disabling the layer, clear the selection detail card whose "source == this source"
            OpenThreads::ScopedLock<OpenThreads::Mutex> lk(globalSelMutex());
            if (globalSelStorage().valid && globalSelStorage().sourceId == _cfg.id)
                globalSelStorage() = Selection();
        }
    }

    void DataSource::startFetch()
    {
        if (_thread) return;
        _thread = new FetchThread(this);
        _thread->startThread();
    }

    void DataSource::stopFetch()
    {
        if (_thread)
        { _thread->cancel(); _thread->join(); delete _thread; _thread = NULL; }
    }

    std::string DataSource::lastError() const
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lk(_errMutex);
        return _lastErrorText;
    }

    size_t DataSource::pointCount() const
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lk(_mutex);
        return _records.size();
    }

    void DataSource::postSnapshot(Snapshot snap)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lk(_mutex);
        _pending = std::move(snap); _dirty = true;
    }

    void DataSource::pollFetch()
    {
        // Fetch thread poll body: one tick every 100ms, fetch once when the refresh interval is
        // reached and enabled
        const int refresh = (_cfg.refreshSeconds > 0) ? _cfg.refreshSeconds : 600;
        const int intervalTicks = (refresh * 1000) / 100;
        if (_enabled)
        {
            if (_tick <= 0)
            {
                std::vector<DataPoint> points; std::string err;
                bool ok = false;
                try { ok = fetch(points, err); }
                catch (...) { ok = false; err = u8"Fetch exception"; }

                Snapshot snap;
                if (ok)
                {
                    snap.records.reserve(points.size());
                    for (size_t i = 0; i < points.size(); ++i)
                    {
                        Record r; r.pt = points[i];
                        r.ecef = osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3d(
                            osg::DegreesToRadians(points[i].lat),
                            osg::DegreesToRadians(points[i].lon), _cfg.liftMeters));
                        snap.records.push_back(r);
                    }
                }
                postSnapshot(std::move(snap));
                _fetchState = ok ? (points.empty() ? 3 : 1) : 2;  // 0/1/2/3 see header
                if (!ok)
                {
                    OpenThreads::ScopedLock<OpenThreads::Mutex> lk(_errMutex);
                    _lastErrorText = err.empty() ? u8"Fetch failed (reason unknown)" : err;
                    std::cout << "[DataSource] " << _cfg.id << " fetch failed: "
                              << _lastErrorText << "\n";
                }
                else
                    std::cout << "[DataSource] " << _cfg.id << " loaded N="
                              << points.size() << "\n";
                _tick = intervalTicks;
            }
            else _tick--;
        }
        else _tick = 0;
    }

    void DataSource::syncIfDirty()
    {
        Snapshot snap;
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lk(_mutex);
            if (!_dirty) return;
            std::swap(snap, _pending); _dirty = false;
            _records = std::move(snap.records);
        }
        if (_geode.valid()) _root->removeChild(_geode.get());
        _geode = buildGeodeFrom(_records);
        _root->addChild(_geode.get());
    }

    osg::Geode* DataSource::buildGeodeFrom(const std::vector<Record>& recs)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        osg::ref_ptr<osg::Vec2Array> sizes = new osg::Vec2Array;
        for (size_t i = 0; i < recs.size(); ++i)
        {
            verts->push_back(osg::Vec3(recs[i].ecef));
            colors->push_back(recs[i].pt.color);
            sizes->push_back(osg::Vec2(recs[i].pt.sizePx, (float)_shapeId));
        }
        geom->setVertexArray(verts.get());
        geom->setColorArray(colors.get(), osg::Array::BIND_PER_VERTEX);
        geom->setTexCoordArray(0, sizes.get());
        geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, (GLsizei)verts->size()));
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        geom->setCullingActive(false);

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geom.get());
        geode->setStateSet(_ss.get());
        return geode.release();
    }

    void DataSource::sampleTitles(std::vector<std::string>& out, size_t maxN) const
    {
        out.clear();
        OpenThreads::ScopedLock<OpenThreads::Mutex> lk(_mutex);
        for (size_t i = 0; i < _records.size() && out.size() < maxN; ++i)
        {
            if (!_records[i].pt.title.empty())
                out.push_back(_records[i].pt.title);
        }
    }

    void DataSource::pickAt(osg::Camera* cam, float mx, float my)
    {
        std::vector<Record> recs;
        { OpenThreads::ScopedLock<OpenThreads::Mutex> lk(_mutex); recs = _records; }
        if (!_enabled || recs.empty() || !cam->getViewport()) return;
        osg::Vec3d eye, center, up; cam->getViewMatrixAsLookAt(eye, center, up);
        osg::Matrixd VPW = cam->getViewMatrix() * cam->getProjectionMatrix()
                         * cam->getViewport()->computeWindowMatrix();
        double bestD2 = 1e18; int best = -1;
        for (size_t i = 0; i < recs.size(); ++i)
        {
            const osg::Vec3d& P = recs[i].ecef;
            if ((eye * P) <= (P * P)) continue;   // front hemisphere culling
            osg::Vec3d win = P * VPW;
            double d2 = (win.x() - mx) * (win.x() - mx) + (win.y() - my) * (win.y() - my);
            float tol = recs[i].pt.sizePx * 0.5f + kPickExtraRadiusPx;
            if (d2 < (double)(tol * tol) && d2 < bestD2) { bestD2 = d2; best = (int)i; }
        }
        if (best >= 0)
        {
            Selection s;
            s.valid = true; s.title = recs[best].pt.title; s.detail = recs[best].pt.detail;
            s.url = recs[best].pt.url; s.sourceId = _cfg.id;
            OpenThreads::ScopedLock<OpenThreads::Mutex> lk(globalSelMutex());
            globalSelStorage() = s;
        }
    }

    void SyncCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        _owner->syncIfDirty();
        traverse(node, nv);
    }

    // ===== HttpJsonSource =====
    HttpJsonSource::HttpJsonSource(const earthee::SourceConfig& cfg, const earthee::AppConfig& app)
    :   DataSource(cfg), _resolvedKey(earthee::resolveKey(app, cfg))
    {
    }

    bool HttpJsonSource::fetch(std::vector<DataPoint>& points, std::string& err)
    {
        const earthee::SourceConfig& cfg = config();
        if (cfg.needsKey() && _resolvedKey.empty())
        { err = u8"API Key not configured"; return false; }

        std::string url = cfg.url;
        if (!cfg.needsKey())
        {   // no-key source: still supports the {key} placeholder (empty is fine, should not appear)
            if (url.find("{key}") != std::string::npos) url.erase(url.find("{key}"), 5);
        }
        else
        {
            std::string::size_type pos = 0;
            while ((pos = url.find("{key}", pos)) != std::string::npos)
                url.replace(pos, 5, _resolvedKey);
        }

        std::string body;
        if (!httpGetWithRetry(url, 2, body))
        { err = u8"Network connection failed (no response)"; return false; }

        picojson::value root;
        std::string perr = picojson::parse(root, body);
        if (!perr.empty()) { err = u8"JSON parse failed: " + perr; return false; }
        if (!parsePoints(root, points)) { err = u8"Data format mismatch"; return false; }
        return true;
    }

    // ===== DataSourceManager =====
    namespace
    {
        // Pick handler shared by all sources: a single click iterates over every source (front
        // hemisphere culling + pixel tolerance happen inside each source).
        // It holds a ref_ptr snapshot instead of raw pointers — the manager guarantees source
        // lifetimes, but the handler may still be triggered by the viewer after the manager is
        // destroyed (during shutdown), so ref_ptr prevents dangling.
        class ManagerPickHandler : public osgGA::GUIEventHandler
        {
        public:
            ManagerPickHandler(const std::vector<osg::ref_ptr<DataSource>>& sources)
            : _sources(sources), _pushed(false) {}

            virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
            {
                if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH
                    && ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
                { _downX = ea.getX(); _downY = ea.getY(); _pushed = true; }
                else if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE
                         && ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
                {
                    float dx = ea.getX() - _downX, dy = ea.getY() - _downY;
                    // Do not pick scene features while the mouse is over an ImGui panel (prevents click-through)
                    bool overImGui = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
                    if (_pushed && dx * dx + dy * dy < kDragThreshPx2 && !overImGui)
                    {
                        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
                        osg::Camera* cam = view->getCamera();
                        const osg::Viewport* vp = cam->getViewport();
                        if (vp)
                        {
                            float my = ea.getY();
                            if (ea.getMouseYOrientation() ==
                                osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS)
                                my = vp->height() - my;
                            for (size_t i = 0; i < _sources.size(); ++i)
                                _sources[i]->pickAt(cam, ea.getX(), my);
                        }
                    }
                    _pushed = false;
                }
                return false;  // do not intercept, pass through to the manipulator
            }
        protected:
            virtual ~ManagerPickHandler() {}
            std::vector<osg::ref_ptr<DataSource>> _sources;
            float _downX, _downY; bool _pushed;
        };
    }

    osg::Group* DataSourceManager::initialize(const earthee::AppConfig& cfg,
                                              osgViewer::Viewer& viewer, LayerRegistry& layers)
    {
        osg::ref_ptr<osg::Group> root = new osg::Group;
        root->setName("DataSources");
        DataSourceFactory& factory = DataSourceFactory::instance();

        for (size_t i = 0; i < cfg.sources.size(); ++i)
        {
            const earthee::SourceConfig& sc = cfg.sources[i];
            osg::ref_ptr<DataSource> source(factory.create(sc, cfg));
            if (!source.valid())
            {
                OSG_WARN << "[DataSourceManager] " << u8"Unknown data source type \"" << sc.type
                         << "\" (id=" << sc.id << ")," << u8" skipped\n";
                continue;
            }

            // Layer catalog registration: enabled / missing-key status at a glance
            earthmark::MarkerVisual visual = earthmark::visualForLayer(sc.id);
            OverlayLayer layer;
            layer.id = sc.id; layer.displayName = sc.displayName;
            layer.group = sc.group; layer.subtitle = sc.subtitle;
            layer.type = OverlayLayer::PointFeed;
            layer.needsKey = sc.needsKey() &&
                             earthee::resolveKey(cfg, sc).empty();  // missing key -> grey out
            layer.shape = visual.shape; layer.iconColor = visual.color;
            layer.enabled = sc.enabled && !layer.needsKey;
            layer.fetchStatus = [source](std::string& err) -> int
            {
                int state = source->fetchState();
                if (state == 2) err = source->lastError();
                return state;
            };
            layer.apply = [source](const OverlayLayer& l)
            { source->setEnabled(l.enabled); };
            layers.add(layer);

            if (layer.enabled) source->setEnabled(true);  // enabled on startup
            root->addChild(source->root());
            _sources.push_back(source.get());
        }

        viewer.addEventHandler(new ManagerPickHandler(_sources));
        if (_sources.empty())
            OSG_WARN << "[DataSourceManager] " << u8"No usable data sources (check the config and console warnings),"
                        u8" the data source panel will be entirely unavailable\n";
        return root.release();
    }

    void DataSourceManager::health(int& okCount, int& enabledCount) const
    {
        okCount = 0; enabledCount = 0;
        for (size_t i = 0; i < _sources.size(); ++i)
        {
            if (!_sources[i]->isEnabled()) continue;
            enabledCount++;
            if (_sources[i]->fetchState() == 1 || _sources[i]->fetchState() == 3) okCount++;
        }
    }

    void DataSourceManager::clearSelectionIfSource(const std::string& sourceId)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lk(globalSelMutex());
        if (globalSelStorage().valid && globalSelStorage().sourceId == sourceId)
            globalSelStorage() = Selection();
    }
}
