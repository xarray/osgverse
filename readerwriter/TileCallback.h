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
        TileCallback()
        :   _x(-1), _y(-1), _z(-1), _skirtRatio(0.02f), _elevationScale(1.0f), _flatten(true),
            _bottomLeft(false), _useWebMercator(false) {}
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        virtual void computeTileExtent(osg::Vec3d& tileMin, osg::Vec3d& tileMax,
                                       double& tileWidth, double& tileHeight) const;
        virtual double mapAltitude(const osg::Vec4& color, double minH = 0.0, double maxH = 20000.0) const;

        virtual osg::Geometry* createTileGeometry(osg::Matrix& outMatrix, osg::Image* elevation,
                                                  const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                                  double width, double height) const;
        virtual osg::Geometry* createTileGeometry(osg::Matrix& outMatrix, TileGeometryHandler* handler,
                                                  const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                                  double width, double height) const;

        enum LayerType { ELEVATION = 0, ORTHOPHOTO, OCEAN_MASK, USER };
        osg::Image* createLayerImage(LayerType id);
        TileGeometryHandler* createLayerHandler(LayerType id);

        /** Set layer data path with wildcards */
        void setLayerPath(LayerType id, const std::string& p) { _layerPaths[id] = p; }
        std::string getLayerPath(LayerType id) { return _layerPaths[id]; }

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

        osg::Vec3d adjustLatitudeLongitudeAltitude(const osg::Vec3d& extent, bool useSphericalMercator) const;
        static std::string createPath(const std::string& pseudoPath, int x, int y, int z);
        static std::string replace(std::string& src, const std::string& match, const std::string& v, bool& c);

    protected:
        virtual ~TileCallback() {}
        virtual void updateLayerData(osg::Node* node, LayerType id);

        std::map<int, std::string> _layerPaths;
        osg::Vec3d _extentMin, _extentMax;
        CreatePathFunc _createPathFunc;
        int _x, _y, _z; float _skirtRatio, _elevationScale;
        bool _flatten, _bottomLeft, _useWebMercator;
    };

    class OSGVERSE_RW_EXPORT TileManager : public osg::Referenced
    {
    public:
        static TileManager* instance();
        bool check(const std::map<int, std::string>& paths, std::vector<int>& updated);
        bool isHandlerExtension(const std::string& ext, std::string& suggested) const;

        void setLayerPath(TileCallback::LayerType id, const std::string& p) { _layerPaths[id] = p; }
        std::string getLayerPath(TileCallback::LayerType id) { return _layerPaths[id]; }

    protected:
        TileManager();
        virtual ~TileManager() {}

        std::map<int, std::string> _layerPaths;
        std::map<std::string, std::string> _acceptHandlerExts;
    };
}

#endif
