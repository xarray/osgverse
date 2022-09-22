#ifndef MANA_PP_SKYBOX_HPP
#define MANA_PP_SKYBOX_HPP

#include <osg/Geometry>
#include <osg/TexMat>
#include <osg/TextureCubeMap>
#include <osg/Transform>

namespace osgVerse
{

/** The skybox node. */
class SkyBox : public osg::Transform
{
public:
    SkyBox();
    SkyBox(const SkyBox& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

    void setEnvironmentMap(const std::string& path, const std::string& ext = "jpg");
    void setEnvironmentMap(osg::Image* image);
    osg::TextureCubeMap* getCurrentSkyTexture() const { return _skymap.get(); }
    
    virtual bool computeLocalToWorldMatrix( osg::Matrix& matrix, osg::NodeVisitor* nv ) const;
    virtual bool computeWorldToLocalMatrix( osg::Matrix& matrix, osg::NodeVisitor* nv ) const;
    
protected:
    virtual ~SkyBox() {}
    void initialize();
    
    osg::observer_ptr<osg::TextureCubeMap> _skymap;
};

}

#endif
