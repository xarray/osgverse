#ifndef MANA_PP_RESOURCE_MANAGER_HPP
#define MANA_PP_RESOURCE_MANAGER_HPP

#include <osg/Image>
#include <osg/Geometry>
#include <osg/Program>
#include <set>

namespace osgVerse
{

    /** Global resource data manager */
    class ResourceManager : public osg::Referenced
    {
    public:
        enum Type { UNDEFINED = 0, IMAGE, BUFFER, SHADER };
        static ResourceManager* instance();

        /** Check a new image and return the shared one if already exists in manager */
        osg::Image* shareImage(osg::Image* image, bool addIfNotShared);

        /** Check a new buffer data and return the shared one if already exists in manager */
        osg::BufferData* shareBuffer(osg::BufferData* buffer, bool addIfNotShared);

        /** Check a new shader and return the shared one if already exists in manager */
        osg::Shader* shareShader(osg::Shader* shader, bool addIfNotShared);

        std::map<unsigned long long, osg::ref_ptr<osg::Image>>& getImages() { return _images; }
        std::map<unsigned long long, osg::ref_ptr<osg::BufferData>>& getBuffers() { return _buffers; }
        std::map<unsigned long long, osg::ref_ptr<osg::Shader>>& getShaders() { return _shaders; }

        std::vector<osg::Image*> getImages(const std::string& name) const;
        std::vector<osg::BufferData*> getBuffers(const std::string& name) const;
        std::vector<osg::Shader*> getShaders(const std::string& name) const;

        typedef std::pair<Type, std::string> TypeAndName;
        typedef std::set<unsigned long long> HashSet;
        std::map<TypeAndName, HashSet>& getNameMap() { return _nameMap; }

    protected:
        ResourceManager();
        virtual ~ResourceManager();

        std::map<unsigned long long, osg::ref_ptr<osg::Image>> _images;
        std::map<unsigned long long, osg::ref_ptr<osg::BufferData>> _buffers;
        std::map<unsigned long long, osg::ref_ptr<osg::Shader>> _shaders;
        std::map<TypeAndName, HashSet> _nameMap;
    };
}

#endif
