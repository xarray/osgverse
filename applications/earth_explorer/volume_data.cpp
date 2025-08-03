#include <osg/io_utils>
#include <osg/BlendFunc>
#include <osg/Texture1D>
#include <osg/Texture3D>
#include <osg/MatrixTransform>
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

osg::MatrixTransform* createVolumeBox(const std::string& vdbFile, const osg::Vec3d& fromLLA,
                                      const osg::Vec3d& toLLA, float minV, float maxV)
{
    osg::Vec3d from = osgVerse::Coordinate::convertLLAtoECEF(fromLLA);
    osg::Vec3d to = osgVerse::Coordinate::convertLLAtoECEF(toLLA);
    osg::Vec3 center = from, size = to - from;
    osg::Matrix enu = osgVerse::Coordinate::convertLLAtoNED(fromLLA);

    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(vdbFile); if (!image) return NULL;
    osg::Vec3d spacing(fabs(size[0]) / image->s(), fabs(size[1]) / image->t(), fabs(size[2]) / image->r());
    enu.setTrans(osg::Vec3());

    // Extent = (spacing[0] * image->s(), spacing[1] * image->t(), spacing[2] * image->r())
    osg::ref_ptr<osg::Image> image1D = osgVerse::generateTransferFunction(2);
    ResultPair pair = createVolumeData(image.get(), image1D.get(), enu, center, spacing, minV, maxV);
    //std::cout << "VDB: " << center << ", " << spacing << "\n";
    return pair.first;
}

class VolumeHandler : public osgGA::GUIEventHandler
{
public:
    VolumeHandler(osg::MatrixTransform* mt) : _transform(mt) {}
    osg::observer_ptr<osg::MatrixTransform> _transform;

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        /*if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            osg::Vec3 pos, scale; osg::Quat rot, so;
            _transform->getMatrix().decompose(pos, rot, scale, so);
            switch (ea.getKey())
            {
            case 'q':
                rot = rot * osg::Quat(-0.1f, osg::X_AXIS);
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix(rot) * osg::Matrix::translate(pos));
                break;
            case 'e':
                rot = rot * osg::Quat(0.1f, osg::X_AXIS);
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix(rot) * osg::Matrix::translate(pos));
                break;
            case 'a':
                rot = rot * osg::Quat(-0.1f, osg::Y_AXIS);
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix(rot) * osg::Matrix::translate(pos));
                break;
            case 'd':
                rot = rot * osg::Quat(0.1f, osg::Y_AXIS);
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix(rot) * osg::Matrix::translate(pos));
                break;
            case 'x':
                rot = rot * osg::Quat(-0.1f, osg::Z_AXIS);
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix(rot) * osg::Matrix::translate(pos));
                break;
            case 'w':
                rot = rot * osg::Quat(0.1f, osg::Z_AXIS);
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix(rot) * osg::Matrix::translate(pos));
                break;
            }
        }*/
        return false;
    }
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

    vdbRoot->addChild(createVolumeBox(mainFolder + "/vdb/kerry_img3d.vdb.verse_vdb",
        osg::Vec3d(osg::inDegrees(-39.9978), osg::inDegrees(174.047), -1250.0),
        osg::Vec3d(osg::inDegrees(-39.6696), osg::inDegrees(174.214), -3.0), -12.0f, 12.0f));
    viewer.addEventHandler(new VolumeHandler(static_cast<osg::MatrixTransform*>(vdbRoot->getChild(0))));
    return vdbRoot.release();
}
