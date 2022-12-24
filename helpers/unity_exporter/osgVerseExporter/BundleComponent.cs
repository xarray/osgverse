using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleComponent : BundleObject
    {
        virtual public void QueryResources()
        {
        }

        virtual public void Preprocess()
        {
        }

        virtual public void Process()
        {
        }

        virtual public void PostProcess()
        {
        }

        public virtual new SceneComponent GetObjectData()
        {
            throw new NotImplementedException("Attempting to call GetObjectData");
        }

        public static void Reset()
        {
            BundleTransform.Reset();
            BundleMeshRenderer.Reset();
            conversions = new Dictionary<Type, Type>();
        }

        public static void QueryComponents(BundleGameObject jgo)
        {
            // for every registered conversion get that component
            bool isQueried = false;
            foreach (KeyValuePair<Type, Type> pair in conversions)
            {
                Component[] components = jgo.unityGameObject.GetComponents(pair.Key);
                if (components.Length > 0) isQueried = true;

                foreach (Component component in components)
                {
                    MeshRenderer meshRenderer = component as MeshRenderer;
                    if (meshRenderer != null && !meshRenderer.enabled) continue;

                    var jcomponent = Activator.CreateInstance(pair.Value) as BundleComponent;
                    if (jcomponent == null)
                        ExportError.FatalError("Export component creation failed");

                    jcomponent.unityComponent = component;
                    jcomponent.jeGameObject = jgo;
                    jgo.AddComponent(jcomponent);
                }
            }

            if (!isQueried)
                Debug.LogWarning("Unregistered game object " + jgo.unityGameObject.name);
        }

        public static void RegisterConversion(Type componentType, Type exportType)
        {
            conversions[componentType] = exportType;
        }

        public static void RegisterStandardComponents()
        {
            RegisterConversion(typeof(Transform), typeof(BundleTransform));
            RegisterConversion(typeof(Camera), typeof(BundleCamera));
            RegisterConversion(typeof(MeshRenderer), typeof(BundleMeshRenderer));
            RegisterConversion(typeof(SkinnedMeshRenderer), typeof(BundleSkinnedMeshRenderer));
            RegisterConversion(typeof(Terrain), typeof(BundleTerrain));
            RegisterConversion(typeof(AnimationHelper), typeof(BundleAnimation));
            RegisterConversion(typeof(BoxCollider), typeof(BundleBoxCollider));
            RegisterConversion(typeof(MeshCollider), typeof(BundleMeshCollider));
            RegisterConversion(typeof(Rigidbody), typeof(BundleRigidBody));
            RegisterConversion(typeof(Light), typeof(BundleLight));
            RegisterConversion(typeof(ParticleSystem), typeof(BundleParticleSystem));
            RegisterConversion(typeof(TimeOfDay), typeof(BundleTimeOfDay));
        }

        static Dictionary<Type, Type> conversions;
        public Component unityComponent;
        public BundleGameObject jeGameObject;
    }

}
#endif