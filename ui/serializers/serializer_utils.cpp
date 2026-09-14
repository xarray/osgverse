#include "serializer_utils.h"
#include <cstdio>
using namespace osgVerse;

static int getDataTypeSize(GLenum dataType)
{
    switch (dataType)
    {
    case GL_BYTE: case GL_UNSIGNED_BYTE: return 1;
    case GL_SHORT: case GL_UNSIGNED_SHORT: return 2;
    case GL_INT: case GL_UNSIGNED_INT: case GL_FLOAT: return 4;
    case GL_DOUBLE: return 8;
    default: return 0;
    }
}

int osgVerse::getArrayComponents(const osg::Array* array)
{
    if (array == NULL) return 0;
    unsigned int elemSize = array->getElementSize();
    int dataSize = getDataTypeSize(array->getDataType());
    if (dataSize <= 0) return 0;
    int components = (int)(elemSize / (unsigned int)dataSize);
    return (components > 0 && components <= 4) ? components : 0;
}

DataValueKind osgVerse::getArrayValueKind(const osg::Array* array)
{
    if (array == NULL) return FloatValue;
    switch (array->getDataType())
    {
    case GL_BYTE: case GL_SHORT: case GL_INT:
        return SignedIntValue;
    case GL_UNSIGNED_BYTE: case GL_UNSIGNED_SHORT: case GL_UNSIGNED_INT:
        return UnsignedIntValue;
    default: return FloatValue;
    }
}

bool osgVerse::readArrayElement(const osg::Array* array, unsigned int index, double* value,
                                int maxComponents)
{
    if (array == NULL || index >= array->getNumElements()) return false;
    const GLvoid* ptr = array->getDataPointer(index);
    if (ptr == NULL) return false;

    int num = getArrayComponents(array);
    if (num <= 0) num = 1;
    if (num > maxComponents) num = maxComponents;
    for (int i = 0; i < num; ++i) value[i] = 0.0;

    switch (array->getDataType())
    {
    case GL_FLOAT:
        { const float* p = (const float*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    case GL_DOUBLE:
        { const double* p = (const double*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    case GL_BYTE:
        { const signed char* p = (const signed char*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    case GL_UNSIGNED_BYTE:
        { const unsigned char* p = (const unsigned char*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    case GL_SHORT:
        { const short* p = (const short*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    case GL_UNSIGNED_SHORT:
        { const unsigned short* p = (const unsigned short*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    case GL_INT:
        { const int* p = (const int*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    case GL_UNSIGNED_INT:
        { const unsigned int* p = (const unsigned int*)ptr; for (int i = 0; i < num; ++i) value[i] = p[i]; } break;
    default: return false;
    }
    return true;
}

bool osgVerse::computeArrayRange(const osg::Array* array, osg::Vec4d& minValue, osg::Vec4d& maxValue)
{
    minValue.set(0.0, 0.0, 0.0, 0.0); maxValue.set(0.0, 0.0, 0.0, 0.0);
    if (array == NULL || array->getNumElements() == 0) return false;

    int num = getArrayComponents(array); if (num <= 0) return false;
    double first[4] = {0.0, 0.0, 0.0, 0.0};
    if (!readArrayElement(array, 0, first, 4)) return false;
    for (int c = 0; c < num; ++c) { minValue[c] = first[c]; maxValue[c] = first[c]; }

    const unsigned int size = array->getNumElements();
    for (unsigned int i = 1; i < size; ++i)
    {
        double v[4] = {0.0, 0.0, 0.0, 0.0};
        if (!readArrayElement(array, i, v, 4)) continue;
        for (int c = 0; c < num; ++c)
        {
            if (v[c] < minValue[c]) minValue[c] = v[c];
            if (v[c] > maxValue[c]) maxValue[c] = v[c];
        }
    }
    return true;
}

std::string osgVerse::formatArrayStorageName(GLenum dataType, int components)
{
    std::string name, suffix;
    switch (dataType)
    {
    case GL_FLOAT: name = "float"; suffix = "f"; break;
    case GL_DOUBLE: name = "double"; suffix = "d"; break;
    case GL_BYTE: name = "char"; suffix = "b"; break;
    case GL_UNSIGNED_BYTE: name = "uchar"; suffix = "ub"; break;
    case GL_SHORT: name = "short"; suffix = "s"; break;
    case GL_UNSIGNED_SHORT: name = "ushort"; suffix = "us"; break;
    case GL_INT: name = "int"; suffix = "i"; break;
    case GL_UNSIGNED_INT: name = "uint"; suffix = "ui"; break;
    default: return "unknown";
    }

    if (components <= 1) return name;
    return "Vec" + std::to_string(components) + suffix;
}

std::string osgVerse::formatTableValue(double value, DataValueKind kind, bool asHex)
{
    char buffer[64];
    if (kind == FloatValue)
        snprintf(buffer, 64, "%.6g", value);
    else if (kind == UnsignedIntValue)
    {
        if (asHex) snprintf(buffer, 64, "0x%llX", (unsigned long long)value);
        else snprintf(buffer, 64, "%llu", (unsigned long long)value);
    }
    else
    {
        if (asHex) snprintf(buffer, 64, "0x%llX", (unsigned long long)(long long)value);
        else snprintf(buffer, 64, "%lld", (long long)value);
    }
    return std::string(buffer);
}

////////////// NumberTableData & NumberTableWindow //////////////

ArrayTableData::ArrayTableData(osg::Array* a)
{
    array = a;
    if (a != NULL)
    {
        components = getArrayComponents(a); if (components <= 0) components = 1;
        kind = getArrayValueKind(a);
        elementName = a->className();
    }
    else
    {
        components = 1; kind = FloatValue; elementName = "unknown";
    }
}

unsigned int ArrayTableData::getNumRows() const
{
    osg::Array* a = array.get();
    return (a != NULL) ? a->getNumElements() : 0;
}

bool ArrayTableData::getRow(unsigned int index, double* value, int maxComponents) const
{ return readArrayElement(array.get(), index, value, maxComponents); }

bool SnapshotTableData::getRow(unsigned int index, double* value, int maxComponents) const
{
    if (index >= rows.size()) return false;
    int num = (components < maxComponents) ? components : maxComponents;
    for (int i = 0; i < num; ++i) value[i] = rows[index][i];
    return true;
}

bool NumberTableWindow::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (!isOpen) return false;
    bool done = Window::show(mgr, content); if (!done) { showEnd(); return false; }

    if (!data.valid() || !data->isValid())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%s", TR("Data expired").c_str());
        showEnd(); return true;
    }

    unsigned int numRows = data->getNumRows();
    int components = (data->components > 0) ? data->components : 1;
    ImGui::Text("%s: %u", TR("Count").c_str(), numRows); ImGui::SameLine();
    ImGui::Text("| %s: %s", TR("Type").c_str(), data->elementName.c_str());
    if (data->kind != FloatValue)
    {
        ImGui::SameLine(); ImGui::Checkbox(TR("Hex").c_str(), &showAsHex);
    }
    if (numRows == 0)
    {
        ImGui::Separator(); ImGui::TextDisabled("%s", TR("(empty)").c_str());
        showEnd(); return true;
    }

    ImGui::Separator();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                          | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##NumberTable", 1 + components, flags, ImVec2(0.0f, avail.y)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        for (int c = 0; c < components; ++c)
            ImGui::TableSetupColumn(data->labels[c].c_str());
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper; clipper.Begin((int)numRows);
        while (clipper.Step())
        {
            for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
            {
                double value[4] = {0.0, 0.0, 0.0, 0.0};
                data->getRow((unsigned int)r, value, 4);
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                char rowName[32]; snprintf(rowName, 32, "%d", r);
                if (ImGui::Selectable(rowName, selectedRow == (unsigned int)r,
                                      ImGuiSelectableFlags_SpanAllColumns))
                    selectedRow = (unsigned int)r;

                for (int c = 0; c < components; ++c)
                {
                    ImGui::TableSetColumnIndex(1 + c);
                    std::string text = formatTableValue(value[c], data->kind, showAsHex);
                    ImGui::TextUnformatted(text.c_str());
                }
            }
        }
        ImGui::EndTable();
    }
    showEnd(); return true;
}
