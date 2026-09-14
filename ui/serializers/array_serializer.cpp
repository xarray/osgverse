#include "serializer_utils.h"
using namespace osgVerse;

namespace
{
    static const char* kComponentLabels[4] = { "x", "y", "z", "w" };
    static const char* kColorLabels[4] = { "r", "g", "b", "a" };

    /** Cached information of an osg::Array, plus its detail table window */
    struct ArrayPanel
    {
        osg::observer_ptr<osg::Array> array;
        std::string typeName, storageName, title;
        int components; DataValueKind kind;
        unsigned int count; bool hasRange, isColor;
        osg::Vec4d minValue, maxValue;
        osg::ref_ptr<NumberTableWindow> window;

        ArrayPanel(const std::string& id)
            : components(0), kind(FloatValue), count(0), hasRange(false), isColor(false)
        { title = std::string("Array###array") + id; }

        const char* getLabel(int c) const
        { return (isColor && c < 4) ? kColorLabels[c] : kComponentLabels[(c < 4) ? c : 3]; }

        void refresh(osg::Array* a, bool color)
        {
            array = a; isColor = color; hasRange = false; count = 0;
            if (a == NULL)
            { typeName.clear(); storageName.clear(); components = 0; return; }

            components = getArrayComponents(a); if (components <= 0) components = 1;
            kind = getArrayValueKind(a); count = a->getNumElements();
            typeName = a->className();
            storageName = formatArrayStorageName(a->getDataType(), components);
            hasRange = computeArrayRange(a, minValue, maxValue);
        }

        void openWindow()
        {
            osg::Array* a = array.get(); if (a == NULL) return;
            if (!window)
            {
                window = new NumberTableWindow(title);
                window->pos = osg::Vec2(180.0f, 150.0f);
                window->size = osg::Vec2(520.0f, 420.0f);
                window->flags = ImGuiWindowFlags_NoCollapse;
            }

            osg::ref_ptr<ArrayTableData> data = new ArrayTableData(a);
            for (int i = 0; i < 4; ++i) data->labels[i] = getLabel(i);
            window->data = data;
            window->isOpen = true; window->sizeApplied = false;
            ImGuiComponentBase::registerFloatingWindow(window.get());
        }
    };

    class ArraySerializerInterface : public SerializerBaseItem
    {
    public:
        // Single array property, e.g. osg::Geometry::VertexArray
        ArraySerializerInterface(osg::Object* owner, LibraryEntry* entry,
                                 const LibraryEntry::Property& prop,
                                 const std::string& title, bool asColor)
        :   SerializerBaseItem(owner, false), _entry(entry), _property(prop), _title(title),
            _fromProperty(true), _asColor(asColor), _panel(_postfix.substr(2))
        { createComponents(); }

        // One element of an array list, e.g. unit 6 of TexCoordArrayList
        ArraySerializerInterface(osg::Array* array, const std::string& title, bool asColor)
        :   SerializerBaseItem(array, false), _title(title), _fromProperty(false),
            _asColor(asColor), _panel(_postfix.substr(2))
        { _arrayObject = array; createComponents(); }

        osg::Array* getArrayObject() const { return _arrayObject.get(); }
        const std::string& getTitle() const { return _title; }

        virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
        { return -1; }  // TODO

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
        { return showInternal(mgr, content, TR(_title) + _postfix); }

    protected:
        void createComponents()
        {
            _button = new Button(TR("Show Details") + _postfix);
            //_button->tooltip = TR(tooltip(_property));
            _button->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
            { _panel.openWindow(); };

            _typeCombo = new ComboBox("##arraytype" + _postfix);
            _typeCombo->width = 90; _typeCombo->readonly = true;

            _totalField = new InputValueField(TR("Total") + _postfix);
            _totalField->type = InputValueField::IntValue;
            _totalField->step = 0.0; _totalField->width = 100; _totalField->readonly = true;

            _minVector = new InputVectorField(TR("Min") + _postfix);
            _maxVector = new InputVectorField(TR("Max") + _postfix);
            _minVector->readonly = true; _maxVector->readonly = true;
        }

        osg::Array* fetchArray()
        {
            osg::Array* array = _arrayObject.get();
            if (_fromProperty)
            {
                osg::Object* value = NULL;
                if (_entry.valid() && _object.valid())
                    _entry->getProperty(_object.get(), _property.name, value);
                array = dynamic_cast<osg::Array*>(value);
            }
            _arrayObject = array; return array;
        }

        virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            osg::Array* array = fetchArray();
            if (isDirty() || array != _panel.array.get()) refresh(array);
            _dirty = false;

            // Line 1: array name and the details button
            ImGui::TextUnformatted(TR(_title).c_str());
            ImGui::SameLine(0.0f, 10.0f);
            _button->readonly = (array == NULL);
            bool done = _button->show(mgr, content);

            if (array == NULL)
            {
                ImGui::TextDisabled("%s", TR("(not assigned)").c_str());
                return done;
            }

            // Line 2: data type and total number of elements
            _typeCombo->index = 0; _typeCombo->show(mgr, content);
            ImGui::SameLine(0.0f, 10.0f); _totalField->show(mgr, content);

            // Line 3/4: value range of the array data
            _minVector->show(mgr, content);
            _maxVector->show(mgr, content);
            return done;
        }

        void refresh(osg::Array* array)
        {
            _panel.refresh(array, _asColor);
            if (array == NULL) return;

            _typeCombo->items.assign(1, _panel.storageName);
            _typeCombo->index = 0;
            _totalField->value = (double)_panel.count;

            InputValueField::Type fieldType = InputValueField::FloatValue;
            if (_panel.kind == SignedIntValue) fieldType = InputValueField::IntValue;
            else if (_panel.kind == UnsignedIntValue) fieldType = InputValueField::UIntValue;

            const int components = (_panel.components > 0) ? _panel.components : 1;
            osg::Vec4d minVec = _panel.minValue, maxVec = _panel.maxValue;
            minVec[3] = (_panel.components > 3) ? minVec[3] : 0.0;
            maxVec[3] = (_panel.components > 3) ? maxVec[3] : 0.0;

            _minVector->type = fieldType; _minVector->vecNumber = components;
            _minVector->format = "%.4g"; _minVector->step = 0.0;
            _minVector->setVector(minVec);
            _maxVector->type = fieldType; _maxVector->vecNumber = components;
            _maxVector->format = "%.4g"; _maxVector->step = 0.0;
            _maxVector->setVector(maxVec);
        }

        osg::ref_ptr<LibraryEntry> _entry;
        LibraryEntry::Property _property;
        osg::observer_ptr<osg::Array> _arrayObject;
        osg::ref_ptr<Button> _button;
        osg::ref_ptr<ComboBox> _typeCombo;
        osg::ref_ptr<InputValueField> _totalField;
        osg::ref_ptr<InputVectorField> _minVector, _maxVector;
        ArrayPanel _panel;
        std::string _title;
        bool _fromProperty, _asColor;
    };

    /** A property holding a list of arrays, e.g. TexCoordArrayList */
    class ArrayListSerializerInterface : public SerializerBaseItem
    {
    public:
        ArrayListSerializerInterface(osg::Object* owner, LibraryEntry* entry,
                                     const LibraryEntry::Property& prop,
                                     const std::string& baseName, bool asColor)
        :   SerializerBaseItem(owner, false), _entry(entry), _property(prop),
            _baseName(baseName), _asColor(asColor) {}

        virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
        { return -1; }  // TODO

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            std::vector<osg::Object*> list;
            if (_entry.valid() && _object.valid())
                _entry->getProperty(_object.get(), _property.name, list);

            bool changed = (list.size() != _children.size());
            for (size_t i = 0; !changed && i < list.size(); ++i)
                if (_children[i]->getArrayObject() != dynamic_cast<osg::Array*>(list[i]))
                    changed = true;

            if (changed)
            {
                _children.clear();
                for (size_t i = 0; i < list.size(); ++i)
                {
                    osg::Array* array = dynamic_cast<osg::Array*>(list[i]);
                    if (array == NULL) continue;
                    _children.push_back(new ArraySerializerInterface(
                        array, _baseName + std::to_string(i), _asColor));
                }
            }

            if (_children.empty())
            { ImGui::TextDisabled("%s", TR("(no array)").c_str()); return false; }

            bool done = false;
            for (size_t i = 0; i < _children.size(); ++i)
                done |= _children[i]->show(mgr, content);
            return done;
        }

    protected:
        virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
        { return false; }  // all done in show()

        osg::ref_ptr<LibraryEntry> _entry;
        LibraryEntry::Property _property;
        std::string _baseName; bool _asColor;
        std::vector<osg::ref_ptr<ArraySerializerInterface>> _children;
    };
}

#define REGISTER_ARRAY_INTERFACE(PROP, TITLE, COLOR) \
    static osgVerse::SerializerBaseItem* createArrayInterface_##PROP( \
        osg::Object* obj, osgVerse::LibraryEntry* entry, \
        const osgVerse::LibraryEntry::Property& prop) \
    { return new ArraySerializerInterface(obj, entry, prop, TITLE, COLOR); } \
    extern "C" void serializerInterfaceFuncCaller_##PROP() {} \
    static osgVerse::SerializerInterfaceProxy proxy_##PROP( \
        #PROP, NULL, createArrayInterface_##PROP);

#define REGISTER_ARRAYLIST_INTERFACE(PROP, BASE, COLOR) \
    static osgVerse::SerializerBaseItem* createArrayInterface_##PROP( \
        osg::Object* obj, osgVerse::LibraryEntry* entry, \
        const osgVerse::LibraryEntry::Property& prop) \
    { return new ArrayListSerializerInterface(obj, entry, prop, BASE, COLOR); } \
    extern "C" void serializerInterfaceFuncCaller_##PROP() {} \
    static osgVerse::SerializerInterfaceProxy proxy_##PROP( \
        #PROP, NULL, createArrayInterface_##PROP);

REGISTER_ARRAY_INTERFACE(VertexArray, "Vertex", false)
REGISTER_ARRAY_INTERFACE(NormalArray, "Normal", false)
REGISTER_ARRAY_INTERFACE(ColorArray, "Color", true)
REGISTER_ARRAYLIST_INTERFACE(TexCoordArrayList, "TexCoord", false)
REGISTER_ARRAYLIST_INTERFACE(VertexAttribArrayList, "Array", false)
