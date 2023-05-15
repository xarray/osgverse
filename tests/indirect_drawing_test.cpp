#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/MatrixTransform>
#include <osg/PrimitiveSetIndirect>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgText/Text>
#include <osgUtil/SmoothingVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <readerwriter/LoadSceneGLTF.h>
#include <readerwriter/LoadTextureKTX.h>
#include <pipeline/Global.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>
#include <random>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#define INSTANCE_COUNT 16384
struct InstanceData
{
    osg::Vec3 pos, rot;
    float scale;
    uint32_t texIndex;
};

osg::Program* createProgram(const char* vert, const char* frag)
{
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, vert);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, frag);
    osg::Program* program = new osg::Program;
    program->addShader(vs); program->addShader(fs);
    return program;
}

osg::Geometry* getGeometry(osg::Node* node)
{
    if (node->asGeode() && node->asGeode()->getNumDrawables() > 0)
    {
        osg::Drawable* drawable = node->asGeode()->getDrawable(0);
        return drawable->asGeometry();
    }

    if (node->asGroup() && node->asGroup()->getNumChildren() > 0)
        return getGeometry(node->asGroup()->getChild(0));
    return NULL;
}

osg::Node* prepareInstances(osg::Node* rock,
                            const std::vector<osg::ref_ptr<osg::Image>>& images, bool indirect)
{
    std::vector<InstanceData> instanceData(INSTANCE_COUNT);
    std::default_random_engine rndGenerator(0/*(unsigned int)time(nullptr)*/);
    std::uniform_real_distribution<float> uniformDist(0.0, 1.0);
    std::uniform_int_distribution<uint32_t> rndTextureIndex(0, images.size());

    // Distribute rocks randomly on two different rings
    const int numInstances = INSTANCE_COUNT / 2;
    for (int i = 0; i < numInstances; ++i)
    {
        osg::Vec2 ring0(7.0f, 11.0f), ring1(14.0f, 18.0f);
        float rho = 0.0f, theta = 0.0f;

        // Inner ring
        rho = sqrt((pow(ring0[1], 2.0f) - pow(ring0[0], 2.0f)) * uniformDist(rndGenerator)
                  + pow(ring0[0], 2.0f));
        theta = 2.0 * osg::PI * uniformDist(rndGenerator);
        instanceData[i].pos = osg::Vec3(
                rho * cos(theta), uniformDist(rndGenerator) * 0.5f - 0.25f, rho * sin(theta));
        instanceData[i].rot = osg::Vec3(
                osg::PI * uniformDist(rndGenerator), osg::PI * uniformDist(rndGenerator),
                osg::PI * uniformDist(rndGenerator));
        instanceData[i].scale = 1.5f + uniformDist(rndGenerator) - uniformDist(rndGenerator);
        instanceData[i].texIndex = rndTextureIndex(rndGenerator);
        instanceData[i].scale *= 0.045f;

        // Outer ring
        rho = sqrt((pow(ring1[1], 2.0f) - pow(ring1[0], 2.0f)) * uniformDist(rndGenerator)
                  + pow(ring1[0], 2.0f));
        theta = 2.0 * osg::PI * uniformDist(rndGenerator);
        instanceData[i + numInstances].pos = osg::Vec3(
                rho * cos(theta), uniformDist(rndGenerator) * 0.5f - 0.25f, rho * sin(theta));
        instanceData[i + numInstances].rot = osg::Vec3(
                osg::PI * uniformDist(rndGenerator), osg::PI * uniformDist(rndGenerator),
                osg::PI * uniformDist(rndGenerator));
        instanceData[i + numInstances].scale = 1.5f + uniformDist(rndGenerator) - uniformDist(rndGenerator);
        instanceData[i + numInstances].texIndex = rndTextureIndex(rndGenerator);
        instanceData[i + numInstances].scale *= 0.035f;
    }

    osg::Geometry* originalGeom = getGeometry(rock);
    osg::Geode* geode = new osg::Geode;
    osg::BoundingSphere bs;
    if (indirect)
    {
        osg::Vec3Array* va0 = static_cast<osg::Vec3Array*>(originalGeom->getVertexArray());
        osg::Vec3Array* na0 = static_cast<osg::Vec3Array*>(originalGeom->getNormalArray());
        osg::Vec2Array* ta0 = static_cast<osg::Vec2Array*>(originalGeom->getTexCoordArray(0));
        osg::DrawElementsUShort* de0 =
            static_cast<osg::DrawElementsUShort*>(originalGeom->getPrimitiveSet(0));

        osg::Vec3Array* va = new osg::Vec3Array;
        osg::Vec3Array* na = new osg::Vec3Array;
        osg::Vec2Array* ta = new osg::Vec2Array;
        osg::Vec4Array* att0 = new osg::Vec4Array;
        osg::Vec4Array* att1 = new osg::Vec4Array;

        osg::MultiDrawElementsIndirectUInt* mde = new osg::MultiDrawElementsIndirectUInt(GL_TRIANGLES);
        osg::DefaultIndirectCommandDrawElements* dide =
            static_cast<osg::DefaultIndirectCommandDrawElements*>(mde->getIndirectCommandArray());
        for (size_t i = 0; i < instanceData.size(); ++i)
        {
            InstanceData& id = instanceData[i];
            bs.expandBy(instanceData[i].pos);

            osg::DrawElementsIndirectCommand cmd;
            cmd.count = de0->size(); cmd.instanceCount = 1;
            cmd.firstIndex = va->size(); cmd.baseVertex = va->size();
            dide->push_back(cmd);

            va->insert(va->end(), va0->begin(), va0->end());
            na->insert(na->end(), na0->begin(), na0->end());
            ta->insert(ta->end(), ta0->begin(), ta0->end());
            att0->insert(att0->end(), va0->size(), osg::Vec4(id.pos, id.scale));
            att1->insert(att1->end(), va0->size(), osg::Vec4(id.rot, (float)id.texIndex));
            mde->insert(mde->end(), de0->begin(), de0->end());
        }

        osg::Geometry* newGeom = new osg::Geometry;
        newGeom->setUseDisplayList(false);
        newGeom->setUseVertexBufferObjects(true);
        newGeom->setVertexArray(va);
        newGeom->setTexCoordArray(0, ta);
        newGeom->setNormalArray(na);
        newGeom->setNormalBinding(originalGeom->getNormalBinding());
        newGeom->setVertexAttribArray(6, att0);
        newGeom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
        newGeom->setVertexAttribNormalize(6, false);
        newGeom->setVertexAttribArray(7, att1);
        newGeom->setVertexAttribBinding(7, osg::Geometry::BIND_PER_VERTEX);
        newGeom->setVertexAttribNormalize(7, false);
        newGeom->addPrimitiveSet(mde);
        geode->addChild(newGeom);

        osg::Program* prog = createProgram(
            "uniform float osg_SimulationTime;\n"
            "attribute vec4 dataPosScale, dataRotIndex;\n"
            "varying vec3 EyeNormal, ViewVector, LightVector;\n"
            "varying vec3 UV;;\n"
            "void main() {\n"
            "    float lt = osg_SimulationTime * 0.5, gt = osg_SimulationTime * 0.05;"
            "    vec3 dataPosition = dataPosScale.xyz, dataRotation = dataRotIndex.xyz;"
            "    float dataScale = dataPosScale.w;\n"

            "    mat3 mx, my, mz;\n"
            "    float s = sin(dataRotation.x + lt), c = cos(dataRotation.x + lt);\n"  // rot X
            "    mx[0] = vec3(c, s, 0.0); mx[1] = vec3(-s, c, 0.0); mx[2] = vec3(0.0, 0.0, 1.0);\n"
            "    s = sin(dataRotation.y + lt); c = cos(dataRotation.y + lt);\n"  // rot Y
            "    my[0] = vec3(c, 0.0, s); my[1] = vec3(0.0, 1.0, 0.0); my[2] = vec3(-s, 0.0, c);\n"
            "    s = sin(dataRotation.z + lt); c = cos(dataRotation.z + lt);\n"  // rot Z
            "    mz[0] = vec3(1.0, 0.0, 0.0); mz[1] = vec3(0.0, c, s); mz[2] = vec3(0.0, -s, c);\n"
            "    mat3 lRotMat = mz * my * mx; mat4 gRotMat;\n"
            "    s = sin(dataRotation.y + gt); c = cos(dataRotation.y + gt);\n"
            "    gRotMat[0] = vec4(c, 0.0, s, 0.0);\n"
            "    gRotMat[1] = vec4(0.0, 1.0, 0.0, 0.0);\n"
            "    gRotMat[2] = vec4(-s, 0.0, c, 0.0);\n"
            "    gRotMat[3] = vec4(0.0, 0.0, 0.0, 1.0);\n"
            "    vec4 pos = vec4((gl_Vertex.xyz * lRotMat) * dataScale + dataPosition, 1.0);\n"

            "    UV = vec3(gl_MultiTexCoord0.xy, dataRotIndex.w);\n"
            "    EyeNormal = mat3(gl_ModelViewMatrix * gRotMat) * inverse(lRotMat) * gl_Normal;\n"
            "    ViewVector = -(gl_ModelViewMatrix * pos).xyz;\n"
            "    LightVector = gl_LightSource[0].position.xyz + ViewVector;\n"
            "    gl_Position = gl_ModelViewProjectionMatrix * gRotMat * pos;\n"
            "}\n",

            "uniform sampler2DArray texArray;\n"
            "varying vec3 EyeNormal, ViewVector, LightVector;\n"
            "varying vec3 UV;\n"
            "void main() {\n"
            "    vec4 color = texture(texArray, vec3(UV));\n"
            "    vec3 N = normalize(EyeNormal);\n"
            "    vec3 L = normalize(LightVector);\n"
            "    vec3 V = normalize(ViewVector);\n"
            "    vec3 R = reflect(-L, N);\n"
            "    vec3 diffuse = max(dot(N, L), 0.1);\n"
            "    vec3 specular = (dot(N, L) > 0.0) ? pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75) * color.r\n"
            "                  : vec3(0.0);\n"
            "    gl_FragColor = vec4(diffuse * color.rgb + specular, color.a);\n"
            "}\n");
        prog->addBindAttribLocation("dataPosScale", 6);
        prog->addBindAttribLocation("dataRotIndex", 7);
        geode->getOrCreateStateSet()->setAttributeAndModes(prog);
    }
    else
    {
        for (size_t i = 0; i < instanceData.size(); ++i)
        {
            InstanceData& id = instanceData[i];
            bs.expandBy(instanceData[i].pos);

            osg::Geometry* newGeom = new osg::Geometry;
            newGeom->setUseDisplayList(false);
            newGeom->setUseVertexBufferObjects(true);
            newGeom->setVertexArray(originalGeom->getVertexArray());
            newGeom->setTexCoordArray(0, originalGeom->getTexCoordArray(0));
            newGeom->setNormalArray(originalGeom->getNormalArray());
            newGeom->setNormalBinding(originalGeom->getNormalBinding());
            newGeom->addPrimitiveSet(originalGeom->getPrimitiveSet(0));
            newGeom->getOrCreateStateSet()->addUniform(new osg::Uniform("dataPosition", id.pos));
            newGeom->getOrCreateStateSet()->addUniform(new osg::Uniform("dataRotation", id.rot));
            newGeom->getOrCreateStateSet()->addUniform(new osg::Uniform("dataScale", id.scale));
            newGeom->getOrCreateStateSet()->addUniform(new osg::Uniform("texIndex", (int)id.texIndex));
            geode->addChild(newGeom);
        }

        geode->getOrCreateStateSet()->setAttributeAndModes(createProgram(
            "uniform vec3 dataPosition, dataRotation;\n"
            "uniform float dataScale, osg_SimulationTime;\n"
            "varying vec3 EyeNormal, ViewVector, LightVector;\n"
            "varying vec2 UV;\n"
            "void main() {\n"
            "    float lt = osg_SimulationTime * 0.5, gt = osg_SimulationTime * 0.05;"
            "    mat3 mx, my, mz;\n"
            "    float s = sin(dataRotation.x + lt), c = cos(dataRotation.x + lt);\n"  // rot X
            "    mx[0] = vec3(c, s, 0.0); mx[1] = vec3(-s, c, 0.0); mx[2] = vec3(0.0, 0.0, 1.0);\n"
            "    s = sin(dataRotation.y + lt); c = cos(dataRotation.y + lt);\n"  // rot Y
            "    my[0] = vec3(c, 0.0, s); my[1] = vec3(0.0, 1.0, 0.0); my[2] = vec3(-s, 0.0, c);\n"
            "    s = sin(dataRotation.z + lt); c = cos(dataRotation.z + lt);\n"  // rot Z
            "    mz[0] = vec3(1.0, 0.0, 0.0); mz[1] = vec3(0.0, c, s); mz[2] = vec3(0.0, -s, c);\n"
            "    mat3 lRotMat = mz * my * mx; mat4 gRotMat;\n"
            "    s = sin(dataRotation.y + gt); c = cos(dataRotation.y + gt);\n"
            "    gRotMat[0] = vec4(c, 0.0, s, 0.0);\n"
            "    gRotMat[1] = vec4(0.0, 1.0, 0.0, 0.0);\n"
            "    gRotMat[2] = vec4(-s, 0.0, c, 0.0);\n"
            "    gRotMat[3] = vec4(0.0, 0.0, 0.0, 1.0);\n"
            "    vec4 pos = vec4((gl_Vertex.xyz * lRotMat) * dataScale + dataPosition, 1.0);\n"

            "    UV = gl_MultiTexCoord0.xy;\n"
            "    EyeNormal = mat3(gl_ModelViewMatrix * gRotMat) * inverse(lRotMat) * gl_Normal;\n"
            "    ViewVector = -(gl_ModelViewMatrix * pos).xyz;\n"
            "    LightVector = gl_LightSource[0].position.xyz + ViewVector;\n"
            "    gl_Position = gl_ModelViewProjectionMatrix * gRotMat * pos;\n"
            "}\n",

            "uniform sampler2DArray texArray;\n"
            "uniform int texIndex;\n"
            "varying vec3 EyeNormal, ViewVector, LightVector;\n"
            "varying vec2 UV;\n"
            "void main() {\n"
            "    vec4 color = texture(texArray, vec3(UV, (float)texIndex));\n"
            "    vec3 N = normalize(EyeNormal);\n"
            "    vec3 L = normalize(LightVector);\n"
            "    vec3 V = normalize(ViewVector);\n"
            "    vec3 R = reflect(-L, N);\n"
            "    vec3 diffuse = max(dot(N, L), 0.1);\n"
            "    vec3 specular = (dot(N, L) > 0.0) ? pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75) * color.r\n"
            "                  : vec3(0.0);\n"
            "    gl_FragColor = vec4(diffuse * color.rgb + specular, color.a);\n"
            "}\n"));
    }

    osg::ref_ptr<osg::Texture2DArray> texArray = new osg::Texture2DArray;
    for (size_t i = 0; i < images.size(); ++i) texArray->setImage(i, images[i].get());
    geode->getOrCreateStateSet()->addUniform(new osg::Uniform("texArray", (int)0));
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, texArray.get());
    geode->setInitialBound(bs);
    return geode;
}

osg::Node* preparePlanet(osg::Node* planet, const std::vector<osg::ref_ptr<osg::Image>>& images)
{
    osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    tex->setImage(images[0].get());
    planet->getOrCreateStateSet()->addUniform(new osg::Uniform("tex2D", (int)0));
    planet->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get());

    planet->getOrCreateStateSet()->setAttributeAndModes(createProgram(
        "varying vec3 EyeNormal, ViewVector, LightVector;\n"
        "varying vec2 UV;\n"
        "void main() {\n"
        "    UV = gl_MultiTexCoord0.xy;\n"
        "    EyeNormal = gl_NormalMatrix * gl_Normal;\n"
        "    ViewVector = -(gl_ModelViewMatrix * gl_Vertex).xyz;\n"
        "    LightVector = gl_LightSource[0].position.xyz + ViewVector;\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n",

        "uniform sampler2D tex2D;\n"
        "varying vec3 EyeNormal, ViewVector, LightVector;\n"
        "varying vec2 UV;\n"
        "void main() {\n"
        "    vec4 color = texture(tex2D, UV) * 1.5;\n"
        "    vec3 N = normalize(EyeNormal);\n"
        "    vec3 L = normalize(LightVector);\n"
        "    vec3 V = normalize(ViewVector);\n"
        "    vec3 R = reflect(-L, N);\n"
        "    vec3 diffuse = max(dot(N, L), 0.1);\n"
        "    vec3 specular = pow(max(dot(R, V), 0.0), 4.0) * vec3(0.5) * color.r;\n"
        "    gl_FragColor = vec4(diffuse * color.rgb + specular, color.a);\n"
        "}\n"));
    return planet;
}

class LogicHandler : public osgGA::GUIEventHandler
{
public:
    osg::observer_ptr<osg::Node> _rocks0;
    osg::observer_ptr<osg::Node> _rocks1;
    osg::ref_ptr<osgText::Text> _text;

    LogicHandler(osg::Group* root, osg::Node* r0, osg::Node* r1)
        : _rocks0(r0), _rocks1(r1)
    {
        _text = new osgText::Text;
        _text->setText("Indirect drawing mode.");
        _text->setPosition(osg::Vec3(10.0f, 10.0f, 0.0f));
        _text->setCharacterSize(50.0f);
        _text->setColor(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));

        osg::Camera* camera = new osg::Camera;
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1280, 0, 1024));
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setClearMask(GL_DEPTH_BUFFER_BIT);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setAllowEventFocus(false);
        camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        osg::Geode* geode = new osg::Geode; geode->addChild(_text.get());
        camera->addChild(geode); root->addChild(camera);
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Left || ea.getKey() == 'z')
            {
                _text->setText("Multiple draw-call mode.");
                _rocks0->setNodeMask(0xffffffff); _rocks1->setNodeMask(0);
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Right || ea.getKey() == 'x')
            {
                _text->setText("Indirect drawing mode.");
                _rocks0->setNodeMask(0); _rocks1->setNodeMask(0xffffffff);
            }
        }
        return false;
    }
};

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> planet = osgVerse::loadGltf(
        BASE_DIR "/models/ExampleData/lavaplanet.gltf", false);
    osg::ref_ptr<osg::Node> rock = osgVerse::loadGltf(
        BASE_DIR "/models/ExampleData/rock01.gltf", false);
    std::vector<osg::ref_ptr<osg::Image>> planetImages = osgVerse::loadKtx(
        BASE_DIR "/models/ExampleData/lavaplanet_rgba.ktx");
    std::vector<osg::ref_ptr<osg::Image>> rockImages = osgVerse::loadKtx(
        BASE_DIR "/models/ExampleData/texturearray_rocks_rgba.ktx");
    
    osg::Node* rocks0 = prepareInstances(rock.get(), rockImages, false);
    osg::Node* rocks1 = prepareInstances(rock.get(), rockImages, true);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(preparePlanet(planet.get(), planetImages));
    root->addChild(rocks0); rocks0->setNodeMask(0);
    root->addChild(rocks1); rocks1->setNodeMask(0xffffffff);
    root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

    osgViewer::Viewer viewer;
    viewer.getCamera()->setClearColor(osg::Vec4());
    viewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    viewer.addEventHandler(new LogicHandler(root.get(), rocks0, rocks1));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
