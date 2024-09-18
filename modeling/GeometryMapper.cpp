#include <osg/Version>
#include <algorithm>
#include "Math.h"
#include "Utilities.h"
#include "GeometryMapper.h"
using namespace osgVerse;

struct VertexIndex : public osg::Referenced
{ unsigned int index; VertexIndex(unsigned int i) {index = i;} };

class MapAttributeVisitor : public MeshCollector
{
public:
    MapAttributeVisitor(PointCloudQuery* q) : _query(q) {}

    void setAttributes(std::vector<osg::Vec4>* c, std::vector<osg::Vec4>* u)
    { _colors = c; _texcoords = u; }

    void setStateSets(std::map<osg::StateSet*, std::vector<size_t>>* vss)
    { _verticesOfStateSets = vss; }

    virtual void apply(osg::Geometry& geom)
    {
        osg::Matrix matrix;
        if (_matrixStack.size() > 0) matrix = _matrixStack.back();

        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom.getVertexArray());
        std::map<osg::StateSet*, unsigned int> stateSetCounts;
        for (size_t i = 0; i < va->size(); ++i)
        {
            std::vector<uint32_t> resultIndices(1);
            _query->findNearest((*va)[i] * matrix, resultIndices, 1);

            unsigned int index = resultIndices[0];
            if (_verticesOfStateSets)
            {
                for (std::map<osg::StateSet*, std::vector<size_t>>::iterator itr =
                     _verticesOfStateSets->begin(); itr != _verticesOfStateSets->end(); ++itr)
                {
                    std::vector<size_t>::iterator found = std::find(
                        itr->second.begin(), itr->second.end(), (size_t)index);
                    if (found != itr->second.end()) stateSetCounts[itr->first]++;
                }
            }

            if (_colors && index < _colors->size())
            {
                osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(geom.getColorArray());
                if (!ca)
                {
                    ca = new osg::Vec4Array(va->size());
                    geom.setColorArray(ca); geom.setColorBinding(osg::Geometry::BIND_PER_VERTEX);
                }
                (*ca)[i] = (*_colors)[index];
            }

            if (_texcoords && index < _texcoords->size())
            {
                osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom.getTexCoordArray(0));
                if (!ta) { ta = new osg::Vec2Array(va->size()); geom.setTexCoordArray(0, ta); }
                (*ta)[i] = osg::Vec2((*_texcoords)[index].x(), (*_texcoords)[index].y());
            }
        }

        // Find preferred state-set
        unsigned int bestCount = 0; osg::StateSet* preferred = NULL;
        for (std::map<osg::StateSet*, unsigned int>::iterator itr = stateSetCounts.begin();
             itr != stateSetCounts.end(); ++itr)
        { if (bestCount < itr->second) {bestCount = itr->second; preferred = itr->first;} }

        if (preferred != NULL) geom.setStateSet(preferred);
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        traverse(geom);
#endif
    }

protected:
    PointCloudQuery* _query;
    std::vector<osg::Vec4>* _colors;
    std::vector<osg::Vec4>* _texcoords;
    std::map<osg::StateSet*, std::vector<size_t>>* _verticesOfStateSets;
};

GeometryMapper::GeometryMapper()
{}

GeometryMapper::~GeometryMapper()
{}

void GeometryMapper::mapAttributes(osg::Node* source, osg::Node* target)
{
    MeshCollector mc; source->accept(mc);
    const std::vector<osg::Vec3>& vertices = mc.getVertices();
    std::vector<osg::Vec4>& colors = mc.getAttributes(MeshCollector::ColorAttr);
    std::vector<osg::Vec4>& texcoords = mc.getAttributes(MeshCollector::UvAttr);
    std::map<osg::StateSet*, std::vector<size_t>>& vssMap = mc.getVerticesOfStateSets();

    PointCloudQuery query;
    for (size_t i = 0; i < vertices.size(); ++i)
        query.addPoint(vertices[i], new VertexIndex(i));
    query.buildIndex();

    MapAttributeVisitor mav(&query);
    mav.setAttributes(&colors, &texcoords);
    mav.setStateSets(&vssMap); target->accept(mav);
}

float GeometryMapper::computeSimilarity(osg::Node* source, osg::Node* target)
{
    MeshCollector mc0; source->accept(mc0);
    MeshCollector mc1; target->accept(mc1);
    const std::vector<osg::Vec3>& vertices0 = mc0.getVertices();
    const std::vector<osg::Vec3>& vertices1 = mc1.getVertices();

    PointCloudQuery query;
    for (size_t i = 0; i < vertices0.size(); ++i)
        query.addPoint(vertices0[i], new VertexIndex(i));
    query.buildIndex();

    std::vector<float> lengthList; float maxD = 0.0f, minD = FLT_MAX;
    for (size_t i = 0; i < vertices1.size(); ++i)
    {
        std::vector<uint32_t> resultIndices(1);
        float d = sqrt(query.findNearest(vertices1[i], resultIndices, 1));
        if (maxD < d) maxD = d; if (minD > d) minD = d;
        lengthList.push_back(d);
    }
    if (vertices1.empty() || maxD == minD) return -1.0f;

    float totalLength = 0.0f, diff = maxD - minD, num = (float)lengthList.size();
    for (size_t i = 0; i < lengthList.size(); ++i)
        totalLength += (lengthList[i] - minD) / diff;
    return totalLength / num;
}
