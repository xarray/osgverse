#ifndef MANA_READERWRITER_EARTHMANIPULATOR_HPP
#define MANA_READERWRITER_EARTHMANIPULATOR_HPP

#include <osg/Version>
#include <osg/MatrixTransform>
#include <osgGA/CameraManipulator>
#include <osgViewer/View>
#include "Export.h"

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT EarthManipulator : public osgGA::CameraManipulator
    {
    public:
        typedef std::function<void (float, float)> AnimationCompletedFunc;

        EarthManipulator();
        virtual const char* className() const { return "EarthManipulator"; }
        virtual void getUsage(osg::ApplicationUsage& usage) const;

        virtual void setByMatrix(const osg::Matrixd&);
        virtual void setByInverseMatrix(const osg::Matrixd&);
        virtual osg::Matrixd getMatrix() const { return getManipulatorMatrix(); }
        virtual osg::Matrixd getInverseMatrix() const { return getViewMatrix(); }

        virtual float getFusionDistanceValue() const { return _distance; }
        virtual osgUtil::SceneView::FusionDistanceMode getFusionDistanceMode() const
        { return osgUtil::SceneView::USE_FUSION_DISTANCE_VALUE; }

        virtual void setNode(osg::Node*);
        virtual osg::Node* getNode() { return _node.get(); }
        virtual const osg::Node* getNode() const { return _node.get(); }

        /** Set the world node to be intersected */
        virtual void setWorldNode(osg::Node* w) { _world = w; }
        virtual osg::Node* getWorldNode() { return _world.get(); }
        virtual const osg::Node* getWorldNode() const { return _world.get(); }

        /** Set the working viewer, internal called by init() if not set */
        void setViewer(osgViewer::View* viewer) { _viewer = viewer; }
        osgViewer::View* getViewer() { return _viewer; }
        const osgViewer::View* getViewer() const { return _viewer; }

        /** Set the ellipsoid model */
        void setEllipsoid(osg::EllipsoidModel* ellipsoid);
        osg::EllipsoidModel* getEllipsoid() { return _ellipsoid.get(); }
        const osg::EllipsoidModel* getEllipsoid() const { return _ellipsoid.get(); }

        /** Set the node to be shown when mouse clicked */
        void setClickedPointNode(osg::MatrixTransform* mt) { _clickedPointNode = mt; };
        osg::MatrixTransform* getClickedPointNode() { return _clickedPointNode.get(); }
        const osg::MatrixTransform* getClickedPointNode() const { return _clickedPointNode.get(); }

        /** Set the view point center before world rotating */
        void setCenter(const osg::Vec3d& center) { _center = center; }
        const osg::Vec3d& getCenter() const { return _center; }

        /** Set a user-customized initial rotation value */
        void setInitialRotation(const osg::Quat& rotation) { _initRotation = rotation; }
        const osg::Quat& getInitialRotation() const { return _initRotation; }

        /** Set the viewing distance */
        void setDistance(double distance) { _distance = distance; }
        double getDistance() const { return (_animationRunning ? _animationDistance : _distance); }

        /** Set the intersection mask. Default is 0xffffffff.
            Objects outside the earth must not be intersected; otherwise incorrect results may occur sometime */
        void setIntersectionMask(unsigned int mask) { _intersectionMask = mask; }
        unsigned int getIntersectionMask() const { return _intersectionMask; }

        /** Set zoom-in/out factor (default = 1, 1) */
        void setZoomFactor(const osg::Vec2& zoom) { _zoomFactor.set(zoom); }
        const osg::Vec2& getZoomFactor() const { return _zoomFactor; }

        /** Set if all user operations are locked */
        void setLocked(bool locked) { _locked = locked; }
        bool getLocked() const { return _locked; }

        /** Set if throwing is allowed */
        void setThrowAllowed(bool allowed) { _throwAllowed = allowed; }
        bool getThrowAllowed() const { return _throwAllowed; }

        /** Set the manipulator matrix by eye point */
        void setByEye(const osg::Vec3d& eye, float doa = 0.0f);
        void setByEye(double latitude, double longitude, double height, float doa = 0.0f);

        /** Start an animation from current position to the given one */
        void moveTo(osg::Vec3d vector, double deltaDistance, double frames = 60.0,
                    AnimationCompletedFunc func = NULL);

        /** Get the manipulator matrix */
        osg::Matrixd getManipulatorMatrix(bool withTilt = true) const;
        osg::Matrixd getViewMatrix(bool withTilt = true) const;

        /** Compute the eye position */
        osg::Vec3d computeEye() const
        { return osg::Vec3d(0.0, 0.0, 0.0) * getMatrix(); }

        /** Compute the eye latitude, longitude and height */
        osg::Vec3d computeEyeLatLonHeight() const
        {
            osg::Vec3d e = computeEye() - _worldCenter; double lat, lon, height;
            _ellipsoid->convertXYZToLatLongHeight(e[0], e[1], e[2], lat, lon, height);
            return osg::Vec3d(lat, lon, height);
        }

        /** Compute the view point */
        osg::Vec3d computeViewPoint() const
        {
            osg::Matrixd matrix(_worldRotation);
            if (_animationRunning) matrix.postMultRotate(_animationRotation);
            return _center * matrix + _worldCenter;
        }

        /** Compute the direction from eye to geocenter */
        osg::Vec3d computeEyeDir() const
        {
            //osg::Vec3d lv = -osg::Z_AXIS * makeRotationMatrix();
            //lv.normalize(); return lv;
            osg::Vec3d dir = _worldCenter - computeEye();
            dir.normalize(); return dir;
        }

        /** Compute the looking vector from eye to view point */
        osg::Vec3d computeLookVector() const
        {
            osg::Vec3d dir = computeViewPoint() - computeEye();
            dir.normalize(); return dir;
        }

        /** Compute degree-of-angle between the north pole projection and the up vector */
        double computeDOA() const
        {
            osg::Matrixd matrix = makeRotationMatrix();
            osg::Vec3d lv = -osg::Z_AXIS * matrix; lv.normalize();
            osg::Vec3d uv = osg::Y_AXIS * matrix; uv.normalize();
            osg::Vec3d np = osg::Z_AXIS - lv * (osg::Z_AXIS * lv);
            if (np.length2() > 0.0)
            {
                np.normalize();
                return atan2((np ^ uv).length(), uv * np);
            }
            return 0.0;
        }

        /** Used by animation controllor to compute the eye changing */
        void computeEyeChanging(osg::Quat& delta_rotate, double& delta_distance,
                                const osg::Vec3d& eye, float absTilt, float absDOA)
        {
            makePositionFromEye(delta_rotate, delta_distance, eye, absTilt - getTilt(), absDOA);
            delta_distance = delta_distance - getDistance();
        }

        /** Get the world rotation value */
        const osg::Quat& getWorldRotation() const { return _worldRotation; }
        const osg::Quat& getAnimationRotation() const { return _animationRotation; }

        /** Get latest clicked position on earth */
        osg::Vec3d getLatestPosition() const
        { return osg::Vec3d(_latestLatitude, _latestLongitude, _latestAltitude); }

        /** Get the tilt value */
        float getTilt() const
        { return (_animationRunning ? _animationTilt : _tilt); }

        /** Make the earth tilt a delta value */
        void makeDeltaTilt(float dy)
        { _tilt -= dy; makeTiltRotation(_tiltRotation, _tilt, osg::X_AXIS); }

        virtual void computeHomePosition();
        virtual void home(double);
        virtual void home(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&);

        virtual void init(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&);
        virtual bool handle(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&);

        void performPan(double x0, double y0, double dx, double dy);
        void performHRotate(double value);
        void performHRotate(double x0, double y0, double dx, double dy);
        void performVRotate(double x0, double y0, double dx, double dy);
        void performRotateAxis(double x0, double y0);
        void performScale(osgGA::GUIEventAdapter::ScrollingMotion scrollMotion) { calcScrollingMotion(scrollMotion); }
        void performScale(double x0, double y0, double dx, double dy)
        { if (dy < 0.0) _distance *= (1.0 + dy * _zoomFactor[0]); else _distance *= (1.0 + dy * _zoomFactor[1]); }

    protected:
        virtual ~EarthManipulator();

        void flushMouseEventStack()
        { _ga_t1 = NULL; _ga_t0 = NULL; }

        void addMouseEventAndUpdate(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
        {
            _ga_t1 = _ga_t0; _ga_t0 = &ea;
            if (calcMovement()) us.requestRedraw();
            us.requestContinuousUpdate(false);
            _thrown = false;
        }

        void setHomePosition(bool autoOrientation, const osg::BoundingSphere& bs);
        void computePosition(const osg::Vec3& eye, const osg::Vec3& center, const osg::Vec3& up);

        bool isMouseMoving();
        bool calcMovement(bool throwing = false);
        bool calcDoubleClickMotion(const osgGA::GUIEventAdapter& ea);
        bool calcScrollingMotion(osgGA::GUIEventAdapter::ScrollingMotion scrollMotion);
        bool calcIntersectPoint(float x, float y, osg::Vec3d& point, bool showPoint = true);
        bool calcTiltCenter(bool useCameraMatrix = true);

        void makePositionFromEye(osg::Quat& new_rotate, double& new_distance,
                                 const osg::Vec3d& eye, float tilt, float doa);
        void makeBestLookingRotation(osg::Quat& new_rotate,
                                     const osg::Matrixd& matrix, const osg::Matrixd& view);

        osg::Matrixd makeRotationMatrix() const
        {
            if (_animationRunning)
            {
                return osg::Matrixd(_animationTiltRotation * _initRotation *
                                    _worldRotation * _animationRotation);
            }
            else
                return osg::Matrixd(_tiltRotation * _initRotation * _worldRotation);
        }

        void makeTiltRotation(osg::Quat& rot, float& angle, const osg::Vec3& axis) const
        {
            static const float c_maxTiltAngle = osg::PI_2 - 0.15f;
            if (angle < 0.0f) angle = 0.0f;
            else if (angle > c_maxTiltAngle) angle = c_maxTiltAngle;
            rot.makeRotate(angle, axis);
        }

        osg::ref_ptr<osg::Node> _node;
        osg::ref_ptr<osg::Node> _world;
        osg::ref_ptr<const osgGA::GUIEventAdapter> _ga_t0;  // Current event
        osg::ref_ptr<const osgGA::GUIEventAdapter> _ga_t1;  // Previous event

        osgViewer::View* _viewer;  // The main viewer
        osg::ref_ptr<osg::MatrixTransform> _clickedPointNode;  // Clicked point geometry

        osg::ref_ptr<osg::EllipsoidModel> _ellipsoid;  // Ellipsoid of the planet
        osg::Vec3d _worldCenter;  // Center of the targeted world model
        osg::Vec3d _tiltCenter;  // The initial center of tilting
        osg::Vec3d _rotateAxis;  // Rotating axis while tilting

        osg::Vec3d _center;  // Viewing offset
        osg::Vec3d _initEyeDir;  // Eye direction within the initial rotation
        osg::Quat _initRotation;
        osg::Quat _worldRotation;
        osg::Quat _tiltRotation;

        osg::Vec2 _zoomFactor;  // Zoom-in/out factor to control scaling speed
        double _latestLatitude, _latestLongitude, _latestAltitude;
        double _distance;  // Distance between eye and view point
        float _tilt;  // Vertical angle to the horizon

        unsigned int _intersectionMask;  // Mask for intersection with the earth
        bool _throwAllowed, _thrown, _locked;

    public:
        struct ControlPoint : public osg::Referenced
        {
            ControlPoint() : _distance(0.0), _tilt(0.0f), _time(0.0f) {}
            ControlPoint(float time, const osg::Quat& rotation, double distance, float tilt)
                : _rotation(rotation), _distance(distance), _tilt(tilt), _time(time) {}
            ControlPoint(const ControlPoint& copy)
                : _rotation(copy._rotation), _distance(copy._distance),
                  _tilt(copy._tilt), _time(copy._time) {}

            virtual ControlPoint* clone() { return new ControlPoint(_time, _rotation, _distance, _tilt); }
            virtual ControlPoint* convert(EarthManipulator*) { return this; }

            osg::Quat _rotation;
            double _distance;
            float _tilt, _time;
        };

        struct ControlPointByEye : public ControlPoint
        {
            ControlPointByEye()
                : ControlPoint(), _absTilt(0.0f), _absDOA(0.0f), _converted(false) {}

            ControlPointByEye(float time, const osg::Vec3d& eye, float absTilt, float absDOA)
                : ControlPoint(), _eye(eye), _absTilt(absTilt), _absDOA(absDOA), _converted(false)
            {
                _time = time;
                if (_absTilt < 0.0f) _absTilt = 0.0f;
                else if (_absTilt > osg::PI_2) _absTilt = osg::PI_2;
            }

            ControlPointByEye(const ControlPointByEye& copy)
                : ControlPoint(copy), _eye(copy._eye), _absTilt(copy._absTilt), _absDOA(copy._absDOA),
                  _converted(copy._converted) {}

            virtual ControlPoint* clone()
            { return new ControlPointByEye(_time, _eye, _absTilt, _absDOA); }

            virtual ControlPoint* convert(EarthManipulator* em)
            {
                if (!_converted)
                {
                    em->computeEyeChanging(_rotation, _distance, _eye, _absTilt, _absDOA);
                    _tilt = _absTilt - em->getTilt();
                    _converted = true;
                }
                return this;
            }

            osg::Vec3d _eye;
            float _absTilt;
            float _absDOA;
            bool _converted;
        };

        struct FindControlPointPtr
        {
            FindControlPointPtr(float t) : _time(t) {}
            bool operator()(const osg::ref_ptr<ControlPoint>& cp) { return _time < cp->_time; }
            float _time;
        };

        struct CompareControlPoints
        {
            bool operator()(const osg::ref_ptr<ControlPoint>& lhs,
                            const osg::ref_ptr<ControlPoint>& rhs) const
            { return lhs->_time < rhs->_time; }
        };

        typedef std::set<osg::ref_ptr<ControlPoint>, CompareControlPoints> ControlPointSet;
        void setControlPoints(const ControlPointSet& pts) { _controlPoints = pts; }
        ControlPointSet& getControlPoints() { return _controlPoints; }
        const ControlPointSet& getControlPoints() const { return _controlPoints; }

        /** Convenient functions for inserting animation keyframes */
        void insertControlPoint(float time, const osg::Vec3d& eye, float tilt = 0.0f, float doa = 0.0f);
        void insertControlPoint(float time, double latitude, double longitude, double height,
                                float tilt = 0.0f, float doa = 0.0f);
        void insertControlPointFromCurrentView(float time);

        /** Operations on user animation control points */
        void insertControlPoint(ControlPoint* cp) { _controlPoints.insert(cp); }
        void eraseControlPoint(float time);
        void clearControlPoints() { _controlPoints.clear(); }

        /** Set the playing mode */
        enum PlayMode { ONCE = 0, REVERSE, LOOP, SWING };
        void setPlayMode(PlayMode mode) { _playMode = mode; }
        PlayMode getPlayMode() const { return _playMode; }

        /** Set the animation speed. Default is 1.0f */
        void setAnimationSpeed(float speed) { _animationSpeed = speed; }
        float getAnimationSpeed() const { return _animationSpeed; }

        /** Check if animation is running */
        bool isAnimationRunning() const { return _animationRunning; }

        /** Start or stop the animation */
        void startAnimation(double time = USE_REFERENCE_TIME, bool userAnimation = true);
        void stopAnimation(bool mergeAnimationPath = true);

    protected:
        void advanceAnimation(const osg::FrameStamp* fs);
        bool applyAnimation(ControlPointSet& pts, float time);

        ControlPointSet _controlPoints;  // Control points for user-defined animation
        ControlPointSet _internalControlPoints;  // Control points for internal animations

        AnimationCompletedFunc _onAnimationCompleted;

        PlayMode _playMode;
        float _animationSpeed;
        float _animationStartTime;

        bool _animationRunning;
        bool _isUserAnimation;
        bool _isReversingInLoop;

        osg::Quat _animationRotation;
        osg::Quat _animationTiltRotation;
        double _animationDistance;
        float _animationTilt;
    };

    class OSGVERSE_RW_EXPORT EarthProjectionMatrixCallback : public osg::CullSettings::ClampProjectionMatrixCallback
    {
    public:
        EarthProjectionMatrixCallback(osg::Camera* camera = 0, const osg::Vec3d& center = osg::Vec3d(),
                                      double radius = osg::WGS_84_RADIUS_POLAR);

        void setMainCamera(osg::Camera* camera) { _camera = camera; }
        osg::Camera* getMainCamera() { return _camera.get(); }
        const osg::Camera* getMainCamera() const { return _camera.get(); }

        void setEarthCenter(const osg::Vec3d& center) { _earthCenter = center; }
        const osg::Vec3d& getEarthCenter() const { return _earthCenter; }

        void setEarthRadius(double radius) { _earthRadius = radius; }
        double getEarthRadius() const { return _earthRadius; }

        void setNearFirstModeThreshold(double threshold) { _nearFirstModeThreshold = threshold; }
        double getNearFirstModeThreshold() const { return _nearFirstModeThreshold; }

        void setRatioFactor(double r, double dr) { _nfrAtRadius = r; _nfrAtDoubleRadius = dr; }
        void getRatioFactor(double& r, double& dr) const { r = _nfrAtRadius; dr = _nfrAtDoubleRadius; }

        void setUseNearFarRatio(bool b) { _useNearFarRatio = b; }
        bool getUseNearFarRatio() const { return _useNearFarRatio; }

        virtual bool clampProjectionMatrixImplementation(osg::Matrixf& projection, double& znear, double& zfar) const
        { return clampImplementation_Earth(projection, znear, zfar, _camera->getNearFarRatio()); }

        virtual bool clampProjectionMatrixImplementation(osg::Matrixd& projection, double& znear, double& zfar) const
        { return clampImplementation_Earth(projection, znear, zfar, _camera->getNearFarRatio()); }

    protected:
        template<class matrix_type, class value_type>
        bool clampImplementation_Earth(matrix_type& projection, double& znear,
                                       double& zfar, value_type near_far_ratio) const
        {
            const osg::Camera* camera = getMainCamera();
            if (!camera) return false;

            double epsilon = 1e-6;
            if (zfar < znear - epsilon) return false;

            if (zfar < znear + epsilon)
            {
                double average = (znear + zfar) * 0.5;
                znear = average - epsilon;
                zfar = average + epsilon;
            }

            double rp = getEarthRadius();
            double nfrAtRadius, nfrAtDoubleRadius;
            getRatioFactor(nfrAtRadius, nfrAtDoubleRadius);

            osg::Vec3d eye, center, up;
            camera->getViewMatrixAsLookAt(eye, center, up, rp);

            double distance = (eye - getEarthCenter()).length();
            if (distance > rp)
            {
                double nfr = getUseNearFarRatio() ?
                    near_far_ratio : nfrAtRadius + nfrAtDoubleRadius * ((distance - rp) / distance);
                double temp_znear = znear, temp_zfar = zfar;
                zfar = osg::minimum(sqrt(distance * distance - rp * rp), temp_zfar);
                znear = osg::clampAbove(zfar * nfr, 1.0);
                if (distance - rp < _nearFirstModeThreshold)
                {
                    // Switch to near plane first mode
                    znear = osg::minimum(1.0, znear);
                    zfar = osg::minimum(osg::clampBelow(znear / nfr, 20000.0), temp_zfar);
                }

                // assign the clamped values back to the computed values
                value_type trans_near_plane = (-znear * projection(2, 2) + projection(3, 2)) /
                                              (-znear * projection(2, 3) + projection(3, 3));
                value_type trans_far_plane = (-zfar * projection(2, 2) + projection(3, 2)) /
                                             (-zfar * projection(2, 3) + projection(3, 3));
                value_type ratio = fabs(2.0 / (trans_near_plane - trans_far_plane));
                value_type center = -(trans_near_plane + trans_far_plane) / 2.0;
                projection.postMult(osg::Matrix(1.0f, 0.0f, 0.0f, 0.0f,
                                                0.0f, 1.0f, 0.0f, 0.0f,
                                                0.0f, 0.0f, ratio, 0.0f,
                                                0.0f, 0.0f, center * ratio, 1.0f));
                return true;
            }
            return false;
        }

        osg::observer_ptr<osg::Camera> _camera;
        osg::Vec3d _earthCenter;
        double _nearFirstModeThreshold;
        double _earthRadius;
        double _nfrAtRadius;
        double _nfrAtDoubleRadius;
        bool _useNearFarRatio;
    };
}

#endif
