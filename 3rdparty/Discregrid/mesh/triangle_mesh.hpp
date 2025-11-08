#pragma once

#include "halfedge.hpp"
#include "entity_containers.hpp"

#include <vector>
#include <array>
#include <cassert>
#include <string>

#include <Eigen/Dense>

namespace Discregrid
{

class TriangleMesh
{

public:

	TriangleMesh(std::vector<Eigen::Vector3d> const& vertices, 
		std::vector<std::array<unsigned int, 3>> const& faces);

	TriangleMesh(double const* vertices,
		unsigned int const* faces, 
		std::size_t nv, std::size_t nf);

	TriangleMesh(std::string const& filename);

	void exportOBJ(std::string const& filename) const;

	// Halfedge modifiers.
	unsigned int source(Halfedge const h) const
	{
		if (h.isBoundary()) return target(opposite(h));
		return m_faces[h.face()][h.edge()];
	}
	unsigned int target(Halfedge const h) const
	{
		if (h.isBoundary()) return source(opposite(h));
		return source(h.next());
	}
	Halfedge opposite(Halfedge const h) const
	{
		if (h.isBoundary()) return m_b2e[h.face()];
		return m_e2e[h.face()][h.edge()];
	}

	// Container getters.
	FaceContainer faces() { return FaceContainer(this); }
	FaceConstContainer faces() const { return FaceConstContainer(this); }
	IncidentFaceContainer incident_faces(unsigned int v) const { 
		return IncidentFaceContainer(v, this); }
	VertexContainer vertices() { return VertexContainer(this); }
	VertexConstContainer vertices() const { return VertexConstContainer(this); }

	// Entity size getters.
	std::size_t nFaces() const { return m_faces.size(); }
	std::size_t nVertices() const { return m_v2e.size(); }
	std::size_t nBorderEdges() const { return m_b2e.size(); }

	// Entity getters.
	unsigned int const& faceVertex(unsigned int f, unsigned int i) const 
	{
		assert(i < 3);
		assert(f < m_faces.size());
		return m_faces[f][i];
	}
	unsigned int& faceVertex(unsigned int f, unsigned int i)
	{
		assert(i < 3);
		assert(f < m_faces.size());
		return m_faces[f][i];
	}

	Eigen::Vector3d const& vertex(unsigned int i) const { return m_vertices[i]; }
	Eigen::Vector3d& vertex(unsigned int i) { return m_vertices[i]; }
	std::array<unsigned int, 3> const& face(unsigned int i) const { 
		return m_faces[i]; }
	std::array<unsigned int, 3>& face(unsigned int i) {
		return m_faces[i];
	}
	Halfedge incident_halfedge(unsigned int v) const { return m_v2e[v]; }

	// Data getters.
	std::vector<Eigen::Vector3d> const& vertex_data() const { 
		return m_vertices; }
	std::vector<Eigen::Vector3d>& vertex_data() { return m_vertices; }    
	std::vector<std::array<unsigned int, 3>> const& face_data() const { 
		return m_faces; }
	std::vector<std::array<unsigned int, 3>>& face_data() { return m_faces; }

	Eigen::Vector3d computeFaceNormal(unsigned int f) const;

private:

	void construct();

private:

	std::vector<Eigen::Vector3d> m_vertices;
	std::vector<std::array<unsigned int, 3>> m_faces;
	std::vector<std::array<Halfedge, 3>> m_e2e;
	std::vector<Halfedge> m_v2e;
	std::vector<Halfedge> m_b2e;
};
}

