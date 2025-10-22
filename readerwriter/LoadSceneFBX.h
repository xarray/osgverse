#ifndef MANA_READERWRITER_LOADSCENE_FBX_HPP
#define MANA_READERWRITER_LOADSCENE_FBX_HPP

#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>

#include <animation/PlayerAnimation.h>
#include "3rdparty/ofbx.h"
#include "Export.h"

namespace osgVerse
{
    class LoaderFBX : public osg::Referenced
    {
    public:
        LoaderFBX(std::istream& in, const std::string& d, bool usingPBR = true);

        osg::MatrixTransform* getRoot() { return _root.get(); }
        ofbx::IScene* getFbxScene() { return _scene; }

    protected:
        struct MeshSkinningData
        {
            typedef std::pair<ofbx::Object*, osg::Matrix> ParentAndBindPose;
            std::map<ofbx::Object*, ParentAndBindPose> boneLinks;
            std::map<ofbx::Object*, osg::Matrix> boneInvBindPoses;
            std::map<ofbx::Object*, std::vector<int>> boneIndices;
            std::map<ofbx::Object*, std::vector<double>> boneWeights;
            std::map<int, std::pair<osg::Geometry*, int>> globalIndexMap;
        };

        struct SkinningData
        {
            osg::ref_ptr<PlayerAnimation> player;
            osg::ref_ptr<osg::Node> meshRoot, boneRoot;
            std::map<osg::Geometry*, PlayerAnimation::GeometryJointData> jointData;
            std::vector<osg::Geometry*> meshList;
            std::vector<osg::Transform*> joints;
        };

        virtual ~LoaderFBX() {}
        osg::Geode* createGeometry(const ofbx::Mesh& mesh, const ofbx::GeometryData& gData);
        void createMaterial(const ofbx::Material* mtlData, osg::StateSet* ss);

        void createAnimation(std::vector<SkinningData>& skinningList,
                             const ofbx::AnimationLayer* layer, const ofbx::AnimationCurveNode* curveNode);
        void mergeMeshBones(std::vector<SkinningData>& skinningList);
        void createPlayers(std::vector<SkinningData>& skinningList);

        std::map<osg::Geode*, MeshSkinningData> _meshBoneMap;
        std::map<osg::Transform*, PlayerAnimation::AnimationData> _animations;
        std::map<osg::Transform*, std::pair<int, osg::Vec3d>> _animationStates;
        std::map<const ofbx::Material*, std::vector<osg::Geometry*>> _geometriesByMtl;
        std::map<const ofbx::Texture*, osg::ref_ptr<osg::Texture2D>> _textureMap;
        osg::ref_ptr<osg::MatrixTransform> _root;
        ofbx::IScene* _scene;
        std::string _workingDir;
        bool _usingMaterialPBR;
    };

    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadFbx(const std::string& file, int usingPBR);
    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadFbx2(std::istream& in, const std::string& dir, int usingPBR);
}

#endif
