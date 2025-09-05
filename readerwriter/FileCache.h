#ifndef MANA_READERWRITER_FILECACHE_HPP
#define MANA_READERWRITER_FILECACHE_HPP

#include <osg/Transform>
#include <osgDB/ReaderWriter>
#include <osgDB/FileCache>
#include <sstream>
#include <iostream>
#include "Export.h"

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT FileCache : public osgDB::FileCache
    {
    public:
        typedef osgDB::ReaderWriter::ReadResult ReadResult;
        typedef osgDB::ReaderWriter::WriteResult WriteResult;
        FileCache(const std::string& path, int blocks = 4096, int size = 1024);

        void setGlobalOptions(osgDB::Options* op) { _options = op; }
        osgDB::Options* getGlobalOptions() { return _options.get(); }

        virtual bool isFileAppropriateForFileCache(const std::string& fileName) const;
        virtual bool existsInCache(const std::string& fileName) const;
        virtual std::string createCacheFileName(const std::string& fileName) const;

        virtual ReadResult readImage(const std::string& fileName, const osgDB::Options* options) const;
        virtual WriteResult writeImage(const osg::Image& image, const std::string& fileName,
                                       const osgDB::Options* options) const;

        virtual ReadResult readObject(const std::string& fileName, const osgDB::Options* options) const;
        virtual WriteResult writeObject(const osg::Object& object, const std::string& fileName,
                                        const osgDB::Options* options) const;

        virtual ReadResult readHeightField(const std::string& fileName, const osgDB::Options* options) const;
        virtual WriteResult writeHeightField(const osg::HeightField& hf, const std::string& fileName,
                                                           const osgDB::Options* options) const;

        virtual ReadResult readNode(const std::string& fileName, const osgDB::Options* options,
                                    bool buildKdTreeIfRequired = true) const;
        virtual WriteResult writeNode(const osg::Node& node, const std::string& fileName,
                                      const osgDB::Options* options) const;

        virtual ReadResult readShader(const std::string& fileName, const osgDB::Options* options) const;
        virtual WriteResult writeShader(const osg::Shader& shader, const std::string& fileName,
                                        const osgDB::Options* options) const;

    protected:
        virtual ~FileCache();
        std::string createCacheFileName(const std::string& fileName, const osgDB::Options* op) const;
        osgDB::ReaderWriter* getReaderWriter(const std::string& fileName) const;

        std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>> _cachedReaderWriters;
        std::map<std::string, std::string> _mimeTypes;
        osg::ref_ptr<osgDB::Options> _options;
        void* _dbObject;
    };
}

#endif
