#include <osg/Notify>
#include <osg/io_utils>
#include <osgUtil/LineSegmentIntersector>
#include <iostream>
#include "EarthManipulator.h"
using namespace osgVerse;

static double g_distanceToCenter = 0.0;

EarthManipulator::EarthManipulator()
:   _viewer(NULL), _tilt(0.0f), _throwAllowed(true), _thrown(false), _locked(false)
{
    _tiltCenter.set(0.0, 0.0, -DBL_MAX);
    _rotateAxis.set(0.0, 0.0, -DBL_MAX);
    _intersectionMask = 0xffffffff;
    setEllipsoid(new osg::EllipsoidModel);

    // Animation controllers
    _playMode = ONCE;
    _animationSpeed = 1.0f;
    _animationStartTime = 0.0f;
    _animationRunning = false;
    _isUserAnimation = true;
    _isReversingInLoop = false;
    _animationDistance = 0.0;
    _animationTilt = 0.0f;
}

EarthManipulator::~EarthManipulator()
{
}

void EarthManipulator::getUsage(osg::ApplicationUsage& usage) const
{
}

void EarthManipulator::setByMatrix(const osg::Matrixd& mat)
{ setByInverseMatrix(osg::Matrixd::inverse(mat)); }

void EarthManipulator::setByInverseMatrix(const osg::Matrixd& mat)
{
    osg::Vec3d eye, viewpoint, up;
    if (_node.valid()) mat.getLookAt(eye, viewpoint, up, _node->getBound().radius());
    else mat.getLookAt(eye, viewpoint, up);  // Avoid too near eye & center in huge scene

    //computePosition( eye, viewpoint, up );
    setByEye(eye, 0.0f);
}

osg::Matrixd EarthManipulator::getManipulatorMatrix(bool withTilt) const
{
    osg::Matrixd matrix;
    if (_animationRunning)
    {
        matrix.makeTranslate(0.0f, 0.0f, _animationDistance);
        if (withTilt) matrix.postMultRotate(_animationTiltRotation);
        matrix.postMultRotate(_initRotation);
        matrix.postMultTranslate(_center);
        matrix.postMultRotate(_worldRotation);
        matrix.postMultRotate(_animationRotation);
        matrix.postMultTranslate(_worldCenter);
    }
    else
    {
        matrix.makeTranslate(0.0f, 0.0f, _distance);
        if (withTilt) matrix.postMultRotate(_tiltRotation);
        matrix.postMultRotate(_initRotation);
        matrix.postMultTranslate(_center);
        matrix.postMultRotate(_worldRotation);
        matrix.postMultTranslate(_worldCenter);
    }
    return matrix;
}

osg::Matrixd EarthManipulator::getViewMatrix(bool withTilt) const
{
    return osg::Matrixd::inverse(getManipulatorMatrix(withTilt));
    /*osg::Matrixd matrix;
    if ( _animationRunning )
    {
        matrix.makeTranslate( 0.0f, 0.0f, -_animationDistance );
        matrix.preMultRotate( _animationTiltRotation.inverse() );
        matrix.preMultRotate( _initRotation.inverse() );
        matrix.preMultTranslate( -_center );
        matrix.preMultRotate( _worldRotation.inverse() );
        matrix.preMultRotate( _animationRotation.inverse() );
        matrix.preMultTranslate( -_worldCenter );
    }
    else
    {
        matrix.makeTranslate( 0.0f, 0.0f, -_distance );
        matrix.preMultRotate( _tiltRotation.inverse() );
        matrix.preMultRotate( _initRotation.inverse() );
        matrix.preMultTranslate( -_center );
        matrix.preMultRotate( _worldRotation.inverse() );
        matrix.preMultTranslate( -_worldCenter );
    }
    return matrix;*/
}

void EarthManipulator::setNode(osg::Node* node)
{
    _node = node;
    if (node) _worldCenter = node->getBound().center();
    if (getAutoComputeHomePosition()) computeHomePosition();
}

void EarthManipulator::setEllipsoid(osg::EllipsoidModel* ellipsoid)
{
    if (ellipsoid)
    {
        _ellipsoid = ellipsoid;
        _distance = 4.0 * _ellipsoid->getRadiusPolar();
    }
}

void EarthManipulator::setByEye(const osg::Vec3d& eye, float doa)
{
    // FIXME: have to reset tilt here as we don't have better solutions...
    _tiltRotation.set(0.0, 0.0, 0.0, 1.0);
    _tilt = 0.0;

    osg::Quat new_rotate;
    double new_distance;
    makePositionFromEye(new_rotate, new_distance, eye, 0.0, doa);

    _worldRotation = _worldRotation * new_rotate;
    _distance = new_distance;

    // Should a little tilt be good?
    calcTiltCenter(false);
    _tilt = osg::PI_4;
    makeTiltRotation(_tiltRotation, _tilt, osg::X_AXIS);
}

void EarthManipulator::setByEye(double latitude, double longitude, double height, float doa)
{
    osg::Vec3d eye;
    _ellipsoid->convertLatLongHeightToXYZ(latitude, longitude, height,
                                          eye.x(), eye.y(), eye.z());
    eye += _worldCenter;
    setByEye(eye, doa);
}

void EarthManipulator::moveTo(osg::Vec3d vector, double deltaDistance, double frames)
{
    osg::Matrixd matrix = makeRotationMatrix();
    osg::Vec3d negLv = osg::Z_AXIS * matrix; negLv.normalize();
    vector -= _worldCenter; vector.normalize();

    // Compute rotation from current eye vectorition to selected one
    osg::Vec3d axis = negLv ^ vector;
    double angle = atan2(axis.length(), vector * negLv);
    axis.normalize();

    osg::Quat rotation;
    rotation.makeRotate(angle, axis);

    // Compute rotation to fit the tilt
    matrix.postMultRotate(rotation);
    makeBestLookingRotation(rotation, matrix, getInverseMatrix());

    _internalControlPoints.clear();
    _internalControlPoints.insert(new ControlPoint(0.0, osg::Quat(), _distance, 0.0f));
    _internalControlPoints.insert(new ControlPoint(frames, rotation, _distance + deltaDistance, 0.0f));
    startAnimation(USE_REFERENCE_TIME, false);
}

void EarthManipulator::computeHomePosition()
{
    osg::BoundingSphere boundingSphere(_worldCenter, _ellipsoid->getRadiusPolar() * 2.0f);
    if (boundingSphere.valid()) setHomePosition(true, boundingSphere);
    else osgGA::CameraManipulator::computeHomePosition();
}

void EarthManipulator::home(double)
{
    if (getAutoComputeHomePosition()) computeHomePosition();
    computePosition(_homeEye, _homeCenter, _homeUp);
}

void EarthManipulator::home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{ home(ea.getTime()); }

void EarthManipulator::init(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    if (!_viewer) _viewer = dynamic_cast<osgViewer::View*>(&us);
    flushMouseEventStack();
}

bool EarthManipulator::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    if (ea.getHandled() || ea.getModKeyMask() > 0) return false;
    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::FRAME:
        if (_animationRunning)
        {
            _thrown = false;
            if (_viewer)
            {
                advanceAnimation(_viewer->getFrameStamp());
                us.requestRedraw();
            }
        }
        else if (_thrown)
        {
            if (calcMovement(true)) us.requestRedraw();
        }
        g_distanceToCenter = getDistance();
        return false;

    case osgGA::GUIEventAdapter::PUSH:
        flushMouseEventStack();
        addMouseEventAndUpdate(ea, us);
        if (ea.getButtonMask() != osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
            performRotateAxis(_ga_t0->getXnormalized(), _ga_t0->getYnormalized());
        return true;

    case osgGA::GUIEventAdapter::RELEASE:
        if (!ea.getButtonMask())
        {
            if (isMouseMoving())
            {
                if (calcMovement())
                {
                    us.requestRedraw();
                    us.requestContinuousUpdate(true);
                    _thrown = _throwAllowed;
                }
            }
            else
            {
                flushMouseEventStack();
                addMouseEventAndUpdate(ea, us);
            }
        }
        else
        {
            flushMouseEventStack();
            addMouseEventAndUpdate(ea, us);
        }
        return true;

    case osgGA::GUIEventAdapter::DOUBLECLICK:
        stopAnimation();
        if (calcDoubleClickMotion(ea)) us.requestRedraw();
        return true;

    case osgGA::GUIEventAdapter::DRAG:
        addMouseEventAndUpdate(ea, us);
        return true;

    case osgGA::GUIEventAdapter::SCROLL:
        if (calcScrollingMotion(ea.getScrollingMotion())) us.requestRedraw();
        return true;

    case osgGA::GUIEventAdapter::KEYDOWN:
        if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Space)
        {
            flushMouseEventStack();
            stopAnimation(false);
            home(ea, us);
            us.requestRedraw();
            us.requestContinuousUpdate(false);
            _thrown = false;
            return true;
        }
        else if (ea.getKey() == 'p')
        {
            if (isAnimationRunning()) stopAnimation();
            else startAnimation();
        }
        else if (ea.getKey() == 'o')
        {
            std::cout << _worldRotation << "; " << _distance << "; " << _tilt << std::endl;
        }
        /*else if ( ea.getKey()=='p' )
        {
            //if ( isAnimationRunning() ) stopAnimation();
            //else startAnimation();
            std::cout << _worldRotation << "; " << _distance << std::endl;
        }
        else if ( ea.getKey()=='a' )
        {
            stopAnimation();
            osg::Vec3d vec = osg::Vec3d(-2.72643e+006, 5.12271e+006, 2.63787e+006);
            moveTo( vec, 6800.0-_distance, 120.0 );
        }
        else
        {
            switch ( ea.getKey() )
            {
            case osgGA::GUIEventAdapter::KEY_Left:
                performPan( ea.getXnormalized(), ea.getYnormalized(), 0.002, 0.0 );
                break;
            case osgGA::GUIEventAdapter::KEY_Right:
                performPan( ea.getXnormalized(), ea.getYnormalized(), -0.002, 0.0 );
                break;
            case osgGA::GUIEventAdapter::KEY_Up:
                performPan( ea.getXnormalized(), ea.getYnormalized(), 0.0, -0.002 );
                break;
            case osgGA::GUIEventAdapter::KEY_Down:
                performPan( ea.getXnormalized(), ea.getYnormalized(), 0.0, 0.002 );
                break;
            case osgGA::GUIEventAdapter::KEY_Insert:
                performScale( ea.getXnormalized(), ea.getYnormalized(), 0.0, -0.01 );
                break;
            case osgGA::GUIEventAdapter::KEY_Delete:
                performScale( ea.getXnormalized(), ea.getYnormalized(), 0.0, 0.01 );
                break;
            case osgGA::GUIEventAdapter::KEY_Home:
                performRotateAxis( ea.getXnormalized(), ea.getYnormalized() );
                performHRotate( ea.getXnormalized(), ea.getYnormalized(), 0.05, 0.0 );
                break;
            case osgGA::GUIEventAdapter::KEY_End:
                performRotateAxis( ea.getXnormalized(), ea.getYnormalized() );
                performHRotate( ea.getXnormalized(), ea.getYnormalized(), -0.05, 0.0 );
                break;
            case osgGA::GUIEventAdapter::KEY_Page_Up:
                performRotateAxis( ea.getXnormalized(), ea.getYnormalized() );
                performVRotate( ea.getXnormalized(), ea.getYnormalized(), 0.0, -0.05 );
                break;
            case osgGA::GUIEventAdapter::KEY_Page_Down:
                performRotateAxis( ea.getXnormalized(), ea.getYnormalized() );
                performVRotate( ea.getXnormalized(), ea.getYnormalized(), 0.0, 0.05 );
                break;
            default: break;
            }
            us.requestRedraw();
            us.requestContinuousUpdate(false);
        }*/
        return false;

    default:
        return false;
    }
}

void EarthManipulator::setHomePosition(bool autoOrientation, const osg::BoundingSphere& bs)
{
    if (autoOrientation)
    {
        osg::Quat rotation;
        osg::Vec3 center = bs.center();
        if (center.length() > 0.01f * bs.radius())
        {
            center.normalize();
            rotation.makeRotate(-osg::Y_AXIS, center);
        }
        else
            rotation.makeRotate(-osg::Y_AXIS, osg::X_AXIS);
        osgGA::CameraManipulator::setHomePosition(
            bs.center() + (rotation * osg::Vec3(0.0f, -3.5f * bs.radius(), 0.0f)),
            osg::Vec3(0.0f, 0.0f, 0.0f), rotation * osg::Z_AXIS, _autoComputeHomePosition);
    }
    else
    {
        osgGA::CameraManipulator::setHomePosition(
            bs.center() + osg::Vec3(0.0f, -3.5f * bs.radius(), 0.0f),
            osg::Vec3(0.0f, 0.0f, 0.0f), osg::Z_AXIS, _autoComputeHomePosition);
    }
}

void EarthManipulator::computePosition(const osg::Vec3& eye, const osg::Vec3& center, const osg::Vec3& up)
{
    osg::Vec3 fv(center - eye); fv.normalize();
    osg::Vec3 sv = fv ^ up; sv.normalize();
    osg::Vec3 uv = sv ^ fv; uv.normalize();

    osg::Matrix rotation_matrix(
        sv[0], uv[0], -fv[0], 0.0f,
        sv[1], uv[1], -fv[1], 0.0f,
        sv[2], uv[2], -fv[2], 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    osg::Quat rot = rotation_matrix.getRotate();

    _initEyeDir = -fv;
    _center = _initEyeDir * _ellipsoid->getRadiusPolar();
    _distance = (eye - _center).length();
    _initRotation = rot.inverse();
    _worldRotation.set(0.0, 0.0, 0.0, 1.0);
    _tiltRotation.set(0.0, 0.0, 0.0, 1.0);
    _tilt = 0.0f;
}

bool EarthManipulator::isMouseMoving()
{
    static const float s_velocity = 0.2f;
    if (!_ga_t0 || !_ga_t1) return false;

    float dt = _ga_t0->getTime() - _ga_t1->getTime();
    float dx = _ga_t0->getXnormalized() - _ga_t1->getXnormalized();
    float dy = _ga_t0->getYnormalized() - _ga_t1->getYnormalized();

    float len = sqrtf(dx * dx + dy * dy);
    return (len > dt * s_velocity);
}

bool EarthManipulator::calcMovement(bool throwing)
{
    if (_locked) return false;
    if (!_ga_t0 || !_ga_t1) return false;

    float x0 = _ga_t0->getXnormalized(), y0 = _ga_t0->getYnormalized();
    float x1 = _ga_t1->getXnormalized(), y1 = _ga_t1->getYnormalized();
    float dx = x0 - x1, dy = y0 - y1;
    if (dx == 0.0f && dy == 0.0f) return false;

    unsigned int buttonMask = _ga_t1->getButtonMask();
    if (buttonMask == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
    {
        performPan(x0, y0, dx, dy);
    }
    else if (buttonMask == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON ||
             buttonMask == (osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON | osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON))
    {
        performHRotate(x0, y0, dx, dy);
        if (!throwing) performVRotate(x0, y0, dx, dy);
    }
    else if (buttonMask == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
    {
        if (!throwing)
            performScale(x0, y0, dx, dy);

        // TODO: Rotate while scaling
        /*osg::Matrixd rotation_matrix = makeRotationMatrix();
        sv = osg::X_AXIS * rotation_matrix;
        uv = osg::Y_AXIS * rotation_matrix;
        lv = -osg::Z_AXIS * rotation_matrix;
        sv.normalize(); uv.normalize(); lv.normalize();

        static const double s_rotateFactorForScaling = 0.9;
        double k = _tilt / (osg::PI_2 + 1.0);
        osg::Vec3d target = lv * (1.0 - k) - uv * k;
        if ( s_rotateFactorForScaling>fabs(target*_rotateAxis) )
        {
            osg::Vec3d axis = target ^ _rotateAxis;
            axis.normalize();

            osg::Quat rotate_scale;
            rotate_scale.makeRotate( dy, axis );
            _worldRotation = _worldRotation * rotate_scale;
        }*/
    }
    else
        return false;
    return true;
}

void EarthManipulator::performPan(double x0, double y0, double dx, double dy)
{
    osg::Vec3d rotateAxis0, rotateAxis1;
    bool ok0 = calcIntersectPoint(x0, y0, rotateAxis0);
    bool ok1 = calcIntersectPoint(x0 - dx, y0 - dy, rotateAxis1);
    if (ok0 && ok1)  // Trackball-like rotation on the earth
    {
        rotateAxis0 -= _worldCenter;
        rotateAxis1 -= _worldCenter;

        osg::Vec3d axis = rotateAxis0 ^ rotateAxis1;
        double angle = atan2(axis.length(), rotateAxis1 * rotateAxis0);
        axis.normalize();

        osg::Quat new_rotate(angle, axis);
        _worldRotation = _worldRotation * new_rotate;
    }
    else if (!(ok0 || ok1))  // Free rotation around the look vector
    {
        osg::Matrixd rotateMatrix = makeRotationMatrix();
        osg::Vec3d sv = osg::X_AXIS * rotateMatrix; sv.normalize();
        osg::Vec3d uv = osg::Y_AXIS * rotateMatrix; uv.normalize();
        osg::Vec3d lv = -osg::Z_AXIS * rotateMatrix; lv.normalize();

        osg::Matrixd mvpMatrix = getInverseMatrix();
        if (_viewer && _viewer->getCamera())
            mvpMatrix = mvpMatrix * _viewer->getCamera()->getProjectionMatrix();
        osg::Vec3d eyeRefPole = (_worldCenter + uv * _ellipsoid->getRadiusEquator()) * mvpMatrix;
        osg::Vec3d eyeSouthPole = (_worldCenter - uv * _ellipsoid->getRadiusEquator()) * mvpMatrix;
        osg::Vec3d eyeEastEdge = (_worldCenter + sv * _ellipsoid->getRadiusEquator()) * mvpMatrix;
        osg::Vec3d eyeWestEdge = (_worldCenter - sv * _ellipsoid->getRadiusEquator()) * mvpMatrix;

        osg::Quat new_rotate_x;
        if (y0 > eyeRefPole.y()) new_rotate_x.makeRotate(-dx, lv);
        else if (y0 < eyeSouthPole.y()) new_rotate_x.makeRotate(dx, lv);

        osg::Quat new_rotate_y;
        if (x0 > eyeEastEdge.x()) new_rotate_y.makeRotate(dy, lv);
        else if (x0 < eyeWestEdge.x()) new_rotate_y.makeRotate(-dy, lv);
        _worldRotation = _worldRotation * (new_rotate_x * new_rotate_y);
    }
}

void EarthManipulator::performHRotate(double value)
{
    if (_rotateAxis.z() != -DBL_MAX)
    {
        osg::Quat rotate_azim;
        rotate_azim.makeRotate(value, _rotateAxis);
        _worldRotation = _worldRotation * rotate_azim;
    }
}

void EarthManipulator::performHRotate(double x0, double y0, double dx, double dy)
{
    if (_rotateAxis.z() != -DBL_MAX)  // Rotation around the axis formed by world center and selected point
    {
        osg::Quat rotate_azim;
        rotate_azim.makeRotate(-dx, _rotateAxis);
        _worldRotation = _worldRotation * rotate_azim;
    }
}

void EarthManipulator::performVRotate(double x0, double y0, double dx, double dy)
{
#if 0  // TODO
    if (_tiltCenter.z() != -DBL_MAX)
    {
        // Rotate the world slightly to keep the tilt center unchanged
        osg::Vec3d currCenter;
        bool ok = calcIntersectPoint(0.0f, 0.0f, currCenter);
        if (ok)
        {
            currCenter -= _worldCenter;
            currCenter.normalize();

            osg::Vec3d axis = currCenter ^ _tiltCenter;
            double angle = atan2(axis.length(), currCenter * _tiltCenter);
            axis.normalize();

            osg::Quat rotate_offset(angle, axis);
            _worldRotation = _worldRotation * rotate_offset;
        }
    }
#endif

    _tilt -= dy;
    makeTiltRotation(_tiltRotation, _tilt, osg::X_AXIS);
}

void EarthManipulator::performRotateAxis(double x0, double y0)
{
    calcTiltCenter(false);
#if 0
    // Compute the rotate axis for rotating around a point
    bool ok = calcIntersectPoint(x0, y0, _rotateAxis);
    if (ok)
    {
        _rotateAxis -= _worldCenter;
        _rotateAxis.normalize();
    }
    else
        _rotateAxis.set(0.0, 0.0, -DBL_MAX);
#else
    // Directly set tilt center to rotate axis
    _rotateAxis = _tiltCenter;
    _rotateAxis.normalize();
#endif
}

bool EarthManipulator::calcDoubleClickMotion(const osgGA::GUIEventAdapter& ea)
{
    if (_locked) return false;

    unsigned int buttonMask = ea.getButtonMask();
    if (buttonMask == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
    {
        osg::Vec3d currentPos;
        bool ok = calcIntersectPoint(ea.getXnormalized(), ea.getYnormalized(), currentPos);
        if (ok) moveTo(currentPos, 0.0, 60.0);
    }
    else
        return false;
    return true;
}

bool EarthManipulator::calcScrollingMotion(osgGA::GUIEventAdapter::ScrollingMotion scrollMotion)
{
    if (_locked) return false;
    const float scrollScale = 1.05f;
    switch (scrollMotion)
    {
    case osgGA::GUIEventAdapter::SCROLL_UP:
        _distance /= scrollScale;
        return true;
    case osgGA::GUIEventAdapter::SCROLL_DOWN:
        _distance *= scrollScale;
        return true;
    default:
        return false;
    }
}

bool EarthManipulator::calcIntersectPoint(float x, float y, osg::Vec3d& point, bool showPoint)
{
    bool result = false;
    if (_clickedPointNode.valid())
        _clickedPointNode->setNodeMask(0);
    if (_viewer)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
        if (_world.valid())
        {
            osg::Matrix invViewProj = osg::Matrix::inverse(
                _viewer->getCamera()->getViewMatrix() * _viewer->getCamera()->getProjectionMatrix());
            osg::Vec3d s = osg::Vec3d(x, y, -1.0) * invViewProj;
            osg::Vec3d e = osg::Vec3d(x, y, 1.0) * invViewProj;

            // FIXME: very bad implementation here...
            // As for RTT situation, the earth node will be placed under a camera node, which
            // makes the near/far plane incorrect and thus affect the intersection here.
            // The only way presently is to extend the line as long as possible, as we've done.
            osg::Vec3d dir = e - s; dir.normalize();
            dir = dir * _world->getBound().radius() * 10.0;
            s -= dir; e += dir;

            intersector = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, s, e);
            osgUtil::IntersectionVisitor iv(intersector.get());
            iv.setTraversalMask(_intersectionMask);
            _world->accept(iv);
        }
        else
        {
            intersector = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, x, y);
            osgUtil::IntersectionVisitor iv(intersector.get());
            iv.setTraversalMask(_intersectionMask);
            _viewer->getCamera()->accept(iv);
        }

        if (intersector.valid() && intersector->containsIntersections())
        {
            osgUtil::LineSegmentIntersector::Intersection hit = intersector->getFirstIntersection();
            point = hit.getWorldIntersectPoint(); result = true;

#if true
            double lat, lon, height;
            _ellipsoid->convertXYZToLatLongHeight( point.x(), point.y(), point.z(), lat, lon, height );
            std::cout << hit.nodePath.back()->getName() << ": Lat = " << osg::RadiansToDegrees(lat)
                      << ", Lon = " << osg::RadiansToDegrees(lon) << ", H = " << height << std::endl;
#endif
        }
    }

    if (_clickedPointNode.valid())
    {
        if (result)
        {
            _clickedPointNode->setNodeMask(0xffffffff);
            if (showPoint) _clickedPointNode->setMatrix(osg::Matrix::translate(point));
        }
    }
    return result;
}

bool EarthManipulator::calcTiltCenter(bool useCameraMatrix)
{
    if (_viewer)
    {
        osg::Matrix matrix(_viewer->getCamera()->getProjectionMatrix());
        if (useCameraMatrix)
            matrix.preMult(_viewer->getCamera()->getViewMatrix());
        else
            matrix.preMult(getViewMatrix(true));
        matrix = osg::Matrix::inverse(matrix);

        osg::Vec3d start(osg::Vec3d(0.0, 0.0, -1.0) * matrix);
        osg::Vec3d end(osg::Vec3d(0.0, 0.0, 1.0) * matrix);

        // FIXME: very bad implementation here...
        // As for RTT situation, the earth node will be placed under a camera node, which
        // makes the near/far plane incorrect and thus affect the intersection here.
        // The only way presently is to extend the line as long as possible, as we've done.
        osg::Vec3d dir = end - start; dir.normalize();
        dir = dir * _world->getBound().radius() * 10.0;
        start -= dir; end += dir;

        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector(
            osgUtil::Intersector::MODEL, start, end);
        osgUtil::IntersectionVisitor iv(intersector.get());
        iv.setTraversalMask(_intersectionMask);
        _viewer->getCamera()->accept(iv);

        if (intersector->containsIntersections())
        {
            osgUtil::LineSegmentIntersector::Intersection hit = intersector->getFirstIntersection();
            _tiltCenter = hit.getWorldIntersectPoint();
            _tiltCenter -= _worldCenter;

            // Compute real radius for resetting the viewing offset
            double realRadius = _tiltCenter.length();
            _center = _initEyeDir * realRadius;
            return true;
        }
        else
            _tiltCenter.set(0.0, 0.0, -DBL_MAX);
    }
    return false;
}

void EarthManipulator::makePositionFromEye(osg::Quat& new_rotate, double& new_distance,
                                           const osg::Vec3d& eye, float tilt, float doa)
{
    // Get basic rotation around the earth
    osg::Vec3d lv, uv;
    osg::Vec3d dir = _worldCenter - eye;
    lv = -osg::Z_AXIS * makeRotationMatrix();
    lv.normalize(); dir.normalize();

    osg::Vec3d axis = lv ^ dir;
    double angle = atan2(axis.length(), dir * lv);
    axis.normalize();
    new_rotate.makeRotate(angle, axis);

    osg::Matrixd matrix(_worldRotation * new_rotate);
    if (_animationRunning) matrix.postMultRotate(_animationRotation);

    // Get new look vector and up vector
    osg::Matrixd preMatrix(_initRotation);
    /*if ( tilt!=0.0 )
    {
        osg::Quat userTiltRotation( tilt, osg::X_AXIS );
        preMatrix.preMultRotate( userTiltRotation );
    }
    if ( _animationRunning ) preMatrix.preMultRotate( _animationTiltRotation );
    else preMatrix.preMultRotate( _tiltRotation );*/
    matrix.preMult(preMatrix);

    uv = osg::Y_AXIS * matrix; uv.normalize();
    lv = -osg::Z_AXIS * matrix; lv.normalize();

    // Compute DOA between the north pole projection and the new up vector
    osg::Vec3d np = osg::Z_AXIS - lv * (osg::Z_AXIS * lv);
    if (np.length2() > 0.0)
    {
        np.normalize();
        axis = np ^ uv;
        angle = atan2(axis.length(), uv * np);
        axis.normalize();

        if (doa != angle)
        {
            new_rotate = new_rotate * osg::Quat(-angle, axis);
            new_rotate = new_rotate * osg::Quat(doa, lv);
        }
    }

    // Compute the best looking rotation value according to world rotation and the tilt
    matrix.makeRotate(_worldRotation * new_rotate);
    if (_animationRunning) matrix.postMultRotate(_animationRotation);
    matrix.preMult(preMatrix);
    makeBestLookingRotation(new_rotate, matrix, getInverseMatrix());

    // Reset to calculate distance without considering tilt
    matrix.makeRotate(_worldRotation * new_rotate);
    if (_animationRunning) matrix.postMultRotate(_animationRotation);
    new_distance = (_center * matrix + _worldCenter - eye).length();
}

void EarthManipulator::makeBestLookingRotation(osg::Quat& new_rotate,
                                               const osg::Matrixd& matrix, const osg::Matrixd& view)
{
    osg::Vec3d lowerEdge;
    bool ok = calcIntersectPoint(0.0, -1.0, lowerEdge, false);
    if (ok) lowerEdge = lowerEdge * view;
    else return;

    // Get new up and side vector and make a new rotate to fit the tilt
    osg::Vec3d sv = osg::X_AXIS * matrix; sv.normalize();
    osg::Vec3d uv = osg::Y_AXIS * matrix; uv.normalize();
    osg::Vec3d eyeCenter = _worldCenter * view;
    osg::Vec3d eyeRefPole = computeViewPoint() * view;

    double angle = 0.0, lowerEdgeY = lowerEdge.y();
    double centerY = eyeCenter.y(), northY = eyeRefPole.y();
    if (centerY<lowerEdgeY * 0.1 && centerY>lowerEdgeY)
    {
        // The earth center is still above the lower edge of the screen
        if ((centerY + northY) > 0.0) angle = (0.0 - centerY) / (northY - centerY);
        else angle = 0.5;
    }
    else if (centerY < lowerEdgeY)
    {
        // The earth center sinks under the lower edge of the screen
        osg::Vec3d bestViewEdge;
        ok = calcIntersectPoint(0.0, -0.8, bestViewEdge, false);
        if (ok)
        {
            bestViewEdge = bestViewEdge * view;
            angle = (bestViewEdge.y() - centerY) / (northY - centerY);
        }
        else
        {
            if (northY < 0.0) northY = 0.0;
            angle = ((northY + lowerEdgeY) * 0.5 - centerY) / (northY - centerY);
        }
    }
    else return;

    if (angle > -1.0 && angle < 1.0) angle = asin(angle);
    else angle = osg::PI_2;
    if (angle > getTilt()) angle = getTilt();
    new_rotate = new_rotate * osg::Quat(angle, sv);
}

void EarthManipulator::insertControlPoint(float time, const osg::Vec3d& eye, float tilt, float doa)
{ _controlPoints.insert(new ControlPointByEye(time, eye, tilt, doa)); }

void EarthManipulator::insertControlPoint(float time, double latitude, double longitude, double height,
    float tilt, float doa)
{
    osg::Vec3d eye;
    _ellipsoid->convertLatLongHeightToXYZ(latitude, longitude, height,
                                          eye.x(), eye.y(), eye.z());
    eye += _worldCenter;
    insertControlPoint(time, eye, tilt, doa);
}

void EarthManipulator::insertControlPointFromCurrentView(float time)
{ _controlPoints.insert(new ControlPoint(time, _worldRotation, _distance, _tilt)); }

void EarthManipulator::eraseControlPoint(float time)
{
    ControlPointSet::iterator itr = std::find_if(_controlPoints.begin(), _controlPoints.end(),
        FindControlPointPtr(time));
    if (itr != _controlPoints.end()) _controlPoints.erase(itr);
}

void EarthManipulator::startAnimation(double time, bool userAnimation)
{
    // Go home, and clear internal point set for reversing animation
    _isUserAnimation = userAnimation;
    if (_isUserAnimation)
    {
        home(time);
        _internalControlPoints.clear();
    }

    // Reset all states
    _animationRunning = true;
    _isReversingInLoop = false;
    _animationTilt = _tilt;
    makeTiltRotation(_animationTiltRotation, _animationTilt, osg::X_AXIS);
    if (time != USE_REFERENCE_TIME)
        _animationStartTime = time;
    else if (_viewer && _viewer->getFrameStamp())
        _animationStartTime = (float)_viewer->getFrameStamp()->getFrameNumber();
}

void EarthManipulator::stopAnimation(bool mergeAnimationPath)
{
    if (!_animationRunning) return;
    _animationRunning = false;
    _isUserAnimation = true;
    _isReversingInLoop = false;
    if (mergeAnimationPath)
    {
        _worldRotation *= _animationRotation;
        _distance = _animationDistance;
        _tilt = _animationTilt;
        makeTiltRotation(_tiltRotation, _tilt, osg::X_AXIS);
    }

    _animationRotation.set(0.0, 0.0, 0.0, 1.0);
    _animationTiltRotation.set(0.0, 0.0, 0.0, 1.0);
    _animationDistance = 0.0;
    _animationTilt = 0.0f;
    _animationStartTime = 0.0f;
}

void EarthManipulator::advanceAnimation(const osg::FrameStamp* fs)
{
    if (!fs) return;
    float frameNumber = (float)fs->getFrameNumber();
    float currentTime = frameNumber - _animationStartTime;

    ControlPointSet* pts = NULL;
    if (_isUserAnimation)
    {
        currentTime *= _animationSpeed;
        if (_internalControlPoints.empty() && !_controlPoints.empty())
        {
            float maxTime = _controlPoints.rbegin()->get()->_time;
            for (ControlPointSet::iterator itr = _controlPoints.begin();
                 itr != _controlPoints.end(); ++itr)
            {
                ControlPoint* cp = (*itr)->clone();
                cp->_time = maxTime - cp->_time;
                _internalControlPoints.insert(cp);
            }
        }

        if (_playMode == REVERSE || _isReversingInLoop) pts = &_internalControlPoints;
        else pts = &_controlPoints;
    }
    else
        pts = &_internalControlPoints;

    if (!applyAnimation(*pts, currentTime))
    {
        if (_isUserAnimation && !pts->empty())
        {
            switch (_playMode)
            {
            case LOOP:
                _animationStartTime += pts->rbegin()->get()->_time; return;
            case SWING:
                _animationStartTime += pts->rbegin()->get()->_time;
                _isReversingInLoop = !_isReversingInLoop; return;
            default:
                break;
            }
        }
        stopAnimation();
    }
}

bool EarthManipulator::applyAnimation(ControlPointSet& pts, float time)
{
    ControlPointSet::iterator itr = std::find_if(pts.begin(), pts.end(), FindControlPointPtr(time));
    if (pts.size() < 1) return false;

    if (itr == pts.begin())
    {
        (*itr)->convert(this);
        return false;
    }
    else if (itr != pts.end())
    {
        ControlPoint* value1 = (*itr)->convert(this);
        ControlPoint* value0 = (*(--itr))->convert(this);

        float blend = (time - value0->_time) / (value1->_time - value0->_time);
        _animationRotation.slerp(blend, value0->_rotation, value1->_rotation);
        _animationDistance = value0->_distance * (1.0f - blend) + value1->_distance * blend;
        _animationTilt = value0->_tilt * (1.0f - blend) + value1->_tilt * blend + _tilt;
        calcTiltCenter(false);
        makeTiltRotation(_animationTiltRotation, _animationTilt, osg::X_AXIS);
        return true;
    }
    else
    {
        ControlPoint* value = (*(pts.rbegin()))->convert(this);
        _animationRotation = value->_rotation;
        _animationDistance = value->_distance;
        _animationTilt = value->_tilt + _tilt;
        calcTiltCenter(false);
        makeTiltRotation(_animationTiltRotation, _animationTilt, osg::X_AXIS);
        return false;
    }
}

EarthProjectionMatrixCallback::EarthProjectionMatrixCallback(osg::Camera* camera, const osg::Vec3d& center, double radius)
:   _camera(camera), _earthCenter(center), _earthRadius(radius), _nearFirstModeThreshold(1000.0),
    _nfrAtRadius(0.00001), _nfrAtDoubleRadius(0.0049), _useNearFarRatio(false)
{}
