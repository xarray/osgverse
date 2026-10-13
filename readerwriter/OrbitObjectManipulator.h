#ifndef MANA_READERWRITER_ORBITOBJECTMANIPULATOR_HPP
#define MANA_READERWRITER_ORBITOBJECTMANIPULATOR_HPP

#include <osg/Node>
#include <osg/Camera>
#include <osg/BoundingBox>
#include <osgGA/TrackballManipulator>
#include "Export.h"

namespace osgVerse
{
    /** A trackball manipulator that can orbit around a chosen scene object instead of always
        revolving around the scene center. Focusing keeps the view direction and the up vector of
        the current view: only the orbit center and the camera distance change, and both the eye
        and the center move together so that the object's center becomes the new orbit center. */
    class OSGVERSE_RW_EXPORT OrbitObjectManipulator : public osgGA::TrackballManipulator
    {
    public:
        OrbitObjectManipulator(int flags = osgGA::StandardManipulator::DEFAULT_SETTINGS)
            : osgGA::TrackballManipulator(flags) {}
        virtual const char* className() const { return "OrbitObjectManipulator"; }

        /** Make 'node' the new orbit center. The view direction and up vector are preserved and the
            eye is moved so that the node's world bounding box is centered and occupies 'ratio' of
            the view (default 2/3). 'camera' is used to read the field of view. */
        bool focusOnObject(osg::Node* node, const osg::Camera* camera, double ratio = 2.0 / 3.0);

        /** Same as focusOnObject() but with an explicit world-space bounding box */
        bool focusOnBox(const osg::BoundingBox& bb, const osg::Camera* camera, double ratio = 2.0 / 3.0);

        /** Remember the object the camera orbits around. This does not move the camera by itself:
            call refreshOrbitCenter() to re-center after the object has been transformed. Pass NULL
            (or call focusOnObject() with another node) to change the orbited object. */
        void setOrbitObject(osg::Node* node) { _orbitObject = node; }
        osg::Node* getOrbitObject() { return _orbitObject.get(); }
        const osg::Node* getOrbitObject() const { return _orbitObject.get(); }

        /** Re-apply the stored orbit object's current world center as the orbit center */
        bool refreshOrbitCenter();

    protected:
        osg::BoundingBox computeOrbitBox(osg::Node* node) const;
        double computeFitDistance(const osg::BoundingBox& bb, const osg::Camera* camera,
                                  double ratio) const;

        osg::observer_ptr<osg::Node> _orbitObject;
    };
}

#endif
