#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/Archive>
#include "3rdparty/leveldb/db.h"

enum LevelDBObjectType { OBJECT, ARCHIVE, IMAGE, HEIGHTFIELD, NODE, SHADER };
class LevelDBArchive : public osgDB::Archive
{
public:
    LevelDBArchive(const osgDB::ReaderWriter* rw, ArchiveStatus status, const std::string& dbName);
    virtual ~LevelDBArchive() { close(); }

    virtual const char* libraryName() const { return "osgVerse"; }
    virtual const char* className() const { return "LevelDBArchive"; }
    virtual bool acceptsExtension(const std::string& /*ext*/) const { return true; }

    virtual void close();
    virtual bool fileExists(const std::string& filename) const;
    virtual std::string getArchiveFileName() const { return _dbName; }
    virtual std::string getMasterFileName() const { return "leveldb://" + _dbName + "/"; }

    virtual osgDB::FileType getFileType(const std::string& filename) const
    {
        if (fileExists(filename)) return osgDB::REGULAR_FILE;
        else return osgDB::FILE_NOT_FOUND;
    }

    virtual bool getFileNames(osgDB::DirectoryContents& fileNames) const { return false; }
    //virtual osgDB::DirectoryContents getDirectoryContents(const std::string& dirName) const;

    osgDB::ReaderWriter::ReadResult readFile(
        LevelDBObjectType type, const std::string& f, const osgDB::Options* o) const;
    osgDB::ReaderWriter::WriteResult writeFile(const osg::Object& obj,
        LevelDBObjectType type, const std::string& f, const osgDB::Options* o) const;

    virtual ReadResult readObject(
        const std::string& f, const osgDB::Options* o  = NULL) const { return readFile(OBJECT, f, o); }
    virtual ReadResult readImage(
        const std::string& f, const osgDB::Options* o  = NULL) const { return readFile(IMAGE, f, o); }
    virtual ReadResult readHeightField(
        const std::string& f, const osgDB::Options* o  = NULL) const { return readFile(HEIGHTFIELD, f, o); }
    virtual ReadResult readNode(
        const std::string& f, const osgDB::Options* o  = NULL) const { return readFile(NODE, f, o); }
    virtual ReadResult readShader(
        const std::string& f, const osgDB::Options* o  = NULL) const { return readFile(SHADER, f, o); }
    virtual WriteResult writeObject(const osg::Object& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return writeFile(obj, OBJECT, f, o); }
    virtual WriteResult writeImage(const osg::Image& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return writeFile(obj, IMAGE, f, o); }
    virtual WriteResult writeHeightField(const osg::HeightField& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return writeFile(obj, HEIGHTFIELD, f, o); }
    virtual WriteResult writeNode(const osg::Node& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return writeFile(obj, NODE, f, o); }
    virtual WriteResult writeShader(const osg::Shader& obj,
        const std::string& f, const osgDB::Options* o = NULL) const { return writeFile(obj, SHADER, f, o); }

protected:
    osg::observer_ptr<osgDB::ReaderWriter> _readerWriter;
    leveldb::DB* _db; std::string _dbName;
};

class ReaderWriterLevelDB : public osgDB::ReaderWriter
{
    friend class LevelDBArchive;
public:
    ReaderWriterLevelDB()
    {
        supportsProtocol("leveldb", "Read from LevelDB database.");
        supportsOption("WriteBufferSize=<s>", "Size in byte, default is 4Mb");

        // Examples:
        // - Writing: osgconv cessna.osg leveldb://test.db/cessna.osg.verse_leveldb
        // - Reading: osgviewer leveldb://test.db/cessna.osg.verse_leveldb
        supportsExtension("verse_leveldb", "Pseudo file extension, used to select DB plugin.");
        supportsExtension("*", "Passes all read files to other plugins to handle actual model loading.");
    }

    virtual ~ReaderWriterLevelDB()
    {
        for (std::map<std::string, leveldb::DB*>::iterator itr = _dbMap.begin();
             itr != _dbMap.end(); ++itr) { delete itr->second; }
    }
    
    bool acceptsProtocol(const std::string& protocol) const
    {
        std::string lowercase_protocol = osgDB::convertToLowerCase(protocol);
        return (_supportedProtocols.count(lowercase_protocol) != 0);
    }

    virtual const char* className() const
    { return "[osgVerse] Scene reader/writer from LevelDB database"; }

    virtual ReadResult openArchive(const std::string& fullFileName, ArchiveStatus status,
                                   unsigned int, const Options* options) const
    {
        // Create archive from DB
        std::string dbName = osgDB::getServerAddress(fullFileName);
        return new LevelDBArchive(this, status, dbName);
    }

    virtual ReadResult readObject(const std::string& fileName, const Options* options) const
    { return readFile(OBJECT, fileName, options); }

    virtual ReadResult readImage(const std::string& fileName, const Options* options) const
    { return readFile(IMAGE, fileName, options); }

    virtual ReadResult readHeightField(const std::string& fileName, const Options* options) const
    { return readFile(HEIGHTFIELD, fileName, options); }

    virtual ReadResult readNode(const std::string& fileName, const Options* options) const
    { return readFile(NODE, fileName, options); }

    virtual ReadResult readShader(const std::string& fileName, const Options* options) const
    { return readFile(SHADER, fileName, options); }

    virtual WriteResult writeObject(const osg::Object& obj, const std::string& fileName, const Options* options) const
    { return writeFile(obj, fileName, options); }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& fileName, const Options* options) const
    { return writeFile(image, fileName, options); }

    virtual WriteResult writeHeightField(const osg::HeightField& heightField, const std::string& fileName, const Options* options) const
    { return writeFile(heightField, fileName, options); }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& fileName, const Options* options) const
    { return writeFile(node, fileName, options); }
    
    virtual WriteResult writeShader(const osg::Shader& s, const std::string& fileName, const Options* options) const
    { return writeFile(s, fileName, options); }

    ReadResult readFile(LevelDBObjectType objectType, osgDB::ReaderWriter* rw,
                        std::istream& fin, const Options* options) const
    {
        switch (objectType)
        {
        case (OBJECT): return rw->readObject(fin, options);
        case (ARCHIVE): return rw->openArchive(fin, options);
        case (IMAGE): return rw->readImage(fin, options);
        case (HEIGHTFIELD): return rw->readHeightField(fin, options);
        case (SHADER): return rw->readShader(fin, options);
        case (NODE): return rw->readNode(fin, options);
        default: break;
        }
        return ReadResult::FILE_NOT_HANDLED;
    }

    WriteResult writeFile(const osg::Object& obj, osgDB::ReaderWriter* rw,
                          std::ostream& fout, const Options* options) const
    {
        const osg::HeightField* heightField = dynamic_cast<const osg::HeightField*>(&obj);
        if (heightField) return rw->writeHeightField(*heightField, fout, options);

        const osg::Node* node = dynamic_cast<const osg::Node*>(&obj);
        if (node) return rw->writeNode(*node, fout, options);

        const osg::Image* image = dynamic_cast<const osg::Image*>(&obj);
        if (image) return rw->writeImage(*image, fout, options);

        const osg::Shader* shader = dynamic_cast<const osg::Shader*>(&obj);
        if (shader) return rw->writeShader(*shader, fout, options);

        return rw->writeObject(obj, fout, options);
    }
    
    virtual bool fileExists(const std::string& filename, const osgDB::Options* options) const
    {
        std::string scheme = osgDB::getServerProtocol(filename);
        if (scheme == "leveldb")
        {
            std::string dbName = osgDB::getServerAddress(filename);
            std::string keyName = osgDB::getServerFileName(filename), value;
            leveldb::DB* db = getOrCreateDatabase(dbName, false);
            if (!db) return false;

            leveldb::Status status = db->Get(leveldb::ReadOptions(), keyName, &value);
            return status.ok();
        }
        return ReaderWriter::fileExists(filename, options);
    }

    ReadResult readFile(LevelDBObjectType objectType, const std::string& fullFileName,
                        const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        if (!acceptsProtocol(scheme))
        {
            if (options && !options->getDatabasePathList().empty())
            {
                if (osgDB::containsServerAddress(options->getDatabasePathList().front()))
                {
                    scheme = osgDB::getServerProtocol(options->getDatabasePathList().front());
                    if (acceptsProtocol(scheme))
                    {
                        std::string newFileName = options->getDatabasePathList().front() + "/" + fileName;
                        return readFile(objectType, newFileName, options);
                    }
                }
            }
            return ReadResult::FILE_NOT_HANDLED;
        }
        else if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        osgDB::ReaderWriter* reader = getReaderWriter(ext, true);
        if (!reader)
        {
            OSG_WARN << "[leveldb] No reader/writer plugin for " << fileName << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }

        // Read data from DB
        std::string dbName = osgDB::getServerAddress(fullFileName);
        std::string keyName = osgDB::getServerFileName(fullFileName);
        leveldb::DB* db = getOrCreateDatabase(dbName, false);
        if (!db) return ReadResult::ERROR_IN_READING_FILE;
        else return read(db, fileName, keyName, objectType, reader, options);
    }

    ReadResult read(leveldb::DB* db, const std::string& fileName, const std::string& keyName,
                    LevelDBObjectType type, osgDB::ReaderWriter* rw, const osgDB::Options* options) const
    {
        std::stringstream buffer; std::string value;
        leveldb::Status status = db->Get(leveldb::ReadOptions(), keyName, &value);
        if (!status.ok()) return ReadResult::FILE_NOT_FOUND;
        else buffer.write(value.data(), value.length());

        // Load by other readerwriter
        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->getDatabasePathList().push_front(osgDB::getFilePath(fileName));
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));
        lOptions->setPluginStringData("filename", fileName);

        ReadResult readResult = readFile(type, rw, buffer, lOptions.get());
        lOptions->getDatabasePathList().pop_front();
        return readResult;
    }

    virtual WriteResult writeFile(const osg::Object& obj, const std::string& fullFileName,
                                  const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        if (scheme != "leveldb")
        {
            if (options && !options->getDatabasePathList().empty())
            {
                if (osgDB::containsServerAddress(options->getDatabasePathList().front()))
                {
                    std::string newFileName = options->getDatabasePathList().front() + "/" + fileName;
                    return writeFile(obj, newFileName, options);
                }
            }
            return WriteResult::FILE_NOT_HANDLED;
        }
        else if (fileName.empty()) return WriteResult::FILE_NOT_HANDLED;

        std::string dbName = osgDB::getServerAddress(fullFileName);
        std::string keyName = osgDB::getServerFileName(fullFileName);
        leveldb::DB* db = getOrCreateDatabase(dbName, true);
        if (!db) return WriteResult::ERROR_IN_WRITING_FILE;

        osgDB::ReaderWriter* writer = getReaderWriter(ext, true);
        if (!writer) return WriteResult::FILE_NOT_HANDLED;
        else return write(db, obj, keyName, writer, options);
    }

    WriteResult write(leveldb::DB* db, const osg::Object& obj, const std::string& keyName,
                      osgDB::ReaderWriter* rw, const osgDB::Options* options) const
    {
        std::stringstream requestBuffer;
        osgDB::ReaderWriter::WriteResult result = writeFile(obj, rw, requestBuffer, options);
        if (!result.success()) return result;

        leveldb::Status status = db->Put(leveldb::WriteOptions(), keyName, requestBuffer.str());
        return status.ok() ? WriteResult::FILE_SAVED : WriteResult::FILE_NOT_HANDLED;
    }

    leveldb::DB* getOrCreateDatabase(const std::string& name, bool createdIfMissing) const
    {
        leveldb::DB* db = NULL;
        DatabaseMap& dbMap = const_cast<DatabaseMap&>(_dbMap);
        if (dbMap.find(name) == dbMap.end())
        {
            leveldb::Options options;
            options.create_if_missing = createdIfMissing;
            options.write_buffer_size = 256 * 1024 * 1024;

            leveldb::Status status = leveldb::DB::Open(options, name, &db);
            if (!status.ok()) return NULL; else dbMap[name] = db;
        }
        else
            db = dbMap[name];
        return db;
    }

    void closeDatabase(const std::string& name)
    {
        DatabaseMap::iterator itr = _dbMap.find(name);
        if (itr != _dbMap.end()) { delete itr->second; _dbMap.erase(itr); }
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_leveldb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    osgDB::ReaderWriter* getReaderWriter(const std::string& extOrMime, bool isExt) const
    {
        std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>>::const_iterator
            it = _cachedReaderWriters.find(extOrMime);
        if (it != _cachedReaderWriters.end()) return const_cast<osgDB::ReaderWriter*>(it->second.get());

        osgDB::ReaderWriter* rw = isExt ? osgDB::Registry::instance()->getReaderWriterForExtension(extOrMime)
            : osgDB::Registry::instance()->getReaderWriterForMimeType(extOrMime);
        if (rw) const_cast<ReaderWriterLevelDB*>(this)->_cachedReaderWriters[extOrMime] = rw; return rw;
    }

    typedef std::map<std::string, leveldb::DB*> DatabaseMap; DatabaseMap _dbMap;
    std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>> _cachedReaderWriters;
};

LevelDBArchive::LevelDBArchive(const osgDB::ReaderWriter* rw, ArchiveStatus status, const std::string& dbName)
    : _readerWriter(NULL), _dbName(dbName)
{
    ReaderWriterLevelDB* rwdb = static_cast<ReaderWriterLevelDB*>(const_cast<ReaderWriter*>(rw));
    if (!rwdb) { _db = NULL; return; } else _readerWriter = rwdb;
    _db = rwdb->getOrCreateDatabase(dbName, status == ArchiveStatus::CREATE);
}

void LevelDBArchive::close()
{
    ReaderWriterLevelDB* rwdb = static_cast<ReaderWriterLevelDB*>(_readerWriter.get());
    if (rwdb) rwdb->closeDatabase(_dbName); _db = NULL; _readerWriter = NULL;
}

bool LevelDBArchive::fileExists(const std::string& filename) const
{
    std::string value; if (!_db) return false;
    leveldb::Status status = _db->Get(leveldb::ReadOptions(), filename, &value);
    return status.ok();
}

osgDB::ReaderWriter::ReadResult LevelDBArchive::readFile(
    LevelDBObjectType type, const std::string& fileName, const osgDB::Options* op) const
{
    std::string ext = osgDB::getFileExtension(fileName);
    osgDB::ReaderWriter* reader = static_cast<ReaderWriterLevelDB*>(_readerWriter.get())->getReaderWriter(ext, true);
    if (!reader) return ReadResult::FILE_NOT_HANDLED;

    ReaderWriterLevelDB* rwdb = static_cast<ReaderWriterLevelDB*>(_readerWriter.get());
    if (!rwdb || !_db) return ReadResult::FILE_NOT_HANDLED;
    return rwdb->read(_db, getMasterFileName() + fileName, fileName, type, reader, op);
}

osgDB::ReaderWriter::WriteResult LevelDBArchive::writeFile(const osg::Object& obj,
    LevelDBObjectType type, const std::string& fileName, const osgDB::Options* op) const
{
    std::string ext = osgDB::getFileExtension(fileName);
    osgDB::ReaderWriter* writer = static_cast<ReaderWriterLevelDB*>(_readerWriter.get())->getReaderWriter(ext, true);
    if (!writer) return WriteResult::FILE_NOT_HANDLED;

    ReaderWriterLevelDB* rwdb = static_cast<ReaderWriterLevelDB*>(_readerWriter.get());
    if (!rwdb || !_db) return WriteResult::FILE_NOT_HANDLED;
    return rwdb->write(_db, obj, fileName, writer, op);
}

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_leveldb, ReaderWriterLevelDB)
