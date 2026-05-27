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
        typedef std::vector<const char*> ExtensionList;
        RenderCallbackXR(const ExtensionList& ext = ExtensionList());
        virtual bool begin(osg::Matrixf& viewL, osg::Matrixf& viewR,
                           osg::Matrixf& projL, osg::Matrixf& projR, double znear, double zfar);
        virtual bool handleEvents(osgGA::EventQueue* ev);

        // https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#semantic-paths-interaction-profiles
        struct InputActionDefinition
        {
            enum Type { BOOLEAN = 1, FLOAT = 2, VECTOR2F = 3, POSE = 4 } type;
            const std::string name, description, pathLeft, pathRight;

            InputActionDefinition(Type t, const std::string& n, const std::string& desc,
                                  const std::string& pL, const std::string& pR)
            : type(t), name(n), description(desc), pathLeft(pL), pathRight(pR) {}
        };
        void addInputActionDefinition(const InputActionDefinition& d) { _inputActions.push_back(d); }
        void addSuggestedInteractionProfile(const std::string& d) { _suggestedProfiles.push_back(d); }
        void clearInputActionDefinitions() { _inputActions.clear(); }
        void clearSuggestedInteractionProfiles() { _suggestedProfiles.clear(); }
        const std::vector<InputActionDefinition>& getInputActionDefinitions()const { return _inputActions; }
        const std::vector<std::string>& getSuggestedInteractionProfiles() const { return _suggestedProfiles; }

        struct HandInputState
        {
            std::map<std::string, osg::Vec2> sticks;
            std::map<std::string, float> sliders;
            std::map<std::string, bool> buttons;
            osg::Matrix aimPose, gripPose;
            bool aimTracked, aimActive, gripTracked, gripActive;

            HandInputState()
            : aimTracked(false), aimActive(false), gripTracked(false), gripActive(false) {}
        };
        virtual bool handleInputs(HandInputState& left, HandInputState& right);

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

        int getSwapchainImageWidth() const { return _swapchainWidth; }
        int getSwapchainImageHeight() const { return _swapchainHeight; }
        bool isSessionReady() const { return _sessionReady; }  // check this after handleEvents()
        bool isRenderable() const { return _shouldRender; }    // check this after begin()

    protected:
        virtual ~RenderCallbackXR();

        std::vector<InputActionDefinition> _inputActions;
        std::vector<std::string> _suggestedProfiles;
        osg::ref_ptr<osg::Referenced> _xrLoader;
        osg::ref_ptr<osg::Referenced> _xrSession;
        osg::Quat _spaceOrientation;
        osg::Vec3 _spacePosition;
        osg::Timer_t _beginFrameTick;
        ReferenceSpaceType _spaceType;
        int _swapchainWidth, _swapchainHeight;
        bool _sessionReady, _shouldRender;
        mutable bool _beganFrame;
    };
}

#endif
