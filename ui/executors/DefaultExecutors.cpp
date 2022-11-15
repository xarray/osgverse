#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/View>

#include <pipeline/Pipeline.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/NodeSelector.h>
#include <pipeline/Utilities.h>
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

// SetNodeCommand: [node]parent, [node]child, [bool]true-to-delete
struct SetNodeExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        osg::Node* parent = NULL, *child = NULL; cmd.get(child, 0, false);
        bool toDel = false; cmd.get(toDel, 1);
        if (child != NULL)
        {
            osg::Group* n = static_cast<osg::Group*>(cmd.object.get()); parent = n;
            if (!n) return false; if (toDel) n->removeChild(child); else n->addChild(child);
            CommandBuffer::instance()->add(RefreshHierarchy, parent, child, toDel);
        }
        else
        {
            osg::Drawable* childD = NULL; cmd.get(childD, 0, false);
            if (childD != NULL)
            {
                osg::Geode* n = static_cast<osg::Geode*>(cmd.object.get()); parent = n;
                if (!n) return false; if (toDel) n->removeDrawable(childD); else n->addDrawable(childD);
                CommandBuffer::instance()->add(RefreshHierarchy, parent, childD, toDel);
            }
        }
        
        osgVerse::TangentSpaceVisitor tsv;
        if (!toDel) parent->accept(tsv);  // Add tangent/bi-normal arrays for normal mapping
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
        if (key == "d_visibility")
        {
            osg::Drawable* d = static_cast<osg::Drawable*>(cmd.object.get());
            bool v; cmd.get(v, 1); if (d) d->setCullCallback(v ? NULL : _drawableHider.get());
        }
        else if (key == "d_name")
        {
            osg::Drawable* d = static_cast<osg::Drawable*>(cmd.object.get());
            std::string v; cmd.get(v, 1); if (d) d->setName(v);
        }
        else if (key == "n_visibility")
        {
            osg::Node* n = static_cast<osg::Node*>(cmd.object.get());
            bool v; cmd.get(v, 1); if (n) n->setCullCallback(v ? NULL : _nodeHider.get());
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

    SetValueExecutor()
    {
        _nodeHider = new DisableNodeCallback;
        _drawableHider = new DisableDrawableCallback;
    }
    osg::ref_ptr<DisableNodeCallback> _nodeHider;
    osg::ref_ptr<DisableDrawableCallback> _drawableHider;
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

        osgVerse::CommandBuffer::instance()->add(
            osgVerse::RefreshSceneCommand, (osgViewer::View*)NULL, (osgVerse::Pipeline*)NULL, false);
        return true;
    }

    virtual bool undo(CommandData& cmd)
    {
        return false;
    }  // TODO
};

// GoHomeCommand: [view]viewer, [node]item-or-null, [matrix]home-pos
struct GoHomeExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        osgViewer::View* view = dynamic_cast<osgViewer::View*>(cmd.object.get());
        osg::Object* obj = NULL; osg::Matrix m; if (!view) return false;
        cmd.get(obj, 0, false); cmd.get(m, 1, false);

        osgGA::CameraManipulator* mani = view->getCameraManipulator();
        if (mani != NULL)
        {
            // TODO: use home position matrix
            if (!obj)
                mani->home(view->getFrameStamp()->getSimulationTime());
            else
            {
                osg::Node* node = dynamic_cast<osg::Node*>(obj);
                if (!node)
                {
                    osg::Drawable* drawable = dynamic_cast<osg::Drawable*>(obj);
                    if (drawable && drawable->getNumParents() > 0) node = drawable->getParent(0);
                    else return false;  // no valid item selected for going home
                }

                if (node->getNumParents() > 0)
                {
                    osg::ComputeBoundsVisitor cbv; cbv.pushMatrix(node->getParent(0)->getWorldMatrices()[0]);
                    node->accept(cbv); osg::BoundingBox bb = cbv.getBoundingBox();

                    osg::Vec3d eye, center, up, dir;
                    mani->getInverseMatrix().getLookAt(eye, center, up);
                    dir = (center - eye) * bb.radius() * 5.0f; center = bb.center();
                    mani->setByInverseMatrix(osg::Matrix::lookAt(center - dir, center, up));
                }
            }
        }
        return true;
    }

    virtual bool undo(CommandData& cmd)
    {
        return false;
    }  // TODO
};

// RefreshSceneCommand: [view]viewer, [pipeline]pipeline, [bool]to-go-home
struct RefreshSceneExecutor : public CommandHandler::CommandExecutor
{
    virtual bool redo(CommandData& cmd)
    {
        osgViewer::View* view = dynamic_cast<osgViewer::View*>(cmd.object.get());
        osgVerse::Pipeline* pipeline = NULL; cmd.get(pipeline);
        bool goHome = false; cmd.get(goHome, 1);
        if (!view) view = defPiew.get(); else defPiew = view;
        if (!view) return false;

        osg::Node* sceneRoot = view->getSceneData();
        osgGA::CameraManipulator* mani = view->getCameraManipulator();
        if (mani != NULL && goHome) mani->home(view->getFrameStamp()->getSimulationTime());

        if (!pipeline) pipeline = defPipeline.get(); else defPipeline = pipeline;
        return true;
    }

    virtual bool undo(CommandData& cmd)
    {
        return false;
    }  // TODO

    osg::observer_ptr<osgVerse::Pipeline> defPipeline;
    osg::observer_ptr<osgViewer::View> defPiew;
};

void loadDefaultExecutors(CommandHandler* handler)
{
    handler->addExecutor(SelectCommand, new SelectExecutor);
    handler->addExecutor(SetNodeCommand, new SetNodeExecutor);
    handler->addExecutor(SetValueCommand, new SetValueExecutor);
    handler->addExecutor(TransformCommand, new TransformExecutor);
    handler->addExecutor(GoHomeCommand, new GoHomeExecutor);
    handler->addExecutor(RefreshSceneCommand, new RefreshSceneExecutor);
}
