#include <osg/io_utils>
#include <osg/BlendFunc>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/ImageSequence>
#include <osg/ImageUtils>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <pipeline/Utilities.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

const char* vertCode = {
    "varying vec4 EyeVertex, TexCoord;\n"
    "void main() {\n"
    "    EyeVertex = gl_ModelViewMatrix * gl_Vertex;\n"
    "    TexCoord = gl_MultiTexCoord0;\n"
    "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "}\n"
};

const char* fragCode = {
    "uniform mat4 osg_ViewMatrix;\n"
    "uniform sampler3D VolumeTexture;\n"
    "uniform sampler1D TransferTexture;\n"
    "uniform vec3 BoundingMin, BoundingMax;\n"
    "uniform vec3 SliceMin, SliceMax;\n"
    "uniform vec2 ValueRange;\n"
    "uniform int RayMarchingSamples, TransferMode;\n"
    "uniform float DensityFactor, DensityPower;\n"
    "varying vec4 EyeVertex, TexCoord;\n"

    "const int maxSamples = 256;\n"
    "vec2 rayIntersectBox(vec3 rayDirection, vec3 rayOrigin) {  //Intersect ray with bounding box\n"
    "    vec3 rayInvDirection = 1.0 / rayDirection;\n"
    "    vec3 bbMinDiff = (BoundingMin - rayOrigin) * rayInvDirection;\n"
    "    vec3 bbMaxDiff = (BoundingMax - rayOrigin) * rayInvDirection;\n"
    "    vec3 imax = max(bbMaxDiff, bbMinDiff);\n"
    "    vec3 imin = min(bbMaxDiff, bbMinDiff);\n"
    "    float back = min(imax.x, min(imax.y, imax.z));\n"
    "    float front = max(max(imin.x, 0.0), max(imin.y, imin.z));\n"
    "    return vec2(back, front);\n"
    "}\n"

    "void main() {\n"
    "    // Get object-space ray origin & direction of each fragment \n"
    "    mat4 invModelView = transpose(osg_ViewMatrix);\n"
    "    vec4 camPos = -vec4(osg_ViewMatrix[3]);\n"
    "    vec3 rayDirection = normalize((invModelView * EyeVertex).xyz);\n"
    "    vec3 rayOrigin = (invModelView * camPos).xyz;\n"

    "    // Intersect ray with the volume's bounding box\n"
    "    // - subtract small increment to avoid errors on front boundary\n"
    "    // - discard points outside the box (no intersection)\n"
    "    vec2 intersection = rayIntersectBox(rayDirection, rayOrigin);\n"
    "    intersection.y -= 0.000001;\n"
    "    if (intersection.x <= intersection.y) discard;\n"

    "    float stepSize = 1.732 / float(RayMarchingSamples);\n"
    "    vec3 rayStart = rayOrigin + rayDirection * intersection.y;\n"
    "    vec3 rayStop = rayOrigin + rayDirection * intersection.x;\n"
    "    vec3 step = (rayStop - rayStart) * stepSize;\n"
    "    vec3 pos = rayStart, invBoundingDiff = 1.0 / (BoundingMax - BoundingMin);\n"

    "    // Raymarch, front to back\n"
    "    float T = 1.0, travel = distance(rayStop, rayStart) / stepSize;\n"
    "    float factor = DensityFactor * stepSize;\n"
    "    int samples = int(ceil(travel));\n"
    "    vec4 resultColor = vec4(0.0);\n"
    "    for (int i = 0; i < maxSamples; ++i) {\n"
    "        vec3 uv = (pos - BoundingMin) * invBoundingDiff;\n"
    "        float density = texture(VolumeTexture, uv).r;\n"
    "        if (any(lessThan(uv, SliceMin)) || any(greaterThan(uv, SliceMax))) density = 0.0;\n"
    "        density = (density - ValueRange.x) / ValueRange.y;\n"
    "        density = pow(clamp(density, 0.0, 1.0), DensityPower);\n"

    "        vec4 value = vec4(0.0);\n"
    "        if (TransferMode == 1) value = texture(TransferTexture, density);\n"
    "        else value = vec4(density); value.a = density;\n"
    "        value *= factor; resultColor += T * value;\n"
    "        T *= 1.0 - value.a;\n"
    "        pos += step; if (i == samples - 1 || T < 0.01) break;\n"
    "    }\n"
    "    gl_FragColor = vec4(resultColor.rgb, 1.0 - T);\n"
    "}\n"
};

class VolumeHandler : public osgGA::GUIEventHandler
{
public:
    VolumeHandler(osg::MatrixTransform* mt, osg::StateSet* ss) : _transform(mt), _stateSet(ss) {}
    osg::observer_ptr<osg::MatrixTransform> _transform;
    osg::observer_ptr<osg::StateSet> _stateSet;

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            osg::Vec3 valueMin, valueMax;
            osg::Uniform* sliceMin = _stateSet->getUniform("SliceMin");
            osg::Uniform* sliceMax = _stateSet->getUniform("SliceMax");
            sliceMin->get(valueMin); sliceMax->get(valueMax);

            osg::Vec3 pos, scale; osg::Quat rot, so;
            _transform->getMatrix().decompose(pos, rot, scale, so);

            switch (ea.getKey())
            {
            case osgGA::GUIEventAdapter::KEY_Left:
                if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                    { if (valueMax.x() > 0.0f) valueMax.x() -= 0.01f; sliceMax->set(valueMax); }
                else
                    { if (valueMin.x() > 0.0f) valueMin.x() -= 0.01f; sliceMin->set(valueMin); }
                break;
            case osgGA::GUIEventAdapter::KEY_Right:
                if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                    { if (valueMax.x() < 1.0f) valueMax.x() += 0.01f; sliceMax->set(valueMax); }
                else
                    { if (valueMin.x() < 1.0f) valueMin.x() += 0.01f; sliceMin->set(valueMin); }
                break;
            case osgGA::GUIEventAdapter::KEY_Down:
                if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                    { if (valueMax.y() > 0.0f) valueMax.y() -= 0.01f; sliceMax->set(valueMax); }
                else
                    { if (valueMin.y() > 0.0f) valueMin.y() -= 0.01f; sliceMin->set(valueMin); }
                break;
            case osgGA::GUIEventAdapter::KEY_Up: std::cout << valueMin << "\n";
                if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                    { if (valueMax.y() < 1.0f) valueMax.y() += 0.01f; sliceMax->set(valueMax); }
                else
                    { if (valueMin.y() < 1.0f) valueMin.y() += 0.01f; sliceMin->set(valueMin); }
                break;
            case osgGA::GUIEventAdapter::KEY_Page_Down:
                scale.z() *= 0.95;
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix::translate(pos)); break;
            case osgGA::GUIEventAdapter::KEY_Page_Up:
                scale.z() *= 1.05;
                _transform->setMatrix(osg::Matrix::scale(scale) * osg::Matrix::translate(pos)); break;
            default: return false;
            }
        }
        return false;
    }
};

class MatrixVolumeCallback : public osg::NodeCallback
{
public:
    MatrixVolumeCallback(osg::StateSet* ss, const osg::Vec3& origin, const osg::Vec3& size)
    : _stateset(ss), _origin(origin), _size(size) { _scale.set(1.0f, 1.0f, 1.0f); }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform* mt = static_cast<osg::MatrixTransform*>(node);
        if (mt && _stateset.valid())
        {
            osg::Vec3 pos, s; osg::Quat rot, so;
            mt->getMatrix().decompose(pos, rot, s, so);

            bool dirty = false;
            if (pos != _origin || s != _scale) dirty = true;
            if (dirty)
            {
                osg::Vec3 size = osg::Vec3(_size[0] * s[0], _size[1] * s[1], _size[2] * s[2]);
                size[0] /= _scale[0]; size[1] /= _scale[1]; size[2] /= _scale[2];
                _stateset->getUniform("BoundingMin")->set(_origin);
                _stateset->getUniform("BoundingMax")->set(_origin + size);
                _origin = pos; //_scale = s;
            }
        }
        traverse(node, nv);
    }

protected:
    osg::observer_ptr<osg::StateSet> _stateset;
    osg::Vec3 _origin, _size, _scale;
};

typedef std::pair<osg::MatrixTransform*, osg::StateSet*> ResultPair;
ResultPair createVolumeData(
    osg::Image* image3D, osg::Image* transferImage1D,
    const std::string& vertCode, const std::string& fragCode,
    const osg::Vec3& origin, const osg::Vec3& spacing, float minValue, float maxValue)
{
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

    osg::ref_ptr<osg::Vec3Array> ta = new osg::Vec3Array;
    ta->push_back(osg::Vec3(0.0f, 0.0f, 0.0f));
    ta->push_back(osg::Vec3(1.0f, 0.0f, 0.0f));
    ta->push_back(osg::Vec3(1.0f, 1.0f, 0.0f));
    ta->push_back(osg::Vec3(0.0f, 1.0f, 0.0f));
    ta->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
    ta->push_back(osg::Vec3(1.0f, 0.0f, 1.0f));
    ta->push_back(osg::Vec3(1.0f, 1.0f, 1.0f));
    ta->push_back(osg::Vec3(0.0f, 1.0f, 1.0f));

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
        geom->setUseDisplayList(false);
        geom->setUseVertexBufferObjects(true);
        geom->setVertexArray(va.get());
        geom->setTexCoordArray(0, ta.get());
        geom->addPrimitiveSet(de.get());
        geode->addDrawable(geom.get());
    }

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));

    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    ss->setAttributeAndModes(program.get());
    ss->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
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
    ss->addUniform(new osg::Uniform("BoundingMin", origin));
    ss->addUniform(new osg::Uniform("BoundingMax", origin + size));
    ss->addUniform(new osg::Uniform("ValueRange", osg::Vec2(minValue, maxValue - minValue)));
    ss->addUniform(new osg::Uniform("RayMarchingSamples", (int)128));
    ss->addUniform(new osg::Uniform("DensityFactor", 2.0f));
    ss->addUniform(new osg::Uniform("DensityPower", 2.0f));
    ss->addUniform(new osg::Uniform("SliceMin", osg::Vec3(0.0f, 0.0f, 0.0f)));
    ss->addUniform(new osg::Uniform("SliceMax", osg::Vec3(1.0f, 1.0f, 1.0f)));
#if 0
    ss->addUniform(new osg::Uniform("IsoSurfaceColor", osg::Vec4(1.0f, 0.98f, 0.92f, 1.0f)));
    ss->addUniform(new osg::Uniform("IsoSurfaceValue", 0.6f));
#endif

    // Create scene nodes
    osg::ref_ptr<osg::MatrixTransform> sceneItem = new osg::MatrixTransform;
    sceneItem->addUpdateCallback(new MatrixVolumeCallback(ss, origin, size));
    sceneItem->setMatrix(osg::Matrix::translate(origin));
    sceneItem->addChild(geode.get());
    return ResultPair(sceneItem.release(), ss);
}

osg::Image* createTransferFunction(const std::vector<std::pair<float, osg::Vec4ub>>& colors)
{
    int tfSize = 101, tfPtr = 0;
    osg::Vec4ub* tfValues = new osg::Vec4ub[tfSize];
    for (size_t i = 1; i < colors.size(); ++i)
    {
        int index0 = osg::minimum(tfSize - 1, (int)(colors[i - 1].first * 100.0f));
        int index1 = osg::minimum(tfSize - 1, (int)(colors[i].first * 100.0f));
        osg::Vec4ub color0 = colors[i - 1].second, color1 = colors[i].second;
        for (int j = tfPtr; j < index0; ++j) tfValues[tfPtr++] = color0;

        for (int j = index0; j < index1; ++j)
        {
            float k = float(j - index0) / float(index1 - index0);
            tfValues[tfPtr++] = osg::Vec4ub(color1[0] * k + color0[0] * (1.0f - k),
                color1[1] * k + color0[1] * (1.0f - k),
                color1[2] * k + color0[2] * (1.0f - k),
                color1[3] * k + color0[3] * (1.0f - k));
        }
    }
    tfValues[tfSize - 1] = colors.back().second;

    osg::ref_ptr<osg::Image> image1D = new osg::Image;
    image1D->setImage(tfSize, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
        (unsigned char*)tfValues, osg::Image::USE_NEW_DELETE);
    return image1D.release();
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <image/image_seq file>" << std::endl;
        return 1;
    }
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(argv[1]);
    if (!image) return 1;

    osg::Vec3d origin, spacing(0.1, 0.1, 0.1); osg::ref_ptr<osg::Image> image1D;
#if 0
    std::vector<std::pair<float, osg::Vec4ub>> colors;
    colors.push_back(std::pair<float, osg::Vec4ub>(0.0f, osg::Vec4ub(0, 0, 0, 255)));
    colors.push_back(std::pair<float, osg::Vec4ub>(0.2f, osg::Vec4ub(0, 0, 255, 255)));
    colors.push_back(std::pair<float, osg::Vec4ub>(0.5f, osg::Vec4ub(0, 255, 0, 255)));
    colors.push_back(std::pair<float, osg::Vec4ub>(0.8f, osg::Vec4ub(255, 255, 0, 255)));
    colors.push_back(std::pair<float, osg::Vec4ub>(1.0f, osg::Vec4ub(255, 0, 0, 255)));
    image1D = createTransferFunction(colors);
#endif
    ResultPair pair = createVolumeData(
        image.get(), image1D.get(), vertCode, fragCode, origin, spacing, 0.0f, 1.0f);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(pair.first);
    root->addChild(osgDB::readNodeFile("axes.osgt"));

    osgViewer::Viewer viewer;
    //viewer.getCamera()->setClearColor(osg::Vec4());
    viewer.addEventHandler(new VolumeHandler(pair.first, pair.second));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
