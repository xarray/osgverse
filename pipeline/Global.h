#ifndef MANA_PP_GLOBAL_HPP
#define MANA_PP_GLOBAL_HPP

#include <algorithm>
#include <osg/Polytope>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/Camera>

#define INITIAL_DRAW 0
#define PRE_DRAW 1
#define POST_DRAW 2
#define FINAL_DRAW 3

#ifdef INSTALL_PATH_PREFIX
    #define BASE_DIR INSTALL_PATH_PREFIX
#else
    #define BASE_DIR ".."
#endif
#define SHADER_DIR BASE_DIR "/shaders/"

namespace osgVerse
{
    /** Global-defined vertex attribute names, for full-featured pipeline use */
    static std::string attributeNames[] =
    {
        /*0*/"osg_Vertex", /*1*/"osg_Weights", /*2*/"osg_Normal", /*3*/"osg_Color",
        /*4*/"osg_SecondaryColor", /*5*/"osg_FogCoord", /*6*/"osg_Tangent", /*7*/"osg_Binormal",
        /*8*/"osg_TexCoord0", /*9*/"osg_TexCoord1", /*10*/"osg_TexCoord2", /*11*/"osg_TexCoord3",
        /*12*/"osg_TexCoord4", /*13*/"osg_TexCoord5", /*14*/"osg_TexCoord6", /*15*/"osg_TexCoord7"
    };

    /** Global-defined texture-map uniform names, for full-featured pipeline use */
    static std::string uniformNames[] =
    {
        /*0*/"DiffuseMap", /*1*/"NormalMap", /*2*/"SpecularMap", /*3*/"ShininessMap",
        /*4*/"AmbientMap", /*5*/"EmissiveMap", /*6*/"ReflectionMap", /*7*/"CustomMap"
    };

    class ComponentCallback;
    struct Component : public osg::Object
    {
        Component() : _parent(NULL), _executionOrder(0), _active(true) {}
        Component(const Component& c, const osg::CopyOp& op)
        : osg::Object(c, op), _parent(NULL), _executionOrder(0), _active(true) {}
        META_Object(osgVerse, Component);

        virtual void run(osg::Object* object, osg::Object* data) {}
        ComponentCallback* _parent; int _executionOrder; bool _active;
    };

    /** Node/drawable callback for compatiblity */
    class ComponentCallback : public osg::Callback
    {
    public:
        virtual bool run(osg::Object* object, osg::Object* data)
        {
            size_t index = 0;
            for (; index < _components.size(); ++index)
            {
                Component* c = _components[index].get();
                if (c->_active && c->_executionOrder <= 0) c->run(object, data);
                else if (c->_executionOrder > 0) break;
            }
            
            bool ok = traverse(object, data);
            for (; index < _components.size(); ++index)
            {
                Component* c = _components[index].get();
                if (c->_active) c->run(object, data);
            }
            return ok;
        }

        void resortComponents()
        {
            std::sort(_components.begin(), _components.end(),
                      [&](const osg::ref_ptr<Component>& a, const osg::ref_ptr<Component>& b)
                      { return a->_executionOrder < b->_executionOrder; });
        }

        void addComponent(Component* c) { c->_parent = this; _components.push_back(c); }
        void clear() { for (auto c : _components) c->_parent = NULL; _components.clear(); }
        unsigned int getNumComponents() const { return _components.size(); }

        Component* getComponent(int i) { return _components[i].get(); }
        const Component* getComponent(int i) const { return _components[i].get(); }

    protected:
        std::vector<osg::ref_ptr<Component>> _components;
    };

    /** Camera draw callback for compatiblity */
    class CameraDrawCallback : public osg::Camera::DrawCallback
    {
    public:
        CameraDrawCallback() {}
        CameraDrawCallback(const CameraDrawCallback& org, const osg::CopyOp& copyop)
            : osg::Camera::DrawCallback(org, copyop), _subCallback(org._subCallback) {}
        META_Object(osgVerse, CameraDrawCallback);

        void setup(osg::Camera* cam, int lv)
        {
            osg::Camera::DrawCallback* cb = NULL;
            switch (lv)
            {
            case 0:
                cb = cam->getInitialDrawCallback();
                if (!cb) cam->setInitialDrawCallback(this); break;
            case 1:
                cb = cam->getPreDrawCallback();
                if (!cb) cam->setPreDrawCallback(this); break;
            case 2:
                cb = cam->getPostDrawCallback();
                if (!cb) cam->setPostDrawCallback(this); break;
            default:
                cb = cam->getFinalDrawCallback();
                if (!cb) cam->setFinalDrawCallback(this); break;
            }

            CameraDrawCallback* dcb = static_cast<CameraDrawCallback*>(cb);
            if (dcb && dcb != this) dcb->setSubCallback(this);
        }

        void setSubCallback(CameraDrawCallback* cb) { _subCallback = cb; }
        CameraDrawCallback* getSubCallback() { return _subCallback.get(); }
        const CameraDrawCallback* getSubCallback() const { return _subCallback.get(); }

        inline void addSubCallback(CameraDrawCallback* nc)
        {
            if (nc)
            {
                if (!_subCallback) _subCallback = nc; 
                else _subCallback->addSubCallback(nc);
            }
        }

        inline void removeSubCallback(CameraDrawCallback* nc)
        {
            if (!nc) return;
            if (_subCallback == nc)
            {
                osg::ref_ptr<CameraDrawCallback> new_cb = _subCallback->getSubCallback();
                _subCallback->setSubCallback(NULL); _subCallback = new_cb;
            }
            else if (_subCallback.valid())
                _subCallback->removeSubCallback(nc);
        }

        virtual void operator()(const osg::Camera& /*camera*/) const {}
        virtual void operator()(osg::RenderInfo& renderInfo) const
        {
            if (renderInfo.getCurrentCamera())
            {
                operator()(*(renderInfo.getCurrentCamera()));
                if (_subCallback.valid()) _subCallback.get()->run(renderInfo);
            }
            else
                OSG_WARN << "Error: Camera::DrawCallback called without valid camera." << std::endl;
        }

    protected:
        virtual ~CameraDrawCallback() {}
        osg::ref_ptr<CameraDrawCallback> _subCallback;
    };

    /** Suggest run this function once to initialize some plugins & environments */
    extern void globalInitialize(int argc, char** argv);
}

#endif
