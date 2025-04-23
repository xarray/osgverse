#ifndef MANA_WRAPPERS_GENERICRESERIALIZER_HPP
#define MANA_WRAPPERS_GENERICRESERIALIZER_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <functional>
#include "GenericInputStream.h"

namespace osgVerse
{
    struct BaseSerializer
    {
        typedef std::function<void(InputStream& is, InputUserData& ud)> ReadFunc;
        BaseSerializer() : _firstVersion(0), _lastVersion(INT_MAX) {}
        int _firstVersion, _lastVersion;
    };

    struct UserSerializer : public BaseSerializer { ReadFunc _reader; };
    struct BoolSerializer : public BaseSerializer { bool _v0; };
    struct StringSerializer : public BaseSerializer { std::string _v0; };
    struct EnumSerializer : public BaseSerializer { std::string _v0; std::set<std::string> _values; };
    template<typename T> struct ValueSerializer : public BaseSerializer { T _v0; bool _hex; };

    class Rewrapper
    {
    public:
        template<typename T> void addValueSerializer(const std::string& prop, T def, bool hex = false)
        { ValueSerializer<T>* s = new ValueSerializer<T>(); s->_v0 = def; s->_hex = hex; add(prop, s); }

        template<typename T> void addRefValueSerializer(const std::string& prop, const T& def, bool hex = false)
        { ValueSerializer<T>* s = new ValueSerializer<T>(); s->_v0 = def; s->_hex = hex; add(prop, s); }

        void addBoolSerializer(const std::string& prop, bool def)
        { BoolSerializer* s = new BoolSerializer; s->_v0 = def; add(prop, s); }

        void addStringSerializer(const std::string& prop, const std::string& def)
        { StringSerializer* s = new StringSerializer; s->_v0 = def; add(prop, s); }

        void addUserSerializer(const std::string& prop, BaseSerializer::ReadFunc f)
        { UserSerializer* s = new UserSerializer; s->_reader = f; add(prop, s); }

        void beginEnumSerializer(const std::string& prop, const std::string& value)
        { _lastEnum = new EnumSerializer; _lastEnum->_v0 = value; add(prop, _lastEnum); }

        void addEnumValue(const std::string& v) { if (_lastEnum) _lastEnum->_values.insert(v); }
        void endEnumSerializer() { _lastEnum = NULL; }

        void add(const std::string& n, BaseSerializer* s) { _serializers.push_back(SerializerPair(n ,s)); }
        void clear() { for (auto pair : _serializers) delete pair.second; _serializers.clear(); }

    protected:
        typedef std::pair<std::string, BaseSerializer*> SerializerPair;
        std::vector<SerializerPair> _serializers;
        EnumSerializer* _lastEnum;
    };
}

#define ADD_USER_SERIALIZER(PROP) wrapper.addUserSerializer(#PROP, &read##PROP)
#define ADD_BOOL_SERIALIZER(PROP, DEF) wrapper.addBoolSerializer(#PROP, DEF)
#define ADD_STRING_SERIALIZER(PROP, DEF) wrapper.addStringSerializer(#PROP, DEF)

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

#define REGISTER_OBJECT_WRAPPER(NAME, CREATEINSTANCE, CLASS, ASSOCIATES) \
    extern "C" void rewrapper_serializer_##NAME(void) {} \
    extern void rewrapper_propfunc_##NAME(Rewrapper& wrapper); \
    void rewrapper_propfunc_##NAME(Rewrapper& wrapper)

#endif
