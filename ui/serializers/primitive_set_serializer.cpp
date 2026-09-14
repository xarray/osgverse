#include "serializer_utils.h"
#include <osg/Geometry>
#include <osg/TemplatePrimitiveIndexFunctor>
#include <cstdio>
using namespace osgVerse;

namespace
{
    static const char* getModeName(GLenum mode)
    {
        switch (mode)
        {
        case GL_POINTS: return "POINTS";
        case GL_LINES: return "LINES";
        case GL_LINE_STRIP: return "LINE_STRIP";
        case GL_LINE_LOOP: return "LINE_LOOP";
        case GL_TRIANGLES: return "TRIANGLES";
        case GL_TRIANGLE_STRIP: return "TRIANGLE_STRIP";
        case GL_TRIANGLE_FAN: return "TRIANGLE_FAN";
        case GL_QUADS: return "QUADS";
        case GL_QUAD_STRIP: return "QUAD_STRIP";
        case GL_POLYGON: return "POLYGON";
        default: return "UNKNOWN";
        }
    }

    /** Collects the exact index groups of all primitives in a PrimitiveSet */
    struct PrimitiveCollector
    {
        std::vector<unsigned int> indices;
        std::vector<unsigned int> offsets;
        std::vector<unsigned char> sizes;

        void begin(int n) { offsets.push_back((unsigned int)indices.size()); sizes.push_back((unsigned char)n); }
        void operator()(unsigned int i0) { begin(1); indices.push_back(i0); }
        void operator()(unsigned int i0, unsigned int i1)
        { begin(2); indices.push_back(i0); indices.push_back(i1); }
        void operator()(unsigned int i0, unsigned int i1, unsigned int i2)
        { begin(3); indices.push_back(i0); indices.push_back(i1); indices.push_back(i2); }
        void operator()(unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3)
        {
            begin(4); indices.push_back(i0); indices.push_back(i1);
            indices.push_back(i2); indices.push_back(i3);
        }
    };

    /** A non-modal window showing primitives in groups and checking vertex attributes */
    struct PrimitiveSetWindow : public Window
    {
        struct Primitive
        {
            unsigned int offset; unsigned char size;
            Primitive() : offset(0), size(0) {}
        };

        osg::observer_ptr<osg::Geometry> geometry;
        std::vector<unsigned int> indices;
        std::vector<Primitive> primitives;
        std::vector<osg::ref_ptr<osg::Array>> attributeArrays;
        std::vector<std::string> attributeNames;
        int setIndex, maxPrimitiveSize, selectedVertex;
        bool toRebuild;

        PrimitiveSetWindow(const std::string& n)
            : Window(n), setIndex(-1), maxPrimitiveSize(1), selectedVertex(-1), toRebuild(true)
        {
            absolutePosSize = true; alpha = 0.97f;
            pos = osg::Vec2(200.0f, 130.0f); size = osg::Vec2(680.0f, 560.0f);
            flags = ImGuiWindowFlags_NoCollapse;
        }

        void collectAttributes()
        {
            attributeArrays.clear(); attributeNames.clear();
            osg::Geometry* geom = geometry.get(); if (geom == NULL) return;

            struct { const char* name; osg::Array* array; } basic[3] = {
                { "Position", geom->getVertexArray() },
                { "Normal", geom->getNormalArray() },
                { "Color", geom->getColorArray() } };
            for (int i = 0; i < 3; ++i)
            {
                if (basic[i].array == NULL) continue;
                attributeArrays.push_back(basic[i].array);
                attributeNames.push_back(basic[i].name);
            }

            const osg::Geometry::ArrayList& texCoords = geom->getTexCoordArrayList();
            for (size_t i = 0; i < texCoords.size(); ++i)
            {
                if (texCoords[i] == NULL) continue;
                attributeArrays.push_back(texCoords[i].get());
                attributeNames.push_back("TexCoord" + std::to_string(i));
            }

            const osg::Geometry::ArrayList& attribs = geom->getVertexAttribArrayList();
            for (size_t i = 0; i < attribs.size(); ++i)
            {
                if (attribs[i] == NULL) continue;
                attributeArrays.push_back(attribs[i].get());
                attributeNames.push_back("Attrib" + std::to_string(i));
            }
        }

        void rebuild()
        {
            indices.clear(); primitives.clear(); maxPrimitiveSize = 1;
            osg::Geometry* geom = geometry.get(); if (geom == NULL) return;

            const osg::Geometry::PrimitiveSetList& list = geom->getPrimitiveSetList();
            if (setIndex < 0 || setIndex >= (int)list.size()) return;
            osg::PrimitiveSet* ps = list[setIndex].get(); if (ps == NULL) return;

            osg::TemplatePrimitiveIndexFunctor<PrimitiveCollector> functor;
            ps->accept(static_cast<osg::PrimitiveIndexFunctor&>(functor));
            indices = functor.indices;
            for (size_t j = 0; j < functor.offsets.size(); ++j)
            {
                Primitive p; p.offset = functor.offsets[j]; p.size = functor.sizes[j];
                if (p.size > maxPrimitiveSize) maxPrimitiveSize = p.size;
                primitives.push_back(p);
            }
            collectAttributes();
            selectedVertex = -1;
        }

        void showVertexInspector()
        {
            ImGui::Separator();
            if (selectedVertex < 0)
            {
                ImGui::TextDisabled("%s", TR("Click an index in the table to inspect its vertex").c_str());
                return;
            }

            ImGui::Text("%s: %d", TR("Vertex").c_str(), selectedVertex);
            if (attributeArrays.empty())
            {
                ImGui::TextDisabled("%s", TR("(no vertex attribute available)").c_str());
                return;
            }

            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                                  | ImGuiTableFlags_SizingFixedFit;
            if (ImGui::BeginTable("##VertexAttributes", 2 + 4, flags))
            {
                ImGui::TableSetupColumn(TR("Attribute").c_str());
                for (int c = 0; c < 4; ++c)
                    ImGui::TableSetupColumn(("##c" + std::to_string(c)).c_str(),
                                            ImGuiTableColumnFlags_WidthFixed, 84.0f);
                for (size_t i = 0; i < attributeArrays.size(); ++i)
                {
                    osg::Array* array = attributeArrays[i].get();
                    if (array == NULL || (unsigned int)selectedVertex >= array->getNumElements()) continue;
                    double value[4] = { 0.0, 0.0, 0.0, 0.0 };
                    if (!readArrayElement(array, (unsigned int)selectedVertex, value, 4)) continue;

                    int components = getArrayComponents(array); if (components <= 0) components = 1;
                    DataValueKind kind = getArrayValueKind(array);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", attributeNames[i].c_str());
                    for (int c = 0; c < components; ++c)
                    {
                        ImGui::TableSetColumnIndex(1 + c);
                        ImGui::TextUnformatted(formatTableValue(value[c], kind, false).c_str());
                    }
                }
                ImGui::EndTable();
            }
        }

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            if (!isOpen) return false;
            if (!Window::show(mgr, content)) { showEnd(); return false; }

            osg::Geometry* geom = geometry.get();
            if (geom == NULL)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%s", TR("Geometry expired").c_str());
                showEnd(); return true;
            }

            if (toRebuild) { rebuild(); toRebuild = false; }

            const osg::Geometry::PrimitiveSetList& list = geom->getPrimitiveSetList();
            osg::PrimitiveSet* ps = (setIndex >= 0 && setIndex < (int)list.size())
                                  ? list[setIndex].get() : NULL;
            if (ps == NULL)
            {
                ImGui::TextDisabled("%s", TR("(empty primitive set)").c_str());
                showEnd(); return true;
            }

            ImGui::Text("%s: %s", TR("Mode").c_str(), getModeName(ps->getMode()));
            ImGui::SameLine(); ImGui::Text("| %s: %d", TR("Primitives").c_str(), (int)primitives.size());
            ImGui::SameLine(); ImGui::Text("| %s: %d", TR("Indices").c_str(), (int)indices.size());
            ImGui::SameLine(); if (ImGui::Button(TR("Refresh").c_str())) toRebuild = true;
            ImGui::Separator();

            if (primitives.empty())
            {
                ImGui::TextDisabled("%s", TR("(no expandable primitive)").c_str());
                showEnd(); return true;
            }

            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                                  | ImGuiTableFlags_ScrollY;
            float tableHeight = ImGui::GetContentRegionAvail().y * 0.55f;
            if (ImGui::BeginTable("##PrimitiveTable", 1 + maxPrimitiveSize, flags, ImVec2(0.0f, tableHeight)))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                for (int c = 0; c < maxPrimitiveSize; ++c)
                    ImGui::TableSetupColumn(("i" + std::to_string(c)).c_str());
                ImGui::TableHeadersRow();

                ImGuiListClipper clipper; clipper.Begin((int)primitives.size());
                while (clipper.Step())
                {
                    for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
                    {
                        const Primitive& p = primitives[r];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        char rowName[32]; snprintf(rowName, 32, "%d", r);
                        ImGui::TextUnformatted(rowName);

                        for (int c = 0; c < maxPrimitiveSize; ++c)
                        {
                            ImGui::TableSetColumnIndex(1 + c);
                            if (c >= (int)p.size) continue;
                            unsigned int index = (p.offset + c < indices.size()) ? indices[p.offset + c] : 0;
                            char cellName[48]; snprintf(cellName, 48, "%u##p%d_%d", index, r, c);
                            if (ImGui::Selectable(cellName, selectedVertex == (int)index))
                                selectedVertex = (int)index;
                        }
                    }
                }
                ImGui::EndTable();
            }

            showVertexInspector();
            showEnd(); return true;
        }
    };

    /** Serializer UI of a single osg::PrimitiveSet */
    class PrimitiveSetSerializerInterface : public SerializerBaseItem
    {
    public:
        PrimitiveSetSerializerInterface(osg::Geometry* geom, osg::PrimitiveSet* ps)
        :   SerializerBaseItem(ps, false), _geometry(geom), _ps(ps)
        {
            _button = new Button(TR("Show Details") + _postfix);
            //_button->tooltip = TR(std::string(ps->libraryName()) + "::" + ps->className());
            _button->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
            { openWindow(); };

            _first = createField(TR("First"));
            _count = createField(TR("Count"));
            _indices = createField(TR("Indices"));
            _rowFirst = createField(TR("First"));
            _rowLength = createField(TR("Length"));
        }

        osg::PrimitiveSet* getPrimitiveSet() const { return _ps.get(); }

        virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
        { return -1; }  // TODO

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            osg::PrimitiveSet* ps = _ps.get();
            std::string title = ps ? ps->className() : "(empty)";
            return showInternal(mgr, content, TR(title) + _postfix);
        }

    protected:
        osg::ref_ptr<InputField> createField(const std::string& name)
        {
            osg::ref_ptr<InputField> field = new InputField(name + _postfix);
            field->readonly = true; field->width = 90;
            return field;
        }

        void openWindow()
        {
            osg::Geometry* geom = _geometry.get(); osg::PrimitiveSet* ps = _ps.get();
            if (geom == NULL || ps == NULL) return;

            if (!_window)
                _window = new PrimitiveSetWindow(TR("Primitive Details")
                                                 + "###primitiveset" + _postfix.substr(2));
            _window->geometry = geom;
            _window->setIndex = -1;
            const osg::Geometry::PrimitiveSetList& list = geom->getPrimitiveSetList();
            for (size_t i = 0; i < list.size(); ++i)
                if (list[i].get() == ps) { _window->setIndex = (int)i; break; }
            _window->isOpen = true; _window->sizeApplied = false; _window->toRebuild = true;
            ImGuiComponentBase::registerFloatingWindow(_window.get());
        }

        virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            osg::PrimitiveSet* ps = _ps.get();
            if (ps == NULL) { ImGui::TextDisabled("%s", TR("(empty)").c_str()); return false; }

            // Line 1: primitive set type name
            ImGui::TextUnformatted(ps->className());
            ImGui::SameLine(0.0f, 8.0f);
            ImGui::TextDisabled("(%s)", getModeName(ps->getMode()));

            bool done = false;
            if (ps->getType() >= osg::PrimitiveSet::DrawArraysIndirectPrimitiveType)
            {
                // Indirect draw calls cannot be expanded into explicit index groups
                _indices->value = std::to_string(ps->getNumIndices());
                _indices->show(mgr, content);
                ImGui::SameLine(0.0f, 10.0f);
                ImGui::Text("%s: %u", TR("Primitives").c_str(), ps->getNumPrimitives());
                return false;  // no details table
            }

            if (osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(ps))
            {
                // Line 2: first vertex and vertex count
                _first->value = std::to_string(da->getFirst()); _first->show(mgr, content);
                ImGui::SameLine(0.0f, 10.0f);
                _count->value = std::to_string((int)da->getCount()); _count->show(mgr, content);
            }
            else if (osg::MultiDrawArrays* mda = dynamic_cast<osg::MultiDrawArrays*>(ps))
            {
                // Line 2+: first & count of each draw command
                const osg::MultiDrawArrays::Firsts& firsts = mda->getFirsts();
                const osg::MultiDrawArrays::Counts& counts = mda->getCounts();
                const size_t num = (firsts.size() < counts.size()) ? firsts.size() : counts.size();
                for (size_t i = 0; i < num; ++i)
                {
                    showRow((int)i, firsts[i], counts[i], mgr, content);
                }
            }
            else if (osg::DrawArrayLengths* dal = dynamic_cast<osg::DrawArrayLengths*>(ps))
            {
                // Line 2+: first & length of each array range
                int first = (int)dal->getFirst();
                _first->value = std::to_string(first); _first->show(mgr, content);
                for (size_t i = 0; i < dal->size(); ++i)
                {
                    showRow((int)i, first, (*dal)[i], mgr, content);
                    first += (*dal)[i];
                }
            }
            else if (osg::DrawElements* de = dynamic_cast<osg::DrawElements*>(ps))
            {
                // Line 2: total number of indices
                _indices->value = std::to_string(de->getNumIndices()); _indices->show(mgr, content);
            }
            else
            {
                _indices->value = std::to_string(ps->getNumIndices()); _indices->show(mgr, content);
                ImGui::SameLine(0.0f, 10.0f);
                ImGui::Text("%s: %u", TR("Primitives").c_str(), ps->getNumPrimitives());
            }

            ImGui::SameLine(0.0f, 10.0f);
            done |= _button->show(mgr, content);
            return done;
        }

        void showRow(int row, int first, int count, ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            // Row index is appended to keep each row's widget ID unique
            _rowFirst->name = TR("First") + std::to_string(row) + _postfix;
            _rowFirst->value = std::to_string(first); _rowFirst->show(mgr, content);
            ImGui::SameLine(0.0f, 10.0f);
            _rowLength->name = TR("Length") + std::to_string(row) + _postfix;
            _rowLength->value = std::to_string(count); _rowLength->show(mgr, content);
        }

        osg::observer_ptr<osg::Geometry> _geometry;
        osg::observer_ptr<osg::PrimitiveSet> _ps;
        osg::ref_ptr<InputField> _first, _count, _indices, _rowFirst, _rowLength;
        osg::ref_ptr<Button> _button;
        osg::ref_ptr<PrimitiveSetWindow> _window;
    };

    /** Serializer UI of osg::Geometry::PrimitiveSetList, holding one item per primitive set */
    class PrimitiveSetListSerializerInterface : public SerializerBaseItem
    {
    public:
        PrimitiveSetListSerializerInterface(osg::Object* obj, LibraryEntry* entry,
                                            const LibraryEntry::Property& prop)
        :   SerializerBaseItem(obj, false), _entry(entry), _property(prop) {}

        virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
        { return -1; }  // TODO

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            osg::ref_ptr<osg::Geometry> geom = dynamic_cast<osg::Geometry*>(_object.get());
            if (!geom.valid()) return false;

            const osg::Geometry::PrimitiveSetList& list = geom->getPrimitiveSetList();
            bool changed = (list.size() != _children.size());
            for (size_t i = 0; !changed && i < list.size(); ++i)
                if (_children[i]->getPrimitiveSet() != list[i].get()) changed = true;

            if (changed)
            {
                _children.clear();
                for (size_t i = 0; i < list.size(); ++i)
                    _children.push_back(new PrimitiveSetSerializerInterface(geom.get(), list[i].get()));
            }

            if (_children.empty())
            { ImGui::TextDisabled("%s", TR("(no primitive set)").c_str()); return false; }

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
        std::vector<osg::ref_ptr<PrimitiveSetSerializerInterface>> _children;
    };
}

static osgVerse::SerializerBaseItem* createPrimitiveSetListInterface(
    osg::Object* obj, osgVerse::LibraryEntry* entry, const osgVerse::LibraryEntry::Property& prop)
{ return new PrimitiveSetListSerializerInterface(obj, entry, prop); }

extern "C" void serializerInterfaceFuncCaller_PrimitiveSetList() {}
static osgVerse::SerializerInterfaceProxy proxy_PrimitiveSetList(
    "PrimitiveSetList", NULL, createPrimitiveSetListInterface);
