#include <osg/Texture2D>
#include <pmp/SurfaceMesh.h>
#include <pmp/utilities.h>
//#include <pmp/algorithms/fairing.h>
#include <pmp/algorithms/remeshing.h>
#include <pmp/algorithms/decimation.h>
#include <pmp/algorithms/hole_filling.h>
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
    bool withNormals = (na.size() >= vertices.size());
    bool withColors = (ca.size() >= vertices.size());
    bool withUVs = (ta.size() >= vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const osg::Vec3& v = vertices[i];
        pmp::Vertex vec = _mesh->add_vertex(pmp::Point(v[0], v[1], v[2]));
        if (withNormals) normals[vec] = pmp::Normal(na[i][0], na[i][1], na[i][2]);
        if (withColors) colors[vec] = pmp::Color(ca[i][0], ca[i][1], ca[i][2]);
        if (withUVs) texcoords[vec] = pmp::TexCoord(ta[i][0], ta[i][1]);
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

osg::Geometry* MeshTopology::output(int eID)
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
        if (normals) {
            const pmp::Normal& n = normals[v];
            na->push_back(osg::Vec3(n[0], n[1], n[2]));
        }
        if (colors) {
            const pmp::Color& c = colors[v];
            ca->push_back(osg::Vec4(c[0], c[1], c[2], 1.0f));
        }
        if (texcoords) {
            const pmp::TexCoord& t = texcoords[v];
            ta->push_back(osg::Vec2(t[0], t[1]));
        }
    }

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get());
    if (texcoords) geom->setTexCoordArray(0, ta.get());
    if (normals)
    {
        geom->setNormalArray(na.get());
        geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (colors)
    {
        geom->setColorArray(ca.get());
        geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    if (eID >= 0)
    {
        std::vector<std::vector<uint32_t>> entities = getEntityFaces();
        if (eID < entities.size()) processGeometryFaces(geom.get(), entities[eID]);
    }
    else
        processGeometryFaces(geom.get(), std::vector<uint32_t>());
    return geom.release();
}

void MeshTopology::processGeometryFaces(osg::Geometry* geom, const std::vector<uint32_t>& faces)
{
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
    if (!faces.empty())
    {
        for (size_t i = 0; i < faces.size(); ++i)
        {
            std::vector<uint32_t> vList = getConnectiveData(MFace, faces[i], QVertices);
            for (size_t t = 0; t < vList.size(); ++t) de->push_back(vList[t]);
        }
    }
    else
    {
        pmp::SurfaceMesh::FaceContainer faces2 = _mesh->faces();
        for (auto f : faces2)
            for (auto h : _mesh->halfedges(f))
            {
                pmp::Vertex v = _mesh->to_vertex(h);
                if (v.is_valid()) de->push_back(v.idx());
                else { OSG_WARN << "[MeshTopology] Invalid mesh vertex" << std::endl; }
            }
    }
    geom->addPrimitiveSet(de.get());
}

osg::Geode* MeshTopology::outputByEntity()
{
    std::vector<std::vector<uint32_t>> entities = getEntityFaces();
    osg::ref_ptr<osg::Geometry> geom0 = output(entities.size());  // not applying primitives
    if (!geom0 || entities.empty()) return NULL;

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom0.get());
    for (size_t i = 1; i < entities.size(); ++i)
    {
        osg::ref_ptr<osg::Geometry> geom = (osg::Geometry*)geom0->clone(osg::CopyOp::SHALLOW_COPY);
        processGeometryFaces(geom.get(), entities[i]);
        geode->addDrawable(geom.get());
    }
    processGeometryFaces(geom0.get(), entities[0]);
    return geode.release();
}

void MeshTopology::prune()
{ _mesh->garbage_collection(); }

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

std::vector<uint32_t> MeshTopology::getConnectiveData(TopologyType t, uint32_t idx, QueryType q) const
{
    std::vector<uint32_t> data;
    switch (t)
    {
    case MVertex:
        switch (q)
        {
        case QHalfEdges:
            for (auto h : _mesh->halfedges(pmp::Vertex(idx))) data.push_back(h.idx()); break;
        case QFaces:
            for (auto f : _mesh->faces(pmp::Vertex(idx))) data.push_back(f.idx()); break;
        }
        break;
    case MHalfEdge:
        switch (q)
        {
        case QVertices:
            data.push_back(_mesh->from_vertex(pmp::Halfedge(idx)).idx());
            data.push_back(_mesh->to_vertex(pmp::Halfedge(idx)).idx()); break;
        case QHalfEdges:
            {
                pmp::Halfedge e0 = _mesh->opposite_halfedge(pmp::Halfedge(idx));
                if (e0.is_valid()) data.push_back(e0.idx()); break;
            }
        case QEdges:
            data.push_back(_mesh->edge(pmp::Halfedge(idx)).idx()); break;
        case QFaces:
            data.push_back(_mesh->face(pmp::Halfedge(idx)).idx()); break;
        }
        break;
    case MEdge:
        switch (q)
        {
        case QVertices:
            data.push_back(_mesh->vertex(pmp::Edge(idx), 0).idx());
            data.push_back(_mesh->vertex(pmp::Edge(idx), 1).idx()); break;
        case QHalfEdges:
            {
                pmp::Halfedge e0 = _mesh->halfedge(pmp::Edge(idx), 0);
                pmp::Halfedge e1 = _mesh->halfedge(pmp::Edge(idx), 1);
                if (e0.is_valid()) data.push_back(e0.idx());
                if (e1.is_valid()) data.push_back(e1.idx()); break;
            }
        case QFaces:
            {
                pmp::Face f0 = _mesh->face(pmp::Edge(idx), 0);
                pmp::Face f1 = _mesh->face(pmp::Edge(idx), 1);
                if (f0.is_valid()) data.push_back(f0.idx());
                if (f1.is_valid()) data.push_back(f1.idx()); break;
            }
        }
        break;
    case MFace:
        switch (q)
        {
        case QVertices:
            for (auto v : _mesh->vertices(pmp::Face(idx))) data.push_back(v.idx()); break;
        case QHalfEdges:
            for (auto h : _mesh->halfedges(pmp::Face(idx))) data.push_back(h.idx()); break;
        }
        break;
    }
    return data;
}

bool MeshTopology::isValid(TopologyType t, uint32_t idx) const
{
    switch (t)
    {
    case MVertex: return pmp::Vertex(idx).is_valid();
    case MHalfEdge: return pmp::Halfedge(idx).is_valid();
    case MEdge: return pmp::Edge(idx).is_valid();
    case MFace: return pmp::Face(idx).is_valid();
    }
    return false;
}

bool MeshTopology::isBoundary(TopologyType t, uint32_t idx) const
{
    switch (t)
    {
    case MVertex: return _mesh->is_boundary(pmp::Vertex(idx));
    case MHalfEdge: return _mesh->is_boundary(pmp::Halfedge(idx));
    case MEdge: return _mesh->is_boundary(pmp::Edge(idx));
    case MFace: return _mesh->is_boundary(pmp::Face(idx));
    }
    return false;
}

bool MeshTopology::isManifoldVertex(uint32_t idx) const
{ return _mesh->is_manifold(pmp::Vertex(idx)); }

uint32_t MeshTopology::findEdge(uint32_t v0, uint32_t v1) const
{ return _mesh->find_edge(pmp::Vertex(v0), pmp::Vertex(v1)).idx(); }

uint32_t MeshTopology::findHalfEdge(uint32_t v0, uint32_t v1) const
{ return _mesh->find_halfedge(pmp::Vertex(v0), pmp::Vertex(v1)).idx(); }

uint32_t MeshTopology::findPreviousHalfEdge(uint32_t idx) const
{ return _mesh->prev_halfedge(pmp::Halfedge(idx)).idx(); }

uint32_t MeshTopology::findNextHalfEdge(uint32_t idx) const
{ return _mesh->next_halfedge(pmp::Halfedge(idx)).idx(); }

void MeshTopology::splitEdge(uint32_t idx, const osg::Vec3& pt)
{ _mesh->split(pmp::Edge(idx), pmp::Point(pt[0], pt[1], pt[2])); }

void MeshTopology::splitFace(uint32_t idx, const osg::Vec3& pt)
{ _mesh->split(pmp::Face(idx), pmp::Point(pt[0], pt[1], pt[2])); }

void MeshTopology::flipEdge(uint32_t idx)
{ _mesh->flip(pmp::Edge(idx)); }

void MeshTopology::collapseHalfEdge(uint32_t idx)
{ _mesh->collapse(pmp::Halfedge(idx)); }

void MeshTopology::deleteFace(uint32_t idx)
{ _mesh->delete_face(pmp::Face(idx)); }

std::vector<std::vector<uint32_t>> MeshTopology::getHalfEdgeBoundaries() const
{
    std::vector<uint32_t> hEdges = getTopologyData(osgVerse::MeshTopology::MHalfEdge);
    std::vector<uint32_t> boundaries;
    for (size_t i = 0; i < hEdges.size(); ++i)
    {
        uint32_t he = hEdges[i];
        if (isBoundary(osgVerse::MeshTopology::MHalfEdge, he))
            boundaries.push_back(he);
    }

    std::set<uint32_t> usedEdges;
    std::vector<std::vector<uint32_t>> edgeChunkList;
    for (size_t i = 0; i < boundaries.size(); ++i)
    {
        uint32_t he = boundaries[i];
        if (usedEdges.find(he) != usedEdges.end()) continue;

        std::vector<uint32_t> subEdges;
        if (findConnectedEdges(he, subEdges, usedEdges))
            edgeChunkList.push_back(subEdges);
    }
    return edgeChunkList;
}

std::vector<std::vector<uint32_t>> MeshTopology::getEntityFaces() const
{
    std::vector<uint32_t> faces = getTopologyData(osgVerse::MeshTopology::MFace);
    std::map<uint32_t, std::set<uint32_t>> faceSetMap;
    for (size_t i = 0; i < faces.size(); ++i)
    {
        uint32_t f = faces[i]; bool alreadyAdded = false;
        for (std::map<uint32_t, std::set<uint32_t>>::iterator itr = faceSetMap.begin();
             itr != faceSetMap.end(); ++itr)
        {
            if (itr->second.find(f) != itr->second.end())
            { alreadyAdded = true; break; }
        }

        if (alreadyAdded) continue; else faceSetMap[f].insert(f);
        addNeighborFaces(faceSetMap[f], f);
    }

    std::vector<std::vector<uint32_t>> entityList;
    for (std::map<uint32_t, std::set<uint32_t>>::iterator itr = faceSetMap.begin();
         itr != faceSetMap.end(); ++itr)
    {
        std::vector<uint32_t> faces;
        faces.assign(itr->second.begin(), itr->second.end());
        entityList.push_back(faces);
    }
    return entityList;
}

std::vector<osg::Vec3> MeshTopology::getVertexData(TopologyType t, const std::vector<uint32_t>& v)
{
    pmp::VertexProperty<pmp::Point> points = _mesh->get_vertex_property<pmp::Point>("v:point");
    std::vector<pmp::Point>& pts = points.vector(); std::set<osg::Vec3> vertices;
    switch (t)
    {
    case MVertex:
        for (size_t i = 0; i < v.size(); ++i)
        {
            const pmp::Point& pt = pts[v[i]];
            vertices.insert(osg::Vec3(pt[0], pt[1], pt[2]));
        }
        break;
    case MHalfEdge: case MEdge: case MFace:
        for (size_t i = 0; i < v.size(); ++i)
        {
            std::vector<uint32_t> connData = getConnectiveData(t, v[i], QVertices);
            for (size_t j = 0; j < connData.size(); ++j)
            {
                const pmp::Point& pt = pts[connData[j]];
                vertices.insert(osg::Vec3(pt[0], pt[1], pt[2]));
            }
        }
        break;
    }
    return std::vector<osg::Vec3>(vertices.begin(), vertices.end());
}

bool MeshTopology::simplify(float percentage, int aspectRatio, int normalDeviation)
{
    try
    {
        pmp::decimate(*_mesh, _mesh->n_vertices() * percentage,
                      aspectRatio, 0.0, 0, normalDeviation);
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
            pmp::uniform_remeshing(*_mesh, l, 10, true); return true;
        }
        catch (const pmp::InvalidInputException& e)
        { OSG_WARN << "[MeshTopology] " << e.what() << std::endl; }
    }
    else
    {
        float bb = pmp::bounds(*_mesh).size();
        try
        {
            pmp::adaptive_remeshing(*_mesh, 0.0010 * bb, 0.0500 * bb,  // min/max length
                                    0.0005 * bb /*approx. error*/, 10, true);
            return true;
        }
        catch (const pmp::InvalidInputException& e)
        { OSG_WARN << "[MeshTopology] " << e.what() << std::endl; }
    }
    return false;
}

bool MeshTopology::findConnectedEdges(
    uint32_t he, std::vector<uint32_t>& subEdges, std::set<uint32_t>& usedEdges) const
{
    std::vector<uint32_t> ptOfHe = getConnectiveData(
        osgVerse::MeshTopology::MHalfEdge, he, osgVerse::MeshTopology::QVertices);
    usedEdges.insert(he); if (ptOfHe.size() < 2) return false;

    //if (topology->isManifoldVertex(ptOfHe[1])) {}  // TODO: check invalid vertex
    subEdges.push_back(he);

    bool hasFollowingEdges = false;
    std::vector<uint32_t> heOnPt = getConnectiveData(
        osgVerse::MeshTopology::MVertex, ptOfHe[1], osgVerse::MeshTopology::QHalfEdges);
    for (size_t j = 0; j < heOnPt.size(); ++j)
    {
        uint32_t he1 = heOnPt[j];
        //if (he1 == subEdges.front()) {}  // TODO: indicate good boundary
        if (usedEdges.find(he1) != usedEdges.end()) continue;
        if (!isBoundary(osgVerse::MeshTopology::MHalfEdge, he1)) continue;

        findConnectedEdges(he1, subEdges, usedEdges);
        hasFollowingEdges = true;
    }
    return hasFollowingEdges;
}

void MeshTopology::addNeighborFaces(std::set<uint32_t>& faceSet, uint32_t f) const
{
    std::vector<uint32_t> ptOfFace = getConnectiveData(
        osgVerse::MeshTopology::MFace, f, osgVerse::MeshTopology::QVertices);
    for (size_t j = 0; j < ptOfFace.size(); ++j)
    {
        std::vector<uint32_t> faceOfPt = getConnectiveData(
            osgVerse::MeshTopology::MVertex, ptOfFace[j], osgVerse::MeshTopology::QFaces);
        for (auto f1 : faceOfPt)
        {
            if (faceSet.find(f1) != faceSet.end()) continue;
            faceSet.insert(f1); //addNeighborFaces(faceSet, f1);  // expand one more to avoid overflow

            std::vector<uint32_t> ptOfFace2 = getConnectiveData(
                osgVerse::MeshTopology::MFace, f1, osgVerse::MeshTopology::QVertices);
            for (size_t k = 0; k < ptOfFace2.size(); ++k)
            {
                std::vector<uint32_t> faceOfPt2 = getConnectiveData(
                    osgVerse::MeshTopology::MVertex, ptOfFace2[j], osgVerse::MeshTopology::QFaces);
                for (auto f2 : faceOfPt)
                {
                    if (faceSet.find(f2) != faceSet.end()) continue;
                    faceSet.insert(f2); addNeighborFaces(faceSet, f2);
                }
            }
        }  // for (auto f1 : faceOfPt)
    }
}
