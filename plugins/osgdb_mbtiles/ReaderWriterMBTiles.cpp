#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/Archive>
#include "3rdparty/sqlite3.h"

enum MbObjectType { OBJECT, ARCHIVE, IMAGE, HEIGHTFIELD, NODE, SHADER };
class MbArchive : public osgDB::Archive
{
public:
    MbArchive(const osgDB::ReaderWriter* rw, ArchiveStatus status,
              const std::string& dbName, const Options* options);
    virtual ~MbArchive() { close(); }

    virtual const char* libraryName() const { return "osgVerse"; }
    virtual const char* className() const { return "MbArchive"; }
    virtual bool acceptsExtension(const std::string& /*ext*/) const { return true; }

    virtual void close();
    virtual bool fileExists(const std::string& filename) const;
    virtual std::string getArchiveFileName() const { return _dbName; }
    virtual std::string getMasterFileName() const { return "mbtiles://" + _dbName + "/"; }

    virtual osgDB::FileType getFileType(const std::string& filename) const
    {
        if (fileExists(filename)) return osgDB::REGULAR_FILE;
        else return osgDB::FILE_NOT_FOUND;
    }

    virtual bool getFileNames(osgDB::DirectoryContents& fileNames) const { return false; }
    //virtual osgDB::DirectoryContents getDirectoryContents(const std::string& dirName) const;

    osgDB::ReaderWriter::ReadResult readFile(
        MbObjectType type, const std::string& f, const osgDB::Options* o) const;
    osgDB::ReaderWriter::WriteResult writeFile(const osg::Object& obj,
        MbObjectType type, const std::string& f, const osgDB::Options* o) const;

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
    sqlite3* _db; std::string _dbName;
};

class ReaderWriterMb : public osgDB::ReaderWriter
{
    friend class MbArchive;
public:
    ReaderWriterMb()
    {
        supportsProtocol("mbtiles", "Read from Sqlite database with mbtiles support.");
        supportsOption("Separator", "Separator of XYZ tile numbers. Default: '-' (x-y-z)");
        supportsOption("TileSetName", "Set tile set name");
        supportsOption("TileDataType", "Set tile set data type. Default: Image");
        supportsOption("TileSetDescription", "Set tile set description");
        supportsOption("TileDataFormat", "Set tile data format. Default: geotiff");
        supportsOption("PrintMetaData", "Print meta data or not. Default: 0");
        supportsOption("PrintTileList", "Print all tile names or not. Default: 0");

        // Examples:
        // - Writing: <ImgConv> image.jpg mbtiles://test.mbtiles/0-0-0.jpg
        // - Reading: osgviewer --image mbtiles://test.mbtiles/0-0-0.jpg
        supportsExtension("verse_mbtiles", "Pseudo file extension, used to select DB plugin.");
        supportsExtension("*", "Passes all read files to other plugins to handle actual model loading.");
    }

    virtual ~ReaderWriterMb()
    {
        for (std::map<std::string, sqlite3*>::iterator itr = _dbMap.begin();
             itr != _dbMap.end(); ++itr) { sqlite3_close(itr->second); }
    }
    
    bool acceptsProtocol(const std::string& protocol) const
    {
        std::string lowercase_protocol = osgDB::convertToLowerCase(protocol);
        return (_supportedProtocols.count(lowercase_protocol) != 0);
    }

    virtual const char* className() const
    { return "[osgVerse] Scene reader/writer from Sqlite database with mbtiles support"; }

    virtual ReadResult openArchive(const std::string& fullFileName, ArchiveStatus status,
                                   unsigned int, const Options* options) const
    {
        // Create archive from DB
        size_t protoEnd = fullFileName.find("//") + 1, addrEnd = fullFileName.find(".mbtiles") + 7;
        std::string dbName = fullFileName.substr(protoEnd + 1, addrEnd - protoEnd);
        return new MbArchive(this, status, dbName, options);
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

    ReadResult readFile(MbObjectType objectType, osgDB::ReaderWriter* rw,
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
        if (scheme == "mbtiles")
        {
            size_t protoEnd = filename.find("//") + 1, addrEnd = filename.find(".mbtiles") + 7;
            std::string dbName = filename.substr(protoEnd + 1, addrEnd - protoEnd);
            std::string keyName = osgDB::getStrippedName(filename.substr(addrEnd + 2));
            sqlite3* db = getOrCreateDatabase(dbName, options, false);
            if (!db) return false;

            std::stringstream buffer; std::string value; char sep = '-';
            if (options)
            {
                std::string sepValue = options->getPluginStringData("Separator");
                if (!sepValue.empty()) sep = sepValue[0];
            }

            std::vector<std::string> tileNums; osgDB::split(keyName, tileNums, sep);
            if (tileNums.size() < 3) return false;
            return findTile(db, tileNums[0], tileNums[1], tileNums[2]);
        }
        return ReaderWriter::fileExists(filename, options);
    }

    ReadResult readFile(MbObjectType objectType, const std::string& fullFileName,
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

        osgDB::ReaderWriter* reader = getReaderWriter(ext, true);
        if (!reader)
        {
            OSG_WARN << "[mbtiles] No reader/writer plugin for " << ext << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }

        // Read data from DB
        size_t protoEnd = fullFileName.find("//") + 1, addrEnd = fullFileName.find(".mbtiles") + 7;
        std::string dbName = fullFileName.substr(protoEnd + 1, addrEnd - protoEnd);
        std::string keyName = osgDB::getStrippedName(fullFileName.substr(addrEnd + 2));

        sqlite3* db = getOrCreateDatabase(dbName, options, false);
        if (!db) return ReadResult::ERROR_IN_READING_FILE;
        
        ReadResult result = read(db, fileName, keyName, objectType, reader, options);
        if (objectType == IMAGE && !result.validImage())
        {
            reader = getReaderWriter("verse_image", true);  // fallback reader
            if (reader) result = read(db, fileName, keyName, objectType, reader, options);
        }
        return result;
    }

    ReadResult read(sqlite3* db, const std::string& fileName, const std::string& keyName,
                    MbObjectType type, osgDB::ReaderWriter* rw, const osgDB::Options* options) const
    {
        std::stringstream buffer; std::string value; char sep = '-';
        if (options)
        {
            std::string sepValue = options->getPluginStringData("Separator");
            if (!sepValue.empty()) sep = sepValue[0];
        }

        std::vector<std::string> tileNums; osgDB::split(keyName, tileNums, sep);
        if (tileNums.size() < 3) return ReadResult::ERROR_IN_READING_FILE;
        else if (!readTile(db, tileNums[0], tileNums[1], tileNums[2], value)) return ReadResult::FILE_NOT_FOUND;
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
        if (scheme != "mbtiles")
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

        size_t protoEnd = fullFileName.find("//") + 1, addrEnd = fullFileName.find(".mbtiles") + 7;
        std::string dbName = fullFileName.substr(protoEnd + 1, addrEnd - protoEnd);
        std::string keyName = osgDB::getStrippedName(fullFileName.substr(addrEnd + 2));
        sqlite3* db = getOrCreateDatabase(dbName, options, true);
        if (!db) return WriteResult::ERROR_IN_WRITING_FILE;

        osgDB::ReaderWriter* writer = getReaderWriter(ext, true);
        if (!writer) return WriteResult::FILE_NOT_HANDLED;
        else return write(db, obj, keyName, writer, options);
    }

    WriteResult write(sqlite3* db, const osg::Object& obj, const std::string& keyName,
                      osgDB::ReaderWriter* rw, const osgDB::Options* options) const
    {
        std::stringstream requestBuffer;
        osgDB::ReaderWriter::WriteResult result = writeFile(obj, rw, requestBuffer, options);
        if (!result.success()) return result;

        char sep = '-';
        if (options)
        {
            std::string sepValue = options->getPluginStringData("Separator");
            if (!sepValue.empty()) sep = sepValue[0];
        }

        std::vector<std::string> tileNums; osgDB::split(keyName, tileNums, sep);
        if (tileNums.size() < 3) return WriteResult::ERROR_IN_WRITING_FILE;
        return writeTile(db, tileNums[0], tileNums[1], tileNums[2], requestBuffer.str());
    }

    sqlite3* getOrCreateDatabase(const std::string& name, const osgDB::Options* opt, bool createdIfMissing) const
    {
        sqlite3* db = NULL;
        DatabaseMap& dbMap = const_cast<DatabaseMap&>(_dbMap);
        if (dbMap.find(name) == dbMap.end())
        {
            // TODO: createdIfMissing?
            std::string realFileName = osgDB::findDataFile(name);
            if (!createdIfMissing && realFileName.empty()) return NULL;

            int rc = sqlite3_open(name.c_str(), &db);
            if (rc != SQLITE_OK) return NULL; else dbMap[name] = db;
            if (!realFileName.empty())
            {
                if (opt)
                {
                    std::string printedStr1 = opt->getPluginStringData("PrintMetaData");
                    if (printedStr1 == "true" || atoi(printedStr1.c_str()) > 0)
                    {
                        std::map<std::string, std::string> md = listMetaData(db);
                        for (std::map<std::string, std::string>::iterator i = md.begin(); i != md.end(); ++i)
                        { std::cout << "Mbtiles value: " << i->first << " = " << i->second << "\n"; }
                    }

                    std::string printedStr2 = opt->getPluginStringData("PrintTileList");
                    if (printedStr2 == "true" || atoi(printedStr2.c_str()) > 0)
                    {
                        std::vector<std::string> tiles = listTiles(db);
                        for (size_t i = 0; i < tiles.size(); ++i)
                            std::cout << "Mbtiles tile " << (i + 1) << "/" << tiles.size()
                                      << ": " << tiles[i] << "\n";
                    }
                }
                return db;  // no need to create initial tables
            }

            // Create necessary tables
            rc = sqlite3_exec(
                db, "CREATE TABLE metadata (name text, value text);", NULL, NULL, NULL);
            rc = sqlite3_exec(
                db, "CREATE TABLE tiles (zoom_level integer, tile_column integer, tile_row integer, "
                "tile_data blob);", NULL, NULL, NULL);

            // Fill metadata
            sqlite3_stmt* stmt = NULL;
            std::string tileName = name, type = "image", desc = "", format = "geotiff";
            if (opt)
            {
                tileName = opt->getPluginStringData("TileSetName");
                type = opt->getPluginStringData("TileDataType");
                desc = opt->getPluginStringData("TileSetDescription");
                format = opt->getPluginStringData("TileDataFormat");
            }

            rc = sqlite3_prepare_v2(
                db, "INSERT INTO metadata (name, value) VALUES (?1, ?2);", -1, &stmt, NULL);
            rc = sqlite3_bind_text(stmt, 1, "name", -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, tileName.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt); rc = sqlite3_reset(stmt);

            rc = sqlite3_bind_text(stmt, 1, "type", -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt); rc = sqlite3_reset(stmt);

            rc = sqlite3_bind_text(stmt, 1, "version", -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, "1.0.0", -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt); rc = sqlite3_reset(stmt);

            rc = sqlite3_bind_text(stmt, 1, "description", -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt); rc = sqlite3_reset(stmt);

            rc = sqlite3_bind_text(stmt, 1, "format", -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, format.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt); rc = sqlite3_finalize(stmt);
        }
        else
            db = dbMap[name];
        return db;
    }

    void closeDatabase(const std::string& name)
    {
        DatabaseMap::iterator itr = _dbMap.find(name);
        if (itr != _dbMap.end()) { sqlite3_close(itr->second); _dbMap.erase(itr); }
    }

    static bool findTile(sqlite3* db, const std::string& x,
                         const std::string& y, const std::string& z)
    {
        // FIXME?
        return true;
    }

    static bool readTile(sqlite3* db, const std::string& z, const std::string& x,
                         const std::string& y, std::string& value)
    {
        sqlite3_stmt* stmt = NULL;
        int rc = sqlite3_prepare(
            db, "SELECT tile_data FROM tiles WHERE "
            "zoom_level = ? AND tile_column = ? AND tile_row = ?;", -1, &stmt, 0);
        if (rc != SQLITE_OK) return false;

        rc += sqlite3_bind_int(stmt, 1, atoi(z.c_str()));
        rc += sqlite3_bind_int(stmt, 2, atoi(x.c_str()));
        rc += sqlite3_bind_int(stmt, 3, atoi(y.c_str()));
        if (rc != SQLITE_OK) return false; else rc = SQLITE_ROW;

        bool firstRow = true;
        while (rc == SQLITE_ROW)
        {
            rc = sqlite3_step(stmt);
            if (firstRow && rc == SQLITE_DONE)
            { sqlite3_finalize(stmt); return false; }

            if (rc == SQLITE_ROW)
            {
                int numBytes = sqlite3_column_bytes(stmt, 0);
                const void* blobRaw = sqlite3_column_blob(stmt, 0);
                value.insert(0, (const char*)blobRaw, numBytes);
                sqlite3_finalize(stmt); return true;
            }
            firstRow = false;
        }
        sqlite3_finalize(stmt); return false;
    }

    static WriteResult writeTile(sqlite3* db, const std::string& z, const std::string& x,
                                 const std::string& y, const std::string& value)
    {
        sqlite3_stmt* stmt = NULL;
        int rc = sqlite3_prepare_v2(
            db, "INSERT INTO tiles (zoom_level, tile_column, tile_row, tile_data) "
            "VALUES (?1, ?2, ?3, ?4);", -1, &stmt, NULL);
        if (rc != SQLITE_OK) return WriteResult::ERROR_IN_WRITING_FILE;

        rc = sqlite3_bind_int(stmt, 1, atoi(z.c_str()));
        rc = sqlite3_bind_int(stmt, 2, atoi(x.c_str()));
        rc = sqlite3_bind_int(stmt, 3, atoi(y.c_str()));
        rc = sqlite3_bind_blob(stmt, 4, value.c_str(), (int)value.size(), SQLITE_STATIC);
        rc = sqlite3_step(stmt); sqlite3_finalize(stmt);
        if (rc != SQLITE_OK) return WriteResult::ERROR_IN_WRITING_FILE;
        return WriteResult::FILE_SAVED;
    }

    static int listTilesCallback(void* rawPtr, int argc, char** argv, char** colName)
    {
        std::vector<std::string>* result = (std::vector<std::string>*)rawPtr;
        std::string sep("-"); if (argc < 3) return 0;
        if (argv[0] == NULL || argv[1] == NULL || argv[2] == NULL) return 0;
        result->push_back(argv[0] + sep + argv[1] + sep + argv[2]); return 0;
    }

    static std::vector<std::string> listTiles(sqlite3* db)
    {
        std::vector<std::string> result;
        int rc = sqlite3_exec(
            db, "SELECT zoom_level, tile_column, tile_row FROM tiles;",
            ReaderWriterMb::listTilesCallback, &result, NULL);
        return result;
    }

    static int listMetaDataCallback(void* rawPtr, int argc, char** argv, char** colName)
    {
        std::map<std::string, std::string>* result = (std::map<std::string, std::string>*)rawPtr;
        if (argc < 2) return 0; else if (argv[0] == NULL || argv[1] == NULL) return 0;
        (*result)[argv[0]] = argv[1]; return 0;
    }

    static std::map<std::string, std::string> listMetaData(sqlite3* db)
    {
        std::map<std::string, std::string> result;
        int rc = sqlite3_exec(
            db, "SELECT name, value FROM metadata;",
            ReaderWriterMb::listMetaDataCallback, &result, NULL);
        return result;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_mbtiles");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    osgDB::ReaderWriter* getReaderWriter(const std::string& extOrMime, bool isExt) const
    {
        if (extOrMime.empty()) return NULL;
        std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>>::const_iterator
            it = _cachedReaderWriters.find(extOrMime);
        if (it != _cachedReaderWriters.end()) return const_cast<osgDB::ReaderWriter*>(it->second.get());

        osgDB::ReaderWriter* rw = isExt ? osgDB::Registry::instance()->getReaderWriterForExtension(extOrMime)
                                        : osgDB::Registry::instance()->getReaderWriterForMimeType(extOrMime);
        if (rw) const_cast<ReaderWriterMb*>(this)->_cachedReaderWriters[extOrMime] = rw; return rw;
    }

    typedef std::map<std::string, sqlite3*> DatabaseMap; DatabaseMap _dbMap;
    std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>> _cachedReaderWriters;
};

MbArchive::MbArchive(const osgDB::ReaderWriter* rw, ArchiveStatus status,
                     const std::string& dbName, const Options* options)
    : _readerWriter(NULL), _dbName(dbName)
{
    ReaderWriterMb* rwdb = static_cast<ReaderWriterMb*>(const_cast<ReaderWriter*>(rw));
    if (!rwdb) { _db = NULL; return; } else _readerWriter = rwdb;
    _db = rwdb->getOrCreateDatabase(dbName, options, status == ArchiveStatus::CREATE);
}

void MbArchive::close()
{
    ReaderWriterMb* rwdb = static_cast<ReaderWriterMb*>(_readerWriter.get());
    if (rwdb) rwdb->closeDatabase(_dbName); _db = NULL; _readerWriter = NULL;
}

bool MbArchive::fileExists(const std::string& filename) const
{ if (!_db) return false; else return true; }  // FIXME

osgDB::ReaderWriter::ReadResult MbArchive::readFile(
    MbObjectType type, const std::string& fileName, const osgDB::Options* op) const
{
    std::string ext = osgDB::getFileExtension(fileName);
    osgDB::ReaderWriter* reader = static_cast<ReaderWriterMb*>(_readerWriter.get())->getReaderWriter(ext, true);
    if (!reader) return ReadResult::FILE_NOT_HANDLED;

    ReaderWriterMb* rwdb = static_cast<ReaderWriterMb*>(_readerWriter.get());
    if (!rwdb || !_db) return ReadResult::FILE_NOT_HANDLED;
    return rwdb->read(_db, getMasterFileName() + fileName, fileName, type, reader, op);
}

osgDB::ReaderWriter::WriteResult MbArchive::writeFile(const osg::Object& obj,
    MbObjectType type, const std::string& fileName, const osgDB::Options* op) const
{
    std::string ext = osgDB::getFileExtension(fileName);
    osgDB::ReaderWriter* writer = static_cast<ReaderWriterMb*>(_readerWriter.get())->getReaderWriter(ext, true);
    if (!writer) return WriteResult::FILE_NOT_HANDLED;

    ReaderWriterMb* rwdb = static_cast<ReaderWriterMb*>(_readerWriter.get());
    if (!rwdb || !_db) return WriteResult::FILE_NOT_HANDLED;
    return rwdb->write(_db, obj, fileName, writer, op);
}

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_mbtiles, ReaderWriterMb)
