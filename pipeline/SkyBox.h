#ifndef MANA_PP_SKYBOX_HPP
#define MANA_PP_SKYBOX_HPP

#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureCubeMap>
#include <osg/Transform>

namespace osgVerse
{
    class Pipeline;

    /** The skybox node. */
    class SkyBox : public osg::Transform
    {
    public:
        SkyBox(Pipeline* p = NULL);
        SkyBox(const SkyBox& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
        static osg::Camera* createSkyCamera(int renderOrder = 10000);

        void setEnvironmentMap(const std::string& path, const std::string& ext, bool rightHanded = false);
        void setEnvironmentMap(osg::Image* image);
        void setEnvironmentMap(osg::Texture* tex, bool keepYAxisUp = false);

        void setSkyShaders(osg::Shader* vs, osg::Shader* fs);
        void setSkyColor(const osg::Vec4ub& color);
        osg::Texture* getCurrentSkyTexture() const { return _skymap.get(); }
    
        virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
        virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
    
    protected:
        virtual ~SkyBox() {}
        void initialize(bool asCube, const osg::Matrixf& texMat);
    
        osg::observer_ptr<Pipeline> _pipeline;
        osg::observer_ptr<osg::Texture> _skymap;
        osg::ref_ptr<osg::StateSet> _stateset;
        osg::ref_ptr<osg::Shader> _vertex, _fragment;
    };
}

#endif
