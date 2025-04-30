#ifndef MANA_WRAPPERS_GENERICRESERIALIZER_HPP
#define MANA_WRAPPERS_GENERICRESERIALIZER_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <memory>
#include <climits>
#include "GenericInputStream.h"

namespace osgVerse
{
    struct BaseSerializer;
    struct ObjectSerializer;
    struct ImageSerializer;
    struct VectorSerializer;
    struct UserSerializer;
    struct BoolSerializer;
    struct StringSerializer;
    struct EnumSerializer;
    template<typename T> struct ValueSerializer;

    class SerializerVisitor
    {
    public:
        virtual void apply(BaseSerializer& obj);
        template<typename T> void apply(ValueSerializer<T>& obj);
        virtual void apply(ObjectSerializer& obj) {}
        virtual void apply(ImageSerializer& obj) {}
        virtual void apply(VectorSerializer& obj) {}
        virtual void apply(UserSerializer& obj) {}
        virtual void apply(BoolSerializer& obj) {}
        virtual void apply(StringSerializer& obj) {}
        virtual void apply(EnumSerializer& obj) {}

        template<typename T>
        void registerType(std::function<void(ValueSerializer<T>&)> func)
        {
            _registry[std::type_index(typeid(T))] = [func](BaseSerializer& baseObj)
            { ValueSerializer<T>& obj = static_cast<ValueSerializer<T>&>(baseObj); func(obj); };
        }

    protected:
        typedef std::map<std::type_index, std::function<void(BaseSerializer&)>> SerializerMap;
        SerializerMap _registry;
    };
#define META_VISITOR() virtual void accept(SerializerVisitor& v) { v.apply(*this); }

    struct BaseSerializer
    {
        typedef std::function<void(InputStream& is, InputUserData& ud)> ReadFunc;
        BaseSerializer() : _firstVersion(0), _lastVersion(INT_MAX) {}
        int _firstVersion, _lastVersion; META_VISITOR()
    };

    struct WrapperAssociate
    {
        WrapperAssociate(const std::string& name) : _firstVersion(0), _lastVersion(INT_MAX), _name(name) {}
        int _firstVersion, _lastVersion; std::string _name;
    };

    struct ObjectSerializer : public BaseSerializer { std::string _type; META_VISITOR() };
    struct ImageSerializer : public BaseSerializer { std::string _type; META_VISITOR() };
    struct VectorSerializer : public BaseSerializer { std::string _type; META_VISITOR() };
    struct UserSerializer : public BaseSerializer { ReadFunc _reader; META_VISITOR() };
    struct BoolSerializer : public BaseSerializer { bool _v0; META_VISITOR() };
    struct GLenumSerializer : public BaseSerializer { std::string _type; unsigned int _v0; META_VISITOR() };
    struct StringSerializer : public BaseSerializer { std::string _v0; META_VISITOR() };
    struct EnumSerializer : public BaseSerializer { std::string _v0; std::set<std::string> _values; META_VISITOR() };
    template<typename T> struct ValueSerializer : public BaseSerializer { T _v0; bool _hex; META_VISITOR() };

    template<typename T> void SerializerVisitor::apply(ValueSerializer<T>& obj)
    { static_cast<BaseSerializer&>(obj).accept(*this); }

    class Rewrapper
    {
    public:
        template<typename T> void addValueSerializer(const std::string& prop, T def, bool hex = false)
        { ValueSerializer<T>* s = new ValueSerializer<T>(); s->_v0 = def; s->_hex = hex; add(prop, s); }

        template<typename T> void addRefValueSerializer(const std::string& prop, const T& def, bool hex = false)
        { ValueSerializer<T>* s = new ValueSerializer<T>(); s->_v0 = def; s->_hex = hex; add(prop, s); }

        void addBoolSerializer(const std::string& prop, bool def)
        { BoolSerializer* s = new BoolSerializer; s->_v0 = def; add(prop, s); }

        void addGLenumSerializer(const std::string& prop, const std::string& type, unsigned int def)
        { GLenumSerializer* s = new GLenumSerializer; s->_type = type; s->_v0 = def; add(prop, s); }

        void addStringSerializer(const std::string& prop, const std::string& def)
        { StringSerializer* s = new StringSerializer; s->_v0 = def; add(prop, s); }

        void addObjectSerializer(const std::string& prop, const std::string& type)
        { ObjectSerializer* s = new ObjectSerializer; s->_type = type; add(prop, s); }

        void addImageSerializer(const std::string& prop, const std::string& type)
        { ImageSerializer* s = new ImageSerializer; s->_type = type; add(prop, s); }

        void addVectorSerializer(const std::string& prop, const std::string& type)
        { VectorSerializer* s = new VectorSerializer; s->_type = type; add(prop, s); }

        void addUserSerializer(const std::string& prop, BaseSerializer::ReadFunc f)
        { UserSerializer* s = new UserSerializer; s->_reader = f; add(prop, s); }

        void beginEnumSerializer(const std::string& prop, const std::string& value)
        { _lastEnum = new EnumSerializer; _lastEnum->_v0 = value; add(prop, _lastEnum); }

        void addEnumValue(const std::string& v) { if (_lastEnum) _lastEnum->_values.insert(v); }
        void endEnumSerializer() { _lastEnum = NULL; }

        void setAssociates(const std::string& a) { splitAssociates(a); }
        void add(const std::string& n, BaseSerializer* s);
        void clear();

        void setUpdatedVersion(int v) { _version = v; }
        int getUpdatedVersion() const { return _version; }

        void markSerializerAsRemoved(const std::string& name);
        void markAssociateAsRemoved(const std::string& name);
        void markAssociateAsAdded(const std::string& name);

        Rewrapper() : _lastEnum(NULL), _version(0) {}
        void accept(SerializerVisitor& v, int inputVersion, bool includingAssociates);

    protected:
        void splitAssociates(const std::string& associates);

        typedef std::pair<std::string, BaseSerializer*> SerializerPair;
        std::vector<SerializerPair> _serializers;
        std::vector<WrapperAssociate> _associates;
        EnumSerializer* _lastEnum; int _version;
    };

    class RewrapperManager
    {
    public:
        static std::shared_ptr<RewrapperManager> instance();
        virtual ~RewrapperManager();

        void addRewrapper(const std::string& name, Rewrapper* r);
        void removeRewrapper(const std::string& name);
        Rewrapper* getRewrapper(const std::string& name);

        typedef std::map<std::string, Rewrapper*> RewrapperMap;
        const RewrapperMap& getRewrappers() const { return _rewrappers; }

    protected:
        RewrapperMap _rewrappers;
    };

    struct RegisterWrapperProxy
    {
    public:
        typedef void (*AddPropFunc)(Rewrapper&);
        RegisterWrapperProxy(const std::string& name, const std::string& associates, AddPropFunc func);
        virtual ~RegisterWrapperProxy(); std::string _className;
    };

    struct UpdateWrapperVersionProxy
    {
        UpdateWrapperVersionProxy(Rewrapper* w, int v) : _wrapper(w)
        { _lastVersion = w->getUpdatedVersion(); w->setUpdatedVersion(v); }

        ~UpdateWrapperVersionProxy() { _wrapper->setUpdatedVersion(_lastVersion); }
        Rewrapper* _wrapper; int _lastVersion;
    };
}

#define ADD_USER_SERIALIZER(PROP) wrapper.addUserSerializer(#PROP, &read##PROP)
#define ADD_BOOL_SERIALIZER(PROP, DEF) wrapper.addBoolSerializer(#PROP, DEF)
#define ADD_GLENUM_SERIALIZER(PROP, TYPE, DEF) wrapper.addGLenumSerializer(#PROP, #TYPE, DEF)
#define ADD_STRING_SERIALIZER(PROP, DEF) wrapper.addStringSerializer(#PROP, DEF)
#define ADD_OBJECT_SERIALIZER(PROP, TYPE, DEF) wrapper.addObjectSerializer(#PROP, #TYPE)
#define ADD_IMAGE_SERIALIZER(PROP, TYPE, DEF) wrapper.addImageSerializer(#PROP, #TYPE)
#define ADD_ISAVECTOR_SERIALIZER(PROP, ETYPE, NUMONROW) wrapper.addVectorSerializer(#PROP, #ETYPE)

#define ADD_CHAR_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<char>(#PROP, DEF)
#define ADD_UCHAR_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned char>(#PROP, DEF)
#define ADD_SHORT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<short>(#PROP, DEF)
#define ADD_USHORT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned short>(#PROP, DEF)
#define ADD_HEXSHORT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned short>(#PROP, DEF, true)
#define ADD_INT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<int>(#PROP, DEF)
#define ADD_UINT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned int>(#PROP, DEF)
#define ADD_GLINT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<GLint>(#PROP, DEF)
#define ADD_HEXINT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned int>(#PROP, DEF, true)
#define ADD_FLOAT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<float>(#PROP, DEF)
#define ADD_DOUBLE_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<double>(#PROP, DEF)
#define ADD_VEC2B_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2b>(#PROP, DEF)
#define ADD_VEC2UB_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2ub>(#PROP, DEF)
#define ADD_VEC2S_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2s>(#PROP, DEF)
#define ADD_VEC2US_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2us>(#PROP, DEF)
#define ADD_VEC2I_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2i>(#PROP, DEF)
#define ADD_VEC2UI_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2ui>(#PROP, DEF)
#define ADD_VEC2F_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2f>(#PROP, DEF)
#define ADD_VEC2D_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2d>(#PROP, DEF)
#define ADD_VEC2_SERIALIZER(PROP, DEF) ADD_VEC2F_SERIALIZER(PROP, DEF)
#define ADD_VEC3B_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3b>(#PROP, DEF)
#define ADD_VEC3UB_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3ub>(#PROP, DEF)
#define ADD_VEC3S_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3s>(#PROP, DEF)
#define ADD_VEC3US_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3us>(#PROP, DEF)
#define ADD_VEC3I_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3i>(#PROP, DEF)
#define ADD_VEC3UI_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3ui>(#PROP, DEF)
#define ADD_VEC3F_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3f>(#PROP, DEF)
#define ADD_VEC3D_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3d>(#PROP, DEF)
#define ADD_VEC3_SERIALIZER(PROP, DEF) ADD_VEC3F_SERIALIZER(PROP, DEF)
#define ADD_VEC4B_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4b>(#PROP, DEF)
#define ADD_VEC4UB_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4ub>(#PROP, DEF)
#define ADD_VEC4S_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4s>(#PROP, DEF)
#define ADD_VEC4US_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4us>(#PROP, DEF)
#define ADD_VEC4I_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4i>(#PROP, DEF)
#define ADD_VEC4UI_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4ui>(#PROP, DEF)
#define ADD_VEC4F_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4f>(#PROP, DEF)
#define ADD_VEC4D_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4d>(#PROP, DEF)
#define ADD_VEC4_SERIALIZER(PROP, DEF) ADD_VEC4F_SERIALIZER(PROP, DEF)
#define ADD_QUAT_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Quat>(#PROP, DEF)
#define ADD_PLANE_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Plane>(#PROP, DEF)
#define ADD_MATRIXF_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Matrixf>(#PROP, DEF)
#define ADD_MATRIXD_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Matrixd>(#PROP, DEF)
#define ADD_BOUNDINGBOXF_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingBoxf>(#PROP, DEF)
#define ADD_BOUNDINGBOXD_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingBoxd>(#PROP, DEF)
#define ADD_BOUNDINGSPHEREF_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingSpheref>(#PROP, DEF)
#define ADD_BOUNDINGSPHERED_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingSphered>(#PROP, DEF)

#define BEGIN_ENUM_SERIALIZER(PROP, DEF) wrapper.beginEnumSerializer(#PROP, #DEF);
#define BEGIN_ENUM_SERIALIZER2(PROP, TYPE, DEF) wrapper.beginEnumSerializer(#PROP, #DEF);
#define ADD_ENUM_VALUE(VALUE) wrapper.addEnumValue(#VALUE);
#define END_ENUM_SERIALIZER() wrapper.endEnumSerializer();

#define UPDATE_TO_VERSION(VER) wrapper.setUpdatedVersion((VER));
#define UPDATE_TO_VERSION_SCOPED(VER) UpdateWrapperVersionProxy uwvp(&wrapper, (VER));
#define ADDED_ASSOCIATE(STR) wrapper.markAssociateAsAdded(STR);
#define REMOVED_ASSOCIATE(STR) wrapper.markAssociateAsRemoved(STR);
#define REMOVE_SERIALIZER(PROP) wrapper.markSerializerAsRemoved(#PROP);

#define REGISTER_OBJECT_WRAPPER(NAME, CREATEINSTANCE, CLASS, ASSOCIATES) \
    extern "C" void rewrapper_serializer_##NAME(void) {} \
    extern void rewrapper_propfunc_##NAME(Rewrapper& wrapper); \
    static RegisterWrapperProxy rewrapper_proxy_##NAME(#CLASS, ASSOCIATES, &rewrapper_propfunc_##NAME); \
    void rewrapper_propfunc_##NAME(Rewrapper& wrapper)

#endif
