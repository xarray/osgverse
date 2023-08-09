using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;

namespace osgVerse
{

    public class ParticleExporter
    {
        public static string ExportParticle(ref SceneData sceneData, ref SceneParticleSystem sps, string spaces)
        {
            string osgData = spaces + "Duration " + sps.duration + "\n"
                           + spaces + "Playing " + sps.playingSpeed + " " + (sps.isLooping ? 1 : 0)
                                                 + " " + (sps.isAutoStarted ? 1 : 0) + "\n"
                           + spaces + "MaxParticles " + sps.maxParticles + "\n"
                           + spaces + "Gravity " + sps.gravity.x + " " + sps.gravity.y + " " + sps.gravity.z + "\n"
                           + spaces + "Rotation " + sps.rotation.x + " " + sps.rotation.y + " " + sps.rotation.z + "\n"
                           + spaces + "StartAttributes " + sps.startAttributes.x + " " + sps.startAttributes.y + " "
                                                         + sps.startAttributes.z + " " + sps.startAttributes.w + "\n"
                           + spaces + "StartColor " + sps.startColor.r + " " + sps.startColor.g + " "
                                                    + sps.startColor.b + " " + sps.startColor.a + "\n";
            for (int i = 0; i < sps.enabledModules.Length; ++i)
            {
                string moduleName = sps.enabledModules[i];
                osgData += spaces + moduleName + " {\n";
                if (moduleName == "Emission")
                {
                    osgData += spaces + "  Type " + sps.emissionType + "\n"
                             + spaces + "  Rate " + sps.emissionRate.Length + " {\n";
                    for (int j = 0; j < sps.emissionRate.Length; ++j)
                    {
                        Vector4 v = sps.emissionRate[j];
                        osgData += spaces + "    " + v.x + " " + v.y + " " + v.z + " " + v.w + "\n";
                    }
                    osgData += spaces + "  }\n";
                }
                else if (moduleName == "TextureSheetAnimation")
                {
                    osgData += spaces + "  Type " + sps.tsaAnimationType + "\n"
                             + spaces + "  Tiles " + sps.tsaNumTiles.x + " " + sps.tsaNumTiles.y + "\n"
                             + spaces + "  CycleCount " + sps.tsaCycleCount + "\n"
                             + spaces + "  FrameOverTime " + sps.tsaFrameOverTime.Length + " {\n";
                    for (int j = 0; j < sps.tsaFrameOverTime.Length; ++j)
                    {
                        Vector4 v = sps.tsaFrameOverTime[j];
                        osgData += spaces + "    " + v.x + " " + v.y + " " + v.z + " " + v.w + "\n";
                    }
                    osgData += spaces + "  }\n";
                }
                else if (moduleName == "Renderer")
                {
                    osgData += spaces + "  ShapeMode " + sps.renderShapeMode + "\n"
                             + spaces + "  SortMode " + sps.renderSortMode + "\n"
                             + spaces + "  Attributes " + sps.renderAttributes.x + " " + sps.renderAttributes.y + " "
                                                        + sps.renderAttributes.z + " " + sps.renderAttributes.w + "\n";

                    SceneMaterial material = sceneData.resources.GetMaterial(sps.renderMaterial);
                    osgData += spaces + "  Material " + material.textureIDs.Length + " {\n";
                    for (int j = 0; j < material.textureIDs.Length; ++j)
                    {
                        SceneTexture texture = sceneData.resources.GetTexture(material.textureIDs[j], false);
                        if (texture == null) continue;

                        Vector4 off = material.textureTilingOffsets[j];
                        osgData += spaces + "    Texture" + j + " \"" + texture.name + "\""
                                                              + " \"" + texture.path + "\"\n"
                                 + spaces + "    TilingOffset" + j + " " + off.x + " " + off.y
                                                                   + " " + off.z + " " + off.w + "\n";
                    }
                    osgData += spaces + "  }\n";
                }
                osgData += spaces + "}\n";
            }
            return osgData;
        }

        public static void Reset()
        {
        }
    }

}
