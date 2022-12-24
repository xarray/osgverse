using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleMesh : BundleResource
    {
        private BundleMesh(Mesh mesh)
        {
            unityMesh = mesh;
            index = allMeshes.Count;
            allMeshes[mesh] = this;

            // we can have duplicates with same name, referenced by index
            //#name = mesh.name;
            string path = AssetDatabase.GetAssetPath(mesh);
            path = Path.GetFileNameWithoutExtension(path);
            name = path + "_" + mesh.name;

            // TODO: handle bones
        }

        void preprocess()
        {
        }

        void process()
        {
        }

        void postprocess()
        {
        }

        new public static void Preprocess()
        {
            foreach (var mesh in allMeshes.Values)
            {
                mesh.preprocess();
            }
        }

        new public static void Process()
        {
            foreach (var mesh in allMeshes.Values)
            {
                mesh.process();
            }
        }

        new public static void PostProcess()
        {
            foreach (var mesh in allMeshes.Values)
            {
                mesh.postprocess();
            }
        }

        public static BundleMesh RegisterMesh(Mesh mesh)
        {
            if (allMeshes.ContainsKey(mesh))
                return allMeshes[mesh];
            return new BundleMesh(mesh);
        }

        new public static void Reset()
        {
        }

        public new SceneMesh GetObjectData()
        {
            var sceneData = new SceneMesh();
            sceneData.name = name;

            // submeshes
            sceneData.subMeshCount = unityMesh.subMeshCount;
            sceneData.triangles = new int[unityMesh.subMeshCount][];
            for (int i = 0; i < unityMesh.subMeshCount; i++)
            {
                sceneData.triangles[i] = unityMesh.GetTriangles(i);
            }

            // Vertices
            sceneData.vertexCount = unityMesh.vertexCount;
            sceneData.vertexPositions = unityMesh.vertices;
            sceneData.vertexUV = unityMesh.uv;
            sceneData.vertexUV2 = unityMesh.uv2;
            sceneData.vertexColors = unityMesh.colors;
            sceneData.vertexNormals = unityMesh.normals;
            sceneData.vertexTangents = unityMesh.tangents;
            sceneData.bindPoses = unityMesh.bindposes;
            sceneData.boneWeights = new SceneBoneWeight[unityMesh.boneWeights.Length];

            sceneData.rootBone = rootBone;
            for (int i = 0; i < unityMesh.boneWeights.Length; i++)
            {
                var bw = unityMesh.boneWeights[i];
                var jbw = new SceneBoneWeight();
                jbw.indexes[0] = bw.boneIndex0;
                jbw.indexes[1] = bw.boneIndex1;
                jbw.indexes[2] = bw.boneIndex2;
                jbw.indexes[3] = bw.boneIndex3;
                jbw.weights[0] = bw.weight0;
                jbw.weights[1] = bw.weight1;
                jbw.weights[2] = bw.weight2;
                jbw.weights[3] = bw.weight3;
                sceneData.boneWeights[i] = jbw;
            }

            if (bones != null)
            {
                SceneTransform[] jbones = new SceneTransform[bones.Length];
                for (int i = 0; i < bones.Length; i++)
                {
                    SceneTransform jt = new SceneTransform();
                    jt.localPosition = bones[i].localPosition;
                    jt.localRotation = bones[i].localRotation;
                    jt.localScale = bones[i].localScale;
                    jt.name = bones[i].gameObject.name;
                    jt.parentName = (bones[i].parent == null) ? "" : bones[i].parent.gameObject.name;
                    jt.type = "Bone";
                    jbones[i] = jt;
                }
                sceneData.bones = jbones;
            }
            return sceneData;
        }

        public static List<SceneMesh> GenerateObjectList()
        {
            List<SceneMesh> meshes = new List<SceneMesh>();
            foreach (var mesh in allMeshes.Values)
                meshes.Add(mesh.GetObjectData());
            return meshes;
        }

        public static Dictionary<Mesh, BundleMesh> allMeshes = new Dictionary<Mesh, BundleMesh>();
        public Mesh unityMesh;
        public int index;
        public BoneWeight[] boneWeights;

        // set by skinned renderer if any
        public Transform[] bones;
        public String rootBone = "";
    }

}
#endif