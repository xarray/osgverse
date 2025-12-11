#ifndef MANA_UI_IMGUI_HPP
#define MANA_UI_IMGUI_HPP

#include <osg/Texture2D>
#include <osgGA/GUIEventHandler>
#include <osgViewer/View>

typedef unsigned long long ImTextureID;
struct ImFont;
struct ImGuiContext;

namespace osgVerse
{
    class ImGuiManager;

    struct ImGuiContentHandler : public osg::Referenced
    {
        std::map<std::string, ImFont*> ImGuiFonts;
        std::map<std::string, ImTextureID> ImGuiTextures;
        ImGuiContext* context;

        virtual void runInternal(ImGuiManager* m) {}
    };

    class ImGuiManager : public osg::Referenced
    {
    public:
        ImGuiManager();
        ImGuiContentHandler* getContentHandler() { return _contentHandler; }
        osgGA::GUIEventHandler* getHandler() { return _imguiHandler.get(); }
        std::map<std::string, osg::ref_ptr<osg::Texture2D>>& getTextures() { return _textures; }

        void setChineseSimplifiedFont(const std::string& path) { _fontData = path; }
        const std::string& getChineseSimplifiedFont() { return _fontData; }

        void initialize(ImGuiContentHandler* cb, bool eventsFrom3D = false);
        void shutdown();  // FIXME: must have context... when to execute it?

        void addToView(osgViewer::View* view, osg::Camera* specCam = NULL);
        osg::Texture* addToTexture(osg::Group* parentOfRtt, int w, int h);

        /** Simulate mouse input in 3D mode: position in [0, 1] */
        void setMouseInput(const osg::Vec2& pos, int button = 0, float wheel = 0.0f);

        void setGuiTexture(const std::string& name, const std::string& file);
        void setGuiTexture(const std::string& name, osg::Texture2D* tex);
        void removeGuiTexture(const std::string& name);

    protected:
        virtual ~ImGuiManager();
        void initializeEventHandler2D();
        void initializeEventHandler3D();

        osg::ref_ptr<ImGuiContentHandler> _contentHandler;
        osg::ref_ptr<osgGA::GUIEventHandler> _imguiHandler;
        std::map<std::string, osg::ref_ptr<osg::Texture2D>> _textures;
        std::string _fontData;
    };
}

#endif
