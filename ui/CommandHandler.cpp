#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "CommandHandler.h"
using namespace osgVerse;

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
    else if (!fromSceneHandler && !_bufferToUI.empty())
    {
        c = _bufferToUI.front(); hasData = true;
        _bufferToUI.pop_front();
    }
    _mutex.unlock(); return hasData;
}

/// Executors
struct LoadModelExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        std::string fileName;
        osg::Group* group = static_cast<osg::Group*>(cmd.object.get());
        if (!cmd.get<std::string>(fileName)) return false;

        osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(fileName);
        if (!loadedModel || !group) return false;

        group->addChild(loadedModel.get()); loadedModel->setName(fileName);
        CommandBuffer::instance()->add(RefreshHierarchy, group, loadedModel.get());
        return true;
    }

    virtual bool undo(CommandData& cmd)
    { return false; }  // TODO
};
///

CommandHandler::CommandHandler()
{
    addExecutor(LoadModelCommand, new LoadModelExecutor);
}

bool CommandHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
    {
        CommandData cmd;
        if (CommandBuffer::instance()->take(cmd, true))
        {
            if (cmd.type == CommandToScene) return false;
            if (_executors.find(cmd.type) == _executors.end())
            {
                OSG_WARN << "[CommandHandler] Unknown command " << cmd.type << std::endl;
                return false;
            }
            
            CommandExecutor* executor = getExecutor(cmd.type);
            if (executor && !executor->redo(cmd))
                OSG_WARN << "[CommandHandler] Failed to execute " << cmd.type << std::endl;
        }
    }
    return false;
}
