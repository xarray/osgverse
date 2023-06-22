#include <osg/Texture2D>
#include <mcut/mcut.h>
#include "CsgBoolean.h"
using namespace osgVerse;

static std::map<CsgBoolean::Operation, McFlags> s_booleanOperators =
{
    { CsgBoolean::A_NOT_B, MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE },
    { CsgBoolean::B_NOT_A, MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW },
    { CsgBoolean::UNION, MC_DISPATCH_FILTER_FRAGMENT_SEALING_OUTSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE },
    { CsgBoolean::INTERSECTION, MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW }
};

CsgBoolean::CsgBoolean(Operation op)
:   _operation(op)
{
}

void CsgBoolean::setMesh(osg::Node* node, bool AorB)
{
}

osg::Node* CsgBoolean::generate()
{
    if (_meshA.vertexCoordsArray.empty() || _meshA.faceIndicesArray.empty() ||
        _meshB.vertexCoordsArray.empty() || _meshB.faceIndicesArray.empty()) return NULL;
    
    McContext context = MC_NULL_HANDLE;
    McResult err = mcCreateContext(&context, MC_NULL_HANDLE);
    McFlags boolOpFlags = s_booleanOperators[_operation];
    
    err = mcDispatch(
        context,
        MC_DISPATCH_VERTEX_ARRAY_DOUBLE | // vertices are in array of doubles
        MC_DISPATCH_ENFORCE_GENERAL_POSITION | // perturb if necessary
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
    
    for (uint32_t i = 0; i < numConnComps; ++i)
    {
        McConnectedComponent connComp = connectedComponents[i];
        
        // Query the vertices
        size_t numBytesV = 0, numBytesF = 0;
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, 0, NULL, &numBytesV);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get vertex count: " << err << std::endl; continue; }
        
        uint32_t ccVertexCount = (uint32_t)(numBytesV / (sizeof(double) * 3));
        std::vector<double> ccVertices((uint64_t)ccVertexCount * 3u, 0);
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE,
                                          numBytesV, (void*)ccVertices.data(), NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get vertices: " << err << std::endl; continue; }
        
        // Triangulated faces
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, 0, NULL, &numBytesF);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get face count: " << err << std::endl; continue; }
        
        std::vector<uint32_t> ccFaceIndices(numBytesF / sizeof(uint32_t), 0);
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION,
                                          numBytesF, ccFaceIndices.data(), NULL);
        if (err != MC_NO_ERROR) { OSG_WARN << "[CsgBoolean] Failed to get faces: " << err << std::endl; continue; }
        
        std::vector<uint32_t> faceSizes(ccFaceIndices.size() / 3, 3);
        const uint32_t ccFaceCount = static_cast<uint32_t>(faceSizes.size());
        
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
        for (uint32_t i = 0; i < ccVertexCount; ++i)
        {
            double x = ccVertices[(uint64_t)i * 3 + 0];
            double y = ccVertices[(uint64_t)i * 3 + 1];
            double z = ccVertices[(uint64_t)i * 3 + 2];
            // TODO
        }
        
        // For each face in CC
        int faceVertexOffsetBase = 0;
        for (uint32_t f = 0; f < ccFaceCount; ++f)
        {
            bool reverseWindingOrder = (fragmentLocation == MC_FRAGMENT_LOCATION_BELOW) &&
                                       (patchLocation == MC_PATCH_LOCATION_OUTSIDE);
            int faceSize = faceSizes.at(f);
            
            // For each vertex in face
            for (int v = (reverseWindingOrder ? (faceSize - 1) : 0);
                 (reverseWindingOrder ? (v >= 0) : (v < faceSize)); v += (reverseWindingOrder ? -1 : 1))
            {
                const int ccVertexIdx = ccFaceIndices[(uint64_t)faceVertexOffsetBase + v];
                // TODO
            }
            faceVertexOffsetBase += faceSize;
        }
        
        // TODO: save to OSG node?
    }
    mcReleaseConnectedComponents(context, (uint32_t)connectedComponents.size(), connectedComponents.data());
    mcReleaseContext(context);
    return NULL;  // TODO
}
