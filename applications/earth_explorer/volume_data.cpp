#include <osg/io_utils>
#include <osg/BlendFunc>
#include <osg/Texture1D>
#include <osg/Texture3D>
#include <osg/MatrixTransform>
#include <osg/ProxyNode>
#include <osg/LOD>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <pipeline/Pipeline.h>
#include <readerwriter/EarthManipulator.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

extern std::string global_volumeToLoad;

class MatrixVolumeCallback : public osg::NodeCallback
{
public:
    MatrixVolumeCallback(osg::StateSet* ss, const osg::Vec3& origin, const osg::Vec3& size)
    :   _stateset(ss), _origin(origin), _size(size) { _scale.set(1.0f, 1.0f, 1.0f); }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform* mt = static_cast<osg::MatrixTransform*>(node);
        if (mt && _stateset.valid())
        {
            osg::Vec3 pos, s; osg::Quat rot, so;
            mt->getWorldMatrices()[0].decompose(pos, rot, s, so);

            bool dirty = false;
            if (pos != _origin || rot != _rotate) dirty = true;
            if (dirty)
            {
                osg::Vec3 size = osg::Vec3(_size[0] * s[0], _size[1] * s[1], _size[2] * s[2]);
                size[0] /= _scale[0]; size[1] /= _scale[1]; size[2] /= _scale[2];
                _stateset->getUniform("BoundingMin")->set(_origin);
                _stateset->getUniform("BoundingMax")->set(_origin + size);
                _stateset->getUniform("RotationOffset")->set(
                    osg::Matrix::translate(-pos) * osg::Matrixf::rotate(rot) * osg::Matrix::translate(pos));
                _origin = pos; _rotate = rot;
            }
        }
        traverse(node, nv);
    }

protected:
    osg::observer_ptr<osg::StateSet> _stateset;
    osg::Quat _rotate;
    osg::Vec3 _origin, _size, _scale;
};

typedef std::pair<osg::MatrixTransform*, osg::StateSet*> ResultPair;
ResultPair createVolumeData(osg::Image* image3D, osg::Image* transferImage1D, const osg::Matrixf& matrix,
                            const osg::Vec3& origin, const osg::Vec3& spacing, float minValue, float maxValue)
{
    // Create geometry & stateset
    osg::Vec3d size(image3D->s() * spacing[0], image3D->t() * spacing[1], image3D->r() * spacing[2]);

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    va->push_back(osg::Vec3(0.0f, 0.0f, 0.0f));
    va->push_back(osg::Vec3(size[0], 0.0f, 0.0f));
    va->push_back(osg::Vec3(size[0], size[1], 0.0f));
    va->push_back(osg::Vec3(0.0f, size[1], 0.0f));
    va->push_back(osg::Vec3(0.0f, 0.0f, size[2]));
    va->push_back(osg::Vec3(size[0], 0.0f, size[2]));
    va->push_back(osg::Vec3(size[0], size[1], size[2]));
    va->push_back(osg::Vec3(0.0f, size[1], size[2]));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    for (int i = 0; i < 6; ++i)
    {
        osg::ref_ptr<osg::DrawElementsUByte> de = new osg::DrawElementsUByte(GL_QUADS);
        switch (i)
        {
        case 0: de->push_back(0); de->push_back(1); de->push_back(2); de->push_back(3); break;
        case 1: de->push_back(4); de->push_back(5); de->push_back(6); de->push_back(7); break;
        case 2: de->push_back(0); de->push_back(1); de->push_back(5); de->push_back(4); break;
        case 3: de->push_back(1); de->push_back(2); de->push_back(6); de->push_back(5); break;
        case 4: de->push_back(2); de->push_back(3); de->push_back(7); de->push_back(6); break;
        case 5: de->push_back(3); de->push_back(0); de->push_back(4); de->push_back(7); break;
        }

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        geom->setVertexArray(va.get()); geom->addPrimitiveSet(de.get());
        geode->setCullingActive(false); geode->addDrawable(geom.get());
    }

    osg::ref_ptr<osg::Texture3D> tex3D = new osg::Texture3D;
    tex3D->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    tex3D->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    tex3D->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::CLAMP);
    tex3D->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::CLAMP);
    tex3D->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP);
    tex3D->setResizeNonPowerOfTwoHint(false);
    tex3D->setImage(image3D);

    osg::ref_ptr<osg::Texture1D> tex1D = new osg::Texture1D;
    if (transferImage1D != NULL)
    {
        tex1D->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
        tex1D->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
        tex1D->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::CLAMP);
        tex1D->setResizeNonPowerOfTwoHint(false);
        tex1D->setImage(transferImage1D);
    }

    osg::StateSet* ss = geode->getOrCreateStateSet();
    //ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    ss->setTextureAttributeAndModes(0, tex3D.get());
    ss->addUniform(new osg::Uniform("VolumeTexture", (int)0));
    if (transferImage1D != NULL)
    {
        ss->setTextureAttributeAndModes(1, tex1D.get());
        ss->addUniform(new osg::Uniform("TransferTexture", (int)1));
        ss->addUniform(new osg::Uniform("TransferMode", (int)1));
    }
    else
        ss->addUniform(new osg::Uniform("TransferMode", (int)0));
    ss->addUniform(new osg::Uniform("RotationOffset",
        osg::Matrixf::translate(-origin) * matrix * osg::Matrixf::translate(origin)));
    ss->addUniform(new osg::Uniform("Color", osg::Vec3(1.0f, 1.0f, 1.0f)));
    ss->addUniform(new osg::Uniform("BoundingMin", origin));
    ss->addUniform(new osg::Uniform("BoundingMax", origin + size));
    ss->addUniform(new osg::Uniform("ValueRange", osg::Vec3(minValue, maxValue - minValue, 0.0f)));
    ss->addUniform(new osg::Uniform("RayMarchingSamples", (int)128));
    ss->addUniform(new osg::Uniform("DensityFactor", 2.0f));
    ss->addUniform(new osg::Uniform("DensityPower", 2.0f));
    ss->addUniform(new osg::Uniform("SliceMin", osg::Vec3(0.0f, 0.0f, 0.0f)));
    ss->addUniform(new osg::Uniform("SliceMax", osg::Vec3(1.0f, 1.0f, 1.0f)));

    // Create scene nodes
    osg::ref_ptr<osg::MatrixTransform> sceneItem = new osg::MatrixTransform;
    sceneItem->setInitialBound(osg::BoundingSphere(osg::Vec3(), spacing[2] * 5.0f));
    sceneItem->addUpdateCallback(new MatrixVolumeCallback(ss, origin, size));
    sceneItem->setMatrix(matrix * osg::Matrix::translate(origin));
    sceneItem->addChild(geode.get());
    return ResultPair(sceneItem.release(), ss);
}

typedef std::pair<osg::Node*, osg::Vec3d> ResultPair2;
typedef std::pair<ResultPair, ResultPair2> VolumeTotalResult;
VolumeTotalResult createVolumeBox(const std::string& vdbFile, const osg::Vec3d& fromLLA,
                                  const osg::Vec3d& toLLA, float minV, float maxV)
{
    osg::Matrix enu = osgVerse::Coordinate::convertLLAtoNED(fromLLA);
    osg::Vec3 center = enu.getTrans(), from, to, size; enu.setTrans(osg::Vec3());

    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(vdbFile);
    if (!image) { OSG_FATAL << "Failed to load volume data: " << vdbFile << "\n"; return VolumeTotalResult(); }

    from = osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3(fromLLA[0], fromLLA[1], 0.0));
    to = osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3(toLLA[0], fromLLA[1], 0.0)); size[0] = (from - to).length();
    from = osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3(fromLLA[0], fromLLA[1], 0.0));
    to = osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3(fromLLA[0], toLLA[1], 0.0)); size[1] = (from - to).length();
    from = osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3(toLLA[0], toLLA[1], fromLLA[2]));
    to = osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3(toLLA[0], toLLA[1], toLLA[2]));
    size[2] = (from - to).length() * 10.0;  // FIXME: for zhijiang depth...
    
    // Extent = (spacing[0] * image->s(), spacing[1] * image->t(), spacing[2] * image->r())
    osg::Vec3d spacing(fabs(size[0]) / image->s(), fabs(size[1]) / image->t(), fabs(size[2]) / image->r());
    osg::ref_ptr<osg::Image> image1D = osgVerse::generateTransferFunction(2);
    ResultPair pair = createVolumeData(image.get(), image1D.get(), enu, center, spacing, minV, maxV);
    std::cout << vdbFile << ": " << center << ", R = " << center.length() << "; Spacing = " << spacing << "\n";

#if false
    osg::LOD* lod = new osg::LOD;
    lod->setCenterMode(osg::LOD::CenterMode::USER_DEFINED_CENTER);
    lod->setCenter(pair.first->getBound().center());
    lod->setRadius(pair.first->getBound().radius());
    lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
    lod->addChild(new osg::Node, osg::WGS_84_RADIUS_POLAR * 0.2f, FLT_MAX);
    lod->addChild(pair.first, 0.0f, osg::WGS_84_RADIUS_POLAR * 0.2f);
#else
    osg::Group* lod = new osg::Group; lod->addChild(pair.first);
#endif
    return VolumeTotalResult(pair, ResultPair2(lod, (fromLLA + toLLA) * 0.5));
}

class VolumeHandler : public osgGA::GUIEventHandler
{
public:
    VolumeHandler() : _index(0) {}
    std::vector<VolumeTotalResult> vdbList;

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            if (!global_volumeToLoad.empty())
            {
                if (global_volumeToLoad.find("kerry") != std::string::npos) _index = 0;
                else if (global_volumeToLoad.find("parhaka") != std::string::npos) _index = 1;
                else if (global_volumeToLoad.find("waipuku") != std::string::npos) _index = 2;

                VolumeTotalResult vdb = vdbList[_index];
                osgVerse::EarthManipulator* manipulator =
                    static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
                manipulator->setByEye(osgVerse::Coordinate::convertLLAtoECEF(
                    osg::Vec3d(vdb.second.second[0], vdb.second.second[1], 1000.0)));
                global_volumeToLoad = "";
            }
        }

        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            VolumeTotalResult& vdb = vdbList[_index];
            osg::MatrixTransform* transform = vdb.first.first;
            osg::StateSet* ss = vdb.first.second;

            //osg::Vec3 pos, scale; osg::Quat rot, so;
            //transform->getMatrix().decompose(pos, rot, scale, so);
            osg::Vec3 valueMin, valueMax, valueRange; float dFactor = 0.0f, dPower = 0.0f;
            osg::Uniform* range = ss->getUniform("ValueRange"); range->get(valueRange);
            osg::Uniform *sliceMin = ss->getUniform("SliceMin"), *sliceMax = ss->getUniform("SliceMax");
            osg::Uniform *factor = ss->getUniform("DensityFactor"), *power = ss->getUniform("DensityPower");
            sliceMin->get(valueMin); sliceMax->get(valueMax); factor->get(dFactor); power->get(dPower);

            switch (ea.getKey())
            {
            case 'H': { if (valueMax.x() > 0.0f) valueMax.x() -= 0.01f; sliceMax->set(valueMax); } break;
            case 'h': { if (valueMin.x() > 0.0f) valueMin.x() -= 0.01f; sliceMin->set(valueMin); } break;
            case 'K': { if (valueMax.x() < 1.0f) valueMax.x() += 0.01f; sliceMax->set(valueMax); } break;
            case 'k': { if (valueMin.x() < 1.0f) valueMin.x() += 0.01f; sliceMin->set(valueMin); } break;
            case 'J': { if (valueMax.z() > 0.0f) valueMax.z() -= 0.01f; sliceMax->set(valueMax); } break;
            case 'j': { if (valueMin.z() > 0.0f) valueMin.z() -= 0.01f; sliceMin->set(valueMin); } break;
            case 'U': { if (valueMax.z() < 1.0f) valueMax.z() += 0.01f; sliceMax->set(valueMax); } break;
            case 'u': { if (valueMin.z() < 1.0f) valueMin.z() += 0.01f; sliceMin->set(valueMin); } break;
            case 'Y': { if (valueMax.y() > 0.0f) valueMax.y() -= 0.01f; sliceMax->set(valueMax); } break;
            case 'y': { if (valueMin.y() > 0.0f) valueMin.y() -= 0.01f; sliceMin->set(valueMin); } break;
            case 'I': { if (valueMax.y() < 1.0f) valueMax.y() += 0.01f; sliceMax->set(valueMax); } break;
            case 'i': { if (valueMin.y() < 1.0f) valueMin.y() += 0.01f; sliceMin->set(valueMin); } break;
            case 'T': { if (dFactor > 0.0f) dFactor -= 0.01f; factor->set(dFactor); } break;
            case 't': { if (dFactor < 5.0f) dFactor += 0.01f; factor->set(dFactor); } break;
            case 'G': { if (dPower > 0.0f) dPower -= 0.01f; power->set(dPower); } break;
            case 'g': { if (dPower < 5.0f) dPower += 0.01f; power->set(dPower); } break;
            case 'O': { valueRange[0] += 0.1f; valueRange[1] -= 0.1f; range->set(valueRange); } break;
            case 'o': { valueRange[0] -= 0.1f; valueRange[1] += 0.1f; range->set(valueRange); } break;
            case 'L': { valueRange[0] -= 0.1f; valueRange[1] -= 0.1f; range->set(valueRange); } break;
            case 'l': { valueRange[0] += 0.1f; valueRange[1] += 0.1f; range->set(valueRange); } break;
            }
        }
        return false;
    }

protected:
    int _index;
};

osg::Node* configureVolumeData(osgViewer::View& viewer, osg::Node* earthRoot,
                               const std::string& mainFolder, unsigned int mask)
{
    osg::ref_ptr<osg::Group> vdbRoot = new osg::Group;
    vdbRoot->setNodeMask(mask);

    osg::Shader* vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "fast_volume.vert");
    osg::Shader* fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "fast_volume.frag");
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME

    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("Volume_VS"); program->addShader(vs);
    fs->setName("Volume_FS"); program->addShader(fs);
    vdbRoot->getOrCreateStateSet()->setAttributeAndModes(program.get());
    //vdbRoot->getOrCreateStateSet()->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    vdbRoot->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    VolumeHandler* handler = new VolumeHandler; float underOffset = 6000.0;
    handler->vdbList.push_back(createVolumeBox(mainFolder + "/vdb/kerry_img3d.vdb.verse_vdb",
        osg::Vec3d(osg::inDegrees(-39.9978), osg::inDegrees(174.047), -1250.0),
        osg::Vec3d(osg::inDegrees(-39.6696), osg::inDegrees(174.214), -3.0), -4.0f, 4.0f));
    handler->vdbList.push_back(createVolumeBox(mainFolder + "/vdb/parihaka_img3d.vdb.verse_vdb",
        osg::Vec3d(osg::inDegrees(-51.0129), osg::inDegrees(-144.937), -1165.0 - underOffset),
        osg::Vec3d(osg::inDegrees(-50.977), osg::inDegrees(-144.856), -6.0 - underOffset), -2000.0f, 2000.0f));
    handler->vdbList.push_back(createVolumeBox(mainFolder + "/vdb/waihapa_img3d.vdb.verse_vdb",
        osg::Vec3d(osg::inDegrees(-51.0208), osg::inDegrees(-145.185), -2500.0 - underOffset),
        osg::Vec3d(osg::inDegrees(-51.0016), osg::inDegrees(-145.154), -0.0 - underOffset), -30000.0f, 30000.0f));

    for (size_t i = 0; i < handler->vdbList.size(); ++i)
        vdbRoot->addChild(handler->vdbList[i].second.first);
    viewer.addEventHandler(handler); return vdbRoot.release();
}
