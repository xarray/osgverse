#ifndef MANA_UTILITIES_HPP
#define MANA_UTILITIES_HPP

#include <osg/Polytope>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/Camera>
struct SMikkTSpaceContext;

namespace osgVerse
{
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