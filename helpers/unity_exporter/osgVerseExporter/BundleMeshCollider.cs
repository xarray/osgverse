using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleMeshCollider : BundleComponent
    {
        override public void Preprocess()
        {
            unityMeshCollider = unityComponent as MeshCollider;
        }

        override public void QueryResources()
        {
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneMeshCollider();
            sceneData.type = "MeshCollider";
            if (unityMeshCollider != null)
            {
                Mesh unityMesh = unityMeshCollider.sharedMesh;
                if (unityMesh != null)
                {
                    sceneData.mesh = new SceneMesh();
                    sceneData.mesh.name = unityMesh.name;

                    // submeshes
                    sceneData.mesh.subMeshCount = unityMesh.subMeshCount;
                    sceneData.mesh.triangles = new int[unityMesh.subMeshCount][];
                    for (int i = 0; i < unityMesh.subMeshCount; i++)
                    {
                        sceneData.mesh.triangles[i] = unityMesh.GetTriangles(i);
                    }

                    // Vertices
                    sceneData.mesh.vertexCount = unityMesh.vertexCount;
                    sceneData.mesh.vertexPositions = unityMesh.vertices;
                    sceneData.mesh.vertexUV = unityMesh.uv;
                    sceneData.mesh.vertexUV2 = unityMesh.uv2;
                    sceneData.mesh.vertexColors = unityMesh.colors;
                    sceneData.mesh.vertexNormals = unityMesh.normals;
                    sceneData.mesh.vertexTangents = unityMesh.tangents;
                }
            }
            return sceneData;
        }

        MeshCollider unityMeshCollider;
    }

}
#endif