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
        float exp = 0.25f, scale0[3] = { 1.0f, 1.0f, 1.0f }, scale1[3] = { 1.0f, 1.0f, 1.0f };
        float bri = 1.0f, sat = 1.0f, con = 1.0f, cr = 0.0f, mg = 0.0f, yb = 0.0f;
        if (err.empty())
        {
            exp = (float)root.get("exposure").get<double>();
            scale0[0] = (float)root.get("sun_scale_r").get<double>();
            scale0[1] = (float)root.get("sun_scale_g").get<double>();
            scale0[2] = (float)root.get("sun_scale_b").get<double>();
            scale1[0] = (float)root.get("sky_scale_r").get<double>();
            scale1[1] = (float)root.get("sky_scale_g").get<double>();
            scale1[2] = (float)root.get("sky_scale_b").get<double>();

            bri = (float)root.get("brightness").get<double>();
            sat = (float)root.get("saturation").get<double>();
            con = (float)root.get("contrast").get<double>();
            cr = (float)root.get("cyan_red").get<double>();
            mg = (float)root.get("magenta_green").get<double>();
            yb = (float)root.get("yellow_blue").get<double>();
        }

        _exposure = new osgVerse::Slider("Exposure");
        _brightness = new osgVerse::Slider("Brightness");
        _saturation = new osgVerse::Slider("Saturation");
        _contrast = new osgVerse::Slider("Contrast");
        _cyanRed = new osgVerse::Slider("Cyan/Red");
        _magentaGreen = new osgVerse::Slider("Magenta/Green");
        _yellowBlue = new osgVerse::Slider("Yellow/Blue");
        _sunColor = new osgVerse::InputVectorField("Light Scale");
        _skyColor = new osgVerse::InputVectorField("Shadow Scale");

        osg::Uniform* exposure = uniforms["exposure"]; exposure->set(exp);
        osg::Uniform* colorAttr = uniforms["color_attributes"]; colorAttr->set(osg::Vec3(bri, sat, con));
        osg::Uniform* colorBal = uniforms["color_balance"]; colorBal->set(osg::Vec3(cr, mg, yb));
        osg::Uniform* sunScale = uniforms["sun_color_scale"]; sunScale->set(osg::Vec3(scale0[0], scale0[1], scale0[2]));
        osg::Uniform* skyScale = uniforms["sky_color_scale"]; skyScale->set(osg::Vec3(scale1[0], scale1[1], scale1[2]));

        setupSlider(*_exposure, exp, 0.0f, 1.0f, [exposure](UI_ARGS) { SETUP_SLIDER_V1(exposure) });
        setupSlider(*_brightness, bri, 0.0f, 3.0f, [colorAttr](UI_ARGS) { SETUP_SLIDER_V3(colorAttr, 0) });
        setupSlider(*_saturation, sat, 0.0f, 3.0f, [colorAttr](UI_ARGS) { SETUP_SLIDER_V3(colorAttr, 1) });
        setupSlider(*_contrast, con, 0.0f, 3.0f, [colorAttr](UI_ARGS) { SETUP_SLIDER_V3(colorAttr, 2) });
        setupSlider(*_cyanRed, cr, -1.0f, 1.0f, [colorBal](UI_ARGS) { SETUP_SLIDER_V3(colorBal, 0) });
        setupSlider(*_magentaGreen, mg, -1.0f, 1.0f, [colorBal](UI_ARGS) { SETUP_SLIDER_V3(colorBal, 1) });
        setupSlider(*_yellowBlue, yb, -1.0f, 1.0f, [colorBal](UI_ARGS) { SETUP_SLIDER_V3(colorBal, 2) });

        setupColorEditor(*_sunColor, osg::Vec3(scale0[0], scale0[1], scale0[2]), [sunScale](UI_ARGS)
        {
            osg::Vec3 ori; osgVerse::InputVectorField* v = static_cast<osgVerse::InputVectorField*>(ctrl);
            sunScale->get(ori); ori.set(v->vecValue[0], v->vecValue[1], v->vecValue[2]); sunScale->set(ori);
        });
        setupColorEditor(*_skyColor, osg::Vec3(scale1[0], scale1[1], scale1[2]), [skyScale](UI_ARGS)
        {
            osg::Vec3 ori; osgVerse::InputVectorField* v = static_cast<osgVerse::InputVectorField*>(ctrl);
            skyScale->get(ori); ori.set(v->vecValue[0], v->vecValue[1], v->vecValue[2]); skyScale->set(ori);
        });

        _save = new osgVerse::Button("Save Config");
        _save->callback = [jsonFile, uniforms](UI_ARGS)
        {
            std::map<std::string, osg::Uniform*> un = uniforms;
            float exp = 0.0f; un["exposure"]->get(exp);
            osg::Vec3 colorAttr; un["color_attributes"]->get(colorAttr);
            osg::Vec3 colorBal; un["color_balance"]->get(colorBal);
            osg::Vec3 scale0; un["sun_color_scale"]->get(scale0);
            osg::Vec3 scale1; un["sky_color_scale"]->get(scale1);

            picojson::object root;
            root["exposure"] = picojson::value((double)exp);
            root["sun_scale_r"] = picojson::value((double)scale0[0]);
            root["sun_scale_g"] = picojson::value((double)scale0[1]);
            root["sun_scale_b"] = picojson::value((double)scale0[2]);
            root["sky_scale_r"] = picojson::value((double)scale1[0]);
            root["sky_scale_g"] = picojson::value((double)scale1[1]);
            root["sky_scale_b"] = picojson::value((double)scale1[2]);

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
            _exposure->show(mgr, this);
            _brightness->show(mgr, this); _saturation->show(mgr, this); _contrast->show(mgr, this);
            _cyanRed->show(mgr, this); _magentaGreen->show(mgr, this); _yellowBlue->show(mgr, this);
            _sunColor->show(mgr, this); _skyColor->show(mgr, this);
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

    void setupColorEditor(osgVerse::InputVectorField& f, const osg::Vec3& v,
                          osgVerse::Slider::ActionCallback cb)
    {
        f.callback = cb; f.asColor = true; f.vecNumber = 3;
        f.vecValue = osg::Vec4d(v[0], v[1], v[2], 1.0);
    }

    osg::ref_ptr<osgVerse::Window> _uniformWindow;
    osg::ref_ptr<osgVerse::Slider> _exposure;
    osg::ref_ptr<osgVerse::Slider> _brightness, _saturation, _contrast;
    osg::ref_ptr<osgVerse::Slider> _cyanRed, _magentaGreen, _yellowBlue;
    osg::ref_ptr<osgVerse::InputVectorField> _sunColor, _skyColor;
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

typedef std::pair<osg::Camera*, osg::Texture*> CameraTexturePair;
CameraTexturePair configureEarthAndAtmosphere(osgViewer::View& viewer, osg::Group* root, osg::Node* earth,
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

    osg::ref_ptr<osg::Texture> rttBuffer2 =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, width, height);
    rttBuffer2->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    rttBuffer2->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    rttBuffer2->setWrap(osg::Texture2D::WRAP_S, osg::Texture::CLAMP);
    rttBuffer2->setWrap(osg::Texture2D::WRAP_T, osg::Texture::CLAMP);

    osg::Camera* rttCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, NULL, NULL, false);
    rttCamera->setViewport(0, 0, rttBuffer->getTextureWidth(), rttBuffer->getTextureHeight());
    rttCamera->attach(osg::Camera::COLOR_BUFFER0, rttBuffer.get(), 0, 0, false, 16, 4);
    rttCamera->attach(osg::Camera::COLOR_BUFFER1, rttBuffer2.get());
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
    ss->setTextureAttributeAndModes(1, osgVerse::createDefaultTexture());
    ss->setTextureAttributeAndModes(2, osgVerse::createTexture2D(
        osgDB::readImageFile(BASE_DIR + "/textures/sunglare.png"), osg::Texture::CLAMP));
    ss->setTextureAttributeAndModes(3, createRawTexture2D(transmittance, 256, 64, true));
    ss->setTextureAttributeAndModes(4, createRawTexture2D(irradiance, 64, 16, true));
    ss->setTextureAttributeAndModes(5, createRawTexture3D(inscatter, 256, 128, 32, false));
    ss->addUniform(new osg::Uniform("sceneSampler", (int)0));
    ss->addUniform(new osg::Uniform("maskSampler", (int)1));
    ss->addUniform(new osg::Uniform("glareSampler", (int)2));
    ss->addUniform(new osg::Uniform("transmittanceSampler", (int)3));
    ss->addUniform(new osg::Uniform("skyIrradianceSampler", (int)4));
    ss->addUniform(new osg::Uniform("inscatterSampler", (int)5));
    ss->addUniform(new osg::Uniform("origin", osg::Vec3(0.0f, 0.0f, 0.0f)));
    ss->addUniform(new osg::Uniform("globalOpaque", 1.0f));
    ss->addUniform(new osg::Uniform("ColorBalanceMode", (int)0));

    std::map<std::string, osg::Uniform*> uniforms;
    uniforms["exposure"] = new osg::Uniform("hdrExposure", 0.25f);
    uniforms["sun_intensity"] = new osg::Uniform("sunIntensity", 100.0f);
    uniforms["sun_color_scale"] = new osg::Uniform("sunColorScale", osg::Vec3(1.0f, 1.0f, 1.0f));
    uniforms["sky_color_scale"] = new osg::Uniform("skyColorScale", osg::Vec3(1.0f, 1.0f, 1.0f));
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
    root->addChild(rttCamera); root->addChild(hudCamera);
    return CameraTexturePair(rttCamera, rttBuffer2.get());
}
