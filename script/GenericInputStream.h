#ifndef MANA_SCRIPT_GENERICINPUTSTREAM_HPP
#define MANA_SCRIPT_GENERICINPUTSTREAM_HPP

#include <osgDB/InputStream>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>

namespace osgVerse
{
    typedef osgDB::InputStream InputStream;  // FIXME

    struct ObjectTypeAndID
    {
        std::string type, id;

        ObjectTypeAndID() {}
        ObjectTypeAndID(const std::string& t, const std::string& i) : type(t), id(i) {}
        virtual ObjectTypeAndID& operator=(const ObjectTypeAndID& r) { type = r.type; id = r.id; return *this; }
        bool valid() const { return !id.empty(); }
    };

    class InputUserData
    {
    public:
        bool get(const char* name, ...);
        bool get(const ObjectTypeAndID& parent, const char* name, ...);
        void add(const char* name, ...);
        void add(const ObjectTypeAndID& parent, const char* name, ...);
        ObjectTypeAndID readObjectFromStream(InputStream& is, const std::string& type);
    };

    class IntLookup
    {
    public:
        typedef int Value;
        typedef std::map<std::string, Value> StringToValue;
        typedef std::map<Value, std::string> ValueToString;
        IntLookup() {}
        unsigned int size() const { return static_cast<unsigned int>(_stringToValue.size()); }

        void add(const char* str, Value value)
        {
            if (_valueToString.find(value) != _valueToString.end())
            {
                std::cerr << "Duplicate enum value " << value
                          << " with old string: " << _valueToString[value]
                          << " and new string: " << str << std::endl;
            }
            _valueToString[value] = str;
            _stringToValue[str] = value;
        }

        void add2(const char* str, const char* newStr, Value value) {
            if (_valueToString.find(value) != _valueToString.end())
            {
                std::cerr << "Duplicate enum value " << value
                          << " with old string: " << _valueToString[value]
                          << " and new strings: " << str << " and " << newStr << std::endl;
            }
            _valueToString[value] = newStr;
            _stringToValue[newStr] = value;
            _stringToValue[str] = value;
        }

        Value getValue(const char* str)
        {
            StringToValue::iterator itr = _stringToValue.find(str);
            if (itr == _stringToValue.end())
            {
                Value value;
                std::stringstream stream;
                stream << str; stream >> value;
                _stringToValue[str] = value;
                return value;
            }
            return itr->second;
        }

        const std::string& getString(Value value)
        {
            ValueToString::iterator itr = _valueToString.find(value);
            if (itr == _valueToString.end())
            {
                std::string str;
                std::stringstream stream;
                stream << value; stream >> str;
                _valueToString[value] = str;
                return _valueToString[value];
            }
            return itr->second;
        }

        StringToValue& getStringToValue() { return _stringToValue; }
        const StringToValue& getStringToValue() const { return _stringToValue; }

        ValueToString& getValueToString() { return _valueToString; }
        const ValueToString& getValueToString() const { return _valueToString; }

    protected:
        StringToValue _stringToValue;
        ValueToString _valueToString;
    };

    class UserLookupTableProxy
    {
    public:
        typedef void (*AddValueFunc)(IntLookup* lookup);
        UserLookupTableProxy(AddValueFunc func) { if (func) (*func)(&_lookup); }
        IntLookup _lookup;
    };
}

#define BEGIN_USER_TABLE(NAME, CLASS) \
    static void add_user_value_func_##NAME(IntLookup*); \
    static UserLookupTableProxy s_user_lookup_table_##NAME(&add_user_value_func_##NAME); \
    static void add_user_value_func_##NAME(IntLookup* lookup) { typedef CLASS##_##NAME MyClass
#define ADD_USER_VALUE(VALUE) lookup->add(#VALUE, MyClass::VALUE)
#define END_USER_TABLE() }

#define USER_READ_FUNC(NAME, FUNCNAME) \
    static int FUNCNAME(InputStream& is) { \
        int value; if (is.isBinary()) is >> value; \
        else { std::string str; is >> str; \
        value = (s_user_lookup_table_##NAME)._lookup.getValue(str.c_str()); } \
        return value; }

#endif
