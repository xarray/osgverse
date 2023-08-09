using System;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleTexture : BundleResource
    {
        public static int GetSuggestedUnit(string propName)
        {
            //- 0 DiffuseMap: diffuse/albedo RGB texture of input scene
            //- 1 NormalMap: tangent-space normal texture of input scene
            //- 2 SpecularMap: specular RGB texture of input scene
            //- 3 ShininessMap: occlusion/roughness/metallic (RGB) texture of input scene
            //- 4 AmbientMap: ambient texture of input scene (FIXME: NOT USED)
            //- 5 EmissiveMap: emissive RGB texture of input scene
            //- 6 ReflectionMap: reflection RGB texture of input scene
            int suggestedUnit = -1;
            if (propName == "_MainTex") suggestedUnit = 0;
            else if (propName == "_BumpMap") suggestedUnit = 1;
            else if (propName == "_Specular") suggestedUnit = 2;
            else if (propName == "_MetallicGlossMap") suggestedUnit = 3;
            else if (propName == "_OcclusionMap") suggestedUnit = 3;
            else if (propName == "_LightMap") suggestedUnit = 5;
            else if (propName == "_EmissionMap") suggestedUnit = 5;
            else if (propName == "_Cube") suggestedUnit = 6;
            else Debug.LogWarning("Unassiagned texture property: " + propName);
            return suggestedUnit;
        }

        private BundleTexture(Texture texture, string prop)
        {
            this.unityTexture = texture as Texture2D;
            allTextures[texture] = this;
            propName = prop;
            name = prop + ": " + texture.name;

            combinable = false;
            if (propName.Contains("Metallic") || propName.Contains("Occlusion"))
                combinable = true;
        }

        void preprocess()
        {
            //Debug.Log("preprocess - " + unityTexture);
        }

        void process()
        {
            //Debug.Log("process - " + unityTexture);
            if (unityTexture != null)
            {
                path = AssetDatabase.GetAssetPath(unityTexture);
                uniqueID = unityTexture.GetInstanceID();

                if (!unityTexture.isReadable)
                {
                    TextureImporter textureImporter = AssetImporter.GetAtPath(path) as TextureImporter;
                    textureImporter.isReadable = true;
                    AssetDatabase.ImportAsset(path);
                }
            }
            
            if (unityTexture != null && unityTexture.isReadable)
            {
                var bytes = unityTexture.GetRawTextureData();
                w = unityTexture.width; h = unityTexture.height;
                format = (int)unityTexture.format;
                base64Length = bytes.Length;
                base64 = System.Convert.ToBase64String(bytes, 0, bytes.Length);
            }
            else
                base64Length = 0;
        }

        void postprocess()
        {
            //Debug.Log("postprocess - " + unityTexture);
        }

        public static BundleTexture RegisterTexture(Texture texture, string propName)
        {
            if (allTextures.ContainsKey(texture))
                return allTextures[texture];
            return new BundleTexture(texture, propName);
        }

        new public static void Preprocess()
        {
            foreach (var texture in allTextures.Values)
            {
                texture.preprocess();
            }
        }

        new public static void Process()
        {
            foreach (var texture in allTextures.Values)
            {
                texture.process();
            }
        }

        new public static void PostProcess()
        {
            foreach (var texture in allTextures.Values)
            {
                texture.postprocess();
            }
        }

        new public static void Reset()
        {
            allTextures = new Dictionary<Texture, BundleTexture>();
        }

        public new SceneTexture GetObjectData()
        {
            var sceneData = new SceneTexture();
            sceneData.uniqueID = uniqueID;
            sceneData.name = name;
            sceneData.propName = propName;
            sceneData.path = path;
            sceneData.w = w; sceneData.h = h;
            sceneData.format = format;
            sceneData.base64 = base64;
            sceneData.base64Length = base64Length;
            return sceneData;
        }

        public static List<SceneTexture> GenerateObjectList()
        {
            List<SceneTexture> textures = new List<SceneTexture>();
            foreach (var texture in allTextures.Values)
                textures.Add(texture.GetObjectData());
            return textures;
        }

        Texture2D unityTexture;
        public new string name;
        public string path, propName;
        public string base64;
        public int uniqueID, w, h, format;
        public int base64Length;
        public bool combinable;
        public static Dictionary<Texture, BundleTexture> allTextures;
    }


}
#endif