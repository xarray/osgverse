using System.Collections.Generic;
using UnityEngine;

namespace osgVerse
{

    public class BundleSkinnedMeshRenderer : BundleComponent
    {
        override public void Preprocess()
        {
            unityMeshRenderer = unityComponent as SkinnedMeshRenderer;
        }

        override public void QueryResources()
        {
            mesh = BundleMesh.RegisterMesh(unityMeshRenderer.sharedMesh);
            mesh.bones = unityMeshRenderer.bones;
            mesh.rootBone = (unityMeshRenderer.rootBone == null) ? "" : unityMeshRenderer.rootBone.gameObject.name;
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
            var sceneData = new SceneSkinnedMeshRenderer();
            sceneData.type = "SkinnedMeshRenderer";
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
        SkinnedMeshRenderer unityMeshRenderer;
    }

}
