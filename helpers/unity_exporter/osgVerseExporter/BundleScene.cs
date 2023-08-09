using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleScene : BundleObject
    {
        public static BundleScene TraverseScene(bool onlySelected)
        {
            var scene = new BundleScene();
            List<GameObject> root = new List<GameObject>();

            if (onlySelected)
            {
                object[] objects = Selection.objects;
                foreach (object o in objects)
                {
                    GameObject go = o as GameObject;
                    if (go != null) root.Add(go);
                }
            }
            else
            {
                // Unity has no root object, so collect root game objects this way
                object[] objects = GameObject.FindObjectsOfType(typeof(GameObject));
                foreach (object o in objects)
                {
                    GameObject go = (GameObject)o;
                    if (go.transform.parent == null) root.Add(go);
                }
            }

            if (root.Count == 0)
            {
                ExportError.FatalError("Cannot Export Empty Scene");
            }

            // traverse the "root" game objects, collecting child game objects and components
            Debug.Log(root.Count + " root game objects to export");
            foreach (var go in root)
            {
                scene.rootGameObjects.Add(Traverse(go));
            }
            return scene;
        }

        public void Preprocess()
        {
            foreach (var jgo in rootGameObjects)
                jgo.Preprocess();
            foreach (var jgo in rootGameObjects)
                jgo.QueryResources();  // discover resources
            BundleResource.Preprocess();
        }

        public void Process()
        {
            BundleResource.Process();
            foreach (var jgo in rootGameObjects)
                jgo.Process();
        }

        public void PostProcess()
        {
            BundleResource.PostProcess();
            foreach (var jgo in rootGameObjects)
                jgo.PostProcess();
        }

        public static void Reset()
        {
        }

        static BundleGameObject Traverse(GameObject obj, BundleGameObject jparent = null)
        {
            BundleGameObject jgo = new BundleGameObject(obj, jparent);
            foreach (Transform child in obj.transform)
            {
                Traverse(child.gameObject, jgo);
            }
            return jgo;
        }

        public SceneData GetSceneData()
        {
            var sceneData = new SceneData();
            sceneData.name = sceneName;
            sceneData.resources = BundleResource.GetObjectData();
            sceneData.hierarchy = new List<SceneGameObject>();

            foreach (var go in rootGameObjects)
                sceneData.hierarchy.Add(go.GetObjectData());
            return sceneData;
        }

        public static string sceneName;
        private List<BundleGameObject> rootGameObjects = new List<BundleGameObject>();
    }

}
#endif