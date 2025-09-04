#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>
#include <osgUtil/Tessellator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <3rdparty/rapidxml/rapidxml.hpp>
#include <3rdparty/rapidxml/rapidxml_utils.hpp>

#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/DatabasePager.h>
#include <readerwriter/TileCallback.h>
#include <pipeline/IncrementalCompiler.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#define XMLNS(c) (xmlns[c].c_str())
#define XNODE_VALUE(name, ns) (currentNode->first_node(name, XMLNS(ns)) ? \
                               currentNode->first_node(name, XMLNS(ns))->value() : "")

void parseGPMLGeometry(osg::Geometry& geom, rapidxml::xml_node<>* valueNode,
                       std::map<std::string, std::string>& xmlns)
{
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
    if (!va) { va = new osg::Vec3Array; geom.setVertexArray(va); }

    for (rapidxml::xml_node<>* primitiveNode = valueNode->first_node();
         primitiveNode; primitiveNode = primitiveNode->next_sibling())
    {
        if (primitiveNode->name() == std::string("Polygon"))
        {
            for (rapidxml::xml_node<>* boundaryNode = primitiveNode->first_node();
                 boundaryNode; boundaryNode = boundaryNode->next_sibling())
            {
                int boundaryType = 0;  // TODO: not handling exterior/interior
                if (boundaryNode->name() == std::string("exterior")) boundaryType = 1;
                else if (boundaryNode->name() == std::string("interior")) boundaryType = -1;
                else std::cout << "Unsupported boundary type: " << boundaryNode->name() << "\n";

                for (rapidxml::xml_node<>* currentNode = boundaryNode->first_node();
                     currentNode; currentNode = currentNode->next_sibling())
                {
                    if (currentNode->name() == std::string("LinearRing"))
                    {
                        rapidxml::xml_node<>* posListNode = currentNode->first_node("posList", XMLNS("gml"));
                        rapidxml::xml_attribute<>* dAttr = posListNode->first_attribute("dimension");
                        size_t dim = (dAttr ? atoi(dAttr->value()) : 2), vStart = va->size();

                        std::vector<std::string> posList; osgDB::split(posListNode->value(), posList, ' ');
                        float firstX = atof(posList[1].c_str()); int splittedID = -1;
                        for (size_t i = 0; i < posList.size(); i += dim)
                        {
                            float x = atof(posList[i + 1].c_str()), y = atof(posList[i].c_str());
                            float z = (dim > 2) ? atof(posList[i + 2].c_str()) : 0.0f;
                            if (fabs(firstX - x) > 240.0)
                                { if (firstX < x) x -= 360.0f; else x += 360.0f; splittedID = (int)i; }
                            va->push_back(osg::Vec3(x, y, z));
                        }

                        if (splittedID >= 0)
                        {
                            size_t vStart2 = va->size(); firstX = atof(posList[splittedID + 1].c_str());
                            for (size_t i = 0; i < posList.size(); i += dim)
                            {
                                float x = atof(posList[i + 1].c_str()), y = atof(posList[i].c_str());
                                float z = (dim > 2) ? atof(posList[i + 2].c_str()) : 0.0f;
                                if (fabs(firstX - x) > 240.0)
                                    { if (firstX < x) x -= 360.0f; else x += 360.0f; }
                                va->push_back(osg::Vec3(x, y, z));
                            }
                            geom.addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, vStart, vStart2 - vStart));
                            geom.addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, vStart2, va->size() - vStart2));
                        }
                        else
                            geom.addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, vStart, va->size() - vStart));
                    }
                    else std::cout << "Unsupported geometry type: " << currentNode->name() << "\n";
                }
            }

            osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
            tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
            tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
            tscx->setTessellationNormal(osg::Z_AXIS);
            tscx->retessellatePolygons(geom);
        }
        else std::cout << "Unsupported primitive type: " << primitiveNode->name() << "\n";
    }
}

osg::Geometry* parseGPMLFeature(rapidxml::xml_node<>* featureNode,
                                std::map<std::string, std::string>& xmlns)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    if (featureNode->name() == std::string("UnclassifiedFeature"))
    {
        rapidxml::xml_node<>* currentNode = featureNode;
        std::string identity = XNODE_VALUE("identity", "gpml");
        std::string revision = XNODE_VALUE("revision", "gpml");
        std::string conjugatePlateId = XNODE_VALUE("conjugatePlateId", "gpml");
        std::string leftPlate = XNODE_VALUE("leftPlate", "gpml");
        std::string rightPlate = XNODE_VALUE("rightPlate", "gpml");
        std::string spreadingAsymmetry = XNODE_VALUE("spreadingAsymmetry", "gpml");
        geom->addDescription(XNODE_VALUE("description", "gml"));
        geom->setName(XNODE_VALUE("name", "gml"));

        std::cout << "UnclassifiedFeature " << geom->getName() << ": desc=" << geom->getDescription(0)
                  << "; left=" << leftPlate << "; right=" << rightPlate << "\n";

        rapidxml::xml_node<>* reconIdNode = featureNode->first_node("reconstructionPlateId", XMLNS("gpml"));
        if (reconIdNode != NULL)
        {
            for (currentNode = reconIdNode->first_node("ConstantValue", XMLNS("gpml"));
                 currentNode; currentNode = currentNode->next_sibling("ConstantValue", XMLNS("gpml")))
            {
                std::string plateType = XNODE_VALUE("valueType", "gpml");
                std::string plateValue = XNODE_VALUE("value", "gpml");
                std::string plateDescription = XNODE_VALUE("description", "gml");
                // TODO
            }
        }

        rapidxml::xml_node<>* shpAttrNode = featureNode->first_node("shapefileAttributes", XMLNS("gpml"));
        if (shpAttrNode != NULL)
        {
            for (rapidxml::xml_node<>* kvNode = shpAttrNode->first_node("KeyValueDictionary", XMLNS("gpml"));
                 kvNode; kvNode = kvNode->next_sibling("KeyValueDictionary", XMLNS("gpml")))
            {
                for (rapidxml::xml_node<>* elementNode = kvNode->first_node("element", XMLNS("gpml"));
                     elementNode; elementNode = elementNode->next_sibling("element", XMLNS("gpml")))
                {
                    for (currentNode = elementNode->first_node("KeyValueDictionaryElement", XMLNS("gpml"));
                         currentNode; currentNode = currentNode->next_sibling("KeyValueDictionaryElement", XMLNS("gpml")))
                    {
                        std::string key = XNODE_VALUE("key", "gpml");
                        std::string value = XNODE_VALUE("value", "gpml");
                        std::string valueType = XNODE_VALUE("valueType", "gpml");
                        // TODO
                    }
                }
            }  // for (...)
        }

        rapidxml::xml_node<>* geometryNode = featureNode->first_node("unclassifiedGeometry", XMLNS("gpml"));
        if (geometryNode != NULL)
        {
            for (currentNode = geometryNode->first_node("ConstantValue", XMLNS("gpml"));
                 currentNode; currentNode = currentNode->next_sibling("ConstantValue", XMLNS("gpml")))
            {
                std::string description = XNODE_VALUE("description", "gml");
                std::string valueType = XNODE_VALUE("valueType", "gpml");
                rapidxml::xml_node<>* valueNode = currentNode->first_node("value", XMLNS("gpml"));
                parseGPMLGeometry(*geom, valueNode, xmlns);
            }
        }

        rapidxml::xml_node<>* timeNode = featureNode->first_node("validTime", XMLNS("gml"));
        if (timeNode != NULL)
        {
            for (rapidxml::xml_node<>* periodNode = timeNode->first_node("TimePeriod", XMLNS("gml"));
                 periodNode; periodNode = periodNode->next_sibling("TimePeriod", XMLNS("gml")))
            {
                rapidxml::xml_node<>* beginNode = periodNode->first_node("begin", XMLNS("gml"));
                rapidxml::xml_node<>* endNode = periodNode->first_node("end", XMLNS("gml"));
                std::string beginValue, endValue;

                currentNode = beginNode->first_node("TimeInstant", XMLNS("gml"));
                beginValue = XNODE_VALUE("timePosition", "gml");
                currentNode = endNode->first_node("TimeInstant", XMLNS("gml"));
                endValue = XNODE_VALUE("timePosition", "gml");

                geom->setUserValue("BeginTime", beginValue);
                geom->setUserValue("EndTime", endValue);

                //if (!(beginValue == "0" && endValue == "0")) return NULL;
            }
        }
    }
    else
    { std::cout << "Unknown feature type: " << featureNode->name() << "\n"; return NULL; }

    geom->setUseVertexBufferObjects(true);
    geom->setUseDisplayList(false);
    return geom.release();
}

osg::Geode* parseGPML(const std::string& gpmlFile)
{
    std::map<std::string, std::string> xmlns;
    xmlns["gpml"] = "http://www.gplates.org/gplates";
    xmlns["gml"] = "http://www.opengis.net/gml";

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    try
    {
        rapidxml::file<> xmlFile(gpmlFile.c_str());
        rapidxml::xml_document<> doc;
        doc.parse<0>(xmlFile.data());

        rapidxml::xml_node<>* root = doc.first_node("FeatureCollection", XMLNS("gpml"));
        if (!root) return NULL;

        for (rapidxml::xml_node<>* memberNode = root->first_node("featureMember", XMLNS("gml"));
             memberNode; memberNode = memberNode->next_sibling("featureMember", XMLNS("gml")))
        {
            for (rapidxml::xml_node<>* featureNode = memberNode->first_node();
                 featureNode; featureNode = featureNode->next_sibling())
            {
                osg::Geometry* geom = parseGPMLFeature(featureNode, xmlns);
                if (geom) geode->addDrawable(geom);
            }
        }
    }
    catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << "\n"; return NULL; }
    return geode.release();
}

class TimelineHandler : public osgGA::GUIEventHandler
{
public:
    TimelineHandler(osg::Geode* g) : _plates(g), _timeValue(0) {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            //_timeValue++;

        }
        return false;
    }

protected:
    osg::observer_ptr<osg::Geode> _plates;
    unsigned int _timeValue;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");
    osgVerse::updateOsgBinaryWrappers();

    std::string mainFolder = BASE_DIR + "/models/Earth"; arguments.read("--folder", mainFolder);
    std::string earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/DOM_lv4.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://" + mainFolder + "/DEM_lv3.mbtiles/{z}-{x}-{y}.tif"
                            " UseWebMercator=1 OriginBottomLeft=1 TileSkirtRatio=0.05 UseEarth3D=0";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    if (!earth) return 1;

    osg::ref_ptr<osg::Geode> plates = parseGPML(MISC_DIR + "PALEOMAP_PlatePolygons.gpml");
    osg::ref_ptr<osg::MatrixTransform> platesMT = new osg::MatrixTransform;
    platesMT->addChild(plates.get()); platesMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 0.1f));

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(earth.get());
    root->addChild(platesMT.get());

    osgViewer::Viewer viewer;
    viewer.getCamera()->setNearFarRatio(0.00001);
    viewer.addEventHandler(new TimelineHandler(plates.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setSceneData(root.get());
    return viewer.run();
}
