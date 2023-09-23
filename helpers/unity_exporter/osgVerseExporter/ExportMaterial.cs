using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

#if UNITY_EDITOR
namespace osgVerse
{

    public class MaterialExporter
    {
        public static string ExportStateSetAttr(bool isTransparent, string spaces)
        {
            string osgData = spaces + "  DataVariance STATIC\n";
            if (isTransparent)
            {
                osgData += spaces + "  rendering_hint TRANSPARENT_BIN\n"
                         + spaces + "  renderBinMode USE\n"
                         + spaces + "  binNumber 10\n"
                         + spaces + "  binName DepthSortedBin\n";
            }
            else
            {
                osgData += spaces + "  rendering_hint DEFAULT_BIN\n"
                         + spaces + "  renderBinMode INHERIT\n";
            }
            return osgData;
        }

        public static string ExportTextureAttr(ref SceneTexture texture, string path, string spaces)
        {
            // Save to new position
            TextureImporter textureImporter = AssetImporter.GetAtPath(texture.path) as TextureImporter;
            if (textureImporter != null)
            {
                string dstPath = path + "/" + textureImporter.assetPath;
                string dstPathNoFile = Path.GetDirectoryName(dstPath);
                if (!Directory.Exists(dstPathNoFile)) Directory.CreateDirectory(dstPathNoFile);

                string srcPath = Path.GetFullPath(textureImporter.assetPath);
                File.Copy(srcPath, dstPath, true); texture.path = textureImporter.assetPath;
            }
            else if (texture.base64Length > 0)
            {
                string dstPath = path + "/" + texture.path;
                string dstPathNoFile = Path.GetDirectoryName(dstPath);
                if (!Directory.Exists(dstPathNoFile)) Directory.CreateDirectory(dstPathNoFile);

                byte[] texData = System.Convert.FromBase64String(texture.base64);
                if (texData != null) File.WriteAllBytes(dstPath, texData);
            }
            
            string dataPath = Path.GetFileName(path);
            texture.path = dataPath + "/" + texture.path;

            string osgData = spaces + "  DataVariance STATIC\n"
                           + spaces + "  name \"" + texture.name + "\"\n"
                           + spaces + "  file \"" + texture.path + "\"\n"
                           + spaces + "  wrap_s REPEAT\n"
                           + spaces + "  wrap_t REPEAT\n"
                           + spaces + "  wrap_r REPEAT\n"
                           + spaces + "  min_filter LINEAR_MIPMAP_LINEAR\n"
                           + spaces + "  mag_filter LINEAR\n"
                           + spaces + "  maxAnisotropy 1\n"
                           + spaces + "  borderColor 0 0 0 0\n"
                           + spaces + "  borderWidth 0\n"
                           + spaces + "  useHardwareMipMapGeneration TRUE\n"
                           + spaces + "  unRefImageDataAfterApply TRUE\n"
                           + spaces + "  internalFormatMode USE_IMAGE_DATA_FORMAT\n"
                           + spaces + "  resizeNonPowerOfTwo TRUE\n";
            return osgData;
        }

        public static string ExportStateSet(ref SceneData sceneData, ref SceneMeshRenderer smr,
                                            string path, string spaces)
        {
            string osgData = spaces + "StateSet {\n" + ExportStateSetAttr(false, spaces);
            for (int i = 0; i < smr.materials.Length; ++i)
            {
                SceneMaterial material = sceneData.resources.GetMaterial(smr.materials[i]);
                if (material.textureIDs == null) continue;

                string shaderData = spaces + "  osgVerse::ShaderData {\n"
                                  + spaces + "    ShaderName \"" + material.shader + "\"\n";
                for (int j = 0; j < material.textureIDs.Length; ++j)
                {
                    int texID = material.textureIDs[j], unit = material.textureUnits[j];
                    SceneTexture texture = sceneData.resources.GetTexture(texID, false);
                    if (texture == null || unit < 0) continue;

                    shaderData += spaces + "    Texture" + unit + " \"" + texture.name + "\"\n"
                                + spaces + "    Path \"" + texture.path + "\"\n"
                                + spaces + "    Type \"" + texture.propName + "\"\n";
                    /*if (i > 0)
                    {
                        Debug.LogWarning("[osgVerse] Multi-material is not supported");
                        continue;  // FIXME: multi-material case
                    }*/

                    // Handle texture tiling and offset
                    osgData += spaces + "  textureUnit " + unit + " {\n"
                             + spaces + "    GL_TEXTURE_2D ON\n";
                    if (material.textureTilingOffsets[j] != indentityTilingOffsetVector)
                    {
                        Vector4 off = material.textureTilingOffsets[j];
                        Matrix4x4 m = Matrix4x4.TRS(new Vector3(off.z, off.w, 0.0f), Quaternion.identity,
                                                    new Vector3(off.x, off.y, 1.0f));
                        osgData += spaces + "    TexMat {\n"
                                 + spaces + "      " + m[0, 0] + " " + m[1, 0] + " " + m[2, 0] + " " + m[3, 0] + "\n"
                                 + spaces + "      " + m[0, 1] + " " + m[1, 1] + " " + m[2, 1] + " " + m[3, 1] + "\n"
                                 + spaces + "      " + m[0, 2] + " " + m[1, 2] + " " + m[2, 2] + " " + m[3, 2] + "\n"
                                 + spaces + "      " + m[0, 3] + " " + m[1, 3] + " " + m[2, 3] + " " + m[3, 3] + "\n"
                                 + spaces + "    }\n";
                    }

                    // Handle texture
                    if (sharedTextureNames.ContainsKey(texID))
                        osgData += spaces + "    Use " + sharedTextureNames[texID] + "\n";
                    else
                    {
                        sharedTextureNames[texID] = "Texture_" + texID;
                        osgData += spaces + "    Texture2D {\n"
                                 + spaces + "      UniqueID Texture_" + texID + "\n"
                                 + ExportTextureAttr(ref texture, path, spaces + "    ")
                                 + spaces + "    }\n";
                    }
                    osgData += spaces + "  }\n";
                }

                // Convert and save metallic/roughness/occlusion combined texture
                if (material.combinedTextures != null && material.combinedTextures.Count > 0)
                {
                    int texID = 0;
                    SceneTexture texture = sceneData.resources.GetCombinedTexture(
                        material.combinedTextures, ref texID);
                    if (texture != null)
                    {
                        osgData += spaces + "  textureUnit 7 {\n"  // FIXME: combined unit
                                 + spaces + "    GL_TEXTURE_2D ON\n";
                        osgData += spaces + "    Texture2D {\n"
                                 + spaces + "      UniqueID Texture_" + texID + "\n"
                                 + ExportTextureAttr(ref texture, path, spaces + "    ")
                                 + spaces + "    }\n";
                        osgData += spaces + "  }\n";
                    }
                }

                // Save shader data for use
                if (material.shaderKeywords != null)
                {
                    shaderData += spaces + "    Keywords ";
                    for (int k = 0; k < material.shaderKeywords.Length; ++k)
                    {
                        shaderData += material.shaderKeywords[k]
                                    + ((k < material.shaderKeywords.Length - 1) ? " " : "\n");
                    }
                }
                osgData += shaderData + spaces + "  }\n";
            }

            // Handle lightmaps
            if (smr.lightmapIndex >= 0)
            {
                SceneTexture texture = sceneData.resources.lightmaps[smr.lightmapIndex];
                if (texture != null)
                {
                    osgData += spaces + "  textureUnit 5 {\n"  // FIXME: lightmap unit
                             + spaces + "    GL_TEXTURE_2D ON\n";

                    // Handle lightmap tiling and offset
                    if (smr.lightmapTilingOffset != indentityTilingOffsetVector)
                    {
                        Vector4 off = smr.lightmapTilingOffset;
                        Matrix4x4 m = Matrix4x4.TRS(new Vector3(off.z, off.w, 0.0f), Quaternion.identity,
                                                    new Vector3(off.x, off.y, 1.0f));
                        osgData += spaces + "    TexMat {\n"
                                 + spaces + "      " + m[0, 0] + " " + m[1, 0] + " " + m[2, 0] + " " + m[3, 0] + "\n"
                                 + spaces + "      " + m[0, 1] + " " + m[1, 1] + " " + m[2, 1] + " " + m[3, 1] + "\n"
                                 + spaces + "      " + m[0, 2] + " " + m[1, 2] + " " + m[2, 2] + " " + m[3, 2] + "\n"
                                 + spaces + "      " + m[0, 3] + " " + m[1, 3] + " " + m[2, 3] + " " + m[3, 3] + "\n"
                                 + spaces + "    }\n";
                    }

                    // Handle texture
                    if (sharedTextureNames.ContainsKey(texture.uniqueID))
                        osgData += spaces + "    Use " + sharedTextureNames[texture.uniqueID] + "\n";
                    else
                    {
                        sharedTextureNames[texture.uniqueID] = "Texture_" + texture.uniqueID;
                        osgData += spaces + "    Texture2D {\n"
                                 + spaces + "      UniqueID Texture_" + texture.uniqueID + "\n"
                                 + ExportTextureAttr(ref texture, path, spaces + "    ")
                                 + spaces + "    }\n";
                    }
                    osgData += spaces + "  }\n";
                }
            }
            osgData += spaces + "}\n";
            return osgData;
        }

        public static void Reset()
        {
            sharedTextureNames = new Dictionary<int, string>();
        }

        public static Vector4 indentityTilingOffsetVector = new Vector4(1.0f, 1.0f, 0.0f, 0.0f);
        public static Dictionary<int, string> sharedTextureNames;
    }

}
#endif