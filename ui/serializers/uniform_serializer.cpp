#include <osg/Uniform>
#include <osg/UniformBase>
#include <osg/StateSet>
#include "serializer_utils.h"
#include <cstdio>
using namespace osgVerse;

namespace
{
    static int getUniformComponentCount(osg::Uniform::Type t)
    {
        switch (t)
        {
        case osg::Uniform::FLOAT: case osg::Uniform::DOUBLE:
        case osg::Uniform::INT: case osg::Uniform::UNSIGNED_INT:
        case osg::Uniform::BOOL: return 1;
        case osg::Uniform::FLOAT_VEC2: case osg::Uniform::DOUBLE_VEC2:
        case osg::Uniform::INT_VEC2: case osg::Uniform::UNSIGNED_INT_VEC2:
        case osg::Uniform::BOOL_VEC2: return 2;
        case osg::Uniform::FLOAT_VEC3: case osg::Uniform::DOUBLE_VEC3:
        case osg::Uniform::INT_VEC3: case osg::Uniform::UNSIGNED_INT_VEC3:
        case osg::Uniform::BOOL_VEC3: return 3;
        case osg::Uniform::FLOAT_VEC4: case osg::Uniform::DOUBLE_VEC4:
        case osg::Uniform::INT_VEC4: case osg::Uniform::UNSIGNED_INT_VEC4:
        case osg::Uniform::BOOL_VEC4: return 4;
        default: return 0;
        }
    }

    static DataValueKind getUniformValueKind(osg::Uniform::Type t)
    {
        switch (t)
        {
        case osg::Uniform::INT: case osg::Uniform::INT_VEC2:
        case osg::Uniform::INT_VEC3: case osg::Uniform::INT_VEC4:
        case osg::Uniform::BOOL: case osg::Uniform::BOOL_VEC2:
        case osg::Uniform::BOOL_VEC3: case osg::Uniform::BOOL_VEC4:
            return SignedIntValue;
        case osg::Uniform::UNSIGNED_INT: case osg::Uniform::UNSIGNED_INT_VEC2:
        case osg::Uniform::UNSIGNED_INT_VEC3: case osg::Uniform::UNSIGNED_INT_VEC4:
            return UnsignedIntValue;
        default: return FloatValue;
        }
    }

    static bool isMatrixType(osg::Uniform::Type t)
    { return t >= osg::Uniform::FLOAT_MAT2 && t <= osg::Uniform::DOUBLE_MAT4x3; }

    // Read the whole uniform value (index < 0) or one element of an array uniform
    #define UNIFORM_READ_SCALAR(TYPE, TYPE_ENUM, EXPR) \
        case osg::Uniform::TYPE_ENUM: { \
            TYPE tmp = (TYPE)0; \
            bool ok = (index < 0) ? u->get(tmp) : u->getElement((unsigned int)index, tmp); \
            if (!ok) return false; v[0] = (EXPR); return true; }

    #define UNIFORM_READ_VECTOR(TYPE, TYPE_ENUM, NUM) \
        case osg::Uniform::TYPE_ENUM: { \
            TYPE tmp; \
            bool ok = (index < 0) ? u->get(tmp) : u->getElement((unsigned int)index, tmp); \
            if (!ok) return false; \
            for (int c = 0; c < NUM; ++c) v[c] = tmp[c]; return true; }

    // Integer/boolean vector uniforms have no single set()/get() overload with a vector type
    #define UNIFORM_READ_VALUE_VECTOR(NUM, TYPE_ENUM, TYPE, ...) \
        case osg::Uniform::TYPE_ENUM: { TYPE args[NUM] = {}; bool ok; \
            ok = (index < 0) ? u->get(__VA_ARGS__) : u->getElement((unsigned int)index, __VA_ARGS__); \
            if (!ok) return false; \
            for (int c = 0; c < NUM; ++c) v[c] = args[c]; return true; }

    static bool readUniformValue(osg::Uniform* u, int index, double* v)
    {
        switch (u->getType())
        {
        UNIFORM_READ_SCALAR(float, FLOAT, tmp)
        UNIFORM_READ_SCALAR(double, DOUBLE, tmp)
        UNIFORM_READ_SCALAR(int, INT, tmp)
        UNIFORM_READ_SCALAR(unsigned int, UNSIGNED_INT, tmp)
        UNIFORM_READ_SCALAR(bool, BOOL, tmp ? 1.0 : 0.0)
        UNIFORM_READ_VECTOR(osg::Vec2, FLOAT_VEC2, 2)
        UNIFORM_READ_VECTOR(osg::Vec3, FLOAT_VEC3, 3)
        UNIFORM_READ_VECTOR(osg::Vec4, FLOAT_VEC4, 4)
        UNIFORM_READ_VECTOR(osg::Vec2d, DOUBLE_VEC2, 2)
        UNIFORM_READ_VECTOR(osg::Vec3d, DOUBLE_VEC3, 3)
        UNIFORM_READ_VECTOR(osg::Vec4d, DOUBLE_VEC4, 4)
        UNIFORM_READ_VALUE_VECTOR(2, INT_VEC2, int, args[0], args[1])
        UNIFORM_READ_VALUE_VECTOR(3, INT_VEC3, int, args[0], args[1], args[2])
        UNIFORM_READ_VALUE_VECTOR(4, INT_VEC4, int, args[0], args[1], args[2], args[3])
        UNIFORM_READ_VALUE_VECTOR(2, UNSIGNED_INT_VEC2, unsigned int, args[0], args[1])
        UNIFORM_READ_VALUE_VECTOR(3, UNSIGNED_INT_VEC3, unsigned int, args[0], args[1], args[2])
        UNIFORM_READ_VALUE_VECTOR(4, UNSIGNED_INT_VEC4, unsigned int,
                                  args[0], args[1], args[2], args[3])
        UNIFORM_READ_VALUE_VECTOR(2, BOOL_VEC2, bool, args[0], args[1])
        UNIFORM_READ_VALUE_VECTOR(3, BOOL_VEC3, bool, args[0], args[1], args[2])
        UNIFORM_READ_VALUE_VECTOR(4, BOOL_VEC4, bool, args[0], args[1], args[2], args[3])
        default: return false;
        }
    }

    #define UNIFORM_WRITE_SCALAR(TYPE, TYPE_ENUM, EXPR) \
        case osg::Uniform::TYPE_ENUM: { TYPE tmp = (TYPE)(EXPR); return u->set(tmp); }

    #define UNIFORM_WRITE_VECTOR(TYPE, TYPE_ENUM, NUM) \
        case osg::Uniform::TYPE_ENUM: { TYPE tmp; \
            for (int c = 0; c < NUM; ++c) tmp[c] = (TYPE::value_type)v[c]; return u->set(tmp); }

    #define UNIFORM_WRITE_VALUE_VECTOR(NUM, TYPE_ENUM, TYPE, ...) \
        case osg::Uniform::TYPE_ENUM: { TYPE args[NUM]; \
            for (int c = 0; c < NUM; ++c) args[c] = (TYPE)v[c]; return u->set(__VA_ARGS__); }

    static bool writeUniformValue(osg::Uniform* u, const double* v)
    {
        switch (u->getType())
        {
        UNIFORM_WRITE_SCALAR(float, FLOAT, v[0])
        UNIFORM_WRITE_SCALAR(double, DOUBLE, v[0])
        UNIFORM_WRITE_SCALAR(int, INT, v[0])
        UNIFORM_WRITE_SCALAR(unsigned int, UNSIGNED_INT, v[0])
        UNIFORM_WRITE_SCALAR(bool, BOOL, v[0] != 0.0)
        UNIFORM_WRITE_VECTOR(osg::Vec2, FLOAT_VEC2, 2)
        UNIFORM_WRITE_VECTOR(osg::Vec3, FLOAT_VEC3, 3)
        UNIFORM_WRITE_VECTOR(osg::Vec4, FLOAT_VEC4, 4)
        UNIFORM_WRITE_VECTOR(osg::Vec2d, DOUBLE_VEC2, 2)
        UNIFORM_WRITE_VECTOR(osg::Vec3d, DOUBLE_VEC3, 3)
        UNIFORM_WRITE_VECTOR(osg::Vec4d, DOUBLE_VEC4, 4)
        UNIFORM_WRITE_VALUE_VECTOR(2, INT_VEC2, int, args[0], args[1])
        UNIFORM_WRITE_VALUE_VECTOR(3, INT_VEC3, int, args[0], args[1], args[2])
        UNIFORM_WRITE_VALUE_VECTOR(4, INT_VEC4, int, args[0], args[1], args[2], args[3])
        UNIFORM_WRITE_VALUE_VECTOR(2, UNSIGNED_INT_VEC2, unsigned int, args[0], args[1])
        UNIFORM_WRITE_VALUE_VECTOR(3, UNSIGNED_INT_VEC3, unsigned int, args[0], args[1], args[2])
        UNIFORM_WRITE_VALUE_VECTOR(4, UNSIGNED_INT_VEC4, unsigned int,
                                   args[0], args[1], args[2], args[3])
        UNIFORM_WRITE_VALUE_VECTOR(2, BOOL_VEC2, bool, args[0], args[1])
        UNIFORM_WRITE_VALUE_VECTOR(3, BOOL_VEC3, bool, args[0], args[1], args[2])
        UNIFORM_WRITE_VALUE_VECTOR(4, BOOL_VEC4, bool, args[0], args[1], args[2], args[3])
        default: return false;
        }
    }
}

/** Serializer UI of a single osg::Uniform, also used by the StateSet serializer */
class UniformSerializerInterface : public SerializerBaseItem
{
public:
    UniformSerializerInterface(osg::UniformBase* uniform, osg::StateSet* parent,
                               int value, int unit = -1)
    :   SerializerBaseItem(uniform, true), _uniform(dynamic_cast<osg::Uniform*>(uniform)),
        _parent(parent), _value(value), _unit(unit)
    {
        osg::Uniform* u = _uniform.get();
        _type = u ? u->getType() : osg::Uniform::UNDEFINED;
        _numElements = u ? (int)u->getNumElements() : 0;
        _components = getUniformComponentCount(_type);
        _kind = getUniformValueKind(_type);
        _name = uniform ? uniform->getName() : "";

        _valueField = new InputValueField(TR(_name) + _postfix);
        _valueField->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            double v[4] = { _valueField->value, 0.0, 0.0, 0.0 };
            osg::Uniform* u = _uniform.get();
            if (u != NULL && writeUniformValue(u, v)) doneEditing();
        };

        _vectorField = new InputVectorField(TR(_name) + _postfix);
        _vectorField->vecNumber = (_components > 0) ? _components : 1;
        _vectorField->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            double v[4] = { 0.0, 0.0, 0.0, 0.0 };
            osg::Vec4d vec; _vectorField->getVector(vec);
            for (int c = 0; c < 4; ++c) v[c] = vec[c];
            osg::Uniform* u = _uniform.get();
            if (u != NULL && writeUniformValue(u, v)) doneEditing();
        };

        _boolCheck = new CheckBox(TR(_name) + _postfix, false);
        _boolCheck->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            double v[4] = { _boolCheck->value ? 1.0 : 0.0, 0.0, 0.0, 0.0 };
            osg::Uniform* u = _uniform.get();
            if (u != NULL && writeUniformValue(u, v)) doneEditing();
        };

        _detailsButton = new Button(TR("Show Details") + _postfix);
        //_detailsButton->tooltip = TR(std::string(uniform->libraryName()) + "::" + uniform->className());
        _detailsButton->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        { openDetailsWindow(); };
    }

    virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
    { return -1; }  // TODO

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        std::string title = TR(_name) + " [" + osg::Uniform::getTypename(_type) + "]" + _postfix;
        if (_unit >= 0) title = TR("Unit" + std::to_string(_unit) + ": ") + title;
        return showInternal(mgr, content, title);
    }

protected:
    void openDetailsWindow()
    {
        osg::Uniform* u = _uniform.get();
        if (u == NULL) return;

        if (!_detailsWindow)
        {
            _detailsWindow = new NumberTableWindow(TR(_name) + "###uniform" + _postfix.substr(2));
            _detailsWindow->pos = osg::Vec2(220.0f, 180.0f);
            _detailsWindow->size = osg::Vec2(460.0f, 360.0f);
            _detailsWindow->flags = ImGuiWindowFlags_NoCollapse;
        }

        const int components = (_components > 0) ? _components : 1;
        osg::ref_ptr<SnapshotTableData> data = new SnapshotTableData(
            _kind, components, osg::Uniform::getTypename(_type));
        const char* floatLabels[4] = { "x", "y", "z", "w" };
        for (int c = 0; c < 4; ++c) data->labels[c] = floatLabels[c];

        double v[4] = { 0.0, 0.0, 0.0, 0.0 };
        for (int i = 0; (i < _numElements) && readUniformValue(u, i, v); ++i)
        {
            osg::Vec4d row;
            for (int c = 0; c < 4; ++c) row[c] = v[c];
            data->rows.push_back(row);
        }

        _detailsWindow->data = data;
        _detailsWindow->isOpen = true; _detailsWindow->sizeApplied = false;
        ImGuiComponentBase::registerFloatingWindow(_detailsWindow.get());
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        osg::Uniform* u = _uniform.get();
        if (u == NULL)
        {
            ImGui::TextDisabled("%s",
                TR("(generic uniform, editing not supported)").c_str());
            return false;
        }

        ImGui::Text("%s", osg::Uniform::getTypename(_type));
        ImGui::SameLine(); ImGui::Text("| %s: %d", TR("Elements").c_str(), _numElements);
        ImGui::SameLine(0.0f, 12.0f);
        if (_numElements > 1) return _detailsButton->show(mgr, content);

        double v[4] = { 0.0, 0.0, 0.0, 0.0 };
        if (isMatrixType(_type)) return showMatrix(u);

        bool done = false;
        if (_type == osg::Uniform::BOOL)
        {
            if (isDirty() && readUniformValue(u, -1, v)) _boolCheck->value = (v[0] != 0.0);
            done |= _boolCheck->show(mgr, content);
        }
        else if (_components == 1)
        {
            if (isDirty() && readUniformValue(u, -1, v))
            {
                _valueField->value = v[0];
                _valueField->type = (_kind == FloatValue) ? InputValueField::FloatValue :
                    ((_kind == UnsignedIntValue) ? InputValueField::UIntValue : InputValueField::IntValue);
            }
            done |= _valueField->show(mgr, content);
        }
        else if (_components > 1)
        {
            if (isDirty() && readUniformValue(u, -1, v))
            {
                osg::Vec4d vec; for (int c = 0; c < 4; ++c) vec[c] = v[c];
                _vectorField->setVector(vec);
                _vectorField->type = (_kind == FloatValue) ? InputValueField::FloatValue :
                    ((_kind == UnsignedIntValue) ? InputValueField::UIntValue : InputValueField::IntValue);
            }
            done |= _vectorField->show(mgr, content);
        }
        else
        {
            ImGui::TextDisabled("%s", TR("(type not supported)").c_str());
        }
        return done;
    }

    bool showMatrix(osg::Uniform* u)
    {
        char buffer[256] = { 0 };
        switch (_type)
        {
        case osg::Uniform::FLOAT_MAT2:
            { osg::Matrix2 m; if (u->get(m)) snprintf(buffer, 256, "[%.3g %.3g; %.3g %.3g]",
                m(0, 0), m(0, 1), m(1, 0), m(1, 1)); } break;
        case osg::Uniform::FLOAT_MAT3:
            { osg::Matrix3 m; if (u->get(m)) snprintf(buffer, 256, "[%.3g %.3g %.3g; ...]",
                m(0, 0), m(0, 1), m(0, 2)); } break;
        case osg::Uniform::FLOAT_MAT4:
            { osg::Matrixf m; if (u->get(m)) snprintf(buffer, 256, "[%.3g %.3g %.3g %.3g; ...]",
                m(0, 0), m(0, 1), m(0, 2), m(0, 3)); } break;
        case osg::Uniform::DOUBLE_MAT4:
            { osg::Matrixd m; if (u->get(m)) snprintf(buffer, 256, "[%.3g %.3g %.3g %.3g; ...]",
                m(0, 0), m(0, 1), m(0, 2), m(0, 3)); } break;
        default:
            ImGui::TextDisabled("%s", TR("(type not supported)").c_str()); return false;
        }
        ImGui::TextUnformatted(buffer);
        return false;
    }

    virtual void showMenuItems(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        osg::Uniform* u = _uniform.get();
        if (ImGui::MenuItem(TR("Delete").c_str()))
        {
            if (u != NULL && _parent.valid()) _parent->removeUniform(u);
            structureChanged(); return;
        }

        if (ImGui::MenuItem(TR((_value & osg::StateAttribute::ON) ?
                               "Turn Off" : "Turn On").c_str()))
        {
            int newValue = _value ^ (int)osg::StateAttribute::ON;
            if (u != NULL && _parent.valid()) _parent->addUniform(u, newValue);
            _value = newValue;
        }

        if (ImGui::MenuItem(TR((_value & osg::StateAttribute::OVERRIDE) ?
                               "Override Off" : "Override On").c_str()))
        {
            int newValue = _value ^ (int)osg::StateAttribute::OVERRIDE;
            if (u != NULL && _parent.valid()) _parent->addUniform(u, newValue);
            _value = newValue;
        }

        if (ImGui::MenuItem(TR((_value & osg::StateAttribute::PROTECTED) ?
                               "Unprotect" : "Protect").c_str()))
        {
            int newValue = _value ^ (int)osg::StateAttribute::PROTECTED;
            if (u != NULL && _parent.valid()) _parent->addUniform(u, newValue);
            _value = newValue;
        }
        SerializerBaseItem::showMenuItems(mgr, content);
    }

private:
    osg::ref_ptr<osg::Uniform> _uniform;
    osg::observer_ptr<osg::StateSet> _parent;
    osg::ref_ptr<InputValueField> _valueField;
    osg::ref_ptr<InputVectorField> _vectorField;
    osg::ref_ptr<CheckBox> _boolCheck;
    osg::ref_ptr<Button> _detailsButton;
    osg::ref_ptr<NumberTableWindow> _detailsWindow;
    std::string _name;
    osg::Uniform::Type _type;
    int _numElements, _components, _value, _unit;
    DataValueKind _kind;
};

SerializerBaseItem* osgVerse::createUniformSerializerItem(
    osg::UniformBase* uniform, osg::StateSet* parent, int value, int unit)
{ return new UniformSerializerInterface(uniform, parent, value, unit); }
