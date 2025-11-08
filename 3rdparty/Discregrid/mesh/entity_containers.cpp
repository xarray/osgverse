#include <mesh/entity_containers.hpp>
#include <mesh/triangle_mesh.hpp>

namespace Discregrid
{


FaceIterator
FaceContainer::end() const
{
    return FaceIterator(static_cast<unsigned int>(m_mesh->nFaces())
        , m_mesh);
}

FaceConstIterator
FaceConstContainer::end() const
{
    return FaceConstIterator(static_cast<unsigned int>(m_mesh->nFaces())
        , m_mesh);
}

VertexIterator
VertexContainer::end() const
{
    return VertexIterator(static_cast<unsigned int>(m_mesh->nVertices()),
        m_mesh);
}


VertexConstIterator
VertexConstContainer::end() const
{
    return VertexConstIterator(static_cast<unsigned int>(m_mesh->nVertices()),
        m_mesh);
}

}

