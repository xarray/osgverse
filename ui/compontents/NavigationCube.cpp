#include "../ImGuiComponents.h"
#include <imgui/imgui.h>
#include <algorithm>
#include <cmath>
#include <vector>
using namespace osgVerse;

namespace
{
    /* Layout of assets/textures/navigation.png (1024 x 1024):
       - left half (512 x 1024) holds the 6 cube faces, each one is a 256 x 256 square:
         row y=256: Z- (Down), Z+ (Up)     row y=512: Y- (Front), Y+ (Back)
         row y=768: X- (Left), X+ (Right), left square is the "minus" face of each axis
       - right half holds a compass dial at (512, 512)-(1024, 1024) and a needle at (714, 202)-(823, 312)
       The texture is uploaded by OSG with a flipped V axis, so a square starting at row sy
       is addressed with V in [1 - (sy + 256) / 1024, 1 - sy / 1024]. */
    const float s_atlasSize = 1024.0f;
    const float s_faceSize = 256.0f;

    // Atlas square of each cube face, in face order: +X, -X, +Y, -Y, +Z, -Z
    const float s_atlasSquares[6][2] = {
        { 256.0f, 768.0f },  // +X: Right
        { 0.0f,   768.0f },  // -X: Left
        { 256.0f, 512.0f },  // +Y: Back
        { 0.0f,   512.0f },  // -Y: Front
        { 256.0f, 256.0f },  // +Z: Up
        { 0.0f,   256.0f }   // -Z: Down
    };

    const ImU32 s_faceColor = IM_COL32(238, 238, 242, 255);
    const ImU32 s_outlineColor = IM_COL32(60, 60, 70, 220);

    // Cube corners, in [-1, 1] range
    const float s_corners[8][3] = {
        { -1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f },
        {  1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
        { -1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f }
    };

    /* Corner indices of each face, ordered as (bottom-left, bottom-right, top-right, top-left)
       as seen from outside of the cube, with the world's +Z axis pointing "up" on side faces */
    const int s_faceCorners[6][4] = {
        { 1, 2, 6, 5 },  // +X
        { 3, 0, 4, 7 },  // -X
        { 2, 3, 7, 6 },  // +Y
        { 0, 1, 5, 4 },  // -Y
        { 4, 5, 6, 7 },  // +Z
        { 3, 2, 1, 0 }   // -Z
    };

    const float s_faceNormals[6][3] = {
        {  1.0f,  0.0f,  0.0f }, { -1.0f,  0.0f,  0.0f }, {  0.0f,  1.0f,  0.0f },
        {  0.0f, -1.0f,  0.0f }, {  0.0f,  0.0f,  1.0f }, {  0.0f,  0.0f, -1.0f }
    };

    struct FaceDepth { float depth; int index; };

    struct NavigationDrawer
    {
        ImVec2 center, itemSize;
        ImVec2 point[8]; float depth[8]; float scale;

        void drawCube(ImDrawList* dl, ImTextureID tex, const osg::Matrix& view)
        {
            for (int i = 0; i < 8; ++i)
            {
                osg::Vec3f p(s_corners[i][0], s_corners[i][1], s_corners[i][2]);
                osg::Vec3f v = p * view;
                point[i] = ImVec2(center.x + v.x() * scale, center.y - v.y() * scale);
                depth[i] = v.z();
            }

            std::vector<FaceDepth> faces;
            for (int f = 0; f < 6; ++f)
            {
                osg::Vec3f n(s_faceNormals[f][0], s_faceNormals[f][1], s_faceNormals[f][2]);
                n = n * view; if (n.z() <= 0.001f) continue;  // back-facing

                FaceDepth fd; fd.index = f;
                const int* ci = s_faceCorners[f];
                fd.depth = (depth[ci[0]] + depth[ci[1]] + depth[ci[2]] + depth[ci[3]]) * 0.25f;
                faces.push_back(fd);
            }
            std::sort(faces.begin(), faces.end(),
                [](const FaceDepth& a, const FaceDepth& b) { return a.depth < b.depth; });

            for (size_t i = 0; i < faces.size(); ++i)
            {
                const int f = faces[i].index; const int* ci = s_faceCorners[f];
                const ImVec2& p1 = point[ci[0]]; const ImVec2& p2 = point[ci[1]];
                const ImVec2& p3 = point[ci[2]]; const ImVec2& p4 = point[ci[3]];
                const float* sq = s_atlasSquares[f];
                const float u0 = sq[0] / s_atlasSize, u1 = (sq[0] + s_faceSize) / s_atlasSize;
                // The texture has a flipped V axis, so the bottom of the atlas square is
                // addressed with the smaller V value
                const float vBottom = 1.0f - (sq[1] + s_faceSize) / s_atlasSize;
                const float vTop = 1.0f - sq[1] / s_atlasSize;

                // A plain quad is drawn first: some faces of the atlas have a transparent
                // background, and this keeps all of them looking identical
                dl->AddQuadFilled(p1, p2, p3, p4, s_faceColor);
                dl->AddImageQuad(tex, p1, p2, p3, p4, ImVec2(u0, vBottom), ImVec2(u1, vBottom),
                                 ImVec2(u1, vTop), ImVec2(u0, vTop), IM_COL32_WHITE);
                dl->AddQuad(p1, p2, p3, p4, s_outlineColor, 1.0f);
            }
        }

        void drawCompass(ImDrawList* dl, ImTextureID tex, float dialAngle)
        {
            /* The dial occupies the right-bottom quarter of the atlas and is rotated around its
               center, while the needle sprite (stored above the dial) is drawn unrotated and so
               always points to the top of the widget */
            const float r = ((itemSize.x < itemSize.y) ? itemSize.x : itemSize.y) * 0.5f;
            const float ca = cosf(dialAngle), sa = sinf(dialAngle);

            // Corner order is (top-left, top-right, bottom-right, bottom-left); the texture has a
            // flipped V axis, so the top of the dial is addressed with the larger V value
            const float uvs[4][2] = { { 0.5f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 0.0f }, { 0.5f, 0.0f } };
            const float lx[4] = { -r, r, r, -r }, ly[4] = { -r, -r, r, r };
            ImVec2 dp[4];
            for (int i = 0; i < 4; ++i)
                dp[i] = ImVec2(center.x + lx[i] * ca - ly[i] * sa,
                               center.y + lx[i] * sa + ly[i] * ca);
            dl->AddImageQuad(tex, dp[0], dp[1], dp[2], dp[3],
                             ImVec2(uvs[0][0], uvs[0][1]), ImVec2(uvs[1][0], uvs[1][1]),
                             ImVec2(uvs[2][0], uvs[2][1]), ImVec2(uvs[3][0], uvs[3][1]), IM_COL32_WHITE);

            // Both the needle size and the dial radius are measured in atlas pixels
            const float dialRadius = 511.0f * 0.5f;
            const float needleU0 = 714.0f / s_atlasSize, needleU1 = 823.0f / s_atlasSize;
            const float needleVTop = 1.0f - 202.0f / s_atlasSize;
            const float needleVBottom = 1.0f - 312.0f / s_atlasSize;
            const float halfW = r * 0.5f * (823.0f - 714.0f) / dialRadius;
            const float halfH = r * 0.5f * (312.0f - 202.0f) / dialRadius;
            dl->AddImageQuad(tex, ImVec2(center.x - halfW, center.y - halfH),
                             ImVec2(center.x + halfW, center.y - halfH),
                             ImVec2(center.x + halfW, center.y + halfH),
                             ImVec2(center.x - halfW, center.y + halfH),
                             ImVec2(needleU0, needleVTop), ImVec2(needleU1, needleVTop),
                             ImVec2(needleU1, needleVBottom), ImVec2(needleU0, needleVBottom),
                             IM_COL32_WHITE);
        }
    };
}

bool NavigationCube::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    ImVec2 itemSize(size[0], size[1]);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(name.c_str(), itemSize);

    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    if (clicked) displayMode = (displayMode == CubeMode) ? CompassMode : CubeMode;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p1(p0.x + itemSize.x, p0.y + itemSize.y);
    dl->AddRectFilled(p0, p1, IM_COL32(40, 42, 48, 180), 6.0f);

    ImTextureID tex = 0;
    if (content != NULL)
    {
        std::map<std::string, ImTextureID>::iterator it = content->ImGuiTextures.find(imageName);
        if (it != content->ImGuiTextures.end()) tex = it->second;
    }

    if (tex == 0)
        dl->AddText(ImVec2(p0.x + 8.0f, p0.y + itemSize.y * 0.5f - 8.0f),
                    IM_COL32(230, 180, 120, 255), "No navigation texture");
    else
    {
        NavigationDrawer drawer;
        drawer.center = ImVec2(p0.x + itemSize.x * 0.5f, p0.y + itemSize.y * 0.5f);
        drawer.itemSize = itemSize;
        drawer.scale = ((itemSize.x < itemSize.y) ? itemSize.x : itemSize.y) * 0.30f;
        if (displayMode == CubeMode)
        {
            osg::Matrix rotation; rotation.makeRotate(viewMatrix.getRotate());
            drawer.drawCube(dl, tex, rotation);
        }
        else
            drawer.drawCompass(dl, tex, dialAngle);
    }

    if (hovered)
    {
        dl->AddRect(p0, p1, IM_COL32(255, 200, 100, 255), 6.0f, 0, 1.5f);
        if (!tooltip.empty()) ImGui::SetTooltip("%s", tooltip.c_str());
    }
    return clicked;
}
