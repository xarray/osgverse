#include <osg/Texture2D>
#include <osg/Geode>
#include <osgUtil/SmoothingVisitor>
#include <mcut/mcut.h>
#include "CsgBoolean.h"
#include "MeshTopology.h"
#include "Utilities.h"
#include "Barycentric.h"
using namespace osgVerse;

static std::map<CsgBoolean::Operation, McFlags> s_booleanOperators =
{
    { CsgBoolean::A_NOT_B, MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE },
    { CsgBoolean::B_NOT_A, MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW },
    { CsgBoolean::UNION, MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE },
    { CsgBoolean::INTERSECTION, MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW }
};

osg::Node* CsgBoolean::process(Operation op, osg::Node* nodeA, osg::Node* nodeB)
{
    CsgBoolean csg(op);
    if (!csg.setMesh(nodeA, true)) return NULL;
    if (!csg.setMesh(nodeB, false)) return NULL;
    return csg.generate();
}

CsgBoolean::CsgBoolean(Operation op)
:   _operation(op)
{
}

bool CsgBoolean::setMesh(osg::Node* node, bool AorB)
{
    /*MeshTopologyVisitor mtv;
    mtv.setWeldingVertices(true);
    if (node) node->accept(mtv);

    osg::ref_ptr<osgVerse::MeshTopology> topology = mtv.generate();
    std::vector<std::vector<uint32_t>> entities = topology->getEntityFaces();
    for (size_t i = 0; i < entities.size(); ++i)
    {
        std::vector<uint32_t>& faces = entities[i];
        printf("%d: %d\n", AorB, faces.size());
    }*/

    MeshCollector collector;
    collector.setWeldingVertices(true);
    //osg::ref_ptr<osg::Geometry> geom = topology->output();
    //if (geom.valid()) collector.apply(*geom);
    node->accept(collector);

    const std::vector<osg::Vec3>& vertices = collector.getVertices();
    const std::vector<unsigned int>& indices = collector.getTriangles();
    MeshData& mesh = AorB ? _meshA : _meshB;
    if (vertices.empty() || indices.empty()) return false;

    mesh.vertexCoordsArray.resize(vertices.size() * 3);
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const osg::Vec3& v = vertices[i];
        mesh.vertexCoordsArray[3 * i + 0] = v[0];
        mesh.vertexCoordsArray[3 * i + 1] = v[1];
        mesh.vertexCoordsArray[3 * i + 2] = v[2];
    }

    mesh.faceSizesArray.resize(indices.size() / 3, 3);
    mesh.faceIndicesArray.resize(indices.size());
    memcpy(&(mesh.faceIndicesArray)[0], &indices[0], indices.size() * sizeof(uint32_t));
    return true;
}

osg::Node* CsgBoolean::generate()
{
    if (_meshA.vertexCoordsArray.empty() || _meshA.faceIndicesArray.empty() ||
        _meshB.vertexCoordsArray.empty() || _meshB.faceIndicesArray.empty()) return NULL;
    
    McContext context = MC_NULL_HANDLE;
    McResult err = mcCreateContext(&context, MC_NULL_HANDLE);
    McFlags boolOpFlags = s_booleanOperators[_operation];
    //std::cout << "Source: Vertices = " << _meshA.vertexCoordsArray.size() / 3
    //          << ", Triangles = " << _meshA.faceSizesArray.size() << std::endl
    //          << "Target: Vertices = " << _meshB.vertexCoordsArray.size() / 3
    //          << ", Triangles = " << _meshB.faceSizesArray.size() << std::endl;

    McUint32 attempts = 1 << 3; McDouble epsilon = 1e-4;
    err = mcBindState(context, MC_CONTEXT_GENERAL_POSITION_ENFORCEMENT_ATTEMPTS, sizeof(McUint32), (void*)&attempts);
    err = mcBindState(context, MC_CONTEXT_GENERAL_POSITION_ENFORCEMENT_CONSTANT, sizeof(McDouble), &epsilon);
    err = mcDispatch(
        context,
        MC_DISPATCH_VERTEX_ARRAY_DOUBLE | // vertices are in array of doubles
        MC_DISPATCH_ENFORCE_GENERAL_POSITION | // perturb if necessary
        MC_DISPATCH_INCLUDE_VERTEX_MAP | MC_DISPATCH_INCLUDE_FACE_MAP |  // vertex / face map
        boolOpFlags, // filter flags which specify the type of output we want
        reinterpret_cast<const void*>(_meshA.vertexCoordsArray.data()),
        reinterpret_cast<const uint32_t*>(_meshA.faceIndicesArray.data()), _meshA.faceSizesArray.data(),
        static_cast<uint32_t>(_meshA.vertexCoordsArray.size() / 3),
        static_cast<uint32_t>(_meshA.faceSizesArray.size()),
        reinterpret_cast<const void*>(_meshB.vertexCoordsArray.data()),
        reinterpret_cast<const uint32_t*>(_meshB.faceIndicesArray.data()),  _meshB.faceSizesArray.data(),
        static_cast<uint32_t>(_meshB.vertexCoordsArray.size() / 3),
        static_cast<uint32_t>(_meshB.faceSizesArray.size()));
    if (err != MC_NO_ERROR)
    {
        OSG_WARN << "[CsgBoolean] Failed to run CSG boolean operation: " << err << std::endl;
        mcReleaseContext(context); return NULL;
    }
    
    // Query the number of available connected component
    uint32_t numConnComps = 0;
    err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, 0, NULL, &numConnComps);
    if (err != MC_NO_ERROR)
    {
        OSG_WARN << "[CsgBoolean] Failed to get number of connected components: " << err << std::endl;
        mcReleaseContext(context); return NULL;
    }
    else if (numConnComps == 0)
    {
        OSG_WARN << "[CsgBoolean] No connected components found" << std::endl;
        mcReleaseContext(context); return NULL;
    }
    
    // Query the data of each connected component from MCUT
    std::vector<McConnectedComponent> connectedComponents(numConnComps, MC_NULL_HANDLE);
    connectedComponents.resize(numConnComps);
    err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT,
                                   (uint32_t)connectedComponents.size(), connectedComponents.data(), NULL);
    if (err != MC_NO_ERROR)
    {
        OSG_WARN << "[CsgBoolean] Failed to get connected components: " << err << std::endl;
        mcReleaseContext(context); return NULL;
    }

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    for (uint32_t i = 0; i < numConnComps; ++i)
    {
        McConnectedComponent connComp = connectedComponents[i];
        
        // Query the vertices
        size_t numBytesV = 0, numBytesF = 0;
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE,
                                          0, NULL, &numBytesV);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get vertex count: " << err << std::endl; continue; }
        
        uint32_t ccVertexCount = (uint32_t)(numBytesV / (sizeof(double) * 3));
        std::vector<double> ccVertices((uint64_t)ccVertexCount * 3u, 0);
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE,
                                          numBytesV, (void*)ccVertices.data(), NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get vertices: " << err << std::endl; continue; }
        
        // Triangulated faces
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION,
                                          0, NULL, &numBytesF);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get face count: " << err << std::endl; continue; }
        
        std::vector<uint32_t> ccFaceIndices(numBytesF / sizeof(uint32_t), 0);
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION,
                                          numBytesF, ccFaceIndices.data(), NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get faces: " << err << std::endl; continue; }
        
        std::vector<uint32_t> faceSizes(ccFaceIndices.size() / 3, 3);
        const uint32_t ccFaceCount = static_cast<uint32_t>(faceSizes.size());
        numBytesV = 0; numBytesF = 0;

        // Get vertex and face map
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_MAP,
                                          0, NULL, &numBytesV);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get vertex map count: " << err << std::endl; continue; }

        std::vector<uint32_t> ccVertexMap(numBytesV / sizeof(uint32_t));
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_MAP,
                                          numBytesV, ccVertexMap.data(), NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get vertex map: " << err << std::endl; continue; }

        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_MAP,
                                          0, NULL, &numBytesF);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get face map count: " << err << std::endl; continue; }

        std::vector<uint32_t> ccFaceMap(numBytesF / sizeof(uint32_t), 0);
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_MAP,
                                          numBytesF, ccFaceMap.data(), NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get face map: " << err << std::endl; continue; }

        // When connected components, pertain particular boolean operations
        McPatchLocation patchLocation = (McPatchLocation)0;
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_PATCH_LOCATION,
                                          sizeof(McPatchLocation), &patchLocation, NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get patch location: " << err << std::endl; continue; }

        McFragmentLocation fragmentLocation = (McFragmentLocation)0;
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FRAGMENT_LOCATION,
                                          sizeof(McFragmentLocation), &fragmentLocation, NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to fragment location: " << err << std::endl; continue; }
        
        // Write vertices and faces to new geometry
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(ccVertexCount);
        for (uint32_t i = 0; i < ccVertexCount; ++i)
        {
            double x = ccVertices[(uint64_t)i * 3 + 0];
            double y = ccVertices[(uint64_t)i * 3 + 1];
            double z = ccVertices[(uint64_t)i * 3 + 2];
            (*va)[i] = osg::Vec3(x, y, z);
        }
        
        // For each face in CC
        int faceVertexOffsetBase = 0;
        size_t faceCountA = _meshA.faceSizesArray.size(),
               vertexCountA = _meshA.vertexCoordsArray.size() / 3;
        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
        for (uint32_t f = 0; f < ccFaceCount; ++f)
        {
            bool reverseWindingOrder = (fragmentLocation == MC_FRAGMENT_LOCATION_BELOW) &&
                                       (patchLocation == MC_PATCH_LOCATION_OUTSIDE);
            int faceSize = faceSizes.at(f);
            if (faceSize > 3) OSG_WARN << "[CsgBoolean] Invalid face size: " << faceSize << std::endl;

            const uint32_t imFaceIdxRaw = ccFaceMap.at(f);
            bool faceFromA = (imFaceIdxRaw < faceCountA);
            uint32_t imFaceIdx = faceFromA ? imFaceIdxRaw : (imFaceIdxRaw - faceCountA);

            // For each vertex in face
            for (int v = (reverseWindingOrder ? (faceSize - 1) : 0);
                 (reverseWindingOrder ? (v >= 0) : (v < faceSize)); v += (reverseWindingOrder ? -1 : 1))
            {
                const int ccVertexIdx = ccFaceIndices[(uint64_t)faceVertexOffsetBase + v];
                de->push_back(ccVertexIdx);

                const uint32_t imVertexIdxRaw = ccVertexMap.at(ccVertexIdx);
                const bool vertexIsIntersection = (imVertexIdxRaw == MC_UNDEFINED_VALUE);
                bool vertexFromA = (imVertexIdxRaw < vertexCountA);
                uint32_t imVertexIdx = vertexFromA ? imVertexIdxRaw : (imVertexIdxRaw - vertexCountA);

                // TODO
                // - IF (vertexIsIntersection)
                //   - Get current intersection vertex (ccVertexIdx)
                //   - Get origin face from imFaceIdx
                //   - Compute barycentric coords of point on origin face
                //   - Get texcoord of vertices of origin face and interpolate a new one
                // - ELSE
                //   - Get texcoord from meshA or meshB
            }
            faceVertexOffsetBase += faceSize;
        }
        
        // Save to OSG geometry
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false);
        geom->setUseVertexBufferObjects(true);
        geom->setVertexArray(va.get());
        geom->addPrimitiveSet(de.get());
        geode->addDrawable(geom.get());
    }
    mcReleaseConnectedComponents(context, (uint32_t)connectedComponents.size(), connectedComponents.data());
    mcReleaseContext(context);

    osgUtil::SmoothingVisitor smv; geode->accept(smv);
    return geode.release();
}
