#include <osg/io_utils>
#include <osg/Depth>
#include <osg/BlendFunc>
#include <osg/VertexAttribDivisor>
#include <osgDB/FileNameUtils>
#include <iostream>
#include "modeling/Math.h"
#include "pipeline/Global.h"
#include "pipeline/Utilities.h"
#include "pipeline/Pipeline.h"
#include "ParticleEngine.h"

#define RAND_VALUE(m, n) ((n - m) * (float)rand() / (float)RAND_MAX + m)
#define RAND_RANGE1(val) ((val) * (float)rand() / (float)RAND_MAX - val * 0.5)
#define RAND_RANGE2(vec) ((vec[1] - vec[0]) * (float)rand() / (float)RAND_MAX + vec[0])
using namespace osgVerse;

class ParticleSystemPoolU3D : public osg::Referenced
{
public:
    static ParticleSystemPoolU3D* instance()
    {
        static osg::ref_ptr<ParticleSystemPoolU3D> s_instance = new ParticleSystemPoolU3D;
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
                unsigned int size = (unsigned int)ceil(it->first->getMaxParticles()),
                             bbType = (int)it->first->getParticleType();
                float bbValue = 0.0f, aspect = (float)it->first->getAspectRatio();
                if (bbType == ParticleSystemU3D::PARTICLE_BillboardNoScale) bbValue = 2.0f;
                else if (bbType == ParticleSystemU3D::PARTICLE_Billboard) bbValue = 1.0f;

                if (maxSize < index + size)
                {
                    OSG_NOTICE << "[ParticleSystemU3D] Total particle size exceeds maximum number "
                               << maxSize << ", some particles may fail to render." << std::endl;
                    it->second->set(osg::Vec4(index, maxSize, bbValue, aspect)); break;
                }
                it->second->set(osg::Vec4(index, index + size, bbValue, aspect)); index += size;
            }

            // Dirty parameter textures which are updated in the last frame
            if (_positionSizeAndColor.valid() && _positionSizeAndColor->getImage())
            { if (_frameNumber % 2) _positionSizeAndColor->getImage()->dirty(); }
            if (_velocityAndEuler.valid() && _velocityAndEuler->getImage())
            { if (!(_frameNumber % 2)) _velocityAndEuler->getImage()->dirty(); }
            _frameNumber = fs->getFrameNumber();
        }

        std::map<ParticleSystemU3D*, osg::ref_ptr<osg::Uniform>>::iterator it = _rangeMap.find(obj);
        if (it != _rangeMap.end())
        {
            osg::Vec4 range; it->second->get(range);
            unsigned int r0 = range[0], r1 = (unsigned int)range[1];
            if (r1 < r0 || maxSize < r1) return;

            // If in CPU_* mode, update every particle system's parameters
            osg::Geometry* g = obj->getInternalGeometry();
            ParticleSystemU3D::UpdateMethod method = obj->getUpdateMethod();
            if (method == ParticleSystemU3D::CPU_TEXTURE_LUT &&
                _positionSizeAndColor.valid() && _positionSizeAndColor->getImage() &&
                _velocityAndEuler.valid() && _velocityAndEuler->getImage())
            {
                osg::Vec4* ptr0 = (osg::Vec4*)_positionSizeAndColor->getImage()->data();
                osg::Vec4* ptr1 = (osg::Vec4*)_velocityAndEuler->getImage()->data();
                obj->updateCPU(fs->getSimulationTime(), r1 - r0, ptr0 + r0, ptr0 + maxSize + r0,
                               ptr1 + r0, ptr1 + maxSize + r0);
            }
            else if (method == ParticleSystemU3D::CPU_VERTEX_ATTRIB && g != NULL)
            {
                osg::Vec4Array* pData = static_cast<osg::Vec4Array*>(g->getVertexAttribArray(4));
                osg::Vec4Array* cData = static_cast<osg::Vec4Array*>(g->getVertexAttribArray(5));
                osg::Vec4Array* vData = static_cast<osg::Vec4Array*>(g->getVertexAttribArray(6));
                osg::Vec4Array* eData = static_cast<osg::Vec4Array*>(g->getVertexAttribArray(7));
                obj->updateCPU(fs->getSimulationTime(), r1 - r0, &(*pData)[0], &(*cData)[0],
                               &(*vData)[0], &(*eData)[0]);
                pData->dirty(); cData->dirty(); vData->dirty(); eData->dirty();
            }
            else if (method == ParticleSystemU3D::GPU_GEOMETRY && g != NULL)
            {
                osg::Vec4Array* pData = static_cast<osg::Vec4Array*>(g->getVertexArray());
                osg::Vec4Array* cData = static_cast<osg::Vec4Array*>(g->getColorArray());
                osg::Vec4Array* vData = static_cast<osg::Vec4Array*>(g->getTexCoordArray(0));
                osg::Vec4Array* eData = static_cast<osg::Vec4Array*>(g->getTexCoordArray(1));
                obj->updateCPU(fs->getSimulationTime(), r1 - r0, &(*pData)[0], &(*cData)[0],
                               &(*vData)[0], &(*eData)[0]);
                pData->dirty(); cData->dirty(); vData->dirty(); eData->dirty();
            }
        }
    }

    osg::Uniform* reallocate(ParticleSystemU3D* obj, size_t size)
    {
        if (_rangeMap.find(obj) == _rangeMap.end())
            _rangeMap[obj] = new osg::Uniform("DataRange", osg::Vec4());
        return _rangeMap[obj].get();
    }

    void deallocate(ParticleSystemU3D* obj)
    {
        if (_rangeMap.find(obj) != _rangeMap.end())
            _rangeMap.erase(_rangeMap.find(obj));
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
        case ParticleSystemU3D::GPU_GEOMETRY:
        case ParticleSystemU3D::CPU_VERTEX_ATTRIB:
            return NULL;
        default:
            {
                osg::ref_ptr<osg::Image> image = new osg::Image;
                image->setDataVariance(osg::Object::DYNAMIC);
                image->allocateImage(_maxTextureSize, _maxTextureSize, 1, GL_RGBA, GL_FLOAT);
                image->setInternalTextureFormat(GL_RGBA32F_ARB);
                memset(image->data(), 0, image->getTotalSizeInBytes());
                //image->setPixelBufferObject(new osg::PixelBufferObject(image.get()));
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
    ParticleSystemPoolU3D()
    {
        srand(osg::Timer::instance()->tick());
        _maxTextureSize = 2048; _frameNumber = (unsigned int)-1;
    }

    std::map<ParticleSystemU3D*, osg::ref_ptr<osg::Uniform>> _rangeMap;
    osg::ref_ptr<osg::Texture2D> _positionSizeAndColor;  // [Half0] pos x, y, z, size
                                                         // [Half1] color r, g, b, a
    osg::ref_ptr<osg::Texture2D> _velocityAndEuler;      // [Half0] velocity x, y, z, life
                                                         // [Half1] euler x, y, z, anim id
    unsigned int _maxTextureSize, _frameNumber;
};

ParticleSystemU3D::ParticleSystemU3D(UpdateMethod upMode)
:   _collisionValues(0.0f, 1.0f, 0.0f, 0.0f), _emissionShapeValues(1.0f, 1.0f, 1.0f, 1.0f),
    _startDirection(0.0f, 0.0f, 0.0f), _textureSheetTiles(1.0f, 1.0f), _emissionCount(100.0f, 0.0f),
    _startLifeRange(1.0f, 5.0f), _startSizeRange(0.1f, 1.0f), _startSpeedRange(0.1f, 1.0f),
    _maxParticles(1000.0), _startDelay(0.0), _gravityScale(1.0), _startTime(0.0),
    _lastSimulationTime(0.0), _duration(1.0), _aspectRatio(16.0 / 9.0), _emissionShape(EMIT_Point),
    _emissionSurface(EMIT_Volume), _particleType(PARTICLE_Billboard),
    _blendingType(BLEND_Modulate), _updateMethod(upMode), _dirty(true), _started(true)
{
    ParticleSystemPoolU3D::instance()->createParameterTexture(0, upMode);
    ParticleSystemPoolU3D::instance()->createParameterTexture(1, upMode);
}

ParticleSystemU3D::ParticleSystemU3D(const ParticleSystemU3D& copy, const osg::CopyOp& copyop)
:   osg::NodeCallback(copy, copyop), _velocityCallback(copy._velocityCallback),
    _deathCallback(copy._deathCallback), _birthCallback(copy._birthCallback),
    _emissionBursts(copy._emissionBursts), _colorPerTime(copy._colorPerTime), _colorPerSpeed(copy._colorPerSpeed),
    _eulersPerTime(copy._eulersPerTime), _eulersPerSpeed(copy._eulersPerSpeed),
    _velocityOffsets(copy._velocityOffsets), _forceOffsets(copy._forceOffsets),
    _scalePerTime(copy._scalePerTime), _scalePerSpeed(copy._scalePerSpeed),
    _collisionPlanes(copy._collisionPlanes), _emissionTarget(copy._emissionTarget),
    _texture(copy._texture), _geometry(copy._geometry),
    _collisionValues(copy._collisionValues), _textureSheetRange(copy._textureSheetRange),
    _textureSheetValues(copy._textureSheetValues), _emissionShapeValues(copy._emissionShapeValues),
    _emissionShapeCenter(copy._emissionShapeCenter), _emissionShapeEulers(copy._emissionShapeEulers),
    _startDirection(copy._startDirection), _textureSheetTiles(copy._textureSheetTiles),
    _emissionCount(copy._emissionCount), _startLifeRange(copy._startLifeRange),
    _startSizeRange(copy._startSizeRange), _startSpeedRange(copy._startSpeedRange),
    _maxParticles(copy._maxParticles), _startDelay(copy._startDelay), _gravityScale(copy._gravityScale),
    _startTime(copy._startTime), _lastSimulationTime(copy._lastSimulationTime), _duration(copy._duration),
    _aspectRatio(copy._aspectRatio), _emissionShape(copy._emissionShape),
    _emissionSurface(copy._emissionSurface), _particleType(copy._particleType),
    _blendingType(copy._blendingType), _updateMethod(copy._updateMethod),
    _dirty(copy._dirty), _started(copy._started)
{
    _startAttitudeRange[0] = copy._startAttitudeRange[0];
    _startAttitudeRange[1] = copy._startAttitudeRange[1];
}

ParticleSystemU3D::~ParticleSystemU3D()
{ ParticleSystemPoolU3D::instance()->deallocate(this); }

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
    {
        _localToWorld = osg::computeLocalToWorld(nv->getNodePath());
        _worldToLocal = osg::Matrix::inverse(_localToWorld);
        ParticleSystemPoolU3D::instance()->apply(nv->getFrameStamp(), this);
    }
    traverse(node, nv);
}

void ParticleSystemU3D::linkTo(osg::Geode* geode, bool applyStates,
                               osg::Shader* vert, osg::Shader* frag, osg::Shader* geom)
{
    if (!geode) return; else if (!_geometry) recreate();
    if (!geode->containsDrawable(_geometry.get())) geode->addDrawable(_geometry.get());
    geode->addUpdateCallback(this); if (!applyStates) return;

    osg::Program* program = new osg::Program;
    program->setName("Particle_PROGRAM");
    if (_updateMethod == GPU_GEOMETRY)
    {
        if (!geom || (geom && geom->getType() != osg::Shader::GEOMETRY))
            OSG_WARN << "[ParticleSystemU3D] GPU_GEOMETRY mode must have a geometry shader attached!\n";
        if (vert->getShaderSource().find("#define USE_GEOM_SHADER") == std::string::npos)
            vert->setShaderSource("#define USE_GEOM_SHADER 1\n" + vert->getShaderSource());
    }
    else if (_updateMethod == CPU_VERTEX_ATTRIB)
    {
        program->addBindAttribLocation("osg_UserPosition", 4);
        program->addBindAttribLocation("osg_UserColor", 5);
        program->addBindAttribLocation("osg_UserVelocity", 6);
        program->addBindAttribLocation("osg_UserEulers", 7);

        if (vert->getShaderSource().find("#define USE_VERTEX_ATTRIB") == std::string::npos)
            vert->setShaderSource("#define USE_VERTEX_ATTRIB 1\n" + vert->getShaderSource());
    }

    program->addShader(vert); Pipeline::createShaderDefinitions(vert, 100, 130);
    program->addShader(frag); Pipeline::createShaderDefinitions(frag, 100, 130);  // FIXME
    if (_updateMethod == GPU_GEOMETRY && geom)
    {
        program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 6);  // FIXME: only billboard?
        program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
        program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLES);
        program->addShader(geom); Pipeline::createShaderDefinitions(geom, 100, 130);
    }

    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setAttributeAndModes(program);
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
}

void ParticleSystemU3D::unlinkFrom(osg::Geode* geode)
{
    if (!geode || !_geometry) return;
    geode->removeDrawable(_geometry.get());
    geode->removeUpdateCallback(this);
}

bool ParticleSystemU3D::updateCPU(double time, unsigned int size, osg::Vec4* ptr0, osg::Vec4* ptr1,
                                  osg::Vec4* ptr2, osg::Vec4* ptr3)
{
    osg::BoundingBox bounds; double dt = time - _lastSimulationTime;
    int numToAdd = osg::maximum((int)(_emissionCount[0] * dt), (_emissionCount[0] > 0.0f) ? 1 : 0),
        sizeInt = (int)size; if (!_started) numToAdd = 0;

    // Remove and create particles
    bool withVelCB = (_velocityCallback != NULL), withDeathCB = (_deathCallback != NULL);
    bool withBirthCB = (_birthCallback != NULL);
    float maxTexSheet = _textureSheetTiles.x() * _textureSheetTiles.y();
    osg::Vec3 force = (osg::Vec3(0.0f, 0.0f, -9.8f) * _worldToLocal) * _gravityScale;
//#pragma omp parallel for
    for (int i = 0; i < sizeInt; ++i)
    {
        osg::Vec4& posSize = *(ptr0 + i); osg::Vec4& color = *(ptr1 + i);
        osg::Vec4& velLife = *(ptr2 + i); osg::Vec4& eulerAnim = *(ptr3 + i);

        velLife.a() -= (float)dt;
        if (velLife.a() > 0.0f)  // update existing particles
        {
            float tRatio = 1.0f - velLife.a() / _duration;
            osg::Vec3 vel = withVelCB ? _velocityCallback(velLife, posSize, _worldToLocal)
                          : osg::Vec3(velLife[0], velLife[1], velLife[2]);
            vel += changeForce(force, dt, tRatio);
            for (int k = 0; k < 3; ++k) posSize[k] += vel[k];

            float speed = vel.length();
            changeSize(posSize, tRatio, speed); changeColor(color, tRatio, speed);
            changeVelocity(velLife, tRatio); changeEulers(eulerAnim, tRatio, speed);
            eulerAnim.a() += _textureSheetValues[0] * dt;
            if (eulerAnim.a() > maxTexSheet) eulerAnim.a() = 0.0f;
        }
        else
        {
            velLife.a() = 0.0f;
            if (withDeathCB) _deathCallback(velLife, posSize, _worldToLocal);
        }

        if (velLife.a() == 0.0f && numToAdd > 0)  // new particle
        {
            emitParticle(velLife, posSize);
            velLife.a() = RAND_RANGE2(_startLifeRange);
            posSize.a() = RAND_RANGE2(_startSizeRange);
            color.set(1.0f, 1.0f, 1.0f, 1.0f); changeColor(color, 0.0f, 0.0f);

            if (withBirthCB) _birthCallback(velLife, posSize, color, _worldToLocal);
            eulerAnim.a() = 0.0f; numToAdd--;
        }
        bounds.expandBy(osg::Vec3(posSize[0], posSize[1], posSize[2]));
    }

    if (_geometry2.valid()) bounds.expandBy(_geometry2->getBound());
    if (_geometry.valid()) _geometry->setInitialBound(bounds);
    _lastSimulationTime = time; return bounds.valid();
}

void ParticleSystemU3D::recreate()
{
    _geometry = (_particleType != PARTICLE_Mesh) ? new osg::Geometry
              : static_cast<osg::Geometry*>(_geometry2->clone(osg::CopyOp::DEEP_COPY_ALL));
    _geometry->setName("ParticleGeometry");
    _geometry->setUseDisplayList(false);
    _geometry->setUseVertexBufferObjects(true);
    switch (_particleType)
    {
    case PARTICLE_Billboard: case PARTICLE_BillboardNoScale:
        if (_updateMethod == GPU_GEOMETRY)
        {
            osg::ref_ptr<osg::Vec4Array> va = new osg::Vec4Array(_maxParticles);
            osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array(_maxParticles);
            osg::ref_ptr<osg::Vec4Array> ta0 = new osg::Vec4Array(_maxParticles);
            osg::ref_ptr<osg::Vec4Array> ta1 = new osg::Vec4Array(_maxParticles);
            _geometry->setVertexArray(va.get()); _geometry->setColorArray(ca.get());
            _geometry->setTexCoordArray(0, ta0.get()); _geometry->setTexCoordArray(1, ta1.get());
            _geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            
            osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_POINTS);
            for (size_t i = 0; i < va->size(); ++i) de->push_back(i);
            _geometry->addPrimitiveSet(de.get());
        }
        else
        {
            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(4);
            osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(4);
            osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(4);
            (*va)[0].set(-0.05f, -0.05f, 0.0f); (*ta)[0].set(0.0f, 0.0f);
            (*va)[1].set(0.05f, -0.05f, 0.0f); (*ta)[1].set(1.0f, 0.0f);
            (*va)[2].set(0.05f, 0.05f, 0.0f); (*ta)[2].set(1.0f, 1.0f);
            (*va)[3].set(-0.05f, 0.05f, 0.0f); (*ta)[3].set(0.0f, 1.0f);
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

    osg::StateSet* ss = _geometry->getOrCreateStateSet();
    if (_updateMethod == CPU_VERTEX_ATTRIB)
    {
        for (int k = 4; k < 8; ++k)
        {
            osg::ref_ptr<osg::Vec4Array> pData = new osg::Vec4Array(_maxParticles);
            _geometry->setVertexAttribArray(k, pData.get());
            _geometry->setVertexAttribBinding(k, osg::Geometry::BIND_PER_VERTEX);
            _geometry->setVertexAttribNormalize(k, GL_FALSE);
            ss->setAttributeAndModes(new osg::VertexAttribDivisor(k, 1));
        }
    }

    ParticleSystemPoolU3D* pool = ParticleSystemPoolU3D::instance();
    if (_updateMethod != GPU_GEOMETRY)
    {
        for (size_t i = 0; i < _geometry->getNumPrimitiveSets(); ++i)
        {
            osg::PrimitiveSet* p = _geometry->getPrimitiveSet(i);
            p->setNumInstances(_maxParticles); p->dirty();
        }
    }

    ss->setTextureAttributeAndModes(0, _texture.get());
    if (pool->getParameterTexture(0))
    {
        ss->setTextureAttributeAndModes(1, pool->getParameterTexture(0));
        ss->addUniform(new osg::Uniform("PosColorTexture", (int)1));
    }
    if (pool->getParameterTexture(1))
    {
        ss->setTextureAttributeAndModes(2, pool->getParameterTexture(1));
        ss->addUniform(new osg::Uniform("VelocityTexture", (int)2));
    }
    ss->addUniform(new osg::Uniform("BaseTexture", (int)0));
    ss->addUniform(new osg::Uniform("TextureSheetRange", _textureSheetRange));
    ss->addUniform(new osg::Uniform("TextureSheetTiles", _textureSheetTiles));
    ss->addUniform(pool->reallocate(this, _maxParticles));

    switch (_blendingType)
    {
    case BLEND_Modulate:
        ss->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN); break;
    case BLEND_Additive:
        ss->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE));
        ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN); break;
    default:
        ss->setMode(GL_BLEND, osg::StateAttribute::OFF);
        ss->setRenderingHint(osg::StateSet::OPAQUE_BIN); break;
    }
}

void ParticleSystemU3D::emitParticle(osg::Vec4& vel, osg::Vec4& pos)
{
    osg::Matrix matrix; osg::Vec3 pos3, vel3 = _startDirection;
    if (_emissionTarget.valid())
    {
        osg::MatrixList mList = _emissionTarget->getWorldMatrices();
        if (!mList.empty()) matrix = mList[0] * _worldToLocal;
    }

    // TODO: emission eulers
    bool useDefVelocity = osg::equivalent(vel3.length2(), 0.0f);
    pos3.set(_emissionShapeCenter[0], _emissionShapeCenter[1], _emissionShapeCenter[2]);
    switch (_emissionShape)
    {
    case EMIT_Box:
        if (useDefVelocity) vel3 = osg::Z_AXIS;
        {
            osg::Vec3 pt(RAND_RANGE1(_emissionShapeValues[0]),
                         RAND_RANGE1(_emissionShapeValues[1]),
                         RAND_RANGE1(_emissionShapeValues[2]));
            if (_emissionSurface != EMIT_Volume)
            {
                // TODO
            }
            pos3 += pt;
        }
        break;
    case EMIT_Circle: case EMIT_Sphere:
        {
            osg::Vec3 pt; float emissionValue2 = (_emissionShape == EMIT_Circle)
                                               ? 0.0f : _emissionShapeValues[2];
            do {
                pt = (_emissionShape == EMIT_Circle) ? osg::Vec3(RAND_RANGE1(2.0f), RAND_RANGE1(2.0f), 0.0f)
                   : osg::Vec3(RAND_RANGE1(2.0f), RAND_RANGE1(2.0f), RAND_RANGE1(2.0f));
            } while (pt.length2() > 1.0f);
            if (_emissionSurface != EMIT_Volume) pt.normalize();
            pos3 += osg::Vec3(pt[0] * _emissionShapeValues[0],
                              pt[1] * _emissionShapeValues[1], pt[2] * emissionValue2);
            if (useDefVelocity) vel3 = pt; if (vel3.length2() == 0.f) vel3 = osg::Z_AXIS;
        }
        break;
    case EMIT_Plane:
        {
            osg::Vec3 pt, N = osg::Vec3(_emissionShapeValues[0],
                                        _emissionShapeValues[1], _emissionShapeValues[2]);
            if (N.length2() < 0.1f) break; else if (useDefVelocity) vel3 = N;
            do
            {
                pt = osg::Vec3(RAND_RANGE1(1.0f), RAND_RANGE1(1.0f), RAND_RANGE1(1.0f));
                pt.normalize(); pt = pt ^ N;
            } while (pt.length2() < 0.1f);
            pos3 += osg::Vec3(pt * RAND_RANGE1(_emissionShapeValues[3]));
        }
        break;
    case EMIT_Mesh:
        // TODO: to vhacd and emit particles
        break;
    default:
        if (useDefVelocity) vel3 = osg::Vec3(RAND_RANGE1(2.0f), RAND_RANGE1(2.0f), RAND_RANGE1(2.0f));
        break;  // EMIT_Point
    }

    vel3.normalize();
    if (!_startAttitudeRange[0].zeroRotation() && !_startAttitudeRange[1].zeroRotation())
    {
        osg::Quat q; q.slerp(RAND_VALUE(0.0, 1.0), _startAttitudeRange[0], _startAttitudeRange[1]);
        vel3 = computeParentRotation(vel3, q) * vel3;
    }
    vel3 = osg::Matrix::transform3x3(osg::Matrix::inverse(matrix), vel3);
    vel3 = vel3 * RAND_RANGE2(_startSpeedRange); vel.set(vel3[0], vel3[1], vel3[2], 0.0f);
    pos = osg::Vec4(pos3 * matrix, 0.0f);
}

template<typename T> void getValueFromMap(std::map<float, T>& dataMap, T& value, float t)
{
    typename std::map<float, T>::iterator it = dataMap.upper_bound(t);
    if (it != dataMap.end())
    {
        float t1 = it->first; T c1 = it->second; it--;
        if (it != dataMap.end())
        {
            float t0 = it->first; T c0 = it->second;
            float r = (t - t0) / (t1 - t0); value = c0 * (1.0f - r) + c1 * r;
        }
        else value = c1;
    }
}

void ParticleSystemU3D::changeColor(osg::Vec4& color, float time, float speed)
{
    if (!_colorPerTime.empty()) getValueFromMap(_colorPerTime, color, time);
    if (!_colorPerSpeed.empty()) getValueFromMap(_colorPerSpeed, color, speed);
}

void ParticleSystemU3D::changeSize(osg::Vec4& posSize, float time, float speed)
{
    float size = posSize[3];
    if (!_scalePerTime.empty()) getValueFromMap(_scalePerTime, size, time);
    if (!_scalePerSpeed.empty()) getValueFromMap(_scalePerSpeed, size, speed);
    posSize[3] = size;
}

void ParticleSystemU3D::changeVelocity(osg::Vec4& vel, float time)
{
    osg::Vec3 vec(vel[0], vel[1], vel[2]);
    if (!_velocityOffsets.empty()) getValueFromMap(_velocityOffsets, vec, time);
    vel.set(vec[0], vec[1], vec[2], vel[3]);
}

void ParticleSystemU3D::changeEulers(osg::Vec4& euler, float time, float speed)
{
    osg::Vec3 vec(euler[0], euler[1], euler[2]);
    if (!_eulersPerTime.empty()) getValueFromMap(_eulersPerTime, vec, time);
    if (!_eulersPerSpeed.empty()) getValueFromMap(_eulersPerSpeed, vec, speed);
    euler.set(vec[0], vec[1], vec[2], euler[3]);
}

osg::Vec3 ParticleSystemU3D::changeForce(const osg::Vec3& initForce, float delta, float time)
{
    osg::Vec3 force(0.0f, 0.0f, 0.0f);
    if (!_forceOffsets.empty()) getValueFromMap(_forceOffsets, force, time);
    return (initForce + force) * delta;
}
