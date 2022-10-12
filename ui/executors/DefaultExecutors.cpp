#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "pipeline//NodeSelector.h"
#include "pipeline/Utilities.h"
#include "../CommandHandler.h"
using namespace osgVerse;

// SelectCommand: [node]item, [object]node-selector,
//                [int]0-single mode, 1-add sel, 2-remove sel, 3-clear all
struct SelectExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        osgVerse::NodeSelector* selector = NULL;
        osg::Node* n = static_cast<osg::Node*>(cmd.object.get());
        if (!n || !cmd.get(selector)) return false;
        
        int mode = 0; cmd.get(mode, 1);
        switch (mode)
        {
        case 1: selector->addSelectedNode(n); break;
        case 2: selector->removeSelectedNode(n); break;
        case 3: selector->clearAllSelectedNodes(); break;
        default:
            selector->clearAllSelectedNodes();
            selector->addSelectedNode(n); break;
        }
        return true;
    }

    virtual bool undo(CommandData& cmd)
    {
        return false;
    }  // TODO
};

// SetValueCommand: [node/drawable]item, [string]key, [any]value
struct SetValueExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        std::string key; if (!cmd.get(key)) return false;
        if (key == "d_name")
        {
            osg::Drawable* d = static_cast<osg::Drawable*>(cmd.object.get());
            osg::Node* n = static_cast<osg::Node*>(cmd.object.get());
            std::string v; cmd.get(v, 1); if (d) d->setName(v);
        }
        else if (key == "n_name")
        {
            osg::Node* n = static_cast<osg::Node*>(cmd.object.get());
            std::string v; cmd.get(v, 1); if (n) n->setName(v);
        }
        else if (key == "n_mask")
        {
            osg::Node* n = static_cast<osg::Node*>(cmd.object.get());
            unsigned int v = 0xffffffff; cmd.get(v, 1); if (n) n->setNodeMask(v);
        }
        return true;
    }

    virtual bool undo(CommandData& cmd)
    {
        return false;
    }  // TODO
};

// TransformCommand: [node]item, [matrix]transformation, [int]0-mt node, 1-pat node
struct TransformExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        osg::Matrix m; int type = 0;
        osg::Node* n = static_cast<osg::Node*>(cmd.object.get());
        if (!n || !cmd.get(m)) return false; cmd.get(type, 1);

        if (type == 1)
        {
            osg::PositionAttitudeTransform* pat =
                static_cast<osg::PositionAttitudeTransform*>(n);
            osg::Vec3 t, s; osg::Quat r, so; m.decompose(t, r, s, so);
            pat->setPosition(t); pat->setScale(s); pat->setAttitude(r);
        }
        else
        {
            osg::MatrixTransform* mt = static_cast<osg::MatrixTransform*>(n);
            mt->setMatrix(m);
        }
        return true;
    }

    virtual bool undo(CommandData& cmd)
    {
        return false;
    }  // TODO
};

// LoadModelCommand: [node]parent, [string]url
struct LoadModelExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        std::string fileName;
        osg::Group* group = static_cast<osg::Group*>(cmd.object.get());
        if (!cmd.get<std::string>(fileName)) return false;

        osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(fileName);
        if (!loadedModel || !group) return false;

        osgVerse::TangentSpaceVisitor tsv;
        loadedModel->accept(tsv);  // Add tangent/bi-normal arrays for normal mapping

        osg::ref_ptr<osg::MatrixTransform> modelRoot = new osg::MatrixTransform;
        modelRoot->setName(fileName); modelRoot->addChild(loadedModel.get());
        group->addChild(modelRoot.get());
        CommandBuffer::instance()->add(
            RefreshHierarchy, group, (osg::Node*)modelRoot.get());
        return true;
    }

    virtual bool undo(CommandData& cmd)
    {
        return false;
    }  // TODO
};

void loadDefaultExecutors(CommandHandler* handler)
{
    handler->addExecutor(SelectCommand, new SelectExecutor);
    handler->addExecutor(SetValueCommand, new SetValueExecutor);
    handler->addExecutor(TransformCommand, new TransformExecutor);
    handler->addExecutor(LoadModelCommand, new LoadModelExecutor);
}
