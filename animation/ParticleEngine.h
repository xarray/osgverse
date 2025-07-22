#ifndef MANA_ANIM_PARTICLEENGINE_HPP
#define MANA_ANIM_PARTICLEENGINE_HPP

#include <osg/Version>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <functional>
#include <map>

namespace Effekseer
{
    class Manager;
    class Effect;
}

namespace osgVerse
{

    class ParticleSystemU3D;
    class ParticleCloud : public osg::Object
    {
    public:
        ParticleCloud();
        ParticleCloud(const ParticleCloud& pc, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
        META_Object(osgVerse, ParticleCloud)

        typedef std::function<bool(ParticleCloud&, unsigned int, std::map<std::string, std::string>&)> Getter;
        typedef std::function<void(ParticleSystemU3D&, ParticleCloud&)> Injector;

        /** Injector is called by ParticleSystemU3D::recreate() to prepare data for assigning to particle system */
        void setInjector(Injector injector) { _injector = injector; }
        Injector getInjector() { return _injector; }

        bool loadFromCsv(std::istream& in, Getter getter, char sep = ',');
        bool load(std::istream& in);
        bool save(std::ostream& out);

        void add(const osg::Vec3& p, const osg::Vec4& c, const osg::Vec3& v = osg::Vec3(),
                 const osg::Vec4& attr = osg::Vec4(), float size = 1.0f);
        void insert(unsigned int index, const osg::Vec3& p, const osg::Vec4& c,
                    const osg::Vec3& v = osg::Vec3(), const osg::Vec4& attr = osg::Vec4(), float size = 1.0f);
        void clear();

        /** Save copies of current positions, velocities and colors manually */
        void backup();

        /** Retrieve copies of current positions, velocities and colors back manually */
        void retrieve();

        const osg::Vec4Array* getData(int id) const;
        osg::Vec4Array* getData(int id);
        osg::Vec4Array* getBackupData(int id);

        osg::Vec4Array* getPositions() { return _positions.get(); }
        osg::Vec4Array* getVelocities() { return _velocities.get(); }
        osg::Vec4Array* getColors() { return _colors.get(); }
        osg::Vec4Array* getAttributes() { return _attributes.get(); }
        unsigned int size() const { return _positions->size(); }

    protected:
        osg::ref_ptr<osg::Vec4Array> _positions, _velocities;
        osg::ref_ptr<osg::Vec4Array> _colors, _attributes;
        osg::ref_ptr<osg::Vec4Array> _positions0, _velocities0, _colors0;
        Injector _injector;
    };

    class ParticleSystemU3D : public osg::NodeCallback
    {
    public:
        enum UpdateMethod { CPU_TEXTURE_LUT, CPU_VERTEX_ATTRIB, GPU_GEOMETRY };
        ParticleSystemU3D(UpdateMethod up = CPU_VERTEX_ATTRIB);
        ParticleSystemU3D(const ParticleSystemU3D& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        void setPointCloud(ParticleCloud* cloud, bool immutable)
        {
            _pointCloud = cloud; _immutable = immutable;
            _maxParticles = (double)cloud->size(); _dirty = true;
        }

        void play() { _started = true; if (_geometry.valid()) _geometry->dirtyBound(); }
        void stop() { _started = false; }
        bool isPlaying() const { return _started; }
        bool isImmutable() const { return _immutable; }
        UpdateMethod getUpdateMethod() const { return _updateMethod; }

        // Link this particle system to a geode to make it work
        void linkTo(osg::Geode* geode, bool applyStates, osg::Shader* vert = NULL,
                    osg::Shader* frag = NULL, osg::Shader* geom = NULL);
        void unlinkFrom(osg::Geode* geode);

        // Update parameters in CPU_* mode:
        // - ptr0: pos x, y, z, size; ptr1: color r, g, b, a
        // - ptr2: velocity x, y, z, life; ptr3: euler x, y, z, anim id
        bool updateCPU(double time, unsigned int size, osg::Vec4* ptr0, osg::Vec4* ptr1,
                       osg::Vec4* ptr2, osg::Vec4* ptr3);

        // Function: new_velocity CB(velocity_and_life, pos_and_size, world_to_local)
        typedef std::function<osg::Vec3(const osg::Vec4&, const osg::Vec4&, const osg::Matrix&)> VelocityCallback;
        void setVelocityCallback(VelocityCallback cb) { _velocityCallback = cb; }

        // Function: CB(velocity_and_life, pos_and_size, world_to_local)
        typedef std::function<void(osg::Vec4&, osg::Vec4&, const osg::Matrix&)> DeathCallback;
        void setDeathCallback(DeathCallback cb) { _deathCallback = cb; }

        // Function: CB(velocity_and_life, pos_and_size, color, world_to_local)
        typedef std::function<void(osg::Vec4&, osg::Vec4&, osg::Vec4&, const osg::Matrix&)> BirthCallback;
        void setBirthCallback(BirthCallback cb) { _birthCallback = cb; }

        enum EmissionShape { EMIT_Point, EMIT_Circle, EMIT_Plane, EMIT_Sphere, EMIT_Box, EMIT_Mesh };
        enum EmissionSurface { EMIT_Volume, EMIT_Shell };
        enum ParticleType { PARTICLE_Billboard, PARTICLE_BillboardNoScale, PARTICLE_Line, PARTICLE_Mesh };
        enum BlendingType { BLEND_None, BLEND_Modulate, BLEND_Additive };

        // Basic properties
        void setGeometry(osg::Geometry* g) { _geometry2 = g; _dirty = true; }
        void setTexture(osg::Texture2D* t) { _texture = t; _dirty = true; }
        void setStartDirection(const osg::Vec3& v) { _startDirection = v; }
        void setStartLifeRange(const osg::Vec2& v) { _startLifeRange = v; }
        void setStartSizeRange(const osg::Vec2& v) { _startSizeRange = v; }
        void setStartAttitude0(const osg::Quat& q) { _startAttitudeRange[0] = q; }
        void setStartAttitude1(const osg::Quat& q) { _startAttitudeRange[1] = q; }
        void setStartSpeedRange(const osg::Vec2& v) { _startSpeedRange = v; }
        void setMaxParticles(double v) { _maxParticles = v; }
        void setStartDelay(double v) { _startDelay = v; }
        void setGravityScale(double v) { _gravityScale = v; }
        void setDuration(double v) { _duration = v; }
        void setAspectRatio(double v) { _aspectRatio = v; }
        void setParticleType(ParticleType t) { _particleType = t; _dirty = true; }
        void setBlendingType(BlendingType t) { _blendingType = t; _dirty = true; }

        osg::Geometry* getInternalGeometry() { return _geometry.get(); }
        osg::Geometry* getGeometry() { return _geometry2.get(); }
        osg::Texture2D* getTexture() { return _texture.get(); }
        const osg::Vec3& getStartDirection() const { return _startDirection; }
        const osg::Vec2& getStartLifeRange() const { return _startLifeRange; }
        const osg::Vec2& getStartSizeRange() const { return _startSizeRange; }
        const osg::Quat& getStartAttitude0() { return _startAttitudeRange[0]; }
        const osg::Quat& getStartAttitude1() { return _startAttitudeRange[1]; }
        const osg::Vec2& getStartSpeedRange() const { return _startSpeedRange; }
        double getMaxParticles() const { return _maxParticles; }
        double getStartDelay() const { return _startDelay; }
        double getGravityScale() const { return _gravityScale; }
        double getDuration() const { return _duration; }
        double getAspectRatio() const { return _aspectRatio; }
        ParticleType getParticleType() const { return _particleType; }
        BlendingType getBlendingType() const { return _blendingType; }

        /// Emission properties
        void setEmissionBursts(const std::map<float, osg::Vec4>& m) { _emissionBursts = m; }
        void setEmissionShapeCenter(const osg::Vec3& v) { _emissionShapeCenter = v; }
        void setEmissionShapeEulers(const osg::Vec3& v) { _emissionShapeEulers = v; }
        void setEmissionShapeValues(const osg::Vec4& v) { _emissionShapeValues = v; }
        void setEmissionCount(const osg::Vec2& v) { _emissionCount = v; }
        void setEmissionShape(EmissionShape s) { _emissionShape = s; }
        void setEmissionSurface(EmissionSurface s) { _emissionSurface = s; }
        void setEmissionTarget(osg::Node* node) { _emissionTarget = node; }

        std::map<float, osg::Vec4>& getEmissionBursts() { return _emissionBursts; }
        const osg::Vec3& getEmissionShapeCenter() const { return _emissionShapeCenter; }
        const osg::Vec3& getEmissionShapeEulers() const { return _emissionShapeEulers; }
        const osg::Vec4& getEmissionShapeValues() const { return _emissionShapeValues; }
        const osg::Vec2& getEmissionCount() const { return _emissionCount; }
        EmissionShape getEmissionShape() const { return _emissionShape; }
        EmissionSurface getEmissionSurface() const { return _emissionSurface; }
        osg::Node* getEmissionTarget() { return _emissionTarget.get(); }

        // Collision properties
        void setCollisionPlanes(const std::vector<osg::Plane>& v) { _collisionPlanes = v; }
        void setCollisionValues(const osg::Vec4& v) { _collisionValues = v; }

        std::vector<osg::Plane>& getCollisionPlanes() { return _collisionPlanes; }
        const osg::Vec4& getCollisionValues() const { return _collisionValues; }

        /// Texture sheet animation properties
        void setTextureSheetRange(const osg::Vec4& v) { _textureSheetRange = v; _dirty = true; }
        void setTextureSheetValues(const osg::Vec4& v) { _textureSheetValues = v; }
        void setTextureSheetTiles(const osg::Vec2& v) { _textureSheetTiles = v; _dirty = true; }

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
        virtual ~ParticleSystemU3D();
        virtual void recreate();
        virtual void emitParticle(osg::Vec4& vel, osg::Vec4& pos);
        virtual void changeColor(osg::Vec4& color, float time, float speed);
        virtual void changeSize(osg::Vec4& posSize, float time, float speed);
        virtual void changeVelocity(osg::Vec4& vel, float time);
        virtual void changeEulers(osg::Vec4& euler, float time, float speed);
        virtual osg::Vec3 changeForce(const osg::Vec3& initForce, float delta, float time);

        VelocityCallback _velocityCallback;
        DeathCallback _deathCallback;
        BirthCallback _birthCallback;

        std::map<float, osg::Vec4> _emissionBursts;  // [time]: count, cycles, interval, probability (0-1)
        std::map<float, osg::Vec4> _colorPerTime, _colorPerSpeed;  // [time/speed]: color
        std::map<float, osg::Vec3> _eulersPerTime, _eulersPerSpeed;  // [time/speed]: euler value
        std::map<float, osg::Vec3> _velocityOffsets, _forceOffsets;  // [time]: offset
        std::map<float, float> _scalePerTime, _scalePerSpeed;  // [time/speed]: scale value
        std::vector<osg::Plane> _collisionPlanes;

        osg::observer_ptr<osg::Node> _emissionTarget;
        osg::ref_ptr<ParticleCloud> _pointCloud;
        osg::ref_ptr<osg::Texture2D> _texture;
        osg::ref_ptr<osg::Geometry> _geometry, _geometry2;
        osg::Matrix _localToWorld, _worldToLocal;
        osg::Quat _startAttitudeRange[2];  // Local attitudes based on start direction
        osg::Vec4 _collisionValues;        // dampen, bounce scale, lifetime loss, min kill speed
        osg::Vec4 _textureSheetRange;      // Sheet X0, Y0, W, H
        osg::Vec4 _textureSheetValues;     // playing speed by lifetime, by velocity, by FPS, and cycles
        osg::Vec4 _emissionShapeValues;    // Plane: normal + size (4); Circle/Sphere: radii (2/3), Box: sizes (3)
        osg::Vec3 _emissionShapeCenter, _emissionShapeEulers;
        osg::Vec3 _startDirection;        // Start direction of each particle, or set to 0 to use shape predefines
        osg::Vec2 _textureSheetTiles;     // texture sheet X, Y numbers
        osg::Vec2 _emissionCount;         // count per time, count per distance
        osg::Vec2 _startLifeRange, _startSizeRange, _startSpeedRange;
        double _maxParticles, _startDelay, _gravityScale;
        double _startTime, _lastSimulationTime, _duration, _aspectRatio;
        EmissionShape _emissionShape;
        EmissionSurface _emissionSurface;
        ParticleType _particleType;
        BlendingType _blendingType;
        UpdateMethod _updateMethod;
        bool _dirty, _started, _immutable;
    };

    class ParticleDrawableEffekseer : public osg::Drawable
    {
    public:
        ParticleDrawableEffekseer(int maxInstances = 8000);
        ParticleDrawableEffekseer(
            const ParticleDrawableEffekseer& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
        virtual const char* libraryName() const { return "osgVerse"; }
        virtual const char* className() const { return "ParticleDrawableEffekseer"; }

        virtual Object* cloneType() const { return new ParticleDrawableEffekseer; }
        virtual Object* clone(const osg::CopyOp& copyop) const
        { return new ParticleDrawableEffekseer(*this, copyop); }
        virtual bool isSameKindAs(const osg::Object* obj) const
        { return dynamic_cast<const ParticleDrawableEffekseer*>(obj) != NULL; }

        enum PlayingState
        {
            INVALID = -1, STOPPED = 0, PLAYING = 1, PAUSED = 2
        };

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
        virtual ~ParticleDrawableEffekseer();
        osg::ref_ptr<osg::Referenced> _data;
    };

}

#endif
