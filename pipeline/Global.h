#ifndef MANA_PP_GLOBAL_HPP
#define MANA_PP_GLOBAL_HPP

#include <algorithm>
#include <osg/Version>
#include <osg/Polytope>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/Camera>

#define INITIAL_DRAW 0
#define PRE_DRAW 1
#define POST_DRAW 2
#define FINAL_DRAW 3

#if defined(VERSE_MSVC)
#   if defined(INSTALL_PATH_PREFIX)
#       define BASE_DIR INSTALL_PATH_PREFIX
#   else
#       define BASE_DIR ".."
#   endif
#elif defined(VERSE_WASM) || defined(VERSE_ANDROID) || defined(VERSE_IOS)
#   define BASE_DIR "assets"
#else
#   define BASE_DIR ".."
#endif
#define SHADER_DIR BASE_DIR "/shaders/"
#define SKYBOX_DIR BASE_DIR "/skyboxes/"
#define MISC_DIR BASE_DIR "/misc/"

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

        virtual void run(osg::Object* object, osg::Referenced* nv) = 0;
        ComponentCallback* _parent; int _executionOrder; bool _active;
    };

    /** Node/drawable callback for compatiblity */
#if OSG_VERSION_GREATER_THAN(3, 2, 1)
    class ComponentCallback : public osg::Callback
#else
    class ComponentCallback
#endif
    {
    public:
#if OSG_VERSION_GREATER_THAN(3, 2, 1)
        virtual bool run(osg::Object* object, osg::Object* data)
        {
            size_t index = 0;
            runNestedComponents(object, data, index, true);

            bool ok = traverse(object, data);
            runNestedComponents(object, data, index, false);
            return ok;
        }
#endif

        void resortComponents()
        {
            std::sort(_components.begin(), _components.end(),
                      [&](const osg::ref_ptr<Component>& a, const osg::ref_ptr<Component>& b)
                      { return a->_executionOrder < b->_executionOrder; });
        }

        void addComponent(Component* c) { c->_parent = this; _components.push_back(c); _dirty = true; }
        void clear() { for (auto c : _components) c->_parent = NULL; _components.clear(); }
        unsigned int getNumComponents() const { return _components.size(); }

        Component* getComponent(int i) { return _components[i].get(); }
        const Component* getComponent(int i) const { return _components[i].get(); }

    protected:
        void runNestedComponents(osg::Object* object, osg::Referenced* data,
                                 size_t& index, bool beforeTraverse)
        {
            if (beforeTraverse)
            {
                if (_dirty) { resortComponents(); _dirty = false; }
                for (; index < _components.size(); ++index)
                {
                    Component* c = _components[index].get();
                    if (c->_active && c->_executionOrder <= 0) c->run(object, data);
                    else if (c->_executionOrder > 0) break;
                }
            }
            else
            {
                for (; index < _components.size(); ++index)
                {
                    Component* c = _components[index].get();
                    if (c->_active) c->run(object, data);
                }
            }
        }

        std::vector<osg::ref_ptr<Component>> _components;
        bool _dirty;
    };

#if OSG_VERSION_GREATER_THAN(3, 2, 1)
    typedef ComponentCallback NodeComponentCallback;
    typedef ComponentCallback DrawableComponentCallback;
#else
    class NodeComponentCallback : public osg::NodeCallback, public ComponentCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            size_t index = 0; runNestedComponents(node, nv, index, true);
            traverse(node, nv); runNestedComponents(node, nv, index, false);
        }
    };

    class DrawableComponentCallback : public osg::Drawable::UpdateCallback, public ComponentCallback
    {
    public:
        virtual void update(osg::NodeVisitor* nv, osg::Drawable* drawable)
        {
            size_t index = 0; runNestedComponents(drawable, nv, index, true);
            runNestedComponents(drawable, nv, index, false);
        }
    };
#endif

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

        void run(osg::RenderInfo& renderInfo) const { operator()(renderInfo); }
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

    /** String vector object for use as UserCallback::Parameter */
    class StringObject : public osg::Object
    {
    public:
        StringObject() {}
        StringObject(const StringObject& co, const osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Object(co, copyop), values(co.values) {}
        META_Object(osgVerse, StringObject);
        std::vector<std::string> values;
    };

    /** Callback for attaching a script to a Node's via there UserDataContainer
        for the purpose of overriding class methods within scripts.*/
#if OSG_VERSION_GREATER_THAN(3, 2, 1)
    class UserCallback : public osg::CallbackObject
    {
    public:
        typedef osg::Parameters Parameters;
        UserCallback() : osg::CallbackObject() {}
        UserCallback(const std::string& n) : osg::CallbackObject(n) {}
        UserCallback(const UserCallback& co, const osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::CallbackObject(co, copyop) {}
        META_Object(osgVerse, UserCallback);

        virtual bool run(osg::Object* object, Parameters& in, Parameters& out) const
        { return false; }
    };
#else
    class UserCallback : public virtual osg::Object
    {
    public:
        UserCallback() {}
        UserCallback(const std::string& n) { setName(n); }
        UserCallback(const UserCallback& co, const osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Object(co, copyop) {}
        META_Object(osgVerse, UserCallback);

        typedef std::vector< osg::ref_ptr<osg::Object> > Parameters;
        virtual bool run(osg::Object* object, Parameters& in, Parameters& out) const
        { return false; }
    };
#endif

    /** Suggest run this function once to initialize some plugins & environments */
    extern void globalInitialize(int argc, char** argv, const std::string& baseDir = BASE_DIR);
}

namespace osg
{
#if OSG_VERSION_LESS_THAN(3, 1, 9)
    class Vec3i
    {
    public:
        typedef int value_type;
        enum { num_components = 3 };
        value_type _v[3];

        Vec3i() { _v[0] = 0; _v[1] = 0; _v[2] = 0; }
        Vec3i(value_type x, value_type y, value_type z) { _v[0] = x; _v[1] = y; _v[2] = z; }

        inline bool operator == (const Vec3i& v) const { return _v[0] == v._v[0] && _v[1] == v._v[1] && _v[2] == v._v[2]; }
        inline bool operator != (const Vec3i& v) const { return _v[0] != v._v[0] || _v[1] != v._v[1] || _v[2] != v._v[2]; }
        inline bool operator <  (const Vec3i& v) const
        {
            if (_v[0] < v._v[0]) return true;
            else if (_v[0] > v._v[0]) return false;
            else if (_v[1] < v._v[1]) return true;
            else if (_v[1] > v._v[1]) return false;
            else return (_v[2] < v._v[2]);
        }

        inline void set(value_type x, value_type y, value_type z) { _v[0] = x; _v[1] = y; _v[2] = z; }
        inline void set(const Vec3i& rhs) { _v[0] = rhs._v[0]; _v[1] = rhs._v[1]; _v[2] = rhs._v[2]; }
        inline value_type& operator [] (int i) { return _v[i]; }
        inline value_type operator [] (int i) const { return _v[i]; }
    };
#endif
}

#endif
