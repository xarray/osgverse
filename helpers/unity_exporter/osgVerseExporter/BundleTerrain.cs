using System;
using System.Collections.Generic;
using UnityEngine;

namespace osgVerse
{

    public class BundleTerrain : BundleComponent
    {
        override public void Preprocess()
        {
            terrain = unityComponent as Terrain;
            terrainData = terrain.terrainData;
            if (terrainData == null) return;

            SplatPrototype[] splats = terrainData.splatPrototypes;
            for (int i = 0; i < splats.Length; ++i)
            {
                textures.Add(BundleTexture.RegisterTexture(splats[i].texture, "SplatTex"));
                textureSizes.Add(splats[i].tileSize);
                textureOffsets.Add(splats[i].tileOffset);
            }
        }

        override public void Process()
        {
            if (terrainData == null) return;
            heightmapHeight = terrainData.heightmapResolution;
            heightmapWidth = terrainData.heightmapResolution;
            size = terrainData.size;

            float[,] heights = terrainData.GetHeights(0, 0, heightmapWidth, heightmapHeight);
            float[] arrayHeight = new float[heightmapWidth * heightmapHeight];
            for (int y = 0; y < heightmapHeight; y++)
            {
                for (int x = 0; x < heightmapWidth; x++)
                    arrayHeight[y * heightmapWidth + x] = heights[x, y];
            }

            alphamapWidth = terrainData.alphamapWidth;
            alphamapHeight = terrainData.alphamapHeight;
            alphamapLayers = terrainData.alphamapLayers;

            float[,,] alphamaps = terrainData.GetAlphamaps(0, 0, alphamapWidth, alphamapHeight);
            float[] arrayAlpha = new float[alphamapWidth * alphamapHeight * alphamapLayers];
            for (int i = 0; i < alphamapLayers; i++)
            {
                for (int y = 0; y < alphamapHeight; y++)
                {
                    for (int x = 0; x < alphamapWidth; x++)
                        arrayAlpha[i * (alphamapHeight * alphamapWidth) + (y * alphamapWidth) + x] = alphamaps[x, y, i];
                }
            }

            var byteHeightArray = new byte[arrayHeight.Length * 4];
            Buffer.BlockCopy(arrayHeight, 0, byteHeightArray, 0, byteHeightArray.Length);
            base64HeightLength = byteHeightArray.Length;
            base64Height = System.Convert.ToBase64String(byteHeightArray, 0, byteHeightArray.Length);

            var byteAlphaArray = new byte[arrayAlpha.Length * 4];
            Buffer.BlockCopy(arrayAlpha, 0, byteAlphaArray, 0, byteAlphaArray.Length);
            base64AlphaLength = byteAlphaArray.Length;
            base64Alpha = System.Convert.ToBase64String(byteAlphaArray, 0, byteAlphaArray.Length);
        }

        override public void QueryResources()
        {
        }

        new public static void Reset()
        {
        }

        public override SceneComponent GetObjectData()
        {
            var sceneData = new SceneTerrain();
            sceneData.type = "Terrain";
            sceneData.heightmapHeight = heightmapHeight;
            sceneData.heightmapWidth = heightmapWidth;
            sceneData.size = size;
            sceneData.alphamapWidth = alphamapWidth;
            sceneData.alphamapHeight = alphamapHeight;
            sceneData.alphamapLayers = alphamapLayers;

            sceneData.heightmapTexture = new SceneTexture();
            sceneData.heightmapTexture.base64 = base64Height;
            sceneData.heightmapTexture.base64Length = base64HeightLength;

            sceneData.alphamapTexture = new SceneTexture();
            sceneData.alphamapTexture.base64 = base64Alpha;
            sceneData.alphamapTexture.base64Length = base64AlphaLength;

            if (textures.Count > 0)
            {
                sceneData.textureIDs = new int[textures.Count];
                for (int i = 0; i < textures.Count; i++)
                    sceneData.textureIDs[i] = textures[i].uniqueID;

                sceneData.textureTilingOffsets = new Vector4[textures.Count];
                for (int i = 0; i < textures.Count; i++)
                {
                    Vector2 sc = textureSizes[i], of = textureOffsets[i];
                    sceneData.textureTilingOffsets[i] = new Vector4(sc.x, sc.y, of.x, of.y);
                }
            }

            if (terrain != null)
            {
                sceneData.lightmapIndex = terrain.lightmapIndex;
                sceneData.lightmapTilingOffset = terrain.lightmapScaleOffset;
            }
            return sceneData;
        }

        public Terrain terrain;
        private TerrainData terrainData;

        Vector3 size;
        int heightmapHeight, heightmapWidth;
        int alphamapWidth, alphamapHeight, alphamapLayers;

        public string base64Height;
        public int base64HeightLength;

        public string base64Alpha;
        public int base64AlphaLength;

        public List<BundleTexture> textures = new List<BundleTexture>();
        public List<Vector2> textureSizes = new List<Vector2>();
        public List<Vector2> textureOffsets = new List<Vector2>();
    }

}
