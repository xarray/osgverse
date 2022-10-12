#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "CommandHandler.h"
using namespace osgVerse;
extern void loadDefaultExecutors(CommandHandler* handler);

CommandBuffer* CommandBuffer::instance()
{
    static osg::ref_ptr<CommandBuffer> s_instance = new CommandBuffer;
    return s_instance.get();
}

bool CommandBuffer::canMerge(const std::list<CommandData>& cList, CommandType t, osg::Object* n) const
{
    if (cList.empty()) return false;
    switch (t)
    {
    case TransformCommand: break;  // Transformation value (matrix) can be placed by later one
    default: return false;
    }

    const CommandData& last = cList.back();
    return (last.type == t && last.object == n);
}

void CommandBuffer::mergeCommand(std::list<CommandData>& cList, CommandType t, osg::Object* n,
                                 const linb::any& v0, const linb::any& v1)
{
    // TODO: merging should happen when this command been executed and taken from command buffer,
    //       so that 'undo' will work on an integrated operation instead of many small fractures
    cList.back() = CommandData{ n, v0, v1, t };  // FIXME: consider complex merging?
}

void CommandBuffer::add(CommandType t, osg::Object* n, const linb::any& v0, const linb::any& v1)
{
    _mutex.lock();
    if (t < CommandToUI)
    {
        if (canMerge(_bufferToScene, t, n)) mergeCommand(_bufferToScene, t, n, v0, v1);
        else _bufferToScene.push_back(CommandData{ n, v0, v1, t });
    }
    else
    {
        if (canMerge(_bufferToUI, t, n)) mergeCommand(_bufferToUI, t, n, v0, v1);
        else _bufferToUI.push_back(CommandData{ n, v0, v1, t });
    }
    _mutex.unlock();
}

bool CommandBuffer::take(CommandData& c, bool fromSceneHandler)
{
    bool hasData = false; _mutex.lock();
    if (fromSceneHandler && !_bufferToScene.empty())
    {
        c = _bufferToScene.front(); hasData = true;
        _bufferToScene.pop_front();
    }
    else if (!fromSceneHandler && !_bufferToUI.empty() && _bufferToScene.empty())
    {   // UI events must be taken after all scene events handled...
        c = _bufferToUI.front(); hasData = true;
        _bufferToUI.pop_front();
    }
    _mutex.unlock(); return hasData;
}

CommandHandler::CommandHandler()
{
    loadDefaultExecutors(this);
}

bool CommandHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
    {
        CommandData cmd;
        while (CommandBuffer::instance()->take(cmd, true))
        {
            if (cmd.type == CommandToScene) continue;
            if (_executors.find(cmd.type) == _executors.end())
            {
                OSG_WARN << "[CommandHandler] Unknown command " << cmd.type << std::endl;
                continue;  // no executors for command...
            }
            
            CommandExecutor* executor = getExecutor(cmd.type);
            if (executor && !executor->redo(cmd))
                OSG_WARN << "[CommandHandler] Failed to execute " << cmd.type << std::endl;
        }
    }
    else if (ea.getEventType() == osgGA::GUIEventAdapter::RESIZE)
    {
        CommandBuffer::instance()->add(
            ResizeEditor, NULL, osg::Vec2(ea.getWindowWidth(), ea.getWindowHeight()));
    }
    return false;
}
