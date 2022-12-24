using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleKeyframe
    {
        public Vector3 pos = new Vector3(0, 0, 0);
        public Vector3 scale = new Vector3(1, 1, 1);
        public Quaternion rot = Quaternion.identity;
        public float time = 0.0f;
    }

    public class BundleAnimationClip
    {
        public string name;
        public Dictionary<string, List<BundleKeyframe>> keyframes = new Dictionary<string, List<BundleKeyframe>>();
    }

    public class BundleAnimation : BundleComponent
    {
        override public void Preprocess()
        {
            // m_LocalScale.x - z
            // m_LocalPosition.x - z
            // m_LocalRotation.x - w

            // for now copy the keys, we could evaluate and maybe should
            // as Unity keys have in/out tangents
            unityAnim = unityComponent as AnimationHelper;

            // no way to get clips like this, going to need a custom component
            // for now, support one animation
            //for (int i = 0; i < unityAnim.GetClipCount(); i++)
            //{
            //    var clip = unityAnim.clips[i];
            //}

            for (int i = 0; i < unityAnim.animationClips.Length; i++)
            {
                AddClip(unityAnim.animationClips[i]);
            }
        }

        void AddClip(AnimationClip clip)
        {
            BundleAnimationClip aclip = new BundleAnimationClip();
            aclip.name = clip.name;
            clips.Add(aclip);

            AnimationClipCurveData[] curveData = AnimationUtility.GetAllCurves(clip, true);
            Dictionary<string, List<AnimationClipCurveData>> animdata = new Dictionary<string, List<AnimationClipCurveData>>();
            for (int i = 0; i < curveData.Length; i++)
            {
                AnimationClipCurveData cd = curveData[i];
                List<AnimationClipCurveData> nodedata;
                if (!animdata.TryGetValue(cd.path, out nodedata))
                    nodedata = animdata[cd.path] = new List<AnimationClipCurveData>();
                nodedata.Add(cd);
            }

            foreach (KeyValuePair<string, List<AnimationClipCurveData>> entry in animdata)
            {
                var boneName = entry.Key;
                var keyframes = aclip.keyframes[boneName] = new List<BundleKeyframe>();

                float maxTime = 0;
                foreach (AnimationClipCurveData cd in entry.Value)
                {
                    var curve = cd.curve;
                    if (curve.keys.Length == 0)
                        continue;
                    if (curve.keys[curve.keys.Length - 1].time > maxTime)
                        maxTime = curve.keys[curve.keys.Length - 1].time;
                }

                Vector3 pos = new Vector3(0, 0, 0);
                Vector3 scale = new Vector3(1, 1, 1);
                Quaternion rot = Quaternion.identity;

                Vector3 lastpos = new Vector3(0, 0, 0);
                Vector3 lastscale = new Vector3(1, 1, 1);
                Quaternion lastrot = Quaternion.identity;

                for (float time = 0.0f; time <= maxTime;)
                {
                    foreach (AnimationClipCurveData cd in entry.Value)
                    {
                        var curve = cd.curve;
                        float value = curve.Evaluate(time);
                        if (cd.propertyName == "m_LocalScale.x")
                            scale.x = value;
                        else if (cd.propertyName == "m_LocalScale.y")
                            scale.y = value;
                        else if (cd.propertyName == "m_LocalScale.z")
                            scale.z = value;
                        else if (cd.propertyName == "m_LocalPosition.x")
                            pos.x = value;
                        else if (cd.propertyName == "m_LocalPosition.y")
                            pos.y = value;
                        else if (cd.propertyName == "m_LocalPosition.z")
                            pos.z = value;
                        else if (cd.propertyName == "m_LocalRotation.x")
                            rot.x = value;
                        else if (cd.propertyName == "m_LocalRotation.y")
                            rot.y = value;
                        else if (cd.propertyName == "m_LocalRotation.z")
                            rot.z = value;
                        else if (cd.propertyName == "m_LocalRotation.w")
                            rot.w = value;
                        else
                            Debug.Log("Unknown Animtation: " + cd.propertyName);
                    }

                    bool needFrame = false;
                    float t = Math.Abs(lastpos.x - pos.x);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastpos.y - pos.y);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastpos.z - pos.z);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastscale.x - scale.x);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastscale.y - scale.y);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastscale.z - scale.z);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastrot.x - rot.x);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastrot.y - rot.y);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastrot.z - rot.z);
                    if (t < 0.01f) needFrame = true;
                    t = Math.Abs(lastrot.w - rot.w);
                    if (t < 0.01f) needFrame = true;

                    if (needFrame)
                    {
                        lastpos = pos;
                        lastrot = rot;
                        lastscale = scale;

                        BundleKeyframe keyframe = new BundleKeyframe();
                        keyframe.pos = pos;
                        keyframe.rot = rot;
                        keyframe.scale = scale;
                        keyframe.time = time;
                        keyframes.Add(keyframe);
                    }
                    time += 0.025f;  // FIXME?
                }
            }
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneAnimation();
            sceneData.type = "Animation";
            sceneData.clips = new SceneAnimationClip[clips.Count];
            for (int i = 0; i < clips.Count; i++)
            {
                BundleAnimationClip clip = clips[i];
                var jclip = sceneData.clips[i] = new SceneAnimationClip();
                jclip.name = clip.name;

                int count = 0;
                SceneAnimationNode[] jnodes = jclip.nodes = new SceneAnimationNode[clip.keyframes.Count];
                foreach (KeyValuePair<string, List<BundleKeyframe>> entry in clip.keyframes)
                {
                    SceneAnimationNode node = new SceneAnimationNode();
                    node.name = entry.Key;
                    node.keyframes = new SceneKeyframe[entry.Value.Count];

                    int kcount = 0;
                    foreach (var key in entry.Value)
                    {
                        var jkeyframe = new SceneKeyframe();
                        jkeyframe.pos = key.pos;
                        jkeyframe.scale = key.scale;
                        jkeyframe.rot = key.rot;
                        jkeyframe.time = key.time;
                        node.keyframes[kcount++] = jkeyframe;
                    }
                    jnodes[count++] = node;
                }
            }
            return sceneData;
        }

        AnimationHelper unityAnim;
        List<BundleAnimationClip> clips = new List<BundleAnimationClip>();
    }

}
#endif