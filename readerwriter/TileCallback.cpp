#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgUtil/SmoothingVisitor>

#include <modeling/Math.h>
#include <pipeline/Utilities.h>
#include "Utilities.h"
#include "TileCallback.h"
using namespace osgVerse;

class FindTileGeometry : public osg::NodeVisitor
{
public:
    FindTileGeometry() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN) {}
    osg::observer_ptr<osg::Geometry> geometry;

    virtual void apply(osg::Geode& node)
    {
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (geom) apply(*geom);
        }
        traverse(node);
    }

    virtual void apply(osg::Geometry& geom)
    { geometry = &geom; traverse(geom); }
};

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

osg::Texture* TileCallback::createLayerImage(LayerType id, bool& emptyPath)
{
    std::string inputAddr = _layerPaths[(int)id];
    emptyPath = (inputAddr.empty()); if (emptyPath) return NULL;

    std::string url = _createPathFunc ? _createPathFunc((int)id, inputAddr, _x, _y, _z)
                    : TileCallback::createPath(inputAddr, _x, _y, _z);
    std::string protocol = osgDB::getServerProtocol(url);
    if (protocol.find("http") != std::string::npos) url += ".verse_web";  // FIXME: only http?

    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(url);
    return !image ? NULL : createTexture2D(image.get(), osg::Texture::CLAMP_TO_EDGE);
}

TileGeometryHandler* TileCallback::createLayerHandler(LayerType id, bool& emptyPath)
{
    std::string inputAddr = _layerPaths[(int)id];
    emptyPath = (inputAddr.empty()); if (emptyPath) return NULL;

    std::string url = _createPathFunc ? _createPathFunc((int)id, inputAddr, _x, _y, _z)
                    : TileCallback::createPath(inputAddr, _x, _y, _z);
    std::string protocol = osgDB::getServerProtocol(url);
    if (protocol.find("http") != std::string::npos) url += ".verse_web";  // FIXME: only http?
    return dynamic_cast<TileGeometryHandler*>(osgDB::readObjectFile(url));
}

double TileCallback::mapAltitude(const osg::Vec4& color, double minH, double maxH) const
{
    double value = minH + (double)color[0] * (maxH - minH);
    return _flatten ? (value * 0.0002) : value;
}

osg::Vec3d TileCallback::adjustLatitudeLongitudeAltitude(const osg::Vec3d& extent, bool useSphericalMercator) const
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

osg::Geometry* TileCallback::createTileGeometry(osg::Matrix& outMatrix, TileGeometryHandler* handler,
                                                const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                                double width, double height) const
{
    if (!_flatten)
    {
        osg::Vec3d center = adjustLatitudeLongitudeAltitude((tileMin + tileMax) * 0.5, _useWebMercator);
        osg::Matrix localToWorld = Coordinate::convertLLAtoENU(center);
        outMatrix = localToWorld;
    }
    return handler ? handler->create(this, outMatrix, tileMin, tileMax, width, height) : NULL;
}

osg::Geometry* TileCallback::createTileGeometry(osg::Matrix& outMatrix, osg::Texture* elevationTex,
                                                const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                                double width, double height) const
{
    osg::Image* elevation = (elevationTex ? elevationTex->getImage(0) : NULL);
    double tileRefSize = osg::inDegrees(tileMax.y() - tileMin.y()) * osg::WGS_84_RADIUS_EQUATOR;
    bool useRealElevation = elevation ? (elevation->getDataType() == GL_FLOAT) : false;
    if (!_flatten)
    {
        osg::Vec3d center = adjustLatitudeLongitudeAltitude((tileMin + tileMax) * 0.5, _useWebMercator);
        osg::Matrix localToWorld = Coordinate::convertLLAtoENU(center);
        osg::Matrix worldToLocal = osg::Matrix::inverse(localToWorld);
        osg::Matrix normalMatrix(localToWorld(0, 0), localToWorld(0, 1), localToWorld(0, 2), 0.0,
                                 localToWorld(1, 0), localToWorld(1, 1), localToWorld(1, 2), 0.0,
                                 localToWorld(2, 0), localToWorld(2, 1), localToWorld(2, 2), 0.0,
                                 0.0, 0.0, 0.0, 1.0);
        unsigned int numRows = 16, numCols = 16;
        unsigned int numVertices = numCols * numRows;
        if (_skirtRatio > 0.0f) numVertices += 2 * (numCols + numRows);
        outMatrix = localToWorld;

        // FIXME: support compute elevation in shaders?
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(numVertices);
        osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(numVertices);
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(numVertices);
        osg::ref_ptr<osg::Vec2Array> ca = new osg::Vec2Array(numVertices);
        double invW = width / (float)(numCols - 1), invH = height / (float)(numRows - 1), lastAlt = 0.0;
        for (unsigned int y = 0; y < numRows; ++y)
            for (unsigned int x = 0; x < numCols; ++x)
            {
                unsigned int vi = x + y * numCols; double altitude = 0.0;
                osg::Vec2 uv((double)x * invW / width, (double)y * invH / height);
                if (elevation)
                {
                    osg::Vec4 elevColor = elevation->getColor(uv);  // FIXME: scale?...
                    if (elevColor[0] > 10e6 || elevColor[0] < -10e6) { altitude = lastAlt; }
                    else altitude = (useRealElevation ? elevColor[0] : mapAltitude(elevColor)) * _elevationScale;
                }

                osg::Vec3d lla = adjustLatitudeLongitudeAltitude(
                    tileMin + osg::Vec3d((double)x * invW, (double)y * invH, altitude), _useWebMercator);
                osg::Vec3d ecef = Coordinate::convertLLAtoECEF(lla);
                (*va)[vi] = osg::Vec3(ecef * worldToLocal); (*ta)[vi] = osg::Vec2(uv[0], uv[1]);
                (*na)[vi] = osg::Vec3(normalMatrix.postMult(ecef)); (*na)[vi].normalize();

                // For ocean plane, save distance to earth center when ALTITUDE = 0
                lla = adjustLatitudeLongitudeAltitude(
                    tileMin + osg::Vec3d((double)x * invW, (double)y * invH, 0.0), _useWebMercator);
                osg::Vec3 v0 = Coordinate::convertLLAtoECEF(lla); lastAlt = altitude;
                (*ca)[vi] = osg::Vec2(v0.length() - osg::WGS_84_RADIUS_EQUATOR, 0.0f);
            }
#if false
        for (unsigned int y = 1; y < numRows - 1; ++y)
            for (unsigned int x = 1; x < numCols - 1; ++x)
            {   // Recompute non-boundary normals
                unsigned int vi = x + y * numCols; const osg::Vec3& v0 = (*va)[vi];
                unsigned int vx0 = (x > 0 ? (x - 1) : x) + y * numCols;
                unsigned int vx1 = (x < numCols - 1 ? (x + 1) : x) + y * numCols;
                unsigned int vy0 = x + (y > 0 ? (y - 1) : y) * numCols;
                unsigned int vy1 = x + (y < numCols - 1 ? (y + 1) : y) * numCols;

                osg::Plane p0(v0, (*va)[vx0], (*va)[vy0]), p1(v0, (*va)[vx1], (*va)[vy1]);
                osg::Plane p2(v0, (*va)[vy0], (*va)[vx1]), p3(v0, (*va)[vy1], (*va)[vx0]);
                osg::Vec3 N = p0.getNormal() + p1.getNormal() + p2.getNormal() + p3.getNormal(); N.normalize();
                (*na)[vi] = (*na)[vi] * 0.7f + N * 0.3f; (*na)[vi].normalize();
            }
#endif
        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
        for (unsigned int y = 0; y < numRows - 1; ++y)
            for (unsigned int x = 0; x < numCols - 1; ++x)
            {
                unsigned int vi = x + y * numCols;
                de->push_back(vi); de->push_back(vi + 1); de->push_back(vi + numCols);
                de->push_back(vi + numCols); de->push_back(vi + 1); de->push_back(vi + numCols + 1);
            }

        if (_skirtRatio > 0.0f)
        {
            float skirtHeight = (float)fabs(tileRefSize * _skirtRatio);
            unsigned int vi = numRows * numCols;
            // row[0]
            unsigned int tile_bottom_row = 0, skirt_bottom_row = vi;
            for (unsigned int c = 0; c < numCols; ++c, ++vi)
            {
                unsigned int si = tile_bottom_row + c; osg::Vec3 N = na->at(si); N.normalize();
                va->at(vi) = va->at(si) - N * skirtHeight; na->at(vi) = N; ta->at(vi) = ta->at(si);
                ca->at(vi) = ca->at(si); ca->at(vi).y() = -1.0f;
            }
            for (unsigned int c = 0; c < numCols - 1; ++c)
            {
                unsigned int tile_i = tile_bottom_row + c, skirt_i = skirt_bottom_row + c;
                de->push_back(tile_i); de->push_back(skirt_i); de->push_back(skirt_i + 1);
                de->push_back(skirt_i + 1); de->push_back(tile_i + 1); de->push_back(tile_i);
            }
            // row[numRows-1]
            unsigned int tile_top_row = (numRows - 1) * numCols, base_top_row = vi;
            for (unsigned int c = 0; c < numCols; ++c, ++vi)
            {
                unsigned int si = tile_top_row + c; osg::Vec3 N = na->at(si); N.normalize();
                va->at(vi) = va->at(si) - N * skirtHeight; na->at(vi) = N; ta->at(vi) = ta->at(si);
                ca->at(vi) = ca->at(si); ca->at(vi).y() = -1.0f;
            }
            for (unsigned int c = 0; c < numCols - 1; ++c)
            {
                unsigned int tile_i = tile_top_row + c, skirt_i = base_top_row + c;
                de->push_back(tile_i); de->push_back(skirt_i + 1); de->push_back(skirt_i);
                de->push_back(skirt_i + 1); de->push_back(tile_i); de->push_back(tile_i + 1);
            }
            // column[0]
            unsigned int tile_left_column = 0, skirt_left_column = vi;
            for (unsigned int r = 0; r < numRows; ++r, ++vi)
            {
                unsigned int si = tile_left_column + r * numCols; osg::Vec3 N = na->at(si); N.normalize();
                va->at(vi) = va->at(si) - N * skirtHeight; na->at(vi) = N; ta->at(vi) = ta->at(si);
                ca->at(vi) = ca->at(si); ca->at(vi).y() = -1.0f;
            }
            for (unsigned int r = 0; r < numRows - 1; ++r)
            {
                unsigned int tile_i = tile_left_column + r * numCols, skirt_i = skirt_left_column + r;
                de->push_back(tile_i); de->push_back(skirt_i + 1); de->push_back(skirt_i);
                de->push_back(skirt_i + 1); de->push_back(tile_i); de->push_back(tile_i + numCols);
            }
            // column[numColums-1]
            unsigned int tile_right_column = numCols - 1, skirt_right_column = vi;
            for (unsigned int r = 0; r < numRows; ++r, ++vi)
            {
                unsigned int si = tile_right_column + r * numCols; osg::Vec3 N = na->at(si); N.normalize();
                va->at(vi) = va->at(si) - N * skirtHeight; na->at(vi) = N; ta->at(vi) = ta->at(si);
                ca->at(vi) = ca->at(si); ca->at(vi).y() = -1.0f;
            }
            for (unsigned int r = 0; r < numRows - 1; ++r)
            {
                unsigned int tile_i = tile_right_column + r * numCols, skirt_i = skirt_right_column + r;
                de->push_back(tile_i); de->push_back(skirt_i); de->push_back(skirt_i + 1);
                de->push_back(skirt_i + 1); de->push_back(tile_i + numCols); de->push_back(tile_i);
            }
        }  // if (_skirtRatio > 0.0f)

        osg::Geometry* geom = new osg::Geometry;
        geom->setVertexArray(va.get()); geom->setTexCoordArray(0, ta.get());
        geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        geom->setVertexAttribArray(1, ca.get()); geom->setVertexAttribNormalize(1, GL_FALSE);
        geom->setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
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

osg::Texture* TileCallback::findAndUseParentData(LayerType id, osg::Group* parent)
{
    // Structure:
    /* PLOD_Z0 - Tile_Z0 - Geom (with maps)
               - File_Z0 - PLOD_Z1_00 - Tile_Z1_00 - Geom (current without maps)
                            (parent)  - File_Z1
                         - PLOD_Z1_10 - Tile_Z1_10
                                      - File_Z1
                         - ...
    */
    osg::observer_ptr<osg::Node> lastLvTileNode;
    osg::observer_ptr<osg::Geometry> lastLvTile;
    if (parent && parent->getNumParents() > 0)
    {
        osg::Group* siblingTileGroup = parent->getParent(0);
        if (siblingTileGroup->getNumParents() > 0)
        {
            osg::Group* lastLvPagingNode = siblingTileGroup->getParent(0);
            FindTileGeometry ftg; lastLvTileNode = lastLvPagingNode->getChild(0);
            lastLvTileNode->accept(ftg); lastLvTile = ftg.geometry;
        }
    }

    if (lastLvTileNode.valid() && lastLvTile.valid())
    {
        TileCallback* lastLvCallback = dynamic_cast<TileCallback*>(lastLvTileNode->getUpdateCallback());
        osg::StateSet* ss = lastLvTile->getOrCreateStateSet();
        if (lastLvCallback)
        {
            // Set UV scale value
            // FIXME: should assume _z + 1 = parentZ... And not suitable for no-shader cases
            std::string uvRangeName = "UvOffset" + std::to_string((int)id);
            osg::Uniform* lastUvUniform = ss->getUniform(uvRangeName);
            osg::Vec4 uvRange((float)(_x - lastLvCallback->getTileX() * 2) * 0.5f,
                              (float)(_y - lastLvCallback->getTileY() * 2) * 0.5f, 0.5f, 0.5f);
            if (lastUvUniform != NULL)
            {
                osg::Vec4 lastRange; lastUvUniform->get(lastRange);
                uvRange.set(uvRange[0] * lastRange[2] + lastRange[0],
                            uvRange[1] * lastRange[3] + lastRange[1],
                            uvRange[2] * lastRange[2], uvRange[3] * lastRange[3]);
            }
            _uvRangesToSet[uvRangeName] = uvRange;

            // Share parent tile's texture data
            int texUnit = -1;
            switch (id)
            {
            case ORTHOPHOTO: texUnit = 0; break;
            case OCEAN_MASK: texUnit = 1; break;
            default: texUnit = 2; break;
            }
            if (texUnit < 0) return NULL;  // TODO: elevation?
            return static_cast<osg::Texture*>(ss->getTextureAttribute(texUnit, osg::StateAttribute::TEXTURE));
        }
    }
    return NULL;
}

bool TileCallback::updateLayerData(osg::NodeVisitor* nv, osg::Node* node, LayerType id)
{
    FindTileGeometry ftg; node->accept(ftg);
    if (!ftg.geometry) return false;

    osg::ref_ptr<osg::Texture> tex; osg::StateSet* ss = ftg.geometry->getOrCreateStateSet();
    int texUnit = -1; bool emptyPath = false;
    switch (id)
    {
    case ELEVATION:
        OSG_NOTICE << "[TileCallback] Elevation layer reloading not implemented at present" << std::endl;
        break;  // FIXME: alter elevation data on the fly?
    case ORTHOPHOTO:
        tex = createLayerImage(id, emptyPath); texUnit = 0; break;
    case OCEAN_MASK:
        tex = createLayerImage(id, emptyPath); texUnit = 1; break;
    default:  // USER
        tex = createLayerImage(id, emptyPath); texUnit = 2; break;
    }

    if (texUnit < 0) return false;
    if (!tex && !emptyPath && node->getNumParents() > 0)
        tex = findAndUseParentData(id, node->getParent(0));
    if (tex.valid())
    {
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
        ss->setTextureAttribute(texUnit, tex.get());
#else
        ss->setTextureAttributeAndModes(texUnit, tex.get());
#endif

        std::string uvRangeName = "UvOffset" + std::to_string((int)id);
        if (!ss->getUniform(uvRangeName))
            ss->getOrCreateUniform(uvRangeName, osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        return true;  // FIXME: for USER layers, use tex2d array instead?
    }
    else if (emptyPath)
    {
        tex = static_cast<osg::Texture*>(ss->getTextureAttribute(texUnit, osg::StateAttribute::TEXTURE));
        if (tex) ss->removeTextureMode(texUnit, tex->getTextureTarget());
        if (tex) ss->removeTextureAttribute(texUnit, tex); return true;
    }
    return false;
}

void TileCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    std::vector<int> updatedID; std::string lastPath;
    if (TileManager::instance()->check(_layerPaths, updatedID))
    {
        for (size_t i = 0; i < updatedID.size(); ++i)
        {
            // load or update layer image, but undo this operation if failed
            LayerType id = (LayerType)updatedID[i]; lastPath = _layerPaths[id];
            _layerPaths[id] = TileManager::instance()->getLayerPath(id);
            if (!updateLayerData(nv, node, id)) _layerPaths[id] = lastPath;
        }
    }

    if (!_uvRangesToSet.empty())
    {
        FindTileGeometry ftg; node->accept(ftg);
        osg::StateSet* ss = ftg.geometry.valid() ? ftg.geometry->getOrCreateStateSet() : NULL;
        if (ss)
        {
            for (std::map<std::string, osg::Vec4>::iterator it = _uvRangesToSet.begin();
                 it != _uvRangesToSet.end(); ++it)
            {
                osg::Uniform* u = ss->getUniform(it->first); if (u) u->set(it->second);
                if (!u) ss->getOrCreateUniform(it->first, osg::Uniform::FLOAT_VEC4)->set(it->second);
            }
        }
        _uvRangesToSet.clear();
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
    _acceptHandlerExts[".terrain"] = ".terrain.verse_terrain";
    _acceptHandlerExts[".verse_terrain"] = "";
}

bool TileManager::check(const std::map<int, std::string>& paths, std::vector<int>& updated)
{
    for (std::map<int, std::string>::iterator it = _layerPaths.begin();
         it != _layerPaths.end(); ++it)
    {
        std::map<int, std::string>::const_iterator it2 = paths.find(it->first);
        if (it2 == paths.end() && !it->second.empty()) updated.push_back(it->first);
        else if (it2->second != it->second) updated.push_back(it->first);
    }
    return !updated.empty();
}

bool TileManager::isHandlerExtension(const std::string& ext, std::string& suggested) const
{
    std::map<std::string, std::string>::const_iterator itr = _acceptHandlerExts.find(ext);
    if (itr != _acceptHandlerExts.end()) { suggested = itr->second; return true; } else return false;
}
