#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Utilities.h>
#include <animation/TweenAnimation.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

// Triplanar-mapping implementation
const char* commonVert = {
    "uniform mat4 osg_ViewMatrixInverse;\n"
    "uniform float localOrWorld;\n"
    "varying vec3 worldNormal, eyeNormal;\n"
    "varying vec4 worldVec, eyeVec;\n"
    "void main(void) {\n"
    "    mat4 worldMatrix = (localOrWorld < 0.0) ? mat4(1.0) : osg_ViewMatrixInverse * gl_ModelViewMatrix;\n"
    "    worldNormal = (worldMatrix * vec4(gl_Normal, 0.0)).xyz;\n"
    "    worldVec = worldMatrix * gl_Vertex;\n"
    "    eyeNormal = vec3(gl_NormalMatrix * gl_Normal);\n"
    "    eyeVec = gl_ModelViewMatrix * gl_Vertex;\n"
    "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "}\n"
};

const char* commonFrag = {
    "uniform sampler2D baseTexture, topTexture;\n"
    "uniform float localOrWorld;\n"
    "varying vec3 worldNormal, eyeNormal;\n"
    "varying vec4 worldVec, eyeVec;\n"

    "vec4 computeLight() {\n"
    "    vec3 N = normalize(eyeNormal);\n"
    "    vec3 E = normalize(-eyeVec.xyz);\n"
    "    vec3 L = gl_LightSource[0].position.w>0.0 ? normalize(gl_LightSource[0].position.xyz - eyeVec.xyz)\n"
    "                                              : normalize(gl_LightSource[0].position.xyz);\n"
    "    vec4 ambient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;\n"
    "    vec4 diffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse;\n"
    "    vec4 specular = gl_LightSource[0].specular * gl_FrontMaterial.specular;\n"
    "    float shininess = gl_FrontMaterial.shininess;\n"
    "    float lambertTerm = dot(N, L);\n"
    "    if (lambertTerm > 0.0) {\n"
    "        vec3 H = normalize(L + E);\n"
    "        float specularFactor = pow(max(dot(H, N), 0.0), shininess);\n"
    "        return vec4(vec3(ambient + diffuse * lambertTerm + specular * specularFactor), diffuse.w);\n"
    "    }\n"
    "    return vec4(ambient.rgb, diffuse.w);\n"
    "}\n"

    "vec3 blendNormal(vec3 normal) {\n"
    "    vec3 blending = abs(normal);\n"
    "    blending = normalize(max(blending, vec3(0.00001)));\n"
    "    blending /= vec3(blending.x + blending.y + blending.z);\n"
    "    return blending;\n"
    "}\n"

    "vec3 triplanarMapping(sampler2D t0, sampler2D t1, sampler2D t2, vec3 normal, vec3 position,\n"
    "                      float scale) {\n"
    "    vec3 normalBlend = blendNormal(normal);\n"
    "    vec3 xColor = texture2D(t0, position.yz * scale).rgb;\n"
    "    vec3 yColor = texture2D(t1, position.xz * scale).rgb;\n"
    "    vec3 zColor = texture2D(t2, position.xy * scale).rgb;\n"
    "    return (xColor * normalBlend.x + yColor * normalBlend.y + zColor * normalBlend.z);\n"
    "}\n"

    "void main(void) {\n"
    "    float scale = abs(localOrWorld);\n"
    "    vec3 rgb = triplanarMapping(baseTexture, baseTexture, topTexture, normalize(worldNormal),\n"
    "                                worldVec.xyz / worldVec.w, scale);\n"
    "    vec4 color = vec4(rgb, 1.0) * computeLight();\n"
    "    gl_FragColor = color;\n"
    "}\n"
};

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(osgDB::readNodeFile("cow.osg"));

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, commonVert));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, commonFrag));

    osg::StateSet* ss = root->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(
        0, osgVerse::createTexture2D(osgDB::readImageFile("Images/blueFlowers.png")),
        osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    ss->setTextureAttributeAndModes(
        1, osgVerse::createTexture2D(osgDB::readImageFile("Images/purpleFlowers.png")),
        osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    ss->setAttributeAndModes(program.get());
    ss->addUniform(new osg::Uniform("baseTexture", (int)0));
    ss->addUniform(new osg::Uniform("topTexture", (int)1));
    ss->addUniform(new osg::Uniform("localOrWorld", 1.0f));

    // Quick test convenient animation functions
    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    handler->addKeyUpCallback('s', [&](int key) {
        osgVerse::doMove(root.get(), osg::Vec3d(0.0, 0.0, -5.0), 1.0, false, true); });
    handler->addKeyUpCallback('w', [&](int key) {
        osgVerse::doMove(root.get(), osg::Vec3d(0.0, 0.0, 5.0), 1.0, false, true); });
    handler->addKeyUpCallback('a', [&](int key) {
        osgVerse::doMove(root.get(), osg::Vec3d(-5.0, 0.0, 0.0), 1.0, false, true); });
    handler->addKeyUpCallback('d', [&](int key) {
        osgVerse::doMove(root.get(), osg::Vec3d(5.0, 0.0, 0.0), 1.0, false, true); });
    handler->addKeyUpCallback('x', [&](int key) {
        osgVerse::doMove(root.get(), osg::Vec3d(0.0, 0.0, 0.0), 1.0, false, false,
            []() { std::cout << "Back to home position." << std::endl; });
    });

    osgViewer::Viewer viewer;
    viewer.addEventHandler(handler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
