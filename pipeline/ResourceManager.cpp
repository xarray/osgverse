#include "nanoid/nanoid.h"
#include "Utilities.h"
#include "ResourceManager.h"
using namespace osgVerse;

ResourceManager* ResourceManager::instance()
{
    static osg::ref_ptr<ResourceManager> s_instance = new ResourceManager;
    return s_instance.get();
}

ResourceManager::ResourceManager()
{ }

ResourceManager::~ResourceManager()
{ }

std::vector<osg::Image*> ResourceManager::getImages(const std::string& name) const
{
    std::vector<osg::Image*> result; TypeAndName t(IMAGE, name);
    std::map<TypeAndName, HashSet>::const_iterator it = _nameMap.find(t);
    if (it != _nameMap.end())
    {
        const HashSet& hashes = it->second;
        for (HashSet::const_iterator it2 = hashes.begin(); it2 != hashes.end(); ++it2)
        {
            std::map<unsigned long long, osg::ref_ptr<osg::Image>>::const_iterator itR = _images.find(*it2);
            if (itR != _images.end()) result.push_back(itR->second.get());
        }
    }
    return result;
}

std::vector<osg::BufferData*> ResourceManager::getBuffers(const std::string& name) const
{
    std::vector<osg::BufferData*> result; TypeAndName t(BUFFER, name);
    std::map<TypeAndName, HashSet>::const_iterator it = _nameMap.find(t);
    if (it != _nameMap.end())
    {
        const HashSet& hashes = it->second;
        for (HashSet::const_iterator it2 = hashes.begin(); it2 != hashes.end(); ++it2)
        {
            std::map<unsigned long long, osg::ref_ptr<osg::BufferData>>::const_iterator itR = _buffers.find(*it2);
            if (itR != _buffers.end()) result.push_back(itR->second.get());
        }
    }
    return result;
}

std::vector<osg::Shader*> ResourceManager::getShaders(const std::string& name) const
{
    std::vector<osg::Shader*> result; TypeAndName t(SHADER, name);
    std::map<TypeAndName, HashSet>::const_iterator it = _nameMap.find(t);
    if (it != _nameMap.end())
    {
        const HashSet& hashes = it->second;
        for (HashSet::const_iterator it2 = hashes.begin(); it2 != hashes.end(); ++it2)
        {
            std::map<unsigned long long, osg::ref_ptr<osg::Shader>>::const_iterator itR = _shaders.find(*it2);
            if (itR != _shaders.end()) result.push_back(itR->second.get());
        }
    }
    return result;
}

osg::Image* ResourceManager::shareImage(osg::Image* image, bool addIfNotShared)
{
    if (!image) return NULL; unsigned long long hash = Hash::getImage(*image);
    if (_images.find(hash) != _images.end()) return _images[hash].get();
    if (addIfNotShared)
    {
        _nameMap[TypeAndName(IMAGE, image->getFileName())].insert(hash);
        _images[hash] = image; return image;
    }
    return NULL;
}

osg::BufferData* ResourceManager::shareBuffer(osg::BufferData* buffer, bool addIfNotShared)
{
    if (!buffer) return NULL; unsigned long long hash = Hash::getBuffer(*buffer);
    if (_buffers.find(hash) != _buffers.end()) return _buffers[hash].get();
    if (addIfNotShared)
    {
        _nameMap[TypeAndName(BUFFER, buffer->getName())].insert(hash);
        _buffers[hash] = buffer; return buffer;
    }
    return NULL;
}

osg::Shader* ResourceManager::shareShader(osg::Shader* shader, bool addIfNotShared)
{
    if (!shader) return NULL; unsigned long long hash = Hash::getShader(*shader);
    if (_shaders.find(hash) != _shaders.end()) return _shaders[hash].get();
    if (addIfNotShared)
    {
        _nameMap[TypeAndName(SHADER, shader->getName())].insert(hash);
        _shaders[hash] = shader; return shader;
    }
    return NULL;
}
