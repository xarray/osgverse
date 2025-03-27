#ifndef MANA_PP_USERINPUT_MODULE_HPP
#define MANA_PP_USERINPUT_MODULE_HPP

#include <osg/CullFace>
#include <osg/PolygonOffset>
#include <osg/Texture2DArray>
#include <osg/Geometry>
#include "Pipeline.h"

namespace osgVerse
{
    class UserInputModule : public RenderingModuleBase
    {
    public:
        struct CustomData : public osg::Referenced  // will be set to camera's user-data
        {
            bool sharingBuffers;
            osg::observer_ptr<osg::Camera> bypassCamera;
            CustomData(bool sh) : sharingBuffers(sh) {}
        };

        UserInputModule(const std::string& name, Pipeline* pipeline, int samples = 0);
        virtual UserInputModule* asUserInputModule() { return this; }

        Pipeline::Stage* createStages(osg::Shader* vs, osg::Shader* fs, Pipeline::Stage* bypass, unsigned int cullMask,
                                      const std::string& cName = "ColorBuffer", osg::Texture* colorBuffer = NULL,
                                      const std::string& dName = "DepthBuffer", osg::Texture* depthBuffer = NULL);
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        virtual ~UserInputModule();

        osg::observer_ptr<Pipeline> _pipeline;
        int _coverageSamples;
    };
}

#endif
