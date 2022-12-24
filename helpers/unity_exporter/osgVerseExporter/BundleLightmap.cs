using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleLightmap : BundleResource
    {
        void preprocess()
        {
        }

        void process()
        {
        }

        void postprocess()
        {
        }

        // TODO: we also want to copy the exr data in case user wants it
        new public static void Preprocess()
        {
            LightmapData[] lightmaps = LightmapSettings.lightmaps;
            for (int i = 0; i < lightmaps.Length; i++)
            {
                var lightmap = lightmaps[i].lightmapColor;
                if (lightmap == null)
                    lightmap = lightmaps[i].lightmapDir;

                string path = AssetDatabase.GetAssetPath(lightmap);
                if (path == "")
                {
                    string currentScenePath = UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene().path;
                    string[] parts = currentScenePath.Split('/', '\\');
                    string sceneName = parts[parts.Length - 1].Split('.')[0];
                    string lightmapPath = Path.GetDirectoryName(currentScenePath) + "/" + sceneName + "/";
                    path = lightmapPath + "Lightmap-" + i + "_comp_light.exr";
                }

                /*
                TextureImporter textureImporter = AssetImporter.GetAtPath(path) as TextureImporter;
                TextureImporterSettings settings = new TextureImporterSettings();
                textureImporter.ReadTextureSettings( settings );

                bool setReadable = false;
                if ( !settings.readable ) setReadable = true;
                settings.readable = true;
                settings.lightmap = true;
                textureImporter.SetTextureSettings( settings );
                AssetDatabase.ImportAsset( path, ImportAssetOptions.ForceUpdate );

                var pixels = lightmap.GetPixels32();
                Texture2D ntexture = new Texture2D(lightmap.width, lightmap.height, TextureFormat.ARGB32, false);
                ntexture.SetPixels32( pixels );
                ntexture.Apply();

                var bytes = ntexture.EncodeToPNG();
                UnityEngine.Object.DestroyImmediate( ntexture );
                if ( setReadable ) settings.readable = false;

                settings.lightmap = true;
                textureImporter.SetTextureSettings(settings);
                AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceUpdate);
                */
                SceneTexture lm = new SceneTexture();
                lm.name = BundleScene.sceneName + "_Lightmap_" + i;
                lm.path = path;
                lm.uniqueID = lightmap.GetInstanceID();
                //lm.base64Length = bytes.Length;
                //lm.base64 =  System.Convert.ToBase64String(bytes, 0, bytes.Length);
                allLightmaps.Add(lm);
            }
        }

        new public static void Process()
        {
        }

        new public static void PostProcess()
        {
        }

        new public static void Reset()
        {
            allLightmaps = new List<SceneTexture>();
        }

        public new SceneTexture GetObjectData()
        {
            return null;
        }

        public static List<SceneTexture> GenerateObjectList()
        {
            List<SceneTexture> lightmaps = new List<SceneTexture>();
            for (int i = 0; i < allLightmaps.Count; i++)
            {
                SceneTexture lightmap = allLightmaps[i];
                SceneTexture jlightmap = new SceneTexture();
                jlightmap.name = lightmap.name;
                jlightmap.path = lightmap.path;
                jlightmap.uniqueID = lightmap.uniqueID;
                jlightmap.base64 = lightmap.base64;
                jlightmap.base64Length = lightmap.base64Length;
                lightmaps.Add(jlightmap);
            }
            return lightmaps;
        }

        static List<SceneTexture> allLightmaps = new List<SceneTexture>();
        public string filename;
        public string base64;
        public int base64Length;
    }

}
#endif