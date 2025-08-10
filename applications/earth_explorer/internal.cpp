#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <pipeline/Pipeline.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

const char* innerVertCode = {
    "uniform mat4 osg_ViewMatrix, osg_ViewMatrixInverse; \n"
    "VERSE_VS_OUT vec3 normalInWorld; \n"
    "VERSE_VS_OUT vec4 vertexInProj, earthCenter; \n"
    "VERSE_VS_OUT vec4 texCoord, color; \n"

    "void main() {\n"
    "    mat4 modelMatrix = osg_ViewMatrixInverse * VERSE_MATRIX_MV; \n"
    "    vertexInProj = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "    earthCenter = VERSE_MATRIX_MVP * vec4(0.0, 0.0, 0.0, 1.0);\n"
    "    normalInWorld = normalize(vec3(osg_ViewMatrixInverse * vec4(VERSE_MATRIX_N * gl_Normal, 0.0))); \n"
    "    texCoord = osg_MultiTexCoord0; color = osg_Color; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* innerFragCode = {
    "uniform sampler2D earthMaskSampler;\n"
    "uniform float osg_SimulationTime; \n"
    "uniform float globalOpaque;\n"
    "VERSE_FS_IN vec3 normalInWorld; \n"
    "VERSE_FS_IN vec4 vertexInProj, earthCenter; \n"
    "VERSE_FS_IN vec4 texCoord, color; \n"

    "#ifdef VERSE_GLES3\n"
    "layout(location = 0) VERSE_FS_OUT vec4 fragColor;\n"
    "layout(location = 1) VERSE_FS_OUT vec4 fragOrigin;\n"
    "#endif\n"

    ///////////////////// https://www.shadertoy.com/view/4dXGR4
    "float snoise(vec3 uv, float res) {\n"
    "    const vec3 s = vec3(1e0, 1e2, 1e4); uv *= res; \n"
    "    vec3 uv0 = floor(mod(uv, res)) * s; \n"
    "    vec3 uv1 = floor(mod(uv + vec3(1.), res)) * s; \n"
    "    vec3 f = fract(uv); f = f * f * (3.0 - 2.0 * f); \n"
    "    vec4 v = vec4(uv0.x + uv0.y + uv0.z, uv1.x + uv0.y + uv0.z, \n"
    "                  uv0.x + uv1.y + uv0.z, uv1.x + uv1.y + uv0.z); \n"
    "    vec4 r = fract(sin(v * 1e-3) * 1e5); \n"
    "    float r0 = mix(mix(r.x, r.y, f.x), mix(r.z, r.w, f.x), f.y); \n"
    "    r = fract(sin((v + uv1.z - uv0.z) * 1e-3) * 1e5); \n"
    "    float r1 = mix(mix(r.x, r.y, f.x), mix(r.z, r.w, f.x), f.y); \n"
    "    return mix(r0, r1, f.z) * 2. - 1.; \n"
    "}\n"

    "vec4 mainImage(vec2 uv) {\n"
    "    float brightness = 0.1; \n"
    "    float radius = 0.24 + brightness * 0.2; \n"
    "    float invRadius = 1.0 / radius; \n"
    "    vec3 orange = vec3(0.8, 0.65, 0.3); \n"
    "    vec3 orangeRed = vec3(0.8, 0.35, 0.1); \n"

    "    float time = osg_SimulationTime * 0.1, aspect = 16.0 / 9.0; \n"
    "    vec2 off = earthCenter.xy * 0.5 / earthCenter.w + vec2(0.5);\n"
    "    vec2 p = uv + vec2(off.x, 1.0 - off.y) - vec2(1.0); p.x *= aspect; \n"
    "    float fade = pow(length(2.0 * p), 0.5); \n"
    "    float fVal1 = 1.0 - fade, fVal2 = 1.0 - fade; \n"
    "    float angle = atan(p.x, p.y) / 6.2832, dist = length(p); \n"
    "    vec3 coord = vec3(angle, dist, time * 0.1); \n"

    "    float newTime1 = abs(snoise(coord + vec3(0.0, -time * (0.35 + brightness * 0.001), time * 0.015), 15.0)); \n"
    "    float newTime2 = abs(snoise(coord + vec3(0.0, -time * (0.15 + brightness * 0.001), time * 0.015), 45.0)); \n"
    "    for (int i = 1; i <= 7; i++) {\n"
    "        float power = pow(2.0, float(i + 1)); \n"
    "        fVal1 += (0.5 / power) * snoise(coord + vec3(0.0, -time, time * 0.2), (power * (10.0) * (newTime1 + 1.0))); \n"
    "        fVal2 += (0.5 / power) * snoise(coord + vec3(0.0, -time, time * 0.2), (power * (25.0) * (newTime2 + 1.0))); \n"
    "    }\n"

    "    float corona = pow(fVal1 * max(1.1 - fade, 0.0), 2.0) * 50.0; \n"
    "    corona += pow(fVal2 * max(1.1 - fade, 0.0), 2.0) * 50.0; corona *= 1.2 - newTime1; \n"
    "    vec3 sphereNormal = vec3(0.0, 0.0, 1.0), dir = vec3(0.0); \n"
    "    vec3 center = vec3(0.5, 0.5, 1.0), starSphere = vec3(0.0); \n"

    "    vec2 sp = -1.0 + 2.0 * uv; sp.x *= aspect; sp *= (2.0 - brightness); \n"
    "    float r = dot(sp, sp); float f = (1.0 - sqrt(abs(1.0 - r))) / (r)+brightness * 0.5; \n"
    "    if (dist < radius) {\n"
    "        vec2 newUv; newUv.x = sp.x * f; newUv.y = sp.y * f; \n"
    "        newUv += vec2(time, 0.0); corona *= pow(dist * invRadius, 24.0); \n"

    "        vec3 texSample = vec3(0.0);// texture(iChannel0, newUv).rgb;\n"
    "        float uOff = (texSample.g * brightness * 4.5 + time); \n"
    "        vec2 starUV = newUv + vec2(uOff, 0.0); \n"
    "        starSphere = vec3(0.0);// texture(iChannel0, starUV).rgb;\n"
    "    }\n"

    "    float starGlow = min(max(1.0 - dist * (1.0 - brightness), 0.0), 1.0); \n"
    "    vec4 outColor; outColor.a = 1.0; \n"
    "    outColor.rgb = /*vec3(f * (0.75 + brightness * 0.3) * orange) + */starSphere + \n"
    "                   corona * orange + starGlow * orangeRed; \n"
    "    return outColor; \n"
    "}\n"
    /////////////////////

    "void main() {\n"
    "    vec2 uv = (vertexInProj.xy / vertexInProj.w) * 0.5 + 0.5;\n"
    "    vec4 maskColor = VERSE_TEX2D(earthMaskSampler, uv);\n"
    "    vec4 finalColor = mainImage(uv);\n"
    "    finalColor.a = maskColor.r * globalOpaque;\n"
    "#ifdef VERSE_GLES3\n"
    "    fragColor = finalColor; \n"
    "    fragOrigin = vec4(1.0); \n"
    "#else\n"
    "    gl_FragData[0] = finalColor; \n"
    "    gl_FragData[1] = vec4(1.0); \n"
    "#endif\n"
    "}\n"
};

osg::Node* configureInternal(osgViewer::View& viewer, osg::Node* earth, osg::Texture* sceneMaskTex, unsigned int mask)
{
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, innerVertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, innerFragCode);
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("Inner_VS"); program->addShader(vs);
    fs->setName("Inner_FS"); program->addShader(fs);
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME

    osg::ref_ptr<osg::Geode> innerRoot = new osg::Geode;
    //innerRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    innerRoot->getOrCreateStateSet()->setTextureAttributeAndModes(0, sceneMaskTex);
    innerRoot->getOrCreateStateSet()->addUniform(new osg::Uniform("earthMaskSampler", (int)0));
    innerRoot->getOrCreateStateSet()->setAttributeAndModes(program.get());
    //innerRoot->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT));
    innerRoot->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    innerRoot->setNodeMask(mask);

    double d = osg::WGS_84_RADIUS_EQUATOR * 0.9;
    innerRoot->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), d)));

    //viewer.addEventHandler(new InternalHandler(clip0.get(), clip1.get(), clip2.get()));
    return innerRoot.release();
}
