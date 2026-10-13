#include <osg/ComputeBoundsVisitor>
#include <cmath>
#include "OrbitObjectManipulator.h"
using namespace osgVerse;

namespace
{
    // osg::ComputeBoundsVisitor seeds its matrix stack with identity, so accepting a sub-node gives
    // the box in the node's *parent* frame. Seeding the stack with the parent's world matrix makes
    // the whole traversal world relative, which is what callers expect.
    struct WorldBoundsVisitor : public osg::ComputeBoundsVisitor
    {
        WorldBoundsVisitor(const osg::Matrix& parentToWorld)
            : osg::ComputeBoundsVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
        { _matrixStack.push_back(parentToWorld); }
    };
}

osg::BoundingBox OrbitObjectManipulator::computeOrbitBox(osg::Node* node) const
{
    if (node == NULL) return osg::BoundingBox();

    osg::Matrix parentToWorld;
    if (node->getNumParents() > 0)
    {
        osg::MatrixList worldMatrices = node->getParent(0)->getWorldMatrices();
        if (!worldMatrices.empty()) parentToWorld = osg::Matrix(worldMatrices[0]);
    }

    WorldBoundsVisitor wbv(parentToWorld);
    node->accept(wbv);
    return wbv.getBoundingBox();
}

double OrbitObjectManipulator::computeFitDistance(const osg::BoundingBox& bb, const osg::Camera* camera,
                                                  double ratio) const
{
    if (!bb.valid()) return 1e-6;

    // Camera basis of the current view (the rotation is kept when focusing, so this basis is also the
    // basis of the resulting view). 'forward' points from the eye towards the orbit center.
    osg::Vec3d right = _rotation * osg::Vec3d(1.0, 0.0, 0.0);
    osg::Vec3d up = _rotation * osg::Vec3d(0.0, 1.0, 0.0);
    osg::Vec3d forward = _rotation * osg::Vec3d(0.0, 0.0, -1.0);

    // Tangents of the horizontal / vertical half field of view (a 45 degree vertical FOV is assumed
    // when no camera is available). An orthographic camera cannot be framed by distance at all.
    double tanX = 0.4142, tanY = 0.4142;
    if (camera != NULL)
    {
        double left, r, bottom, top, zNear, zFar;
        if (camera->getProjectionMatrixAsFrustum(left, r, bottom, top, zNear, zFar))
        {
            if (zNear > 1e-6)
            {
                tanX = 0.5 * (r - left) / zNear; tanY = 0.5 * (top - bottom) / zNear;
            }
        }
        else if (camera->getProjectionMatrixAsOrtho(left, r, bottom, top, zNear, zFar))
            return 3.0 * (bb._max - bb._min).length() * 0.5;
    }
    if (tanX < 1e-6) tanX = 1e-6; if (tanY < 1e-6) tanY = 1e-6;
    if (ratio < 1e-3) ratio = 1e-3;

    // A box corner projects to lateral / (depth * tan) in normalized viewport units, where the
    // viewport edge is 1. Requiring that value to stay within +/- 'ratio' for every corner makes the
    // box cover 'ratio' of the viewport, i.e. depth >= lateral / (ratio * tan).
    const double invRatio = 1.0 / ratio;
    const osg::Vec3d center = bb.center();
    double dist = 0.0;
    for (unsigned int i = 0; i < 8; ++i)
    {
        osg::Vec3d p = osg::Vec3d(bb.corner(i)) - center;
        double a = p * right, b = p * up, w = p * forward;
        double da = invRatio * std::fabs(a) / tanX - w;
        double db = invRatio * std::fabs(b) / tanY - w;
        if (da > dist) dist = da; if (db > dist) dist = db;
    }
    return (dist > 1e-6) ? dist : 1e-6;
}

bool OrbitObjectManipulator::focusOnBox(const osg::BoundingBox& bb, const osg::Camera* camera,
                                        double ratio)
{
    if (!bb.valid()) return false;

    // The base manipulator clamps wheel-zoom with a minimum distance of 0.05 * model size, where the
    // model is the whole attached scene. For a large scene that minimum is far bigger than the
    // distance needed to look at one small object, so zooming in would not zoom but instead "push the
    // camera forward" (OrbitManipulator::zoomModel), driving the orbit center through the model.
    // Re-anchor the minimum to the focused object so zooming behaves normally around it.
    double radius = (bb._max - bb._min).length() * 0.5;
    double minDist = radius * 0.05; if (minDist < 1e-3) minDist = 1e-3;
    setMinimumDistance(minDist, false);

    // Move the orbit center to the object's center and adjust the distance so the object's bounding
    // box fills 'ratio' of the view. The rotation (view direction and up vector) is left untouched,
    // which makes the eye move together with the center.
    setCenter(bb.center());
    setDistance(computeFitDistance(bb, camera, ratio));
    return true;
}

bool OrbitObjectManipulator::focusOnObject(osg::Node* node, const osg::Camera* camera, double ratio)
{
    if (node == NULL) return false;

    osg::BoundingBox bb = computeOrbitBox(node);
    if (!bb.valid()) return false;

    setOrbitObject(node);
    return focusOnBox(bb, camera, ratio);
}

bool OrbitObjectManipulator::refreshOrbitCenter()
{
    if (!_orbitObject.valid()) return false;

    osg::BoundingBox bb = computeOrbitBox(_orbitObject.get());
    if (!bb.valid()) return false;

    setCenter(bb.center());
    return true;
}
