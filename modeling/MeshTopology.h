#ifndef MANA_MODELING_MESHTOPOLOGY_HPP
#define MANA_MODELING_MESHTOPOLOGY_HPP

#include <osg/Geode>
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
        enum QueryType { QVertices, QHalfEdges, QEdges, QFaces };
        MeshTopology();

        /** Generate mesh structure */
        pmp::SurfaceMesh* generate(MeshCollector* collector);

        /** Generate OSG scene geometry */
        osg::Geometry* output(int entity = -1);
        osg::Geode* outputByEntity();

        /** Remove all deleted elements */
        void prune();

        /** Topology getters */
        size_t getNumTopologyData(TopologyType t) const;
        std::vector<uint32_t> getTopologyData(TopologyType t) const;

        /** Connectivity queries
            - MVertex
              - QHalfEdges: all half-edges of this vertex
              - QFaces: all faces containing this vertex
            - MHalfEdge
              - QVertices: from-vertex and to-vertex of this half-edge
              - QHalfEdges: the opposite half-edge
              - QEdges: the edge containing this half-edge
              - QFaces: the face containing this half-edge
            - MEdge
              - QVertices: 2 vertices of this edge
              - QHalfEdges: 1 or 2 half-edges of this edge
              - QFaces: 1 or 2 faces of this edge
            - MFace
              - QVertices: all vertices of this face
              - QHalfEdges: all half-edges of this face
        */
        std::vector<uint32_t> getConnectiveData(
            TopologyType t, uint32_t idx, QueryType q) const;

        bool isValid(TopologyType t, uint32_t idx) const;
        bool isBoundary(TopologyType t, uint32_t idx) const;
        bool isManifoldVertex(uint32_t idx) const;

        uint32_t findEdge(uint32_t v0, uint32_t v1) const;
        uint32_t findHalfEdge(uint32_t v0, uint32_t v1) const;
        uint32_t findPreviousHalfEdge(uint32_t idx) const;
        uint32_t findNextHalfEdge(uint32_t idx) const;

        /** Get all boundaries, each as a half-edge index list */
        std::vector<std::vector<uint32_t>> getHalfEdgeBoundaries() const;

        /** Get all entities, each as a face index list */
        std::vector<std::vector<uint32_t>> getEntityFaces() const;

        /** Get vertex data of the given index list */
        std::vector<osg::Vec3> getVertexData(TopologyType t, const std::vector<uint32_t>& v);

        /** Topological operations */
        void splitEdge(uint32_t idx, const osg::Vec3& pt);
        void splitFace(uint32_t idx, const osg::Vec3& pt);
        void flipEdge(uint32_t idx);
        void collapseHalfEdge(uint32_t idx);
        void deleteFace(uint32_t idx);

        /** Algorithms */
        bool simplify(float percentage, int aspectRatio = 10, int normalDeviation = 180);
        bool remesh(float uniformValue, bool adaptive);

    protected:
        virtual ~MeshTopology();

        bool findConnectedEdges(
            uint32_t he, std::vector<uint32_t>& subEdges, std::set<uint32_t>& usedEdges) const;
        void addNeighborFaces(std::set<uint32_t>& faceSet, uint32_t f) const;
        void processGeometryFaces(osg::Geometry* geom, const std::vector<uint32_t>& faces);

        pmp::SurfaceMesh* _mesh;
    };
}

#endif
