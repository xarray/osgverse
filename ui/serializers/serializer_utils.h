#ifndef MANA_UI_SERIALIZERUTILS_HPP
#define MANA_UI_SERIALIZERUTILS_HPP

#include <osg/Array>
#include <osg/Uniform>
#include <osg/UniformBase>
#include <osg/Program>
#include <osg/Shader>
#include <osg/StateSet>
#include "../SerializerInterface.h"

namespace osgVerse
{
    enum DataValueKind { FloatValue = 0, SignedIntValue, UnsignedIntValue };

    /** Create a serializer UI item for a uniform (owned by a StateSet) */
    class SerializerBaseItem;
    SerializerBaseItem* createUniformSerializerItem(
        osg::UniformBase* uniform, osg::StateSet* parent, int value, int unit = -1);

    /** Read-only data source of a number table window */
    struct NumberTableData : public osg::Referenced
    {
        DataValueKind kind; int components;
        std::string elementName;
        std::vector<std::string> labels;

        NumberTableData(DataValueKind k = FloatValue, int c = 1, const std::string& n = "")
            : kind(k), components(c), elementName(n)
        {
            const char* defaultLabels[4] = { "x", "y", "z", "w" };
            for (int i = 0; i < 4; ++i) labels.push_back(defaultLabels[i]);
        }

        virtual unsigned int getNumRows() const = 0;
        virtual bool getRow(unsigned int index, double* value, int maxComponents) const = 0;
        virtual osg::Array* getSourceArray() const { return NULL; }
        virtual bool isValid() const { return true; }
    };

    /** Number table data implemented on an osg::Array instance */
    struct ArrayTableData : public NumberTableData
    {
        osg::observer_ptr<osg::Array> array;
        ArrayTableData(osg::Array* a);

        virtual unsigned int getNumRows() const;
        virtual bool getRow(unsigned int index, double* value, int maxComponents) const;
        virtual osg::Array* getSourceArray() const { return array.get(); }
        virtual bool isValid() const { return array.get() != NULL; }
    };

    /** Number table data implemented on a snapshot of copied values */
    struct SnapshotTableData : public NumberTableData
    {
        std::vector<osg::Vec4d> rows;
        SnapshotTableData(DataValueKind k, int c, const std::string& n)
            : NumberTableData(k, c, n) {}

        virtual unsigned int getNumRows() const { return (unsigned int)rows.size(); }
        virtual bool getRow(unsigned int index, double* value, int maxComponents) const;
    };

    /** Utilities for reading osg::Array data without knowing its concrete type */
    int getArrayComponents(const osg::Array* array);
    DataValueKind getArrayValueKind(const osg::Array* array);
    bool readArrayElement(const osg::Array* array, unsigned int index, double* value,
                          int maxComponents);
    bool computeArrayRange(const osg::Array* array, osg::Vec4d& minValue, osg::Vec4d& maxValue);
    std::string formatArrayStorageName(GLenum dataType, int components);
    std::string formatTableValue(double value, DataValueKind kind, bool asHex);

    /** A non-modal, read-only table window showing an array-like number set */
    struct NumberTableWindow : public Window
    {
        osg::ref_ptr<NumberTableData> data;
        bool showAsHex;
        unsigned int selectedRow;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        NumberTableWindow(const std::string& n)
            : Window(n), showAsHex(false), selectedRow(0)
        { absolutePosSize = true; alpha = 0.97f; }
    };
}

#endif
