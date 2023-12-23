#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/SmoothingVisitor>
#include "pipeline/Utilities.h"
#include "LoadSceneFBX.h"

namespace osgVerse
{
    LoaderFBX::LoaderFBX(std::istream& in, const std::string& d)
        : _scene(NULL), _workingDir(d + "/")
    {
        std::istreambuf_iterator<char> eos;
        std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
        if (data.empty()) { OSG_WARN << "[LoaderFBX] Unable to read from stream\n"; return; }

        _scene = ofbx::load((ofbx::u8*)&data[0], data.size(), (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
        if (!_scene) { OSG_WARN << "[LoaderFBX] Unable to parse FBX scene\n"; return; }

        //const ofbx::Object* const* objects = _scene->getAllObjects();
        _root = new osg::MatrixTransform;
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
        _root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
#endif

        int meshCount = _scene->getMeshCount();
        for (int i = 0; i < meshCount; ++i)
        {
            const ofbx::Mesh& mesh = *_scene->getMesh(i);
            osg::ref_ptr<osg::Geode> geode;

            std::string meshName(mesh.name);
            if (meshName.empty())
            {
                ofbx::Object* meshParent = mesh.getParent();
                if (meshParent != NULL) meshName = meshParent->name;
                meshName += "_Mesh" + std::to_string(i);
            }

            osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
            mt->setMatrix(osg::Matrix(mesh.getGeometricMatrix().m) *
                osg::Matrix(mesh.getGlobalTransform().m));
            mt->setName(meshName);
            _root->addChild(mt.get());

            const ofbx::Pose* pData = mesh.getPose();
            if (pData != NULL)
            {
                //OSG_NOTICE << "[LoaderFBX] <POSE> " << pData->name << " not implemented\n";
            }

            const ofbx::Geometry* gData = mesh.getGeometry();
            if (gData != NULL)
            {
                geode = createGeometry(mesh, *gData);
                geode->setName(meshName + "_Geode");

                osg::ref_ptr<osg::MatrixTransform> mtSelf = new osg::MatrixTransform;
                mtSelf->addChild(geode.get());
                mtSelf->setName(meshName + "_SelfTransform");
                mt->addChild(mtSelf.get());
            }
        }

        // Apply materials to geometries
        for (std::map<const ofbx::Material*, std::vector<osg::Geometry*>>::iterator
            itr = _geometriesByMtl.begin(); itr != _geometriesByMtl.end(); ++itr)
        {
            std::vector<osg::Geometry*>& geometries = itr->second;
            for (size_t i = 0; i < geometries.size(); ++i)
                createMaterial(itr->first, geometries[i]->getOrCreateStateSet());
        }

        // Merge and configure skeleton and skinning data
        std::vector<SkinningData> skinningList;
        mergeMeshBones(skinningList); createPlayers(skinningList);

        // Get and apply animations
        int animCount = _scene->getAnimationStackCount();
        for (int i = 0; i < animCount; ++i)
        {
            const ofbx::AnimationStack* animStack = _scene->getAnimationStack(i);
            const ofbx::TakeInfo* takeInfo = _scene->getTakeInfo(animStack->name);
            for (int j = 0; animStack->getLayer(j); ++j)
            {
                const ofbx::AnimationLayer* layer = animStack->getLayer(j);
                for (int k = 0; layer->getCurveNode(k); ++k)
                {
                    const ofbx::AnimationCurveNode* curveNode = layer->getCurveNode(k);
                    if (curveNode->getCurve(0)) createAnimation(curveNode);  // FIXME: rigid animation only
                }
            }
            // TODO: more than 1 animations?
        }
    }

    osg::Geode* LoaderFBX::createGeometry(const ofbx::Mesh& mesh, const ofbx::Geometry& gData)
    {
        int vCount = gData.getVertexCount(), iCount = gData.getIndexCount();
        const ofbx::Vec3* vData = gData.getVertices();
        const ofbx::Vec3* nData = gData.getNormals();
        const ofbx::Vec3* tData = gData.getTangents();
        const ofbx::Vec4* cData = gData.getColors();
        const ofbx::Vec2* uvData0 = gData.getUVs(0);
        const ofbx::Vec2* uvData1 = gData.getUVs(1);
        const int* iData = gData.getFaceIndices();
        const int* mData = gData.getMaterials();

        if (vCount <= 0 || iCount <= 0) return NULL;
        if (vCount != iCount)
            OSG_WARN << "[LoaderFBX] Unknown geometry layout: " << vCount << " / " << iCount << "\n";

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(vCount);
        osg::ref_ptr<osg::Vec3Array> na = nData ? new osg::Vec3Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec4Array> ta = tData ? new osg::Vec4Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec4Array> ca = cData ? new osg::Vec4Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec2Array> uv0 = uvData0 ? new osg::Vec2Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec2Array> uv1 = uvData1 ? new osg::Vec2Array(vCount) : NULL;
        for (int i = 0; i < vCount; ++i)
        {
            (*va)[i] = osg::Vec3(vData[i].x, vData[i].y, vData[i].z);
            if (nData) (*na)[i] = osg::Vec3(nData[i].x, nData[i].y, nData[i].z);
            if (tData) (*ta)[i] = osg::Vec4(tData[i].x, tData[i].y, tData[i].z, 1.0f);
            if (cData) (*ca)[i] = osg::Vec4(cData[i].x, cData[i].y, cData[i].z, cData[i].w);
            if (uvData0) (*uv0)[i] = osg::Vec2(uvData0[i].x, uvData0[i].y);
            if (uvData1) (*uv1)[i] = osg::Vec2(uvData1[i].x, uvData1[i].y);
        }

        std::map<int, osg::ref_ptr<osg::DrawElementsUInt>> primitivesByMtl;
        if (mData != NULL)
        {
            for (int i = 0; i < iCount; i += 3)
            {
                int mtlIndex = mData[i / 3], index = 0;
                osg::ref_ptr<osg::DrawElementsUInt>& de = primitivesByMtl[mtlIndex];
                if (!de) de = new osg::DrawElementsUInt(GL_TRIANGLES);

                index = iData[i + 0]; de->push_back(index < 0 ? ((-index) - 1) : index);
                index = iData[i + 1]; de->push_back(index < 0 ? ((-index) - 1) : index);
                index = iData[i + 2]; de->push_back(index < 0 ? ((-index) - 1) : index);
            }
        }
        else
        {
            osg::ref_ptr<osg::DrawElementsUInt>& de = primitivesByMtl[0];
            de = new osg::DrawElementsUInt(GL_TRIANGLES);
            for (int i = 0; i < iCount; ++i)
            {
                int index = iData[i];
                de->push_back(index < 0 ? ((-index) - 1) : index);
            }
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        std::map<int, std::pair<osg::Geometry*, int>> globalIndexMap;
        for (std::map<int, osg::ref_ptr<osg::DrawElementsUInt>>::iterator itr = primitivesByMtl.begin();
             itr != primitivesByMtl.end(); ++itr)
        {
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            osg::ref_ptr<osg::Vec3Array> subVA = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> subNA = nData ? new osg::Vec3Array : NULL;
            osg::ref_ptr<osg::Vec4Array> subTA = tData ? new osg::Vec4Array : NULL;
            osg::ref_ptr<osg::Vec4Array> subCA = cData ? new osg::Vec4Array : NULL;
            osg::ref_ptr<osg::Vec2Array> subUV0 = uvData0 ? new osg::Vec2Array : NULL;
            osg::ref_ptr<osg::Vec2Array> subUV1 = uvData1 ? new osg::Vec2Array : NULL;

            osg::DrawElementsUInt* de = itr->second.get();
            std::map<unsigned int, unsigned int> globalToLocalMap;
            for (size_t i = 0; i < de->size(); ++i)
            {
                unsigned int idx = (*de)[i];
                if (globalToLocalMap.find(idx) == globalToLocalMap.end())
                {
                    globalToLocalMap[idx] = subVA->size();
                    if (globalIndexMap.find(idx) != globalIndexMap.end())
                    {
                        OSG_NOTICE << "[LoaderFBX] global vertex index (in an FBX mesh) " << idx
                                   << " seems to be reused by multiple geometries" << std::endl;
                    }
                    globalIndexMap[idx] = std::pair<osg::Geometry*, int>(geom.get(), subVA->size());

                    subVA->push_back((*va)[idx]); if (nData) subNA->push_back((*na)[idx]);
                    if (tData) subTA->push_back((*ta)[idx]); if (cData) subCA->push_back((*ca)[idx]);
                    if (uvData0) subUV0->push_back((*uv0)[idx]);
                    if (uvData1) subUV1->push_back((*uv1)[idx]);
                }
                (*de)[i] = globalToLocalMap[idx];
            }

            geom->setVertexArray(subVA.get());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
            if (nData) geom->setNormalArray(subNA.get(), osg::Array::BIND_PER_VERTEX);
            if (tData) geom->setVertexAttribArray(6, subTA.get(), osg::Array::BIND_PER_VERTEX);
            if (cData) geom->setColorArray(subCA.get(), osg::Array::BIND_PER_VERTEX);
            if (uvData0) geom->setTexCoordArray(0, subUV0.get(), osg::Array::BIND_PER_VERTEX);
            if (uvData1) geom->setTexCoordArray(1, subUV1.get(), osg::Array::BIND_PER_VERTEX);
#else
            if (nData) { geom->setNormalArray(subNA.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (tData) { geom->setVertexAttribArray(6, subTA.get()); geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX); }
            if (cData) { geom->setColorArray(subCA.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (uvData0) { geom->setTexCoordArray(0, subUV0.get()); }
            if (uvData1) { geom->setTexCoordArray(1, subUV1.get()); }
#endif
            geom->addPrimitiveSet(de);
            geode->addDrawable(geom.get());
            if (!nData) osgUtil::SmoothingVisitor::smooth(*geom);

            if (itr->first < mesh.getMaterialCount())
            {
                const ofbx::Material* mtl = mesh.getMaterial(itr->first);
                _geometriesByMtl[mtl].push_back(geom.get());
            }
            else
                OSG_NOTICE << "[LoaderFBX] No material on this geometry\n";
        }

        const ofbx::Skin* skin = gData.getSkin();
        if (skin != NULL)
        {
            MeshSkinningData& msd = _meshBoneMap[geode.get()];
            msd.globalIndexMap = globalIndexMap;
            for (int i = 0; i < skin->getClusterCount(); ++i)
            {
                const ofbx::Cluster* cluster = skin->getCluster(i);
                ofbx::Object* boneNode = const_cast<ofbx::Object*>(cluster->getLink());

                if (boneNode->getParent())
                {
                    MeshSkinningData::ParentAndBindPose parentAndPose(
                        boneNode->getParent(), osg::Matrix(cluster->getTransformLinkMatrix().m));
                    msd.boneLinks[boneNode] = parentAndPose;
                }

                std::vector<int>& boneIndices = msd.boneIndices[boneNode];
                std::vector<double>& boneWeights = msd.boneWeights[boneNode];
                boneIndices.resize(cluster->getIndicesCount());
                boneWeights.resize(cluster->getWeightsCount());
                memcpy(&boneIndices[0], cluster->getIndices(), cluster->getIndicesCount());
                memcpy(&boneWeights[0], cluster->getWeights(), cluster->getWeightsCount());
            }
        }

        const ofbx::BlendShape* bs = gData.getBlendShape();
        if (bs != NULL)
        {
            OSG_NOTICE << "[LoaderFBX] <BLENDSHAPE> " << bs->name << " not implemented\n";
            MeshSkinningData& msd = _meshBoneMap[geode.get()];
            // TODO
        }
        return geode.release();
    }

    void LoaderFBX::createAnimation(const ofbx::AnimationCurveNode* curveNode)
    {
        const ofbx::Object* bone = curveNode->getBone();
        ofbx::Vec3 pivot = bone->getRotationPivot();
        ofbx::RotationOrder order = bone->getRotationOrder();  // FIXME: always xyz...

        osg::observer_ptr<osg::MatrixTransform> mt; std::string boneName(bone->name);
        for (unsigned int i = 0; i < _root->getNumChildren(); ++i)
        {
            osg::MatrixTransform* child = dynamic_cast<osg::MatrixTransform*>(_root->getChild(i));
            if (child && child->getNumChildren() > 0 && child->getName() == boneName)
            {
                osg::MatrixTransform* childSelf = dynamic_cast<osg::MatrixTransform*>(child->getChild(0));
                if (childSelf) { mt = childSelf; break; }
            }
        }

        std::string propertyName = curveNode->name;
        osg::Vec3d pivotOffset(pivot.x, pivot.y, pivot.z);
        if (!mt) { OSG_NOTICE << "[LoaderFBX] Unable to find animation " << boneName << std::endl; return; }

        osg::ref_ptr<osg::AnimationPath> animationPath;
        osg::observer_ptr<osg::AnimationPathCallback> apcb;
        apcb = dynamic_cast<osg::AnimationPathCallback*>(mt->getUpdateCallback());
        if (apcb.valid())
            animationPath = apcb->getAnimationPath();
        else
        {
            animationPath = new osg::AnimationPath;
            animationPath->setLoopMode(osg::AnimationPath::LOOP);
            apcb = new osg::AnimationPathCallback(animationPath.get());
            apcb->setPivotPoint(pivotOffset);
            mt->setUpdateCallback(apcb.get());
        }

        int type = -1;  // 0: T, 1: R, 2: S
        if (propertyName == "T") type = 0;
        else if (propertyName == "R") type = 1;
        else if (propertyName == "S") type = 2;
        else
        {
            OSG_NOTICE << "[LoaderFBX] Unsupported animation property <" << propertyName
                       << "> on node " << boneName << std::endl;
            return;
        }

        osg::AnimationPath::TimeControlPointMap& cpMap = animationPath->getTimeControlPointMap();
        std::map<float, osg::Vec3d> tempPositionMap, tempEulerMap;
        for (int n = 0; n < 3 && curveNode->getCurve(n); ++n)
        {
            const ofbx::AnimationCurve* curve = curveNode->getCurve(n);
            const ofbx::i64* timePtr = curve->getKeyTime();
            const float* valuePtr = curve->getKeyValue();

            for (int k = 0; k < curve->getKeyCount(); ++k)
            {
                float t = (float)ofbx::fbxTimeToSeconds(timePtr[k]), v = valuePtr[k];
                osg::AnimationPath::ControlPoint& cp = cpMap[t];
                switch (type)
                {
                case 0: tempPositionMap[t][n] = v; break;
                case 1: tempEulerMap[t][n] = osg::inDegrees(v); break;
                case 2: { osg::Vec3d p = cp.getScale(); p[n] = v; cp.setScale(p); break; }
                }
            }
        }

        for (std::map<float, osg::Vec3d>::iterator itr = tempPositionMap.begin();
            itr != tempPositionMap.end(); ++itr)
        {
            cpMap[itr->first].setPosition(itr->second + pivotOffset);
        }

        for (std::map<float, osg::Vec3d>::iterator itr = tempEulerMap.begin();
            itr != tempEulerMap.end(); ++itr)
        {
            osg::Matrix m = osg::Matrix::rotate(itr->second[0], osg::X_AXIS)
                * osg::Matrix::rotate(itr->second[1], osg::Y_AXIS)
                * osg::Matrix::rotate(itr->second[2], osg::Z_AXIS);
            cpMap[itr->first].setRotation(m.getRotate());
        }
    }

    void LoaderFBX::createMaterial(const ofbx::Material* mtlData, osg::StateSet* ss)
    {
        for (int i = 0; i < ofbx::Texture::COUNT; ++i)
        {
            const ofbx::Texture* tData = mtlData->getTexture((ofbx::Texture::TextureType)i);
            if (tData == NULL) continue;

            osg::Texture2D* tex2D = _textureMap[tData].get();
            if (!tex2D)
            {
                const ofbx::DataView& name = tData->getFileName();
                const ofbx::DataView& content = tData->getEmbeddedData();
                if (!name.begin || !name.end) continue;

                std::string originalName(name.begin, name.end);
                std::string ext = osgDB::getFileExtension(originalName);
                std::string fileName = osgDB::convertStringFromCurrentCodePageToUTF8(originalName);

                tex2D = new osg::Texture2D;
                tex2D->setResizeNonPowerOfTwoHint(false);
                tex2D->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                tex2D->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
                tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
                tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

                osg::ref_ptr<osg::Image> image;
                if (content.begin != NULL && content.begin != content.end)
                {
                    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
                    if (rw == NULL)  // try our luck!
                        rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_image");

                    if (rw != NULL)
                    {
                        std::vector<unsigned char> buffer(content.begin + 4, content.end);
                        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
                        ss.write((char*)&buffer[0], buffer.size());
                        image = rw->readImage(ss).getImage();
                        if (image.valid()) image->setFileName(originalName);
                    }

                }

                if (!image)
                {
                    std::string realFile = osgDB::findDataFile(fileName);
                    if (!realFile.empty()) image = osgDB::readImageFile(realFile);
                }

                if (!image)
                {
                    fileName = osgDB::getSimpleFileName(originalName);
                    image = osgDB::readImageFile(_workingDir + fileName);
                    originalName = _workingDir + fileName;
                }

                if (!image) continue;
                if (ext == "dds" || ext == "DDS") image->flipVertical();  // FIXME: optional?
                tex2D->setImage(image.get());
                tex2D->setName(originalName);

                _textureMap[tData] = tex2D;
                OSG_NOTICE << "[LoaderFBX] " << originalName << " loaded for "
                           << uniformNames[i] << std::endl;
            }

            if (tex2D->getImage() != NULL)
                ss->setTextureAttributeAndModes(i, tex2D);
            //ss->addUniform(new osg::Uniform(uniformNames[i].c_str(), i));
        }
    }

    class FindTransformVisitor : public osg::NodeVisitor
    {
    public:
        FindTransformVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN) {}
        std::vector<osg::Transform*> bones;

        virtual void apply(osg::Transform& node)
        { bones.push_back(&node); traverse(node); }
    };

    void LoaderFBX::mergeMeshBones(std::vector<SkinningData>& skinningList)
    {
        std::map<ofbx::Object*, osg::MatrixTransform*> boneMapper;
        std::map<osg::Transform*, ofbx::Object*> boneMapper2;
        std::map<osg::Node*, osg::Geode*> boneToGeodeMap;
        for (std::map<osg::Geode*, MeshSkinningData>::iterator itr = _meshBoneMap.begin();
             itr != _meshBoneMap.end(); ++itr)
        {
            MeshSkinningData& msd = itr->second;
            std::map<ofbx::Object*, MeshSkinningData::ParentAndBindPose>::iterator itr2;
            for (itr2 = msd.boneLinks.begin(); itr2 != msd.boneLinks.end(); ++itr2)
            {
                ofbx::Object *child = itr2->first, *parent = itr2->second.first;
                osg::MatrixTransform *childMT = NULL, *parentMT = NULL;
                if (boneMapper.find(child) == boneMapper.end())
                {
                    childMT = new osg::MatrixTransform; childMT->setName(child->name);
                    boneMapper[child] = childMT; boneMapper2[childMT] = child;
                }
                if (boneMapper.find(parent) == boneMapper.end())
                {
                    parentMT = new osg::MatrixTransform; parentMT->setName(parent->name);
                    boneMapper[parent] = parentMT; boneMapper2[parentMT] = parent;
                }

                childMT = boneMapper[child]; parentMT = boneMapper[parent];
                childMT->setMatrix(itr2->second.second);
                if (!parentMT->containsNode(childMT)) parentMT->addChild(childMT);
                boneToGeodeMap[childMT] = itr->first;
                boneToGeodeMap[parentMT] = itr->first;
            }
        }

        // Find bones
        std::vector<std::vector<osg::Transform*>> skeletonList;
        for (std::map<ofbx::Object*, osg::MatrixTransform*>::iterator itr = boneMapper.begin();
             itr != boneMapper.end(); ++itr)
        {
            if (itr->second->getNumParents() > 0) continue;
            FindTransformVisitor ftv; itr->second->accept(ftv);
            skeletonList.push_back(ftv.bones);
        }

        // Fill skinning data with bones and geometry-related data
        skinningList.resize(skeletonList.size());
        for (size_t i = 0; i < skinningList.size(); ++i)
        {
            SkinningData& sd = skinningList[i];
            sd.boneRoot = skeletonList[i].front();
            sd.joints = skeletonList[i];

            osg::Geode* geode = boneToGeodeMap[sd.boneRoot.get()];
            if (!geode) { OSG_WARN << "[LoaderFBX] Invalid bone root data" << std::endl; continue; }
            for (size_t j = 0; j < geode->getNumDrawables(); ++j)
            {
                osg::Geometry* geom = geode->getDrawable(j)->asGeometry();
                if (!geom) continue; else sd.meshList.push_back(geom);

                PlayerAnimation::GeometryJointData& gjd = sd.jointData[geom];
                gjd._weightList.resize(static_cast<osg::Vec3Array*>(geom->getVertexArray())->size());
                gjd._stateset = geom->getStateSet();
            }

            MeshSkinningData& msd = _meshBoneMap[geode];
            for (size_t j = 0; j < sd.joints.size(); ++j)
            {
                osg::Transform* boneT = sd.joints[j]; ofbx::Object* fbxNode = boneMapper2[boneT];
                if (!fbxNode) { OSG_WARN << "[LoaderFBX] Invalid bone data" << std::endl; continue; }

                std::vector<int>& indices = msd.boneIndices[fbxNode];
                std::vector<double>& weights = msd.boneWeights[fbxNode];
                if (indices.empty() || indices.size() != weights.size()) continue;

                std::map<int, std::pair<osg::Geometry*, int>>& globalMap = msd.globalIndexMap;
                for (size_t k = 0; k < indices.size(); ++k)
                {
                    std::pair<osg::Geometry*, int>& kv = globalMap[indices[k]];
                    if (kv.first == NULL)
                    { OSG_WARN << "[LoaderFBX] Invalid index " << indices[k] << std::endl; continue; }

                    PlayerAnimation::GeometryJointData& gjd = sd.jointData[kv.first];
                    gjd._invBindPoseMap[boneT] = boneT->asMatrixTransform()->getMatrix();
                    gjd._weightList[kv.second].push_back(
                        std::pair<osg::Transform*, float>(boneT, weights[k]));
                }
            }
        }  // for (size_t i = 0; i < skinningList.size(); ++i)
    }

    void LoaderFBX::createPlayers(std::vector<SkinningData>& skinningList)
    {
        // TODO
    }

    osg::ref_ptr<osg::Group> loadFbx(const std::string& file)
    {
        std::string workDir = osgDB::getFilePath(file);
        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        if (!in)
        {
            OSG_WARN << "[LoaderFBX] file " << file << " not readable" << std::endl;
            return NULL;
        }

        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, workDir);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadFbx2(std::istream& in, const std::string& dir)
    {
        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, dir);
        return loader->getRoot();
    }
}
