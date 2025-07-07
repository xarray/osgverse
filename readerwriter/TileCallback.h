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
        :   _x(-1), _y(-1), _z(-1), _skirtRatio(0.02f), _flatten(true),
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

        enum LayerType { ELEVATION = 0, ORTHOPHOTO, VECTOR, USER };
        osg::Image* createLayerImage(LayerType id);
        TileGeometryHandler* createLayerHandler(LayerType id);

        void setLayerPath(LayerType id, const std::string& p) { _layerPaths[id] = p; }
        void setTotalExtent(const osg::Vec3d& e0, const osg::Vec3d& e1) { _extentMin = e0; _extentMax = e1; }
        void setCreatePathFunction(CreatePathFunc f) { _createPathFunc = f; }
        void setTileNumber(int x, int y, int z) { _x = x; _y = y; _z = z; }

        void setSkirtRatio(float s) { _skirtRatio = s; }
        void setFlatten(bool b) { _flatten = b; }
        void setBottomLeft(bool b) { _bottomLeft = b; }
        void setUseWebMercator(bool b) { _useWebMercator = b; }

        bool getFlatten() const { return _flatten; }
        bool getBottomLeft() const { return _bottomLeft; }
        bool getUseWebMercator() const { return _useWebMercator; }

        osg::Vec3d adjustLatitudeLongitudeAltitude(const osg::Vec3d& extent, bool useSphericalMercator) const;
        static std::string createPath(const std::string& pseudoPath, int x, int y, int z);
        static std::string replace(std::string& src, const std::string& match, const std::string& v, bool& c);

    protected:
        virtual ~TileCallback() {}

        std::map<int, std::string> _layerPaths;
        osg::Vec3d _extentMin, _extentMax;
        CreatePathFunc _createPathFunc;
        int _x, _y, _z; float _skirtRatio;
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
