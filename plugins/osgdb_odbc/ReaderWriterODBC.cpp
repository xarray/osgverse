#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ConvertUTF>
#include <osgDB/Registry>
#include <osgDB/Archive>
#include "readerwriter/Utilities.h"

#ifdef USE_ODBC_API
#   define OTL_ODBC
#   if defined(__linux__)
#      define OTL_ODBC_UNIX
#   endif
#   define OTL_ODBC_MULTI_MODE 1
#   define OTL_ODBC_POSTGRESQL 1
#   define OTL_ODBC_MYSQL 1
#   define OTL_DESTRUCTORS_DO_NOT_THROW 1
#   define OTL_EXCEPTION_ENABLE_ERROR_OFFSET 1
#   define OTL_EXTENDED_EXCEPTION 1
#   define OTL_TRACE_ENABLE_STREAM_LABELS 1
#   define OTL_UNCAUGHT_EXCEPTION_ON 1
#   define OTL_VALUE_TEMPLATE_ON 1
#   define OTL_DEFAULT_STRING_NULL_TO_VAL ""
#   define OTL_THROWS_EX_OUT_OF_RANGE
#   define OTL_UNICODE 1
#   define OTL_STL 1
#   include "3rdparty/otlv4.h"
#endif
#include "3rdparty/sqlite3.h"

struct DatabaseConnector
{
    DatabaseConnector(const std::string& name, const std::string& b, bool o = true)
        : backend(b), withOdbc(o), base(NULL)
    {
#ifdef USE_ODBC_API
        if (withOdbc) { otl_connect* connect = new otl_connect; base = connect; }
#else
        withOdbc = false; backend = "sqlite";
#endif
        if (!withOdbc) { sqlite3* connect = NULL; sqlite3_open(name.c_str(), &connect); base = connect; }
        if (backend.empty()) backend = withOdbc ? "mysql" : "sqlite";
    }

    ~DatabaseConnector()
    {
#ifdef USE_ODBC_API
        if (withOdbc) { otl_connect* connect = (otl_connect*)base; delete connect; }
#endif
        if (!withOdbc) { sqlite3* connect = (sqlite3*)base; sqlite3_close(connect); }
    }

    bool connected() const
    {
#ifdef USE_ODBC_API
        if (withOdbc)
        {
            otl_connect* connect = (otl_connect*)base;
            return connect->connected;
        }
#endif
        return base != NULL;
    }

    bool login(const std::string& logonStr)
    {
#ifdef USE_ODBC_API
        if (withOdbc)
        {
            otl_connect* connect = (otl_connect*)base;
            try
            {
                connect->rlogon(logonStr.c_str());
                connect->set_max_long_size(20 * 1024 * 1024);  // 20mb
                return true;
            }
            catch (otl_exception& ex)
            {
                OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << ex.stm_text << "' exception: "
                           << ex.msg << " (state = " << ex.sqlstate << ", var = " << ex.var_info << ")\n";
            }
        }
#endif
        if (!withOdbc) { return true; }  // Fallback to Sqlite, do nothing
        return false;
    }

    bool run(const std::string& cmd)
    {
#ifdef USE_ODBC_API
        if (withOdbc)
        {
            otl_connect* connect = (otl_connect*)base;
            try { if (!cmd.empty()) otl_cursor::direct_exec(*connect, cmd.c_str()); return true; }
            catch (otl_exception& ex)
            {
                OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << ex.stm_text << "' exception: "
                           << ex.msg << " (state = " << ex.sqlstate << ", var = " << ex.var_info << ")\n";
            }
        }
#endif
        if (!withOdbc)
        {
            sqlite3* connect = (sqlite3*)base;
            int rc = sqlite3_exec(connect, cmd.c_str(), NULL, NULL, NULL); if (rc == SQLITE_OK) return true;
            //else OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << cmd << "' exception: " << rc << "\n";
        }
        return false;
    }

    bool check(const std::string& cmd, const std::string& keyName)
    {
#ifdef USE_ODBC_API
        if (withOdbc)
        {
            otl_connect* connect = (otl_connect*)base;
            std::wstring keyW = osgDB::convertUTF8toUTF16(keyName);
            try
            {
                otl_long_unicode_string f0(keyW.data(), otl_short_int_max, (int)keyW.size());
                otl_stream in(1, cmd.c_str(), *connect); in << f0;
                int dummy = 0; if (in >> dummy) return true;
            }
            catch (otl_exception&) {}
        }
#endif
        if (!withOdbc)
        {
            sqlite3* connect = (sqlite3*)base; sqlite3_stmt* stmt = NULL;
            int rc = sqlite3_prepare_v2(connect, cmd.c_str(), -1, &stmt, NULL);
            rc = sqlite3_bind_text(stmt, 1, keyName.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(stmt); sqlite3_finalize(stmt);

            if (rc == SQLITE_ROW) return true;
            else if (rc == SQLITE_OK || rc == SQLITE_DONE) return false;
            else OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << cmd << "' exception: " << rc << "\n";
        }
        return false;
    }

    bool read(const std::string& cmd, const std::string& keyName, std::ostream& buffer)
    {
#ifdef USE_ODBC_API
        if (withOdbc)
        {
            otl_connect* connect = (otl_connect*)base;
            std::wstring keyW = osgDB::convertUTF8toUTF16(keyName);
            try
            {
                otl_long_unicode_string f0(keyW.data(), otl_short_int_max, (int)keyW.size());
                otl_stream in(1, cmd.c_str(), *connect); in << f0;
                if (!in.eof())
                {
                    int f1 = 0; in >> f1; if (f1 >= connect->get_max_long_size()) connect->set_max_long_size(f1 + 1);
                    otl_long_string f2(osg::maximum(otl_short_int_max, f1 + 1)); in >> f2;
                    if (f2.len() > 0) { buffer.write((char*)f2.v, f2.len()); return true; }
                }
            }
            catch (otl_exception& ex)
            {
                OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << ex.stm_text << "' exception: "
                           << ex.msg << " (state = " << ex.sqlstate << ", var = " << ex.var_info << ")\n";
            }
        }
#endif
        if (!withOdbc)
        {
            sqlite3* connect = (sqlite3*)base; sqlite3_stmt* stmt = NULL;
            int rc = sqlite3_prepare(connect, cmd.c_str(), -1, &stmt, 0);
            if (rc == SQLITE_OK || rc == SQLITE_DONE)
            {
                sqlite3_bind_text(stmt, 1, keyName.c_str(), -1, SQLITE_STATIC);
                bool firstRow = true; rc = SQLITE_ROW;
                while (rc == SQLITE_ROW)
                {
                    rc = sqlite3_step(stmt);
                    if (firstRow && rc == SQLITE_DONE)
                    { sqlite3_finalize(stmt); return false; }

                    if (rc == SQLITE_ROW)
                    {
                        int numBytes = sqlite3_column_int(stmt, 0);
                        //int numBytes = sqlite3_column_bytes(stmt, 1);
                        const void* blobRaw = sqlite3_column_blob(stmt, 1);
                        buffer.write((char*)blobRaw, numBytes);
                        sqlite3_finalize(stmt); return true;
                    }
                    firstRow = false;
                }
                sqlite3_finalize(stmt);
            }
            else OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << cmd << "' exception: " << rc << "\n";
        }
        return false;
    }

    bool write(const std::string& cmd, const std::string& keyName, const std::stringstream& inStream)
    {
        std::string buffer = inStream.str(); int bufferSize = (int)buffer.size();
#ifdef USE_ODBC_API
        if (withOdbc)
        {
            otl_connect* connect = (otl_connect*)base;
            std::wstring keyW = osgDB::convertUTF8toUTF16(keyName);
            try
            {
                if (bufferSize >= connect->get_max_long_size()) connect->set_max_long_size(bufferSize + 1);
                otl_long_unicode_string f1(keyW.data(), otl_short_int_max, (int)keyW.size());
                otl_long_string f2(buffer.data(), osg::maximum(otl_short_int_max, bufferSize + 1), bufferSize);
                otl_stream out(1, cmd.c_str(), *connect); out << f1 << (int)bufferSize << f2; return true;
            }
            catch (otl_exception& ex)
            {
                OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << ex.stm_text << "' exception: "
                           << ex.msg << " (state = " << ex.sqlstate << ", var = " << ex.var_info << ")\n";
            }
        }
#endif
        if (!withOdbc)
        {
            sqlite3* connect = (sqlite3*)base; sqlite3_stmt* stmt = NULL;
            int rc = sqlite3_prepare(connect, cmd.c_str(), -1, &stmt, 0);
            if (rc == SQLITE_OK || rc == SQLITE_DONE)
            {
                sqlite3_bind_text(stmt, 1, keyName.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, (int)bufferSize);
                sqlite3_bind_blob(stmt, 3, buffer.c_str(), bufferSize, SQLITE_STATIC);
                rc = sqlite3_step(stmt); sqlite3_finalize(stmt);
            }
            if (rc == SQLITE_OK || rc == SQLITE_DONE) return true;
            else OSG_NOTICE << "[ReaderWriterOdbc] SQL '" << cmd << "' exception: " << rc << "\n";
        }
        return false;
    }

    void* base; std::string backend;
    bool withOdbc;
};

enum OdbcObjectType { OBJECT, ARCHIVE, IMAGE, HEIGHTFIELD, NODE, SHADER };
class OdbcArchive : public osgDB::Archive
{
public:
    OdbcArchive(const osgDB::ReaderWriter* rw, ArchiveStatus status, const std::string& dbName);
    virtual ~OdbcArchive() { close(); }

    virtual const char* libraryName() const { return "osgVerse"; }
    virtual const char* className() const { return "OdbcArchive"; }
    virtual bool acceptsExtension(const std::string& /*ext*/) const { return true; }

    virtual void close();
    virtual bool fileExists(const std::string& filename) const;
    virtual std::string getArchiveFileName() const { return _dbName; }
    virtual std::string getMasterFileName() const { return "odbc://" + _dbName + "/"; }

    virtual osgDB::FileType getFileType(const std::string& filename) const
    {
        if (fileExists(filename)) return osgDB::REGULAR_FILE;
        else return osgDB::FILE_NOT_FOUND;
    }

    virtual bool getFileNames(osgDB::DirectoryContents& fileNames) const { return false; }
    //virtual osgDB::DirectoryContents getDirectoryContents(const std::string& dirName) const;

    osgDB::ReaderWriter::ReadResult readFile(
        OdbcObjectType type, const std::string& f, const osgDB::Options* o) const;
    osgDB::ReaderWriter::WriteResult writeFile(const osg::Object& obj,
        OdbcObjectType type, const std::string& f, const osgDB::Options* o) const;

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
    DatabaseConnector* _db; std::string _dbName;
};

class ReaderWriterOdbc : public osgDB::ReaderWriter
{
    friend class OdbcArchive;
public:
    ReaderWriterOdbc()
    {
#ifdef USE_ODBC_API
        otl_connect::otl_initialize();
#endif
        supportsProtocol("odbc", "Read from ODBC database.");
        supportsOption("Connector=<t>", "Database connector (odbc or sqlite)");
        supportsOption("Backend=<t>", "Database backend (default: mysql)");
        supportsOption("DefaultDatabase=<t>", "Working database, can be empty to ignore database selecting (USE DATABASE)");
        supportsOption("DefaultTable=<t>", "Working table name ('verse_table' if not set), will be created with SQL: "
                                           "CREATE TABLE verse_table (name TEXT, size INT, value LONGBLOB);");
        supportsOption("PathSeparator", "Set path separator used in database. Default is ignored");

        // Examples:
        // - Writing: osgconv cessna.osg odbc://usr:pwd@test.db/cessna.osg.verse_odbc
        // - Reading: osgviewer odbc://usr:pwd@test.db/cessna.osg.verse_odbc
        supportsExtension("verse_odbc", "Pseudo file extension, used to select ODBC plugin.");
        supportsExtension("*", "Passes all read files to other plugins to handle actual model loading.");
    }

    virtual ~ReaderWriterOdbc()
    {
        for (std::map<std::string, DatabaseConnector*>::iterator itr = _dbMap.begin();
             itr != _dbMap.end(); ++itr) { delete itr->second; }
    }
    
    bool acceptsProtocol(const std::string& protocol) const
    {
        std::string lowercase_protocol = osgDB::convertToLowerCase(protocol);
        return (_supportedProtocols.count(lowercase_protocol) != 0);
    }

    virtual const char* className() const
    { return "[osgVerse] Scene reader/writer from ODBC/OTL database"; }

    virtual ReadResult openArchive(const std::string& fullFileName, ArchiveStatus status,
                                   unsigned int, const Options* options) const
    {
        // Create archive from DB
        std::string dbName = osgDB::getServerAddress(fullFileName);
        return new OdbcArchive(this, status, dbName);
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

    ReadResult readFile(OdbcObjectType objectType, osgDB::ReaderWriter* rw,
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
    
    virtual bool fileExists(const std::string& fullFileName, const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        if (scheme == "odbc")
        {
            std::string dbName = osgDB::getServerAddress(fileName);
            std::string keyName = osgDB::getServerFileName(fileName);
            DatabaseConnector* db = getOrCreateDatabase(dbName, options);
            if (!db) return false; else return findKey(db, keyName, options);
        }
        return ReaderWriter::fileExists(fullFileName, options);
    }

    ReadResult readFile(OdbcObjectType objectType, const std::string& fullFileName,
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

        osgDB::ReaderWriter* reader = getReaderWriter(ext);
        if (!reader)
        {
            OSG_WARN << "[ReaderWriterOdbc] No reader/writer plugin for " << fileName << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }

        // Read data from DB
        std::string dbName = osgDB::getServerAddress(fileName);
        std::string keyName = osgDB::getServerFileName(fileName);
        DatabaseConnector* db = getOrCreateDatabase(dbName, options);
        if (!db) return ReadResult::ERROR_IN_READING_FILE;
        else return read(db, fileName, keyName, objectType, reader, options);
    }

    virtual WriteResult writeFile(const osg::Object& obj, const std::string& fullFileName,
                                  const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        if (scheme != "odbc")
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

        std::string dbName = osgDB::getServerAddress(fileName);
        std::string keyName = osgDB::getServerFileName(fileName);
        DatabaseConnector* db = getOrCreateDatabase(dbName, options);
        if (!db) return WriteResult::ERROR_IN_WRITING_FILE;

        osgDB::ReaderWriter* writer = getReaderWriter(ext);
        if (!writer) return WriteResult::FILE_NOT_HANDLED;
        else return write(db, obj, keyName, writer, options);
    }

    bool findKey(DatabaseConnector* db, const std::string& keyName, const osgDB::Options* options) const
    {
        std::string table = options ? options->getPluginStringData("DefaultTable") : "";
        std::string key, sep = options ? options->getPluginStringData("PathSeparator") : "";
        if (table.empty()) table = "verse_table";

        std::string cmd = "SELECT 1 FROM " + table;
        cmd += (db->backend == "mysql") ? " WHERE name = :name<varchar> LIMIT 1;"
                                        : " WHERE name = ? LIMIT 1;";
        key = sep.empty() ? keyName : osgVerse::normalizeUrl(keyName, sep);
        return db->check(cmd, key);
    }

    ReadResult read(DatabaseConnector* db, const std::string& fileName, const std::string& keyName,
                    OdbcObjectType type, osgDB::ReaderWriter* rw, const osgDB::Options* options) const
    {
        std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
        std::string table = options ? options->getPluginStringData("DefaultTable") : "";
        std::string key, sep = options ? options->getPluginStringData("PathSeparator") : "";
        if (table.empty()) table = "verse_table";

        std::string cmd = "SELECT size, value FROM " + table;
        cmd += (db->backend == "mysql") ? " WHERE name = :name<varchar>;" : " WHERE name = ?;";
        key = sep.empty() ? keyName : osgVerse::normalizeUrl(keyName, sep);
        if (!db->read(cmd, key, buffer))
        {
            OSG_WARN << "[ReaderWriterOdbc] Unable to read key: " << key << "\n";
            return ReadResult::FILE_NOT_FOUND;
        }

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

    WriteResult write(DatabaseConnector* db, const osg::Object& obj, const std::string& keyName,
                      osgDB::ReaderWriter* rw, const osgDB::Options* options) const
    {
        std::stringstream requestBuffer;
        osgDB::ReaderWriter::WriteResult result = writeFile(obj, rw, requestBuffer, options);
        if (!result.success()) return result;

        std::string table = options ? options->getPluginStringData("DefaultTable") : "";
        std::string key, sep = options ? options->getPluginStringData("PathSeparator") : "";
        if (table.empty()) table = "verse_table";

        std::string items = "(name, size, value) VALUES(:name<varchar>, :size<int>, :value<raw_long>)";
        std::string cmd = "REPLACE INTO " + table + items + ";";
        if (db->backend != "mysql")
        {
            items = "(name, size, value) VALUES(?, ?, ?)";
            cmd = "INSERT INTO " + table + items + " ON CONFLICT(name) "
                + "DO UPDATE SET size = EXCLUDED.size, value = EXCLUDED.value;";
        }

        key = sep.empty() ? keyName : osgVerse::normalizeUrl(keyName, sep);
        if (db->write(cmd, key, requestBuffer)) return WriteResult::FILE_SAVED;
        else { OSG_WARN << "[ReaderWriterOdbc] Unable to write key: " << key << "\n"; }
        return WriteResult::FILE_NOT_HANDLED;
    }

    DatabaseConnector* getOrCreateDatabase(const std::string& name, const osgDB::Options* opt) const
    {
        DatabaseConnector* db = NULL;
        DatabaseMap& dbMap = const_cast<DatabaseMap&>(_dbMap);
        if (dbMap.find(name) != dbMap.end()) db = dbMap[name];

        if (!db)
        {
            std::vector<std::string> kv0; osgDB::split(name, kv0, '@');
            std::string usr, pwd, dsn = kv0.back();
            if (kv0.size() > 1)
            {
                std::vector<std::string> kv1; osgDB::split(kv0.front(), kv1, ':');
                usr = kv1.front(); if (kv1.size() > 1) pwd = kv1.back();
            }

            std::string logonStr = "UID=" + usr + ";PWD=" + pwd + ";DSN=" + dsn + ";charset=utf8mb4";
            std::string dbName = opt ? opt->getPluginStringData("DefaultDatabase") : "";
            std::string table = opt ? opt->getPluginStringData("DefaultTable") : "";
            std::string backend = opt ? opt->getPluginStringData("Backend") : "";
            std::string connector = opt ? opt->getPluginStringData("Connector") : "";
            std::transform(backend.begin(), backend.end(), backend.begin(), ::tolower);
            if (table.empty()) table = "verse_table";

            std::string cmd0 = dbName.empty() ? "" : ("USE " + dbName + ";");
            std::string cmd1 = "SELECT 1 FROM " + table + " WHERE 1=0;";  // check if table exists
            std::string cmd2 = "CREATE TABLE " + table + "(name VARCHAR(255) PRIMARY KEY, size INT, value LONGBLOB);";
            if (!backend.empty() && backend != "mysql")
            {
                if (backend == "postgresql")
                    cmd2 = "CREATE TABLE " + table + "(name VARCHAR(255) PRIMARY KEY, size INT, value BYTEA);";
                else if (backend == "sqlserver")
                    cmd2 = "CREATE TABLE " + table + "(name VARCHAR(255) PRIMARY KEY, size INT, value VARBINARY(MAX));";
                else if (connector == "sqlite" || backend == "sqlite")
                    cmd2 = "CREATE TABLE " + table + "(name VARCHAR(255) PRIMARY KEY, size INT, value BLOB);";
            }

            db = new DatabaseConnector(name, backend, connector != "sqlite");
            OSG_NOTICE << "[ReaderWriterOdbc] Create database: " << name << ", backend = " << db->backend << "\n";
            if (!db->login(logonStr.c_str())) { delete db; return NULL; }
            if (!db->run(cmd0.c_str())) { delete db; return NULL; }
            if (!db->run(cmd1.c_str())) db->run(cmd2.c_str());
            if (!db->connected())
            {
                OSG_WARN << "[ReaderWriterOdbc] Failed to create " << name << "\n";
                delete db; return NULL;
            }
            dbMap[name] = db;
        }
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

        bool usePseudo = (ext == "verse_odbc");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    osgDB::ReaderWriter* getReaderWriter(const std::string& ext) const
    {
        if (ext.empty()) return NULL;
        std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>>::const_iterator
            it = _cachedReaderWriters.find(ext);
        if (it != _cachedReaderWriters.end()) return const_cast<osgDB::ReaderWriter*>(it->second.get());

        osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
        if (rw) const_cast<ReaderWriterOdbc*>(this)->_cachedReaderWriters[ext] = rw; return rw;
    }

    typedef std::map<std::string, DatabaseConnector*> DatabaseMap; DatabaseMap _dbMap;
    std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>> _cachedReaderWriters;
};

OdbcArchive::OdbcArchive(const osgDB::ReaderWriter* rw, ArchiveStatus status, const std::string& dbName)
    : _readerWriter(NULL), _db(NULL), _dbName(dbName)
{
    ReaderWriterOdbc* rwdb = static_cast<ReaderWriterOdbc*>(const_cast<ReaderWriter*>(rw));
    if (!rwdb) { _db = NULL; return; } else _readerWriter = rwdb;
}

void OdbcArchive::close()
{
    ReaderWriterOdbc* rwdb = static_cast<ReaderWriterOdbc*>(_readerWriter.get());
    if (rwdb) rwdb->closeDatabase(_dbName); _db = NULL; _readerWriter = NULL;
}

bool OdbcArchive::fileExists(const std::string& filename) const
{ if (!_db) return false; else return true; }  // FIXME

osgDB::ReaderWriter::ReadResult OdbcArchive::readFile(
    OdbcObjectType type, const std::string& fileName, const osgDB::Options* op) const
{
    std::string ext = osgDB::getFileExtension(fileName);
    osgDB::ReaderWriter* reader = static_cast<ReaderWriterOdbc*>(_readerWriter.get())->getReaderWriter(ext);
    if (!reader) return ReadResult::FILE_NOT_HANDLED;

    ReaderWriterOdbc* rwdb = static_cast<ReaderWriterOdbc*>(_readerWriter.get());
    if (rwdb && !_db) const_cast<OdbcArchive*>(this)->_db = rwdb->getOrCreateDatabase(_dbName, op);
    if (!rwdb || !_db) return ReadResult::FILE_NOT_HANDLED;
    return rwdb->read(_db, getMasterFileName() + fileName, fileName, type, reader, op);
}

osgDB::ReaderWriter::WriteResult OdbcArchive::writeFile(const osg::Object& obj,
    OdbcObjectType type, const std::string& fileName, const osgDB::Options* op) const
{
    std::string ext = osgDB::getFileExtension(fileName);
    osgDB::ReaderWriter* writer = static_cast<ReaderWriterOdbc*>(_readerWriter.get())->getReaderWriter(ext);
    if (!writer) return WriteResult::FILE_NOT_HANDLED;

    ReaderWriterOdbc* rwdb = static_cast<ReaderWriterOdbc*>(_readerWriter.get());
    if (rwdb && !_db) const_cast<OdbcArchive*>(this)->_db = rwdb->getOrCreateDatabase(_dbName, op);
    if (!rwdb || !_db) return WriteResult::FILE_NOT_HANDLED;
    return rwdb->write(_db, obj, fileName, writer, op);
}

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_odbc, ReaderWriterOdbc)
