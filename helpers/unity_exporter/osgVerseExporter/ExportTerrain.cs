using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;

#if UNITY_EDITOR
namespace osgVerse
{

    public class TerrainExporter
    {
        public static string ExportTerrain(ref SceneData sceneData, ref SceneTerrain st, string spaces)
        {
            string osgData = spaces + "Size " + st.size.x + " " + st.size.y + " " + st.size.z + "\n";

            // Handle heightmap data
            osgData += spaces + "HeightMap " + st.heightmapWidth + " " + st.heightmapHeight + " {\n";
            byte[] heightData = System.Convert.FromBase64String(st.heightmapTexture.base64);
            System.Text.StringBuilder sb = new System.Text.StringBuilder();
            for (int y = 0; y < st.heightmapHeight; y++)
            {
                sb.Append(spaces + "  ");
                for (int x = 0; x < st.heightmapWidth; x++)
                {
                    int index = (y * st.heightmapWidth) + x;
                    sb.Append(System.BitConverter.ToSingle(heightData, index * 4) + " ");
                }
                sb.Append("\n");
            }
            osgData += sb.ToString() + spaces + "}\n";

            // Handle alphamap layers
            osgData += spaces + "AlphaMap " + st.alphamapWidth + " " + st.alphamapHeight
                                            + " " + st.alphamapLayers + " {\n";
            byte[] alphaData = System.Convert.FromBase64String(st.alphamapTexture.base64);
            System.Text.StringBuilder sb2 = new System.Text.StringBuilder();
            for (int i = 0; i < st.alphamapLayers; i++)
            {
                sb2.Append(spaces + "  Layer " + i + " {\n");
                for (int y = 0; y < st.alphamapHeight; y++)
                {
                    sb2.Append(spaces + "    ");
                    for (int x = 0; x < st.alphamapWidth; x++)
                    {
                        int index = i * (st.alphamapHeight * st.alphamapWidth) + (y * st.alphamapWidth) + x;
                        sb2.Append(System.BitConverter.ToSingle(alphaData, index * 4) + " ");
                    }
                    sb2.Append("\n");
                }
                sb2.Append(spaces + "  }\n");
            }
            osgData += sb2.ToString() + spaces + "}\n";

            // Handle all splat textures
            for (int i = 0; i < st.textureIDs.Length; i++)
            {
                int texID = st.textureIDs[i];
                SceneTexture texture = sceneData.resources.GetTexture(texID, false);
                if (texture == null) continue;

                Vector4 off = st.textureTilingOffsets[i];
                osgData += spaces + "Splat" + i + " \"" + texture.name + "\""
                            + " \"" + texture.path + "\"\n";
                osgData += spaces + "SplatTilingOffset" + i + " "
                         + off.x + " " + off.y + " " + off.z + " " + off.w + "\n";
            }

            // Handle lightmaps
            if (st.lightmapIndex >= 0)
            {
                SceneTexture texture = sceneData.resources.lightmaps[st.lightmapIndex];
                if (texture != null)
                {
                    osgData += spaces + "Lightmap \"" + texture.name + "\""
                            + " \"" + texture.path + "\"\n";
                    osgData += spaces + "LightmapTilingOffset " + st.lightmapTilingOffset.x + " "
                             + st.lightmapTilingOffset.y + " " + st.lightmapTilingOffset.z + " "
                             + st.lightmapTilingOffset.w + "\n";
                }
            }
            return osgData;
        }

        public static void Reset()
        {
        }
    }

}
#endif