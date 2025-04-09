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
#include <pipeline/Utilities.h>
#include <script/JsonScript.h>
#include <script/PythonScript.h>
#include <wrappers/Export.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

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
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    osgViewer::Viewer viewer;
    viewer.setSceneData(root.get());

    osg::ref_ptr<osgVerse::PythonScript> scripter0 = new osgVerse::PythonScript;
    scripter0->setRootNode(root.get());
    osg::ref_ptr<osgVerse::JsonScript> scripter1 = new osgVerse::JsonScript;
    scripter1->setRootNode(root.get());
    if (arguments.read("--python"))
    {
        // TODO
        std::cout << "PYTHON...\n";
    }
    else
    {
        osg::Node* n1 = osgDB::readNodeFile("cessna.osg.0,1,0.trans");
        osg::Node* n2 = osgDB::readNodeFile("cow.osg.2,2,2.scale.0,0,10.trans");
        root->addChild(n1); root->addChild(n2);

        // osgVerseScript test
        std::string id1 = scripter1->createFromObject(n1).value;
        std::string id2 = scripter1->createFromObject(n2).value;
        std::cout << "Created objects: " << id1 << ", " << id2 << "\n";

        picojson::value exe, ret1, ret2, ret3, ret4, ret5;
        std::string s1 = "{\"object\": \"" + id1 + "\", \"property\": \"Matrix\"}";
        std::string s2 = "{\"object\": \"" + id2 + "/0\", \"property\": \"Matrix\"}";
        std::string s3 = "{\"object\": \"" + id1 + "\", \"properties\": "
                          "{\"Matrix\": \"1 0 0 0 0 1 0 0 0 0 1 0 20 1 0 1\"}}";
        std::string s4 = "{\"class\": \"Vec3Array\"}";
        std::string s5 = "{\"object\": \"" + id1 + "\"}";

        picojson::parse(exe, s1); ret1 = scripter1->execute(osgVerse::JsonScript::EXE_Get, exe);
        picojson::parse(exe, s2); ret2 = scripter1->execute(osgVerse::JsonScript::EXE_Get, exe);
        picojson::parse(exe, s3); ret3 = scripter1->execute(osgVerse::JsonScript::EXE_Set, exe);
        picojson::parse(exe, s4); ret4 = scripter1->execute(osgVerse::JsonScript::EXE_List, exe);
        picojson::parse(exe, s5); ret5 = scripter1->execute(osgVerse::JsonScript::EXE_Get, exe);
        std::cout << "Exe1 (Get property): " << ret1.serialize(true);
        std::cout << "Exe2 (Get child property): " << ret2.serialize(true);
        std::cout << "Exe3 (Set property): " << ret3.serialize(true);
        std::cout << "Exe4 (List properties): " << ret4.serialize(true);
        std::cout << "Exe5 (Invalid command): " << ret5.serialize(true);

        std::string s6_1 = "{\"class\": \"osg::Vec3Array\", \"properties\": [";
                           "{\"vector\": \"0 0 0, 10 0 0, 10 10 0, 0 10 0\"}]}";
        std::string s6_2 = "{\"class\": \"osg::Vec3Array\", \"properties\": [";
                           "{\"vector\": \"0 0 1, 0 0 1, 0 0 1, 0 0 1\"}]}";
        std::string s6_3 = "{\"class\": \"osg::Vec4Array\", \"properties\": [";
                           "{\"vector\": \"1 0 0 1, 1 1 0 1, 0 1 0 1, 0 0 1 1\"}]}";
        std::string s6_4 = "{\"class\": \"osg::DrawArrays\", \"properties\": "
                            "[{\"First\": 0}, {\"Count\": 4}]}";
        picojson::parse(exe, s6_1); ret1 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
        picojson::parse(exe, s6_2); ret2 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
        picojson::parse(exe, s6_3); ret3 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
        picojson::parse(exe, s6_4); ret4 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);

        std::string s7 = "{\"class\": \"osg::Geometry\", \"properties\": ["
                             "{\"UseDisplayList\": false},"
                             "{\"UseVertexBufferObjects\": true},"
                             "{\"VertexArray\": \"" + ret1.get("object").to_str() + "\"},"
                             "{\"NormalArray\": \"" + ret2.get("object").to_str() + "\"},"
                             "{\"ColorArray\": \"" + ret3.get("object").to_str() + "\"},"
                             "{\"PrimitiveSetList\": \"" + ret4.get("object").to_str() + "\"}"
                         "]}";
        std::string s8 = "{\"class\": \"osg::Geode\"}";
        picojson::parse(exe, s7); ret5 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
        picojson::parse(exe, s8); ret1 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);

        std::string s9 = "{\"object\": \"root\", \"method\": \"addChild\", \"properties\": "
                          "[\"" + ret1.get("object").to_str() + "\"]}";
        std::string s10 = "{\"object\": \"" + ret1.get("object").to_str() + "\", \"method\": \"addDrawable\", "
                          "\"properties\": [\"" + ret5.get("object").to_str() + "\"]}";
        picojson::parse(exe, s9); ret2 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
        picojson::parse(exe, s10); ret3 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
        std::cout << "Exe6 (Create geometry): " << ret5.serialize(true);
        std::cout << "Exe7 (Create geode): " << ret1.serialize(true);
        std::cout << "Exe8 (Add geode to root): " << ret2.serialize(true);
        std::cout << "Exe9 (Add geometry to geode): " << ret3.serialize(true);

        osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
        handler->addKeyUpCallback('t', [&](int key) {
            s1 = "{\"class\": \"osg::MatrixTransform\"}";
            s2 = "{\"type\": \"node\", \"uri\": \"dumptruck.osgt\"}";
            picojson::parse(exe, s1); ret1 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
            picojson::parse(exe, s2); ret2 = scripter1->execute(osgVerse::JsonScript::EXE_Creation, exe);
            std::cout << "Exe10 (Create transform node): " << ret1.serialize(true);
            std::cout << "Exe11 (Load model): " << ret2.serialize(true);

            s3 = "{\"object\": \"root\", \"method\": \"addChild\", \"properties\": [\""
                + ret1.get("object").to_str() + "\"]}";
            s4 = "{\"object\": \"" + ret1.get("object").to_str() + "\", \"method\": \"addChild\", "
                + "\"properties\": [\"" + ret2.get("object").to_str() + "\"]}";
            s5 = "{\"object\": \"" + ret1.get("object").to_str() + "\", \"properties\": "
                 "{\"Matrix\": \"1 0 0 0 0 1 0 0 0 0 1 0 -30 0 0 1\"}}";
            picojson::parse(exe, s3); ret3 = scripter1->execute(osgVerse::JsonScript::EXE_Set, exe);
            picojson::parse(exe, s4); ret4 = scripter1->execute(osgVerse::JsonScript::EXE_Set, exe);
            picojson::parse(exe, s5); ret5 = scripter1->execute(osgVerse::JsonScript::EXE_Set, exe);

            std::cout << "Exe12 (Add transform to root): " << ret3.serialize(true);
            std::cout << "Exe13 (Add model to transform): " << ret4.serialize(true);
            std::cout << "Exe14 (Set matrix): " << ret5.serialize(true);
        });
        viewer.addEventHandler(handler);
    }

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setUpViewInWindow(50, 50, 800, 600);
    return viewer.run();
}
