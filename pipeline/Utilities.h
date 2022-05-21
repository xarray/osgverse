#ifndef MANA_PP_UTILITIES_HPP
#define MANA_PP_UTILITIES_HPP

#include <osg/Polytope>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/Camera>
struct SMikkTSpaceContext;

namespace osgVerse
{
    /** Global-defined vertex attribute names, for full-featured pipeline use */
    static std::string attributeNames[] =
    {
        /*0*/"osg_Vertex", /*1*/"osg_Weights", /*2*/"osg_Normal", /*3*/"osg_Color",
        /*4*/"osg_SecondaryColor", /*5*/"osg_FogCoord", /*6*/"osg_Tangent", /*7*/"osg_Binormal",
        /*8*/"osg_TexCoord0", /*9*/"osg_TexCoord1", /*10*/"osg_TexCoord2", /*11*/"osg_TexCoord3",
        /*12*/"osg_TexCoord4", /*13*/"osg_TexCoord5", /*14*/"osg_TexCoord6", /*15*/"osg_TexCoord7"
    };

    /** Global-defined texture-map uniform names, for full-featured pipeline use */
    static std::string uniformNames[] =
    {
        /*0*/"DiffuseMap", /*1*/"NormalMap", /*2*/"SpecularMap", /*3*/"ShininessMap",
        /*4*/"AmbientMap", /*5*/"EmissiveMap", /*6*/"ReflectionMap"
    };

    /** Suggest run this function once to initialize some plugins & environments */
    extern void globalInitialize();

    /** Create default texture for untextured model */
    extern osg::Texture2D* createDefaultTexture(const osg::Vec4& color);

    /** Create 2D texture from an input image */
    extern osg::Texture2D* createTexture2D(osg::Image* img, osg::Texture::WrapMode m = osg::Texture::REPEAT);

    /** Create a XOY quad, often for screen-rendering use */
    extern osg::Geode* createScreenQuad(const osg::Vec3& corner, float width, float height,
                                        const osg::Vec4& uvRange);

    /** Create a standard pre-render RTT camera, may contain a quad for deferred use */
    extern osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture* tex,
                                        osg::GraphicsContext* gc, bool screenSpaced);

    /** Create a standard post-render HUD camera, may contain a quad for display use */
    extern osg::Camera* createHUDCamera(osg::GraphicsContext* gc, int w, int h, const osg::Vec3& quadPt,
                                        float quadW, float quadH, bool screenSpaced);

    /** The tangent/binormal computing visitor */
    class TangentSpaceVisitor : public osg::NodeVisitor
    {
    public:
        TangentSpaceVisitor(const float angularThreshold = 180.0f);
        virtual ~TangentSpaceVisitor();
        virtual void apply(osg::Drawable& node) {}  // do nothing
        virtual void apply(osg::Geometry& geometry) {}  // do nothing
        virtual void apply(osg::Geode& node);

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

        virtual void apply(osg::Drawable& node) {}  // do nothing
        virtual void apply(osg::Geometry& geometry) {}  // do nothing
        virtual void apply(osg::Node& node);
        virtual void apply(osg::Geode& node);
        void apply(osg::StateSet& ss);

    protected:
        double _nStrength, _spScale, _spContrast;
        int _normalMapUnit, _specMapUnit;
        bool _nInvert;
    };

    /** The frustum geometry which is used by shadow computation */
    struct Frustum
    {
        osg::Vec3d corners[8];
        osg::Vec3d centerNearPlane, centerFarPlane;
        osg::Vec3d center, frustumDir;

        /** Create frustum polytope from given MVP matrices, may change near/far if needed */
        void create(const osg::Matrix& modelview, const osg::Matrix& proj,
                    float preferredNear = -1.0f, float preferredFar = -1.0f);

        /** Compute minimum light-space bounding box from given frustum and some reference points.
            Result in light-space can be used for setting shadow camera pojection matrix */
        osg::BoundingBox createShadowBound(const std::vector<osg::Vec3>& refPoints,
                                           const osg::Matrix& worldToLocal);
    };

}

#endif
