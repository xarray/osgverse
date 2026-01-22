#include <osg/io_utils>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ConvertUTF>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <random>

osg::Geode* createColoredCube(float size, const osg::Vec4& color)
{
    float halfSize = size / 2.0f;
    osg::Vec3Array* vertices = new osg::Vec3Array(8);
    (*vertices)[0].set(-halfSize, -halfSize, -halfSize);
    (*vertices)[1].set(halfSize, -halfSize, -halfSize);
    (*vertices)[2].set(halfSize, halfSize, -halfSize);
    (*vertices)[3].set(-halfSize, halfSize, -halfSize);
    (*vertices)[4].set(-halfSize, -halfSize, halfSize);
    (*vertices)[5].set(halfSize, -halfSize, halfSize);
    (*vertices)[6].set(halfSize, halfSize, halfSize);
    (*vertices)[7].set(-halfSize, halfSize, halfSize);

    osg::DrawElementsUInt* indices = new osg::DrawElementsUInt(GL_QUADS, 24);
    (*indices)[0] = 0; (*indices)[1] = 1; (*indices)[2] = 2; (*indices)[3] = 3;
    (*indices)[4] = 5; (*indices)[5] = 4; (*indices)[6] = 7; (*indices)[7] = 6;
    (*indices)[8] = 4; (*indices)[9] = 0; (*indices)[10] = 3; (*indices)[11] = 7;
    (*indices)[12] = 1; (*indices)[13] = 5; (*indices)[14] = 6; (*indices)[15] = 2;
    (*indices)[16] = 4; (*indices)[17] = 5; (*indices)[18] = 1; (*indices)[19] = 0;
    (*indices)[20] = 3; (*indices)[21] = 2; (*indices)[22] = 6; (*indices)[23] = 7;

    osg::Vec4Array* colors = new osg::Vec4Array;
    for (size_t i = 0; i < 8; ++i) colors->push_back(color);

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(indices);

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geometry);
    return geode;
}

osg::Group* createMoireGrid(int gridSize, float cubeSize, float spacing)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> colorDist(0.3, 1.0);

    osg::Group* gridGroup = new osg::Group;
    for (int i = 0; i < gridSize; ++i)
    {
        for (int j = 0; j < gridSize; ++j)
        {
            osg::MatrixTransform* transform = new osg::MatrixTransform();
            float x = (i - gridSize / 2.0f) * (cubeSize + spacing);
            float y = (j - gridSize / 2.0f) * (cubeSize + spacing), z = 0.0f;

            std::uniform_real_distribution<> offsetDist(-0.001, 0.001); z += offsetDist(gen);
            transform->setMatrix(osg::Matrix::translate(x, y, z));

            float r = colorDist(gen), g = colorDist(gen), b = colorDist(gen);
            osg::Geode* cube = createColoredCube(cubeSize, osg::Vec4(r, g, b, 1.0f));
            transform->addChild(cube);
            gridGroup->addChild(transform);
        }
    }
    return gridGroup;
}

osg::Group* createDepthConflictLayer(int layers, float cubeSize, float layerSpacing)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> colorDist(0.3, 1.0);
    std::uniform_real_distribution<> offsetDist(-0.001, 0.001);

    osg::Group* layerGroup = new osg::Group;
    for (int layer = 0; layer < layers; ++layer)
    {
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                float x = (i - 1) * (cubeSize * 2.0f);
                float y = (j - 1) * (cubeSize * 2.0f);
                float z = layer * layerSpacing;
                x += offsetDist(gen); y += offsetDist(gen); z += offsetDist(gen);

                osg::MatrixTransform* transform = new osg::MatrixTransform;
                transform->setMatrix(osg::Matrix::translate(x, y, z));

                float ratio = (float)layer / layers;
                float r = ratio * colorDist(gen);
                float g = (1.0 - ratio) * colorDist(gen);
                float b = 0.5 * colorDist(gen);

                osg::Geode* cube = createColoredCube(cubeSize, osg::Vec4(r, g, b, 1.0f));
                //cube->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
                //cube->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                transform->addChild(cube);
                layerGroup->addChild(transform);
            }
        }
    }
    return layerGroup;
}

osg::Geode* createPlane(float distance, int index)
{
    osg::Geometry* geom = osg::createTexturedQuadGeometry(
        osg::Vec3(-5, -5, distance), osg::Vec3(10, 0, 0), osg::Vec3(0, 10, 0));

    float ratio = (float)index / 15.0f; osg::Vec4 color(ratio, 1.0 - ratio, 0.5, 1.0);
    osg::Vec4Array* colors = new osg::Vec4Array();
    for (size_t i = 0; i < 4; ++i) colors->push_back(color);
    geom->setColorArray(colors);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geom);
    return geode;
}

// https://outerra.blogspot.com/2012/11/maximizing-depth-buffer-range-and.html
const char* vertCode = R"(
    uniform float logDepth_Far;
    uniform float logDepth_Near;
    varying vec4 color;
    varying float logDepth, hcoff;

    void main() {
        color = gl_Color;
        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

        float FCOEF = 2.0 / log2(logDepth_Far + 1.0);
        gl_Position.z = log2(max(logDepth_Near, 1.0 + gl_Position.w)) * FCOEF - 1.0;
        logDepth = 1.0 + gl_Position.w; hcoff = (0.5 * FCOEF);
    }
)";

const char* fragCode = R"(
    varying vec4 color;
    varying float logDepth, hcoff;

    void main() {
        gl_FragColor = color;
        gl_FragDepth = log2(logDepth) * hcoff;
    }
)";

class LogDepthMatrixCallback : public osg::Camera::ClampProjectionMatrixCallback
{
public:
    LogDepthMatrixCallback(osg::Camera* cam) : _camera(cam) {}

    virtual bool clampProjectionMatrixImplementation(osg::Matrixf& projection, double& znear, double& zfar) const
    {
        double ratio = _camera.valid() ? _camera->getNearFarRatio() : 0.00001;
        bool success = osg::clampProjectionMatrix(projection, znear, zfar, ratio);
        if (success) updateConstants(znear, zfar); return success;
    }

    virtual bool clampProjectionMatrixImplementation(osg::Matrixd& projection, double& znear, double& zfar) const
    {
        double ratio = _camera.valid() ? _camera->getNearFarRatio() : 0.00001;
        bool success = osg::clampProjectionMatrix(projection, znear, zfar, ratio);
        if (success) updateConstants(znear, zfar); return success;
    }

protected:
    void updateConstants(double near, double far) const
    {
        osg::StateSet* ss = _camera->getStateSet();
        osg::Uniform* uniformF = ss->getOrCreateUniform("logDepth_Far", osg::Uniform::FLOAT);
        osg::Uniform* uniformN = ss->getOrCreateUniform("logDepth_Near", osg::Uniform::FLOAT);
        uniformF->set((float)far); uniformN->set((float)near);
    }

    osg::observer_ptr<osg::Camera> _camera;
};

static void setupFloatingPointDepthBuffer(osg::Camera* camera)
{
    osg::Texture2D* depthTexture = new osg::Texture2D;
    depthTexture->setTextureSize(1920, 1080);
    depthTexture->setInternalFormat(GL_DEPTH_COMPONENT32F);
    depthTexture->setSourceFormat(GL_DEPTH_COMPONENT);
    depthTexture->setSourceType(GL_FLOAT);
    depthTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    depthTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    depthTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
    depthTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
    camera->attach(osg::Camera::DEPTH_BUFFER, depthTexture);
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    osg::DisplaySettings::instance()->setNumMultiSamples(2);

    osg::Group* grid = createMoireGrid(50, 0.1f, 0.02f);
    osg::Group* layers = createDepthConflictLayer(10, 0.3f, 0.01f);

    osg::MatrixTransform* layerTransform = new osg::MatrixTransform();
    layerTransform->setMatrix(osg::Matrix::translate(5.0f, 0.0f, 0.0f));
    layerTransform->addChild(layers);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(grid);
    root->addChild(layerTransform);
    root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osgViewer::Viewer viewer;
    viewer.getCamera()->setProjectionMatrixAsPerspective(30.0, 1.0, 0.01, 10000.0);
    if (arguments.read("--log-z"))
    {
        osg::Program* prog = new osg::Program;
        prog->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode));
        prog->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));
        root->getOrCreateStateSet()->setAttribute(prog);

        setupFloatingPointDepthBuffer(viewer.getCamera());
        viewer.getCamera()->setClampProjectionMatrixCallback(new LogDepthMatrixCallback(viewer.getCamera()));
    }

    if (!arguments.read("--auto-compute"))
    {
        osg::StateSet* ss = viewer.getCamera()->getStateSet();
        ss->getOrCreateUniform("logDepth_Far", osg::Uniform::FLOAT)->set(10000.0f);
        ss->getOrCreateUniform("logDepth_Near", osg::Uniform::FLOAT)->set(0.01f);
        viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    }

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewInWindow(50, 50, 960, 600);
    viewer.realize();
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
