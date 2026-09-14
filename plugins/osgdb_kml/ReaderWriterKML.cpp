#include <osg/io_utils>
#include <osg/Math>
#include <osg/ValueObject>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <pipeline/Drawer2D.h>
#include <readerwriter/FeatureDefinition.h>
#include <readerwriter/Utilities.h>
#include "3rdparty/rapidxml/rapidxml.hpp"

#include <fstream>
#include <map>
#include <sstream>
#include <string.h>
#include <vector>

namespace
{
    typedef rapidxml::xml_node<> XmlNode;
    typedef rapidxml::xml_attribute<> XmlAttr;
    typedef std::map<std::string, std::string> PropertyMap;

    // -------- XML helpers, namespace-prefix aware (e.g. <kml:coordinates>) --------
    static std::string localName(const char* name)
    {
        if (name == NULL) return std::string();
        const char* pos = strrchr(name, ':');
        return pos ? std::string(pos + 1) : std::string(name);
    }

    static bool isElement(XmlNode* node)
    { return node != NULL && node->type() == rapidxml::node_element; }

    static bool isText(XmlNode* node)
    {
        return node != NULL &&
               (node->type() == rapidxml::node_data || node->type() == rapidxml::node_cdata);
    }

    static std::string nodeValue(XmlNode* node)
    {
        if (node == NULL || node->value() == NULL) return std::string();
        return std::string(node->value(), node->value_size());
    }

    /** Note: rapidxml gives text/CDATA/comment nodes an empty (instead of NULL) name,
        therefore type() must be checked to tell them from real elements */
    static std::string nodeText(XmlNode* node)
    {
        if (node == NULL) return std::string();
        std::string text;
        for (XmlNode* c = node->first_node(); c != NULL; c = c->next_sibling())
            if (isText(c)) text += nodeValue(c);  // text and CDATA parts
        if (text.empty()) text = nodeValue(node);
        return text;
    }

    static bool isNamed(XmlNode* node, const char* expected)
    {
        if (!isElement(node) || node->name() == NULL) return false;
        return osgDB::equalCaseInsensitive(localName(node->name()), expected);
    }

    static XmlNode* findChild(XmlNode* parent, const char* name, XmlNode* start = NULL)
    {
        if (parent == NULL) return NULL;
        for (XmlNode* n = start ? start->next_sibling() : parent->first_node(); n != NULL;
             n = n->next_sibling())
        {
            if (isNamed(n, name)) return n;
        }
        return NULL;
    }

    static std::string childText(XmlNode* parent, const char* name)
    { return nodeValue(findChild(parent, name)); }

    static std::string attributeText(XmlAttr* attr)
    {
        if (attr == NULL || attr->value() == NULL) return std::string();
        return std::string(attr->value(), attr->value_size());
    }

    static bool hasElementChild(XmlNode* node)
    {
        for (XmlNode* c = node ? node->first_node() : NULL; c != NULL; c = c->next_sibling())
            if (isElement(c)) return true;
        return false;
    }

    // -------- KML element classification --------
    static bool isGeometryElement(const std::string& name)
    {
        return name == "Point" || name == "LineString" || name == "LinearRing" ||
               name == "Polygon" || name == "MultiGeometry" || name == "Model";
    }

    static bool isGeometryContainer(const std::string& name)
    { return name == "MultiGeometry"; }

    static bool isPropertyHolder(const std::string& name)
    { return name == "NetworkLink"; }

    // -------- coordinates: "longitude,latitude[,altitude]" separated by whitespace --------
    static bool parseOneCoordinate(const std::string& token, osg::Vec3& result)
    {
        double value[3] = { 0.0, 0.0, 0.0 }; int index = 0; size_t start = 0;
        while (index < 3)
        {
            size_t comma = token.find(',', start);
            std::string part = (comma == std::string::npos) ? token.substr(start)
                                                            : token.substr(start, comma - start);
            if (!part.empty()) value[index++] = osg::asciiToDouble(part.c_str());
            if (comma == std::string::npos) break; start = comma + 1;
        }
        if (index < 2) return false;
        result.set((float)value[0], (float)value[1], (float)value[2]); return true;
    }

    static size_t parseCoordinates(const std::string& text, osg::Vec3Array* va)
    {
        if (va == NULL) return 0;
        std::istringstream iss(text); std::string token; size_t count = 0;
        while (iss >> token)
        {
            osg::Vec3 pt; if (parseOneCoordinate(token, pt)) { va->push_back(pt); ++count; }
        }
        return count;
    }

    static void removeClosedPoint(osg::Vec3Array* va)
    {
        if (va && va->size() > 1 && (*va)[0] == va->back()) va->pop_back();
    }

    static bool hasPoints(osgVerse::Feature* feature)
    {
        if (feature == NULL) return false;
        const std::vector<osg::ref_ptr<osg::Vec3Array>>& ptList = feature->getPointList();
        for (size_t i = 0; i < ptList.size(); ++i)
            if (ptList[i].valid() && !ptList[i]->empty()) return true;
        return false;
    }

    // -------- property flattening: nested elements become "Parent.Child" keys --------
    static void flattenProperties(XmlNode* node, const std::string& prefix, PropertyMap& out)
    {
        for (XmlNode* c = node->first_node(); c != NULL; c = c->next_sibling())
        {
            if (!isElement(c)) continue;
            std::string path = prefix.empty() ? localName(c->name())
                                              : (prefix + "." + localName(c->name()));
            if (hasElementChild(c)) flattenProperties(c, path, out);
            else
            {
                std::string value = nodeText(c);
                if (!value.empty()) out[path] = value;
            }
        }
    }

    static void applyProperties(osgVerse::Feature* feature, const PropertyMap& props)
    {
        for (PropertyMap::const_iterator it = props.begin(); it != props.end(); ++it)
            feature->setUserValue(it->first, it->second);
    }

    static void flattenInto(osgVerse::Feature* feature, XmlNode* node, const std::string& prefix)
    {
        PropertyMap props; flattenProperties(node, prefix, props); applyProperties(feature, props);
    }

    // -------- styles: collect <Style>/<StyleMap> and merge into features --------
    struct KMLContext
    {
        std::map<std::string, PropertyMap> styles;
        std::map<std::string, std::map<std::string, std::string>> styleMapRefs;

        const PropertyMap* resolveStyle(const std::string& url, int depth = 0) const
        {
            if (url.empty() || url[0] != '#' || depth > 4) return NULL;
            std::string id = url.substr(1);
            std::map<std::string, std::map<std::string, std::string>>::const_iterator sm =
                styleMapRefs.find(id);
            if (sm != styleMapRefs.end())
            {
                if (sm->second.empty()) return NULL;
                std::map<std::string, std::string>::const_iterator pair = sm->second.find("normal");
                if (pair == sm->second.end()) pair = sm->second.begin();
                return resolveStyle(pair->second, depth + 1);
            }

            std::map<std::string, PropertyMap>::const_iterator st = styles.find(id);
            return (st == styles.end()) ? NULL : &(st->second);
        }
    };

    static void parseStyle(KMLContext& ctx, XmlNode* style)
    {
        XmlAttr* attr = style->first_attribute("id"); if (attr == NULL) return;
        PropertyMap props; flattenProperties(style, "", props);
        ctx.styles[attributeText(attr)] = props;
    }

    static void parseStyleMap(KMLContext& ctx, XmlNode* styleMap)
    {
        XmlAttr* attr = styleMap->first_attribute("id"); if (attr == NULL) return;
        std::string id = attributeText(attr);
        for (XmlNode* pair = findChild(styleMap, "Pair"); pair != NULL;
             pair = findChild(styleMap, "Pair", pair))
        {
            std::string key = childText(pair, "key"), url = childText(pair, "styleUrl");
            if (!key.empty() && !url.empty()) ctx.styleMapRefs[id][key] = url;
        }
    }

    static void applyStyleProperties(osgVerse::Feature* feature, XmlNode* holder,
                                     const KMLContext& ctx)
    {
        XmlNode* inlineStyle = findChild(holder, "Style");
        if (inlineStyle != NULL) flattenInto(feature, inlineStyle, "");

        std::string url = childText(holder, "styleUrl");
        if (url.empty()) return;
        const PropertyMap* style = ctx.resolveStyle(url);
        if (style != NULL) applyProperties(feature, *style);
    }

    // -------- ExtendedData: <Data>, <SchemaData>/<SimpleData>, <SimpleData> --------
    static void applyExtendedData(osgVerse::Feature* feature, XmlNode* extended)
    {
        for (XmlNode* c = extended->first_node(); c != NULL; c = c->next_sibling())
        {
            if (!isElement(c)) continue;
            std::string name = localName(c->name());
            if (name == "Data")
            {
                std::string key = attributeText(c->first_attribute("name"));
                if (key.empty()) continue;
                std::string value = nodeText(findChild(c, "value"));
                if (value.empty()) value = nodeText(c);
                feature->setUserValue(key, value);
            }
            else if (name == "SimpleData")
            {
                std::string key = attributeText(c->first_attribute("name"));
                if (!key.empty()) feature->setUserValue(key, nodeText(c));
            }
            else if (name == "SchemaData")
            {
                for (XmlNode* s = findChild(c, "SimpleData"); s != NULL;
                     s = findChild(c, "SimpleData", s))
                {
                    std::string key = attributeText(s->first_attribute("name"));
                    if (!key.empty()) feature->setUserValue(key, nodeText(s));
                }
            }
        }
    }

    static void applyCommonProperties(osgVerse::Feature* feature, XmlNode* holder)
    {
        std::string id = attributeText(holder->first_attribute("id"));
        if (!id.empty()) feature->setUserValue("id", id);

        // ExtendedData is applied first so that standard KML tags are not shadowed
        for (XmlNode* c = findChild(holder, "ExtendedData"); c != NULL;
             c = findChild(holder, "ExtendedData", c))
        {
            applyExtendedData(feature, c);
        }

        for (XmlNode* c = holder->first_node(); c != NULL; c = c->next_sibling())
        {
            if (!isElement(c)) continue;
            std::string name = localName(c->name());
            if (isGeometryElement(name) || isPropertyHolder(name)) continue;
            if (isNamed(c, "ExtendedData")) continue;
            if (isNamed(c, "Style") || isNamed(c, "StyleMap")) continue;

            if (hasElementChild(c)) flattenInto(feature, c, name);
            else
            {
                std::string value = nodeText(c);
                if (value.empty()) continue;
                feature->setUserValue(name, value);
                if (name == "name") feature->setName(value);
            }
        }
    }

    // -------- geometry builders --------
    static void buildPolygonRing(osgVerse::Feature* feature, XmlNode* boundary)
    {
        XmlNode* ring = findChild(boundary, "LinearRing");
        if (ring == NULL) ring = boundary;
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        parseCoordinates(childText(ring, "coordinates"), va.get());
        removeClosedPoint(va.get());
        if (va->empty()) return;
        feature->addPoints(va.get());
    }

    static void buildGeometry(osgVerse::Feature* feature, XmlNode* geom)
    {
        std::string name = localName(geom->name());
        feature->setUserValue("kmlType", name);

        // Geometry-level simple attributes, e.g. <altitudeMode>, <extrude>, <tessellate>
        for (XmlNode* c = geom->first_node(); c != NULL; c = c->next_sibling())
        {
            if (!isElement(c) || isNamed(c, "coordinates") || hasElementChild(c)) continue;
            std::string value = nodeText(c);
            if (!value.empty()) feature->setUserValue(localName(c->name()), value);
        }

        if (name == "Point" || name == "LineString" || name == "LinearRing")
        {
            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
            parseCoordinates(childText(geom, "coordinates"), va.get());
            if (va->empty()) return;
            if (name == "Point") feature->setType(GL_POINTS);
            else if (name == "LineString") feature->setType(GL_LINE_STRIP);
            else feature->setType(GL_LINE_LOOP);
            feature->addPoints(va.get());
        }
        else if (name == "Polygon")
        {
            feature->setType(GL_POLYGON);
            for (XmlNode* b = findChild(geom, "outerBoundaryIs"); b != NULL;
                 b = findChild(geom, "outerBoundaryIs", b))
            {
                buildPolygonRing(feature, b);
            }
            for (XmlNode* b = findChild(geom, "innerBoundaryIs"); b != NULL;
                 b = findChild(geom, "innerBoundaryIs", b))
            {
                buildPolygonRing(feature, b);  // holes, appended after the outer ring
            }
        }
        else if (name == "Model")
        {
            // The referenced model file is NOT loaded here; only its placement is parsed
            XmlNode* location = findChild(geom, "Location");
            if (location != NULL)
            {
                osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
                double lon = osg::asciiToDouble(childText(location, "longitude").c_str());
                double lat = osg::asciiToDouble(childText(location, "latitude").c_str());
                double alt = osg::asciiToDouble(childText(location, "altitude").c_str());
                va->push_back(osg::Vec3((float)lon, (float)lat, (float)alt));
                feature->setType(GL_POINTS); feature->addPoints(va.get());
            }
            flattenInto(feature, geom, "");  // Location.*, Orientation.*, Scale.*, Link.href
        }
    }

    static void parsePlacemark(osgVerse::FeatureCollection* coll, KMLContext& ctx, XmlNode* pm)
    {
        size_t numBefore = coll->features.size();
        for (XmlNode* c = pm->first_node(); c != NULL; c = c->next_sibling())
        {
            if (!isElement(c)) continue;
            std::string name = localName(c->name());
            if (!isGeometryElement(name)) continue;

            if (isGeometryContainer(name))
            {
                for (XmlNode* sub = c->first_node(); sub != NULL; sub = sub->next_sibling())
                {
                    if (!isElement(sub)) continue;
                    std::string subName = localName(sub->name());
                    if (!isGeometryElement(subName) || isGeometryContainer(subName)) continue;

                    osg::ref_ptr<osgVerse::Feature> feature = new osgVerse::Feature;
                    applyCommonProperties(feature.get(), pm);
                    applyStyleProperties(feature.get(), pm, ctx);
                    buildGeometry(feature.get(), sub);
                    if (hasPoints(feature.get())) coll->push_back(feature.get());
                }
            }
            else
            {
                osg::ref_ptr<osgVerse::Feature> feature = new osgVerse::Feature;
                applyCommonProperties(feature.get(), pm);
                applyStyleProperties(feature.get(), pm, ctx);
                buildGeometry(feature.get(), c);
                if (hasPoints(feature.get())) coll->push_back(feature.get());
            }
        }

        // A Placemark without usable geometry is still kept for its properties
        if (coll->features.size() == numBefore)
        {
            osg::ref_ptr<osgVerse::Feature> feature = new osgVerse::Feature;
            feature->setUserValue("kmlType", std::string("Placemark"));
            applyCommonProperties(feature.get(), pm);
            applyStyleProperties(feature.get(), pm, ctx);
            coll->push_back(feature.get());
        }
    }

    static void parsePropertyHolder(osgVerse::FeatureCollection* coll, KMLContext& ctx,
                                    XmlNode* holder, const std::string& type)
    {
        // e.g. NetworkLink: only parsed into properties, referenced contents are not loaded
        osg::ref_ptr<osgVerse::Feature> feature = new osgVerse::Feature;
        feature->setUserValue("kmlType", type);
        applyCommonProperties(feature.get(), holder);
        applyStyleProperties(feature.get(), holder, ctx);
        coll->push_back(feature.get());
    }

    static void parseContainer(osgVerse::FeatureCollection* coll, KMLContext& ctx, XmlNode* parent)
    {
        for (XmlNode* c = parent->first_node(); c != NULL; c = c->next_sibling())
        {
            if (!isElement(c)) continue;
            std::string name = localName(c->name());
            if (name == "Style") parseStyle(ctx, c);
            else if (name == "StyleMap") parseStyleMap(ctx, c);
            else if (name == "Placemark") parsePlacemark(coll, ctx, c);
            else if (name == "Folder" || name == "Document") parseContainer(coll, ctx, c);
            else if (isPropertyHolder(name)) parsePropertyHolder(coll, ctx, c, name);
        }
    }

    static osgVerse::FeatureCollection* parseKML(const std::string& xml)
    {
        if (xml.empty()) return NULL;
        std::vector<char> buffer(xml.begin(), xml.end()); buffer.push_back('\0');
        rapidxml::xml_document<> doc;
        try
        {
            doc.parse<rapidxml::parse_default | rapidxml::parse_trim_whitespace>(buffer.data());
        }
        catch (const std::exception& err)
        {
            OSG_WARN << "[ReaderWriterKML] Failed to parse XML: " << err.what() << std::endl;
            return NULL;
        }

        osg::ref_ptr<osgVerse::FeatureCollection> coll = new osgVerse::FeatureCollection;
        XmlNode* root = doc.first_node();
        while (root != NULL && !isElement(root)) root = root->next_sibling();
        if (root == NULL)
        {
            OSG_WARN << "[ReaderWriterKML] No root element found" << std::endl;
            return coll.release();
        }

        KMLContext ctx;
        if (isNamed(root, "Placemark")) parsePlacemark(coll.get(), ctx, root);
        else if (isPropertyHolder(localName(root->name())))
            parsePropertyHolder(coll.get(), ctx, root, localName(root->name()));
        else parseContainer(coll.get(), ctx, root);
        return coll.release();
    }

    /** Extract and parse the KML entry inside a KMZ (zip) buffer, or parse the buffer directly */
    static osgVerse::FeatureCollection* readFeatureCollection(std::string& buffer)
    {
        if (buffer.size() > 4 && buffer[0] == 'P' && buffer[1] == 'K' &&
            (unsigned char)buffer[2] == 3 && (unsigned char)buffer[3] == 4)
        {
            osg::ref_ptr<osg::Referenced> zip = osgVerse::CompressAuxiliary::createHandle(
                osgVerse::CompressAuxiliary::ZIP, (unsigned char*)buffer.data(), buffer.size());
            if (!zip.valid()) { OSG_WARN << "[ReaderWriterKML] Invalid KMZ archive" << std::endl; return NULL; }

            std::vector<std::string> files = osgVerse::CompressAuxiliary::listContents(zip.get());
            std::string target;
            for (size_t i = 0; i < files.size(); ++i)
            {
                if (osgDB::getLowerCaseFileExtension(files[i]) != "kml") continue;
                if (osgDB::equalCaseInsensitive(osgDB::getSimpleFileName(files[i]), "doc.kml"))
                    { target = files[i]; break; }
                if (target.empty()) target = files[i];
            }

            std::vector<unsigned char> data;
            if (!target.empty()) data = osgVerse::CompressAuxiliary::extract(zip.get(), target);
            osgVerse::CompressAuxiliary::destroyHandle(zip.get());
            if (data.empty())
            {
                OSG_WARN << "[ReaderWriterKML] No KML file found inside the KMZ archive" << std::endl;
                return NULL;
            }
            return parseKML(std::string((char*)data.data(), data.size()));
        }
        return parseKML(buffer);
    }
}

class ReaderWriterKML : public osgDB::ReaderWriter
{
public:
    ReaderWriterKML()
    {
        supportsExtension("verse_kml", "osgVerse pseudo-loader");
        supportsExtension("kml", "Keyhole Markup Language data file");
        supportsExtension("kmz", "Compressed Keyhole Markup Language data file");
        supportsOption("IncludeFeatures", "Add FeatureCollection as UserData of the result Geometry/Image. Default: 0");
        supportsOption("ImageWidth", "Image resolution. Default: 512");
        supportsOption("ImageHeight", "Image resolution. Default: 512");
    }

    virtual const char* className() const
    {
        return "[osgVerse] KML/KMZ feature data format reader";
    }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));

        std::ifstream in(fileName.c_str(), std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readObject(in, lOptions.get());
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));

        std::ifstream in(fileName.c_str(), std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readNode(in, lOptions.get());
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));

        std::ifstream in(fileName.c_str(), std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readImage(in, lOptions.get());
    }

    virtual ReadResult readObject(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osgVerse::FeatureCollection> coll = readFeatureCollection(buffer);
        if (!coll.valid()) return ReadResult::ERROR_IN_READING_FILE;
        return coll.release();
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osgVerse::FeatureCollection> coll = readFeatureCollection(buffer);
        if (!coll.valid()) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osg::Geode> geode = new osg::Geode; osg::ref_ptr<osg::Geometry> geom;
        for (size_t i = 0; i < coll->features.size(); ++i)
        {
            osgVerse::Feature* feature = coll->features[i].get();
            if (!hasPoints(feature)) continue;
            if (!geom)
            {
                geom = new osg::Geometry; geode->addDrawable(geom.get());
                geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            }

            osgVerse::addFeatureToGeometry(*feature, geom.get(), true);
            if (geom->getNumPrimitiveSets() > 1024) geom = NULL;
        }

        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) geode->setUserData(coll.get());
        }
        return geode.get();
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osgVerse::FeatureCollection> coll = readFeatureCollection(buffer);
        if (!coll.valid()) return ReadResult::ERROR_IN_READING_FILE;

        std::string wStr = options ? options->getPluginStringData("ImageWidth") : "512";
        std::string hStr = options ? options->getPluginStringData("ImageHeight") : "512";
        int w = atoi(wStr.c_str()), h = atoi(hStr.c_str()); if (w < 1) w = 512; if (h < 1) h = 512;

        osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) drawer->setUserData(coll.get());
        }
        drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));
        drawer->start(false); drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

        const osg::BoundingBox& bb = coll->bound;
        osg::Vec2 off(-bb.xMin(), -bb.yMin()),
                  sc((float)w / (bb.xMax() - bb.xMin()), (float)h / (bb.yMax() - bb.yMin()));
        osgVerse::DrawerStyleData fillStyle(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
        for (size_t i = 0; i < coll->features.size(); ++i)
        {
            osgVerse::Feature* feature = coll->features[i].get();
            if (!hasPoints(feature)) continue;
            osgVerse::drawFeatureToImage(*feature, drawer.get(), off, sc, &fillStyle);
        }
        drawer->finish(); return drawer.get();
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_kml");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_kml, ReaderWriterKML)
