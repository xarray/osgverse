#pragma once

#include "entity_iterators.hpp"

namespace Discregrid
{

class TriangleMesh;

class FaceContainer
{

public:

	FaceIterator begin() const
	{
		return FaceIterator(0, m_mesh);
	}
	FaceIterator end() const;

private:

	friend class TriangleMesh;
	FaceContainer(TriangleMesh* mesh) : m_mesh(mesh) {}

	TriangleMesh* m_mesh;
};

class FaceConstContainer
{

public:

	FaceConstIterator begin() const
	{
		return FaceConstIterator(0, m_mesh);
	}
	FaceConstIterator end() const;

private:

	friend class TriangleMesh;
	FaceConstContainer(TriangleMesh const* mesh) : m_mesh(mesh) {}

	TriangleMesh const* m_mesh;
};


class IncidentFaceContainer
{

public:

	IncidentFaceIterator begin() const
	{
		return IncidentFaceIterator(m_v, m_mesh);
	}
	IncidentFaceIterator end() const
	{
		return IncidentFaceIterator();
	}

private:

	friend class TriangleMesh;
	IncidentFaceContainer(unsigned int v, TriangleMesh const* mesh) 
		: m_v(v), m_mesh(mesh) {}

	TriangleMesh const* m_mesh;
	unsigned int m_v;
};

class VertexContainer
{

public:

	VertexIterator begin() const
	{
		return VertexIterator(0, m_mesh);
	}
	VertexIterator end() const;

private:

	friend class TriangleMesh;
	VertexContainer(TriangleMesh* mesh) : m_mesh(mesh) {}

	TriangleMesh* m_mesh;
};

class VertexConstContainer
{

public:

	VertexConstIterator begin() const
	{
		return VertexConstIterator(0, m_mesh);
	}
	VertexConstIterator end() const;

private:

	friend class TriangleMesh;
	VertexConstContainer(TriangleMesh const* mesh) : m_mesh(mesh) {}

	TriangleMesh const* m_mesh;
};
}

