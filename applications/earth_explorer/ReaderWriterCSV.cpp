#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>

#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <modeling/GeometryMerger.h>
#include <pipeline/Utilities.h>
#include <pipeline/IntersectionManager.h>

struct VehicleData : public osg::Referenced
{ std::vector<std::pair<osg::Vec3d, osg::Vec2>> dataList; };

class ReaderWriterCSV : public osgDB::ReaderWriter
{
public:
    ReaderWriterCSV()
    {
        supportsExtension("verse_csv", "osgVerse pseudo-loader");
        supportsExtension("csv", "CSV batch data file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] CSV batch data format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        size_t downsamples = 0, new_sz = 0;
        if (options)
        {
            const std::string& dsStr = options->getPluginStringData("Downsamples");
            downsamples = atoi(dsStr.c_str());
        }

        std::map<size_t, std::string> indexMap;
        std::map<std::string, std::string> valueMap;
        osg::ref_ptr<VehicleData> vehicleData = new VehicleData;
        std::string headerLine, line0; unsigned int rowID = 0;
        std::ifstream in(fileName.c_str()); double z = 0.0;
        if (!in) { std::cout << "[ReaderWriterCSV] Failed to load " << fileName << "\n"; return NULL; }

        std::vector<osgVerse::GeometryMerger::GeometryPair> geomList;
        std::vector<osgVerse::GeometryMerger::GeometryPair> lineList;
        std::vector<osg::ref_ptr<osg::Geometry>> geomRefList;
        osg::Node* earth = options ? (osg::Node*)options->getPluginData("EarthRoot") : NULL;
        while (std::getline(in, line0))
        {
            std::string line = trim(line0); rowID++;
            if (line.empty()) continue;
            if (line[0] == '#') continue;

            std::vector<std::string> values, rings;
            osgVerse::StringAuxiliary::split(line, values, ',', false);
            if (!valueMap.empty())
            {
                size_t numColumns = valueMap.size();
                if (numColumns != values.size())
                {
                    std::cout << line << "\n";
                    std::cout << "[ReaderWriterCSV] line " << rowID << " has different values (" << values.size()
                              << ") than " << numColumns << " header columns" << std::endl; continue;
                }

                for (size_t i = 0; i < values.size(); ++i) valueMap[indexMap[i]] = values[i];
                if (valueMap.find("vertices") == valueMap.end()) continue;

                double height = (valueMap.find("Z") == valueMap.end()) ? -1.0 : atof(valueMap["Z"].c_str());
                if (height < 0.0 && valueMap.find("mean_heigh") != valueMap.end()) height = atof(valueMap["mean_heigh"].c_str());
                if (height < 0.0 && valueMap.find("mean_temp") != valueMap.end()) height = atof(valueMap["mean_temp"].c_str());

                double labelCar = (valueMap.find("Label") == valueMap.end()) ? -1.0 : atof(valueMap["Label"].c_str());
                double roadCar = (valueMap.find("on_road") == valueMap.end()) ? -1.0 : atof(valueMap["on_road"].c_str());
                const std::string& vData = valueMap["vertices"]; osg::Vec3d center, ecef, N;
                std::vector<std::vector<osg::Vec3d>> polygons; osgVerse::StringAuxiliary::split(vData, rings, '|', true);
                if (height < 0.0 && labelCar < 0.0 && roadCar < 0.0) std::cout << "Unsupported data? " << headerLine << "\n";

                osg::Matrix localToWorld, worldToLocal;
                for (size_t r = 0; r < rings.size(); ++r)
                {
                    std::vector<osg::Vec3d> polygon; std::vector<std::string> vertices;
                    osgVerse::StringAuxiliary::split(rings[r], vertices, ' ', true);
                    for (size_t j = 0; j < vertices.size(); j += 2)
                    {
                        polygon.push_back(osg::Vec3d(osg::inDegrees(atof(vertices[j + 1].c_str())),
                                                     osg::inDegrees(atof(vertices[j + 0].c_str())), z));
                        if (r == 0) center += polygon.back();
                    }
                    if (r == 0) center *= 1.0 / (double)polygon.size();
                    if (polygon.size() > 2) polygon.push_back(polygon.front());

                    if (r == 0)
                    {
                        ecef = osgVerse::Coordinate::convertLLAtoECEF(center);
                        localToWorld = osgVerse::Coordinate::convertLLAtoENU(center);
                        worldToLocal = osg::Matrix::inverse(localToWorld);
                        N = ecef; N.normalize();

                        if (earth != NULL)
                        {
                            osgVerse::IntersectionResult result =
                                osgVerse::findNearestIntersection(earth, ecef + N * 30000.0, ecef - N * 10000.0);
                            if (result.drawable.valid()) localToWorld.setTrans(result.getWorldIntersectPoint());
                            else std::cout << "No intersection for building: " << line.substr(0, 10) << "\n";
                        }
                        vehicleData->dataList.push_back(std::pair<osg::Vec3d, osg::Vec2>(ecef, osg::Vec2(labelCar, roadCar)));
                    }

                    for (size_t j = 0; j < polygon.size(); ++j)
                    {
                        osg::Vec3d pt = osgVerse::Coordinate::convertLLAtoECEF(polygon[j]);
                        polygon[j] = pt * worldToLocal;
                    }
                    polygons.push_back(polygon);
                }
                if (downsamples > 0) polygons.resize(1);  // LOD rough level uses simple geometry

                osg::Geometry* geom = height < 0.0 ? createLineGeometry(polygons.front(), 20.0f)
                                    : createExtrusionGeometry(polygons, osg::Z_AXIS * osg::clampAbove(height * 5.0, 1.0));
                osg::Vec4Array* ca = new osg::Vec4Array; geom->setColorArray(ca);
                geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
                ca->assign(static_cast<osg::Vec3Array*>(geom->getVertexArray())->size(), heightColor(height, roadCar));

                if (height < 0.0) lineList.push_back(osgVerse::GeometryMerger::GeometryPair(geom, localToWorld));
                else geomList.push_back(osgVerse::GeometryMerger::GeometryPair(geom, localToWorld));
                geomRefList.push_back(geom);
            }
            else
            {
                headerLine = line;
                for (size_t i = 0; i < values.size(); ++i)
                    { indexMap[i] = values[i]; valueMap[values[i]] = ""; }
            }
        }

        osg::MatrixTransform* root = new osg::MatrixTransform;
        if (!geomList.empty())
        {
            osg::Matrix l2w = geomList[0].second; osg::Matrix w2l = osg::Matrix::inverse(l2w);
            for (size_t i = 0; i < geomList.size(); ++i) geomList[i].second = geomList[i].second * w2l;

            if (downsamples > 0)
            {
                std::sort(geomList.begin(), geomList.end(),
                          [](const osgVerse::GeometryMerger::GeometryPair& lhs, const osgVerse::GeometryMerger::GeometryPair& rhs)
                { return lhs.first->getBound().radius() < rhs.first->getBound().radius(); });

                float toRemove = 1.0f - (1.0f / (float)downsamples); size_t allSize = geomList.size();
                geomList.erase(geomList.begin(), geomList.begin() + size_t((float)allSize * toRemove));
            }

            osgVerse::GeometryMerger merger; osg::Geode* geode = new osg::Geode;
            osg::ref_ptr<osg::Geometry> mergedGeom = merger.process(geomList, 0);
            geode->addDrawable(mergedGeom.get());

            osg::MatrixTransform* mt = new osg::MatrixTransform;
            mt->addChild(geode); mt->setMatrix(l2w); root->addChild(mt);
        }

        if (!lineList.empty())
        {
            osg::Matrix l2w = lineList[0].second; osg::Matrix w2l = osg::Matrix::inverse(l2w);
            for (size_t i = 0; i < lineList.size(); ++i) lineList[i].second = lineList[i].second * w2l;
            if (downsamples > 0)
            {
                for (size_t i = 0; i < lineList.size(); ++i)
                {
                    if (i % downsamples != 0) continue;
                    lineList[new_sz++] = std::move(lineList[i]);
                }
                lineList.resize(new_sz);
            }

            osg::Vec3Array* va = new osg::Vec3Array; osg::Vec3Array* na = new osg::Vec3Array;
            osg::Vec4Array* ca = new osg::Vec4Array; osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_LINES);
            for (size_t i = 0; i < lineList.size(); ++i)
            {
                osg::Geometry* g0 = lineList[i].first; osg::Matrix mat = lineList[i].second;
                osg::Vec3Array* v0 = static_cast<osg::Vec3Array*>(g0->getVertexArray());
                osg::Vec3Array* n0 = static_cast<osg::Vec3Array*>(g0->getNormalArray());
                osg::Vec4Array* c0 = static_cast<osg::Vec4Array*>(g0->getColorArray());
                osg::DrawElementsUInt* d0 = static_cast<osg::DrawElementsUInt*>(g0->getPrimitiveSet(0));

                size_t vStart = va->size(); for (size_t j = 0; j < d0->size(); ++j) de->push_back((*d0)[j] + vStart);
                for (size_t j = 0; j < v0->size(); ++j)
                { va->push_back((*v0)[j] * mat); na->push_back((*n0)[j]); ca->push_back((*c0)[j]); }
                
            }

            osg::Geometry* geom = new osg::Geometry;
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            geom->setVertexArray(va); geom->addPrimitiveSet(de);
            geom->setNormalArray(na); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
            geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

            osg::Geode* geode = new osg::Geode; geode->addDrawable(geom);
            osg::MatrixTransform* mt = new osg::MatrixTransform;
            mt->addChild(geode); mt->setMatrix(l2w); root->addChild(mt);
        }
        root->setUserData(vehicleData.get());
        return root;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_csv");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    static std::string trim(const std::string& str)
    {
        if (!str.size()) return str;
        std::string::size_type first = str.find_first_not_of(" \t");
        std::string::size_type last = str.find_last_not_of("  \t\r\n");
        if ((first == str.npos) || (last == str.npos)) return std::string("");
        return str.substr(first, last - first + 1);
    }

    static osg::Vec4 heightColor(float h, float onRoad)
    {
        if (h < 0.0f) return onRoad > 0.5f ? osgVerse::StringAuxiliary::hexColorToRGB("#ffff00")
                                           : osgVerse::StringAuxiliary::hexColorToRGB("#ff00ff");
        else if (h <= 5.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#e6f4fd");
        else if (h <= 10.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#a7def5");
        else if (h <= 20.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#6fb1df");
        else if (h <= 30.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#4696b2");
        else if (h <= 40.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#49af5b");
        else if (h <= 50.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#9fcf51");
        else if (h <= 60.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#f9da55");
        else if (h <= 70.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#f58c37");
        else if (h <= 80.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#e64f29");
        else if (h <= 90.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#dd3427");
        else if (h <= 100.0f) return osgVerse::StringAuxiliary::hexColorToRGB("#bb1b23");
        else return osgVerse::StringAuxiliary::hexColorToRGB("#931519");
    }

    static void tessellateGeometry(osg::Geometry& geom, const osg::Vec3& axis)
    {
        osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
        tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
        tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
        if (axis.length2() > 0.1f) tscx->setTessellationNormal(axis);
        tscx->retessellatePolygons(geom);
    }

    static osg::Geometry* createLineGeometry(const osgVerse::PointList3D& outer, float offset)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;
        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_LINES);
        for (size_t i = 0; i < outer.size(); ++i)
        {
            va->push_back(outer[i] + osg::Z_AXIS * offset); na->push_back(osg::Z_AXIS);
            de->push_back(i); de->push_back((i + 1) % outer.size());
        }

        osg::ref_ptr<osg::Geometry> geom = osgVerse::createGeometry(va.get(), na.get(), NULL, de.get());
        return geom.release();
    }

    static osg::Geometry* createExtrusionGeometry(const std::vector<osgVerse::PointList3D>& polys, const osg::Vec3& height)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array, vaCap = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        osgVerse::PointList3D pathEx = polys.front();

        osg::ref_ptr<osg::DrawElementsUInt> deWall = new osg::DrawElementsUInt(GL_QUADS);
        bool closed = (pathEx.front() == pathEx.back());
        if (closed && pathEx.front() == pathEx.back()) pathEx.pop_back();

        size_t eSize = pathEx.size(); float eStep = 1.0f / (float)eSize;
        for (size_t i = 0; i <= eSize; ++i)
        {   // outer walls
            if (!closed && i == eSize) continue; size_t start = va->size();
            va->push_back(pathEx[i % eSize]); ta->push_back(osg::Vec2((float)i * eStep, 0.0f));
            va->push_back(pathEx[i % eSize] + height); ta->push_back(osg::Vec2((float)i * eStep, 1.0f));
            vaCap->push_back(va->back());
            va->push_back(pathEx[(i + 1) % eSize]); ta->push_back(osg::Vec2((float)(i + 1) * eStep, 0.0f));
            va->push_back(pathEx[(i + 1) % eSize] + height); ta->push_back(osg::Vec2((float)(i + 1) * eStep, 1.0f));
            vaCap->push_back(va->back());

            osg::Plane plane(va->at(start), va->at(start + 1), va->at(start + 2));
            osg::Vec3 N = plane.getNormal(); na->push_back(N); na->push_back(N); na->push_back(N); na->push_back(N);
            deWall->push_back(start + 1); deWall->push_back(start);
            deWall->push_back(start + 2); deWall->push_back(start + 3);
        }

        size_t startCap = va->size(); osg::Vec3 hNormal = height; hNormal.normalize();
        osg::ref_ptr<osg::DrawElementsUShort> deCap = new osg::DrawElementsUShort(GL_POLYGON);
        va->insert(va->end(), vaCap->begin(), vaCap->end());
        for (size_t i = startCap; i < va->size(); ++i)
        { ta->push_back(osg::Vec2(0.5f, 0.5f)); na->push_back(hNormal); deCap->push_back(i); }

        osg::ref_ptr<osg::Geometry> geom = osgVerse::createGeometry(va.get(), na.get(), ta.get(), deWall.get());
        std::vector<osg::ref_ptr<osg::DrawElementsUShort>> deCapList; deCapList.push_back(deCap);
        for (size_t j = 1; j < polys.size(); ++j)
        {
            vaCap = new osg::Vec3Array; deCap = new osg::DrawElementsUShort(GL_POLYGON);
            pathEx = polys[j]; eSize = pathEx.size(); startCap = va->size();
            for (size_t i = 0; i <= eSize; ++i)
            { vaCap->push_back(pathEx[i % eSize] + height); vaCap->push_back(pathEx[(i + 1) % eSize] + height); }

            va->insert(va->end(), vaCap->begin(), vaCap->end());
            for (size_t i = startCap; i < va->size(); ++i)
            { ta->push_back(osg::Vec2(0.5f, 0.5f)); na->push_back(hNormal); deCap->push_back(i); }
            deCapList.push_back(deCap);
        }
        for (size_t i = 0; i < deCapList.size(); ++i) geom->addPrimitiveSet(deCapList[i].get());
        tessellateGeometry(*geom, hNormal); return geom.release();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_csv, ReaderWriterCSV)
