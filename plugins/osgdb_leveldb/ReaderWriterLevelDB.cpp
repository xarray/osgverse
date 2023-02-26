#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <leveldb/db.h>

class ReaderWriterLevelDB : public osgDB::ReaderWriter
{
public:
    enum ObjectType { OBJECT, ARCHIVE, IMAGE, HEIGHTFIELD, NODE };
    
    ReaderWriterLevelDB()
    {
        supportsProtocol("leveldb", "Read from LevelDB database.");

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

    virtual const char* className() const
    { return "[osgVerse] Scene reader/writer from LevelDB database"; }

    virtual ReadResult openArchive(const std::string& fileName, ArchiveStatus status,
                                   unsigned int, const Options* options) const
    {
        if (status != READ) return ReadResult(ReadResult::FILE_NOT_HANDLED);
        else return readFile(ARCHIVE, fileName, options);
    }

    virtual ReadResult readObject(const std::string& fileName, const Options* options) const
    { return readFile(OBJECT, fileName, options); }

    virtual ReadResult readImage(const std::string& fileName, const Options* options) const
    { return readFile(IMAGE, fileName, options); }

    virtual ReadResult readHeightField(const std::string& fileName, const Options* options) const
    { return readFile(HEIGHTFIELD, fileName, options); }

    virtual ReadResult readNode(const std::string& fileName, const Options* options) const
    { return readFile(NODE, fileName, options); }

    virtual WriteResult writeObject(const osg::Object& obj, const std::string& fileName, const Options* options) const
    { return writeFile(obj, fileName, options); }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& fileName, const Options* options) const
    { return writeFile(image, fileName, options); }

    virtual WriteResult writeHeightField(const osg::HeightField& heightField, const std::string& fileName, const Options* options) const
    { return writeFile(heightField, fileName, options); }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& fileName, const Options* options) const
    { return writeFile(node, fileName, options); }

    ReadResult readFile(ObjectType objectType, osgDB::ReaderWriter* rw,
                        std::istream& fin, const Options* options) const
    {
        switch (objectType)
        {
        case (OBJECT): return rw->readObject(fin, options);
        case (ARCHIVE): return rw->openArchive(fin, options);
        case (IMAGE): return rw->readImage(fin, options);
        case (HEIGHTFIELD): return rw->readHeightField(fin, options);
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

            leveldb::Status status = db->Get(leveldb::ReadOptions(), dbName, &value);
            return status.ok();
        }
        return ReaderWriter::fileExists(filename, options);
    }

    virtual ReadResult readFile(ObjectType objectType, const std::string& fullFileName,
                                const osgDB::Options* options) const
    {
        std::string fileName(fullFileName);
        std::string ext = osgDB::getFileExtension(fullFileName);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        bool usePseudo = (ext == "verse_leveldb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getFileExtension(fileName);
        }

        if (scheme != "leveldb")
        {
            if (options && !options->getDatabasePathList().empty())
            {
                if (osgDB::containsServerAddress(options->getDatabasePathList().front()))
                {
                    std::string newFileName = options->getDatabasePathList().front() + "/" + fileName;
                    return readFile(objectType, newFileName, options);
                }
            }
            if (!usePseudo) return ReadResult::FILE_NOT_HANDLED;
        }

        osgDB::ReaderWriter* reader =
            osgDB::Registry::instance()->getReaderWriterForExtension(ext);
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

        ReadResult readResult = readFile(objectType, reader, buffer, lOptions.get());
        lOptions->getDatabasePathList().pop_front();
        return readResult;
    }

    virtual WriteResult writeFile(const osg::Object& obj, const std::string& fullFileName,
                                  const osgDB::Options* options) const
    {
        std::string fileName(fullFileName);
        std::string ext = osgDB::getFileExtension(fullFileName);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        bool usePseudo = (ext == "verse_leveldb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getFileExtension(fileName);
        }

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

        osgDB::ReaderWriter* writer = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
        if (!writer) return WriteResult::FILE_NOT_HANDLED;

        std::stringstream requestBuffer;
        osgDB::ReaderWriter::WriteResult result = writeFile(obj, writer, requestBuffer, options);
        if (!result.success()) return result;

        // Create database if missing
        std::string dbName = osgDB::getServerAddress(fullFileName);
        std::string keyName = osgDB::getServerFileName(fullFileName);
        leveldb::DB* db = getOrCreateDatabase(dbName, true);
        if (!db) return WriteResult::ERROR_IN_WRITING_FILE;

        leveldb::Status status = db->Put(leveldb::WriteOptions(), keyName, requestBuffer.str());
        return status.ok() ? WriteResult::FILE_SAVED : WriteResult::FILE_NOT_HANDLED;
    }

protected:
    leveldb::DB* getOrCreateDatabase(const std::string& name, bool createdIfMissing) const
    {
        leveldb::DB* db = NULL;
        DatabaseMap& dbMap = const_cast<DatabaseMap&>(_dbMap);
        if (dbMap.find(name) == dbMap.end())
        {
            leveldb::Options options; options.create_if_missing = createdIfMissing;
            leveldb::Status status = leveldb::DB::Open(options, name, &db);
            if (!status.ok()) return NULL; else dbMap[name] = db;
        }
        else
            db = dbMap[name];
        return db;
    }

    typedef std::map<std::string, leveldb::DB*> DatabaseMap;
    DatabaseMap _dbMap;
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_leveldb, ReaderWriterLevelDB)
