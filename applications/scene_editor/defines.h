#ifndef MANA_DEFINES_HPP
#define MANA_DEFINES_HPP

#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>
#include <osgDB/ConvertUTF>

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <ui/SceneHierarchy.h>
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

USE_SERIALIZER_INTERFACE(USER)
USE_SERIALIZER_INTERFACE(OBJECT)
USE_SERIALIZER_INTERFACE(IMAGE)
USE_SERIALIZER_INTERFACE(BOOL)
USE_SERIALIZER_INTERFACE(CHAR)
USE_SERIALIZER_INTERFACE(UCHAR)
USE_SERIALIZER_INTERFACE(SHORT)
USE_SERIALIZER_INTERFACE(USHORT)
USE_SERIALIZER_INTERFACE(INT)
USE_SERIALIZER_INTERFACE(UINT)
USE_SERIALIZER_INTERFACE(FLOAT)
USE_SERIALIZER_INTERFACE(VEC2F)
USE_SERIALIZER_INTERFACE(VEC3F)
USE_SERIALIZER_INTERFACE(VEC4F)
USE_SERIALIZER_INTERFACE(DOUBLE)
USE_SERIALIZER_INTERFACE(VEC2D)
USE_SERIALIZER_INTERFACE(VEC3D)
USE_SERIALIZER_INTERFACE(VEC4D)
USE_SERIALIZER_INTERFACE(QUAT)
USE_SERIALIZER_INTERFACE(PLANE)
USE_SERIALIZER_INTERFACE(MATRIX)
USE_SERIALIZER_INTERFACE(STRING)
USE_SERIALIZER_INTERFACE(ENUM)
USE_SERIALIZER_INTERFACE(GLENUM)
USE_SERIALIZER_INTERFACE(LIST)
#if OSG_VERSION_GREATER_THAN(3, 4, 0)
USE_SERIALIZER_INTERFACE(VEC2B)
USE_SERIALIZER_INTERFACE(VEC2UB)
USE_SERIALIZER_INTERFACE(VEC2S)
USE_SERIALIZER_INTERFACE(VEC2US)
USE_SERIALIZER_INTERFACE(VEC2I)
USE_SERIALIZER_INTERFACE(VEC2UI)
USE_SERIALIZER_INTERFACE(VEC3B)
USE_SERIALIZER_INTERFACE(VEC3UB)
USE_SERIALIZER_INTERFACE(VEC3S)
USE_SERIALIZER_INTERFACE(VEC3US)
USE_SERIALIZER_INTERFACE(VEC3I)
USE_SERIALIZER_INTERFACE(VEC3UI)
USE_SERIALIZER_INTERFACE(VEC4B)
USE_SERIALIZER_INTERFACE(VEC4UB)
USE_SERIALIZER_INTERFACE(VEC4S)
USE_SERIALIZER_INTERFACE(VEC4US)
USE_SERIALIZER_INTERFACE(VEC4I)
USE_SERIALIZER_INTERFACE(VEC4UI)
USE_SERIALIZER_INTERFACE(MATRIXD)
USE_SERIALIZER_INTERFACE(MATRIXF)
USE_SERIALIZER_INTERFACE(BOUNDINGBOXF)
USE_SERIALIZER_INTERFACE(BOUNDINGBOXD)
USE_SERIALIZER_INTERFACE(BOUNDINGSPHEREF)
USE_SERIALIZER_INTERFACE(BOUNDINGSPHERED)
USE_SERIALIZER_INTERFACE(VECTOR)
#endif

#define A2U(s) osgDB::convertStringFromCurrentCodePageToUTF8(s)
#define U2A(s) osgDB::convertStringFromUTF8toCurrentCodePage(s)
#define W2U(s) osgDB::convertUTF16toUTF8(s)
#define U2W(s) osgDB::convertUTF8toUTF16(s)
#define TR0(s) osgVerse::ImGuiComponentBase::TR(s)

class EditorContentHandler : public osgVerse::ImGuiContentHandler
{
public:
    EditorContentHandler(osgViewer::View* view, osg::Group* root);
    osgVerse::Window* getHierarchy() { return _hierarchy.get(); }
    osgVerse::Window* getProperties() { return _properties.get(); }
    osgVerse::MainMenuBar* getMainMenu() { return _mainMenu.get(); }

    void handleCommands();
    virtual void runInternal(osgVerse::ImGuiManager* mgr);

protected:
    void createEditorMenu1();
    void createEditorMenu2();
    void createEditorMenu3();

    osg::ref_ptr<osgVerse::MainMenuBar> _mainMenu;
    osg::ref_ptr<osgVerse::Window> _hierarchy, _properties;
    osg::ref_ptr<osgVerse::SceneHierarchy> _hierarchyData;

    osg::ref_ptr<osgVerse::LibraryEntry> _entry;
    std::vector<osg::ref_ptr<osgVerse::SerializerInterface>> _interfaces;
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
