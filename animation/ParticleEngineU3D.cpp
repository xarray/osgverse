#include <osgDB/FileNameUtils>
#include "pipeline/Global.h"
#include "pipeline/Utilities.h"
#include "ParticleEngine.h"

#define RAND_VALUE(m, n) ((n - m) * (float)rand() / (float)RAND_MAX + m)
#define RAND_RANGE(vec) ((vec[1] - vec[0]) * (float)rand() / (float)RAND_MAX + vec[0])
using namespace osgVerse;

class ParticleTexturePoolU3D : public osg::Referenced
{
public:
    static ParticleTexturePoolU3D* instance()
    {
        static osg::ref_ptr<ParticleTexturePoolU3D> s_instance = new ParticleTexturePoolU3D;
        return s_instance.get();
    }

    void apply(const osg::FrameStamp* fs, ParticleSystemU3D* obj)
    {
        unsigned int index = 0, maxSize = _maxTextureSize * _maxTextureSize / 2;
        if (fs->getFrameNumber() != _frameNumber)
        {
            // New frame arrived, update range uniforms
            for (std::map<ParticleSystemU3D*, osg::ref_ptr<osg::Uniform>>::iterator
                 it = _rangeMap.begin(); it != _rangeMap.end(); ++it)
            {
                unsigned int size = (unsigned int)it->first->getMaxParticles();
                bool bb = (it->first->getParticleType() == ParticleSystemU3D::PARTICLE_Billboard);
                it->second->set(osg::Vec3(index, index + size, bb ? 1.0f : 0.0f)); index += size;
                if (maxSize < index + size)
                    OSG_NOTICE << "[ParticleSystemU3D] Total particle size exceeds maximum number "
                               << maxSize << ", some particles may fail to render." << std::endl;
            }

            // Dirty parameter textures which are updated in the last frame
            if (_positionSizeAndColor.valid() && _positionSizeAndColor->getImage())
                _positionSizeAndColor->getImage()->dirty();
            if (_velocityAndEuler.valid() && _velocityAndEuler->getImage())
                _velocityAndEuler->getImage()->dirty();
            _frameNumber = fs->getFrameNumber();
        }

        std::map<ParticleSystemU3D*, osg::ref_ptr<osg::Uniform>>::iterator it = _rangeMap.find(obj);
        if (it != _rangeMap.end())
        {
            osg::Vec3 range; it->second->get(range);
            unsigned int r0 = range[0], r1 = (unsigned int)range[1];
            if (r1 < r0 || maxSize < r1) return;

            // If in CPU_ONLY mode, update every particle system's parameters
            if (_positionSizeAndColor.valid() && _positionSizeAndColor->getImage() &&
                _velocityAndEuler.valid() && _velocityAndEuler->getImage())
            {
                osg::Vec4* ptr0 = (osg::Vec4*)_positionSizeAndColor->getImage()->data();
                osg::Vec4* ptr1 = (osg::Vec4*)_velocityAndEuler->getImage()->data();
                obj->updateCPU(fs->getSimulationTime(), r1 - r0, ptr0 + r0, ptr0 + maxSize + r0,
                               ptr1 + r0, ptr1 + maxSize + r0);
            }
        }
    }

    osg::Uniform* reallocate(ParticleSystemU3D* obj, size_t size)
    {
        if (_rangeMap.find(obj) == _rangeMap.end())
            _rangeMap[obj] = new osg::Uniform("DataRange", osg::Vec3());
        return _rangeMap[obj].get();
    }

    void deallocate(ParticleSystemU3D* obj)
    {
        if (!obj || !this) return;
        if (_rangeMap.find(obj) != _rangeMap.end()) _rangeMap.erase(_rangeMap.find(obj));
    }

    osg::Texture2D* createParameterTexture(int index, ParticleSystemU3D::UpdateMethod method)
    {
        osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::MIRROR);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::MIRROR);
        switch (method)
        {
        case ParticleSystemU3D::FRAME_RT:
            tex->setTextureSize(_maxTextureSize, _maxTextureSize);
            tex->setInternalFormat(GL_RGBA32F_ARB);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_FLOAT); break;
        default:
            {
                osg::ref_ptr<osg::Image> image = new osg::Image;
                image->allocateImage(_maxTextureSize, _maxTextureSize, 1, GL_RGBA, GL_FLOAT);
                image->setInternalTextureFormat(GL_RGBA32F_ARB);
                memset(image->data(), 0, image->getTotalSizeInBytes());
                tex->setImage(image.get());
            }
            break;
        }

        if (index == 1) _velocityAndEuler = tex;
        else _positionSizeAndColor = tex;
        return tex.get();
    }

    osg::Texture2D* getParameterTexture(int index)
    {
        if (index == 1) return _velocityAndEuler.get();
        else return _positionSizeAndColor.get();
    }

protected:
    ParticleTexturePoolU3D()
    {
        srand(osg::Timer::instance()->tick());
        _maxTextureSize = 2048; _frameNumber = (unsigned int)-1;
        createParameterTexture(0, ParticleSystemU3D::CPU_ONLY);
        createParameterTexture(1, ParticleSystemU3D::CPU_ONLY);  // FIXME
    }

    std::map<ParticleSystemU3D*, osg::ref_ptr<osg::Uniform>> _rangeMap;
    osg::ref_ptr<osg::Texture2D> _positionSizeAndColor;  // [Half0] pos x, y, z, size
                                                         // [Half1] color r, g, b, a
    osg::ref_ptr<osg::Texture2D> _velocityAndEuler;      // [Half0] velocity x, y, z, life
                                                         // [Half1] euler x, y, z, anim id
    unsigned int _maxTextureSize, _frameNumber;
};

ParticleSystemU3D::ParticleSystemU3D()
:   _collisionValues(0.0f, 1.0f, 0.0f, 0.0f), _emissionShapeValues(1.0f, 1.0f, 1.0f),
    _textureSheetTiles(1.0f, 1.0f), _emissionCount(100.0f, 0.0f), _startLifeRange(1.0f, 5.0f),
    _startSizeRange(0.1f, 1.0f), _startSpeedRange(0.1f, 1.0f), _maxParticles(1000.0),
    _startDelay(0.0), _gravityScale(1.0), _emissionShape(EMIT_Point),
    _emissionSurface(EMIT_Volume), _particleType(PARTICLE_Billboard), _lastSimulationTime(0.0), _dirty(true)
{
}

ParticleSystemU3D::ParticleSystemU3D(const ParticleSystemU3D& copy, const osg::CopyOp& copyop)
:   osg::NodeCallback(copy, copyop), _emissionBursts(copy._emissionBursts),
    _colorPerTime(copy._colorPerTime), _colorPerSpeed(copy._colorPerSpeed),
    _eulersPerTime(copy._eulersPerTime), _eulersPerSpeed(copy._eulersPerSpeed),
    _velocityOffsets(copy._velocityOffsets), _forceOffsets(copy._forceOffsets),
    _scalePerTime(copy._scalePerTime), _scalePerSpeed(copy._scalePerSpeed),
    _collisionPlanes(copy._collisionPlanes), _texture(copy._texture), _geometry(copy._geometry),
    _collisionValues(copy._collisionValues), _textureSheetRange(copy._textureSheetRange),
    _textureSheetValues(copy._textureSheetValues), _emissionShapeCenter(copy._emissionShapeCenter),
    _emissionShapeEulers(copy._emissionShapeEulers), _emissionShapeValues(copy._emissionShapeValues),
    _textureSheetTiles(copy._textureSheetTiles), _emissionCount(copy._emissionCount),
    _startLifeRange(copy._startLifeRange), _startSizeRange(copy._startSizeRange),
    _startSpeedRange(copy._startSpeedRange), _maxParticles(copy._maxParticles),
    _startDelay(copy._startDelay), _gravityScale(copy._gravityScale), _emissionShape(copy._emissionShape),
    _emissionSurface(copy._emissionSurface), _particleType(copy._particleType),
    _lastSimulationTime(copy._lastSimulationTime), _dirty(copy._dirty) {}

ParticleSystemU3D::~ParticleSystemU3D()
{ ParticleTexturePoolU3D::instance()->deallocate(this); }

void ParticleSystemU3D::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if (_dirty)
    {
        if (node->asGeode() && _geometry.valid())
            node->asGeode()->removeDrawable(_geometry.get());
        recreate(); _dirty = false;
        if (node->asGeode())
            node->asGeode()->addDrawable(_geometry.get());
    }

    if (nv && nv->getFrameStamp())
    ParticleTexturePoolU3D::instance()->apply(nv->getFrameStamp(), this);
    traverse(node, nv);
}

void ParticleSystemU3D::linkTo(osg::Geode* geode)
{
    if (!_geometry) recreate();
    if (!geode->containsDrawable(_geometry.get()))
        geode->addDrawable(_geometry.get());
    geode->addUpdateCallback(this);
}

void ParticleSystemU3D::updateCPU(double time, unsigned int size, osg::Vec4* ptr0, osg::Vec4* ptr1,
                                  osg::Vec4* ptr2, osg::Vec4* ptr3)
{
    double dt = time - _lastSimulationTime;
    int numToAdd = osg::maximum((int)(_emissionCount[0] * dt), (_emissionCount[0] > 0.0f) ? 1 : 0);

    // Remove and create particles
    for (unsigned int i = 0; i < size; ++i)
    {
        osg::Vec4& posSize = *(ptr0 + i); osg::Vec4& color = *(ptr1 + i);
        osg::Vec4& velLife = *(ptr2 + i); osg::Vec4& eulerAnim = *(ptr3 + i);

        // TODO: many attributes to set...
        if (velLife.a() == 0.0f && numToAdd > 0)
        {
            osg::Vec3 dir = osg::X_AXIS * osg::Matrix::rotate(RAND_VALUE(0.0f, osg::PI_2), osg::Y_AXIS)
                                        * osg::Matrix::rotate(RAND_VALUE(0.0f, osg::PI * 2.0f), osg::Z_AXIS);
            dir = dir * RAND_RANGE(_startSpeedRange) + osg::Vec3(0.0f, 0.0f, -9.8f) * _gravityScale;
            velLife.set(dir[0], dir[1], dir[2], RAND_RANGE(_startLifeRange));
            posSize.set(_emissionShapeCenter[0], _emissionShapeCenter[1], _emissionShapeCenter[2],
                        RAND_RANGE(_startSizeRange));
            color.set(1.0f, 1.0f, 1.0f, 1.0f);
            numToAdd--;
        }
        else
        {
            velLife.a() -= (float)dt;
            if (velLife.a() > 0.0f)
            {
                osg::Vec3 vel(velLife[0] * dt, velLife[1] * dt, velLife[2] * dt);
                for (int k = 0; k < 3; ++k) posSize[k] += vel[k];
            }
            else velLife.a() = 0.0f;
        }
    }
    _lastSimulationTime = time;
}

void ParticleSystemU3D::recreate()
{
    _geometry = (_particleType != PARTICLE_Mesh) ? new osg::Geometry
              : static_cast<osg::Geometry*>(_geometry2->clone(osg::CopyOp::DEEP_COPY_ALL));
    _geometry->setName("ParticleGeometry");
    _geometry->setUseDisplayList(false);
    _geometry->setUseVertexBufferObjects(true);
    _geometry->setComputeBoundingBoxCallback(new osgVerse::DisableBoundingBoxCallback);
    switch (_particleType)
    {
    case PARTICLE_Billboard:
        {
            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(4);
            osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(4);
            osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(4);
            (*va)[0].set(-0.005f, -0.005f, 0.0f); (*ta)[0].set(0.0f, 0.0f);
            (*va)[1].set(0.005f, -0.005f, 0.0f); (*ta)[1].set(1.0f, 0.0f);
            (*va)[2].set(0.005f, 0.005f, 0.0f); (*ta)[2].set(1.0f, 1.0f);
            (*va)[3].set(-0.005f, 0.005f, 0.0f); (*ta)[3].set(0.0f, 1.0f);
            for (int i = 0; i < 4; ++i) (*na)[i] = osg::Z_AXIS;
            _geometry->setVertexArray(va.get()); _geometry->setNormalArray(na.get());
            _geometry->setTexCoordArray(0, ta.get());
            _geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

            osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
            de->push_back(0); de->push_back(1); de->push_back(2);
            de->push_back(0); de->push_back(2); de->push_back(3);
            _geometry->addPrimitiveSet(de.get());
        }
        break;
    case PARTICLE_Line:
        break;  // TODO
    default:
        break;
    }

    ParticleTexturePoolU3D* pool = ParticleTexturePoolU3D::instance();
    for (size_t i = 0; i < _geometry->getNumPrimitiveSets(); ++i)
    {
        osg::PrimitiveSet* p = _geometry->getPrimitiveSet(i);
        p->setNumInstances(_maxParticles); p->dirty();
    }

    osg::StateSet* ss = _geometry->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0, _texture.get());
    ss->setTextureAttributeAndModes(1, pool->getParameterTexture(0));
    ss->setTextureAttributeAndModes(2, pool->getParameterTexture(1));
    ss->addUniform(new osg::Uniform("BaseTexture", (int)0));
    ss->addUniform(new osg::Uniform("PosColorTexture", (int)1));
    ss->addUniform(new osg::Uniform("VelocityTexture", (int)2));
    ss->addUniform(pool->reallocate(this, _maxParticles));
}
