#ifndef MANA_PP_USERINPUT_MODULE_HPP
#define MANA_PP_USERINPUT_MODULE_HPP

#include <osg/CullFace>
#include <osg/PolygonOffset>
#include <osg/Texture2DArray>
#include <osg/Geometry>
#include "Pipeline.h"

namespace osgVerse
{
    class UserInputModule : public osg::NodeCallback
    {
    public:
        UserInputModule(const std::string& name, Pipeline* pipeline);
        Pipeline::Stage* createStages(osg::Shader* vs, osg::Shader* fs, unsigned int cullMask,
                                      osg::Texture* colorBuffer = NULL, osg::Texture* depthBuffer = NULL);

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        virtual ~UserInputModule();

        osg::observer_ptr<Pipeline> _pipeline;
    };
}

#endif
