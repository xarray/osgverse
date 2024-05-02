#ifndef MANA_DEFINES_HPP
#define MANA_DEFINES_HPP

#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>
#include <osgDB/ConvertUTF>

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <ui/SerializerInterface.h>
#include <ui/CommandHandler.h>
#include <ui/UserComponent.h>
#include <pipeline/SkyBox.h>
#include <pipeline/NodeSelector.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

USE_SERIALIZER_INTERFACE(RW_USER)
USE_SERIALIZER_INTERFACE(RW_OBJECT)
USE_SERIALIZER_INTERFACE(RW_IMAGE)
USE_SERIALIZER_INTERFACE(RW_BOOL)
USE_SERIALIZER_INTERFACE(RW_CHAR)
USE_SERIALIZER_INTERFACE(RW_UCHAR)
USE_SERIALIZER_INTERFACE(RW_SHORT)
USE_SERIALIZER_INTERFACE(RW_USHORT)
USE_SERIALIZER_INTERFACE(RW_INT)
USE_SERIALIZER_INTERFACE(RW_UINT)
USE_SERIALIZER_INTERFACE(RW_FLOAT)
USE_SERIALIZER_INTERFACE(RW_VEC2F)
USE_SERIALIZER_INTERFACE(RW_VEC3F)
USE_SERIALIZER_INTERFACE(RW_VEC4F)
USE_SERIALIZER_INTERFACE(RW_DOUBLE)
USE_SERIALIZER_INTERFACE(RW_VEC2D)
USE_SERIALIZER_INTERFACE(RW_VEC3D)
USE_SERIALIZER_INTERFACE(RW_VEC4D)
USE_SERIALIZER_INTERFACE(RW_QUAT)
USE_SERIALIZER_INTERFACE(RW_PLANE)
USE_SERIALIZER_INTERFACE(RW_MATRIX)
USE_SERIALIZER_INTERFACE(RW_STRING)
USE_SERIALIZER_INTERFACE(RW_ENUM)
USE_SERIALIZER_INTERFACE(RW_GLENUM)
USE_SERIALIZER_INTERFACE(RW_LIST)
#if OSG_VERSION_GREATER_THAN(3, 4, 0)
USE_SERIALIZER_INTERFACE(RW_VEC2B)
USE_SERIALIZER_INTERFACE(RW_VEC2UB)
USE_SERIALIZER_INTERFACE(RW_VEC2S)
USE_SERIALIZER_INTERFACE(RW_VEC2US)
USE_SERIALIZER_INTERFACE(RW_VEC2I)
USE_SERIALIZER_INTERFACE(RW_VEC2UI)
USE_SERIALIZER_INTERFACE(RW_VEC3B)
USE_SERIALIZER_INTERFACE(RW_VEC3UB)
USE_SERIALIZER_INTERFACE(RW_VEC3S)
USE_SERIALIZER_INTERFACE(RW_VEC3US)
USE_SERIALIZER_INTERFACE(RW_VEC3I)
USE_SERIALIZER_INTERFACE(RW_VEC3UI)
USE_SERIALIZER_INTERFACE(RW_VEC4B)
USE_SERIALIZER_INTERFACE(RW_VEC4UB)
USE_SERIALIZER_INTERFACE(RW_VEC4S)
USE_SERIALIZER_INTERFACE(RW_VEC4US)
USE_SERIALIZER_INTERFACE(RW_VEC4I)
USE_SERIALIZER_INTERFACE(RW_VEC4UI)
USE_SERIALIZER_INTERFACE(RW_MATRIXD)
USE_SERIALIZER_INTERFACE(RW_MATRIXF)
USE_SERIALIZER_INTERFACE(RW_BOUNDINGBOXF)
USE_SERIALIZER_INTERFACE(RW_BOUNDINGBOXD)
USE_SERIALIZER_INTERFACE(RW_BOUNDINGSPHEREF)
USE_SERIALIZER_INTERFACE(RW_BOUNDINGSPHERED)
USE_SERIALIZER_INTERFACE(RW_VECTOR)
#endif

#define A2U(s) osgDB::convertStringFromCurrentCodePageToUTF8(s)
#define U2A(s) osgDB::convertStringFromUTF8toCurrentCodePage(s)
#define W2U(s) osgDB::convertUTF16toUTF8(s)
#define U2W(s) osgDB::convertUTF8toUTF16(s)
class Hierarchy;
class Properties;
class SceneLogic;

class EditorContentHandler : public osgVerse::ImGuiContentHandler
{
public:
    EditorContentHandler();
    Hierarchy* getHierarchy() { return _hierarchy.get(); }
    Properties* getProperties() { return _properties.get(); }
    SceneLogic* getSceneLogic() { return _sceneLogic.get(); }
    osgVerse::MainMenuBar* getMainMenu() { return _mainMenu.get(); }

    void handleCommands();
    virtual void runInternal(osgVerse::ImGuiManager* mgr);

protected:
    void createEditorMenu1();
    void createEditorMenu2();
    void createEditorMenu3();

    osg::ref_ptr<osgVerse::MainMenuBar> _mainMenu;
    osg::ref_ptr<Hierarchy> _hierarchy;
    osg::ref_ptr<Properties> _properties;
    osg::ref_ptr<SceneLogic> _sceneLogic;
    unsigned int _uiFrameNumber;
};

struct GlobalData
{
    osg::observer_ptr<osg::Camera> mainCamera;
    osg::observer_ptr<osg::Group> sceneRoot, auxiliaryRoot;
    osg::observer_ptr<osgVerse::NodeSelector> selector;
    osg::observer_ptr<osgVerse::Pipeline> pipeline;
    osg::observer_ptr<osgVerse::ShadowModule> shadow;
    osg::observer_ptr<osgViewer::View> view;
};
extern GlobalData g_data;

#endif
