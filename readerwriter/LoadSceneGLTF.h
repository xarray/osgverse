#ifndef MANA_READERWRITER_LOADSCENE_GLTF_HPP
#define MANA_READERWRITER_LOADSCENE_GLTF_HPP

#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>

#define TINYGLTF_USE_RAPIDJSON 1
#include "3rdparty/tiny_gltf.h"
#include "animation/PlayerAnimation.h"
#include "Export.h"

namespace osgVerse
{
    class LoaderGLTF : public osg::Referenced
    {
    public:
        // usingPBR: 0 - disabled, 1: enabled, 2: forced
        LoaderGLTF(std::istream& in, const std::string& d, bool isBinary, int usingPBR, bool yUp = true);

        osg::Group* getRoot() { return _root.get(); }
        tinygltf::Model& getModelData() { return _modelDef; }

    protected:
        struct DeferredMeshData
        {
            osg::ref_ptr<osg::Geode> meshRoot;
            tinygltf::Mesh mesh; int skinIndex;
            DeferredMeshData() : skinIndex(-1) {}
            DeferredMeshData(osg::Geode* g, tinygltf::Mesh& m, int i)
                : meshRoot(g), mesh(m), skinIndex(i) {}
        };

        struct SkinningData
        {
            osg::ref_ptr<PlayerAnimation> player;
            osg::ref_ptr<osg::Geode> meshRoot;
            osg::ref_ptr<osg::Group> skeletonRoot;
            std::map<osg::Geometry*, PlayerAnimation::GeometryJointData> jointData;
            std::vector<osg::Geometry*> meshList; std::vector<int> joints;
            int skeletonBaseIndex, invBindPoseAccessor;
            SkinningData() : skeletonBaseIndex(-1), invBindPoseAccessor(-1) {}
        };

        virtual ~LoaderGLTF() {}
        osg::Node* createNode(int id, tinygltf::Node& node);
        osg::Texture* createTexture(const std::string& name, tinygltf::Texture& tex);
        osg::ref_ptr<osg::Geometry> createFromExtGaussianSplattingSPZ2(const std::string& name, int bufferViewID);

        bool createMesh(osg::Geode* geode, tinygltf::Mesh& mesh, int skinIndex);
        void createMaterial(osg::StateSet* ss, tinygltf::Material mat);
        void createInvBindMatrices(SkinningData& sd, const std::vector<osg::Transform*>& bones,
                                   tinygltf::Accessor& accessor);
        void createAnimationSampler(PlayerAnimation::AnimationData& anim, const std::string& p,
                                    tinygltf::Accessor& in, tinygltf::Accessor& out);
        void createBlendshapeData(osg::Geometry* geom, std::map<std::string, int>& target);
        void applyBlendshapeWeights(osg::Geode* geode, const std::vector<double>& weights,
                                    const tinygltf::Value& targetNames);

        inline void copyBufferData(void* dst, const void* src, size_t size,
                                   size_t stride, size_t count)
        {
            if (stride > 0 && count > 0)
            {
                size_t elemSize = size / count;
                for (size_t i = 0; i < count; ++i)
                    memcpy((char*)dst + i * elemSize, (const char*)src + i * stride, elemSize);
            }
            else
                memcpy(dst, src, size);
        }

        std::map<std::pair<int, int>, osg::observer_ptr<osg::Image>> _ormImageMap;
        std::map<int, osg::observer_ptr<osg::Image>> _imageMap;
        std::map<int, osg::Node*> _nodeCreationMap;
        std::vector<DeferredMeshData> _deferredMeshList;
        std::vector<SkinningData> _skinningDataList;
        osg::ref_ptr<osg::MatrixTransform> _root;
        tinygltf::Model _modelDef;
        std::string _workingDir;
        int _usingMaterialPBR; bool _3dtilesFormat;
    };

    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadGltf(
        const std::string& file, bool isBinary, int usingPBR, bool yUp = true);
    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadGltf2(
        std::istream& in, const std::string& dir, bool isBinary, int usingPBR, bool yUp = true);
}

#endif
