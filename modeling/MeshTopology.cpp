#include <osg/Texture2D>
#include <pmp/SurfaceMesh.h>
#include <pmp/algorithms/SurfaceFeatures.h>
#include <pmp/algorithms/SurfaceFairing.h>
#include <pmp/algorithms/SurfaceRemeshing.h>
#include <pmp/algorithms/SurfaceSimplification.h>
#include "MeshTopology.h"
#include "Utilities.h"
using namespace osgVerse;

MeshTopology::MeshTopology() : _mesh(NULL)
{
}

MeshTopology::~MeshTopology()
{ if (_mesh != NULL) delete _mesh; }


pmp::SurfaceMesh* MeshTopology::generate(MeshCollector* collector)
{
    if (_mesh != NULL) delete _mesh;
    _mesh = new pmp::SurfaceMesh;

    pmp::VertexProperty<pmp::Normal> normals = _mesh->vertex_property<pmp::Normal>("v:normal");
    pmp::VertexProperty<pmp::Color> colors = _mesh->vertex_property<pmp::Color>("v:color");
    pmp::VertexProperty<pmp::TexCoord> texcoords = _mesh->vertex_property<pmp::TexCoord>("v:tex");

    std::vector<osg::Vec4>& na = collector->getAttributes(MeshCollector::NormalAttr);
    std::vector<osg::Vec4>& ca = collector->getAttributes(MeshCollector::ColorAttr);
    std::vector<osg::Vec4>& ta = collector->getAttributes(MeshCollector::UvAttr);
    if (na.empty()) _mesh->remove_vertex_property(normals);
    if (ca.empty()) _mesh->remove_vertex_property(colors);
    if (ta.empty()) _mesh->remove_vertex_property(texcoords);

    const std::vector<osg::Vec3>& vertices = collector->getVertices();
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const osg::Vec3& v = vertices[i];
        pmp::Vertex vec = _mesh->add_vertex(pmp::Point(v[0], v[1], v[2]));
        if (!na.empty()) normals[vec] = pmp::Normal(na[i][0], na[i][1], na[i][2]);
        if (!ca.empty()) colors[vec] = pmp::Color(ca[i][0], ca[i][1], ca[i][2]);
        if (!ta.empty()) texcoords[vec] = pmp::TexCoord(ta[i][0], ta[i][1]);
    }

    const std::vector<unsigned int>& indices = collector->getTriangles();
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        std::vector<pmp::Vertex> face;
        face.push_back(pmp::Vertex(indices[i + 0]));
        face.push_back(pmp::Vertex(indices[i + 1]));
        face.push_back(pmp::Vertex(indices[i + 2]));

        try { _mesh->add_face(face); }
        catch (pmp::TopologyException& e)
        { OSG_WARN << "[MeshTopology] " << e.what() << std::endl; }
    }
    return _mesh;
}

osg::Geometry* MeshTopology::output()
{
    if (!_mesh) return NULL;
    pmp::VertexProperty<pmp::Point> points = _mesh->get_vertex_property<pmp::Point>("v:point");
    pmp::VertexProperty<pmp::Normal> normals = _mesh->get_vertex_property<pmp::Normal>("v:normal");
    pmp::VertexProperty<pmp::Color> colors = _mesh->get_vertex_property<pmp::Color>("v:color");
    pmp::VertexProperty<pmp::TexCoord> texcoords = _mesh->get_vertex_property<pmp::TexCoord>("v:tex");

    osg::ref_ptr<osg::Vec3Array> va = (points) ? new osg::Vec3Array : NULL;
    osg::ref_ptr<osg::Vec3Array> na = (normals) ? new osg::Vec3Array : NULL;
    osg::ref_ptr<osg::Vec4Array> ca = (colors) ? new osg::Vec4Array : NULL;
    osg::ref_ptr<osg::Vec2Array> ta = (texcoords) ? new osg::Vec2Array : NULL;
    if (!va) return NULL;

    std::vector<pmp::Point>& pts = points.vector();
    for (size_t i = 0; i < pts.size(); ++i)
    {
        const pmp::Point& pt = pts[i]; pmp::Vertex v(i);
        va->push_back(osg::Vec3(pt[0], pt[1], pt[2]));
        if (normals) { const pmp::Normal& n = normals[v];
                       na->push_back(osg::Vec3(n[0], n[1], n[2])); }
        if (colors) { const pmp::Color& c = colors[v];
                      ca->push_back(osg::Vec4(c[0], c[1], c[2], 1.0f)); }
        if (texcoords) { const pmp::TexCoord& t = texcoords[v];
                         ta->push_back(osg::Vec2(t[0], t[1])); }
    }

    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (auto f : _mesh->faces())
    {
        for (auto h : _mesh->halfedges(f))
        {
            pmp::Vertex v = _mesh->to_vertex(h);
            if (v.is_valid()) de->push_back(v.idx());
            else { OSG_WARN << "[MeshTopology] Invalid mesh vertex" << std::endl; }
        }
    }

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get());
    if (texcoords) geom->setTexCoordArray(0, ta.get());
    if (normals) { geom->setNormalArray(na.get());
                   geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
    if (colors) { geom->setColorArray(ca.get());
                  geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }
    geom->addPrimitiveSet(de.get());
    return geom.release();
}

size_t MeshTopology::getNumTopologyData(TopologyType t) const
{
    switch (t)
    {
    case MVertex: return _mesh->vertices_size();
    case MHalfEdge: return _mesh->halfedges_size();
    case MEdge: return _mesh->edges_size();
    case MFace: return _mesh->faces_size();
    }
    return 0;
}

std::vector<uint32_t> MeshTopology::getTopologyData(TopologyType t) const
{
    std::vector<uint32_t> data;
    switch (t)
    {
    case MVertex:
        for (auto v : _mesh->vertices()) data.push_back(v.idx()); break;
    case MHalfEdge:
        for (auto h : _mesh->halfedges()) data.push_back(h.idx()); break;
    case MEdge:
        for (auto e : _mesh->edges()) data.push_back(e.idx()); break;
    case MFace:
        for (auto f : _mesh->faces()) data.push_back(f.idx()); break;
    }
    return data;
}

std::vector<uint32_t> MeshTopology::getConnectiveData(
        TopologyType t, uint32_t idx, QueryType q) const
{
    std::vector<uint32_t> data;
    switch (t)
    {
    case MVertex:
        switch (q)
        {
        case CHalfEdges:
            for (auto h : _mesh->halfedges(pmp::Vertex(idx))) data.push_back(h.idx()); break;
        case CFaces:
            for (auto f : _mesh->faces(pmp::Vertex(idx))) data.push_back(f.idx()); break;
        }
        break;
    case MHalfEdge:
        switch (q)
        {
        case CVertices:
            data.push_back(_mesh->from_vertex(pmp::Halfedge(idx)).idx());
            data.push_back(_mesh->to_vertex(pmp::Halfedge(idx)).idx()); break;
        case CHalfEdges:
            data.push_back(_mesh->opposite_halfedge(pmp::Halfedge(idx)).idx()); break;
        case CEdges:
            data.push_back(_mesh->edge(pmp::Halfedge(idx)).idx()); break;
        case CFaces:
            data.push_back(_mesh->face(pmp::Halfedge(idx)).idx()); break;
        }
        break;
    case MEdge:
        switch (q)
        {
        case CVertices:
            data.push_back(_mesh->vertex(pmp::Edge(idx), 0).idx());
            data.push_back(_mesh->vertex(pmp::Edge(idx), 1).idx()); break;
        case CHalfEdges:
            data.push_back(_mesh->halfedge(pmp::Edge(idx), 0).idx());
            data.push_back(_mesh->halfedge(pmp::Edge(idx), 1).idx()); break;
        case CFaces:
            data.push_back(_mesh->face(pmp::Edge(idx), 0).idx());
            data.push_back(_mesh->face(pmp::Edge(idx), 1).idx()); break;
        }
        break;
    case MFace:
        switch (q)
        {
        case CVertices:
            for (auto v : _mesh->vertices(pmp::Face(idx))) data.push_back(v.idx()); break;
        case CHalfEdges:
            for (auto h : _mesh->halfedges(pmp::Face(idx))) data.push_back(h.idx()); break;
        }
        break;
    }
    return data;
}

uint32_t MeshTopology::findEdge(uint32_t v0, uint32_t v1)
{ return _mesh->find_edge(pmp::Vertex(v0), pmp::Vertex(v1)).idx(); }

uint32_t MeshTopology::findHalfEdge(uint32_t v0, uint32_t v1)
{ return _mesh->find_halfedge(pmp::Vertex(v0), pmp::Vertex(v1)).idx(); }

void MeshTopology::splitEdge(uint32_t idx, const osg::Vec3& pt)
{ _mesh->split(pmp::Edge(idx), pmp::Point(pt[0], pt[1], pt[2])); }

void MeshTopology::splitFace(uint32_t idx, const osg::Vec3& pt)
{ _mesh->split(pmp::Face(idx), pmp::Point(pt[0], pt[1], pt[2])); }

void MeshTopology::flipEdge(uint32_t idx)
{ _mesh->flip(pmp::Edge(idx)); }

void MeshTopology::collapseHalfEdge(uint32_t idx)
{ _mesh->collapse(pmp::Halfedge(idx)); }

bool MeshTopology::simplify(float percentage, int aspectRatio, int normalDeviation)
{
    try
    {
        pmp::SurfaceSimplification ss(*_mesh);
        ss.initialize(aspectRatio, 0.0, 0.0, normalDeviation, 0.0);
        ss.simplify(_mesh->n_vertices() * percentage);
        return true;
    }
    catch (const pmp::InvalidInputException& e)
    { OSG_WARN << "[MeshTopology] " << e.what() << std::endl; }
    return false;
}

bool MeshTopology::remesh(float uniformValue, bool adaptive)
{
    if (!adaptive)
    {
        try
        {
            float l = uniformValue;
            if (l <= 0.0f)
            {
                for (auto eit : _mesh->edges())
                    l += pmp::distance(_mesh->position(_mesh->vertex(eit, 0)),
                                       _mesh->position(_mesh->vertex(eit, 1)));
                l /= (float)_mesh->n_edges();
            }
            pmp::SurfaceRemeshing(*_mesh).uniform_remeshing(l); return true;
        }
        catch (const pmp::InvalidInputException& e)
        { OSG_WARN << "[MeshTopology] " << e.what() << std::endl; }
    }
    else
    {
        float bb = _mesh->bounds().size();
        try
        {
            pmp::SurfaceRemeshing(*_mesh).adaptive_remeshing(
                0.0010 * bb, 0.0500 * bb,  // min/max length
                0.0005 * bb); // approx. error
            return true;
        }
        catch (const pmp::InvalidInputException& e)
        { OSG_WARN << "[MeshTopology] " << e.what() << std::endl; }
    }
    return false;
}
