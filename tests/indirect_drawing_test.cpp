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

#define INSTANCE_COUNT 30000
struct InstanceData
{
    osg::Vec3 pos, rot;
    float scale;
    uint32_t texIndex;
};

osg::Program* createProgram(const char* name, const char* vert, const char* frag)
{
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, vert);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, frag);
    osg::Program* program = new osg::Program;
    program->addShader(vs); program->addShader(fs);
    program->setName(name);
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

        osg::DrawElementsIndirectUInt* mde = new osg::DrawElementsIndirectUInt(GL_TRIANGLES);
        osg::DefaultIndirectCommandDrawElements* dide =
            static_cast<osg::DefaultIndirectCommandDrawElements*>(mde->getIndirectCommandArray());
        for (size_t i = 0; i < instanceData.size(); ++i)
        {
            InstanceData& id = instanceData[i];
            bs.expandBy(instanceData[i].pos);

            va->insert(va->end(), va0->begin(), va0->end());
            na->insert(na->end(), na0->begin(), na0->end());
            ta->insert(ta->end(), ta0->begin(), ta0->end());
            att0->insert(att0->end(), va0->size(), osg::Vec4(id.pos, id.scale));
            att1->insert(att1->end(), va0->size(), osg::Vec4(id.rot, (float)id.texIndex));
            
            size_t vIndex = va->size() - va0->size();
            for (size_t n = 0; n < de0->size(); ++n) mde->push_back((*de0)[n] + vIndex);
            //mde->insert(mde->end(), de0->begin(), de0->end());
        }
        
        osg::DrawElementsIndirectCommand cmd;
        cmd.count = mde->size(); cmd.instanceCount = 1;
        cmd.firstIndex = 0; cmd.baseVertex = 0;
        dide->push_back(cmd);

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

        osg::Program* prog = createProgram("IndirectProgram",
            "#version 400\n"
            "uniform mat4 osg_ModelViewMatrix, osg_ModelViewProjectionMatrix;\n"
            "uniform mat3 osg_NormalMatrix;\n"
            "in vec4 osg_Vertex, osg_MultiTexCoord0;\n"
            "in vec3 osg_Normal;\n"
            "uniform float osg_SimulationTime;\n"
            "in vec4 dataPosScale, dataRotIndex;\n"
            "out vec3 EyeNormal, ViewVector, LightVector;\n"
            "out vec3 UV;\n"
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
            "    vec4 pos = vec4((osg_Vertex.xyz * lRotMat) * dataScale + dataPosition, 1.0);\n"

            "    UV = vec3(osg_MultiTexCoord0.xy, dataRotIndex.w);\n"
            "    EyeNormal = mat3(osg_ModelViewMatrix * gRotMat) * inverse(lRotMat) * osg_Normal;\n"
            "    ViewVector = -(osg_ModelViewMatrix * pos).xyz;\n"
            "    LightVector = vec3(1.0, 0.5, 0.5) + ViewVector;\n"
            "    gl_Position = osg_ModelViewProjectionMatrix * gRotMat * pos;\n"
            "}\n",

            "#version 400\n"
            "uniform sampler2DArray texArray;\n"
            "in vec3 EyeNormal, ViewVector, LightVector;\n"
            "in vec3 UV;\nout vec4 fragColor;\n"
            "void main() {\n"
            "    vec4 color = texture(texArray, vec3(UV));\n"
            "    vec3 N = normalize(EyeNormal);\n"
            "    vec3 L = normalize(LightVector);\n"
            "    vec3 V = normalize(ViewVector);\n"
            "    vec3 R = reflect(-L, N);\n"
            "    vec3 diffuse = vec3(max(dot(N, L), 0.1));\n"
            "    vec3 specular = (dot(N, L) > 0.0) ? pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75) * color.r\n"
            "                  : vec3(0.0);\n"
            "    fragColor = vec4(diffuse * color.rgb + specular, color.a);\n"
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

        geode->getOrCreateStateSet()->setAttributeAndModes(createProgram("RegularProgram",
            "#version 400\n"
            "uniform mat4 osg_ModelViewMatrix, osg_ModelViewProjectionMatrix;\n"
            "uniform mat3 osg_NormalMatrix;\n"
            "in vec4 osg_Vertex, osg_MultiTexCoord0;\n"
            "in vec3 osg_Normal;\n"
            "uniform vec3 dataPosition, dataRotation;\n"
            "uniform float dataScale, osg_SimulationTime;\n"
            "out vec3 EyeNormal, ViewVector, LightVector;\n"
            "out vec2 UV;\n"
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
            "    vec4 pos = vec4((osg_Vertex.xyz * lRotMat) * dataScale + dataPosition, 1.0);\n"

            "    UV = osg_MultiTexCoord0.xy;\n"
            "    EyeNormal = mat3(osg_ModelViewMatrix * gRotMat) * inverse(lRotMat) * osg_Normal;\n"
            "    ViewVector = -(osg_ModelViewMatrix * pos).xyz;\n"
            "    LightVector = vec3(1.0, 0.5, 0.5) + ViewVector;\n"
            "    gl_Position = osg_ModelViewProjectionMatrix * gRotMat * pos;\n"
            "}\n",

            "#version 400\n"
            "uniform sampler2DArray texArray;\n"
            "uniform int texIndex;\n"
            "in vec3 EyeNormal, ViewVector, LightVector;\n"
            "in vec2 UV;\nout vec4 fragColor;\n"
            "void main() {\n"
            "    vec4 color = texture(texArray, vec3(UV, float(texIndex)));\n"
            "    vec3 N = normalize(EyeNormal);\n"
            "    vec3 L = normalize(LightVector);\n"
            "    vec3 V = normalize(ViewVector);\n"
            "    vec3 R = reflect(-L, N);\n"
            "    vec3 diffuse = vec3(max(dot(N, L), 0.1));\n"
            "    vec3 specular = (dot(N, L) > 0.0) ? pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75) * color.r\n"
            "                  : vec3(0.0);\n"
            "    fragColor = vec4(diffuse * color.rgb + specular, color.a);\n"
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

    planet->getOrCreateStateSet()->setAttributeAndModes(createProgram("Planet",
        "#version 400\n"
        "uniform mat4 osg_ModelViewMatrix, osg_ModelViewProjectionMatrix;\n"
        "uniform mat3 osg_NormalMatrix;\n"
        "in vec4 osg_Vertex, osg_MultiTexCoord0;\n"
        "in vec3 osg_Normal;\n"
        "out vec3 EyeNormal, ViewVector, LightVector;\n"
        "out vec2 UV;\n"
        "void main() {\n"
        "    UV = osg_MultiTexCoord0.xy;\n"
        "    EyeNormal = osg_NormalMatrix * osg_Normal;\n"
        "    ViewVector = -(osg_ModelViewMatrix * osg_Vertex).xyz;\n"
        "    LightVector = vec3(1.0, 0.5, 0.5) + ViewVector;\n"
        "    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;\n"
        "}\n",

        "#version 400\n"
        "uniform sampler2D tex2D;\n"
        "in vec3 EyeNormal, ViewVector, LightVector;\n"
        "in vec2 UV;\nout vec4 fragColor;\n"
        "void main() {\n"
        "    vec4 color = texture2D(tex2D, UV) * 1.5;\n"
        "    vec3 N = normalize(EyeNormal);\n"
        "    vec3 L = normalize(LightVector);\n"
        "    vec3 V = normalize(ViewVector);\n"
        "    vec3 R = reflect(-L, N);\n"
        "    vec3 diffuse = vec3(max(dot(N, L), 0.1));\n"
        "    vec3 specular = pow(max(dot(R, V), 0.0), 4.0) * vec3(0.5) * color.r;\n"
        "    fragColor = vec4(diffuse * color.rgb + specular, color.a);\n"
        "}\n"));
    return planet;
}

osg::Node* prepareSkySphere(osg::Node* node, const std::vector<osg::ref_ptr<osg::Image>>& images)
{
    osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    tex->setImage(images[0].get());
    node->getOrCreateStateSet()->addUniform(new osg::Uniform("tex2D", (int)0));
    node->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get());
    node->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode);

    node->getOrCreateStateSet()->setAttributeAndModes(createProgram("SkySphere",
        "varying vec2 UV;\n"
        "void main() {\n"
        "    UV = gl_MultiTexCoord0.xy;\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n",

        "#version 400\n"
        "uniform sampler2D tex2D;\n"
        "varying vec2 UV;\n"
        "void main() {\n"
        "    vec4 color = texture(tex2D, UV);\n"
        "    gl_FragColor = color * 0.01;\n"
        "}\n"));

    osg::MatrixTransform* mt = new osg::MatrixTransform;
    mt->setMatrix(osg::Matrix::scale(1000.0f, 1000.0f, 1000.0f));
    mt->addChild(node);
    return mt;
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
        _text->setCharacterSize(20.0f);
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
                _text->dirtyDisplayList();
                _rocks0->setNodeMask(0xffffffff); _rocks1->setNodeMask(0);
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Right || ea.getKey() == 'x')
            {
                _text->setText("Indirect drawing mode.");
                _text->dirtyDisplayList();
                _rocks0->setNodeMask(0); _rocks1->setNodeMask(0xffffffff);
            }
        }
        return false;
    }
};

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> skySphere = osgVerse::loadGltf(
        BASE_DIR "/models/ExampleData/sphere.gltf", false);
    std::vector<osg::ref_ptr<osg::Image>> skyImages = osgVerse::loadKtx(
        BASE_DIR "/models/ExampleData/skysphere_rgba.ktx");
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
    root->addChild(prepareSkySphere(skySphere.get(), skyImages));
    root->addChild(preparePlanet(planet.get(), planetImages));
    root->addChild(rocks0); rocks0->setNodeMask(0);
    root->addChild(rocks1); rocks1->setNodeMask(0xffffffff);
    //root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

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
    
    viewer.getCameraManipulator()->setHomePosition(
        osg::Vec3(-6.62242, -10.131, -38.5468), osg::Vec3(-6.46025, -9.8792, -37.5927), -osg::Y_AXIS);
    viewer.getCameraManipulator()->home(0.0);
    while (!viewer.done())
    {
        //osg::Vec3 eye, center, up;
        //viewer.getCamera()->getViewMatrixAsLookAt(eye, center, up);
        //std::cout << eye << "; " << center << "; " << up << "\n";
        viewer.frame();
    }
    return 0;
}
