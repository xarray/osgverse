using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleTransform : BundleComponent
    {
        override public void Preprocess()
        {
            unityTransform = unityComponent as Transform;
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneTransform();
            sceneData.type = "Transform";
            if (unityTransform != null)
            {
                sceneData.localPosition = unityTransform.localPosition;
                sceneData.localRotation = unityTransform.localRotation;
                sceneData.localScale = unityTransform.localScale;
            }
            return sceneData;
        }

        Transform unityTransform;
    }

}
#endif