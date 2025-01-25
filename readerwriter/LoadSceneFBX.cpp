#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Material>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/SmoothingVisitor>

#include "pipeline/Utilities.h"
#include "LoadSceneFBX.h"
#define DISABLE_SKINNING_DATA 0

namespace osgVerse
{
    LoaderFBX::LoaderFBX(std::istream& in, const std::string& d)
        : _scene(NULL), _workingDir(d + "/")
    {
        std::istreambuf_iterator<char> eos;
        std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
        if (data.empty()) { OSG_WARN << "[LoaderFBX] Unable to read from stream\n"; return; }

        _scene = ofbx::load((ofbx::u8*)&data[0], data.size(), (ofbx::u64)ofbx::LoadFlags::NONE);
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

            const ofbx::Pose* pData = mesh.getPose();
            if (pData != NULL)
            {
                //OSG_NOTICE << "[LoaderFBX] <POSE> " << pData->name << " not implemented\n";
            }

            const ofbx::GeometryData& gData = mesh.getGeometryData();
            if (gData.hasVertices())
            {
                geode = createGeometry(mesh, gData);
                geode->setName(meshName);
                _root->addChild(geode.get());
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
                    if (curveNode->getBone())
                    {
                        // TODO: bone animation
                    }
                    else if (curveNode->getCurve(0))
                        createAnimation(layer, curveNode);  // rigid animation
                }
            }
            // TODO: more than 1 animations?
        }
    }

    osg::Geode* LoaderFBX::createGeometry(const ofbx::Mesh& mesh, const ofbx::GeometryData& gData)
    {
        osg::Matrix matrix = osg::Matrix(mesh.getGeometricMatrix().m)*
                             osg::Matrix(mesh.getGlobalTransform().m);
        osg::Matrix invMatrix = osg::Matrix::inverse(matrix);
        ofbx::Vec3Attributes vData = gData.getPositions();
        ofbx::Vec3Attributes nData = gData.getNormals();
        ofbx::Vec3Attributes tData = gData.getTangents();
        ofbx::Vec4Attributes cData = gData.getColors();
        ofbx::Vec2Attributes uvData0 = gData.getUVs(0);
        ofbx::Vec2Attributes uvData1 = gData.getUVs(1);

        int vCount = vData.count, pCount = gData.getPartitionCount();
        if (vCount <= 0 || pCount <= 0) return NULL;

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(vCount);
        osg::ref_ptr<osg::Vec3Array> na = nData.values != NULL ? new osg::Vec3Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec4Array> ta = tData.values != NULL ? new osg::Vec4Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec4Array> ca = cData.values != NULL ? new osg::Vec4Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec2Array> uv0 = uvData0.values != NULL ? new osg::Vec2Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec2Array> uv1 = uvData1.values != NULL ? new osg::Vec2Array(vCount) : NULL;
        for (int i = 0; i < vCount; ++i)
        {
            ofbx::Vec3 vec = vData.get(i); (*va)[i] = osg::Vec3(vec.x, vec.y, vec.z) * matrix;
            if (nData.values != NULL)
            {
                vec = (i < nData.count) ? nData.get(i) : nData.get(0);
                (*na)[i] = osg::Matrix::transform3x3(invMatrix, osg::Vec3(vec.x, vec.y, vec.z));
            }
            if (tData.values != NULL)
            {
                vec = (i < tData.count) ? tData.get(i) : tData.get(0);
                (*ta)[i] = osg::Vec4(vec.x, vec.y, vec.z, 1.0f);
            }
            if (cData.values != NULL)
            {
                ofbx::Vec4 color = (i < cData.count) ? cData.get(i) : cData.get(0);
                (*ca)[i] = osg::Vec4(color.x, color.y, color.z, color.w);
            }
            if (uvData0.values != NULL)
            {
                ofbx::Vec2 tex = (i < uvData0.count) ? uvData0.get(i) : uvData0.get(0);
                (*uv0)[i] = osg::Vec2(tex.x, tex.y);
            }
            if (uvData1.values != NULL)
            {
                ofbx::Vec2 tex = (i < uvData1.count) ? uvData1.get(i) : uvData1.get(0);
                (*uv1)[i] = osg::Vec2(tex.x, tex.y);
            }
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        std::map<int, std::pair<osg::Geometry*, int>> globalIndexMap;
        for (int i = 0; i < pCount; ++i)
        {   // each ofbx::Mesh can have several materials == partitions
            const ofbx::GeometryPartition& partition = gData.getPartition(i);
            osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
            for (int p = 0; p < partition.polygon_count; ++p)
            {
                const ofbx::GeometryPartition::Polygon& polygon = partition.polygons[p];
                std::vector<int> indices(3 * (polygon.vertex_count - 2));
                if (ofbx::triangulate(gData, polygon, &indices[0]) > 0)
                    de->insert(de->end(), indices.begin(), indices.end());
            }

            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            osg::ref_ptr<osg::Vec3Array> subVA = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> subNA = nData.values ? new osg::Vec3Array : NULL;
            osg::ref_ptr<osg::Vec4Array> subTA = tData.values ? new osg::Vec4Array : NULL;
            osg::ref_ptr<osg::Vec4Array> subCA = cData.values ? new osg::Vec4Array : NULL;
            osg::ref_ptr<osg::Vec2Array> subUV0 = uvData0.values ? new osg::Vec2Array : NULL;
            osg::ref_ptr<osg::Vec2Array> subUV1 = uvData1.values ? new osg::Vec2Array : NULL;

            std::map<unsigned int, unsigned int> globalToLocalMap;
            for (size_t j = 0; j < de->size(); ++j)
            {
                unsigned int idx = (*de)[j];
                if (globalToLocalMap.find(idx) == globalToLocalMap.end())
                {
                    globalToLocalMap[idx] = subVA->size();
                    if (globalIndexMap.find(idx) != globalIndexMap.end())
                    {
                        OSG_NOTICE << "[LoaderFBX] global vertex index (in an FBX mesh) " << idx
                                   << " seems to be reused by multiple geometries" << std::endl;
                    }
                    globalIndexMap[idx] = std::pair<osg::Geometry*, int>(geom.get(), subVA->size());

                    subVA->push_back((*va)[idx]);
                    if (nData.values) subNA->push_back((*na)[idx]);
                    if (tData.values) subTA->push_back((*ta)[idx]);
                    if (cData.values) subCA->push_back((*ca)[idx]);
                    if (uvData0.values) subUV0->push_back((*uv0)[idx]);
                    if (uvData1.values) subUV1->push_back((*uv1)[idx]);
                }
                (*de)[j] = globalToLocalMap[idx];
            }

            geom->setVertexArray(subVA.get());
            if (nData.values)
            { geom->setNormalArray(subNA.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (tData.values)
            { geom->setVertexAttribArray(6, subTA.get()); geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX); }
            if (cData.values)
            { geom->setColorArray(subCA.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (uvData0.values) { geom->setTexCoordArray(0, subUV0.get()); }
            if (uvData1.values) { geom->setTexCoordArray(1, subUV1.get()); }
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            geom->addPrimitiveSet(de.get());
            geode->addDrawable(geom.get());
            if (!nData.values) osgUtil::SmoothingVisitor::smooth(*geom);

            if (i >= 0 && i < mesh.getMaterialCount())
            {
                const ofbx::Material* mtl = mesh.getMaterial(i);
                _geometriesByMtl[mtl].push_back(geom.get());
            }
            else
                OSG_NOTICE << "[LoaderFBX] No material on geometry: " << i << "\n";
        }

        const ofbx::Skin* skin = mesh.getSkin();
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

                if (cluster->getIndicesCount() == 0) continue;
                std::vector<int>& boneIndices = msd.boneIndices[boneNode];
                std::vector<double>& boneWeights = msd.boneWeights[boneNode];
                boneIndices.assign(cluster->getIndices(),
                                   cluster->getIndices() + cluster->getIndicesCount());
                boneWeights.assign(cluster->getWeights(),
                                   cluster->getWeights() + cluster->getWeightsCount());
            }
        }

        const ofbx::BlendShape* bs = mesh.getBlendShape();
        if (bs != NULL)
        {
            OSG_NOTICE << "[LoaderFBX] <BLENDSHAPE> " << bs->name << " not implemented\n";
            MeshSkinningData& msd = _meshBoneMap[geode.get()];
            // TODO
        }
        return geode.release();
    }

    void LoaderFBX::createAnimation(const ofbx::AnimationLayer* layer,
                                    const ofbx::AnimationCurveNode* curveNode)
    {
        const ofbx::Object* bone = curveNode->getBone();
        ofbx::DVec3 pivot = bone->getRotationPivot();
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
        if (!mt)
        {
            OSG_NOTICE << "[LoaderFBX] Unable to find bone node " << boneName << " matching animation "
                       << propertyName << ", in layer " << layer->name << std::endl; return;
        }

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
        { cpMap[itr->first].setPosition(itr->second + pivotOffset); }

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
                ofbx::DataView name = tData->getFileName();
                ofbx::DataView content = tData->getEmbeddedData();
                if (!name.begin || !name.end) continue;

                std::string originalName(name.begin, name.end);
                std::string ext = osgDB::getFileExtension(originalName);
                std::string fileName = osgDB::convertStringFromCurrentCodePageToUTF8(originalName);
                std::string simpleFileName = osgDB::getSimpleFileName(originalName);

                tex2D = new osg::Texture2D;
                tex2D->setResizeNonPowerOfTwoHint(false);
                tex2D->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                tex2D->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
                tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
                tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

                osg::ref_ptr<osg::Image> image;
                if (content.begin == NULL)
                {
                    for (int k = 0; k < _scene->getEmbeddedDataCount(); ++k)
                    {
                        ofbx::DataView embeddedView = _scene->getEmbeddedFilename(k);
                        std::string embeddedName = osgDB::getSimpleFileName(
                            std::string(embeddedView.begin, embeddedView.end));
                        if (simpleFileName == embeddedName)
                        { content = _scene->getEmbeddedData(k); break; }
                    }
                }

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
                        if (image.valid()) image->setFileName(simpleFileName);
                    }
                }

                if (!image)
                {
                    std::string realFile = osgDB::findDataFile(fileName);
                    if (!realFile.empty()) image = osgDB::readImageFile(realFile);
                }

                if (!image)
                {
                    image = osgDB::readImageFile(_workingDir + simpleFileName);
                    originalName = _workingDir + simpleFileName;
                }

                if (!image) continue;
                if (ext == "dds" || ext == "DDS") image->flipVertical();  // FIXME: optional?
                tex2D->setImage(image.get()); tex2D->setName(originalName);

                _textureMap[tData] = tex2D;
                OSG_NOTICE << "[LoaderFBX] " << originalName << " loaded for "
                           << uniformNames[i] << std::endl;
            }

            if (tex2D->getImage() != NULL)
                ss->setTextureAttributeAndModes(i, tex2D);
            //ss->addUniform(new osg::Uniform(uniformNames[i].c_str(), i));
        }

#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
        ofbx::Color dC = mtlData->getDiffuseColor(), sC = mtlData->getSpecularColor();
        ofbx::Color aC = mtlData->getAmbientColor(), eC = mtlData->getEmissiveColor();

        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(dC.r, dC.g, dC.b, 1.0f));
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(aC.r, aC.g, aC.b, 1.0f));
        material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(sC.r, sC.g, sC.b, 1.0f));
        material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(eC.r, eC.g, eC.b, 1.0f));
        ss->setAttributeAndModes(material.get());
#endif
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
        std::map<osg::MatrixTransform*, osg::Matrix> globalMatrixMapper2;
        std::map<osg::Geode*, std::set<osg::Node*>> boneToGeodeMap;
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
                globalMatrixMapper2[childMT] = itr2->second.second;
                if (!parentMT->containsNode(childMT)) parentMT->addChild(childMT);
                boneToGeodeMap[itr->first].insert(childMT);
                boneToGeodeMap[itr->first].insert(parentMT);
            }
        }

        // Find complete bones
        std::vector<std::vector<osg::Transform*>> skeletonList;
        for (std::map<ofbx::Object*, osg::MatrixTransform*>::iterator itr = boneMapper.begin();
             itr != boneMapper.end(); ++itr)
        {
            if (itr->second->getNumParents() > 0) continue;
            FindTransformVisitor ftv; itr->second->accept(ftv);
            skeletonList.push_back(ftv.bones);

            // Apply matrices after converted them to local space
            osg::Transform* rootBone = ftv.bones.front();
            for (size_t i = 0; i < ftv.bones.size(); ++i)
            {
                osg::MatrixTransform* bone = ftv.bones[i]->asMatrixTransform();
                if (globalMatrixMapper2.find(bone) == globalMatrixMapper2.end())
                {
                    OSG_NOTICE << "[LoaderFBX] Unable to apply matrix to bone: "
                               << bone->getName() << std::endl; continue;
                }

                osg::MatrixList parentMatrices = bone->getWorldMatrices(rootBone);
                osg::Matrix parentInv = parentMatrices.empty() ? osg::Matrix()
                                      : osg::Matrix::inverse(parentMatrices[0]);
                bone->setMatrix(globalMatrixMapper2[bone] * parentInv);
            }
        }

        // Fill skinning data with bones and geometry-related data
        skinningList.resize(_meshBoneMap.size()); int index = 0;
        for (std::map<osg::Geode*, MeshSkinningData>::iterator itr0 = _meshBoneMap.begin();
             itr0 != _meshBoneMap.end(); ++itr0, ++index)
        {
            osg::Geode* geode = itr0->first;
            MeshSkinningData& msd = itr0->second;
            std::set<osg::Node*>& belongings = boneToGeodeMap[geode];

            SkinningData& sd = skinningList[index]; sd.meshRoot = geode;
            for (size_t i = 0; i < skeletonList.size(); ++i)
            {
                if (belongings.find(skeletonList[i].front()) == belongings.end()) continue;
                sd.boneRoot = skeletonList[i].front(); sd.joints = skeletonList[i];
            }

            // Allocate weight-list (per vertex)
            for (size_t j = 0; j < geode->getNumDrawables(); ++j)
            {
                osg::Geometry* geom = geode->getDrawable(j)->asGeometry();
                if (!geom) continue; else sd.meshList.push_back(geom);

                PlayerAnimation::GeometryJointData& gjd = sd.jointData[geom];
                gjd._weightList.resize(static_cast<osg::Vec3Array*>(geom->getVertexArray())->size());
                gjd._stateset = geom->getStateSet();
            }

            // Fill weight-list with <bone, weight> pair for every vertex
            if (sd.joints.empty()) return;
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
                    if (globalMap.find(indices[k]) == globalMap.end())
                    { OSG_WARN << "[LoaderFBX] Invalid index " << indices[k] << std::endl; continue; }

                    std::pair<osg::Geometry*, int>& kv = globalMap[indices[k]];
                    PlayerAnimation::GeometryJointData& gjd = sd.jointData[kv.first];
                    gjd._invBindPoseMap[boneT] =  boneT->asMatrixTransform()->getMatrix();
                    gjd._weightList[kv.second].push_back(
                        std::pair<osg::Transform*, float>(boneT, weights[k]));
                }
                //std::cout << boneT->getName().c_str() << boneT->asMatrixTransform()->getMatrix();
            }

            // Check weight-list and ensure each has 4 bone-weight pair
            std::pair<osg::Transform*, float> emptyWeight(sd.joints.front(), 0.0f);
            for (size_t j = 0; j < geode->getNumDrawables(); ++j)
            {
                osg::Geometry* geom = geode->getDrawable(j)->asGeometry();
                PlayerAnimation::GeometryJointData& gjd = sd.jointData[geom];
                for (size_t k = 0; k < gjd._weightList.size(); ++k)
                    gjd._weightList[k].resize(4, emptyWeight);
            }
        }  // for (size_t i = 0; i < skinningList.size(); ++i)
    }

    void LoaderFBX::createPlayers(std::vector<SkinningData>& skinningList)
    {
        for (size_t i = 0; i < skinningList.size(); ++i)
        {
            SkinningData& sd = skinningList[i];
            if (sd.joints.empty() || !sd.meshRoot) continue;

#if !DISABLE_SKINNING_DATA
            sd.player = new PlayerAnimation;
            sd.player->setModelRoot(_root.get());
            sd.player->initialize(sd.joints, sd.meshList, sd.jointData);
            sd.meshRoot->addUpdateCallback(sd.player.get());
#endif
        }
    }

    osg::ref_ptr<osg::Group> loadFbx(const std::string& file)
    {
        std::string workDir = osgDB::getFilePath(file), http = osgDB::getServerProtocol(file);
        if (!http.empty()) return NULL;
        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        if (!in)
        {
            OSG_WARN << "[LoaderFBX] file " << file << " not readable" << std::endl;
            return NULL;
        }

        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, workDir);
        if (loader->getRoot()) loader->getRoot()->setName(file);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadFbx2(std::istream& in, const std::string& dir)
    {
        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, dir);
        return loader->getRoot();
    }
}
