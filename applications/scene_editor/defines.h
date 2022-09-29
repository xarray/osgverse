#ifndef MANA_DEFINES_HPP
#define MANA_DEFINES_HPP

#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>
#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <ui/ImGui.h>
#include <ui/ImGuiComponents.h>
#include <ui/CommandHandler.h>
#include <pipeline/NodeSelector.h>
#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

struct GlobalData
{
    osg::observer_ptr<osg::Camera> mainCamera;
    osg::observer_ptr<osg::Group> sceneRoot, auxiliaryRoot;
    osg::observer_ptr<osgVerse::NodeSelector> selector;
};
extern GlobalData g_data;

#endif
