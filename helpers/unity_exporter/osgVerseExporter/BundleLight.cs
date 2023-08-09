using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleLight : BundleComponent
    {
        override public void Preprocess()
        {
            unityLight = unityComponent as Light;
        }

        override public void QueryResources()
        {
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneLight();
            sceneData.type = "Light";
            sceneData.lightType = "Point";
            sceneData.color = unityLight.color;
            sceneData.range = unityLight.range;
            sceneData.castsShadows = (unityLight.shadows != LightShadows.None);
            sceneData.realtime = true;

            SerializedObject serial = new SerializedObject(unityLight);
            SerializedProperty lightmapProp = serial.FindProperty("m_Lightmapping");
            if (lightmapProp.intValue != 0)
            {
                // not a realtime light
                sceneData.realtime = false;
            }
            return sceneData;
        }

        Light unityLight;
    }

}
#endif