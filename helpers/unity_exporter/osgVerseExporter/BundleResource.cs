using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleResource : BundleObject
    {
        public static void Reset()
        {
            BundleMesh.Reset();
            BundleMaterial.Reset();
            BundleTexture.Reset();
            BundleShader.Reset();
            BundleLightmap.Reset();
        }

        public static void Preprocess()
        {
            BundleTexture.Preprocess();
            BundleShader.Preprocess();
            BundleMaterial.Preprocess();
            BundleLightmap.Preprocess();
            BundleMesh.Preprocess();
        }

        public static void Process()
        {
            BundleTexture.Process();
            BundleShader.Process();
            BundleMaterial.Process();
            BundleLightmap.Process();
            BundleMesh.Process();
        }

        public static void PostProcess()
        {
            BundleTexture.PostProcess();
            BundleShader.PostProcess();
            BundleMaterial.PostProcess();
            BundleLightmap.PostProcess();
            BundleMesh.PostProcess();
        }

        public static new SceneResources GetObjectData()
        {
            var sceneData = new SceneResources();
            sceneData.textures = BundleTexture.GenerateObjectList();
            sceneData.lightmaps = BundleLightmap.GenerateObjectList();
            sceneData.shaders = BundleShader.GenerateObjectList();
            sceneData.materials = BundleMaterial.GenerateObjectList();
            sceneData.meshes = BundleMesh.GenerateObjectList();
            return sceneData;
        }
    }

}
#endif