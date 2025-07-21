#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <picojson.h>
#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/DatabasePager.h>
#include <pipeline/Pipeline.h>
#include <ui/ImGui.h>
#include <ui/ImGuiComponents.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

#define UI_ARGS osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* handler, osgVerse::ImGuiComponentBase* ctrl
#define SETUP_SLIDER_V1(uniform) \
    osgVerse::Slider* s = static_cast<osgVerse::Slider*>(ctrl); uniform->set((float)s->value);
#define SETUP_SLIDER_V3(uniform, num) \
    osgVerse::Slider* s = static_cast<osgVerse::Slider*>(ctrl); \
    osg::Vec3 ori; uniform->get(ori); ori[num] = s->value; uniform->set(ori);
static osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;

class AdjusterHandler : public osgVerse::ImGuiContentHandler
{
public:
    AdjusterHandler(std::map<std::string, osg::Uniform*>& uniforms, const std::string& jsonFile)
    {
        std::ifstream json(jsonFile.c_str(), std::ios::in); _jsonFile = jsonFile;
        picojson::value root; std::string err = picojson::parse(root, json);
        float exp = 0.25f, sun = 100.0f;
        float bri = 1.0f, sat = 1.0f, con = 1.0f, cr = 0.0f, mg = 0.0f, yb = 0.0f;
        if (err.empty())
        {
            exp = (float)root.get("exposure").get<double>();
            sun = (float)root.get("sun_intensity").get<double>();
            bri = (float)root.get("brightness").get<double>();
            sat = (float)root.get("saturation").get<double>();
            con = (float)root.get("contrast").get<double>();
            cr = (float)root.get("cyan_red").get<double>();
            mg = (float)root.get("magenta_green").get<double>();
            yb = (float)root.get("yellow_blue").get<double>();
        }

        _exposure = new osgVerse::Slider("Exposure");
        _intensity = new osgVerse::Slider("Sun Intensity");
        _brightness = new osgVerse::Slider("Brightness");
        _saturation = new osgVerse::Slider("Saturation");
        _contrast = new osgVerse::Slider("Contrast");
        _cyanRed = new osgVerse::Slider("Cyan/Red");
        _magentaGreen = new osgVerse::Slider("Magenta/Green");
        _yellowBlue = new osgVerse::Slider("Yellow/Blue");

        osg::Uniform* exposure = uniforms["exposure"]; exposure->set(exp);
        osg::Uniform* intensity = uniforms["sun_intensity"]; intensity->set(sun);
        osg::Uniform* colorAttr = uniforms["color_attributes"]; colorAttr->set(osg::Vec3(bri, sat, con));
        osg::Uniform* colorBal = uniforms["color_balance"]; colorBal->set(osg::Vec3(cr, mg, yb));

        setupSlider(*_exposure, exp, 0.0f, 1.0f, [exposure](UI_ARGS) { SETUP_SLIDER_V1(exposure) });
        setupSlider(*_intensity, sun, 0.0f, 1000.0f, [intensity](UI_ARGS) { SETUP_SLIDER_V1(intensity) });
        setupSlider(*_brightness, bri, 0.0f, 3.0f, [colorAttr](UI_ARGS) { SETUP_SLIDER_V3(colorAttr, 0) });
        setupSlider(*_saturation, sat, 0.0f, 3.0f, [colorAttr](UI_ARGS) { SETUP_SLIDER_V3(colorAttr, 1) });
        setupSlider(*_contrast, con, 0.0f, 3.0f, [colorAttr](UI_ARGS) { SETUP_SLIDER_V3(colorAttr, 2) });
        setupSlider(*_cyanRed, cr, -1.0f, 1.0f, [colorBal](UI_ARGS) { SETUP_SLIDER_V3(colorBal, 0) });
        setupSlider(*_magentaGreen, mg, -1.0f, 1.0f, [colorBal](UI_ARGS) { SETUP_SLIDER_V3(colorBal, 1) });
        setupSlider(*_yellowBlue, yb, -1.0f, 1.0f, [colorBal](UI_ARGS) { SETUP_SLIDER_V3(colorBal, 2) });

        _save = new osgVerse::Button("Save Config");
        _save->callback = [jsonFile, uniforms](UI_ARGS)
        {
            std::map<std::string, osg::Uniform*> un = uniforms;
            float exp = 0.0f; un["exposure"]->get(exp);
            float sun = 0.0f; un["sun_intensity"]->get(sun);
            osg::Vec3 colorAttr; un["color_attributes"]->get(colorAttr);
            osg::Vec3 colorBal; un["color_balance"]->get(colorBal);

            picojson::object root;
            root["exposure"] = picojson::value((double)exp);
            root["sun_intensity"] = picojson::value((double)sun);
            root["brightness"] = picojson::value((double)colorAttr[0]);
            root["saturation"] = picojson::value((double)colorAttr[1]);
            root["contrast"] = picojson::value((double)colorAttr[2]);
            root["cyan_red"] = picojson::value((double)colorBal[0]);
            root["magenta_green"] = picojson::value((double)colorBal[1]);
            root["yellow_blue"] = picojson::value((double)colorBal[2]);

            std::string jsonData = picojson::value(root).serialize(true);
            std::ofstream json(jsonFile.c_str(), std::ios::out);
            json.write(jsonData.data(), jsonData.size()); json.close();
        };

        _uniformWindow = new osgVerse::Window("Uniforms##adjuster");
        _uniformWindow->pos = osg::Vec2(0.0f, 0.02f);
        _uniformWindow->size = osg::Vec2(0.15f, 0.96f);
        _uniformWindow->alpha = 0.9f;
        _uniformWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
        _uniformWindow->userData = this;
    }

    virtual void runInternal(osgVerse::ImGuiManager* mgr)
    {
        ImGui::PushFont(ImGuiFonts["LXGWFasmartGothic"]);
        bool done = _uniformWindow->show(mgr, this);
        if (done)
        {
            _exposure->show(mgr, this); _intensity->show(mgr, this);
            _brightness->show(mgr, this); _saturation->show(mgr, this); _contrast->show(mgr, this);
            _cyanRed->show(mgr, this); _magentaGreen->show(mgr, this); _yellowBlue->show(mgr, this);
            _save->show(mgr, this); _uniformWindow->showEnd();
        }
        ImGui::PopFont();
    }

protected:
    void setupSlider(osgVerse::Slider& s, float v, float v0, float v1,
        osgVerse::Slider::ActionCallback cb)
    {
        s.type = osgVerse::Slider::FloatValue; s.value = v;
        s.minValue = v0; s.maxValue = v1; s.callback = cb;
    }

    osg::ref_ptr<osgVerse::Window> _uniformWindow;
    osg::ref_ptr<osgVerse::Slider> _exposure, _intensity;
    osg::ref_ptr<osgVerse::Slider> _brightness, _saturation, _contrast;
    osg::ref_ptr<osgVerse::Slider> _cyanRed, _magentaGreen, _yellowBlue;
    osg::ref_ptr<osgVerse::Button> _save;
    std::string _jsonFile;
};

static unsigned char* loadAllData(const std::string& file, unsigned int& size, unsigned int offset)
{
    std::ifstream ifs(file.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!ifs) return NULL;

    size = (int)ifs.tellg() - offset;
    ifs.seekg(offset, std::ios::beg); ifs.clear();

    unsigned char* imageData = new unsigned char[size];
    ifs.read((char*)imageData, size); ifs.close();
    return imageData;
}

static osg::Texture* createRawTexture2D(unsigned char* data, int w, int h, bool rgb)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    tex2D->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    tex2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex2D->setSourceType(GL_FLOAT);
    if (rgb) { tex2D->setInternalFormat(GL_RGB16F_ARB); tex2D->setSourceFormat(GL_RGB); }
    else { tex2D->setInternalFormat(GL_RGBA16F_ARB); tex2D->setSourceFormat(GL_RGBA); }

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage(w, h, 1, tex2D->getInternalFormat(), tex2D->getSourceFormat(),
                    tex2D->getSourceType(), data, osg::Image::USE_NEW_DELETE);
    tex2D->setImage(image.get());
    return tex2D.release();
}

static osg::Texture* createRawTexture3D(unsigned char* data, int w, int h, int d, bool rgb)
{
    osg::ref_ptr<osg::Texture3D> tex3D = new osg::Texture3D;
    tex3D->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex3D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex3D->setSourceType(GL_FLOAT);
    if (rgb) { tex3D->setInternalFormat(GL_RGB16F_ARB); tex3D->setSourceFormat(GL_RGB); }
    else { tex3D->setInternalFormat(GL_RGBA16F_ARB); tex3D->setSourceFormat(GL_RGBA); }

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage(w, h, d, tex3D->getInternalFormat(), tex3D->getSourceFormat(),
                    tex3D->getSourceType(), data, osg::Image::USE_NEW_DELETE);
    tex3D->setImage(image.get());
    return tex3D.release();
}

osg::Camera* configureEarthAndAtmosphere(osgViewer::View& viewer, osg::Group* root, osg::Node* earth,
                                         const std::string& mainFolder, int width, int height)
{
    // Create RTT camera to render the globe
    osg::Shader* vs1 = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_globe.vert.glsl");
    osg::Shader* fs1 = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_globe.frag.glsl");

    osg::ref_ptr<osg::Program> program1 = new osg::Program;
    vs1->setName("Scattering_Globe_VS"); program1->addShader(vs1);
    fs1->setName("Scattering_Globe_FS"); program1->addShader(fs1);
    osgVerse::Pipeline::createShaderDefinitions(vs1, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs1, 100, 130);  // FIXME

    osg::ref_ptr<osg::Texture> rttBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, width, height);
    rttBuffer->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    rttBuffer->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    rttBuffer->setWrap(osg::Texture2D::WRAP_S, osg::Texture::CLAMP);
    rttBuffer->setWrap(osg::Texture2D::WRAP_T, osg::Texture::CLAMP);

    osg::Camera* rttCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, NULL, NULL, false);
    rttCamera->setViewport(0, 0, rttBuffer->getTextureWidth(), rttBuffer->getTextureHeight());
    rttCamera->attach(osg::Camera::COLOR_BUFFER0, rttBuffer.get(), 0, 0, false, 16, 4);
    earth->getOrCreateStateSet()->setAttributeAndModes(program1.get());

    // Create the atmosphere HUD
    osg::Shader* vs2 = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_sky.vert.glsl");
    osg::Shader* fs2 = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_sky.frag.glsl");

    osg::ref_ptr<osg::Program> program2 = new osg::Program;
    vs2->setName("Scattering_Sky_VS"); program2->addShader(vs2);
    fs2->setName("Scattering_Sky_FS"); program2->addShader(fs2);
    osgVerse::Pipeline::createShaderDefinitions(vs2, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs2, 100, 130);  // FIXME

    osg::Camera* hudCamera = osgVerse::createHUDCamera(NULL, width, height, osg::Vec3(), 1.0f, 1.0f, true);
    hudCamera->getOrCreateStateSet()->setAttributeAndModes(program2.get());
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, rttBuffer.get());

    // Setup global stateset
    unsigned int size = 0;
    unsigned char* transmittance = loadAllData(BASE_DIR + "/textures/transmittance.raw", size, 0);
    unsigned char* irradiance = loadAllData(BASE_DIR + "/textures/irradiance.raw", size, 0);
    unsigned char* inscatter = loadAllData(BASE_DIR + "/textures/inscatter.raw", size, 0);

    osg::StateSet* ss = root->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0, osgVerse::createDefaultTexture());
    ss->setTextureAttributeAndModes(1, osgVerse::createTexture2D(
        osgDB::readImageFile(BASE_DIR + "/textures/sunglare.png"), osg::Texture::CLAMP));
    ss->setTextureAttributeAndModes(2, createRawTexture2D(transmittance, 256, 64, true));
    ss->setTextureAttributeAndModes(3, createRawTexture2D(irradiance, 64, 16, true));
    ss->setTextureAttributeAndModes(4, createRawTexture3D(inscatter, 256, 128, 32, false));
    ss->addUniform(new osg::Uniform("sceneSampler", (int)0));
    ss->addUniform(new osg::Uniform("glareSampler", (int)1));
    ss->addUniform(new osg::Uniform("transmittanceSampler", (int)2));
    ss->addUniform(new osg::Uniform("skyIrradianceSampler", (int)3));
    ss->addUniform(new osg::Uniform("inscatterSampler", (int)4));
    ss->addUniform(new osg::Uniform("origin", osg::Vec3(0.0f, 0.0f, 0.0f)));
    ss->addUniform(new osg::Uniform("opaque", 1.0f));
    ss->addUniform(new osg::Uniform("ColorBalanceMode", (int)0));

    std::map<std::string, osg::Uniform*> uniforms;
    uniforms["exposure"] = new osg::Uniform("hdrExposure", 0.25f);
    uniforms["sun_intensity"] = new osg::Uniform("sunIntensity", 100.0f);
    uniforms["color_attributes"] = new osg::Uniform("ColorAttribute", osg::Vec3(1.0f, 1.0f, 1.0f));
    uniforms["color_balance"] = new osg::Uniform("ColorBalance", osg::Vec3(0.0f, 0.0f, 0.0f));  // [-1, 1]
    for (std::map<std::string, osg::Uniform*>::iterator i = uniforms.begin();
         i != uniforms.end(); ++i) ss->addUniform(i->second);

    // Finish configuration
    osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
    postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    postCamera->setRenderOrder(osg::Camera::POST_RENDER, 20001);
    postCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    root->addChild(postCamera.get());

    imgui->setChineseSimplifiedFont(MISC_DIR + "LXGWFasmartGothic.otf");
    imgui->initialize(new AdjusterHandler(uniforms, mainFolder + "/uniforms.json"));
    imgui->addToView(&viewer, postCamera.get());

    rttCamera->addChild(earth);
    root->addChild(rttCamera);
    root->addChild(hudCamera);
    return rttCamera;
}
