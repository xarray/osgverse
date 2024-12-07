#include <osgDB/FileNameUtils>
#include "pipeline/Global.h"
#include "ParticleEngine.h"
using namespace osgVerse;

ParticleSystemU3D::ParticleSystemU3D()
:   _collisionValues(0.0f, 1.0f, 0.0f, 0.0f), _emissionShapeValues(1.0f, 1.0f, 1.0f),
    _textureSheetTiles(1.0f, 1.0f), _emissionCount(100.0f, 0.0f), _startLifeRange(1.0f, 5.0f),
    _startSizeRange(0.1f, 1.0f), _startSpeedRange(0.1f, 1.0f), _maxParticles(10000.0),
    _startDelay(0.0), _gravityScale(1.0), _emissionShape(EMIT_Point),
    _emissionSurface(EMIT_Volume), _particleType(PARTICLE_Billboard)
{
}

ParticleSystemU3D::ParticleSystemU3D(const ParticleSystemU3D& copy, const osg::CopyOp& copyop)
:   osg::NodeCallback(copy, copyop), _emissionBursts(copy._emissionBursts),
    _colorPerTime(copy._colorPerTime), _colorPerSpeed(copy._colorPerSpeed),
    _scalePerTime(copy._scalePerTime), _scalePerSpeed(copy._scalePerSpeed),
    _eulersPerTime(copy._eulersPerTime), _eulersPerSpeed(copy._eulersPerSpeed),
    _velocityOffsets(copy._velocityOffsets), _forceOffsets(copy._forceOffsets),
    _collisionPlanes(copy._collisionPlanes), _texture(copy._texture), _geometry(copy._geometry),
    _collisionValues(copy._collisionValues), _textureSheetRange(copy._textureSheetRange),
    _textureSheetValues(copy._textureSheetValues), _emissionShapeCenter(copy._emissionShapeCenter),
    _emissionShapeEulers(copy._emissionShapeEulers), _emissionShapeValues(copy._emissionShapeValues),
    _textureSheetTiles(copy._textureSheetTiles), _emissionCount(copy._emissionCount),
    _startLifeRange(copy._startLifeRange), _startSizeRange(copy._startSizeRange),
    _startSpeedRange(copy._startSpeedRange), _maxParticles(copy._maxParticles),
    _startDelay(copy._startDelay), _gravityScale(copy._gravityScale), _emissionShape(copy._emissionShape),
    _emissionSurface(copy._emissionSurface), _particleType(copy._particleType) {}

void ParticleSystemU3D::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    traverse(node, nv);
}
