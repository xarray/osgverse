#ifdef VERSE_ENABLE_RASTERIZER
#   include <rasterizer/Occluder.h>
#   include <rasterizer/Rasterizer.h>
#endif
#include <osg/Texture2D>
#include "Rasterizer.h"

namespace osgVerse
{
#ifdef VERSE_ENABLE_RASTERIZER
    static __m128 convertFromVec4(const osg::Vec4& v)
    { __m128 vec = _mm_load_ps(v.ptr()); return vec; }

    static __m128 convertFromVec3(const osg::Vec3& v)
    { osg::Vec4 v4(v, 1.0f); return convertFromVec4(v4); }

    static osg::Vec4 convertToVec4(__m128 vec)
    { osg::Vec4 v; _mm_store_ps(v.ptr(), vec); return v; }

    static osg::Vec3 convertToVec3(__m128 vec)
    { osg::Vec4 v = convertToVec4(vec); return osg::Vec3(v[0], v[1], v[2]); }

    UserOccluder::UserOccluder(const std::string& name, const std::vector<osg::Vec3> vertices,
                               const osg::BoundingBoxf& refBound) : _name(name)
    {
        std::vector<__m128> vList;
        for (size_t i = 0; i < vertices.size(); ++i)
            vList.push_back(convertFromVec3(vertices[i]));
        __m128 refMin = convertFromVec3(refBound._min);
        __m128 refMax = convertFromVec3(refBound._max);
        _privateData = Occluder::bake(vList, refMin, refMax);
    }
    UserOccluder::~UserOccluder() {}

    osg::BoundingBoxf UserOccluder::getBound() const
    {
        osg::BoundingBoxf bb(convertToVec3(_privateData->m_boundsMin),
                             convertToVec3(_privateData->m_boundsMax));
        return bb;
    }

    osg::Vec3 UserOccluder::getCenter() const
    { return convertToVec3(_privateData->m_center); }

    // TODO
#else
    UserOccluder::UserOccluder(const std::string& name, const std::vector<osg::Vec3> vertices,
                               const osg::BoundingBoxf& refBound) : _name(name) {}
    UserOccluder::~UserOccluder() {}
    osg::BoundingBoxf UserOccluder::getBound() const { osg::BoundingBoxf bb; return bb; }
    osg::Vec3 UserOccluder::getCenter() const { osg::Vec3 vv; return vv; }

    Rasterizer::Rasterizer(unsigned int, unsigned int) {}
    Rasterizer::~Rasterizer() {}
    void Rasterizer::setModelViewProjection(const osg::Matrixd&) {}
    void Rasterizer::render(std::vector<float>& depthData)
    { OSG_WARN << "[Rasterizer] No AVX support, Rasterizer is not implemented" << std::endl; }
    bool Rasterizer::queryVisibility(const osg::BoundingBoxf& occluderBound, bool& needsClipping)
    { return false; }
    void Rasterizer::rasterize(UserOccluder& occluder, bool needsClipping) {}
    void Rasterizer::clear() {}
#endif

    void Rasterizer::removeOccluder(UserOccluder* o)
    {
        std::set<osg::ref_ptr<UserOccluder>>::iterator it = _occluders.find(o);
        if (it != _occluders.end()) _occluders.erase(it);
    }
}
