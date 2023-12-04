#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>

#include <ofbx.h>
#include <animation/PlayerAnimation.h>
#include "Export.h"

namespace osgVerse
{
    class LoaderFBX : public osg::Referenced
    {
    public:
        LoaderFBX(std::istream& in, const std::string& d);

        osg::MatrixTransform* getRoot() { return _root.get(); }
        ofbx::IScene* getFbxScene() { return _scene; }

    protected:
        virtual ~LoaderFBX() {}
        osg::Geode* createGeometry(const ofbx::Mesh& mesh, const ofbx::Geometry& gData);
        void createAnimation(const ofbx::AnimationCurveNode* curveNode);
        void createMaterial(const ofbx::Material* mtlData, osg::StateSet* ss);

        struct MeshSkinningData
        {
            typedef std::pair<ofbx::Object*, osg::Matrix> ParentAndBindPose;
            std::map<ofbx::Object*, ParentAndBindPose> boneLinks;
        };

        std::map<osg::Geode*, MeshSkinningData> _meshBoneMap;
        std::map<const ofbx::Material*, std::vector<osg::Geometry*>> _geometriesByMtl;
        std::map<const ofbx::Texture*, osg::ref_ptr<osg::Texture2D>> _textureMap;
        osg::ref_ptr<osg::MatrixTransform> _root;
        ofbx::IScene* _scene;
        std::string _workingDir;
    };

    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadFbx(const std::string& file);
    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadFbx2(std::istream& in, const std::string& dir);
}
