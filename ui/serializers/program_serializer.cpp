#include <osg/Program>
#include <osg/Shader>
#include <osgDB/FileNameUtils>
#include "serializer_utils.h"
#include <cstdio>
using namespace osgVerse;

namespace
{
    static int countTextLines(const std::string& text)
    {
        if (text.empty()) return 0;
        int lines = 1;
        for (size_t i = 0; i < text.size(); ++i) { if (text[i] == '\n') ++lines; }
        return lines;
    }

    static int shaderTextResizeCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            std::string* text = static_cast<std::string*>(data->UserData);
            text->resize(data->BufTextLen);
            data->Buf = const_cast<char*>(text->c_str());
        }
        return 0;
    }

    /** A non-modal window showing and editing the shader sources of a Program */
    struct ShaderEditorWindow : public Window
    {
        osg::observer_ptr<osg::Program> program;
        osg::ref_ptr<ComboBox> shaderCombo;
        std::string buffer;
        int selectedShader;
        bool toReload;

        ShaderEditorWindow(const std::string& n)
            : Window(n), selectedShader(0), toReload(true)
        {
            absolutePosSize = true; alpha = 0.97f;
            pos = osg::Vec2(200.0f, 130.0f); size = osg::Vec2(760.0f, 560.0f);
            flags = ImGuiWindowFlags_NoCollapse;
            shaderCombo = new ComboBox("Shader##shadereditor");
            shaderCombo->width = 360;
        }

        std::string getShaderLabel(osg::Shader* shader, int id) const
        {
            if (shader == NULL) return std::to_string(id) + ": (empty)";
            std::string name = std::to_string(id) + ": " + shader->getTypename();
            const std::string& file = shader->getFileName();
            if (!file.empty()) name += " (" + osgDB::getSimpleFileName(file) + ")";
            return name;
        }

        osg::Shader* getCurrentShader() const
        {
            osg::Program* prog = program.get();
            if (prog == NULL || selectedShader < 0) return NULL;
            if (selectedShader >= (int)prog->getNumShaders()) return NULL;
            return prog->getShader((unsigned int)selectedShader);
        }

        void reload()
        {
            osg::Program* prog = program.get();
            buffer.clear(); shaderCombo->items.clear();
            if (prog == NULL) return;

            for (unsigned int i = 0; i < prog->getNumShaders(); ++i)
                shaderCombo->items.push_back(getShaderLabel(prog->getShader(i), (int)i));
            if (selectedShader >= (int)prog->getNumShaders()) selectedShader = 0;
            osg::Shader* shader = prog->getShader((unsigned int)selectedShader);
            if (shader != NULL) buffer = shader->getShaderSource();
        }

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            if (!isOpen) return false;
            if (!Window::show(mgr, content)) { showEnd(); return false; }

            osg::Program* prog = program.get();
            if (prog == NULL)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%s", TR("Program expired").c_str());
                showEnd(); return true;
            }

            if (toReload) { reload(); toReload = false; }
            if (prog->getNumShaders() == 0)
            {
                ImGui::TextDisabled("%s", TR("(no shader)").c_str());
                showEnd(); return true;
            }

            shaderCombo->index = selectedShader;
            shaderCombo->show(mgr, content);
            if (shaderCombo->index != selectedShader)
            {
                selectedShader = shaderCombo->index;
                osg::Shader* shader = getCurrentShader();
                if (shader != NULL) buffer = shader->getShaderSource();
            }
            ImGui::SameLine();
            if (ImGui::Button(TR("Reload").c_str()))
            {
                osg::Shader* shader = getCurrentShader();
                if (shader != NULL) buffer = shader->getShaderSource();
            }
            ImGui::SameLine(); ImGui::Text("| %s: %d", TR("Lines").c_str(), countTextLines(buffer));

            float buttonHeight = ImGui::GetFrameHeightWithSpacing();
            ImVec2 textSize(-FLT_MIN, ImGui::GetContentRegionAvail().y - buttonHeight - 4.0f);
            ImGui::InputTextMultiline("##ShaderSource", const_cast<char*>(buffer.c_str()),
                                      buffer.size() + 1, textSize,
                                      ImGuiInputTextFlags_AllowTabInput
                                      | ImGuiInputTextFlags_CallbackResize,
                                      shaderTextResizeCallback, &buffer);

            if (ImGui::Button(TR("Apply").c_str(), ImVec2(120.0f, 0.0f)))
            {
                osg::Shader* shader = getCurrentShader();
                if (shader != NULL) shader->setShaderSource(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button(TR("Revert").c_str(), ImVec2(120.0f, 0.0f)))
            {
                osg::Shader* shader = getCurrentShader();
                if (shader != NULL) buffer = shader->getShaderSource();
            }

            showEnd(); return true;
        }
    };

    class ProgramShadersSerializerInterface : public SerializerInterface
    {
    public:
        ProgramShadersSerializerInterface(osg::Object* obj, LibraryEntry* entry,
                                          const LibraryEntry::Property& prop)
        :   SerializerInterface(obj, entry, prop, false)
        {
            _button = new Button(TR("Edit Shaders") + _postfix);
            _button->tooltip = TR("Open the shader source editor");
            _button->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
            { openWindow(); };
        }

        virtual ItemType getType() const { return ListType; }

        virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
        {
            osg::ref_ptr<osg::Program> prog = dynamic_cast<osg::Program*>(_object.get());
            if (!prog.valid()) return false;

            const unsigned int numShaders = prog->getNumShaders();
            ImGui::Text("%s: %u", TR("Shaders").c_str(), numShaders);
            ImGui::SameLine(0.0f, 12.0f); _button->show(mgr, content);
            if (numShaders == 0)
            {
                ImGui::TextDisabled("%s", TR("(no shader)").c_str());
                return false;
            }

            int totalLines = 0;
            for (unsigned int i = 0; i < numShaders; ++i)
            {
                osg::Shader* shader = prog->getShader(i);
                if (shader) totalLines += countTextLines(shader->getShaderSource());
            }
            ImGui::Text("%s: %d", TR("Total lines").c_str(), totalLines);

            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                                  | ImGuiTableFlags_SizingFixedFit;
            if (ImGui::BeginTable(("##Shaders" + _postfix).c_str(), 4, flags))
            {
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn(TR("Type").c_str());
                ImGui::TableSetupColumn(TR("Lines").c_str());
                ImGui::TableSetupColumn(TR("File").c_str());
                for (unsigned int i = 0; i < numShaders; ++i)
                {
                    osg::Shader* shader = prog->getShader(i);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%u", i);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", shader ? shader->getTypename() : "(empty)");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%d", shader ? countTextLines(shader->getShaderSource()) : 0);
                    ImGui::TableSetColumnIndex(3);
                    std::string file = shader ? shader->getFileName() : "";
                    ImGui::Text("%s", file.empty() ? TR("(embedded)").c_str()
                                                   : osgDB::getSimpleFileName(file).c_str());
                }
                ImGui::EndTable();
            }
            return false;
        }

    protected:
        void openWindow()
        {
            osg::Program* prog = dynamic_cast<osg::Program*>(_object.get());
            if (prog == NULL) return;
            if (!_window)
                _window = new ShaderEditorWindow(TR("Shader Editor") + "###shadereditor" + _postfix.substr(2));
            _window->program = prog;
            _window->isOpen = true; _window->sizeApplied = false; _window->toReload = true;
            ImGuiComponentBase::registerFloatingWindow(_window.get());
        }

        osg::ref_ptr<Button> _button;
        osg::ref_ptr<ShaderEditorWindow> _window;
    };
}

static osgVerse::SerializerInterface* createProgramShadersInterface(
    osg::Object* obj, osgVerse::LibraryEntry* entry, const osgVerse::LibraryEntry::Property& prop)
{ return new ProgramShadersSerializerInterface(obj, entry, prop); }

extern "C" void serializerInterfaceFuncCaller_Shaders() {}
static osgVerse::SerializerInterfaceProxy proxy_Shaders(
    "Shaders", NULL, createProgramShadersInterface);
