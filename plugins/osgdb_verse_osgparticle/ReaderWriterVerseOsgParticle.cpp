#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgParticle/ParticleSystemUpdater>
#include <ui/UserComponent.h>

class ParticleSystemUpdaterComponent : public osgVerse::UserComponent
{
public:
    class ParticleSystemUpdaterItem : public osgVerse::PropertyItem
    {
    public:
        ParticleSystemUpdaterItem() {}

        // Return true here if the editor should be refreshed when UI elemenet is activated
        virtual bool needRefreshUI() const { return false; }

        // Return the title shown at the property window
        virtual std::string title() const { return "OSG Particle System Updater"; }

        // Update the target object (ParticleSystemUpdater)
        virtual void updateTarget(osgVerse::ImGuiComponentBase* c = NULL) {}

        // Show UI elements, and return true if any of the elemenets is activated
        virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content)
        { return false; }
    };
    
    ParticleSystemUpdaterComponent(osg::Object* target = NULL, osg::Camera* cam = NULL)
    {
        ParticleSystemUpdaterItem* psu = new ParticleSystemUpdaterItem;
        psu->setTarget(target, osgVerse::PropertyItem::ComponentType);
        psu->setCamera(cam); setPropertyUI(psu);
    }

    ParticleSystemUpdaterComponent(const ParticleSystemUpdaterComponent& c, const osg::CopyOp& op)
        : osgVerse::UserComponent(c, op) {}
    META_Object(osgVerse, ParticleSystemUpdaterComponent);
};

class ReaderWriterVerseOsgParticle : public osgDB::ReaderWriter
{
public:
    ReaderWriterVerseOsgParticle()
    {
        supportsExtension("verse_osgparticle", "Verse wrapper of osgParticle library");
    }

    virtual const char* className() const
    {
        return "[osgVerse] osgParticle pseudo-loader / wrapper";
    }

    virtual ReadResult readObject(const std::string& name, const Options*) const
    {
        std::string optionName = osgDB::getStrippedName(name);
        // TODO: add custom option?

        osg::ref_ptr<osgVerse::UserComponentGroup> group = new osgVerse::UserComponentGroup;
        group->add("osgParticle::ParticleSystemUpdater", new ParticleSystemUpdaterComponent);
        return group.get();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_osgparticle, ReaderWriterVerseOsgParticle)
