#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

#include <modeling/Math.h>
#include <pipeline/Utilities.h>
#include "Utilities.h"
#include "TileCallback.h"
using namespace osgVerse;

osg::Vec3d TileCallback::adjustLatitudeLongitudeAltitude(const osg::Vec3d& extent, bool useSphericalMercator)
{
    osg::Vec3d lla(osg::inDegrees(extent[1]), osg::inDegrees(extent[0]), extent[2]);
    if (useSphericalMercator)
    {
        double n = 2.0 * lla.x();
        double adjustedLatitude = atan(0.5 * (exp(n) - exp(-n)));
        return osg::Vec3d(adjustedLatitude, lla.y(), lla.z());
    }
    return lla;
}

std::string TileCallback::createPath(const std::string& pseudoPath, int x, int y, int z)
{
    std::string path = pseudoPath; bool changed = false;
    path = replace(path, "{z}", std::to_string(z), changed);
    path = replace(path, "{x}", std::to_string(x), changed);
    path = replace(path, "{y}", std::to_string(y), changed); return path;
}

std::string TileCallback::replace(std::string& src, const std::string& match, const std::string& v, bool& c)
{
    size_t levelPos = src.find(match); if (levelPos == std::string::npos) { c = false; return src; }
    src.replace(levelPos, match.length(), v); c = true; return src;
}

osg::Image* TileCallback::createLayerImage(LayerType id)
{
    std::string inputAddr = _layerPaths[(int)id]; if (inputAddr.empty()) return NULL;
    std::string url = _createPathFunc ? _createPathFunc((int)id, inputAddr, _x, _y, _z)
                    : TileCallback::createPath(inputAddr, _x, _y, _z);
    if (!osgDB::getServerProtocol(url).empty()) url += ".verse_web";
    return osgDB::readImageFile(url);
}

double TileCallback::mapAltitude(const osg::Vec4& color) const
{
    return color[0] * 8900.0;  // Highest on earth?
}

void TileCallback::computeTileExtent(osg::Vec3d& tileMin, osg::Vec3d& tileMax,
                                     double& tileWidth, double& tileHeight) const
{
    double multiplier = pow(0.5, double(_z));
    tileWidth = multiplier * (_extentMax.x() - _extentMin.x());
    tileHeight = multiplier * (_extentMax.y() - _extentMin.y());
    if (!_bottomLeft)
    {
        osg::Vec3d origin(_extentMin.x(), _extentMax.y(), _extentMin.z());
        tileMin = origin + osg::Vec3d(double(_x) * tileWidth, -double(_y + 1) * tileHeight, 0.0);
        tileMax = origin + osg::Vec3d(double(_x + 1) * tileWidth, -double(_y) * tileHeight, 1.0);
    }
    else
    {
        tileMin = _extentMin + osg::Vec3d(double(_x) * tileWidth, double(_y) * tileHeight, 0.0);
        tileMax = _extentMin + osg::Vec3d(double(_x + 1) * tileWidth, double(_y + 1) * tileHeight, 1.0);
    }
}

osg::Geometry* TileCallback::createTileGeometry(osg::Matrix& outMatrix, osg::Image* elevation,
                                                const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                                double width, double height) const
{
    if (!_flatten)
    {
        osg::Vec3d center = adjustLatitudeLongitudeAltitude((tileMin + tileMax) * 0.5, true);
        osg::Matrix localToWorld = Coordinate::convertLLAtoENU(center);
        osg::Matrix worldToLocal = osg::Matrix::inverse(localToWorld);
        osg::Matrix normalMatrix(localToWorld(0, 0), localToWorld(0, 1), localToWorld(0, 2), 0.0,
                                 localToWorld(1, 0), localToWorld(1, 1), localToWorld(1, 2), 0.0,
                                 localToWorld(2, 0), localToWorld(2, 1), localToWorld(2, 2), 0.0,
                                 0.0, 0.0, 0.0, 1.0);
        unsigned int numRows = 16, numCols = 16;
        outMatrix = localToWorld;

        // FIXME: support compute elevation in shaders?
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(numCols * numRows);
        osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(numCols * numRows);
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(numCols * numRows);
        double invW = width / (float)(numCols - 1), invH = height / (float)(numRows - 1);
        for (unsigned int y = 0; y < numRows; ++y)
            for (unsigned int x = 0; x < numCols; ++x)
            {
                unsigned int vi = x + y * numCols; double altitude = 0.0;
                osg::Vec2 uv((double)x * invW / width, (double)y * invH / height);
                if (elevation) altitude = mapAltitude(elevation->getColor(uv));

                osg::Vec3d lla = adjustLatitudeLongitudeAltitude(
                    tileMin + osg::Vec3d((double)x * invW, (double)y * invH, altitude), true);
                osg::Vec3d ecef = Coordinate::convertLLAtoECEF(lla);
                (*va)[vi] = osg::Vec3(ecef * worldToLocal);
                (*na)[vi] = osg::Vec3(ecef * normalMatrix);
                (*ta)[vi] = osg::Vec2(uv[0], uv[1]);
            }

        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
        for (unsigned int y = 0; y < numRows - 1; ++y)
            for (unsigned int x = 0; x < numCols - 1; ++x)
            {
                unsigned int vi = x + y * numCols;
                de->push_back(vi); de->push_back(vi + 1); de->push_back(vi + numCols);
                de->push_back(vi + numCols); de->push_back(vi + 1); de->push_back(vi + numCols + 1);
            }

        osg::Geometry* geom = new osg::Geometry;
        geom->setVertexArray(va.get()); geom->setTexCoordArray(0, ta.get());
        geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        geom->addPrimitiveSet(de.get()); return geom;
    }
    else if (elevation)
    {
        // FIXME: support altitude on flatten map?
        return osg::createTexturedQuadGeometry(tileMin, osg::X_AXIS * width, osg::Y_AXIS * height);
    }
    else
        return osg::createTexturedQuadGeometry(tileMin, osg::X_AXIS * width, osg::Y_AXIS * height);
}

void TileCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    std::vector<int> updatedID;
    if (TileManager::instance()->check(_layerPaths, updatedID))
    {
        //
    }
    traverse(node, nv);
}

////////////////////////// TileManager //////////////////////////

TileManager* TileManager::instance()
{
    static osg::ref_ptr<TileManager> s_instance = new TileManager;
    return s_instance.get();
}

TileManager::TileManager()
{
}

bool TileManager::check(const std::map<int, std::string>& paths, std::vector<int>& updated)
{
    for (std::map<int, std::string>::iterator it = _layerPaths.begin();
         it != _layerPaths.end(); ++it)
    {
        if (it->second.empty()) continue;
        std::map<int, std::string>::const_iterator it2 = paths.find(it->first);

        if (it2 == paths.end()) updated.push_back(it->first);
        else if (it2->second != it->second) updated.push_back(it->first);
    }
    return !updated.empty();
}
