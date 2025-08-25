#include <osg/Version>
#include <osg/io_utils>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include "3rdparty/simdb.hpp"
#include "FileCache.h"
#include <iostream>
#include <sstream>
using namespace osgVerse;

FileCache::FileCache(const std::string& path, int blocks, int size)
    : osgDB::FileCache(path)
{ _dbObject = new simdb(path.c_str(), size, blocks); }

FileCache::~FileCache()
{
    if (_dbObject != NULL)
    { simdb* db = (simdb*)_dbObject; delete db; }
}

osgDB::ReaderWriter* FileCache::getReaderWriter(const std::string& fileName) const
{
    std::string ext = osgDB::getFileExtension(fileName);
    std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>>::
        const_iterator it = _cachedReaderWriters.find(ext);
    if (it != _cachedReaderWriters.end()) return const_cast<osgDB::ReaderWriter*>(it->second.get());

    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
    if (rw) const_cast<FileCache*>(this)->_cachedReaderWriters[ext] = rw; return rw;
}

bool FileCache::isFileAppropriateForFileCache(const std::string& originalFileName) const
{
    std::string protocol = osgDB::getServerProtocol(originalFileName);
    if (protocol.empty()) return false;
    else if (protocol.find("db") != std::string::npos) return false;
    return true;
}

bool FileCache::existsInCache(const std::string& fileName) const
{
    simdb* db = (simdb*)_dbObject;
    std::string keyName = createCacheFileName(fileName);

    long long len = db->len(keyName.data(), keyName.length());
    if (len > 0) return !isCachedFileBlackListed(fileName);
    return false;
}

std::string FileCache::createCacheFileName(const std::string& fileName) const
{ return fileName; }

#define READ_FUNCTOR(fileName, func) \
    simdb* db = (simdb*)_dbObject; osgDB::ReaderWriter* rw = getReaderWriter(fileName); \
    std::string key = createCacheFileName(fileName); long long len = db->len(key.data(), key.length()); \
    if (rw && len > 0) { \
        std::string value(len, '\0'); std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); \
        if (db->get(key.data(), key.length(), (void*)value.data(), value.length())) \
        { ss.write(value.data(), value.length()); return rw-> func (ss, options); } \
    } return NULL;

#define WRITE_FUNCTOR(fileName, func) \
    std::string key = createCacheFileName(fileName); osgDB::ReaderWriter* rw = getReaderWriter(fileName); if (rw) { \
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); simdb* db = (simdb*)_dbObject; \
        if (rw-> func (obj, ss, options).success()) { \
            std::string value((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>()); \
            if (db->put(key.data(), key.length(), (void*)value.data(), value.length())) removeFileFromBlackListed(fileName); \
            return FileCache::WriteResult::FILE_SAVED; } \
    } return FileCache::WriteResult::FILE_NOT_HANDLED;

FileCache::ReadResult FileCache::readObject(const std::string& fileName, const osgDB::Options* options) const
{ READ_FUNCTOR(fileName, readObject); }

FileCache::WriteResult FileCache::writeObject(const osg::Object& obj, const std::string& fileName,
                                              const osgDB::Options* options) const
{ WRITE_FUNCTOR(fileName, writeObject); }

FileCache::ReadResult FileCache::readImage(const std::string& fileName, const osgDB::Options* options) const
{ READ_FUNCTOR(fileName, readImage); }

FileCache::WriteResult FileCache::writeImage(const osg::Image& obj, const std::string& fileName,
                                             const osgDB::Options* options) const
{ WRITE_FUNCTOR(fileName, writeImage); }

FileCache::ReadResult FileCache::readNode(const std::string& fileName, const osgDB::Options* options,
                                          bool buildKdTreeIfRequired) const
{
    // FIXME: buildKdTreeIfRequired not used?
    READ_FUNCTOR(fileName, readNode);
}

FileCache::WriteResult FileCache::writeNode(const osg::Node& obj, const std::string& fileName,
                                            const osgDB::Options* options) const
{ WRITE_FUNCTOR(fileName, writeNode); }

FileCache::ReadResult FileCache::readShader(const std::string& fileName, const osgDB::Options* options) const
{ READ_FUNCTOR(fileName, readShader); }

FileCache::WriteResult FileCache::writeShader(const osg::Shader& obj, const std::string& fileName,
                                              const osgDB::Options* options) const
{ WRITE_FUNCTOR(fileName, writeShader); }

FileCache::ReadResult FileCache::readHeightField(const std::string& fileName, const osgDB::Options* options) const
{ READ_FUNCTOR(fileName, readHeightField); }

FileCache::WriteResult FileCache::writeHeightField(const osg::HeightField& obj, const std::string& fileName,
                                                   const osgDB::Options* options) const
{ WRITE_FUNCTOR(fileName, writeHeightField); }
