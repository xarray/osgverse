
#include <mesh/entity_iterators.hpp>
#include <mesh/triangle_mesh.hpp>

namespace Discregrid
{


unsigned int
FaceIterator::vertex(unsigned int i) const
{
    return m_mesh->faceVertex(m_index, i);
}

FaceIterator::reference
FaceIterator::operator*()
{
    return m_mesh->face(m_index);
}

FaceConstIterator::reference
FaceConstIterator::operator*()
{
    return m_mesh->face(m_index);
}

unsigned int&
FaceIterator::vertex(unsigned int i)
{
    return m_mesh->faceVertex(m_index, i);
}

VertexIterator::reference
VertexIterator::operator*()
{
    return m_mesh->vertex(m_index);
}


VertexConstIterator::reference
VertexConstIterator::operator*()
{
    return m_mesh->vertex(m_index);
}



unsigned int
VertexIterator::index() const
{
    return m_index;
}

IncidentFaceIterator::_Mytype&
IncidentFaceIterator::operator++()
{
    Halfedge o = m_mesh->opposite(m_h);
    if (o.isBoundary())
    {
        m_h = Halfedge();
        return *this;
    }
    m_h = o.next();
    if (m_h == m_begin)
    {
        m_h = Halfedge();
    }
    return *this;
}


IncidentFaceIterator::IncidentFaceIterator(unsigned int v, TriangleMesh const* mesh) 
    : m_mesh(mesh), m_h(mesh->incident_halfedge(v))
, m_begin(mesh->incident_halfedge(v))
{
    if (m_h.isBoundary())
        m_h = mesh->opposite(m_h).next();
}

}

