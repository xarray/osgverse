#ifndef MANA_ANIM_PARTICLEENGINE_HPP
#define MANA_ANIM_PARTICLEENGINE_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <map>

namespace Effekseer
{
    class Manager;
    class Effect;
}

namespace osgVerse
{

    class ParticleSystemU3D : public osg::NodeCallback
    {
    public:
        enum UpdateMethod { CPU_ONLY, FRAME_RT };
        ParticleSystemU3D(UpdateMethod method);
        ParticleSystemU3D(const ParticleSystemU3D& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        // Link this particle system to a geode to make it work
        void linkTo(osg::Geode* geode);

        enum EmissionShape { EMIT_Point, EMIT_Plane, EMIT_Sphere, EMIT_Box, EMIT_Mesh };
        enum EmissionSurface { EMIT_Volume, EMIT_Shell };
        enum ParticleType { PARTICLE_Billboard, PARTICLE_Line, PARTICLE_Mesh };

        // Basic properties
        void setGeometry(osg::Geometry* g) { _geometry = g; _dirty = true; }
        void setTexture(osg::Texture2D* t) { _texture = t; _dirty = true; }
        void setStartLifeRange(const osg::Vec2& v) { _startLifeRange = v; }
        void setStartSizeRange(const osg::Vec2& v) { _startSizeRange = v; }
        void setStartSpeedRange(const osg::Vec2& v) { _startSpeedRange = v; }
        void setMaxParticles(double v) { _maxParticles = v; }
        void setStartDelay(double v) { _startDelay = v; }
        void setGravityScale(double v) { _gravityScale = v; }
        void setParticleType(ParticleType t) { _particleType = t; _dirty = true; }

        osg::Geometry* getGeometry() { return _geometry.get(); }
        osg::Texture2D* getTexture() { return _texture.get(); }
        const osg::Vec2& getStartLifeRange() const { return _startLifeRange; }
        const osg::Vec2& getStartSizeRange() const { return _startSizeRange; }
        const osg::Vec2& getStartSpeedRange() const { return _startSpeedRange; }
        double getMaxParticles() const { return _maxParticles; }
        double getStartDelay() const { return _startDelay; }
        double getGravityScale() const { return _gravityScale; }
        ParticleType getParticleType() const { return _particleType; }

        /// Emission properties
        void setEmissionBursts(const std::map<float, osg::Vec4>& m) { _emissionBursts = m; }
        void setEmissionShapeCenter(const osg::Vec3& v) { _emissionShapeCenter = v; }
        void setEmissionShapeEulers(const osg::Vec3& v) { _emissionShapeEulers = v; }
        void setEmissionShapeValues(const osg::Vec3& v) { _emissionShapeValues = v; }
        void setEmissionCount(const osg::Vec2& v) { _emissionCount = v; }
        void setEmissionShape(EmissionShape s) { _emissionShape = s; }
        void setEmissionSurface(EmissionSurface s) { _emissionSurface = s; }

        std::map<float, osg::Vec4>& getEmissionBursts() { return _emissionBursts; }
        const osg::Vec3& getEmissionShapeCenter() const { return _emissionShapeCenter; }
        const osg::Vec3& getEmissionShapeEulers() const { return _emissionShapeEulers; }
        const osg::Vec3& getEmissionShapeValues() const { return _emissionShapeValues; }
        const osg::Vec2& getEmissionCount() const { return _emissionCount; }
        EmissionShape getEmissionShape() const  { return _emissionShape; }
        EmissionSurface getEmissionSurface() const { return _emissionSurface; }

        // Collision properties
        void setCollisionPlanes(const std::vector<osg::Plane>& v) { _collisionPlanes = v; }
        void setCollisionValues(const osg::Vec4& v) { _collisionValues = v; }

        std::vector<osg::Plane>& getCollisionPlanes() { return _collisionPlanes; }
        const osg::Vec4& getCollisionValues() const { return _collisionValues; }

        /// Texture sheet animation properties
        void setTextureSheetRange(const osg::Vec4& v) { _textureSheetRange = v; }
        void setTextureSheetValues(const osg::Vec4& v) { _textureSheetValues = v; }
        void setTextureSheetTiles(const osg::Vec2& v) { _textureSheetTiles = v; }

        const osg::Vec4& getTextureSheetRange() const { return _textureSheetRange; }
        const osg::Vec4& getTextureSheetValues() const { return _textureSheetValues; }
        const osg::Vec2& getTextureSheetTiles() const { return _textureSheetTiles; }

        // Timeline based operators
        void setColorPerTime(const std::map<float, osg::Vec4>& m) { _colorPerTime = m; }
        void setColorPerSpeed(const std::map<float, osg::Vec4>& m) { _colorPerSpeed = m; }
        void setEulersPerTime(const std::map<float, osg::Vec3>& m) { _eulersPerTime = m; }
        void setEulersPerSpeed(const std::map<float, osg::Vec3>& m) { _eulersPerSpeed = m; }
        void setVelocityPerTime(const std::map<float, osg::Vec3>& m) { _velocityOffsets = m; }
        void setForcePerTime(const std::map<float, osg::Vec3>& m) { _forceOffsets = m; }
        void setScalePerTime(const std::map<float, float>& m) { _scalePerTime = m; }
        void setScalePerSpeed(const std::map<float, float>& m) { _scalePerSpeed = m; }

        std::map<float, osg::Vec4>& getColorPerTime() { return _colorPerTime; }
        std::map<float, osg::Vec4>& getColorPerSpeed() { return _colorPerSpeed; }
        std::map<float, osg::Vec3>& getEulersPerTime() { return _eulersPerTime; }
        std::map<float, osg::Vec3>& getEulersPerSpeed() { return _eulersPerSpeed; }
        std::map<float, osg::Vec3>& getVelocityPerTime() { return _velocityOffsets; }
        std::map<float, osg::Vec3>& getForcePerTime() { return _forceOffsets; }
        std::map<float, float>& getScalePerTime() { return _scalePerTime; }
        std::map<float, float>& getScalePerSpeed() { return _scalePerSpeed; }

    protected:
        void recreate();

        std::map<float, osg::Vec4> _emissionBursts;  // [time]: count, cycles, interval, probability (0-1)
        std::map<float, osg::Vec4> _colorPerTime, _colorPerSpeed;  // [time/speed]: color
        std::map<float, osg::Vec3> _eulersPerTime, _eulersPerSpeed;  // [time/speed]: euler value
        std::map<float, osg::Vec3> _velocityOffsets, _forceOffsets;  // [time]: offset
        std::map<float, float> _scalePerTime, _scalePerSpeed;  // [time/speed]: scale value
        std::vector<osg::Plane> _collisionPlanes;

        osg::ref_ptr<osg::Texture2D> _texture;
        osg::ref_ptr<osg::Geometry> _geometry, _geometry2;
        osg::Vec4 _collisionValues;      // dampen, bounce scale, lifetime loss, min kill speed
        osg::Vec4 _textureSheetRange;    // Sheet X0, Y0, W, H
        osg::Vec4 _textureSheetValues;   // playing speed by lifetime, by velocity, by FPS, and cycles
        osg::Vec3 _emissionShapeCenter, _emissionShapeEulers;
        osg::Vec3 _emissionShapeValues;  // Plane: normal; Sphere: radii, Box: sizes
        osg::Vec2 _textureSheetTiles;    // texture sheet X, Y
        osg::Vec2 _emissionCount;        // count per time, count per distance
        osg::Vec2 _startLifeRange, _startSizeRange, _startSpeedRange;
        double _maxParticles, _startDelay, _gravityScale;
        EmissionShape _emissionShape;
        EmissionSurface _emissionSurface;
        ParticleType _particleType;
        bool _dirty;
    };

    class ParticleDrawable : public osg::Drawable
    {
    public:
        ParticleDrawable(int maxInstances = 8000);
        ParticleDrawable(const ParticleDrawable& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
        virtual const char* libraryName() const { return "osgVerse"; }
        virtual const char* className() const { return "ParticleDrawable"; }

        virtual Object* cloneType() const { return new ParticleDrawable; }
        virtual Object* clone(const osg::CopyOp& copyop) const { return new ParticleDrawable(*this, copyop); }
        virtual bool isSameKindAs(const osg::Object* obj) const
        { return dynamic_cast<const ParticleDrawable*>(obj) != NULL; }

        enum PlayingState
        { INVALID = -1, STOPPED = 0, PLAYING = 1, PAUSED = 2 };

        Effekseer::Effect* createEffect(const std::string& name, const std::string& fileName);
        void destroyEffect(const std::string& name);
        bool playEffect(const std::string& name, PlayingState state);

        PlayingState getEffectState(const std::string& name) const;
        Effekseer::Effect* getEffect(const std::string& name) const;
        Effekseer::Manager* getManager() const;

#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
        virtual osg::BoundingBox computeBoundingBox() const;
        virtual osg::BoundingSphere computeBound() const;
#else
        virtual osg::BoundingBox computeBound() const;
#endif
        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
        virtual void releaseGLObjects(osg::State* state) const;

    protected:
        virtual ~ParticleDrawable();

        osg::ref_ptr<osg::Referenced> _data;
    };

}

#endif
