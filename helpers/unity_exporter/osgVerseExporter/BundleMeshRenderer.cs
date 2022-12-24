using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleMeshRenderer : BundleComponent
    {
        override public void Preprocess()
        {
            unityMeshRenderer = unityComponent as MeshRenderer;
            unityMeshFilter = jeGameObject.unityGameObject.GetComponent<MeshFilter>();
            if (unityMeshFilter == null)
            {
                ExportError.FatalError("MeshRenderer with no MeshFilter");
            }
        }

        override public void QueryResources()
        {
            mesh = BundleMesh.RegisterMesh(unityMeshFilter.sharedMesh);
            for (int i = 0; i < unityMeshRenderer.sharedMaterials.Length; i++)
            {
                materials.Add(BundleMaterial.RegisterMaterial(unityMeshRenderer.sharedMaterials[i]));
            }
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneMeshRenderer();
            sceneData.type = "MeshRenderer";
            sceneData.mesh = mesh.name;
            sceneData.enabled = unityMeshRenderer.enabled;
            sceneData.castShadows = (unityMeshRenderer.shadowCastingMode != UnityEngine.Rendering.ShadowCastingMode.Off);
            sceneData.receiveShadows = unityMeshRenderer.receiveShadows;
            sceneData.lightmapIndex = unityMeshRenderer.lightmapIndex;
            sceneData.lightmapTilingOffset = unityMeshRenderer.lightmapScaleOffset;

            sceneData.materials = new string[materials.Count];
            for (int i = 0; i < materials.Count; i++)
            {
                sceneData.materials[i] = materials[i].name;
            }
            return sceneData;
        }

        public BundleMesh mesh;
        public List<BundleMaterial> materials = new List<BundleMaterial>();
        MeshRenderer unityMeshRenderer;
        MeshFilter unityMeshFilter;
    }

}
#endif