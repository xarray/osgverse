#include <osg/io_utils>
#include <osg/PatchParameter>
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

osg::Program* createProgram(const char* vert, const char* frag)
{
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, vert);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, frag);
    osg::Program* program = new osg::Program;
    program->addShader(vs); program->addShader(fs);
    return program;
}

#define PATCH_SIZE 64
#define UV_SCALE 1.0f
osg::Node* prepareHeightMap(osg::Image* heightMap, const std::vector<osg::ref_ptr<osg::Image>>& images)
{
    const uint32_t vertexCount = PATCH_SIZE * PATCH_SIZE;
    const float wx = 2.0f, wy = 2.0f;

    osg::Vec3Array* va = new osg::Vec3Array(vertexCount);
    osg::Vec3Array* na = new osg::Vec3Array(vertexCount);
    osg::Vec2Array* ta = new osg::Vec2Array(vertexCount);
    for (int x = 0; x < PATCH_SIZE; x++)
    {
        for (int y = 0; y < PATCH_SIZE; y++)
        {
            uint32_t index = (x + y * PATCH_SIZE);
            (*va)[index][0] = x * wx + wx / 2.0f - (float)PATCH_SIZE * wx / 2.0f;
            (*va)[index][1] = y * wy + wy / 2.0f - (float)PATCH_SIZE * wy / 2.0f;
            (*va)[index][2] = 0.0f;
            (*ta)[index] = osg::Vec2((float)x / PATCH_SIZE, (float)y / PATCH_SIZE) * UV_SCALE;
        }
    }

    uint16_t* heightData = (uint16_t*)heightMap->data();
    uint32_t dim = heightMap->s();
    uint32_t scale = dim / PATCH_SIZE;
    for (int x = 0; x < PATCH_SIZE; x++)
    {
        for (int y = 0; y < PATCH_SIZE; y++)
        {
            // Get height samples centered around current position
            float heights[3][3];
            for (int hx = -1; hx <= 1; hx++)
            {
                for (int hy = -1; hy <= 1; hy++)
                {
                    int idX = (x + hx) * scale, idY = (y + hy) * scale;
                    idX = std::max(0, std::min(idX, (int)dim - 1)) / scale;
                    idY = std::max(0, std::min(idY, (int)dim - 1)) / scale;
                    float h = *(heightData + (idX + idY * dim) * scale) / 65535.0f;
                    heights[hx + 1][hy + 1] = h;
                }
            }

            // Calculate the normal
            osg::Vec3 normal;
            normal.x() = heights[0][0] - heights[2][0] + 2.0f * heights[0][1]
                       - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
            normal.y() = heights[0][0] + 2.0f * heights[1][0] + heights[2][0]
                       - heights[0][2] - 2.0f * heights[1][2] - heights[2][2];
            normal.z() = 0.25f * sqrt(1.0f - normal.x() * normal.x() - normal.y() * normal.y());
            (*na)[x + y * PATCH_SIZE] = osg::Vec3(normal[0] * 2.0f, normal[1] * 2.0f, normal[2]);
            (*na)[x + y * PATCH_SIZE].normalize();
        }
    }

    // Indices
    const uint32_t w = (PATCH_SIZE - 1);
    const uint32_t indexCount = w * w * 4;
    uint32_t* indices = new uint32_t[indexCount];
    for (int x = 0; x < w; x++)
    {
        for (int y = 0; y < w; y++)
        {
            uint32_t index = (x + y * w) * 4;
            indices[index] = (x + y * PATCH_SIZE);
            indices[index + 1] = indices[index] + PATCH_SIZE;
            indices[index + 2] = indices[index + 1] + 1;
            indices[index + 3] = indices[index] + 1;
        }
    }

    // Geometry
    osg::Geometry* geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va);
    geom->setTexCoordArray(0, ta);
    geom->setNormalArray(na);
    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(
        new osg::DrawElementsUInt(osg::PrimitiveSet::PATCHES, indices, indices + indexCount));

    // Statesets
    osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

#if !defined(VERSE_ENABLE_MTT) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
    heightMap->setDataType(GL_UNSIGNED_SHORT);
    heightMap->setPixelFormat(GL_LUMINANCE);
    heightMap->setInternalTextureFormat(GL_LUMINANCE16);
#endif
    tex->setImage(heightMap);

    osg::ref_ptr<osg::Texture2DArray> texArray = new osg::Texture2DArray;
    texArray->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    texArray->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    for (size_t i = 0; i < images.size(); ++i) texArray->setImage(i, images[i].get());

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geom);
    geode->getOrCreateStateSet()->addUniform(new osg::Uniform("texHeight", (int)0));
    geode->getOrCreateStateSet()->addUniform(new osg::Uniform("texArray", (int)1));
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get());
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(1, texArray.get());

    const char* vert = {
        "#version 130\n"
        "out vec3 vecNormal;\n"
        "out vec2 vecUV;\n"
        "void main() {\n"
        "    vecUV = gl_MultiTexCoord0.xy;\n"
        "    vecNormal = gl_Normal;\n"
        "    gl_Position = gl_Vertex;\n"
        "}\n"
    };

    const char* tesc = {
        "#version 400\n"
        "uniform mat4 modelView, projection;\n"
        "uniform vec2 viewportDim;\n"
        "uniform float tessellationFactor;\n"
        "layout(vertices = 4) out;\n"
        "in vec3 vecNormal[];\n"
        "in vec2 vecUV[];\n"
        "out vec3 ctrlNormal[4];\n"
        "out vec2 ctrlUV[4];\n"

        "float screenSpaceTessFactor(vec4 p0, vec4 p1) {\n"
        "    vec4 midPoint = 0.5 * (p0 + p1);\n"
        "    float radius = distance(p0, p1) / 2.0;\n"
        "    vec4 v0 = modelView * midPoint;\n"

        "    vec4 clip0 = (projection * (v0 - vec4(radius, vec3(0.0))));\n"
        "    vec4 clip1 = (projection * (v0 + vec4(radius, vec3(0.0))));\n"
        "    clip0 /= clip0.w; clip1 /= clip1.w;\n"
        "    clip0.xy *= viewportDim; clip1.xy *= viewportDim;\n"
        "    return clamp(distance(clip0, clip1) / 20.0 * tessellationFactor, 1.0, 64.0);\n"
        "}\n"

        "void main() {\n"
        "    if (gl_InvocationID == 0) {\n"
        "        if (tessellationFactor > 0.0f) {\n"
        "           gl_TessLevelOuter[0] = screenSpaceTessFactor(gl_in[3].gl_Position, gl_in[0].gl_Position);\n"
        "           gl_TessLevelOuter[1] = screenSpaceTessFactor(gl_in[0].gl_Position, gl_in[1].gl_Position);\n"
        "           gl_TessLevelOuter[2] = screenSpaceTessFactor(gl_in[1].gl_Position, gl_in[2].gl_Position);\n"
        "           gl_TessLevelOuter[3] = screenSpaceTessFactor(gl_in[2].gl_Position, gl_in[3].gl_Position);\n"
        "           gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);\n"
        "           gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);\n"
        "       } else {\n"
        "           gl_TessLevelInner[0] = 1.0; gl_TessLevelInner[1] = 1.0;\n"
        "           gl_TessLevelOuter[0] = 1.0; gl_TessLevelOuter[1] = 1.0;\n"
        "           gl_TessLevelOuter[2] = 1.0; gl_TessLevelOuter[3] = 1.0;\n"
        "       }\n"
        "   }\n"

        "   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
        "   ctrlNormal[gl_InvocationID] = vecNormal[gl_InvocationID];\n"
        "   ctrlUV[gl_InvocationID] = vecUV[gl_InvocationID];\n"
        "}\n"
    };

    const char* tese = {
        "#version 400\n"
        "uniform mat4 modelView, projection;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec2 viewportDim;\n"
        "uniform sampler2D texHeight;\n"
        "layout(quads, equal_spacing, cw) in;\n"
        "in vec3 ctrlNormal[];\n"
        "in vec2 ctrlUV[];\n"
        "out vec3 outNormal;\n"
        "out vec2 outUV;\n"
        "out vec3 outViewVec;\n"
        "out vec3 outLightVec;\n"
        "out vec3 outEyePos;\n"
        "out vec3 outWorldPos;\n"

        "void main() {\n"
        "    vec2 uv1 = mix(ctrlUV[0], ctrlUV[1], gl_TessCoord.x);\n"
        "    vec2 uv2 = mix(ctrlUV[3], ctrlUV[2], gl_TessCoord.x);\n"
        "    outUV = mix(uv1, uv2, gl_TessCoord.y);\n"
        "    vec3 n1 = mix(ctrlNormal[0], ctrlNormal[1], gl_TessCoord.x);\n"
        "    vec3 n2 = mix(ctrlNormal[3], ctrlNormal[2], gl_TessCoord.x);\n"
        "    outNormal = mix(n1, n2, gl_TessCoord.y);\n"

        "    vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);\n"
        "    vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);\n"
        "    vec4 pos = mix(pos1, pos2, gl_TessCoord.y);\n"
        "    pos.z += textureLod(texHeight, outUV, 0.0).r * 32.0;\n"
        "    gl_Position = projection * modelView * pos;\n"

        "    outViewVec = -pos.xyz; outWorldPos = pos.xyz;\n"
        "    outLightVec = normalize(lightPos.xyz + outViewVec);\n"
        "    outEyePos = vec3(modelView * pos);\n"
        "}\n"
    };

    const char* frag = {
        "#version 400\n"
        "uniform sampler2D texHeight;\n"
        "uniform sampler2DArray texArray;\n"
        "in vec3 outNormal;\n"
        "in vec2 outUV;\n"
        "in vec3 outViewVec;\n"
        "in vec3 outLightVec;\n"
        "in vec3 outEyePos;\n"
        "in vec3 outWorldPos;\n"
        "out vec4 outFragColor;\n"

        "vec3 sampleTerrainLayer() {\n"
        "    vec2 layers[6]; vec3 color = vec3(0.0);\n"
        "    layers[0] = vec2(-10.0, 10.0); layers[1] = vec2(5.0, 45.0);\n"
        "    layers[2] = vec2(45.0, 80.0); layers[3] = vec2(75.0, 100.0);\n"
        "    layers[4] = vec2(95.0, 140.0); layers[5] = vec2(140.0, 190.0);\n"
        "    float height = textureLod(texHeight, outUV, 0.0).r * 255.0;\n"
        "    for (int i = 0; i < 6; i++) {\n"
        "        float range = layers[i].y - layers[i].x;\n"
        "        float weight = (range - abs(height - layers[i].y)) / range;\n"
        "        weight = max(0.0, weight);\n"
        "        color += weight * texture(texArray, vec3(outUV * 16.0, i)).rgb;\n"
        "    }\n"
        "    return color;\n"
        "}\n"

        "float fog(float density) {\n"
        "    const float LOG2 = -1.442695;\n"
        "    float dist = gl_FragCoord.z / gl_FragCoord.w * 0.05;\n"
        "    float d = density * dist;\n"
        "    return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);\n"
        "}\n"

        "void main() {\n"
        "    vec3 N = normalize(outNormal);\n"
        "    vec3 L = normalize(outLightVec);\n"
        "    vec3 ambient = vec3(0.5), diffuse = max(dot(N, L), 0.0) * vec3(1.0);\n"
        "    vec4 color = vec4((ambient + diffuse) * sampleTerrainLayer(), 1.0);\n"
        "    const vec4 fogColor = vec4(0.47, 0.5, 0.67, 0.0);\n"
        "    outFragColor = mix(color, fogColor, fog(0.25));\n"
        "}\n"
    };

    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vert));
    program->addShader(new osg::Shader(osg::Shader::TESSCONTROL, tesc));
    program->addShader(new osg::Shader(osg::Shader::TESSEVALUATION, tese));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, frag));
    geode->getOrCreateStateSet()->setAttributeAndModes(program);
    geode->getOrCreateStateSet()->setAttributeAndModes(new osg::PatchParameter(4));
    return geode;
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

    node->getOrCreateStateSet()->setAttributeAndModes(createProgram(
        "varying vec2 UV;\n"
        "void main() {\n"
        "    UV = gl_MultiTexCoord0.xy;\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n",

        "uniform sampler2D tex2D;\n"
        "varying vec2 UV;\n"
        "void main() {\n"
        "    vec4 color = texture(tex2D, UV);\n"
        "    gl_FragColor = color;\n"
        "}\n"));

    osg::MatrixTransform* mt = new osg::MatrixTransform;
    mt->setMatrix(osg::Matrix::scale(1000.0f, 1000.0f, 1000.0f) *
                  osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    mt->addChild(node);
    return mt;
}

class LogicHandler : public osgGA::GUIEventHandler
{
public:
    osg::ref_ptr<osgText::Text> _text;
    osg::observer_ptr<osg::Group> _root;

    LogicHandler(osg::Group* root) : _root(root)
    {
        _text = new osgText::Text;
        _text->setText("Tesselation mode.");
        _text->setPosition(osg::Vec3(10.0f, 10.0f, 0.0f));
        _text->setCharacterSize(20.0f);
        _text->setColor(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f)); 
        root->getOrCreateStateSet()->addUniform(new osg::Uniform("tessellationFactor", 0.75f));

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
        osg::StateSet* ss = _root->getOrCreateStateSet();
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Left || ea.getKey() == 'z')
            {
                _text->setText("Disable tesselation mode.");
                ss->getOrCreateUniform("tessellationFactor", osg::Uniform::FLOAT)->set(0.0f);
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Right || ea.getKey() == 'x')
            {
                _text->setText("Tesselation mode. Value = 0.75");
                ss->getOrCreateUniform("tessellationFactor", osg::Uniform::FLOAT)->set(0.75f);
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
            osg::Camera* cam = view->getCamera();

            ss->getOrCreateUniform("modelView", osg::Uniform::FLOAT_MAT4)->set(
                osg::Matrixf(cam->getViewMatrix()));
            ss->getOrCreateUniform("projection", osg::Uniform::FLOAT_MAT4)->set(
                osg::Matrixf(cam->getProjectionMatrix()));
            ss->getOrCreateUniform("viewportDim", osg::Uniform::FLOAT_VEC2)->set(
                osg::Vec2(ea.getWindowWidth(), ea.getWindowHeight()));
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
    std::vector<osg::ref_ptr<osg::Image>> terrainImages = osgVerse::loadKtx(
        BASE_DIR "/models/ExampleData/terrain_texturearray_rgba.ktx");
    std::vector<osg::ref_ptr<osg::Image>> terrainHeight = osgVerse::loadKtx(
        BASE_DIR "/models/ExampleData/terrain_heightmap_r16.ktx");
    
    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(prepareHeightMap(terrainHeight[0].get(), terrainImages));
    root->addChild(prepareSkySphere(skySphere.get(), skyImages));
    root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

    osgViewer::Viewer viewer;
    viewer.getCamera()->setClearColor(osg::Vec4());
    viewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    viewer.addEventHandler(new LogicHandler(root.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    
    viewer.getCameraManipulator()->setHomePosition(
        osg::Vec3(3.45756, -86.8474, 31.1284), osg::Vec3(3.47222, -85.8838, 30.8612), osg::Z_AXIS);
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
