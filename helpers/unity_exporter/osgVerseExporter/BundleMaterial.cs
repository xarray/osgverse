using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleMaterial : BundleResource
    {
        private BundleMaterial(Material material)
        {
            this.unityMaterial = material;
            this.index = allMaterials.Count;
            this.name = material.name;
            allMaterials[material] = this;
            shader = BundleShader.RegisterShader(material.shader);

            for (int i = 0; i < ShaderUtil.GetPropertyCount(material.shader); ++i)
            {
                // Find and record all textures used by current shader
                if (ShaderUtil.GetPropertyType(material.shader, i) == ShaderUtil.ShaderPropertyType.TexEnv)
                {
                    string propName = ShaderUtil.GetPropertyName(material.shader, i);
                    Texture subtexture = material.GetTexture(propName);
                    if (subtexture == null)
                    {
                        //Debug.LogWarning("Material " + material.name + " can't load texture for " + propName);
                        continue;
                    }

                    textures.Add(BundleTexture.RegisterTexture(subtexture, propName));
                    textureUnits.Add(BundleTexture.GetSuggestedUnit(propName));
                    textureScales.Add(material.GetTextureScale(propName));
                    textureOffsets.Add(material.GetTextureOffset(propName));
                }
            }

            // If no shader textures used, try to record the main texture
            if (textures.Count == 0 && material.mainTexture != null)
            {
                textures.Add(BundleTexture.RegisterTexture(material.mainTexture, "_MainTex"));
                textureUnits.Add(BundleTexture.GetSuggestedUnit("_MainTex"));
                textureScales.Add(unityMaterial.mainTextureScale);
                textureOffsets.Add(unityMaterial.mainTextureOffset);
            }
        }

        void preprocess()
        {
            //Debug.Log("preprocess - " + unityMaterial);
        }

        void process()
        {
            //Debug.Log("process - " + unityMaterial);
        }

        void postprocess()
        {
            //Debug.Log("postprocess - " + unityMaterial);
        }

        new public static void Preprocess()
        {
            foreach (var material in allMaterials.Values)
            {
                material.preprocess();
            }
        }

        new public static void Process()
        {
            foreach (var material in allMaterials.Values)
            {
                material.process();
            }
        }

        new public static void PostProcess()
        {
            foreach (var material in allMaterials.Values)
            {
                material.postprocess();
            }
        }

        new public static void Reset()
        {
            allMaterials = new Dictionary<Material, BundleMaterial>();
        }

        public static BundleMaterial RegisterMaterial(Material material)
        {
            // we're trying to go off sharedMaterials, but we're getting
            // "instance" materials or something between meshes that share a material
            // so go off name for now
            foreach (Material m in allMaterials.Keys)
            {
                if (m.name == material.name)
                    return allMaterials[m];
            }
            return new BundleMaterial(material);
        }

        public new SceneMaterial GetObjectData()
        {
            var sceneData = new SceneMaterial();
            sceneData.name = name;
            sceneData.shader = shader.name;
            sceneData.passCount = unityMaterial.passCount;
            sceneData.renderQueue = unityMaterial.renderQueue;

            sceneData.combinedTextures = new Dictionary<int, SceneTexture>();
            if (textures.Count > 0)
            {
                // Make combined data first
                for (int i = 0; i < textures.Count;)
                {
                    if (textures[i].combinable)
                    {
                        sceneData.combinedTextures[textures[i].uniqueID] =
                            textures[i].GetObjectData();
                        textures.RemoveAt(i);
                        textureUnits.RemoveAt(i);
                        textureOffsets.RemoveAt(i);
                    }
                    else i++;
                }

                // Save all other textures
                sceneData.textureIDs = new int[textures.Count];
                for (int i = 0; i < textures.Count; i++)
                    sceneData.textureIDs[i] = textures[i].uniqueID;

                sceneData.textureUnits = new int[textureUnits.Count];
                for (int i = 0; i < textureUnits.Count; i++)
                    sceneData.textureUnits[i] = textureUnits[i];

                sceneData.textureTilingOffsets = new Vector4[textureUnits.Count];
                for (int i = 0; i < textureUnits.Count; i++)
                {
                    Vector2 sc = textureScales[i], of = textureOffsets[i];
                    sceneData.textureTilingOffsets[i] = new Vector4(sc.x, sc.y, of.x, of.y);
                }
            }

            if (unityMaterial.shaderKeywords.Length > 0)
            {
                sceneData.shaderKeywords = new string[unityMaterial.shaderKeywords.Length];
                for (int i = 0; i < unityMaterial.shaderKeywords.Length; i++)
                    sceneData.shaderKeywords[i] = unityMaterial.shaderKeywords[i];
            }
            //sceneData.color = unityMaterial.color;
            return sceneData;
        }

        public static List<SceneMaterial> GenerateObjectList()
        {
            List<SceneMaterial> materials = new List<SceneMaterial>();
            foreach (var material in allMaterials.Values)
                materials.Add(material.GetObjectData());
            return materials;
        }

        public static Dictionary<Material, BundleMaterial> allMaterials;
        public Material unityMaterial;
        public int index;

        public List<BundleTexture> textures = new List<BundleTexture>();
        public List<int> textureUnits = new List<int>();
        public List<Vector2> textureScales = new List<Vector2>();
        public List<Vector2> textureOffsets = new List<Vector2>();
        public BundleShader shader;
    }

}
#endif