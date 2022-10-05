#include <osg/io_utils>
#include <osg/Version>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
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
            if (pData != NULL) OSG_NOTICE << "[LoaderFBX] <POSE> not implemented\n";

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

        // Apply materials to geometries
        for (std::map<const ofbx::Material*, std::vector<osg::Geometry*>>::iterator
            itr = _geometriesByMtl.begin(); itr != _geometriesByMtl.end(); ++itr)
        {
            std::vector<osg::Geometry*>& geometries = itr->second;
            for (size_t i = 0; i < geometries.size(); ++i)
                createMaterial(itr->first, geometries[i]->getOrCreateStateSet());
        }
    }

    osg::Geode* LoaderFBX::createGeometry(const ofbx::Mesh& mesh, const ofbx::Geometry& gData)
    {
        int vCount = gData.getVertexCount(), iCount = gData.getIndexCount();
        const ofbx::Vec3* vData = gData.getVertices();
        const ofbx::Vec3* nData = gData.getNormals();
        const ofbx::Vec4* cData = gData.getColors();
        const ofbx::Vec2* tData0 = gData.getUVs(0);
        const ofbx::Vec2* tData1 = gData.getUVs(1);
        const int* iData = gData.getFaceIndices();
        const int* mData = gData.getMaterials();

        if (vCount <= 0 || iCount <= 0) return NULL;
        if (vCount != iCount)
            OSG_WARN << "[LoaderFBX] Unknown geometry layout: " << vCount << " / " << iCount << "\n";

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(vCount);
        osg::ref_ptr<osg::Vec3Array> na = nData ? new osg::Vec3Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec4Array> ca = cData ? new osg::Vec4Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec2Array> ta0 = tData0 ? new osg::Vec2Array(vCount) : NULL;
        osg::ref_ptr<osg::Vec2Array> ta1 = tData1 ? new osg::Vec2Array(vCount) : NULL;
        for (int i = 0; i < vCount; ++i)
        {
            (*va)[i] = osg::Vec3(vData[i].x, vData[i].y, vData[i].z);
            if (nData) (*na)[i] = osg::Vec3(nData[i].x, nData[i].y, nData[i].z);
            if (cData) (*ca)[i] = osg::Vec4(cData[i].x, cData[i].y, cData[i].z, cData[i].w);
            if (tData0) (*ta0)[i] = osg::Vec2(tData0[i].x, tData0[i].y);
            if (tData1) (*ta1)[i] = osg::Vec2(tData1[i].x, tData1[i].y);
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
        for (std::map<int, osg::ref_ptr<osg::DrawElementsUInt>>::iterator itr = primitivesByMtl.begin();
            itr != primitivesByMtl.end(); ++itr)
        {
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setVertexArray(va.get());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
            if (nData) geom->setNormalArray(na.get(), osg::Array::BIND_PER_VERTEX);
            if (cData) geom->setColorArray(ca.get(), osg::Array::BIND_PER_VERTEX);
            if (tData0) geom->setTexCoordArray(0, ta0.get(), osg::Array::BIND_PER_VERTEX);
            if (tData1) geom->setTexCoordArray(1, ta1.get(), osg::Array::BIND_PER_VERTEX);
#else
            if (nData) { geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (cData) { geom->setColorArray(ca.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }
            if (tData0) { geom->setTexCoordArray(0, ta0.get()); }
            if (tData1) { geom->setTexCoordArray(1, ta1.get()); }
#endif
            geom->addPrimitiveSet(itr->second.get());
            geode->addDrawable(geom.get());

            if (itr->first < mesh.getMaterialCount())
            {
                const ofbx::Material* mtl = mesh.getMaterial(itr->first);
                _geometriesByMtl[mtl].push_back(geom.get());
            }
            else
                OSG_NOTICE << "[LoaderFBX] No material on this geometry\n";
        }

        if (gData.getSkin())
        {
            OSG_NOTICE << "[LoaderFBX] <SKIN> not implemented\n";
        }

        if (gData.getBlendShape())
        {
            OSG_NOTICE << "<BLENDSHAPE> not implemented\n";
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
                    if (rw != NULL)
                    {
                        std::vector<unsigned char> buffer(content.begin + 4, content.end);
                        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
                        ss.write((char*)&buffer[0], buffer.size());
                        image = rw->readImage(ss).getImage();
                        image->setFileName(originalName);
                    }
                    else
                        image = osgDB::readImageFile(fileName);
                }
                else
                    image = osgDB::readImageFile(fileName);

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

            ss->setTextureAttributeAndModes(i, tex2D);
            //ss->addUniform(new osg::Uniform(uniformNames[i].c_str(), i));
        }
    }

    osg::ref_ptr<osg::Group> loadFbx(const std::string& file)
    {
        std::string workDir = osgDB::getFilePath(file);
        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, workDir);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadFbx2(std::istream& in, const std::string& dir)
    {
        osg::ref_ptr<LoaderFBX> loader = new LoaderFBX(in, dir);
        return loader->getRoot();
    }
}
