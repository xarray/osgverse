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
#include <pipeline/IntersectionManager.h>

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

        std::map<size_t, std::string> indexMap;
        std::map<std::string, std::string> valueMap;
        std::string line0; unsigned int rowID = 0;
        std::ifstream in(fileName.c_str()); double z = 0.0;
        if (!in) { std::cout << "[ReaderWriterCSV] Failed to load " << fileName << "\n"; return NULL; }

        std::vector<osgVerse::GeometryMerger::GeometryPair> geomList;
        std::vector<osgVerse::GeometryMerger::GeometryPair> lineList;
        std::vector<osg::ref_ptr<osg::Geometry>> geomRefList;
        while (std::getline(in, line0))
        {
            std::string line = trim(line0); rowID++;
            if (line.empty()) continue;
            if (line[0] == '#') continue;

            std::vector<std::string> values, rings;
            splitString(line, values, ',', false);
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
                double label = (valueMap.find("Label") == valueMap.end()) ? -1.0 : atof(valueMap["Label"].c_str());
                const std::string& vData = valueMap["vertices"]; osg::Vec3d center;
                std::vector<osg::Vec3d> polygon; splitString(vData, rings, '|', true);

                std::vector<std::string> vertices; splitString(rings[0], vertices, ' ', true);  // FIXME: only consider outer ring?
                for (size_t j = 0; j < vertices.size(); j += 2)
                {
                    polygon.push_back(osg::Vec3d(osg::inDegrees(atof(vertices[j + 1].c_str())),
                                                 osg::inDegrees(atof(vertices[j + 0].c_str())), z));
                    center += polygon.back();
                }
                center *= 1.0 / (double)polygon.size();
                if (polygon.size() > 2) polygon.push_back(polygon.front());
            
                osg::Vec3d ecef = osgVerse::Coordinate::convertLLAtoECEF(center);
                osg::Matrix localToWorld = osgVerse::Coordinate::convertLLAtoENU(center);
                osg::Matrix worldToLocal = osg::Matrix::inverse(localToWorld);
                osg::Vec3d N = ecef; N.normalize();
                for (size_t j = 0; j < polygon.size(); ++j)
                {
                    osg::Vec3d pt = osgVerse::Coordinate::convertLLAtoECEF(polygon[j]);
                    polygon[j] = pt * worldToLocal;
                }

                osg::Geometry* geom = height < 0.0 ? createLineGeometry(polygon, 100.0f)
                                    : createExtrusionGeometry(polygon, osg::Z_AXIS * (height * 5.0));
                osg::Vec4Array* ca = new osg::Vec4Array; geom->setColorArray(ca);
                geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
                ca->assign(static_cast<osg::Vec3Array*>(geom->getVertexArray())->size(), heightColor(height));

                if (height < 0.0) lineList.push_back(osgVerse::GeometryMerger::GeometryPair(geom, localToWorld));
                else geomList.push_back(osgVerse::GeometryMerger::GeometryPair(geom, localToWorld));
                geomRefList.push_back(geom);
            }
            else
            {
                for (size_t i = 0; i < values.size(); ++i)
                    { indexMap[i] = values[i]; valueMap[values[i]] = ""; }
            }
        }

        osg::MatrixTransform* root = new osg::MatrixTransform;
        if (!geomList.empty())
        {
            osg::Matrix l2w = geomList[0].second; osg::Matrix w2l = osg::Matrix::inverse(l2w);
            for (size_t i = 0; i < geomList.size(); ++i) geomList[i].second = geomList[i].second * w2l;

            if (options)
            {
                const std::string& dsStr = options->getPluginStringData("Downsamples");
                size_t downsamples = atoi(dsStr.c_str()), new_sz = 0;
                if (downsamples > 0)
                {
                    for (size_t i = 0; i < geomList.size(); ++i)
                    {
                        if (i % downsamples != 0) continue;
                        geomList[new_sz++] = std::move(geomList[i]);
                    }
                    geomList.resize(new_sz);
                }
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

            osg::Vec3Array* va = new osg::Vec3Array; osg::Vec3Array* na = new osg::Vec3Array;
            osg::Vec4Array* ca = new osg::Vec4Array; osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_LINES);
            for (size_t i = 0; i < lineList.size(); ++i)
            {
                osg::Geometry* g0 = lineList[i].first; osg::Matrix mat = lineList[i].second;
                osg::Vec3Array* v0 = static_cast<osg::Vec3Array*>(g0->getVertexArray());
                osg::Vec4Array* c0 = static_cast<osg::Vec4Array*>(g0->getColorArray());
                osg::DrawElementsUInt* d0 = static_cast<osg::DrawElementsUInt*>(g0->getPrimitiveSet(0));

                size_t vStart = va->size();
                for (size_t j = 0; j < v0->size(); ++j) { va->push_back((*v0)[j] * mat); ca->push_back((*c0)[j]); }
                for (size_t j = 0; j < d0->size(); ++j) de->push_back((*d0)[j] + vStart);
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

    static void splitString(const std::string& src, std::vector<std::string>& slist, char sep, bool ignoreEmpty)
    {
        if (src.empty()) return;
        std::string::size_type start = 0;
        bool inQuotes = false;

        for (std::string::size_type i = 0; i < src.size(); ++i)
        {
            if (src[i] == '"')
                inQuotes = !inQuotes;
            else if (src[i] == sep && !inQuotes)
            {
                if (!ignoreEmpty || (i - start) > 0)
                    slist.push_back(src.substr(start, i - start));
                start = i + 1;
            }
        }
        if (!ignoreEmpty || (src.size() - start) > 0)
            slist.push_back(src.substr(start, src.size() - start));
    }

    static osg::Vec4 hexColorToRGB(const std::string& hexColor)
    {
        if (hexColor.empty() || hexColor[0] != '#') return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        std::string colorPart = hexColor.substr(1);
        unsigned long hexValue = 0;
        if (colorPart.length() == 3)
        {   // #abc -> #aabbcc
            std::string expanded;
            for (char c : colorPart) expanded += std::string(2, c);
            colorPart = expanded;
        }
        else if (colorPart.length() != 6) return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);

        std::istringstream iss(colorPart);
        if (!(iss >> std::hex >> hexValue)) return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        return osg::Vec4(((hexValue >> 16) & 0xFF) / 255.0f, ((hexValue >> 8) & 0xFF) / 255.0f,
                         (hexValue & 0xFF) / 255.0f, 1.0f);
    }

    static osg::Vec4 heightColor(float h)
    {
        if (h <= 0.0f) return hexColorToRGB("#ffffff");
        if (h <= 5.0f) return hexColorToRGB("#e6f4fd");
        else if (h <= 10.0f) return hexColorToRGB("#a7def5");
        else if (h <= 20.0f) return hexColorToRGB("#6fb1df");
        else if (h <= 30.0f) return hexColorToRGB("#4696b2");
        else if (h <= 40.0f) return hexColorToRGB("#49af5b");
        else if (h <= 50.0f) return hexColorToRGB("#9fcf51");
        else if (h <= 60.0f) return hexColorToRGB("#f9da55");
        else if (h <= 70.0f) return hexColorToRGB("#f58c37");
        else if (h <= 80.0f) return hexColorToRGB("#e64f29");
        else if (h <= 90.0f) return hexColorToRGB("#dd3427");
        else if (h <= 100.0f) return hexColorToRGB("#bb1b23");
        else return hexColorToRGB("#931519");
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

    static osg::Geometry* createExtrusionGeometry(const osgVerse::PointList3D& outer, const osg::Vec3& height)
    {
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array, vaCap = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
        osgVerse::PointList3D pathEx = outer;

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
        {
            ta->push_back(osg::Vec2(0.5f, 0.5f));
            na->push_back(hNormal); deCap->push_back(i);
        }

        osg::ref_ptr<osg::Geometry> geom = osgVerse::createGeometry(va.get(), na.get(), ta.get(), deWall.get());
        geom->addPrimitiveSet(deCap.get()); tessellateGeometry(*geom, hNormal); return geom.release();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_csv, ReaderWriterCSV)
