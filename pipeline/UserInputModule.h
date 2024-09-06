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
        UserInputModule(const std::string& name, Pipeline* pipeline);
        virtual UserInputModule* asUserInputModule() { return this; }

        Pipeline::Stage* createStages(unsigned int cullMask, osg::Shader* vs, osg::Shader* fs,
                                      const std::string& cName = "ColorBuffer", osg::Texture* colorBuffer = NULL,
                                      const std::string& dName = "DepthBuffer", osg::Texture* depthBuffer = NULL);
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        virtual ~UserInputModule();

        osg::observer_ptr<Pipeline> _pipeline;
    };
}

#endif
