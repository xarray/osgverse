#include <osg/Version>
#include <osg/TriangleIndexFunctor>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <pipeline/Global.h>
#include "DracoProcessor.h"
#include "Utilities.h"
#include <mutex>
using namespace osgVerse;

#include <metis/metis.h>
#include <meshoptimizer/meshoptimizer.h>
#ifdef VERSE_USE_DRACO
#   include <draco/mesh/mesh.h>
#   include <draco/compression/encode.h>
#   include <draco/compression/decode.h>
#endif

struct CollectFaceOperator
{
    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (i1 == i2 || i2 == i3 || i1 == i3) return;
        triangles.push_back(osg::Vec3i(i1, i2, i3));
    }
    std::vector<osg::Vec3i> triangles;
};

bool MeshOptimizer::decodeData(std::istream& in, osg::Geometry* geom)
{
    return false;
}

bool MeshOptimizer::encodeData(std::ostream& out, osg::Geometry* geom)
{
    return false;
}

bool MeshOptimizer::optimize(osg::Geometry* geom)
{
    osg::TriangleIndexFunctor<CollectFaceOperator> functor;
    if (geom) geom->accept(functor); else return false;
    osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    osg::Vec3Array* na = dynamic_cast<osg::Vec3Array*>(geom->getNormalArray());
    osg::Vec4Array* ca = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
    osg::Vec2Array* ta = dynamic_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));

    std::vector<meshopt_Stream> streams;
    bool validN = (na && na->size() == va->size());
    bool validC = (ca && ca->size() == va->size());
    bool validT = (ta && ta->size() == va->size());
    if (va && !va->empty())
    {
        meshopt_Stream ms; ms.data = &(*va)[0];
        ms.size = sizeof(float) * 3; ms.stride = ms.size;
        streams.push_back(ms);

        if (validN)
        {
            meshopt_Stream ms; ms.data = &(*na)[0];
            ms.size = sizeof(float) * 3; ms.stride = ms.size;
            streams.push_back(ms);
        }
        if (validC)
        {
            meshopt_Stream ms; ms.data = &(*ca)[0];
            ms.size = sizeof(float) * 4; ms.stride = ms.size;
            streams.push_back(ms);
        }
        if (validT)
        {
            meshopt_Stream ms; ms.data = &(*ta)[0];
            ms.size = sizeof(float) * 2; ms.stride = ms.size;
            streams.push_back(ms);
        }
    }

    if (streams.empty() || functor.triangles.empty())
    { OSG_WARN << "[MeshOptimizer] No enough data to optimize\n"; return false; }

    unsigned int totalIndices = functor.triangles.size() * 3;
    std::vector<unsigned int> indices(totalIndices);
    memcpy(&indices[0], &functor.triangles[0], totalIndices * sizeof(int));

    std::vector<unsigned int> remap(totalIndices);
    size_t totalVertices = meshopt_generateVertexRemapMulti(
        &remap[0], &indices[0], totalIndices, va->size(), &streams[0], streams.size());
    std::vector<osg::Vec3> newP(totalVertices), newN(totalVertices);
    std::vector<osg::Vec4> newC(totalVertices); std::vector<osg::Vec2> newT(totalVertices);

    meshopt_remapIndexBuffer(&indices[0], &indices[0], totalIndices, remap.data());
    meshopt_remapVertexBuffer(&newP[0], &(*va)[0], va->size(), sizeof(osg::Vec3), remap.data());
    if (validN)
        meshopt_remapVertexBuffer(&newN[0], &(*na)[0], va->size(), sizeof(osg::Vec3), remap.data());
    if (validC)
        meshopt_remapVertexBuffer(&newC[0], &(*ca)[0], va->size(), sizeof(osg::Vec4), remap.data());
    if (validT)
        meshopt_remapVertexBuffer(&newT[0], &(*ta)[0], va->size(), sizeof(osg::Vec2), remap.data());

    meshopt_optimizeVertexCache(&indices[0], &indices[0], totalIndices, totalIndices);
    meshopt_optimizeOverdraw(&indices[0], &indices[0], totalIndices, (float*)&newP[0],
                             totalVertices, sizeof(osg::Vec3), 1.05f);
    meshopt_optimizeVertexFetch(&newP[0], &indices[0], totalIndices, &newP[0],
                                totalVertices, sizeof(osg::Vec3));

    meshopt_optimizeVertexFetchRemap(&remap[0], &indices[0], totalIndices, totalVertices);
    meshopt_remapVertexBuffer(&newP[0], &newP[0], totalVertices, sizeof(osg::Vec3), remap.data());
    if (validN)
        meshopt_remapVertexBuffer(&newN[0], &newN[0], totalVertices, sizeof(osg::Vec3), remap.data());
    if (validC)
        meshopt_remapVertexBuffer(&newC[0], &newC[0], totalVertices, sizeof(osg::Vec4), remap.data());
    if (validT)
        meshopt_remapVertexBuffer(&newT[0], &newT[0], totalVertices, sizeof(osg::Vec2), remap.data());

    // Recreate the geometry
    geom->setVertexArray(new osg::Vec3Array(newP.begin(), newP.end()));
    if (validT) geom->setTexCoordArray(0, new osg::Vec2Array(newT.begin(), newT.end()));
    if (validN)
    {
        geom->setNormalArray(new osg::Vec3Array(newN.begin(), newN.end()));
        geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (validC)
    {
        geom->setColorArray(new osg::Vec4Array(newC.begin(), newC.end()));
        geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    geom->removePrimitiveSet(0, geom->getNumPrimitiveSets());
    if (indices.size() < 65535)
    {
        osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_TRIANGLES);
        de->resize(indices.size()); geom->addPrimitiveSet(de);
        for (size_t i = 0; i < indices.size(); ++i) (*de)[i] = indices[i];
    }
    else
    {
        osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES);
        de->resize(indices.size()); geom->addPrimitiveSet(de);
        memcpy(&(*de)[0], indices.data(), sizeof(unsigned int) * indices.size());
    }
    return true;
}

std::vector<MeshOptimizer::Cluster> MeshOptimizer::clusterize(
        osg::Geometry* geom, const std::vector<unsigned int>& indices, size_t kClusterSize, int kMetisSlop)
{
    std::vector<MeshOptimizer::Cluster> clusters;
    osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    if (!va || indices.empty() || (va && va->empty())) return clusters;

    std::vector<unsigned int> shadowib(indices.size());
    meshopt_generateShadowIndexBuffer(
        &shadowib[0], &indices[0], indices.size(), &(*va)[0], va->size(),
        sizeof(osg::Vec3), sizeof(osg::Vec3));

    std::vector<std::vector<int> > trilist(va->size());
    for (size_t i = 0; i < indices.size(); ++i)
        trilist[shadowib[i]].push_back(int(i / 3));

    std::vector<int> xadj(indices.size() / 3 + 1), adjncy, adjwgt;
    std::vector<int> part(indices.size() / 3), scratch;
    for (size_t i = 0; i < indices.size() / 3; ++i)
    {
        unsigned int a = shadowib[i * 3 + 0], b = shadowib[i * 3 + 1],
                     c = shadowib[i * 3 + 2]; scratch.clear();
        scratch.insert(scratch.end(), trilist[a].begin(), trilist[a].end());
        scratch.insert(scratch.end(), trilist[b].begin(), trilist[b].end());
        scratch.insert(scratch.end(), trilist[c].begin(), trilist[c].end());
        std::sort(scratch.begin(), scratch.end());

        for (size_t j = 0; j < scratch.size(); ++j)
        {
            if (scratch[j] == int(i)) continue;
            if (j == 0 || scratch[j] != scratch[j - 1])
                { adjncy.push_back(scratch[j]); adjwgt.push_back(1); }
            else if (j != 0)
                adjwgt.back()++;
        }
        xadj[i + 1] = int(adjncy.size());
    }

    int options[METIS_NOPTIONS]; METIS_SetDefaultOptions(options);
    options[METIS_OPTION_SEED] = 42;
    options[METIS_OPTION_UFACTOR] = 1; // minimize partition imbalance

    // since Metis can't enforce partition sizes, add a little slop to reduce the change
    // we need to split results further
    int nvtxs = int(indices.size() / 3), ncon = 1, edgecut = 0;
    int nparts = int(indices.size() / 3 + (kClusterSize - kMetisSlop) - 1) / (kClusterSize - kMetisSlop);
    if (nparts > 1)
    {   // not sure why this is a special case that we need to handle but okay metis
        int r = METIS_PartGraphRecursive(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL,
                                         &adjwgt[0], &nparts, NULL, NULL, options, &edgecut, &part[0]);
        if (r != METIS_OK);
        {
            OSG_NOTICE << "[MeshOptimizer] Failed to clusterize with METIS: " << r << std::endl;
            return clusters;
        }
    }

    clusters.resize(nparts);
    for (size_t i = 0; i < indices.size() / 3; ++i)
    {
        clusters[part[i]].indices.push_back(indices[i * 3 + 0]);
        clusters[part[i]].indices.push_back(indices[i * 3 + 1]);
        clusters[part[i]].indices.push_back(indices[i * 3 + 2]);
    }

    for (int i = 0; i < nparts; ++i)
    {   // need to split the cluster further...
        // this could use meshopt but we're trying to get a complete baseline from metis
        clusters[i].parentError = FLT_MAX;
        if (clusters[i].indices.size() > kClusterSize * 3)
        {
            std::vector<Cluster> splits = clusterize(geom, clusters[i].indices);
            if (splits.empty()) continue; else clusters[i] = splits[0];
            for (size_t j = 1; j < splits.size(); ++j) clusters.push_back(splits[j]);
        }
    }
    return clusters;
}

std::vector<std::vector<int>> MeshOptimizer::partition(
        const std::vector<Cluster>& clusters, const std::vector<int>& pending,
        const std::vector<int>& remap, size_t kGroupSize)
{
    std::vector<std::vector<int>> result, vertices(remap.size());
    for (size_t i = 0; i < pending.size(); ++i)
    {
        const Cluster& cluster = clusters[pending[i]];
        for (size_t j = 0; j < cluster.indices.size(); ++j)
        {
            int v = remap[cluster.indices[j]]; std::vector<int>& list = vertices[v];
            if (list.empty() || list.back() != int(i)) list.push_back(int(i));
        }
    }

    std::map<std::pair<int, int>, int> adjacency;
    for (size_t v = 0; v < vertices.size(); ++v)
    {
        const std::vector<int>& list = vertices[v];
        for (size_t i = 0; i < list.size(); ++i)
            for (size_t j = i + 1; j < list.size(); ++j)
                adjacency[std::make_pair(std::min(list[i], list[j]), std::max(list[i], list[j]))]++;
    }

    std::vector<std::vector<std::pair<int, int>>> neighbors(pending.size());
    for (std::map<std::pair<int, int>, int>::iterator it = adjacency.begin(); it != adjacency.end(); ++it)
    {
        neighbors[it->first.first].push_back(std::make_pair(it->first.second, it->second));
        neighbors[it->first.second].push_back(std::make_pair(it->first.first, it->second));
    }

    std::vector<int> part(pending.size()), xadj(pending.size() + 1), adjncy, adjwgt;
    for (size_t i = 0; i < pending.size(); ++i)
    {
        for (size_t j = 0; j < neighbors[i].size(); ++j)
        {
            adjncy.push_back(neighbors[i][j].first);
            adjwgt.push_back(neighbors[i][j].second);
        }
        xadj[i + 1] = int(adjncy.size());
    }

    int options[METIS_NOPTIONS]; METIS_SetDefaultOptions(options);
    options[METIS_OPTION_SEED] = 42;
    options[METIS_OPTION_UFACTOR] = 100;

    int nvtxs = int(pending.size()), ncon = 1, edgecut = 0;
    int nparts = int(pending.size() + kGroupSize - 1) / kGroupSize;
    if (nparts > 1)
    {
        int r = METIS_PartGraphRecursive(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL,
                                         &adjwgt[0], &nparts, NULL, NULL, options, &edgecut, &part[0]);
        if (r != METIS_OK);
        {
            OSG_NOTICE << "[MeshOptimizer] Failed to partition with METIS: " << r << std::endl;
            return result;
        }
    }

    result.resize(nparts);
    for (size_t i = 0; i < part.size(); ++i)
        result[part[i]].push_back(pending[i]);
    return result;
}

#ifdef VERSE_USE_DRACO
static osg::Array* createDataArray(draco::Mesh* mesh, const draco::PointAttribute* attr)
{
    osg::Array* outArray = NULL;
    switch (attr->data_type())
    {
    case draco::DT_FLOAT32:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::FloatArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2Array(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3Array(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4Array(mesh->num_points()); break;
        }
        break;
    case draco::DT_FLOAT64:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::DoubleArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2dArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3dArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4dArray(mesh->num_points()); break;
        }
        break;
    case draco::DT_INT8:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::ByteArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2bArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3bArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4bArray(mesh->num_points()); break;
        }
        break;
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
    case draco::DT_UINT8:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::UByteArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2ubArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3ubArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4ubArray(mesh->num_points()); break;
        }
        break;
#endif
    case draco::DT_INT16:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::ShortArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2sArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3sArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4sArray(mesh->num_points()); break;
        }
        break;
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
    case draco::DT_UINT16:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::UShortArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2usArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3usArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4usArray(mesh->num_points()); break;
        }
        break;
    case draco::DT_INT32:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::IntArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2iArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3iArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4iArray(mesh->num_points()); break;
        }
        break;
    case draco::DT_UINT32:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::UIntArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2uiArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3uiArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4uiArray(mesh->num_points()); break;
        }
        break;
#endif
    }

    if (outArray)
    {
        unsigned char* data = new unsigned char[attr->byte_stride()];
        const char* dst = (const char*)(outArray->getDataPointer());
        unsigned int elemSize = outArray->getTotalDataSize() / outArray->getNumElements();
        for (draco::PointIndex i(0); i < mesh->num_points(); ++i)
        {
            const draco::AttributeValueIndex valIndex = attr->mapped_index(i);
            attr->GetValue(valIndex, data);
            memcpy((void*)(dst + i.value() * elemSize), data, attr->byte_stride());
        }
        delete[] data;
    }
    else
        OSG_WARN << "[DracoProcessor] Unsupported Draco data type" << std::endl;
    return outArray;
}

static int addGeometryAttribute(draco::Mesh* mesh, draco::GeometryAttribute::Type type,
                                const osg::Array* arrayPtr)
{
    draco::DataType dt = draco::DT_INVALID;
    switch (arrayPtr->getDataType())
    {
    case GL_BYTE: dt = draco::DT_INT8; break;
    case GL_SHORT: dt = draco::DT_INT16; break;
    case GL_INT: dt = draco::DT_INT32; break;
    case GL_UNSIGNED_BYTE: dt = draco::DT_UINT8; break;
    case GL_UNSIGNED_SHORT: dt = draco::DT_UINT16; break;
    case GL_UNSIGNED_INT: dt = draco::DT_UINT32; break;
    case GL_FLOAT: dt = draco::DT_FLOAT32; break;
    case GL_DOUBLE: dt = draco::DT_FLOAT64; break;
    }

    draco::GeometryAttribute attr;
    attr.Init(type, nullptr, arrayPtr->getDataSize(), dt, false,
              draco::DataTypeLength(dt) * (int64_t)arrayPtr->getDataSize(), 0);
    return mesh->AddAttribute(attr, true, arrayPtr->getNumElements());
}
#endif

osg::Geometry* DracoProcessor::decodeDracoData(std::istream& in)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    if (decodeDracoData(in, geom.get()))
        return geom.release();
    return NULL;
}

bool DracoProcessor::decodeDracoData(std::istream& in, osg::Geometry* geom)
{
#ifdef VERSE_USE_DRACO
    std::string data((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    if (data.empty()) return false;

    draco::DecoderBuffer buffer;
    buffer.Init(data.data(), data.size());

    draco::StatusOr<draco::EncodedGeometryType> statusor =
        draco::Decoder::GetEncodedGeometryType(&buffer);
    if (!statusor.ok())
    {
        OSG_WARN << "[DracoProcessor] Error initing: " << statusor.status() << std::endl;
        return false;
    }
    else if (statusor.value() != draco::TRIANGULAR_MESH)
    {
        OSG_WARN << "[DracoProcessor] Unsupported mesh type: " << statusor.value() << std::endl;
        return false;
    }

    draco::Decoder decoder;
    draco::StatusOr<std::unique_ptr<draco::Mesh>> statusor2 =
        decoder.DecodeMeshFromBuffer(&buffer);
    if (!statusor2.ok())
    {
        OSG_WARN << "[DracoProcessor] Error decoding: " << statusor2.status() << std::endl;
        return false;
    }


    std::unique_ptr<draco::Mesh> mesh = std::move(statusor2).value();
    const draco::PointAttribute* va = mesh->GetNamedAttribute(draco::GeometryAttribute::POSITION);
    const draco::PointAttribute* na = mesh->GetNamedAttribute(draco::GeometryAttribute::NORMAL);
    const draco::PointAttribute* ca = mesh->GetNamedAttribute(draco::GeometryAttribute::COLOR);
    const draco::PointAttribute* ta = mesh->GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);

    if (va) geom->setVertexArray(createDataArray(mesh.get(), va));
    if (ta) geom->setTexCoordArray(0, createDataArray(mesh.get(), ta));
    if (na)
    {
        geom->setNormalArray(createDataArray(mesh.get(), na));
        geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (ca)
    {
        geom->setColorArray(createDataArray(mesh.get(), ca));
        geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    unsigned int index = 0;
    osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES, mesh->num_faces() * 3);
    for (draco::FaceIndex f(0); f < mesh->num_faces(); ++f)
    {
        const draco::Mesh::Face& face = mesh->face(f);
        (*de)[index++] = face[0].value();
        (*de)[index++] = face[1].value();
        (*de)[index++] = face[2].value();
    }
    geom->addPrimitiveSet(de);
    return true;
#else
    OSG_WARN << "[DracoProcessor] Dependency not found" << std::endl;
    return false;
#endif
}

bool DracoProcessor::encodeDracoData(std::ostream& out, osg::Geometry* geom)
{
#ifdef VERSE_USE_DRACO
    const int speed = 10 - _compressionLevel;
    draco::Encoder encoder;
    encoder.SetSpeedOptions(speed, speed);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, _posQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, _normalQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, _uvQuantizationBits);

    osg::TriangleIndexFunctor<CollectFaceOperator> functor;
    if (geom) geom->accept(functor);
    if (functor.triangles.empty())
    { OSG_WARN << "[DracoProcessor] No triangles to encode\n"; return false; }

    std::unique_ptr<draco::Mesh> mesh(new draco::Mesh());
    mesh->SetNumFaces(functor.triangles.size());
    for (size_t i = 0; i < functor.triangles.size(); ++i)
    {
        const osg::Vec3i& tri = functor.triangles[i];
        mesh->SetFace(draco::FaceIndex(i), {
            draco::PointIndex(tri[0]), draco::PointIndex(tri[1]), draco::PointIndex(tri[2]) });
    }

    const osg::Array* va = geom->getVertexArray();
    const osg::Array* na = geom->getNormalArray();
    const osg::Array* ca = geom->getColorArray();
    const osg::Array* ta = geom->getTexCoordArray(0);
    if (!va) { OSG_WARN << "[DracoProcessor] No vertex array to encode\n"; return false; }
    else mesh->set_num_points(va->getNumElements());

    int posAttrID = -1, uvAttrID = -1, normalAttrID = -1, colorAttrID = -1;
    posAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::POSITION, va);
    if (ta) uvAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::TEX_COORD, ta);
    if (na && geom->getNormalBinding() == osg::Geometry::BIND_PER_VERTEX)
        normalAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::NORMAL, na);
    if (ca && geom->getColorBinding() == osg::Geometry::BIND_PER_VERTEX)
        colorAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::COLOR, ca);

    if (posAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(posAttrID);
        const char* dst = (const char*)(va->getDataPointer());
        unsigned int elemSize = va->getTotalDataSize() / va->getNumElements();
        for (unsigned int i = 0; i < va->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), dst + i * elemSize);
    }

    if (normalAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(normalAttrID);
        const char* dst = (const char*)(na->getDataPointer());
        unsigned int elemSize = na->getTotalDataSize() / na->getNumElements();
        for (unsigned int i = 0; i < na->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), dst + i * elemSize);
    }

    if (colorAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(colorAttrID);
        const char* dst = (const char*)(ca->getDataPointer());
        unsigned int elemSize = ca->getTotalDataSize() / ca->getNumElements();
        for (unsigned int i = 0; i < ca->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), dst + i * elemSize);
    }

    if (uvAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(uvAttrID);
        if (attr->num_components() != 2)
        {
            // disable the predicator as it is causing an issue with decompression
            encoder.SetAttributePredictionScheme(
                draco::GeometryAttribute::Type::TEX_COORD, draco::PREDICTION_NONE);
        }

        const char* dst = (const char*)(ta->getDataPointer());
        unsigned int elemSize = ta->getTotalDataSize() / ta->getNumElements();
        for (unsigned int i = 0; i < ta->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), dst + i * elemSize);
    }

    draco::EncoderBuffer buffer;
    draco::Status status = encoder.EncodeMeshToBuffer(*mesh, &buffer);
    if (status.ok()) out.write(buffer.data(), buffer.size());
    return status.ok();
#else
    OSG_WARN << "[DracoProcessor] Dependency not found" << std::endl;
    return false;
#endif
}

DracoGeometry::DracoGeometry() : osg::Geometry() {}
DracoGeometry::DracoGeometry(const DracoGeometry& copy, const osg::CopyOp& op)
    : osg::Geometry(copy, op) {}
DracoGeometry::DracoGeometry(const osg::Geometry& copy, const osg::CopyOp& op)
    : osg::Geometry(copy, op) {}
