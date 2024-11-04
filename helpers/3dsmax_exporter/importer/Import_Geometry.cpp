/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The Geometry import implementation
*/

#include <osg/Geometry>
#include <osg/TriangleIndexFunctor>
#include "ImportImplementor.h"

struct CollectIndicesFunctor
{
    std::vector<unsigned int> _faces;
    void operator()( unsigned int p1, unsigned int p2, unsigned int p3 )
    {
        if ( p1==p2 || p2==p3 || p1==p3 )
            return;
        
        _faces.push_back( p1 );
        _faces.push_back( p2 );
        _faces.push_back( p3 );
    }
};

void ImportImplementor::createMeshContent( Mesh& mesh, MtlID mtlID, const osg::Geometry* geom,
                                           std::vector<unsigned int>& faces, unsigned int totalUnits )
{
    // Record vertices, colors and texture coordinates
    // FIXME: Ignored colors and normals at present
    const osg::Vec3Array* va = dynamic_cast<const osg::Vec3Array*>( geom->getVertexArray() );
    
    // Set vertices
    unsigned int vertStart = mesh.getNumVerts();
    if ( va )
    {
        mesh.setNumVerts( vertStart + va->size(), true );
        for ( unsigned int i=0; i<va->size(); ++i )
            mesh.verts[vertStart + i] = convertPoint( (*va)[i] );
    }
    else
    {
        NotifyBox( GetString(IDS_NOTFOUND), L"createMeshContent", L"Vertex array" );
        return;
    }
    
    // Set triangle face indices
    unsigned int ptr = 0, numFaces = faces.size() / 3, faceStart = mesh.getNumFaces();
    mesh.setNumFaces( faceStart + numFaces, true );
    for ( unsigned int i=0; i<numFaces; ++i, ptr+=3 )
    {
        unsigned int index = faceStart + i;
        mesh.faces[index].setVerts( vertStart+faces[ptr], vertStart+faces[ptr+1], vertStart+faces[ptr+2] );
        mesh.faces[index].setEdgeVisFlags( EDGE_VIS, EDGE_VIS, EDGE_VIS );
        mesh.faces[index].setMatID( mtlID );
    }
    
    // Collect texture coordinates
    std::vector< osg::ref_ptr<osg::Array> > texCoordsList(totalUnits);
    for ( unsigned int u=0; u<totalUnits; ++u )
    {
        if ( u<geom->getNumTexCoordArrays() )
            texCoordsList[u] = const_cast<osg::Array*>( geom->getTexCoordArray(u) );
        else  // Add some empty data to make the max mapping units work
            texCoordsList[u] = new osg::Vec2Array(va->size());
    }
    
    // Set texture coordinates
    for ( unsigned int u=0; u<texCoordsList.size(); ++u )
    {
        unsigned int ch = u + 1;
        mesh.setMapSupport( ch, TRUE );
        
        // Set UV/UVW vertices
        MeshMap& meshMap = mesh.Map(ch);
        unsigned int mapVertStart = meshMap.getNumVerts();
        const osg::Vec2Array* uv = dynamic_cast<const osg::Vec2Array*>( texCoordsList[u].get() );
        if ( uv )
        {
            meshMap.setNumVerts( mapVertStart + uv->size(), true );
            UVVert* tverts = meshMap.tv;
            for ( unsigned int i=0; i<uv->size(); ++i )
                tverts[mapVertStart + i] = convertPoint2( (*uv)[i] );
        }
        else
        {
            const osg::Vec3Array* uvw = dynamic_cast<const osg::Vec3Array*>( texCoordsList[u].get() );
            if ( uvw )
            {
                meshMap.setNumVerts( mapVertStart + uvw->size(), true );
                UVVert* tverts = meshMap.tv;
                for ( unsigned int i=0; i<uvw->size(); ++i )
                    tverts[mapVertStart + i] = convertPoint( (*uvw)[i] );
            }
            else
            {
                NotifyBox( GetString(IDS_NOTFOUND), L"createMeshContent", L"Texture coordinates" );
                mesh.setMapSupport( ch, FALSE );
                continue;
            }
        }
        
        // Set texture face indices
        if ( meshMap.getNumVerts()==mesh.getNumVerts() )
        {
            unsigned int ptr = 0;
            meshMap.setNumFaces( faceStart + numFaces, true );
            
            TVFace* tfaces = meshMap.tf;
            for ( unsigned int i=0; i<numFaces; ++i, ptr+=3 )
            {
                tfaces[faceStart + i].setTVerts(
                    mapVertStart+faces[ptr], mapVertStart+faces[ptr+1], mapVertStart+faces[ptr+2] );
            }
        }
        else
            mesh.setMapSupport( ch, FALSE );
    }
}

void ImportImplementor::importMultiGeometry( INode* maxNode, const osg::Node* geode,
                                             std::vector<const osg::Geometry*>& geoms )
{
    Object* maxObj = getMaxObject(geode);
    if ( !maxObj )
    {
        TriObject* triObj = CreateNewTriObject();
        Mesh& mesh = triObj->GetMesh();
        
        MultiMtl* multiMtl = NewDefaultMultiMtl(); 
        multiMtl->SetNumSubMtls( geoms.size() );
        _materialMap[geode] = multiMtl;
        
        // Checkk maximum used texture units
        unsigned int totalUnits = 0;
        for ( unsigned int i=0; i<geoms.size(); ++i )
        {
            totalUnits = osg::maximum(geoms[i]->getNumTexCoordArrays(), totalUnits);
        }
        mesh.setNumMaps( totalUnits<1 ? 2 : totalUnits+1 );  // 'One color channel' + 'one or more texcoords'
        
        // Collect all triangle faces and create mesh
        const osg::StateSet* ss = geode->getStateSet();
        for ( unsigned int i=0; i<geoms.size(); ++i )
        {
            const osg::Geometry* geom = geoms[i];
            osg::StateSet* ssDrawable = const_cast<osg::StateSet*>(geom->getStateSet());
            if (ssDrawable) { if (ss) ssDrawable->merge(*ss); }
            else ssDrawable = const_cast<osg::StateSet*>(ss);
            
            MSTR mtlName; mtlName.printf(L"SubMat %d", i);
            StdMat* stdMtl = static_cast<StdMat*>( createStandardMaterial(ssDrawable) );
            multiMtl->SetSubMtlAndName( i, stdMtl, mtlName );
            
            osg::TriangleIndexFunctor<CollectIndicesFunctor> cif;
            geom->accept( cif );
            createMeshContent( mesh, i, geom, cif._faces, totalUnits );
        }
        
        // Finalize the mesh
        mesh.DeleteIsoVerts();
        mesh.buildNormals();
        mesh.buildBoundingBox();
        mesh.BuildStripsAndEdges();
        mesh.InvalidateEdgeList();
        
        maxObj = triObj;
        _objectMap[geode] = maxObj;
    }
    
    Mtl* maxMtl = dynamic_cast<Mtl*>( getMaxMaterial(geode) );
    maxNode->SetObjectRef( maxObj );
    maxNode->SetMtl( maxMtl );
}

void ImportImplementor::importGeometry( INode* maxNode, const osg::Geometry* geom )
{
    Object* maxObj = getMaxObject(geom);
    if ( !maxObj )
    {
        TriObject* triObj = CreateNewTriObject();
        Mesh& mesh = triObj->GetMesh();
        
        unsigned int totalUnits = geom->getNumTexCoordArrays();
        mesh.setNumMaps( totalUnits<1 ? 2 : totalUnits+1 );  // 'One color channel' + 'one or more texcoords'
        
        // Collect all triangle faces and create mesh
        osg::TriangleIndexFunctor<CollectIndicesFunctor> cif;
        geom->accept( cif );
        createMeshContent( mesh, 0, geom, cif._faces, totalUnits );
        
        // Finalize the mesh
        mesh.DeleteIsoVerts();
        mesh.buildNormals();
        mesh.buildBoundingBox();
        mesh.BuildStripsAndEdges();
        mesh.InvalidateEdgeList();
        
        maxObj = triObj;
        _objectMap[geom] = maxObj;
    }
    maxNode->SetObjectRef( maxObj );
}
