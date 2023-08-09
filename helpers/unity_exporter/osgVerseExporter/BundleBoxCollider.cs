using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleBoxCollider : BundleComponent
    {
        override public void Preprocess()
        {
            unityBoxCollider = unityComponent as BoxCollider;
        }

        override public void QueryResources()
        {
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneBoxCollider();
            sceneData.type = "BoxCollider";
            sceneData.size = unityBoxCollider.size;
            sceneData.center = unityBoxCollider.center;
            return sceneData;
        }

        BoxCollider unityBoxCollider;
    }

}
#endif