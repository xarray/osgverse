using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleGameObject : BundleObject
    {

        public void AddComponent(BundleComponent component)
        {
            components.Add(component);
        }

        public BundleGameObject(GameObject go, BundleGameObject parent)
        {
            allGameObjectLookup[go] = this;
            this.unityGameObject = go;
            this.parent = parent;
            this.name = go.name;

            if (parent != null)
                parent.children.Add(this);
            BundleComponent.QueryComponents(this);
        }

        // first pass is preprocess
        public void Preprocess()
        {
            foreach (var component in components)
                component.Preprocess();
            foreach (var child in children)
                child.Preprocess();
        }

        // next we query resources
        public void QueryResources()
        {
            foreach (var component in components)
                component.QueryResources();
            foreach (var child in children)
                child.QueryResources();
        }

        public void Process()
        {
            foreach (var component in components)
                component.Process();
            foreach (var child in children)
                child.Process();
        }

        public void PostProcess()
        {
            foreach (var component in components)
                component.PostProcess();
            foreach (var child in children)
                child.PostProcess();
        }

        public static void Reset()
        {
            allGameObjectLookup = new Dictionary<GameObject, BundleGameObject>();
        }

        public new SceneGameObject GetObjectData()
        {
            SceneGameObject sceneData = new SceneGameObject();
            sceneData.name = name;
            sceneData.children = new List<SceneGameObject>();
            foreach (var child in children)
                sceneData.children.Add(child.GetObjectData());

            sceneData.components = new List<SceneComponent>();
            foreach (var component in components)
                sceneData.components.Add(component.GetObjectData());
            return sceneData;
        }

        public BundleTransform transform;
        public BundleGameObject parent;
        public List<BundleGameObject> children = new List<BundleGameObject>();
        public List<BundleComponent> components = new List<BundleComponent>();
        public GameObject unityGameObject;
        public static Dictionary<GameObject, BundleGameObject> allGameObjectLookup;
    }

}
#endif