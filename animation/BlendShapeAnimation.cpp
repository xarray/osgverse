#include <osg/io_utils>
#include "BlendShapeAnimation.h"
using namespace osgVerse;

BlendShapeAnimation::BlendShapeAnimation()
{
}

void BlendShapeAnimation::apply(const std::vector<std::string>& names,
                                const std::vector<double>& weights)
{
    size_t num = osg::minimum(names.size(), _blendshapes.size());
    size_t wNum = weights.size();
    for (size_t i = 0; i < num; ++i)
    {
        BlendShapeData* bsd = _blendshapes[i].get();
        if (i < wNum) bsd->weight = weights[i];
        bsd->name = names[i]; _blendshapeMap[bsd->name] = bsd;
    }
}

BlendShapeAnimation::BlendShapeData* BlendShapeAnimation::getBlendShapeData(const std::string& key)
{
    if (_blendshapeMap.find(key) != _blendshapeMap.end())
        return _blendshapeMap[key].get();
    else return NULL;
}

void BlendShapeAnimation::update(osg::NodeVisitor* nv, osg::Drawable* drawable)
{
    osg::Geometry* geom = drawable->asGeometry();
    if (geom && geom->getVertexArray())
    {
        if (!_originalData) backupGeometryData(geom);
        handleBlending(geom, nv);
    }
}

void BlendShapeAnimation::backupGeometryData(osg::Geometry* geom)
{
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
    osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom->getNormalArray());
    osg::Vec4Array* ta = static_cast<osg::Vec4Array*>(geom->getVertexAttribArray(6));
    size_t vCount = va->size();

    _originalData = new BlendShapeData(1.0);
    _originalData->vertices = va;
    if (na && na->size() == vCount) _originalData->normals = na;
    if (ta && ta->size() == vCount) _originalData->tangents = ta;

    if (geom->getUseDisplayList() || !geom->getUseVertexBufferObjects())
    {
        geom->setUseDisplayList(false);
        geom->setUseVertexBufferObjects(true);
        OSG_NOTICE << "[BlendShapeAnimation] Obsoleted geometry type" << std::endl;
    }
}

void BlendShapeAnimation::handleBlending(osg::Geometry* geom, osg::NodeVisitor* nv)
{
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
    osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom->getNormalArray());
    osg::Vec4Array* ta = static_cast<osg::Vec4Array*>(geom->getVertexAttribArray(6));
    size_t vCount = va->size(), oriCount = _originalData->vertices->size();
    if (vCount != oriCount)
    {
        OSG_WARN << "[BlendShapeAnimation] Blendshape vertices count (" << vCount
                 << ") must equal to original ones (" << oriCount << ")" << std::endl;
        _originalData = NULL; return;
    }
    if (na && na->size() != vCount) na = NULL;
    if (ta && ta->size() != vCount) ta = NULL;

    memcpy(&(*va)[0], &(*(_originalData->vertices))[0], vCount * sizeof(osg::Vec3));
    for (size_t i = 0; i < _blendshapes.size(); ++i)
    {
        BlendShapeData* bsd = _blendshapes[i].get();
        if (osg::equivalent(bsd->weight, 0.0) || !bsd->vertices.valid()) continue;

        size_t numV = osg::minimum(bsd->vertices->size(), vCount);
        for (size_t v = 0; v < numV; ++v)
        {
            (*va)[v] += (*(bsd->vertices))[v] * bsd->weight;
            if (na && bsd->normals.valid())
                (*na)[v] += (*(bsd->normals))[v] * bsd->weight;
            if (ta && bsd->tangents.valid())
                (*ta)[v] += (*(bsd->tangents))[v] * bsd->weight;
        }
    }
    va->dirty(); geom->dirtyBound();
    if (na) na->dirty(); if (ta) ta->dirty();
}
