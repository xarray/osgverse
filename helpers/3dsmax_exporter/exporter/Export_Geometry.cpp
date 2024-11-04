/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The Geometry export implementation
*/

#include <osg/Geometry>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osgUtil/Optimizer>
#include "ExportImplementor.h"
#include <MeshNormalSpec.h>

static osg::Vec3 createVertexNormal( Mesh* mesh, int fi, int vi )
{
    Face& face = mesh->faces[fi];
	RVertex* rv = mesh->getRVertPtr( face.getVert(vi) );
	DWORD smGroup = face.smGroup;
	
	//Check for explicit normals
	MeshNormalSpec* meshNormal = mesh->GetSpecifiedNormals();
	if ( meshNormal && meshNormal->GetNumFaces() )
	{
	    int normID = meshNormal->Face(fi).GetNormalID(vi);
		if ( meshNormal->GetNormalExplicit(normID) )
		    return convertPoint(meshNormal->Normal(normID));
	}
	
	// // Get the normal from face or smoothing group
	Point3 normal = mesh->getFaceNormal(fi);;
	if ( rv->rFlags&SPECIFIED_NORMAL || (rv->rFlags&NORCT_MASK)==0x1 )
	    normal = rv->rn.getNormal();
	else if ( (rv->rFlags&NORCT_MASK) && smGroup )
	{
	    int numNormals = rv->rFlags & NORCT_MASK;
	    for ( int i=0; i<numNormals; ++i )
	    {
	        if ( rv->ern[i].getSmGroup()&smGroup )
	            normal = rv->ern[i].getNormal();
	    }
	}
	return convertPoint(normal);
}

void ExportImplementor::createMultiMeshes( osg::Geode* geode, Mesh* mesh, Mtl* mtl, Mtl* originMtl,
                                           int numSubMeshes, int i1, int i2, int i3 )
{
    int numGeometries = numSubMeshes>0 ? numSubMeshes : 1;
    std::vector< osg::ref_ptr<osg::DrawArrays> > daList(numGeometries);
    std::vector< osg::ref_ptr<osg::Vec3Array> > vaList(numGeometries);
    std::vector< osg::ref_ptr<osg::Vec3Array> > naList(numGeometries);
    std::vector< osg::ref_ptr<osg::Vec4Array> > caList(numGeometries);
    
    struct TexCoordAttribute
    {
        TexCoordAttribute( unsigned int u, osg::Vec2Array* ta, osg::Texture* tex )
        : _unit(u), _texcoords(ta), _texture(tex) {}
        
        unsigned int _unit;
        osg::ref_ptr<osg::Vec2Array> _texcoords;
        osg::Texture* _texture;
    };
    std::vector< std::vector<TexCoordAttribute> > taList(numGeometries);
    
    // Create geoemtries and associated arrays
    for ( int i=0; i<numGeometries; ++i )
    {
        daList[i] = new osg::DrawArrays(GL_TRIANGLES, 0, 0);
        vaList[i] = new osg::Vec3Array;
        naList[i] = new osg::Vec3Array;
        
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->addPrimitiveSet( daList[i].get() );
        geom->setVertexArray( vaList[i].get() );
        geom->setNormalArray( naList[i].get() );
        geom->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
        
        if ( mesh->vcFace )
        {
            caList[i] = new osg::Vec4Array;
            geom->setColorArray( caList[i].get() );
            geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
        }
        
        // Retrieve state set corresponding to the meterial
        osg::StateSet* ss = NULL;
        if ( numSubMeshes>0 )
        {
            if ( mtl && mtl->NumSubMtls()>i )
                ss = getStateSet( mtl->GetSubMtl(i) );
            
            // If multi-meterial is baked using methods like lighting-map, it will result in a shell-material
            // with two multi-meterials, and the rendering image will be mixed by them both. In that case,
            // we have to find the state set of the original sub-material and merged it with the baked one.
            if ( originMtl && originMtl->NumSubMtls()>i )
            {
                osg::StateSet* ssToMerge = getStateSet( originMtl->GetSubMtl(i) );
                if ( ssToMerge ) ss->merge( *ssToMerge );
            }
        }
        else
            ss = getStateSet(mtl);
        
        // Set stateset and texture coordinate arrays
        if ( ss )
        {
            osg::StateSet::TextureAttributeList& tattrs = ss->getTextureAttributeList();
            for ( unsigned int u=0; u<tattrs.size(); ++u )
            {
                osg::StateSet::AttributeList::iterator itr =
                    tattrs[u].find( osg::StateAttribute::TypeMemberPair(osg::StateAttribute::TEXTURE, 0) );
                if ( itr!=tattrs[u].end() )
                {
                    osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
                    geom->setTexCoordArray( u, ta.get() );
                    
                    osg::StateAttribute* sa = itr->second.first.get();
                    taList[i].push_back( TexCoordAttribute(u, ta, static_cast<osg::Texture*>(sa)) );
                }
            }
            geom->setStateSet(ss);
        }
        
        // Add to geode
        geode->addDrawable( geom.get() );
    }
    
    // Apply vertices, normals, colors and texcoords
    for ( int j=0; j<mesh->getNumFaces(); ++j )
    {
        Face& face = mesh->faces[j];
        int index = face.getMatID() % numGeometries;
        
        // Vertices
        Point3& v1 = mesh->verts[ face.v[i1] ]; vaList[index]->push_back( convertPoint(v1) );
        Point3& v2 = mesh->verts[ face.v[i2] ]; vaList[index]->push_back( convertPoint(v2) );
        Point3& v3 = mesh->verts[ face.v[i3] ]; vaList[index]->push_back( convertPoint(v3) );
        
        // Normals
        naList[index]->push_back( createVertexNormal(mesh, j, i1) );
        naList[index]->push_back( createVertexNormal(mesh, j, i2) );
        naList[index]->push_back( createVertexNormal(mesh, j, i3) );
        
        // Colors
        if ( mesh->vcFace )
        {
            Point3& c1 = mesh->vertCol[ mesh->vcFace[j].t[i1] ]; caList[index]->push_back( convertPoint4(c1) );
            Point3& c2 = mesh->vertCol[ mesh->vcFace[j].t[i2] ]; caList[index]->push_back( convertPoint4(c2) );
            Point3& c3 = mesh->vertCol[ mesh->vcFace[j].t[i3] ]; caList[index]->push_back( convertPoint4(c3) );
        }
        
        // Texture coordinates
        for ( std::vector<TexCoordAttribute>::iterator itr=taList[index].begin(); itr!=taList[index].end(); ++itr )
        {
            // Check if there is UV map (channel 0 is the vertex colors)
            if ( mesh->numMaps-1<=(int)itr->_unit ) continue;
            
            // Retrieve the map channel
            osg::Vec2Array* ta = itr->_texcoords.get();
            MeshMap& meshMap = mesh->maps[itr->_unit+1];
            if ( !ta || !meshMap.vnum || !meshMap.tf ) continue;
            
            UVVert uv1 = meshMap.tv[ meshMap.tf[j].t[i1] ]; ta->push_back( convertPoint2(uv1) );
            UVVert uv2 = meshMap.tv[ meshMap.tf[j].t[i2] ]; ta->push_back( convertPoint2(uv2) );
            UVVert uv3 = meshMap.tv[ meshMap.tf[j].t[i3] ]; ta->push_back( convertPoint2(uv3) );
            
            if ( !itr->_texture ) continue;
            if ( uv1.x>1.0f || uv1.x<0.0f || uv2.x>1.0f || uv2.x<0.0f || uv3.x>1.0f || uv3.x<0.0f )
                itr->_texture->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT );
            if ( uv1.y>1.0f || uv1.y<0.0f || uv2.y>1.0f || uv2.y<0.0f || uv3.y>1.0f || uv3.y<0.0f )
                itr->_texture->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT );
        }
    }
    
    // Check geometry data again and update the primitive sets
    for ( int i=0; i<numGeometries; ++i )
    {
        unsigned int vertexSize = vaList[i]->size();
        daList[i]->setCount( vertexSize );
        if ( naList[i] && naList[i]->size()!=vertexSize ) naList[i]->resize( vertexSize );
        if ( caList[i] && caList[i]->size()!=vertexSize ) caList[i]->resize( vertexSize );
        for ( std::vector<TexCoordAttribute>::iterator itr=taList[i].begin(); itr!=taList[i].end(); ++itr )
        {
            osg::Vec2Array* ta = itr->_texcoords.get();
            if ( ta && ta->size()!=vertexSize ) ta->resize( vertexSize );
        }
    }
    
    typedef osgUtil::Optimizer OPT;
    osgUtil::Optimizer optimizer;
    optimizer.optimize( geode,
                        OPT::MERGE_GEOMETRY | OPT::CHECK_GEOMETRY | OPT::MAKE_FAST_GEOMETRY |
                        OPT::TRISTRIP_GEOMETRY | OPT::TESSELLATE_GEOMETRY /*| OPT::INDEX_MESH*/ );
}

osg::MatrixTransform* ExportImplementor::exportGeomObject( INode* maxNode, Object* obj )
{
    osg::MatrixTransform* mt = exportTransform(maxNode);
    osg::Group* parentOfGeode = mt;
    
    // Add an offset transform node if necessary
    osg::Matrix objectMatrix = getObjectMatrix(maxNode, 0);
    if ( !objectMatrix.isIdentity() )
    {
        osg::ref_ptr<osg::MatrixTransform> mtOffset = new osg::MatrixTransform;
        mtOffset->setName( mt->getName() + "-OFFSET" );
        
        mt->addChild( mtOffset.get() );
        parentOfGeode = mtOffset.get();
    }
    
    // Create the geode
    osg::ref_ptr<osg::Geode> geode = getGeode(obj);
    if ( !geode )
    {
        // Calculate meshes of this geometry object
        int numSubMeshes = 0;
        Mtl* mtl = maxNode->GetMtl(), *originMtl = NULL;
        if ( mtl )
        {
            if ( mtl->ClassID()==Class_ID(BAKE_SHELL_CLASS_ID, 0) )
            {
                originMtl = mtl->GetSubMtl(0);
                mtl = mtl->GetSubMtl(1);
            }
            if ( mtl && mtl->ClassID()==Class_ID(MULTI_CLASS_ID, 0) )
                numSubMeshes = mtl->NumSubMtls();
        }
        
        // Calculate order of vertices on a face
        int i1 = 0, i2 = 1, i3 = 2;
        Matrix3 tm = maxNode->GetObjTMAfterWSM(0);
        if ( DotProd(CrossProd(tm.GetRow(0),tm.GetRow(1)),tm.GetRow(2))<0.0 )
        {
            i1 = 2; i2 = 1; i3 = 0;
        }
        
        // Obtain the triangle object
        BOOL needToDeleteTriangle = FALSE;
        if ( !obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)) )
            return NULL;
        TriObject* tri = (TriObject*)obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
        if ( obj!=tri ) needToDeleteTriangle = TRUE;
        
        Mesh* mesh = &tri->GetMesh();
        mesh->checkNormals( TRUE );
        
        // Add meshes to a newly created geode
        geode = new osg::Geode;
        geode->setName( mt->getName() + " - GEODE" );
        if ( mesh->numVerts>0 && mesh->faces )
            createMultiMeshes( geode.get(), mesh, mtl, originMtl, numSubMeshes, i1, i2, i3 );
        
        if ( needToDeleteTriangle ) delete tri;
        _geodeMap[obj] = geode.get();
    }
    parentOfGeode->addChild( geode.get() );
    return mt;
}
