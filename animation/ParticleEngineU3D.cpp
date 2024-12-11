#include <osgDB/FileNameUtils>
#include "pipeline/Global.h"
#include "ParticleEngine.h"
using namespace osgVerse;

class ParticleTexturePoolU3D : public osg::Referenced
{
public:
    static ParticleTexturePoolU3D* instance()
    {
        static osg::ref_ptr<ParticleTexturePoolU3D> s_instance = new ParticleTexturePoolU3D;
        return s_instance.get();
    }

    osg::Texture2D* createParameterTexture(
            int index, ParticleSystemU3D::UpdateMethod method, int maxSize = 4096)
    {
        osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::MIRROR);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::MIRROR);
        switch (method)
        {
        case ParticleSystemU3D::FRAME_RT:
            tex->setTextureSize(maxSize, maxSize);
            tex->setInternalFormat(GL_RGBA32F_ARB);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_FLOAT); break;
        default:
            {
                osg::ref_ptr<osg::Image> image = new osg::Image;
                image->allocateImage(maxSize, maxSize, 1, GL_RGBA, GL_FLOAT);
                image->setInternalTextureFormat(GL_RGBA32F_ARB);
                tex->setImage(image.get());
            }
            break;
        }

        if (index == 1) _velocityAndAngular = tex;
        else _positionSizeAndColor = tex;
        return tex.get();
    }

    osg::Texture2D* getParameterTexture(int index)
    {
        if (index == 1) return _velocityAndAngular.get();
        else return _positionSizeAndColor.get();
    }

protected:
    ParticleTexturePoolU3D() {}
    osg::ref_ptr<osg::Texture2D> _positionSizeAndColor;
    osg::ref_ptr<osg::Texture2D> _velocityAndAngular;
};

ParticleSystemU3D::ParticleSystemU3D(UpdateMethod method)
:   _collisionValues(0.0f, 1.0f, 0.0f, 0.0f), _emissionShapeValues(1.0f, 1.0f, 1.0f),
    _textureSheetTiles(1.0f, 1.0f), _emissionCount(100.0f, 0.0f), _startLifeRange(1.0f, 5.0f),
    _startSizeRange(0.1f, 1.0f), _startSpeedRange(0.1f, 1.0f), _maxParticles(10000.0),
    _startDelay(0.0), _gravityScale(1.0), _emissionShape(EMIT_Point),
    _emissionSurface(EMIT_Volume), _particleType(PARTICLE_Billboard), _dirty(true)
{
    ParticleTexturePoolU3D* pool = ParticleTexturePoolU3D::instance();
    pool->createParameterTexture(0, method);
    pool->createParameterTexture(1, method);
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
    _emissionSurface(copy._emissionSurface), _particleType(copy._particleType), _dirty(copy._dirty) {}

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
    traverse(node, nv);
}

void ParticleSystemU3D::linkTo(osg::Geode* geode)
{
    if (!_geometry) recreate();
    geode->addDrawable(_geometry.get());
    geode->addUpdateCallback(this);
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
    case PARTICLE_Billboard:
        {
            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(4);
            osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(4);
            osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(4);
            (*va)[0].set(-0.5f, -0.5f, 0.0f); (*ta)[0].set(0.0f, 0.0f);
            (*va)[1].set(0.5f, -0.5f, 0.0f); (*ta)[1].set(1.0f, 0.0f);
            (*va)[2].set(0.5f, 0.5f, 0.0f); (*ta)[2].set(1.0f, 1.0f);
            (*va)[3].set(-0.5f, 0.5f, 0.0f); (*ta)[3].set(0.0f, 1.0f);
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

    for (size_t i = 0; i < _geometry->getNumPrimitiveSets(); ++i)
    {
        osg::PrimitiveSet* p = _geometry->getPrimitiveSet(i);
        p->setNumInstances(_maxParticles); p->dirty();
    }

    osg::StateSet* ss = _geometry->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0, _texture.get());
    ss->setTextureAttributeAndModes(
        1, ParticleTexturePoolU3D::instance()->getParameterTexture(0));
    ss->setTextureAttributeAndModes(
        2, ParticleTexturePoolU3D::instance()->getParameterTexture(1));
    ss->addUniform(new osg::Uniform("BaseTexture", (int)0));
    ss->addUniform(new osg::Uniform("PosColorTexture", (int)1));
    ss->addUniform(new osg::Uniform("VelocityTexture", (int)2));
}
