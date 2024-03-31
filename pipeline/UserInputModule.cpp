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

    UserInputModule::~UserInputModule()
    {}

    Pipeline::Stage* UserInputModule::createStages(unsigned int cullMask, osg::Shader* vs, osg::Shader* fs,
                                                   const std::string& cName, osg::Texture* colorBuffer,
                                                   const std::string& dName, osg::Texture* depthBuffer)
    {
        if (colorBuffer != NULL || depthBuffer != NULL)
        {
            Pipeline::BufferDescriptions buffers;
            {
                Pipeline::BufferDescription desc0(cName, osgVerse::Pipeline::RGBA_INT8);
#ifdef VERSE_WASM
                Pipeline::BufferDescription desc1(dName, osgVerse::Pipeline::DEPTH32);
#else
                Pipeline::BufferDescription desc1(dName, osgVerse::Pipeline::DEPTH24_STENCIL8);
#endif
                if (colorBuffer != NULL) desc0.bufferToShare = colorBuffer;
                if (depthBuffer != NULL) desc1.bufferToShare = depthBuffer;
                buffers.push_back(desc0); buffers.push_back(desc1);
            }

            // Draw on existing buffers, no clear masks
            // This requires single-threaded only!!
            Pipeline::Stage* stage = _pipeline->addInputStage(getName(), cullMask, 0, vs, fs, buffers);
            stage->camera->setClearMask(0); return stage;
        }
        else
        {
            Pipeline::Stage* stage = _pipeline->addInputStage(
                getName(), cullMask, 0, vs, fs, 2, cName.c_str(), osgVerse::Pipeline::RGBA_INT8,
#ifdef VERSE_WASM
                dName.c_str(), osgVerse::Pipeline::DEPTH32);
#else
                dName.c_str(), osgVerse::Pipeline::DEPTH24_STENCIL8);
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
