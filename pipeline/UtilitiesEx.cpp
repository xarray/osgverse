#include <osg/io_utils>
#include <osg/Version>
#include <osg/ComputeBoundsVisitor>
#include <osg/FrameBufferObject>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/Geode>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ConvertUTF>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <osgGA/CameraManipulator>
#include <osgGA/EventQueue>
#include <chrono>
#include <locale>
#include <codecvt>
#include <iostream>
#include <iomanip>
#include <array>
#include <random>

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "3rdparty/xxhash.h"
#include "modeling/Math.h"
#include "ShaderLibrary.h"
#include "Pipeline.h"
#include "ResourceManager.h"
#include "Utilities.h"

#define srnd() (2 * frandom(&seed) - 1)
using namespace osgVerse;

/************** ScreenSnapshotCallback **************/
ScreenSnapshotCallback::ScreenSnapshotCallback(bool c, int fps)
: _lastTime(0), _count(0), _capturing(c) { _image = new osg::Image; setCaptureFrequency(fps); }

void ScreenSnapshotCallback::flush(const std::string& prefix)
{
    if (_outputList.empty())
    {
        OSG_WARN << "[ScreenSnapshotCallback] Nothing to write: no frame has been captured yet "
                 << "(is the callback really attached to the drawn camera, and is capturing on?)"
                 << std::endl;
        return;
    }

    for (std::map<std::string, osg::ref_ptr<osg::Image>>::iterator it = _outputList.begin();
         it != _outputList.end(); ++it)
    {
        std::string file = prefix + it->first;
        if (osgDB::writeImageFile(*(it->second), file))
            OSG_NOTICE << "[ScreenSnapshotCallback] Wrote " << file << std::endl;
        else
            OSG_WARN << "[ScreenSnapshotCallback] Failed to write " << file
                     << " (is a plugin for this image format installed?)" << std::endl;
    }
    _outputList.clear();
}

void ScreenSnapshotCallback::operator()(const osg::Camera & camera) const
{
    int width = 800, height = 600; if (!_capturing) return;
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
    glReadBuffer(GL_BACK);  // read from back buffer (gc must be double-buffered)
#endif
    const osg::GraphicsContext* gc = camera.getGraphicsContext();
    if (gc && gc->getTraits())
    {
        width = gc->getTraits()->width;
        height = gc->getTraits()->height;
    }

    osg::Timer_t t0 = osg::Timer::instance()->tick();
    if (_interval <= osg::Timer::instance()->delta_m(_lastTime, t0))
    {
        _image->readPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
        if (!_filePrefix.empty())
        {
            ScreenSnapshotCallback* nonconst = const_cast<ScreenSnapshotCallback*>(this);
            std::stringstream oss; oss << "_" << std::setw(4) << std::setfill('0') << _count;
            nonconst->_outputList[_filePrefix + oss.str() + ".png"] =
                static_cast<osg::Image*>(_image->clone(osg::CopyOp::DEEP_COPY_ALL));  // FIXME: write out in thread
        }
        _lastTime = t0; _count++;
    }
}

/************** Agent (AI) view & capture helper **************/
namespace
{
    /** The per-frame driver of the agent automation: it waits for the scene to settle, optionally
        sends a few key strokes, then enables the snapshot callback for one or more frames and
        finally quits the viewer. It lives in an event handler (instead of a plain draw callback)
        so that it is able to post synthetic key events to the same event queue the user would use,
        which is what makes it possible to toggle features (clustering on/off, shadows...) and see
        the difference in the captured images without editing the test case again */
    class AgentFrameHandler : public osgGA::GUIEventHandler
    {
    public:
        AgentFrameHandler(osgViewer::View* viewer, ScreenSnapshotCallback* cb,
                          int captureFrame, const std::vector<int>& keys, bool toQuit)
        :   _viewer(viewer), _callback(cb), _captureFrame(captureFrame), _keys(keys),
            _toQuit(toQuit), _frame(0), _keysSent(false), _capturing(false), _finished(false) {}

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
        {
            if (ea.getEventType() != osgGA::GUIEventAdapter::FRAME || _finished) return false;
            if (!_callback.valid()) return false; else _frame++;

            // Keys are sent a couple of frames before the capture, so their effects (which may
            // need a frame each: state switching, buffer resizing, TAA history dropping...) are
            // fully applied to the image we are about to read
            if (_frame == _captureFrame && !_keysSent)
            {
                osgGA::EventQueue* eq = _viewer.valid() ? _viewer->getEventQueue() : NULL;
                if (eq)
                {
                    for (size_t i = 0; i < _keys.size(); ++i)
                    { eq->keyPress(_keys[i]); eq->keyRelease(_keys[i]); }
                }
                _keysSent = true;
            }

            if (_frame == _captureFrame + 2 && !_capturing)
            { _callback->setCapturing(true); _capturing = true; }
            else if (_capturing && _frame >= _captureFrame + 4)
            {
                _callback->setCapturing(false); _callback->flush("");
                _finished = true;
                OSG_NOTICE << "[Agent] Snapshot saved as '" << _callback->getFilePrefix()
                           << "_<index>.png'" << std::endl;
                if (_toQuit)
                {
                    osgViewer::ViewerBase* vb =
                        dynamic_cast<osgViewer::ViewerBase*>(&aa);
                    if (vb) vb->setDone(true);
                }
            }
            return false;
        }

    protected:
        osg::observer_ptr<osgViewer::View> _viewer;
        osg::observer_ptr<ScreenSnapshotCallback> _callback;
        std::vector<int> _keys;
        int _captureFrame, _frame;
        bool _toQuit, _keysSent, _capturing, _finished;
    };
}

bool osgVerse::configureViewerForAgent(osg::ArgumentParser& arguments, osgViewer::View* viewer)
{
    // Parse (and always consume) all agent options, even when the mode is not enabled, so that
    // they never leak into osgDB::readNodeFiles() and get mistaken for file names
    bool enabled = arguments.read("--agent") || arguments.read("--ai");
    int frames = 60, fps = 0, winW = 0, winH = 0;
    std::string output = "agent_snapshot", keyString;
    osg::Vec3 eye(0.0f, 0.0f, 0.0f), center(0.0f, 0.0f, 0.0f), up(0.0f, 0.0f, 1.0f);
    bool hasEye = arguments.read("--agent-eye", eye.x(), eye.y(), eye.z());
    if (arguments.read("--agent-center", center.x(), center.y(), center.z())) hasEye = true;
    arguments.read("--agent-up", up.x(), up.y(), up.z());
    arguments.read("--agent-frames", frames);
    arguments.read("--agent-output", output);
    arguments.read("--agent-fps", fps);
    arguments.read("--agent-size", winW, winH);
    arguments.read("--agent-keys", keyString);
    bool keep = arguments.read("--agent-keep");
    if (!enabled || viewer == NULL) return false;
    if (frames < 1) frames = 1;

    // 1) Window size. It is best applied before realize(); after that the resize is forwarded to
    // the graphics window (when there is one), but a not-yet-created context simply keeps it
    if (winW > 0 && winH > 0)
    {
        osg::GraphicsContext* gc = viewer->getCamera() ? viewer->getCamera()->getGraphicsContext() : NULL;
        osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc);
        if (gw) { gw->setWindowRectangle(0, 0, winW, winH); gw->resized(0, 0, winW, winH); }
        if (viewer->getCamera()) viewer->getCamera()->setViewport(new osg::Viewport(0, 0, winW, winH));
    }

    // 2) Initial view. The manipulator is preferred so that a later user interaction starts from
    // the same place; without one the camera view matrix is set directly
    if (hasEye && viewer->getCamera())
    {
        osg::Matrixd lookAt = osg::Matrixd::lookAt(eye, center, up);
        osgGA::CameraManipulator* manip = viewer->getCameraManipulator();
        if (manip) manip->setByInverseMatrix(lookAt);
        else viewer->getCamera()->setViewMatrix(lookAt);
    }

    // 3) Snapshot callback. It has to be attached to the camera which really draws the image on
    // screen: when a pipeline is in use the main camera is detached (setGraphicsContext(NULL))
    // and one of its slaves (the forward pass, or the last stage) owns the window. The last
    // slave with a graphics context is therefore preferred, and the main camera is the fallback
    // for a plain viewer
    osg::ref_ptr<ScreenSnapshotCallback> callback = new ScreenSnapshotCallback(false, fps);
    callback->setFilePrefix(output);
    osg::Camera* shotCamera = viewer->getCamera();
    for (unsigned int i = viewer->getNumSlaves(); i > 0; --i)
    {
        osg::View::Slave& slave = viewer->getSlave(i - 1);
        if (slave._camera.valid() && slave._camera->getGraphicsContext())
        { shotCamera = slave._camera.get(); break; }
    }

    if (shotCamera)
    {
        osg::Camera::DrawCallback* existing = shotCamera->getFinalDrawCallback();
        callback->setup(shotCamera, FINAL_DRAW);
        if (existing != NULL && existing != callback.get() &&
            dynamic_cast<CameraDrawCallback*>(existing) == NULL)
        {
            OSG_WARN << "[Agent] The camera <" << shotCamera->getName()
                     << "> already has a final draw callback (" << existing->className()
                     << "), which is not an osgVerse CameraDrawCallback: the screenshot callback "
                     << "can not be chained and screenshots may fail." << std::endl;
        }
        OSG_NOTICE << "[Agent] Screenshot attached to camera <" << shotCamera->getName()
                   << ">" << std::endl;
    }

    // 4) Keys to press before the capture: single characters ("c") or numeric key codes ("99")
    std::vector<int> keys;
    if (!keyString.empty())
    {
        std::vector<std::string> tokens; StringAuxiliary::split(keyString, tokens, ',');
        for (size_t i = 0; i < tokens.size(); ++i)
        {
            if (tokens[i].empty()) continue;
            if (tokens[i].size() == 1) keys.push_back((int)tokens[i][0]);
            else keys.push_back((int)atoi(tokens[i].c_str()));
        }
    }

    // 5) Automation driver
    viewer->addEventHandler(new AgentFrameHandler(viewer, callback.get(), frames, keys, !keep));
    OSG_NOTICE << "[Agent] Automation enabled: capture after " << frames << " frame(s) as '"
               << output << "_<index>.png'" << (keep ? " (window kept open)" : ", then quit")
               << std::endl;
    return true;
}

/************** Hash **************/
static XXH64_hash_t g_hashSeed = XXH64("osgVerse", 8, 0);

void* Hash::startStream()
{
    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, g_hashSeed); return state;
}

bool Hash::addToStream(void* s, const char* data, unsigned int size)
{
    XXH64_state_t* state = (XXH64_state_t*)s; if (!s) return false;
    return XXH64_update(state, data, size) != XXH_ERROR;
}

unsigned long long Hash::getStream(void* s, bool toFinish)
{
    XXH64_state_t* state = (XXH64_state_t*)s; if (!s) return 0;
    XXH64_hash_t v = XXH64_digest(state);
    if (toFinish) XXH64_freeState(state); return v;
}

unsigned long long Hash::get(const char* data, unsigned int size)
{ return XXH64(data, size, g_hashSeed); }

unsigned long long Hash::getImage(osg::Image& image)
{
    unsigned int allSize = image.getTotalDataSize(), numMipmaps = image.getNumMipmapLevels();
    XXH64_state_t* state = (XXH64_state_t*)startStream();
    XXH64_update(state, image.data(), image.getTotalSizeInBytes());
    for (unsigned int j = 1; j < numMipmaps; ++j)
    {
        unsigned int size = (j < numMipmaps - 1 ? image.getMipmapOffset(j + 1) : allSize)
                          - image.getMipmapOffset(j);
        XXH64_update(state, image.getMipmapData(j), size);
    }
    XXH64_hash_t v = XXH64_digest(state);
    XXH64_freeState(state); return v;
}

unsigned long long Hash::getBuffer(osg::BufferData& buffer)
{
    XXH64_state_t* state = (XXH64_state_t*)startStream();
    XXH64_update(state, buffer.getDataPointer(), buffer.getTotalDataSize());
    XXH64_hash_t v = XXH64_digest(state);
    XXH64_freeState(state); return v;
}

unsigned long long Hash::getShader(osg::Shader& shader)
{
    XXH64_state_t* state = (XXH64_state_t*)startStream();
    if (shader.getShaderBinary())
    {
        osg::ShaderBinary* bin = shader.getShaderBinary();
        XXH64_update(state, bin->getData(), bin->getSize());
    }
    else
    {
        const std::string& src = shader.getShaderSource();
        XXH64_update(state, src.data(), src.size());
    }
    XXH64_hash_t v = XXH64_digest(state);
    XXH64_freeState(state); return v;
}

/************** StringAuxiliary **************/

osg::Vec4 StringAuxiliary::hexColorToRGB(const std::string& hexColor, bool withAlpha)
{
    if (hexColor.empty() || hexColor[0] != '#') return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    std::string colorPart = hexColor.substr(1), ex; unsigned long hexValue = 0;
    if (!withAlpha)
    {   // #abc -> #aabbcc
        if (colorPart.length() == 3) { for (char c : colorPart) ex += std::string(2, c); colorPart = ex; }
        else if (colorPart.length() != 6) return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else if (colorPart.length() != 8)
        return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);

    std::istringstream iss(colorPart);
    if (!(iss >> std::hex >> hexValue)) return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (withAlpha) return osg::Vec4(((hexValue >> 24) & 0xFF) / 255.0f, ((hexValue >> 16) & 0xFF) / 255.0f,
                                    ((hexValue >> 8) & 0xFF) / 255.0f, (hexValue & 0xFF) / 255.0f);
    else return osg::Vec4(((hexValue >> 16) & 0xFF) / 255.0f, ((hexValue >> 8) & 0xFF) / 255.0f,
                          (hexValue & 0xFF) / 255.0f, 1.0f);
}

void StringAuxiliary::split(const std::string& src, std::vector<std::string>& slist,
                            char sep, bool ignoreEmpty)
{
    if (src.empty()) return; bool inQuotes = false;
    std::string::size_type start = 0;
    
    for (std::string::size_type i = 0; i < src.size(); ++i)
    {
        if (src[i] == '"') inQuotes = !inQuotes;
        else if (src[i] == sep && !inQuotes)
        {
            if (!ignoreEmpty || (i - start) > 0)
                slist.push_back(src.substr(start, i - start));
            start = i + 1;
        }
    }
    if (!ignoreEmpty || (src.size() - start) > 0)
        slist.push_back(src.substr(start, src.size() - start));
}

std::string StringAuxiliary::trim(const std::string& str)
{
    if (!str.size()) return str;
    std::string::size_type first = str.find_first_not_of(" \t");
    std::string::size_type last = str.find_last_not_of("  \t\r\n");
    if ((first == str.npos) || (last == str.npos)) return std::string("");
    return str.substr(first, last - first + 1);
}

std::string StringAuxiliary::convertNativePath(const std::string& inputPath)
{
    std::string result = inputPath; if (inputPath.empty()) return inputPath;
    const char nativeSep = osgDB::getNativePathSeparator();
    for (size_t i = 0; i < result.length(); ++i)
    { if (result[i] == '/' || result[i] == '\\') {result[i] = nativeSep;} }
    return result;
}

std::string StringAuxiliary::convertUTF16toUTF8(const wchar_t* source, unsigned sourceLength)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    return osgDB::convertUTF16toUTF8(source, sourceLength);
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(std::wstring(source, sourceLength));
#endif
}

std::wstring StringAuxiliary::convertUTF8toUTF16(const char* source, unsigned sourceLength)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    return osgDB::convertUTF8toUTF16(source, sourceLength);
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(std::string(source, sourceLength));
#endif
}

/************** EarthAtmosphereOcean **************/

static long lrandom(long* seed)
{
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return *seed;
}

static float frandom(long* seed)
{ long r = lrandom(seed) >> (31 - 24); return r / (float)(1 << 24); }

static float grandom(float mean, float stdDeviation, long* seed)
{
    float x1, x2, w, y1;
    static float y2;
    static int use_last = 0;
    if (use_last) { y1 = y2; use_last = 0; }
    else
    {
        do {
            x1 = 2.0f * frandom(seed) - 1.0f;
            x2 = 2.0f * frandom(seed) - 1.0f;
            w = x1 * x1 + x2 * x2;
        } while (w >= 1.0f);
        w = sqrt((-2.0f * log(w)) / w);
        y1 = x1 * w; y2 = x2 * w; use_last = 1;
    }
    return mean + y1 * stdDeviation;
}

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

void EarthAtmosphereOcean::applyToGlobe(osg::StateSet* ss, osg::Texture* baseTex, osg::Texture* maskTex,
                                        osg::Texture* extraTex, osg::Shader* vs, osg::Shader* fs, Pipeline* ref)
{
    if (!ss || !vs || !fs) return; vs->setName("Scattering_Globe_VS"); fs->setName("Scattering_Globe_FS");
    ResourceManager* res = ResourceManager::instance(); res->shareShader(vs, true); res->shareShader(fs, true);
    if (!ref)
    {
        int cxtVer = 0, glslVer = 0; guessOpenGLVersions(cxtVer, glslVer);
        Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
        Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);
    }

    osgVerse::applyTextureAttributeAndModes(ss, 0, baseTex);
    osgVerse::applyTextureAttributeAndModes(ss, 1, maskTex);
    osgVerse::applyTextureAttributeAndModes(ss, 2, extraTex);
    ss->getOrCreateUniform("SceneSampler", osg::Uniform::INT)->set((int)0);
    ss->getOrCreateUniform("MaskSampler", osg::Uniform::INT)->set((int)1);
    ss->getOrCreateUniform("ExtraLayerSampler", osg::Uniform::INT)->set((int)2);
    ss->getOrCreateUniform("UvOffset1", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    ss->getOrCreateUniform("UvOffset2", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    ss->getOrCreateUniform("UvOffset3", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));

    osg::Program* program = apply(ss, vs, fs, 3, ref);
    program->addBindAttribLocation("osg_GlobeData", GLOBE_ATTRIBUTE_INDEX);  // for computing ocean plane
}

void EarthAtmosphereOcean::applyToOcean(osg::StateSet* ss, osg::Texture* postMaskTex, osg::Texture* waveTex,
                                        osg::Shader* vs, osg::Shader* fs, Pipeline* ref)
{
    if (!ss || !vs || !fs) return; vs->setName("Global_Ocean_VS"); fs->setName("Global_Ocean_FS");
    ResourceManager* res = ResourceManager::instance(); res->shareShader(vs, true); res->shareShader(fs, true);
    if (!ref)
    {
        int cxtVer = 0, glslVer = 0; guessOpenGLVersions(cxtVer, glslVer);
        Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
        Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);
    }

    osgVerse::applyTextureAttributeAndModes(ss, 0, postMaskTex);
    osgVerse::applyTextureAttributeAndModes(ss, 1, waveTex);
    ss->getOrCreateUniform("EarthMaskSampler", osg::Uniform::INT)->set((int)0);
    ss->getOrCreateUniform("WavesSampler", osg::Uniform::INT)->set((int)1);
    apply(ss, vs, fs, 2, ref);
    for (std::map<std::string, osg::ref_ptr<osg::Uniform>>::iterator it = oceanUniforms.begin();
         it != oceanUniforms.end(); ++it) ss->addUniform(it->second.get());
}

void EarthAtmosphereOcean::applyToAtmosphere(osg::StateSet* ss, osg::Texture* earthSceneTex,
                                             osg::Shader* vs, osg::Shader* fs, Pipeline* ref)
{
    if (!ss || !vs || !fs) return; vs->setName("Scattering_Sky_VS"); fs->setName("Scattering_Sky_FS");
    ResourceManager* res = ResourceManager::instance(); res->shareShader(vs, true); res->shareShader(fs, true);
    if (!ref)
    {
        int cxtVer = 0, glslVer = 0; guessOpenGLVersions(cxtVer, glslVer);
        Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
        Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);
    }

    osgVerse::applyTextureAttributeAndModes(ss, 0, earthSceneTex);
    ss->getOrCreateUniform("SceneSampler", osg::Uniform::INT)->set((int)0);
    apply(ss, vs, fs, 1, ref);
}

osg::Program* EarthAtmosphereOcean::apply(osg::StateSet* ss, osg::Shader* vs, osg::Shader* fs,
                                          unsigned int startU, Pipeline* refPipeline)
{
    if (!ss || !vs || !fs) return NULL;
    if (refPipeline)
    {
        refPipeline->createShaderDefinitionsFromPipeline(vs);
        refPipeline->createShaderDefinitionsFromPipeline(fs);
    }

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(vs); program->addShader(fs);
    ss->setAttributeAndModes(program.get());
    osgVerse::applyTextureAttributeAndModes(ss, startU + 0, transmittance.get());
    osgVerse::applyTextureAttributeAndModes(ss, startU + 1, irradiance.get());
    osgVerse::applyTextureAttributeAndModes(ss, startU + 2, inscatter.get());
    osgVerse::applyTextureAttributeAndModes(ss, startU + 3, glare.get());
    ss->getOrCreateUniform("TransmittanceSampler", osg::Uniform::INT)->set((int)startU + 0);
    ss->getOrCreateUniform("SkyIrradianceSampler", osg::Uniform::INT)->set((int)startU + 1);
    ss->getOrCreateUniform("InscatterSampler", osg::Uniform::INT)->set((int)startU + 2);
    ss->getOrCreateUniform("GlareSampler", osg::Uniform::INT)->set((int)startU + 3);
    for (std::map<std::string, osg::ref_ptr<osg::Uniform>>::iterator it = commonUniforms.begin();
         it != commonUniforms.end(); ++it) ss->addUniform(it->second.get());
    return program.get();
}

void EarthAtmosphereOcean::update(osg::Camera* camera)
{
    osg::Matrix invViewMatrix = camera->getInverseViewMatrix();
    osg::Matrix projMatrix = camera->getProjectionMatrix();
    osg::Vec3d worldCam = invViewMatrix.getTrans();
    osg::Vec3d worldLLA = Coordinate::convertECEFtoLLA(worldCam);
    commonUniforms["CameraToWorld"]->set(osg::Matrixf(invViewMatrix));
    commonUniforms["CameraToScreen"]->set(osg::Matrixf(projMatrix));
    commonUniforms["ScreenToCamera"]->set(osg::Matrixf::inverse(projMatrix));
    commonUniforms["WorldCameraPos"]->set(osg::Vec3(worldCam));
    commonUniforms["WorldCameraLLA"]->set(osg::Vec3(worldLLA));
}

void EarthAtmosphereOcean::updateOcean(osg::Camera* camera)
{
    osg::Matrix ctol = camera->getInverseViewMatrix();
    osg::Vec3d cl = osg::Vec3() * ctol;
    double radius = osg::WGS_84_RADIUS_EQUATOR;
    if ((radius == 0.0 && cl.z() > oceanMinZ) || (radius > 0.0 && cl.length() > radius + oceanMinZ))
    { oceanFromLocal = osg::Matrix(); oceanOffset = osg::Vec3(); }

    osg::Vec3d ux, uy, uz, oo;
    if (radius == 0.0)
    {
        ux = osg::X_AXIS; uy = osg::Y_AXIS; uz = osg::Z_AXIS;
        oo = osg::Vec3d(cl.x(), cl.y(), 0.0);
    }
    else
    {   // spherical ocean
        uz = cl; uz.normalize(); // unit z vector of ocean frame, in local space
        if (!oceanFromLocal.isIdentity())
        {
            ux = osg::Vec3d(oceanFromLocal(1, 0), oceanFromLocal(1, 1), oceanFromLocal(1, 2));
            ux = (ux ^ uz); ux.normalize();
        }
        else
            { ux = osg::Z_AXIS; ux = (ux ^ uz); ux.normalize(); }
        uy = (uz ^ ux); // unit y vector
        oo = uz * radius; // origin of ocean frame, in local space
    }

    osg::Matrix ltoo(ux.x(), uy.x(), uz.x(), 0.0,
                     ux.y(), uy.y(), uz.y(), 0.0,
                     ux.z(), uy.z(), uz.z(), 0.0,
                     -(ux * oo), -(uy * oo), -(uz * oo), 1.0);
    osg::Matrix ctoo = ctol * ltoo;  // compute ctoo = CameraToOcean transform
    if (!oceanFromLocal.isIdentity())
    {
        osg::Vec3d delta = osg::Vec3d() * osg::Matrix::inverse(oceanFromLocal) * ltoo;
        oceanOffset += delta;
    }
    oceanFromLocal = ltoo;

    // Compute ocean-world-camera matrices
    osg::Matrix ctos = camera->getProjectionMatrix();
    osg::Matrix stoc = osg::Matrix::inverse(ctos);
    osg::Matrix otoc = osg::Matrix::inverse(ctoo);
    otoc.setTrans(osg::Vec3());

    osg::Vec3 wDir, oDir;
    osg::Uniform* worldSunDir = commonUniforms["WorldSunDir"].get();
    osg::Uniform* oceanSunDir = oceanUniforms["OceanSunDir"].get();
    worldSunDir->get(wDir); oceanSunDir->set(osg::Matrixf::transform3x3(wDir, ltoo));

    osg::Vec3d oc = osg::Vec3d() * ctoo; float h = oc.z();  // if h < 0, we are under the ocean...
    commonUniforms["UnderOcean"]->set(h);
    oceanUniforms["CameraToOcean"]->set(osg::Matrixf(ctoo));
    oceanUniforms["OceanToCamera"]->set(osg::Matrixf(otoc));
    oceanUniforms["OceanToWorld"]->set(osg::Matrixf::inverse(ltoo));
    oceanUniforms["OceanCameraPos"]->set(osg::Vec3(-oceanOffset.x(), -oceanOffset.y(), h));

    // Compute horizon1 & horizon2
    osg::Vec4d temp;
    temp = osg::Vec4d(0.0, 0.0, 0.0, 1.0) * stoc;
    temp = osg::Vec4d(temp[0], temp[1], temp[2], 0.0) * ctoo;
    osg::Vec3d A0(temp[0], temp[1], temp[2]);
    temp = osg::Vec4d(1.0, 0.0, 0.0, 0.0) * stoc;
    temp = osg::Vec4d(temp[0], temp[1], temp[2], 0.0) * ctoo;
    osg::Vec3d dA(temp[0], temp[1], temp[2]);
    temp = osg::Vec4d(0.0, 1.0, 0.0, 0.0) * stoc;
    temp = osg::Vec4d(temp[0], temp[1], temp[2], 0.0) * ctoo;
    osg::Vec3d B(temp[0], temp[1], temp[2]);

    if (radius == 0.0)
    {
        oceanUniforms["Horizon1"]->set(osg::Vec3(-(h * 1e-6 + A0.z()) / B.z(), -dA.z() / B.z(), 0.0));
        oceanUniforms["Horizon2"]->set(osg::Vec3());
    }
    else
    {
        double h1 = h * (h + 2.0 * radius), h2 = (h + radius) * (h + radius);
        double alpha = (B * B) * h1 - B.z() * B.z() * h2;
        alpha = -fabs(alpha);  // FIXME: modified from proland to keep ocean always seen, can it work?

        double beta0 = ((A0 * B) * h1 - B.z() * A0.z() * h2) / alpha;
        double beta1 = ((dA * B) * h1 - B.z() * dA.z() * h2) / alpha;
        double gamma0 = ((A0 * A0) * h1 - A0.z() * A0.z() * h2) / alpha;
        double gamma1 = ((A0 * dA) * h1 - A0.z() * dA.z() * h2) / alpha;
        double gamma2 = ((dA * dA) * h1 - dA.z() * dA.z() * h2) / alpha;
        oceanUniforms["Horizon1"]->set(osg::Vec3(-beta0, -beta1, alpha));
        oceanUniforms["Horizon2"]->set(osg::Vec3(
            beta0 * beta0 - gamma0, 2.0 * (beta0 * beta1 - gamma1), beta1 * beta1 - gamma2));
    }

    // angle under which a screen pixel is viewed from the camera
    float fov = 0.0f, aspectRatio = 0.0f, znear = 0.0f, zfar = 0.0f;
    float width = camera->getViewport() ? camera->getViewport()->width() : 1920.0f;
    float height = camera->getViewport() ? camera->getViewport()->height() : 1080.0f;
    camera->getProjectionMatrix().getPerspective(fov, aspectRatio, znear, zfar);

    // FIXME: pixelSize affects wave tiling, should be treated carefully
    float pixelSize = atan(tan(osg::inDegrees(fov * aspectRatio * (float)oceanPixelScale)) / (height / 2.0f));
    float k = log(oceanLambdaAndHeight[1]) / log(2.0f) - log(oceanLambdaAndHeight[0]) / log(2.0f);
    oceanUniforms["HeightOffset"]->set(-oceanLambdaAndHeight[2]);
    oceanUniforms["SeaGridLODs"]->set(osg::Vec4(
        oceanGridResolution, pixelSize * oceanGridResolution,
        log(oceanLambdaAndHeight[0]) / log(2.0f), (oceanWaveCount - 1.0f) / k));
}

bool EarthAtmosphereOcean::create(const std::string& tr, const std::string& ir,
                                  const std::string& gl, const std::string& in)
{
    unsigned int trSize = 0, irSize = 0, inSize = 0;
    unsigned char *trData = loadAllData(tr, trSize, 0), *irData = loadAllData(ir, irSize, 0);
    unsigned char *inData = loadAllData(in, inSize, 0);
    if (!trData || !irData || !inData) return false;

    osg::ref_ptr<osg::Texture> trTex = rawFloatingTexture2D(trData, 256, 64, true);
    osg::ref_ptr<osg::Texture> irTex = rawFloatingTexture2D(irData, 64, 16, true);
    osg::ref_ptr<osg::Texture> inTex = rawFloatingTexture3D(inData, 256, 128, 32, false);
    osg::ref_ptr<osg::Texture> glTex = createTexture2D(osgDB::readImageFile(gl), osg::Texture::CLAMP);
    return create(trTex.get(), irTex.get(), glTex.get(), inTex.get());
}

bool EarthAtmosphereOcean::create(osg::Texture* tr, osg::Texture* ir, osg::Texture* gl, osg::Texture* in)
{
    transmittance = tr; irradiance = ir; glare = gl; inscatter = in;
    commonUniforms["CameraToWorld"] = new osg::Uniform("CameraToWorld", osg::Matrixf());
    commonUniforms["CameraToScreen"] = new osg::Uniform("CameraToScreen", osg::Matrixf());
    commonUniforms["ScreenToCamera"] = new osg::Uniform("ScreenToCamera", osg::Matrixf());
    commonUniforms["WorldSunDir"] = new osg::Uniform("WorldSunDir", osg::Vec3(-1.0f, 0.0f, 0.0f));
    commonUniforms["WorldCameraPos"] = new osg::Uniform("WorldCameraPos", osg::Vec3(0.0f, 0.0f, 0.0f));
    commonUniforms["WorldCameraLLA"] = new osg::Uniform("WorldCameraLLA", osg::Vec3(0.0f, 0.0f, 0.0f));
    commonUniforms["EarthOrigin"] = new osg::Uniform("EarthOrigin", osg::Vec3(0.0f, 0.0f, 0.0f));
    commonUniforms["GlobalOpaque"] = new osg::Uniform("GlobalOpaque", 1.0f);
    commonUniforms["OceanOpaque"] = new osg::Uniform("OceanOpaque", 1.0f);
    commonUniforms["UnderOcean"] = new osg::Uniform("UnderOcean", 1.0f);
    commonUniforms["HdrExposure"] = new osg::Uniform("HdrExposure", 0.25f);

    oceanUniforms["ScreenSize"] = new osg::Uniform("ScreenSize", osg::Vec2(1920.0f, 1080.0f));
    oceanUniforms["CameraToOcean"] = new osg::Uniform("CameraToOcean", osg::Matrixf());
    oceanUniforms["OceanToCamera"] = new osg::Uniform("OceanToCamera", osg::Matrixf());
    oceanUniforms["OceanToWorld"] = new osg::Uniform("OceanToWorld", osg::Matrixf());
    oceanUniforms["SeaGridLODs"] = new osg::Uniform("SeaGridLODs", osg::Vec4());
    oceanUniforms["SeaColor"] = new osg::Uniform("SeaColor", osg::Vec3(1.f / 255.f, 4.f / 255.f, 12.f / 255.f));
    oceanUniforms["OceanCameraPos"] = new osg::Uniform("OceanCameraPos", osg::Vec3());
    oceanUniforms["OceanSunDir"] = new osg::Uniform("OceanSunDir", osg::Vec3());
    oceanUniforms["Horizon1"] = new osg::Uniform("Horizon1", osg::Vec3());
    oceanUniforms["Horizon2"] = new osg::Uniform("Horizon2", osg::Vec3());
    oceanUniforms["Radius"] = new osg::Uniform("Radius", (float)osg::WGS_84_RADIUS_EQUATOR);
    oceanUniforms["WaveCount"] = new osg::Uniform("WaveCount", (float)oceanWaveCount);
    oceanUniforms["HeightOffset"] = new osg::Uniform("HeightOffset", 0.0f);
    oceanUniforms["SeaRoughness"] = new osg::Uniform("SeaRoughness", 0.0f);
    return true;
}

osg::Geometry* EarthAtmosphereOcean::createOceanGrid(int width, int height)
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
    oceanUniforms["ScreenSize"] = new osg::Uniform("ScreenSize", osg::Vec2((float)width, (float)height));

    float f = 1.25f;
    int NX = int(f * width / oceanGridResolution);
    int NY = int(f * height / oceanGridResolution);
    for (int i = 0; i < NY; ++i)
    {
        for (int j = 0; j < NX; ++j)
            va->push_back(osg::Vec3(2.0 * f * j / ((float)NX - 1.0f) - f,
                                    2.0 * f * i / ((float)NY - 1.0f) - f, 0.0f));
    }

    for (int i = 0; i < NY - 1; ++i)
    {
        for (int j = 0; j < NX - 1; ++j)
        {
            de->push_back(i * NX + j); de->push_back(i * NX + j + 1); de->push_back((i + 1) * NX + j);
            de->push_back((i + 1) * NX + j); de->push_back(i * NX + j + 1); de->push_back((i + 1) * NX + j + 1);
        }
    }

    osg::Geometry* geom = new osg::Geometry;
    geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va); geom->addPrimitiveSet(de); return geom;
}

osg::Texture* EarthAtmosphereOcean::createOceanWaves(float& seaRoughness)
{
    long seed = 1234567;
    float min = log(oceanLambdaAndHeight[0]) / log(2.0f);
    float max = log(oceanLambdaAndHeight[1]) / log(2.0f);

    std::vector<osg::Vec4> waves(oceanWaveCount);
    float sigmaXsq = 0.0f, sigmaYsq = 0.0f;
    float heightVariance = 0.0f, amplitudeMax = 0.0f;
    oceanLambdaAndHeight[2] = 0.0f;

#define nbAngles  5 // impair
#define angle(i)  (1.5 * (((i) % nbAngles) / (float)(nbAngles / 2) - 1))
#define dangle()  (1.5 / (float)(nbAngles / 2))
    float s = 0.0f;
    float Wa[nbAngles];  // normalised gaussian samples
    int index[nbAngles]; // to hash angle order
    for (int i = 0; i < nbAngles; i++)
    {
        index[i] = i; float a = angle(i); // (i/(float)(nbAngle/2)-1)*1.5;
        s += Wa[i] = exp(-.5f * a * a);
    }
    for (int i = 0; i < nbAngles; i++) Wa[i] /= s;

    const float waveDispersion = 0.9f, U0 = 10.0f;
    const int spectrumType = 2;
    for (int i = 0; i < oceanWaveCount; ++i)
    {
        float x = i / float(oceanWaveCount - 1.0f);
        float lambda = pow(2.0f, (1.0f - x) * min + x * max);
        float ktheta = grandom(0.0f, 1.0f, &seed) * waveDispersion;
        float knorm = 2.0f * osg::PI / lambda;
        float omega = sqrt(9.81f * knorm);
        float amplitude = 0.0f;
        if (spectrumType == 1)
            amplitude = oceanLambdaAndHeight[3] * grandom(0.5f, 0.15f, &seed)
                      / (knorm * oceanLambdaAndHeight[1] / (2.0f * osg::PI));
        else if (spectrumType == 2)
        {
            float step = (max - min) / (oceanWaveCount - 1); // dlambda/di
            float omega0 = 9.81f / U0; // 100.0;
            if ((i % (nbAngles)) == 0)
            {   // scramble angle ordre
                for (int k = 0; k < nbAngles; k++)
                {   // do N swap in indices
                    int n1 = lrandom(&seed) % nbAngles, n2 = lrandom(&seed) % nbAngles, n;
                    n = index[n1]; index[n1] = index[n2]; index[n2] = n;
                }
            }
            ktheta = waveDispersion * (angle(index[(i) % nbAngles]) + .4 * srnd() * dangle());
            ktheta *= 1 / (1 + 40 * pow(omega0 / omega, 4));
            amplitude = (8.1e-3 * 9.81f * 9.81f) / pow(omega, 5.0f) * exp(-0.74f * pow(omega0 / omega, 4.0f));
            amplitude *= .5 * sqrt(2.0f * 3.14f * 9.81f / lambda) * nbAngles * step; // (2/step-step/2);
            amplitude = 3.0f * oceanLambdaAndHeight[3] * sqrt(amplitude);
        }

        // cull breaking trochoids ( d(x+Acos(kx))=1-Akcos(); must be >0 )
        if (amplitude > 1.0f / knorm) amplitude = 1.0f / knorm;
        else if (amplitude < -1.0f / knorm) amplitude = -1.0f / knorm;

        waves[i].x() = amplitude; waves[i].y() = omega;
        waves[i].z() = knorm * cos(ktheta);
        waves[i].w() = knorm * sin(ktheta);
        sigmaXsq += pow(cos(ktheta), 2.0f) * (1.0f - sqrt(1.0f - knorm * knorm * amplitude * amplitude));
        sigmaYsq += pow(sin(ktheta), 2.0f) * (1.0f - sqrt(1.0f - knorm * knorm * amplitude * amplitude));
        oceanLambdaAndHeight[2] -= knorm * amplitude * amplitude * 0.5f;
        heightVariance += amplitude * amplitude * (2.0f - knorm * knorm * amplitude * amplitude) * 0.25f;
        amplitudeMax += fabs(amplitude);
    }

    float var = 4.0f;
    float h0 = oceanLambdaAndHeight[2] - var * sqrt(heightVariance);
    float h1 = oceanLambdaAndHeight[2] + var * sqrt(heightVariance);
    amplitudeMax = h1 - h0;

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(oceanWaveCount, 1, 1, GL_RGBA, GL_FLOAT);
    image->setInternalTextureFormat(GL_RGBA32F_ARB);
    memcpy(image->data(), waves.data(), waves.size() * sizeof(osg::Vec4));

#if false
    osg::ref_ptr<osg::Texture1D> tex = new osg::Texture1D;
#else
    osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
#endif
    tex->setImage(image.get()); tex->setResizeNonPowerOfTwoHint(false);
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    if (oceanUniforms.find("SeaRoughness") != oceanUniforms.end())
        oceanUniforms["SeaRoughness"]->set(sigmaXsq);
    seaRoughness = sigmaXsq; return tex.release();
}

osg::Texture* EarthAtmosphereOcean::rawFloatingTexture2D(unsigned char* data, int w, int h, bool rgb)
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
    tex2D->setImage(image.get()); return tex2D.release();
}

osg::Texture* EarthAtmosphereOcean::rawFloatingTexture3D(unsigned char* data, int w, int h, int d, bool rgb)
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
    tex3D->setImage(image.get()); return tex3D.release();
}

/************** SSIM **************/
namespace
{
    struct SsimImageF
    {
        int w, h, c; std::vector<float> data;
        float& at(int x, int y, int ch) { return data[(y * w + x) * c + ch]; }
        const float& at(int x, int y, int ch) const { return data[(y * w + x) * c + ch]; }
        SsimImageF(int ww = 0, int hh = 0, int cc = 0)
            : w(ww), h(hh), c(cc), data((size_t)ww * (size_t)hh * (size_t)cc, 0.0f) {}
    };

    SsimImageF ssimConvertImage(const osg::Image& img)
    {
        SsimImageF result;
        result.w = img.s(); result.h = img.t();
        result.c = osg::Image::computeNumComponents(img.getPixelFormat());
        result.data.resize(result.w * result.h * result.c);

        for (int y = 0; y < result.h; ++y)
            for (int x = 0; x < result.w; ++x)
            {
                osg::Vec4 color = img.getColor(x, y);
                for (int c = 0; c < result.c; ++c) result.at(x, y, c) = color[c];
            }
        return result;
    }

    std::vector<float> ssimGaussianKernel(int size, float sigma)
    {
        std::vector<float> kernel(size * size);
        float sum = 0.0f, sigma_sq = 2.0f * sigma * sigma;
        int half = size / 2;

        for (int y = -half; y <= half; ++y)
            for (int x = -half; x <= half; ++x)
            {
                float val = std::exp(-(x * x + y * y) / sigma_sq);
                kernel[(y + half) * size + (x + half)] = val; sum += val;
            }
        for (auto& v : kernel) v /= sum;
        return kernel;
    }

    void ssimGaussianBlur(const SsimImageF& src, SsimImageF& dst,
                          const std::vector<float>& kernel, int ksize)
    {
        int half = ksize / 2;
#pragma omp parallel for schedule(dynamic)
        for (int c = 0; c < src.c; ++c)
            for (int y = 0; y < src.h; ++y)
                for (int x = 0; x < src.w; ++x)
                {
                    float sum = 0.0f;
                    for (int ky = -half; ky <= half; ++ky)
                        for (int kx = -half; kx <= half; ++kx)
                        {
                            int py = osg::clampBetween(y + ky, 0, src.h - 1);
                            int px = osg::clampBetween(x + kx, 0, src.w - 1);
                            float weight = kernel[(ky + half) * ksize + (kx + half)];
                            float pixel = src.at(px, py, c); sum += pixel * weight;
                        }
                    dst.at(x, y, c) = sum;
                }
    }
}

SSIM::Result SSIM::compute(const osg::Image& img0, const osg::Image& img1, float C1, float C2, int winSize, float sigma)
{
    SSIM::Result result; result.width = img0.s(); result.height = img0.t();
    if (img0.s() != img1.s() || img0.t() != img1.t())
    { OSG_NOTICE << "[SSIM] Image dimensions mismatch\n"; return result; }

    SsimImageF f0 = ssimConvertImage(img0);
    SsimImageF f1 = ssimConvertImage(img1);
    int size = f0.w * f0.h * f0.c;

    std::vector<float> kernel = ssimGaussianKernel(winSize, sigma);
    SsimImageF mu0{ f0.w, f0.h, f0.c }, mu1{ f0.w, f0.h, f0.c };
#pragma omp parallel sections
    {
#pragma omp section
        ssimGaussianBlur(f0, mu0, kernel, winSize);
#pragma omp section
        ssimGaussianBlur(f1, mu1, kernel, winSize);
    }

    SsimImageF img0_sq{ f0.w, f0.h, f0.c };
    SsimImageF img1_sq{ f0.w, f0.h, f0.c };
    SsimImageF img0_img1{ f0.w, f0.h, f0.c };
#pragma omp parallel for schedule(static)
    for (int i = 0; i < size; ++i)
    {
        img0_sq.data[i] = f0.data[i] * f0.data[i];
        img1_sq.data[i] = f1.data[i] * f1.data[i];
        img0_img1.data[i] = f0.data[i] * f1.data[i];
    }

    SsimImageF sigma0_sq{ f0.w, f0.h, f0.c }, sigma1_sq{ f0.w, f0.h, f0.c }, sigma01{ f0.w, f0.h, f0.c };
#pragma omp parallel sections
    {
#pragma omp section
        ssimGaussianBlur(img0_sq, sigma0_sq, kernel, winSize);
#pragma omp section
        ssimGaussianBlur(img1_sq, sigma1_sq, kernel, winSize);
#pragma omp section
        ssimGaussianBlur(img0_img1, sigma01, kernel, winSize);
    }

#pragma omp parallel for schedule(static)
    for (int i = 0; i < size; ++i)
    {
        sigma0_sq.data[i] -= mu0.data[i] * mu0.data[i];
        sigma1_sq.data[i] -= mu1.data[i] * mu1.data[i];
        sigma01.data[i] -= mu0.data[i] * mu1.data[i];
    }
    
    double ssim_sum = 0.0; result.ssim_map.resize(size);
#pragma omp parallel for reduction(+:ssim_sum) schedule(static)
    for (int i = 0; i < size; ++i)
    {
        float mu0_sq = mu0.data[i] * mu0.data[i];
        float mu1_sq = mu1.data[i] * mu1.data[i];
        float mu0_mu1 = mu0.data[i] * mu1.data[i];
        float numerator = (2.0f * mu0_mu1 + C1) * (2.0f * sigma01.data[i] + C2);
        float denominator = (mu0_sq + mu1_sq + C1) * (sigma0_sq.data[i] + sigma1_sq.data[i] + C2);
        result.ssim_map[i] = numerator / denominator;
        ssim_sum += result.ssim_map[i];
    }
    result.ssim = static_cast<float>(ssim_sum / size);
    return result;
}

/************** TextureCopier **************/
TextureCopier* TextureCopier::instance()
{
    static osg::ref_ptr<TextureCopier> s_instance = new TextureCopier;
    return s_instance.get();
}

TextureCopier::CopyRegion TextureCopier::calculateCenteredCopyRegion(int srcW, int srcH, int dstW, int dstH)
{
    CopyRegion region{};
    float scaleX = static_cast<float>(dstW) / srcW;
    float scaleY = static_cast<float>(dstH) / srcH;
    float scale = osg::minimum(scaleX, scaleY);

    int copyW = 0, copyH = 0;
    if (scale >= 1.0f)
    {   // copy entire src texture and center it
        copyW = srcW; copyH = srcH;
        region.srcX = 0; region.srcY = 0;
        region.srcWidth = srcW; region.srcHeight = srcH;

        region.dstX = (dstW - copyW) / 2;
        region.dstY = (dstH - copyH) / 2;
        region.dstWidth = copyW; region.dstHeight = copyH;
    }
    else
    {   // clip src texture to fill entire target
        copyW = dstW; copyH = dstH;
        region.srcX = (srcW - copyW) / 2;
        region.srcY = (srcH - copyH) / 2;
        region.srcWidth = copyW; region.srcHeight = copyH;
        region.dstX = 0; region.dstY = 0;
        region.dstWidth = dstW; region.dstHeight = dstH;
    }
    return region;
}

bool TextureCopier::operator()(osg::Texture2D& srcT, osg::FrameBufferObject* srcFBO, osg::Texture2D& dstT, osg::State* state) const
{
    if (srcT.getImage() && srcT.getImage()->valid())
        return (*this)(srcT, srcFBO, 0, 0, srcT.getImage()->s(), srcT.getImage()->t(), dstT, 0, 0, state);
    else
        return (*this)(srcT, srcFBO, 0, 0, srcT.getTextureWidth(), srcT.getTextureHeight(), dstT, 0, 0, state);
}

bool TextureCopier::operator()(osg::Texture2D& srcTexture, osg::FrameBufferObject* srcFBO, int srcX, int srcY, int srcW, int srcH,
                               osg::Texture2D& dstTexture, int dstX, int dstY, osg::State* state) const
{
    unsigned int srcFboID = 0, contextID = state ? state->getContextID() : 0;
    osg::Texture::TextureObject* srcObj = srcTexture.getTextureObject(contextID);
    osg::Texture::TextureObject* dstObj = dstTexture.getTextureObject(contextID);
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
    srcFboID = srcFBO ? srcFBO->getHandle(contextID) : 0; if (!srcObj || !dstObj) return false;
    return (*this)(srcObj->id(), srcFboID, srcX, srcY, srcW, srcH, dstObj->id(), dstX, dstY, state);
#else
    OSG_NOTICE << "[TextureCopier] Not implemented\n"; return false;
#endif
}

bool TextureCopier::operator()(unsigned int srcTexture, unsigned int srcFBO, int srcX, int srcY, int srcW, int srcH,
                               unsigned int dstTexture, int dstX, int dstY, osg::State* state) const
{
    typedef void (GL_APIENTRY* CopyImageSubDataProc)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY,
                                                     GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX,
                                                     GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
    if (!_initialized)
    {
        CopyImageSubDataProc glCopyImageSubData = NULL;
        osg::setGLExtensionFuncPtr(
            glCopyImageSubData, "glCopyImageSubData", "glCopyImageSubDataEXT", "glCopyImageSubDataNV");
        _glCopyImageSubDataPtr = (void*)glCopyImageSubData; _initialized = true;
    }

    if (srcTexture != 0 && _glCopyImageSubDataPtr != NULL)
    {
        CopyImageSubDataProc glCopyImageSubData = (CopyImageSubDataProc)_glCopyImageSubDataPtr;
        glCopyImageSubData(srcTexture, GL_TEXTURE_2D, 0, srcX, srcY, 0,
                           dstTexture, GL_TEXTURE_2D, 0, dstX, dstY, 0, srcW, srcH, 1);
    }
    else if (state)
    {
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
        osg::GLExtensions* ext = state->get<osg::GLExtensions>();
        if (ext->isFrameBufferObjectSupported)
#else
        osg::FBOExtensions* ext = osg::FBOExtensions::instance(state->getContextID(), true);
        if (ext->isSupported())
#endif
        {
            unsigned int resolveFboId = 0;
            if (_texToFboMap.find(dstTexture) == _texToFboMap.end())
            {
                ext->glGenFramebuffers(1, &resolveFboId);
                _texToFboMap[dstTexture] = resolveFboId;
            }
            else
                resolveFboId = _texToFboMap[dstTexture];

            ext->glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, resolveFboId);
            ext->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, dstTexture, 0);
            ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, srcFBO);
            ext->glBlitFramebuffer(srcX, srcY, srcX + srcW, srcX + srcH,
                                   dstX, dstY, dstX + srcW, dstY + srcH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, srcFBO);
            /*ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, srcFBO);
            glBindTexture(GL_TEXTURE_2D, dstTexture);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, dstX, dstY, srcX, srcY, srcW, srcH);*/
        }
    }
    return (state && !state->checkGLErrors("TextureCopier"));
}
