#ifndef MANA_READERWRITER_TILECALLBACK_HPP
#define MANA_READERWRITER_TILECALLBACK_HPP

#include <osg/Image>
#include <osg/Geometry>
#include <osgDB/ReaderWriter>
#include <functional>
#include <set>
#include "Export.h"

typedef std::string (*CreatePathFunc)(int, const std::string&, int, int, int);

namespace osgVerse
{
    class TileCallback;
    struct TileGeometryHandler : public osg::Object
    {
        TileGeometryHandler() {}
        TileGeometryHandler(const TileGeometryHandler& c, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY)
            : osg::Object(c, op) {}
        META_Object(osgVerse, TileGeometryHandler)

        virtual osg::Geometry* create(const TileCallback* cb, const osg::Matrix& outMatrix,
                                      const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                      double width, double height) const { return NULL; }
    };

    class OSGVERSE_RW_EXPORT TileCallback : public osg::NodeCallback
    {
    public:
        TileCallback(bool g = true)
        :   _x(-1), _y(-1), _z(-1), _skirtRatio(0.02f), _elevationScale(1.0f), _withGlobeAttr(g), _flatten(true),
            _bottomLeft(false), _useWebMercator(false), _layersDone(false) { _createPathFunc = NULL; }
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        virtual void computeTileExtent(osg::Vec3d& tileMin, osg::Vec3d& tileMax,
                                       double& tileWidth, double& tileHeight) const;
        virtual double mapAltitude(const osg::Vec4& color, double minH = 0.0, double maxH = 20000.0) const;

        virtual osg::Geometry* createTileGeometry(osg::Matrix& outMatrix, osg::Texture* elevation,
                                                  const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                                  double width, double height) const;
        virtual osg::Geometry* createTileGeometry(osg::Matrix& outMatrix, TileGeometryHandler* handler,
                                                  const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                                  double width, double height) const;
        virtual void updateTileGeometry(osg::Geometry* geometry, osg::Texture* elevation, const std::string& range,
                                        const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                        double width, double height) const;
        virtual void updateSkirtData(osg::Geometry* geometry, double tileRefSize, bool addingTriangles) const;

        enum LayerType { ELEVATION = 0, ORTHOPHOTO, OCEAN_MASK, USER };
        enum LayerState { DONE = 0, DEFERRED, FAILED };
        typedef std::pair<std::string, LayerState> DataPathPair;

        virtual osg::Texture* findAndUseParentData(LayerType id, osg::Group* parent);
        osg::Texture* createLayerImage(LayerType id, bool& emptyPath, const osgDB::Options* opt,
                                       osg::NodeVisitor::ImageRequestHandler* irh = NULL);
        TileGeometryHandler* createLayerHandler(LayerType id, bool& emptyPath, const osgDB::Options* opt);

        /** Set layer data path with wildcards */
        void setLayerPath(LayerType id, const std::string& p) { _layerPaths[id] = DataPathPair(p, DONE); }
        std::string getLayerPath(LayerType id) { return _layerPaths[id].first; }

        /** Set layer data path state */
        void setLayerPathState(LayerType id, LayerState s) { _layerPaths[id].second = s; }
        LayerState getLayerPathState(LayerType id) { return _layerPaths[id].second; }

        /** Set global extent size: default is (-180, -90) to (180, 90) */
        void setTotalExtent(const osg::Vec3d& e0, const osg::Vec3d& e1) { _extentMin = e0; _extentMax = e1; }
        void setMinExtent(const osg::Vec3d& e) { _extentMin = e; }
        void setMaxExtent(const osg::Vec3d& e) { _extentMax = e; }
        const osg::Vec3d& getMinExtent() const { return _extentMin; }
        const osg::Vec3d& getMaxExtent() const { return _extentMax; }

        /** Set tile column (x), tile row (y) and tile level (z) */
        void setTileNumber(int x, int y, int z) { _x = x; _y = y; _z = z; }
        void setTileX(int v) { _x = v; } int getTileX() const { return _x; }
        void setTileY(int v) { _y = v; } int getTileY() const { return _y; }
        void setTileZ(int v) { _z = v; } int getTileZ() const { return _z; }

        /** Set a custom function to construct tile path string */
        void setCreatePathFunction(CreatePathFunc f) { _createPathFunc = f; }
        CreatePathFunc getCreatePathFunction() const { return _createPathFunc; }

        /** Set tile skirt length ratio */
        void setSkirtRatio(float s) { _skirtRatio = s; }
        float getSkirtRatio() const { return _skirtRatio; }

        /** Set evevation scale */
        void setElevationScale(float s) { _elevationScale = s; }
        float getElevationScale() const { return _elevationScale; }

        /** Set if the tile is flatten 2D or earth 3D */
        void setFlatten(bool b) { _flatten = b; }
        bool getFlatten() const { return _flatten; }

        /** Set image origin (bottom-left or top-left) */
        void setBottomLeft(bool b) { _bottomLeft = b; }
        bool getBottomLeft() const { return _bottomLeft; }

        /** Set use web-mercator (square tile) or not (2:1 rectangle tile) */
        void setUseWebMercator(bool b) { _useWebMercator = b; }
        bool getUseWebMercator() const { return _useWebMercator; }

        /** Set if all layers are loaded or not */
        void setLayersDone(bool b) { _layersDone = b; }
        bool getLayersDone() const { return _layersDone; }

        osg::Vec3d adjustLatitudeLongitudeAltitude(const osg::Vec3d& extent, bool useSphericalMercator) const;
        const osg::Matrix& getTileWorldToLocalMatrix() const { return _worldToLocal; }

        static std::string createPath(const std::string& pseudoPath, int x, int y, int z);
        static std::string replace(std::string& src, const std::string& match, const std::string& v, bool& c);
        static std::pair<osg::Geometry*, TileCallback*> findParentTile(osg::Group* parentLOD);

    protected:
        virtual ~TileCallback() {}
        virtual bool updateLayerData(osg::NodeVisitor* nv, osg::Node* node, LayerType id);

        std::map<int, DataPathPair> _layerPaths;
        std::map<std::string, osg::Vec4> _uvRangesToSet;
        std::map<std::string, osg::ref_ptr<osg::Referenced>> _imageRequests;
        osg::ref_ptr<osg::Texture> _elevationRef;
        osg::Matrix _worldToLocal;
        osg::Vec3d _extentMin, _extentMax;
        CreatePathFunc _createPathFunc;
        int _x, _y, _z; float _skirtRatio, _elevationScale;
        bool _withGlobeAttr, _flatten, _bottomLeft, _useWebMercator, _layersDone;
    };

    class OSGVERSE_RW_EXPORT TileManager : public osg::Referenced
    {
    public:
        static TileManager* instance();
        bool check(const std::map<int, TileCallback::DataPathPair>& paths, std::vector<int>& updated);
        bool isHandlerExtension(const std::string& ext, std::string& suggested) const;

        void setLayerPath(TileCallback::LayerType id, const std::string& p) { _layerPaths[id] = p; }
        std::string getLayerPath(TileCallback::LayerType id) { return _layerPaths[id]; }

        void setTileLoadingOptions(osgDB::Options* op) { _options = op; }
        osgDB::Options* getTileLoadingOptions() { return _options.get(); }

        bool shouldMorph(TileCallback& cb) const;
        void updateTileGeometry(TileCallback& cb, osg::Geometry* geom);
        osgDB::ReaderWriter* getReaderWriter(const std::string& protocol, const std::string& url);

        struct DynamicTileCallback : public osg::Referenced
        {
            virtual bool shouldMorph(TileCallback& cb) const { return false; }
            virtual bool updateEntireTileGeometry(TileCallback& cb, osg::Geometry* geom) = 0;
            virtual osg::Vec3 updateTileVertex(TileCallback& cb, double lat, double lon) = 0;
        };
        void setDynamicCallback(DynamicTileCallback* cb) { _dynamicCallback = cb; }
        DynamicTileCallback* getDynamicCallback() { return _dynamicCallback.get(); }

    protected:
        TileManager();
        virtual ~TileManager() {}

        std::map<int, std::string> _layerPaths;
        std::map<std::string, std::string> _acceptHandlerExts;
        std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>> _cachedReaderWriters;
        osg::ref_ptr<DynamicTileCallback> _dynamicCallback;
        osg::ref_ptr<osgDB::Options> _options;
    };
}

#endif
