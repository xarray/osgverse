#include <osg/io_utils>
#include <osg/ClipPlane>
#include <osg/Texture2D>
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

#define NUMVIEWS "12"
#define NV "4, 3"
#define EYESEP "5.0"

void createShaders(osg::StateSet* ss)
{
    static const char* vertSource = {
        "#version 130\n"
        "#extension GL_EXT_geometry_shader4 : enable\n"
        "varying vec4 v_color;\n"
        "varying vec3 v_normal;\n"
        "void main(void)\n"
        "{\n"
        "    v_color = gl_Color;\n"
        "    v_normal = gl_NormalMatrix * gl_Normal;\n"
        "    gl_Position = gl_ModelViewMatrix * gl_Vertex;\n"
        "}\n"
    };

    static const char* geomSource = {
        "#version 130\n"
        "#extension GL_EXT_geometry_shader4 : enable\n"
        "uniform mat4 osg_ViewMatrixInverse;\n"
        "varying in vec4 v_color[];\n"
        "varying in vec3 v_normal[];\n"
        "varying out vec4 v_color_out;\n"
        "void main(void)\n"
        "{\n"
        "    vec2 NV = vec2(" NV "); \n"
        "    float eyeSep = " EYESEP "f;\n"
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
        "            v_color_out = vec4(v_normal[i], 1.0);\n"
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
        "#extension GL_EXT_geometry_shader4 : enable\n"
        "varying vec4 v_color_out;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = v_color_out;\n"
        "}\n"
    };

    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vertSource));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragSource));
    program->addShader(new osg::Shader(osg::Shader::GEOMETRY, geomSource));
    program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, atoi(NUMVIEWS) * 3);
    program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
    program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
    ss->setAttributeAndModes(program);
    ss->setMode(GL_CLIP_PLANE0, osg::StateAttribute::ON);
    ss->setMode(GL_CLIP_PLANE1, osg::StateAttribute::ON);
    ss->setMode(GL_CLIP_PLANE2, osg::StateAttribute::ON);
    ss->setMode(GL_CLIP_PLANE3, osg::StateAttribute::ON);
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile("cessna.osg");
    createShaders(scene->getOrCreateStateSet());

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(scene.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
