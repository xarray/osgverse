#include <osg/io_utils>
#include <osg/LineWidth>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Simplifier>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <iostream>
#include <sstream>
#include <pipeline/Utilities.h>
#include <pipeline/IntersectionManager.h>
#include <modeling/Utilities.h>
#include <modeling/Math.h>
#include <modeling/MeshTopology.h>

#include <3rdparty/Eigen/Core>
#include <3rdparty/BSplineFitting/spline_curve_fitting.h>
#include <3rdparty/tinyspline/tinysplinecxx.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

static std::map<int, osg::observer_ptr<osg::Node>> g_toothMap;
static std::map<int, std::vector<osg::Vec3>> g_borderMap;
static std::map<int, std::pair<osg::Vec4, osg::Quat>> g_obbMap;
static osg::ref_ptr<osg::Geometry> g_delaunay, g_gumline;
static bool g_upGum = false;

std::vector<osg::Vec3> generateGumCenterline(const std::vector<Eigen::Vector2d>& curveSamples,
                                             float z, float power)
{
#if false
    OpenCubicBSplineCurve curve0(0.002); SplineCurveFitting scf;
    scf.fitAOpenCurve(curveSamples, curve0, 18, 20, 0.005, 0.005, 0.0001);

    // Fit gum center-line to a spline and resample it
    std::vector<osg::Vec3> centerLine;
    const std::vector<Eigen::Vector2d>& samples = curve0.getSamples();
    for (size_t i = 0; i < samples.size(); i += 10)
        centerLine.push_back(osg::Vec3(samples[i].x(), samples[i].y(), z));
#else
    int maxHalf = 400;
    osg::BoundingBox bb; float invMaxHalf = 1.0f / (float)maxHalf;
    for (size_t i = 0; i < curveSamples.size(); ++i)
        bb.expandBy(osg::Vec3(curveSamples[i].x(), curveSamples[i].y(), 0.0f));

    float cx = bb.center().x(), cy = bb.yMin();
    std::vector<osg::Vec3> centerLine;
    centerLine.push_back(osg::Vec3(bb.xMin(), bb.yMax(), z));
    for (int i = 1 - maxHalf; i < 0; ++i)
    {
        float xx = (float)i * invMaxHalf; float yy = pow(-xx, power);
        centerLine.push_back(osg::Vec3(
            bb.xMin() * (-xx) + cx * (1.0f + xx), bb.yMax() * yy + cy * (1.0f - yy), z));
    }
    centerLine.push_back(osg::Vec3(cx, cy, z));
    for (int i = 1; i < maxHalf; ++i)
    {
        float xx = (float)i * invMaxHalf; float yy = pow(xx, power);
        centerLine.push_back(osg::Vec3(
            bb.xMax() * xx + cx * (1.0f - xx), bb.yMax() * yy + cy * (1.0f - yy), z));
    }
    centerLine.push_back(osg::Vec3(bb.xMax(), bb.yMax(), z));
#endif
    return centerLine;
}

osg::Vec3 getIntersectionFromBorders(std::map<int, osgVerse::PointList2D>& polygons,
                                     const osg::Vec3& start, const osg::Vec3& end,
                                     const osg::Vec3& lastRef, bool* noResult = NULL,
                                     float hOffset = 0.1f, float vOffset = 0.0f)
{
    osgVerse::LineType2D line(osg::Vec2(start[0], start[1]), osg::Vec2(end[0], end[1]));
    std::vector<osg::Vec3> result; osg::Vec3 dir = end - start;
    float length = dir.normalize(), length2 = (line.second - line.first).length();
    for (std::map<int, osgVerse::PointList2D>::iterator itr = polygons.begin();
         itr != polygons.end(); ++itr)
    {
        std::vector<osgVerse::PointType2D> intersections =
            osgVerse::GeometryAlgorithm::intersectionWithPolygon2D(line, itr->second);
        std::vector<osg::Vec3>& border = g_borderMap[itr->first];
        osg::Vec3 center; for (size_t i = 0; i < border.size(); ++i) center += border[i];
        center /= (float)border.size(); osg::Vec2 center2(center[0], center[1]);

        for (size_t i = 0; i < intersections.size(); ++i)
        {
            float z = border[intersections[i].second].z();
            osg::Vec3 pt(intersections[i].first, z);
            float lC = (osg::Vec2(pt[0], pt[1]) - center2).length();
            float l1 = (osg::Vec2(pt[0], pt[1]) - line.first).length();
            if (l1 < lC * 0.5f) continue;  // intersections near teeth gap are also ignored
            float l2 = osg::minimum(length - l1, hOffset);
            if (l1 > 0.1f && l1 < length) result.push_back(pt + dir * l2 + osg::Z_AXIS * vOffset);
        }
    }

    if (noResult) *noResult = result.empty();
    if (result.empty())
    {
        float length1 = (osg::Vec2(lastRef[0], lastRef[1]) - line.first).length();
        osg::Vec3 emptyRef = start + dir * length1;
        emptyRef.z() = lastRef.z(); return emptyRef;
    }
    else
    {
        float maxLength = 0.0f; osg::Vec3 selected;
        for (size_t i = 0; i < result.size(); ++i)
        {
            osg::Vec3 dir1 = result[i] - start; float len1 = dir1.normalize();
            if (maxLength < len1) { maxLength = len1; selected = result[i]; }
        }
        return selected;
    }
}

void fitSpline(std::vector<osg::Vec3>& outline, std::vector<osg::Vec3>* refOutline = NULL,
               std::vector<osg::Vec3>* refCenters = NULL)
{
    std::vector<tinyspline::real> ctrlPoints;
    for (size_t i = 0; i < outline.size(); ++i)
    { for (int j = 0; j < 3; ++j) ctrlPoints.push_back(outline[i][j]); }

    tinyspline::BSpline spline = tinyspline::BSpline::interpolateCatmullRom(ctrlPoints, 3);
    size_t numOutline = outline.size(); outline.clear();
    std::vector<tinyspline::real> samples = spline.sample(numOutline);

    std::vector<osg::Vec3> outline1, refOutline1, refCenters1;
    for (size_t i = 0; i < samples.size(); i += 3)
        outline1.push_back(osg::Vec3(samples[i], samples[i + 1], samples[i + 2]));

    if (refCenters) { refCenters1 = *refCenters; refCenters->clear(); }
    if (refOutline) { refOutline1 = *refOutline; refOutline->clear(); }
    for (size_t i = 0; i < outline1.size(); i += 5)
    {
        outline.push_back(outline1[i]);
        if (refCenters) refCenters->push_back(refCenters1[i]);
        if (refOutline) refOutline->push_back(refOutline1[i]);
    }
}

std::vector<osg::Vec3> generateGumOutline(
    const std::vector<osg::Vec3>& centerLine, float width, bool checkBorders,
    std::vector<osg::Vec3>* refOutline = NULL, std::vector<osg::Vec3>* refCenters = NULL,
    float hOffset = 0.1f, float vOffset = 0.0f)
{
    std::vector<osg::Vec3> outline; size_t num = centerLine.size();
    osg::Vec3 sideA, sideB, result, lastRef, axis; double angle = 0.0f;

    // Get border as polygons
    std::map<int, osgVerse::PointList2D> polygons;
    if (checkBorders)
    {
        for (std::map<int, std::vector<osg::Vec3>>::iterator itr = g_borderMap.begin();
             itr != g_borderMap.end(); ++itr)
        {
            std::vector<osg::Vec3>& border = itr->second;
            osgVerse::PointList2D& polygon = polygons[itr->first];
            for (size_t i = 0; i < border.size(); ++i)
                polygon.push_back(osgVerse::PointType2D(osg::Vec2(border[i][0], border[i][1]), i));
        }
    }

    // Get outline from center-line
    std::vector<osg::Vec3> outline0, refOutline0, refCenters0;
    std::vector<std::pair<int, int>> noResultList; bool noResult = false;
    for (size_t i = 1; i < num; ++i)
    {
        // Inside gumline
        osg::Vec3 dir = centerLine[i] - centerLine[i - 1]; dir.normalize();
        sideA = dir ^ osg::Z_AXIS; result = centerLine[i] + sideA * width;
        refCenters0.push_back(centerLine[i]);
        refOutline0.push_back(result); if (i == 1) lastRef = result;  // init first
        if (checkBorders)
        {
            bool noResult0 = (noResult == true);
            result = getIntersectionFromBorders(
                polygons, centerLine[i], result, lastRef, &noResult, hOffset, vOffset);
            if (noResult)
            {
                if (noResult0) noResultList.back().second = outline0.size();
                else noResultList.push_back(std::pair<int, int>(outline0.size(), outline0.size()));
            }
        }
        outline0.push_back(result); lastRef = result;
    }

    // Fix points not intersecting with teeth borders
    for (size_t i = 0; i < noResultList.size(); ++i)
    {
        int l = noResultList[i].first, r = noResultList[i].second;
        if (l <= 0 || outline0.size() <= r + 1) continue;
        osg::Vec3 lRef = outline0[l - 1], rRef = outline0[r + 1];
        for (int k = l; k <= r; ++k)
        {
            float ratio = (float)(1 + k - l) / (float)(2 + r - l);
            outline0[k] = lRef * (1.0f - ratio) + rRef * ratio;
        }
    }

    // Smooth inside part of the gumline
    fitSpline(outline0, &refOutline0, &refCenters0);
    outline.insert(outline.end(), outline0.begin(), outline0.end());
    if (refOutline) refOutline->insert(refOutline->end(), refOutline0.begin(), refOutline0.end());
    if (refCenters) refCenters->insert(refCenters->end(), refCenters0.begin(), refCenters0.end());
    outline0.clear(); refOutline0.clear(); refCenters0.clear(); noResultList.clear(); noResult = false;

    for (size_t i = num - 1; i > 0; --i)
    {
        osg::Vec3 dir = centerLine[i - 1] - centerLine[i]; dir.normalize(); sideB = dir ^ osg::Z_AXIS;
        if (i == num - 1)
        {
            // Left end
            osg::Quat q0; q0.makeRotate(sideA, sideB); q0.getRotate(angle, axis);
            for (size_t i = 1; i < 8; ++i)
            {
                osg::Vec3 side = osg::Quat(angle * (float)i / 8.0, axis) * sideA;
                result = centerLine[num - 1] + side * width; if (refOutline) refOutline->push_back(result);
                if (checkBorders) result = getIntersectionFromBorders(
                    polygons, centerLine[num - 1], result, lastRef, NULL, hOffset, vOffset);
                if (refCenters) refCenters->push_back(centerLine[num - 1]);
                outline.push_back(result); lastRef = result;
            }
        }

        // Outside gumline
        result = centerLine[i] + sideB * width;
        refOutline0.push_back(result); refCenters0.push_back(centerLine[i]);
        if (checkBorders)
        {
            bool noResult0 = (noResult == true);
            result = getIntersectionFromBorders(
                polygons, centerLine[i], result, lastRef, &noResult, hOffset, vOffset);
            if (noResult)
            {
                if (noResult0) noResultList.back().second = outline0.size();
                else noResultList.push_back(std::pair<int, int>(outline0.size(), outline0.size()));
            }
        }
        outline0.push_back(result); lastRef = result;
    }

    // Fix points not intersecting with teeth borders
    for (size_t i = 0; i < noResultList.size(); ++i)
    {
        int l = noResultList[i].first, r = noResultList[i].second;
        if (l <= 0 || outline0.size() <= r + 1) continue;
        osg::Vec3 lRef = outline0[l - 1], rRef = outline0[r + 1];
        for (int k = l; k <= r; ++k)
        {
            float ratio = (float)(1 + k - l) / (float)(2 + r - l);
            osg::Vec3 dir = refOutline0[k] - refCenters0[k]; dir.normalize();
            outline0[k] = lRef * (1.0f - ratio) + rRef * ratio - dir * (1.0f - abs(0.5f - ratio));
            //outline0[k] = lRef * (1.0f - ratio) + rRef * ratio;
        }
    }

    // Smooth outside part of the gumline
    fitSpline(outline0, &refOutline0, &refCenters0);
    outline.insert(outline.end(), outline0.begin(), outline0.end());
    if (refOutline) refOutline->insert(refOutline->end(), refOutline0.begin(), refOutline0.end());
    if (refCenters) refCenters->insert(refCenters->end(), refCenters0.begin(), refCenters0.end());
    outline0.clear(); refOutline0.clear(); refCenters0.clear();

    // Right end
    osg::Vec3 dir = centerLine[1] - centerLine[0]; dir.normalize(); sideA = dir ^ osg::Z_AXIS;
    osg::Quat q1; q1.makeRotate(sideB, sideA); q1.getRotate(angle, axis);
    for (size_t i = 1; i < 8; ++i)
    {
        osg::Vec3 side = osg::Quat(angle * (float)i / 8.0, axis) * sideB;
        result = centerLine[0] + side * width; if (refOutline) refOutline->push_back(result);
        if (refCenters) refCenters->push_back(centerLine[0]);
        if (checkBorders) result = getIntersectionFromBorders(
            polygons, centerLine[0], result, lastRef, NULL, hOffset, vOffset);
        outline.push_back(result); lastRef = result;
    }
    return outline;
}

std::vector<osg::Vec3> generateGumOutline(const std::vector<osg::Vec3>& refOutline,
                                          const std::vector<osg::Vec3>& refCenters,
                                          float z, float uniform)
{
    std::vector<osg::Vec3> outline; float maxR = 0.0f;
    outline.assign(refOutline.begin(), refOutline.end());
    for (size_t i = 0; i < outline.size(); ++i)
    {
        osg::Vec3 dir = refOutline[i] - refCenters[i]; dir.z() = 0.0f;
        float r = dir.normalize(); if (maxR < r) maxR = r;
        outline[i].z() = z;
    }

    if (uniform > 0.0f)
    {
        for (size_t i = 0; i < outline.size(); ++i)
        {
            osg::Vec3 dir = refOutline[i] - refCenters[i], newPt;
            dir.z() = 0.0f; dir.normalize();

            float cosine = dir * osg::Y_AXIS;
            if (cosine < 0.0f) newPt = refCenters[i] + dir * maxR * (0.5f + 0.5f * (1.0f + cosine));
            else newPt = refCenters[i] + dir * maxR;
            outline[i] = outline[i] * (1.0f - uniform) + newPt * uniform;
        }
    }
    return outline;
}

void refineOutlineWithTeeth(std::vector<osg::Vec3>& outline, const std::vector<osg::Vec3>& refCenters)
{
    for (size_t i = 0; i < outline.size(); ++i)
    {
        osg::Vec3 start = outline[i], end = refCenters[i];
        for (std::map<int, osg::observer_ptr<osg::Node>>::iterator itr = g_toothMap.begin();
             itr != g_toothMap.end(); ++itr)
        {
            //osgVerse::IntersectionResult result =
            //    osgVerse::findNearestIntersection(itr->second.get(), start, end);
            //if (!result.drawable) continue;
        }
    }
}

void createOutlineGeometry(osg::Geode* geode, const std::vector<osg::Vec3>& outline)
{
    osg::Vec3Array* va = new osg::Vec3Array;
    osg::Vec4Array* ca = new osg::Vec4Array; ca->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    for (size_t i = 0; i < outline.size(); ++i) va->assign(outline.begin(), outline.end());

    osg::Geometry* geom = new osg::Geometry;
    geom->setVertexArray(va);
    geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    geom->addPrimitiveSet(new osg::DrawArrays(GL_LINE_LOOP, 0, va->size()));
    geom->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(4.0f));
    geode->addDrawable(geom);
}

bool createDelaunay(osg::Geode* geode)
{
    osgVerse::PointList3D points; osgVerse::PointList2D points2D;
    osgVerse::EdgeList edges; osg::BoundingBox bb;
    std::map<int, osg::Vec3> centerMap; float maxTeethRadius = 0.0f;
    for (std::map<int, std::vector<osg::Vec3>>::iterator itr = g_borderMap.begin();
         itr != g_borderMap.end(); ++itr)
    {
        std::vector<osg::Vec3>& border = itr->second;
        size_t startIdx = 0, endIdx = 0; osg::Vec3& center = centerMap[itr->first];
        for (size_t i = 0; i < border.size(); ++i) center += border[i];
        center *= 1.0f / (float)border.size();

        std::pair<osg::Vec4, osg::Quat>& obb = g_obbMap[itr->first];
        if (maxTeethRadius < obb.first.w()) maxTeethRadius = obb.first.w();
        //std::cout << obb.first << "\n";

        for (int nn = 0; nn < 4; ++nn)
        {
            for (size_t i = 0; i < border.size(); ++i)
            {
                size_t idx = points.size();
                osg::Vec3 dir = border[i] - center; float len = dir.normalize();
                osg::Vec3 point = center + dir * (len * (float)(nn + 1) * 0.25f);
                point.z() -= (1.0f - (float)(nn + 1) * 0.25f);

                if (i == border.size() - 1) endIdx = idx;
                else if (i == 0) startIdx = idx;
                points.push_back(point); bb.expandBy(point);
                points2D.push_back(osgVerse::PointType2D(osg::Vec2(point.x(), point.y()), idx));
                //if (i > 0) edges.push_back(osgVerse::EdgeType(idx - 1, idx));
            }
            //edges.push_back(osgVerse::EdgeType(endIdx, startIdx));
        }
        //std::cout << "Edges of teeth " << itr->first << ": " << startIdx << " - " << endIdx << "\n";
    }

    // Compute center-line
    std::vector<Eigen::Vector2d> curveSamples;
    for (std::map<int, std::vector<osg::Vec3>>::iterator itr = g_borderMap.begin();
        itr != g_borderMap.end(); ++itr)
    {
        osg::Vec3& center = centerMap[itr->first];
        curveSamples.push_back(Eigen::Vector2d(center.x(), center.y()));
    }

    float baseZ = (g_upGum ? bb.zMax() + 5.0f : bb.zMin() - 5.0f);
    std::vector<osg::Vec3> refOutline, refCenters;
    std::vector<osg::Vec3> centerline = generateGumCenterline(curveSamples, baseZ, 2.5f);
    std::vector<osg::Vec3> outlineTlist[8];
    osg::Vec2 outlineOffsets[8] = {
        osg::Vec2(0.4f, 0.1f), osg::Vec2(0.36f, 0.14f), osg::Vec2(0.32f, 0.18f), osg::Vec2(0.28f, 0.2f),
        osg::Vec2(0.24f, 0.18f), osg::Vec2(0.2f, 0.1f), osg::Vec2(0.16f, 0.05f), osg::Vec2(0.12f, 0.02f)
    };
    for (int i = 0; i < 8; ++i)
    {
        outlineTlist[i] = generateGumOutline(
            centerline, maxTeethRadius + 0.5f, true, NULL, NULL,
            outlineOffsets[i].x(), g_upGum ? -outlineOffsets[i].y() : outlineOffsets[i].y());
    }
    std::vector<osg::Vec3> outlineT = generateGumOutline(
        centerline, maxTeethRadius + 0.5f, true, &refOutline, &refCenters, 0.1f, 0.0f);
    std::vector<osg::Vec3> outlineB = generateGumOutline(outlineT, refCenters, baseZ, 1.0f);
    size_t numOutline = outlineT.size(), numLayers = 9;

    std::vector<std::vector<osg::Vec3>> outlineM(numLayers);
    for (size_t layer = 0; layer < numLayers; ++layer)
    {
        float ratio = (float)(layer + 1) / (float)(numLayers + 1);
        outlineM[layer] = generateGumOutline(outlineTlist[0], refCenters, 0.0f, 1.0f - ratio * ratio);
        for (size_t i = 0; i < numOutline; ++i)
        {
            osg::Vec3& b = outlineB[i], t = outlineTlist[0][i];
            outlineM[layer][i].z() = b.z() * (1.0f - ratio) + t.z() * ratio;
        }
        refineOutlineWithTeeth(outlineM[layer], refCenters);
        //createOutlineGeometry(geode, outlineM[layer]);
    }
    //createOutlineGeometry(geode, outlineB);
    //createOutlineGeometry(geode, outlineT);
    createOutlineGeometry(geode, centerline);

    // Find top gumline and then compute top outer boundary
    size_t outerIdx0 = points.size();
    if (!outlineT.empty())
    {
        points.push_back(outlineT[0]);
        points2D.push_back(osgVerse::PointType2D(  // FIXME: will cause delaunay failed, use refOutline
            osg::Vec2(outlineT[0][0], outlineT[0][1]), outerIdx0));
        edges.push_back(osgVerse::EdgeType(outerIdx0 + (numOutline - 1), outerIdx0));
    }

    for (size_t i = 1; i < outlineT.size(); ++i)
    {
        points.push_back(outlineT[i]);
        points2D.push_back(osgVerse::PointType2D(  // FIXME: will cause delaunay failed, use refOutline
            osg::Vec2(outlineT[i][0], outlineT[i][1]), outerIdx0 + i));
        edges.push_back(osgVerse::EdgeType(outerIdx0 + (i - 1), outerIdx0 + i));
    }
    std::cout << "Edges of gumline: " << outerIdx0 << " - " << (outerIdx0 + (numOutline - 1)) << "\n";

    std::vector<size_t> indices = osgVerse::GeometryAlgorithm::delaunayTriangulation(points2D, edges);
    if (g_upGum) std::reverse(indices.begin(), indices.end());

    // Generate side geometry
    osg::DrawElementsUInt* de0 = static_cast<osg::DrawElementsUInt*>(g_gumline->getPrimitiveSet(0));
    osg::Vec3Array* va0 = static_cast<osg::Vec3Array*>(g_gumline->getVertexArray());
    osg::Vec2Array* ta0 = static_cast<osg::Vec2Array*>(g_gumline->getTexCoordArray(0));
    osg::Vec4Array* ca0 = static_cast<osg::Vec4Array*>(g_gumline->getColorArray());
    outlineM.insert(outlineM.begin(), outlineB);
    for (int i = 0; i < 8; ++i) outlineM.push_back(outlineTlist[i]);
    outlineM.push_back(outlineT); va0->clear(); de0->clear();

    for (size_t i = 0; i < outlineM.size(); ++i)
    {
        std::vector<osg::Vec3>& outline = outlineM[i]; size_t maxIdx = outline.size();
        for (size_t j = 0; j < maxIdx; ++j)
        {
            ta0->push_back(osg::Vec2((float)j / (float)(maxIdx - 1),
                                     (float)i / (float)(outlineM.size() - 1)));
            va0->push_back(outline[j]);
        }
        if (i == 0) continue;

        size_t index1 = i * maxIdx, index0 = (i - 1) * maxIdx;
        for (size_t j = 0; j < maxIdx; ++j)
        {
            if (g_upGum)
            {
                de0->push_back(index1 + j); de0->push_back(index1 + ((j + 1) % maxIdx));
                de0->push_back(index0 + ((j + 1) % maxIdx)); de0->push_back(index0 + j);
            }
            else
            {
                de0->push_back(index0 + j); de0->push_back(index0 + ((j + 1) % maxIdx));
                de0->push_back(index1 + ((j + 1) % maxIdx)); de0->push_back(index1 + j);
            }
        }
    }

    size_t numVA = va0->size(); ta0->resize(numVA); ca0->resize(numVA);
    for (size_t i = 0; i < numVA; ++i) (*ca0)[i] = osg::Vec4(0.82f, 0.58f, 0.59f, 1.0f);
    de0->dirty(); va0->dirty(); ta0->dirty(); ca0->dirty(); g_gumline->dirtyBound();
    
    // Generate top-cap geometry
    osg::DrawElementsUInt* de = static_cast<osg::DrawElementsUInt*>(g_delaunay->getPrimitiveSet(0));
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(g_delaunay->getVertexArray());
    osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(g_delaunay->getTexCoordArray(0));
    osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(g_delaunay->getColorArray());
    if (indices.empty())
    {
        points.erase(points.begin(), points.begin() + outerIdx0);
        de->setMode(GL_LINE_STRIP);
    }
    else
    {
        de->setMode(GL_TRIANGLES);
        de->assign(indices.begin(), indices.end());
    }
    
    numVA = points.size();
    if (va->size() != numVA) { va->resize(numVA); ta->resize(numVA); ca->resize(numVA); }
    for (size_t i = 0; i < numVA; ++i)
    {
        (*va)[i] = points[i]; (*ta)[i] = osg::Vec2(0.5f, 1.0f);
        (*ca)[i] = osg::Vec4(0.82f, 0.58f, 0.59f, 1.0f);
    }
    de->dirty(); va->dirty(); ta->dirty(); ca->dirty(); g_delaunay->dirtyBound();

    //osgUtil::Simplifier simplifier(5.0f); simplifier.simplify(*g_delaunay);
    osgUtil::SmoothingVisitor smv; geode->accept(smv); return true;
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc < 2) return 0;

    std::string dirName = argv[1]; char end = *dirName.rbegin();
    if (end != '/' && end != '\\') dirName += osgDB::getNativePathSeparator();

    osgDB::DirectoryContents contents = osgDB::getDirectoryContents(dirName);
    for (size_t i = 0; i < contents.size(); ++i)
    {
        std::string fileName = contents[i], line;
        std::string ext = osgDB::getFileExtension(fileName), checkName = fileName;
        size_t idPrefix = fileName.find("tooth");
        if (idPrefix != std::string::npos) checkName = fileName.substr(idPrefix + 5);
        if (ext != "obj") continue;

        int id = std::stoi(checkName);
        if (g_upGum) { if (id >= 30) continue; } else { if (id < 30) continue; }
        if (checkName.find("border") != std::string::npos)
        {
            std::ifstream in(dirName + fileName);
            std::vector<osg::Vec3>& border = g_borderMap[id];
            while (std::getline(in, line))
            {
                std::stringstream ss; ss << line;
                std::string tag; osg::Vec3 pos;
                ss >> tag >> pos[0] >> pos[1] >> pos[2];
                if (border.size() > 0)
                {
                    osg::Vec3 last = border.back();
                    if (last != pos) border.push_back(pos);
                }
                else border.push_back(pos);
            }
            if (border.back() == border.front()) border.pop_back();
        }
        else
        {
            g_toothMap[id] = osgDB::readNodeFile(dirName + fileName + ".-90,0,0.rot");
            root->addChild(g_toothMap[id].get());

            std::pair<osg::Vec4, osg::Quat>& pair = g_obbMap[id];
            osgVerse::BoundingVolumeVisitor bvv;
            g_toothMap[id]->accept(bvv);

            osg::BoundingBox bb = bvv.computeOBB(pair.second);
            osg::Vec3 extent = bb._max - bb._min; osg::Vec3 corners[8];
            corners[0] = pair.second * bb._min;
            corners[1] = pair.second * (bb._min + osg::Vec3(extent[0], 0.0f, 0.0f));
            corners[2] = pair.second * (bb._min + osg::Vec3(0.0f, extent[1], 0.0f));
            corners[3] = pair.second * (bb._min + osg::Vec3(extent[0], extent[1], 0.0f));
            corners[4] = pair.second * (bb._min + osg::Vec3(0.0f, 0.0f, extent[2]));
            corners[5] = pair.second * (bb._min + osg::Vec3(extent[0], 0.0f, extent[2]));
            corners[6] = pair.second * (bb._min + osg::Vec3(0.0f, extent[1], extent[2]));
            corners[7] = pair.second * bb._max;

            osg::Vec3 dir[3] = { corners[1] - corners[0], corners[2] - corners[0], corners[4] - corners[0] };
            float bestCosine = 0.0f, minCosine = 1.0f, teethSize = 0.0f; int bestK = 0;
            for (int k = 0; k < 3; ++k)
            {
                dir[k].normalize(); float cosine = 1.0f - abs(dir[k] * osg::Z_AXIS);
                if (cosine < minCosine) { minCosine = cosine; bestK = k; }
            }

            if (bestK == 0) teethSize = osg::maximum(extent[1], extent[2]);
            else if (bestK == 1) teethSize = osg::maximum(extent[0], extent[2]);
            else teethSize = osg::maximum(extent[0], extent[1]);
            pair.first = osg::Vec4(dir[bestK], teethSize * 0.5f);
        }
    }

    osg::ref_ptr<osg::Geode> borderNode = new osg::Geode;
    for (std::map<int, std::vector<osg::Vec3>>::iterator itr = g_borderMap.begin();
         itr != g_borderMap.end(); ++itr)
    {
        std::vector<osg::Vec3>& border = itr->second;
        osg::Vec3Array* va = new osg::Vec3Array;
        osg::Vec4Array* ca = new osg::Vec4Array; ca->push_back(osg::Vec4());
        for (size_t i = 0; i < border.size(); ++i) { va->push_back(border[i]); }
        
        osg::Geometry* geom = new osg::Geometry;
        geom->setName(std::to_string(itr->first));
        geom->setUseDisplayList(false);
        geom->setUseVertexBufferObjects(true);
        geom->setVertexArray(va);
        geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_OVERALL);
        geom->addPrimitiveSet(new osg::DrawArrays(GL_LINE_LOOP, 0, va->size()));
        geom->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(4.0f));
        geom->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        borderNode->addDrawable(geom);
    }
    borderNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    borderNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    root->addChild(borderNode.get());

    osg::ref_ptr<osg::Geode> delaunayNode = new osg::Geode;
    {
        g_delaunay = new osg::Geometry;
        g_delaunay->setUseDisplayList(false);
        g_delaunay->setUseVertexBufferObjects(true);
        g_delaunay->setVertexArray(new osg::Vec3Array); g_delaunay->setTexCoordArray(0, new osg::Vec2Array);
        g_delaunay->setColorArray(new osg::Vec4Array); g_delaunay->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        g_delaunay->addPrimitiveSet(new osg::DrawElementsUInt(GL_TRIANGLES));
        delaunayNode->addDrawable(g_delaunay.get());

        g_gumline = new osg::Geometry;
        g_gumline->setUseDisplayList(false);
        g_gumline->setUseVertexBufferObjects(true);
        g_gumline->setVertexArray(new osg::Vec3Array); g_gumline->setTexCoordArray(0, new osg::Vec2Array);
        g_gumline->setColorArray(new osg::Vec4Array); g_gumline->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        g_gumline->addPrimitiveSet(new osg::DrawElementsUInt(GL_QUADS));
        delaunayNode->addDrawable(g_gumline.get());
        createDelaunay(delaunayNode.get());
    }
    //delaunayNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    root->addChild(delaunayNode.get());
    osgDB::writeNodeFile(*delaunayNode, "../result.obj");

    // Handler related
    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    std::map<int, osg::observer_ptr<osg::Node>>::iterator teethItr = g_toothMap.end();
    std::map<int, std::vector<osg::Vec3>>::iterator borderItr = g_borderMap.end();

    handler->addKeyUpCallback(osgGA::GUIEventAdapter::KEY_Left, [&](int key) {
        if (teethItr != g_toothMap.begin()) { teethItr--; borderItr--; }
        else { teethItr = g_toothMap.end(); borderItr = g_borderMap.end(); }
        for (size_t i = 0; i < borderNode->getNumDrawables(); ++i)
        {
            osg::Geometry* g = borderNode->getDrawable(i)->asGeometry();
            osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(g->getColorArray());
            if (g->getName() != std::to_string(borderItr->first)) ca->back() = osg::Vec4();
            else ca->back() = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f); ca->dirty();
        }
    });

    handler->addKeyUpCallback(osgGA::GUIEventAdapter::KEY_Right, [&](int key) {
        if (teethItr != g_toothMap.end()) { teethItr++; borderItr++; }
        else { teethItr = g_toothMap.begin(); borderItr = g_borderMap.begin(); }
        for (size_t i = 0; i < borderNode->getNumDrawables(); ++i)
        {
            osg::Geometry* g = borderNode->getDrawable(i)->asGeometry();
            osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(g->getColorArray());
            if (g->getName() != std::to_string(borderItr->first)) ca->back() = osg::Vec4();
            else ca->back() = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f); ca->dirty();
        }
    });

    handler->addKeyDownCallback('h', [&](int key) {
        if (teethItr == g_toothMap.end()) return;
        osg::MatrixTransform* mt = teethItr->second->asTransform()->asMatrixTransform();
        std::vector<osg::Vec3>& border = borderItr->second; osg::Matrix m = mt->getMatrix();
        for (size_t i = 0; i < border.size(); ++i) border[i].x() -= 0.01f;
        if (!createDelaunay(delaunayNode.get())) return;
        m.setTrans(m.getTrans() - osg::Vec3(0.01f, 0.0f, 0.0f)); mt->setMatrix(m);
    });

    handler->addKeyDownCallback('k', [&](int key) {
        if (teethItr == g_toothMap.end()) return;
        osg::MatrixTransform* mt = teethItr->second->asTransform()->asMatrixTransform();
        std::vector<osg::Vec3>& border = borderItr->second; osg::Matrix m = mt->getMatrix();
        for (size_t i = 0; i < border.size(); ++i) border[i].x() += 0.01f;
        if (!createDelaunay(delaunayNode.get())) return;
        m.setTrans(m.getTrans() + osg::Vec3(0.01f, 0.0f, 0.0f)); mt->setMatrix(m);
    });

    handler->addKeyDownCallback('j', [&](int key) {
        if (teethItr == g_toothMap.end()) return;
        osg::MatrixTransform* mt = teethItr->second->asTransform()->asMatrixTransform();
        std::vector<osg::Vec3>& border = borderItr->second; osg::Matrix m = mt->getMatrix();
        for (size_t i = 0; i < border.size(); ++i) border[i].y() += 0.01f;
        if (!createDelaunay(delaunayNode.get())) return;
        m.setTrans(m.getTrans() + osg::Vec3(0.0f, 0.01f, 0.0f)); mt->setMatrix(m);
    });

    handler->addKeyDownCallback('u', [&](int key) {
        if (teethItr == g_toothMap.end()) return;
        osg::MatrixTransform* mt = teethItr->second->asTransform()->asMatrixTransform();
        std::vector<osg::Vec3>& border = borderItr->second; osg::Matrix m = mt->getMatrix();
        for (size_t i = 0; i < border.size(); ++i) border[i].y() -= 0.01f;
        if (!createDelaunay(delaunayNode.get())) return;
        m.setTrans(m.getTrans() - osg::Vec3(0.0f, 0.01f, 0.0f)); mt->setMatrix(m);
    });

    handler->addKeyUpCallback(osgGA::GUIEventAdapter::KEY_Tab, [&](int key) {
        for (std::map<int, osg::observer_ptr<osg::Node>>::iterator itr = g_toothMap.begin();
             itr != g_toothMap.end(); ++itr) itr->second->setNodeMask(~itr->second->getNodeMask());
    });

    // Start viewer
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(handler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
