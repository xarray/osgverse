#ifndef MANA_PP_RENDER_CALLBACK_XR_HPP
#define MANA_PP_RENDER_CALLBACK_XR_HPP

#include <osg/Camera>
#include <osgGA/EventQueue>
#include "Utilities.h"

namespace osgVerse
{
    /** OpenXR loader and presenter
        To test with Monado (https://monado.freedesktop.org/developing-with-monado.html)
        - Build monado and install it, ensure 'openxr_loader' in system path
        - export XR_RUNTIME_JSON=<install_path>/openxr_monado.json
        - export QWERTY_ENABLE=1; export XRT_DEBUG_GUI=1
        - cd <install_path>/bin & ./monado-service.exe
    */
    class RenderCallbackXR : public CameraDrawCallback
    {
    public:
        RenderCallbackXR();
        virtual bool begin(osg::Matrix& viewL, osg::Matrix& viewR,
                           osg::Matrix& projL, osg::Matrix& projR, double znear, double zfar);
        virtual bool handleEvents(osgGA::EventQueue* ev);

        virtual void operator()(osg::RenderInfo& renderInfo) const;
        virtual void releaseGLObjects(osg::State* state = 0) const;

        enum ReferenceSpaceType
        {
            VIEW = 1,    // origin at HMD and follow HMD, X-right, y-up, z-back
            LOCAL = 2,   // origin at HMD, y-up, no tilt
            STAGE = 3    // origin at room floor center, y-up, has bounds
        };
        void setReferenceSpaceType(ReferenceSpaceType t) { _spaceType = t; }
        ReferenceSpaceType getReferenceSpaceType() const { return _spaceType; }

        void setOrientation(const osg::Quat& q) { _spaceOrientation = q; }
        const osg::Quat& getOrientation() const { return _spaceOrientation; }

        void setPosition(const osg::Vec3& p) { _spacePosition = p; }
        const osg::Vec3& getPosition() const { return _spacePosition; }

    protected:
        virtual ~RenderCallbackXR();

        osg::ref_ptr<osg::Referenced> _xrLoader;
        osg::ref_ptr<osg::Referenced> _xrSession;
        osg::Quat _spaceOrientation;
        osg::Vec3 _spacePosition;
        osg::Timer_t _beginFrameTick;
        ReferenceSpaceType _spaceType;
        bool _sessionReady, _shouldRender;
        mutable bool _beganFrame;
    };
}

#endif
