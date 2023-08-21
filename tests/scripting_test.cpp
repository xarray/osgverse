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
#include <script/JsonScript.h>
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

    // osgVerseScript test
    osg::ref_ptr<osgVerse::JsonScript> scripter = new osgVerse::JsonScript;
    scripter->setRootNode(root.get());

    std::string id1 = scripter->createFromObject(n1).value;
    std::string id2 = scripter->createFromObject(n2).value;
    std::cout << "Created objects: " << id1 << ", " << id2 << "\n";

    picojson::value exe, ret1, ret2, ret3, ret4, ret5;
    std::string s1 = "{\"object\": \"" + id1 + "\", \"property\": \"Matrix\"}";
    std::string s2 = "{\"object\": \"" + id2 + "/0\", \"property\": \"Matrix\"}";
    std::string s3 = "{\"object\": \"" + id1 + "\", \"properties\": "
                      "{\"Matrix\": \"1 0 0 0 0 1 0 0 0 0 1 0 20 1 0 1\"}}";
    std::string s4 = "{\"class\": \"MatrixTransform\"}";
    std::string s5 = "{\"object\": \"" + id1 + "\"}";

    picojson::parse(exe, s1); ret1 = scripter->execute(osgVerse::JsonScript::EXE_Get, exe);
    picojson::parse(exe, s2); ret2 = scripter->execute(osgVerse::JsonScript::EXE_Get, exe);
    picojson::parse(exe, s3); ret3 = scripter->execute(osgVerse::JsonScript::EXE_Set, exe);
    picojson::parse(exe, s4); ret4 = scripter->execute(osgVerse::JsonScript::EXE_List, exe);
    picojson::parse(exe, s5); ret5 = scripter->execute(osgVerse::JsonScript::EXE_Get, exe);
    std::cout << "Exe1: " << ret1.serialize(true);
    std::cout << "Exe2: " << ret2.serialize(true);
    std::cout << "Exe3: " << ret3.serialize(true);
    std::cout << "Exe4: " << ret4.serialize(true);
    std::cout << "Exe5 (FAILED): " << ret5.serialize(true);

    KeyEventHandler* handler = new KeyEventHandler;
    handler->addCallback('t', [&]() {
        s1 = "{\"class\": \"osg::MatrixTransform\"}";
        s2 = "{\"type\": \"node\", \"uri\": \"dumptruck.osgt\"}";
        picojson::parse(exe, s1); ret1 = scripter->execute(osgVerse::JsonScript::EXE_Creation, exe);
        picojson::parse(exe, s2); ret2 = scripter->execute(osgVerse::JsonScript::EXE_Creation, exe);
        std::cout << "Exe6: " << ret1.serialize(true);
        std::cout << "Exe7: " << ret2.serialize(true);

        s3 = "{\"object\": \"root\", \"method\": \"addChild\", \"properties\": [\""
            + ret1.get("object").to_str() + "\"]}";
        s4 = "{\"object\": \"" + ret1.get("object").to_str() + "\", \"method\": \"addChild\", "
            + "\"properties\": [\"" + ret2.get("object").to_str() + "\"]}";
        s5 = "{\"object\": \"" + ret1.get("object").to_str() + "\", \"properties\": "
             "{\"Matrix\": \"1 0 0 0 0 1 0 0 0 0 1 0 -30 0 0 1\"}}";
        picojson::parse(exe, s3); ret3 = scripter->execute(osgVerse::JsonScript::EXE_Set, exe);
        picojson::parse(exe, s4); ret4 = scripter->execute(osgVerse::JsonScript::EXE_Set, exe);
        picojson::parse(exe, s5); ret5 = scripter->execute(osgVerse::JsonScript::EXE_Set, exe);

        std::cout << "Exe8: " << ret3.serialize(true);
        std::cout << "Exe9: " << ret4.serialize(true);
        std::cout << "Exe10: " << ret5.serialize(true);
    });

    osgViewer::Viewer viewer;
    viewer.addEventHandler(handler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewInWindow(50, 50, 800, 600);
    return viewer.run();
}
