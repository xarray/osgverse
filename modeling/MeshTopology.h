#ifndef MANA_MODELING_MESHTOPOLOGY_HPP
#define MANA_MODELING_MESHTOPOLOGY_HPP

#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <map>
#include <vector>
#include <iostream>
namespace pmp { class SurfaceMesh; }

namespace osgVerse
{
    class MeshCollector;
    
    class MeshTopology : public osg::Referenced
    {
    public:
        enum TopologyType { MVertex, MHalfEdge, MEdge, MFace };
        enum QueryType { CVertices, CHalfEdges, CEdges, CFaces };
        MeshTopology();

        /** Generate mesh structure */
        pmp::SurfaceMesh* generate(MeshCollector* collector);

        /** Generate OSG scene geometry */
        osg::Geometry* output();

        /** Topology getters */
        size_t getNumTopologyData(TopologyType t) const;
        std::vector<uint32_t> getTopologyData(TopologyType t) const;

        /** Connectivity queries */
        std::vector<uint32_t> getConnectiveData(
            TopologyType t, uint32_t idx, QueryType q) const;
        uint32_t findEdge(uint32_t v0, uint32_t v1);
        uint32_t findHalfEdge(uint32_t v0, uint32_t v1);

        /** Topological operations */
        void splitEdge(uint32_t idx, const osg::Vec3& pt);
        void splitFace(uint32_t idx, const osg::Vec3& pt);
        void flipEdge(uint32_t idx);
        void collapseHalfEdge(uint32_t idx);

        /** Algorithms */
        bool simplify(float percentage, int aspectRatio = 10, int normalDeviation = 180);
        bool remesh(float uniformValue, bool adaptive);

    protected:
        virtual ~MeshTopology();
        pmp::SurfaceMesh* _mesh;
    };
}

#endif
