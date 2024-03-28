#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osgDB/ReadFile>
#include <iostream>
#include "UserInputModule.h"
#include "Utilities.h"

namespace osgVerse
{
    UserInputModule::UserInputModule(const std::string& name, Pipeline* pipeline)
    :   _pipeline(pipeline)
    {
        setName(name);
        if (pipeline) pipeline->addModule(name, this);
    }

    Pipeline::Stage* UserInputModule::createStages(osg::Shader* vs, osg::Shader* fs, unsigned int cullMask,
                                                   osg::Texture* colorBuffer, osg::Texture* depthBuffer)
    {
        if (colorBuffer != NULL && depthBuffer != NULL)
        {
            Pipeline::BufferDescriptions buffers;
            {
                Pipeline::BufferDescription desc0("ColorBuffer", osgVerse::Pipeline::RGBA_INT8);
#ifdef VERSE_WASM
                Pipeline::BufferDescription desc1("DepthBuffer", osgVerse::Pipeline::DEPTH32);
#else
                Pipeline::BufferDescription desc1("DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);
#endif
                desc0.bufferToShare = colorBuffer; desc1.bufferToShare = depthBuffer;
                buffers.push_back(desc0); buffers.push_back(desc1);
            }
            Pipeline::Stage* stage = _pipeline->addInputStage(getName(), cullMask, 0, vs, fs, buffers);
            return stage;
        }
        else
        {
            Pipeline::Stage* stage = _pipeline->addInputStage(
                getName(), cullMask, 0, vs, fs, 2,
                "ColorBuffer", osgVerse::Pipeline::RGBA_INT8,
#ifdef VERSE_WASM
                "DepthBuffer", osgVerse::Pipeline::DEPTH32);
#else
                "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);
#endif
            // TODO: combine this with pipeline color/depth?
            return stage;
        }
    }

    void UserInputModule::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        traverse(node, nv);
    }
}
