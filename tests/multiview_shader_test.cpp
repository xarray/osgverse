#include <osg/io_utils>
#include <osg/ClipPlane>
#include <osg/Texture2D>
#include <osg/LightSource>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#define NUMVIEWS "16"
#define NV "4, 4"
#define EYESEP "5.0"

void createShaders(osg::StateSet* ss)
{
    static const char* vertSource = {
        "#version 130\n"
        "out vec4 eyeVertex_gs, texCoord0_gs, texCoord1_gs;\n"
        "out vec3 eyeNormal_gs;\n"
        "void main(void)\n"
        "{\n"
        "    eyeVertex_gs = gl_ModelViewMatrix * gl_Vertex;\n"
        "    eyeNormal_gs = normalize(gl_NormalMatrix * gl_Normal);\n"
        "    texCoord0_gs = gl_MultiTexCoord0;\n"
        "    texCoord1_gs = gl_MultiTexCoord1;\n"
        "    gl_FrontColor = gl_Color;\n"
        "    gl_Position = gl_ModelViewMatrix * gl_Vertex;\n"
        "}\n"
    };

    static const char* geomSource = {
        "#version 130\n"
        "#extension GL_EXT_geometry_shader4 : enable\n"
        "in vec4 eyeVertex_gs[], texCoord0_gs[], texCoord1_gs[];\n"
        "in vec3 eyeNormal_gs[];\n"
        "out vec4 eyeVertex, texCoord0, texCoord1;\n"
        "out vec3 eyeNormal;\n"
        "void main(void)\n"
        "{\n"
        "    vec2 NV = vec2(" NV "); \n"
        "    float eyeSep = " EYESEP ";\n"
        "    int numViews = " NUMVIEWS ";\n"

        "    vec2 T = -1.0 + 1.0 / NV;\n"
        "    mat4x4 TSP = mat4x4(1.0 / NV.x, 0.0, 0.0, 0.0,\n"
        "                        0.0, 1.0 / NV.y, 0.0, 0.0,\n"
        "                        0.0, 0.0, 1.0, 0.0,\n"
        "                        T.x, T.y, 0.0, 1.0) * gl_ProjectionMatrix;\n"
        "    vec4 Tv = vec4(-float(numViews / 2) * eyeSep, 0.0, 0.0, 0.0);\n"
        "    if (mod(numViews, 2) == 0) Tv.x += eyeSep * 0.5;\n"

        "    for (int k = 0; k < numViews; ++k) {\n"
        "        int Sx = k % int(NV.x), Sy = int(floor(k / NV.x));\n"
        "        for (int i = 0; i < 3; ++i) {\n"
        "            vec4 tmp = TSP * (Tv + gl_PositionIn[i]);\n"
        "            vec2 coeff = (vec2(2.0) / NV) * tmp.w;\n"
        "            gl_ClipDistance[0] = tmp.x + tmp.w;\n"
        "            gl_ClipDistance[1] = coeff.x - (tmp.x + tmp.w);\n"
        "            gl_ClipDistance[2] = tmp.y + tmp.w;\n"
        "            gl_ClipDistance[3] = coeff.y - (tmp.y + tmp.w);\n"

        "            eyeVertex = eyeVertex_gs[i]; eyeNormal = eyeNormal_gs[i];\n"
        "            texCoord0 = texCoord0_gs[i]; texCoord1 = texCoord1_gs[i];\n"
        "            gl_Position = tmp;\n"
        "            gl_Position.xy += (vec2(float(Sx), float(Sy)) / NV) * tmp.w * 2.0;\n"
        "            EmitVertex();\n"
        "        }\n"
        "        EndPrimitive();\n"
        "        Tv.x += eyeSep;\n"
        "    }\n"
        "}\n"
    };

    static const char* fragSource = {
        "#version 130\n"
        "uniform sampler2D DiffuseMap;\n"
        "in vec4 eyeVertex, texCoord0, texCoord1;\n"
        "in vec3 eyeNormal;\n"

        "void main(void)\n"
        "{\n"
        "    vec3 normalDirection = normalize(eyeNormal);\n"
        "    vec3 viewDirection = -normalize(vec3(eyeVertex));\n"
        "    vec3 lightDirection = vec3(0.0, 0.0, 0.0);\n"
        "    float attenuation = 1.0;\n"
        "    vec3 totalLighting = vec3(gl_LightModel.ambient);\n"

        "    for (int index = 0; index < 1; index++) {\n"
        "        if (gl_LightSource[index].position.w == 0.0) {\n"
        "            attenuation = 1.0;\n"
        "            lightDirection = normalize(vec3(gl_LightSource[index].position));\n"
        "        } else if (gl_LightSource[index].spotCutoff > 90.0) {\n"
        "             vec3 positionToLightSource = vec3(gl_LightSource[index].position - eyeVertex);\n"
        "             attenuation = 1.0 / length(positionToLightSource);\n"
        "             lightDirection = normalize(positionToLightSource);\n"
        "        } else if (gl_LightSource[index].spotCutoff <= 90.0) {\n"
        "            vec3 positionToLightSource = vec3(gl_LightSource[index].position - eyeVertex);\n"
        "            attenuation = 1.0 / length(positionToLightSource);\n"
        "            lightDirection = normalize(positionToLightSource);\n"
        "            float clamped = max(0.0, dot(-lightDirection, gl_LightSource[0].spotDirection));\n"
        "            if (clamped < gl_LightSource[0].spotCosCutoff) attenuation = 0.0;\n"
        "            else attenuation = attenuation * pow(clamped, gl_LightSource[0].spotExponent);\n"
        "        }\n"

        "        vec3 diffuseReflection = attenuation * vec3(gl_LightSource[index].diffuse)\n"
        "                               * max(0.0, dot(normalDirection, lightDirection));\n"
        "        vec3 specularReflection = vec3(0.0);\n"
        "        if (dot(normalDirection, lightDirection) >= 0.0) {\n"
        "            specularReflection = vec3(gl_LightSource[index].specular) *\n"
        "                pow(max(0.0, dot(reflect(-lightDirection, normalDirection), viewDirection)),\n"
        "                    64.0) * attenuation;\n"
        "        }\n"
        "        totalLighting += diffuseReflection + specularReflection;\n"
        "    }\n"
        "    vec4 baseColor = texture2D(DiffuseMap, texCoord0.xy);\n"
        "    gl_FragColor = baseColor * vec4(totalLighting, 1.0);\n"
        "}\n"
    };

    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vertSource));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragSource));
    program->addShader(new osg::Shader(osg::Shader::GEOMETRY, geomSource));
    program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, atoi(NUMVIEWS) * 3);
    program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
    program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLES);
    ss->setAttributeAndModes(program);

    ss->setMode(GL_CLIP_PLANE0, osg::StateAttribute::ON);
    ss->setMode(GL_CLIP_PLANE1, osg::StateAttribute::ON);
    ss->setMode(GL_CLIP_PLANE2, osg::StateAttribute::ON);
    ss->setMode(GL_CLIP_PLANE3, osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("DiffuseMap", (int)0));
}

osg::Texture2D* createDefaultTexture(const osg::Vec4& color)
{
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    image->setInternalTextureFormat(GL_RGBA);

    osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
    *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);

    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
    tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
    tex2D->setImage(image.get()); return tex2D.release();
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene = (argc > 1) ? osgDB::readNodeFile(argv[1])
                                  : osgDB::readNodeFile("cessna.osg");
    createShaders(scene->getOrCreateStateSet());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(scene.get());
    root->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
