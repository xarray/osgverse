using System;
using System.Collections.Generic;
using UnityEngine;

namespace osgVerse
{

    public class BundleShader : BundleResource
    {
        private BundleShader(Shader shader)
        {
            this.unityShader = shader;
            this.name = shader.name;
            allShaders[shader] = this;
        }

        public static BundleShader RegisterShader(Shader shader)
        {
            if (allShaders.ContainsKey(shader))
                return allShaders[shader];
            return new BundleShader(shader);
        }

        void preprocess()
        {
            //Debug.Log("preprocess - " + unityShader);
        }

        void process()
        {
            //Debug.Log("process - " + unityShader);
        }

        void postprocess()
        {
            //Debug.Log("postprocess - " + unityShader);
        }

        new public static void Preprocess()
        {
            foreach (var shader in allShaders.Values)
            {
                shader.preprocess();
            }
        }

        new public static void Process()
        {
            foreach (var shader in allShaders.Values)
            {
                shader.process();
            }
        }

        new public static void PostProcess()
        {
            foreach (var shader in allShaders.Values)
            {
                shader.postprocess();
            }
        }

        new public static void Reset()
        {
            allShaders = new Dictionary<Shader, BundleShader>();
        }

        public new SceneShader GetObjectData()
        {
            var sceneData = new SceneShader();
            sceneData.name = name;
            sceneData.renderQueue = unityShader.renderQueue;
            return sceneData;
        }

        public static List<SceneShader> GenerateObjectList()
        {
            List<SceneShader> shaders = new List<SceneShader>();
            foreach (var shader in allShaders.Values)
                shaders.Add(shader.GetObjectData());
            return shaders;
        }

        public static Dictionary<Shader, BundleShader> allShaders;
        Shader unityShader;
    }


}
