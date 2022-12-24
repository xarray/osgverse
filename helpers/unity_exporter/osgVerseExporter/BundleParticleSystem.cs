using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class BundleParticleSystem : BundleComponent
    {
        override public void Preprocess()
        {
            unityParticle = unityComponent as ParticleSystem;
            unityParticleRenderer = unityComponent.GetComponent<Renderer>() as ParticleSystemRenderer;
        }

        override public void QueryResources()
        {
            if (unityParticleRenderer != null)
                material = BundleMaterial.RegisterMaterial(unityParticleRenderer.sharedMaterial);
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneParticleSystem();
            sceneData.type = "ParticleSystem";

            List<string> enabledModules = new List<string>();
            if (unityParticle != null)
            {
                // Basic
                sceneData.maxParticles = unityParticle.maxParticles;
                sceneData.duration = unityParticle.duration;
                sceneData.playingSpeed = unityParticle.playbackSpeed;
                sceneData.isLooping = unityParticle.loop;
                sceneData.isAutoStarted = unityParticle.playOnAwake;
                sceneData.gravity = Physics.gravity * unityParticle.gravityModifier;
                sceneData.rotation = unityParticle.startRotation3D;
                sceneData.startAttributes = new Vector4(unityParticle.startLifetime, unityParticle.startSize,
                                                        unityParticle.startSpeed, unityParticle.startDelay);
                sceneData.startColor = unityParticle.startColor;

                if (unityParticle.emission.enabled)
                {
                    sceneData.emissionRate = createCurveData(unityParticle.emission.rate);
                    sceneData.emissionType = unityParticle.emission.type.ToString();
                    enabledModules.Add("Emission");
                }

                if (unityParticle.textureSheetAnimation.enabled)
                {
                    sceneData.tsaFrameOverTime = createCurveData(unityParticle.textureSheetAnimation.frameOverTime);
                    sceneData.tsaNumTiles = new Vector2(unityParticle.textureSheetAnimation.numTilesX,
                                                        unityParticle.textureSheetAnimation.numTilesY);
                    sceneData.tsaCycleCount = unityParticle.textureSheetAnimation.cycleCount;
                    sceneData.tsaAnimationType = unityParticle.textureSheetAnimation.animation.ToString();
                    enabledModules.Add("TextureSheetAnimation");
                }
            }

            if (unityParticleRenderer != null)
            {
                sceneData.renderAttributes = new Vector4(
                    unityParticleRenderer.minParticleSize, unityParticleRenderer.maxParticleSize,
                    unityParticleRenderer.normalDirection, unityParticleRenderer.sortingFudge);
                sceneData.renderShapeMode = unityParticleRenderer.renderMode.ToString();
                sceneData.renderSortMode = unityParticleRenderer.sortMode.ToString();
                enabledModules.Add("Renderer");
            }
            sceneData.renderMaterial = material.name;

            sceneData.enabledModules = new string[enabledModules.Count];
            for (int i = 0; i < enabledModules.Count; ++i)
                sceneData.enabledModules[i] = enabledModules[i];
            return sceneData;
        }

        private Vector4[] createCurveData(ParticleSystem.MinMaxCurve inputCurve)
        {
            Vector4[] keyframeData;
            AnimationCurve curve = inputCurve.curveMax;
            if (curve == null) curve = inputCurve.curveMin;
            if (curve == null)
            {
#if false
            // FIXME: Unity limitation of reading MinMaxCurve
            if ( inputCurve.constantMax<=inputCurve.constantMin )
            {
                keyframeData = new Vector4[2];
                keyframeData[0] = new Vector4(0.0f, 0.0f, 0.0f, 0.0f);
                keyframeData[1] = new Vector4(1.0f, 1.0f, 0.0f, 0.0f);
                return keyframeData;
            }
#endif
                keyframeData = new Vector4[1];
                keyframeData[0] = new Vector4(0, inputCurve.constantMax, inputCurve.constantMin, 0.0f);
                return keyframeData;
            }

            keyframeData = new Vector4[curve.length];
            for (int i = 0; i < curve.length; ++i)
            {
                Keyframe key = curve.keys[i];
                keyframeData[i] = new Vector4(key.time, key.value, key.inTangent, key.outTangent);
            }
            return keyframeData;
        }

        ParticleSystem unityParticle;
        ParticleSystemRenderer unityParticleRenderer;
        BundleMaterial material;
    }

}
#endif