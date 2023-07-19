#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ClassInterface>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <script/ScriptBase.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class KeyEventHandler : public osgGA::GUIEventHandler
{
public:
    typedef std::function<void()> KeyCallback;
    void addCallback(int key, KeyCallback cb) { _keyCallbacks[key] = cb; }
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            int key = ea.getKey();
            if (_keyCallbacks.find(key) != _keyCallbacks.end())
            { _keyCallbacks[key](); }
        }
        return false;
    }

protected:
    std::map<int, KeyCallback> _keyCallbacks;
};

void printClassInfo(osgDB::ClassInterface& classMgr, osg::Object* obj)
{
    osgDB::ObjectWrapper* ow = classMgr.getObjectWrapper(obj);
    std::cout << obj->libraryName() << "::" << obj->className() << " information:\n";

    osgDB::ClassInterface::PropertyMap propMap;
    if (classMgr.getSupportedProperties(obj, propMap))
    {
        for (osgDB::ClassInterface::PropertyMap::iterator itr = propMap.begin();
            itr != propMap.end(); ++itr)
        { std::cout << "  " << itr->first << ": " << classMgr.getTypeName(itr->second) << "\n"; }
    }

    const osgDB::ObjectWrapper::MethodObjectMap& methods = ow->getMethodObjectMap();
    const osgDB::ObjectWrapper::RevisionAssociateList& associates = ow->getAssociates();
    osgDB::ObjectWrapperManager* owm = osgDB::Registry::instance()->getObjectWrapperManager();

    for (osgDB::ObjectWrapper::MethodObjectMap::const_iterator mitr = methods.begin();
         mitr != methods.end(); ++mitr) std::cout << "  " << mitr->first << "() .... class method\n";
    for (osgDB::ObjectWrapper::RevisionAssociateList::const_iterator citr = associates.begin();
         citr != associates.end(); ++citr)
    {
        osgDB::ObjectWrapper* owP = owm->findWrapper(citr->_name); if (owP == ow) continue;
        const osgDB::ObjectWrapper::MethodObjectMap& methodsP = owP->getMethodObjectMap();
        for (osgDB::ObjectWrapper::MethodObjectMap::const_iterator mitr = methodsP.begin();
             mitr != methodsP.end(); ++mitr) std::cout << "  " << mitr->first << "() .... parent method\n";
    }
    std::cout << std::endl;
}

int main(int argc, char** argv)
{
    osg::Node* n1 = osgDB::readNodeFile("cessna.osg.0,1,0.trans");
    osg::Node* n2 = osgDB::readNodeFile("cow.osg.2,2,2.scale.0,0,10.trans");

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(n1); root->addChild(n2);

    KeyEventHandler* handler = new KeyEventHandler;

#if false
    // STEP-1: List all classes
    osgDB::ObjectWrapperManager* owm = osgDB::Registry::instance()->getObjectWrapperManager();
    owm->findWrapper("osg::Node");  // preload the serializer library

    osgDB::ObjectWrapperManager::WrapperMap& wrappers = owm->getWrapperMap();
    for (osgDB::ObjectWrapperManager::WrapperMap::iterator itr = wrappers.begin();
         itr != wrappers.end(); ++itr)
    {
        osgDB::ObjectWrapper* wrapper = itr->second.get();
        const osgDB::ObjectWrapper::SerializerList& params = wrapper->getSerializerList();
        const osgDB::ObjectWrapper::MethodObjectMap& methods = wrapper->getMethodObjectMap();
        const osgDB::ObjectWrapper::RevisionAssociateList& associates = wrapper->getAssociates();

        std::cout << "Class " << itr->first << ": ";
        for (size_t i = 0; i < params.size(); ++i)
        {
            osgDB::BaseSerializer* s = params[i].get();
            if (i > 0) std::cout << "; "; std::cout << s->getName();
        }
        std::cout << ";; ";

        for (osgDB::ObjectWrapper::MethodObjectMap::const_iterator mitr = methods.begin();
             mitr != methods.end(); ++mitr) std::cout << mitr->first << "(); ";
        std::cout << "\n";

        for (osgDB::ObjectWrapper::RevisionAssociateList::const_iterator citr = associates.begin();
             citr != associates.end(); ++citr)
        { if (citr != associates.begin()) std::cout << " -> "; std::cout << citr->_name; }
        std::cout << "\n\n";
    }
#elif false
    // STEP-2: Understand class interface
    osgDB::ClassInterface classMgr;
    printClassInfo(classMgr, n1);

    osg::Matrix m1, m2, m3;
    if (classMgr.getProperty(n1, "Matrix", m1)) std::cout << "Node-1: " << m1 << "\n";
    if (classMgr.getProperty(n2, "Matrix", m2)) std::cout << "Node-2: " << m2 << "\n";
    if (classMgr.getProperty(n2->asGroup()->getChild(0), "Matrix", m3))
        std::cout << "Node-2 Scale-Child: " << m3 << "\n";

    bool swapped = false;
    handler->addCallback('t', [&]() {
        swapped = !swapped;  // press 't' to switch between m1 & m2
        classMgr.setProperty(n1, "Matrix", swapped ? m2 : m1);
        classMgr.setProperty(n2, "Matrix", swapped ? m1 : m2);
    });
#elif false
    // STEP-3: Create object and call its methods
    osgDB::ClassInterface classMgr;

    osg::Parameters mIn, mOut; mIn.push_back(n2);
    classMgr.run(root.get(), "removeChild", mIn, mOut);

    osg::Object* vArray = classMgr.createObject("osg::Vec3Array");
    osg::Object* nArray = classMgr.createObject("osg::Vec3Array");
    osg::Object* cArray = classMgr.createObject("osg::Vec4Array");
    osg::Object* drawArrays = classMgr.createObject("osg::DrawArrays");
    osg::Object* geom = classMgr.createObject("osg::Geometry");
    osg::Object* geode = classMgr.createObject("osg::Geode");

    printClassInfo(classMgr, vArray);
    printClassInfo(classMgr, drawArrays);
    printClassInfo(classMgr, geom);
    printClassInfo(classMgr, geode);

    osgDB::BaseSerializer::Type type;
    osgDB::BaseSerializer* bs = NULL; osgDB::VectorBaseSerializer* vs = NULL;

    vs = dynamic_cast<osgDB::VectorBaseSerializer*>(classMgr.getSerializer(vArray, "vector", type));
    if (vs)
    {
        vs->addElement(*vArray, (void*)osg::Vec3(0.0f, 0.0f, 0.0f).ptr());
        vs->addElement(*vArray, (void*)osg::Vec3(5.0f, 0.0f, 0.0f).ptr());
        vs->addElement(*vArray, (void*)osg::Vec3(5.0f, 0.0f, 5.0f).ptr());
        vs->addElement(*vArray, (void*)osg::Vec3(0.0f, 0.0f, 5.0f).ptr());
    }

    vs = dynamic_cast<osgDB::VectorBaseSerializer*>(classMgr.getSerializer(nArray, "vector", type));
    if (vs)
    {
        vs->addElement(*nArray, (void*)osg::Vec3(0.0f, -1.0f, 0.0f).ptr());
        vs->addElement(*nArray, (void*)osg::Vec3(0.0f, -1.0f, 0.0f).ptr());
        vs->addElement(*nArray, (void*)osg::Vec3(0.0f, -1.0f, 0.0f).ptr());
        vs->addElement(*nArray, (void*)osg::Vec3(0.0f, -1.0f, 0.0f).ptr());
    }
    bs = classMgr.getSerializer(nArray, "Binding", type);
    if (bs && bs->getIntLookup())
        classMgr.setProperty(nArray, "Binding", bs->getIntLookup()->getValue("BIND_PER_VERTEX"));

    vs = dynamic_cast<osgDB::VectorBaseSerializer*>(classMgr.getSerializer(cArray, "vector", type));
    if (vs)
    {
        vs->addElement(*cArray, (void*)osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f).ptr());
        vs->addElement(*cArray, (void*)osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f).ptr());
        vs->addElement(*cArray, (void*)osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f).ptr());
        vs->addElement(*cArray, (void*)osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f).ptr());
    }
    bs = classMgr.getSerializer(cArray, "Binding", type);
    if (bs && bs->getIntLookup())
        classMgr.setProperty(cArray, "Binding", bs->getIntLookup()->getValue("BIND_PER_VERTEX"));

    classMgr.setProperty(drawArrays, "Mode", GL_QUADS);
    classMgr.setProperty(drawArrays, "First", 0);
    classMgr.setProperty(drawArrays, "Count", 4);
    
    classMgr.setProperty(geom, "UseDisplayList", false);
    classMgr.setProperty(geom, "UseVertexBufferObjects", true);
    classMgr.setProperty(geom, "VertexArray", vArray);
    classMgr.setProperty(geom, "NormalArray", nArray);
    classMgr.setProperty(geom, "ColorArray", cArray);

    vs = dynamic_cast<osgDB::VectorBaseSerializer*>(
        classMgr.getSerializer(geom, "PrimitiveSetList", type));
    if (vs) vs->addElement(*geom, (void*)&drawArrays);

    mIn.clear(); mIn.push_back(geom);
    classMgr.run(geode, "addDrawable", mIn, mOut);

    mIn.clear(); mIn.push_back(geode);
    classMgr.run(root.get(), "addChild", mIn, mOut);

    bool shown = true;
    handler->addCallback('t', [&]() {
        shown = !shown;  // press 't' to show/hide the quad
        classMgr.setProperty(geode, "NodeMask", shown ? 0xffffffff : 0);
    });
#else
    // osgVerseScript test
    osg::ref_ptr<osgVerse::ScriptBase> scripter = new osgVerse::ScriptBase;

    osgVerse::LibraryEntry* osgLib = scripter->getOrCreateEntry("osg");
    const std::set<std::string>& osgClasses = osgLib->getClasses();
    for (std::set<std::string>::const_iterator itr = osgClasses.begin();
         itr != osgClasses.end(); ++itr)
    {
        std::vector<osgVerse::LibraryEntry::Property> props = osgLib->getPropertyNames(*itr);
        std::vector<osgVerse::LibraryEntry::Method> methods = osgLib->getMethodNames(*itr);
        
        std::cout << "Class " << *itr << ": [PROP] ";
        for (size_t i = 0; i < props.size(); ++i)
        {
            if (props[i].outdated) continue; if (i > 0) std::cout << "; ";
            std::cout << props[i].typeName << " " << props[i].name;
        }

        if (!methods.empty()) std::cout << "\n\t\t [METHOD] ";
        for (size_t i = 0; i < methods.size(); ++i)
        {
            if (methods[i].outdated) continue;
            if (i > 0) std::cout << "; "; std::cout << methods[i].name;
        }
        std::cout << "\n";
    }

    std::string id1 = scripter->createFromObject(n1).value;
    std::string id2 = scripter->createFromObject(n2).value;
    std::cout << "Created objects: " << id1 << ", " << id2 << "\n";

    osgVerse::ScriptBase::Result r1 = scripter->get(id2, "Matrix");
    osgVerse::ScriptBase::Result r2 = scripter->get(id2 + "/0", "Matrix");
    std::cout << "Matrix: " << r1.value << ", " << r2.value << "\n";
#endif

    osgViewer::Viewer viewer;
    viewer.addEventHandler(handler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewInWindow(50, 50, 800, 600);
    return viewer.run();
}
