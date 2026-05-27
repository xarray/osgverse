#ifndef MANA_READERWRITER_LOADSCENE_FBX_HPP
#define MANA_READERWRITER_LOADSCENE_FBX_HPP

#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>

#include <animation/PlayerAnimation.h>
#include "3rdparty/ufbx.h"
#include "Export.h"

namespace osgVerse
{
    class LoaderFBX : public osg::Referenced
    {
    public:
        LoaderFBX(std::istream& in, const std::string& d, bool usingPBR = true);

        osg::MatrixTransform* getRoot() { return _root.get(); }
        ufbx_scene* getFbxScene() { return _scene; }

    protected:
        virtual ~LoaderFBX() {}
        void createNode(osg::Group* parent, osg::MatrixTransform* node, ufbx_node* srcNode);
        osg::Node* createMesh(const osg::Matrix& matrix, ufbx_mesh* srcMesh);
        osg::StateSet* createMaterial(ufbx_material* mtl);
        osg::Texture* applyMaterialData(ufbx_material_map* color, ufbx_material_map* factor);

        inline osg::Vec2 toVec2(const ufbx_vec2& v) const { return osg::Vec2(v.x, v.y); }
        inline osg::Vec3 toVec3(const ufbx_vec3& v) const { return osg::Vec3(v.x, v.y, v.z); }
        inline osg::Vec4 toVec4(const ufbx_vec4& v) const { return osg::Vec4(v.x, v.y, v.z, v.w); }
        inline osg::Quat toQuat(const ufbx_quat& v) const { return osg::Vec4(v.x, v.y, v.z, v.w); }
        inline osg::Matrix toMatrix(const ufbx_matrix& m) const
        {
            return osg::Matrix(m.m00, m.m10, m.m20, 0.0,
                               m.m01, m.m11, m.m21, 0.0,
                               m.m02, m.m12, m.m22, 0.0,
                               m.m03, m.m13, m.m23, 1.0);
        }

        inline osg::Vec4 toColorValue(ufbx_material_map& map) const
        {
            switch (map.value_components)
            {
            case 4: return toVec4(map.value_vec4);
            case 3: return osg::Vec4(toVec3(map.value_vec3), 1.0f);
            case 1: return osg::Vec4(map.value_real, map.value_real, map.value_real, 1.0f);
            }
            return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        
        std::map<unsigned int, osg::ref_ptr<osg::Image>> _images;
        std::map<ufbx_material*, osg::ref_ptr<osg::StateSet>> _materials;
        std::map<osg::Transform*, PlayerAnimation::AnimationData> _animations;
        std::map<osg::Transform*, std::pair<int, osg::Vec3d>> _animationStates;
        std::vector<std::pair<int, osg::Matrix>> _boneIndexAndMatrices;

        osg::ref_ptr<osg::MatrixTransform> _root;
        ufbx_scene* _scene;
        std::string _workingDir;
        bool _usingMaterialPBR;
    };

    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadFbx(const std::string& file, int usingPBR);
    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadFbx2(std::istream& in, const std::string& dir, int usingPBR);
}

#endif
