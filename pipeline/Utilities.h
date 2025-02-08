#ifndef MANA_PP_UTILITIES_HPP
#define MANA_PP_UTILITIES_HPP

#include <osg/Polytope>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/Camera>
#include <osgGA/GUIEventHandler>
#include "Global.h"
#include <functional>
#include <mutex>
struct SMikkTSpaceContext;

namespace osgVerse
{
    /** Get unique node-path like name (e.g. rootName/childA/subChildB/0) */
    extern std::string getNodePathID(osg::Object& obj, osg::Node* root = NULL, char sep = '/');

    /** Get object from root node and path ID */
    extern osg::Object* getFromPathID(const std::string& id, osg::Object* root, char sep = '/');

    /** Create 2D noises. e.g. for SSAO use */
    extern osg::Texture* generateNoises2D(int numCols, int numRows);

    /** Create poisson noises. e.g. for PCF shadow use */
    extern osg::Texture* generatePoissonDiscDistribution(int numCols, int numRows = 1);

    /** Create default texture for untextured model */
    extern osg::Texture2D* createDefaultTexture(const osg::Vec4& color);

    /** Create 2D texture from an input image */
    extern osg::Texture2D* createTexture2D(osg::Image* img, osg::Texture::WrapMode m = osg::Texture::REPEAT);

    /** Create a XOY quad, often for screen-rendering use */
    extern osg::Geode* createScreenQuad(const osg::Vec3& corner, float width, float height,
                                        const osg::Vec4& uvRange);
    
    /** Create a pre-render RTT camera for capturing sub-scene to image */
    extern osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Image* image,
                                        osg::Node* child, osg::GraphicsContext* gc = NULL);

    /** Create a pre-render RTT camera, which may contain a quad for deferred use */
    extern osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture* tex,
                                        osg::GraphicsContext* gc, bool screenSpaced);

    /** Create a list of RTT cameras to render a cubemap */
    extern osg::Group* createRTTCube(osg::Camera::BufferComponent buffer, osg::TextureCubeMap* tex,
                                     osg::Node* child, osg::GraphicsContext* gc = NULL);

    /** Create a post-render HUD camera, for displaying UI in (0,0,-1) - (w,h,1) range */
    extern osg::Camera* createHUDCamera(osg::GraphicsContext* gc, int w, int h);

    /** Create a post-render HUD camera, which may contain a quad for display use */
    extern osg::Camera* createHUDCamera(osg::GraphicsContext* gc, int w, int h, const osg::Vec3& quadPt,
                                        float quadW, float quadH, bool screenSpaced);

    /** Align the camera (usually RTT) to a bounding box to capture one of its face exactly */
    extern void alignCameraToBox(osg::Camera* camera, const osg::BoundingBoxd& bb, int resW, int resH,
                                 osg::TextureCubeMap::Face face = osg::TextureCubeMap::POSITIVE_Z);

    /** Create heightmap from given scene graph */
    extern osg::HeightField* createHeightField(osg::Node* node, int resX, int resY, osg::View* viewer = NULL);

    /** Create snapshot from given scene graph, may work as texture part of createHeightField() */
    extern osg::Image* createSnapshot(osg::Node* node, int resX, int resY, osg::View* viewer = NULL);

    /** The tangent/binormal computing visitor */
    class TangentSpaceVisitor : public osg::NodeVisitor
    {
    public:
        TangentSpaceVisitor(const float angularThreshold = 180.0f);
        virtual ~TangentSpaceVisitor();
        virtual void apply(osg::Geode& node);
        virtual void apply(osg::Geometry& geometry);

    protected:
        SMikkTSpaceContext* _mikkiTSpace;
        float _angularThreshold;
    };

    /** The normal-map & specular-map generator */
    class NormalMapGenerator : public osg::NodeVisitor
    {
    public:
        NormalMapGenerator(double nStrength = 2.0, double spScale = 0.2,
                           double spContrast = 1.0, bool nInvert = false);
        void setTextureUnits(int n, int sp) { _normalMapUnit = n; _specMapUnit = sp; }
        void setCacheFolder(const std::string& folder) { _cacheFolder = folder; }

        virtual void apply(osg::Node& node);
        virtual void apply(osg::Geode& node);
        virtual void apply(osg::Drawable& geometry);
        void apply(osg::StateSet& ss);

    protected:
        std::string _cacheFolder;
        double _nStrength, _spScale, _spContrast;
        int _normalMapUnit, _specMapUnit;
        bool _nInvert;
    };

    /** The frustum geometry which is used by shadow computation */
    struct Frustum
    {
        typedef std::pair<osg::Vec3d, osg::Vec3d> AABB;
        osg::Vec3d corners[8];
        osg::Vec3d centerNearPlane, centerFarPlane;
        osg::Vec3d center, frustumDir;

        /** Create frustum polytope from given MVP matrices, may change near/far if needed */
        void create(const osg::Matrix& modelview, const osg::Matrix& proj,
                    double preferredNear = -1.0, double preferredFar = -1.0);

        /** Compute minimum light-space bounding box from given frustum and some reference points.
            Result in light-space can be used for setting shadow camera pojection matrix */
        AABB createShadowBound(const std::vector<osg::Vec3d>& refPoints, const osg::Matrix& worldToLocal);
    };

    /** Quick event handler for testing purpose */
    class QuickEventHandler : public osgGA::GUIEventHandler
    {
    public:
        typedef std::function<void(int, int)> MouseCallback;
        typedef std::function<void (int)> KeyCallback;
        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

        void addMouseDownCallback(int btn, MouseCallback cb) { _pushCallbacks[btn] = cb; }
        void addMouseUpCallback(int btn, MouseCallback cb) { _clickCallbacks[btn] = cb; }
        void addDoubleClickCallback(int btn, MouseCallback cb) { _dbClickCallbacks[btn] = cb; }

        void addKeyDownCallback(int key, KeyCallback cb) { _keyCallbacks0[key] = cb; }
        void addKeyUpCallback(int key, KeyCallback cb) { _keyCallbacks1[key] = cb; }

        void addKeyDownCallback(int* keys, int num, KeyCallback cb)
        { for (int i = 0; i < num; ++i) _keyCallbacks0[keys[i]] = cb; }
        void addKeyUpCallback(int* keys, int num, KeyCallback cb)
        { for (int i = 0; i < num; ++i) _keyCallbacks1[keys[i]] = cb; }

    protected:
        std::map<int, MouseCallback> _pushCallbacks, _clickCallbacks, _dbClickCallbacks;
        std::map<int, KeyCallback> _keyCallbacks0, _keyCallbacks1;
    };

    /** Colorized LOG handler that can help display shader information */
    class ConsoleHandler : public osg::NotifyHandler
    {
    public:
        ConsoleHandler();
        virtual void notifyLevel0(osg::NotifySeverity severity, const std::string& message);
        virtual void notifyLevel1(osg::NotifySeverity severity, const std::string& message);
        virtual void notifyLevel2(osg::NotifySeverity severity, const std::string& message);
        virtual void notifyLevel3(osg::NotifySeverity severity, const std::string& message) {}

        void setShowStackTrace(bool b) { _showStackTrace = b; }
        bool getShowStackTrace() const { return _showStackTrace; }

        typedef std::function<std::string(GLenum, const std::string&, const std::string&)> ShaderLogCallback;
        typedef std::function<std::string(const std::string&, const std::string&)> ProgramLogCallback;

        void setShaderLogCallback(ShaderLogCallback cb) { _shaderCallback = cb; }
        void setProgramLogCallback(ProgramLogCallback cb) { _programCallback = cb; }

    protected:
        virtual void notify(osg::NotifySeverity severity, const char* message);
        std::string convertInformation(const std::string& msg);
        std::string getDateTimeTick();

        ShaderLogCallback _shaderCallback;
        ProgramLogCallback _programCallback;
        void* _handle; bool _showStackTrace;
    };

    /** Handle IME events under Windows */
#ifdef VERSE_WINDOWS
    class TextInputMethodManager : public osg::Referenced
    {
    public:
        static TextInputMethodManager* instance();
        static void disable(osg::GraphicsContext* gc);
        virtual void bind(osg::GraphicsContext* gc) = 0;
        virtual void unbind() = 0;
        virtual void updateNotifier() = 0;
        virtual void setFocus(bool b) = 0;
    };
#endif

    class DisableBoundingBoxCallback : public osg::Drawable::ComputeBoundingBoxCallback
    {
    public:
        virtual osg::BoundingBox computeBound(const osg::Drawable&) const
        { return osg::BoundingBox(); }
    };

    class DisableDrawableCallback : public osg::Drawable::CullCallback
    {
    public:
        virtual bool cull(osg::NodeVisitor*, osg::Drawable* drawable, osg::State*) const
        { return true; }
    };

    class DisableNodeCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        { /*traverse(node, nv);*/ }
    };
}

#ifdef OSG_LIBRARY_STATIC
#   define USE_OSG_PLUGINS_ONLY() \
    USE_OSGPLUGIN(glsl) \
    USE_OSGPLUGIN(trans) \
    USE_OSGPLUGIN(rot) \
    USE_OSGPLUGIN(scale) \
    USE_OSGPLUGIN(osg) \
    USE_OSGPLUGIN(osg2) \
    USE_OSGPLUGIN(rgb) \
    USE_OSGPLUGIN(bmp) \
    USE_DOTOSGWRAPPER_LIBRARY(osg) \
    USE_SERIALIZER_WRAPPER_LIBRARY(osg) \
    USE_SERIALIZER_WRAPPER_LIBRARY(osgSim) \
    USE_SERIALIZER_WRAPPER_LIBRARY(osgTerrain) \
    USE_SERIALIZER_WRAPPER_LIBRARY(osgText)
// Note: plugins depending on external libraries should be called manually
//  USE_OSGPLUGIN(jpg)
//  USE_OSGPLUGIN(png)
//  USE_OSGPLUGIN(freetype)
#   if defined(VERSE_WASM) || defined(VERSE_NO_NATIVE_WINDOW)
#       define USE_OSG_PLUGINS() USE_OSG_PLUGINS_ONLY()
#   else
#       define USE_OSG_PLUGINS() \
            USE_OSG_PLUGINS_ONLY() \
            USE_GRAPHICSWINDOW()
#   endif
#else
#   define USE_OSG_PLUGINS()
#endif

#ifdef VERSE_STATIC_BUILD
#   define USE_VERSE_PLUGINS() \
    USE_OSGPLUGIN(verse_ept) \
    USE_OSGPLUGIN(verse_fbx) \
    USE_OSGPLUGIN(verse_gltf) \
    USE_OSGPLUGIN(verse_ktx) \
    USE_OSGPLUGIN(verse_mvt) \
    USE_OSGPLUGIN(verse_web) \
    USE_OSGPLUGIN(verse_image) \
    USE_OSGPLUGIN(verse_leveldb) \
    USE_OSGPLUGIN(verse_tiles) \
    USE_OSGPLUGIN(pbrlayout)
// Note: plugins depending on external libraries should be called manually
//  USE_OSGPLUGIN(verse_webp)
//  USE_OSGPLUGIN(verse_ms)
//  USE_OSGPLUGIN(verse_cesium)
//  USE_OSGPLUGIN(verse_vdb)
#else
#   define USE_VERSE_PLUGINS()
#endif

#endif
