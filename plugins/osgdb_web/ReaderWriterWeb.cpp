#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <libhv/all/client/requests.h>
#include <readerwriter/Utilities.h>

class ReaderWriterWeb : public osgDB::ReaderWriter
{
public:
    enum ObjectType { OBJECT, ARCHIVE, IMAGE, HEIGHTFIELD, NODE };
    
    ReaderWriterWeb()
    {
        _client = new hv::HttpClient;

        supportsProtocol("http", "Read from http port using libhv.");
        supportsProtocol("https", "Read from https port using libhv.");
        supportsProtocol("ftp", "Read from ftp port using libhv.");
        supportsProtocol("ftps", "Read from ftps port using libhv.");

        // Examples:
        // osgviewer --image https://www.baidu.com/img/PCtm_d9c8750bed0b3c7d089fa7d55720d6cf.png.verse_web
        // osgviewer --image ftp://ftp.techtrade.si/SLIKE/0002133.jpg.verse_web
        supportsExtension("verse_web", "Pseudo file extension, used to select libhv plugin.");
        supportsExtension("*", "Passes all read files to other plugins to handle actual model loading.");
    }

    virtual ~ReaderWriterWeb()
    {
        delete _client;
    }

    virtual const char* className() const
    { return "[osgVerse] Scene reader/writer from web protocols"; }

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
        case (NODE):
            {
                ReadResult rr = rw->readNode(fin, options);
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
                if (rr.validNode())
                {
                    osgVerse::FixedFunctionOptimizer ffo;
                    rr.getNode()->accept(ffo);
                }
#endif
                return rr;
            }
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
        if (osgDB::containsServerAddress(filename))
        {
            // TODO check remote existing state
            OSG_NOTICE << "[libhv] fileExists() not implemented." << std::endl;
        }
        return ReaderWriter::fileExists(filename, options);
    }

    virtual ReadResult readFile(ObjectType objectType, const std::string& fullFileName,
                                const osgDB::Options* options) const
    {
        std::string fileName(fullFileName);
        std::string ext = osgDB::getFileExtension(fullFileName);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        bool usePseudo = (ext == "verse_web");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getFileExtension(fileName);
        }

        if (!osgDB::containsServerAddress(fileName))
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
            OSG_WARN << "[libhv] No reader/writer plugin for " << fileName << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }

#ifdef __EMSCRIPTEN__
        osg::ref_ptr<osgVerse::WebFetcher> wf = new osgVerse::WebFetcher;
        bool succeed = wf->httpGet(osgDB::getServerFileName(fileName));
        if (!succeed)
        {
            OSG_WARN << "[emfetch] Failed getting " << fileName << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
        buffer.write((char*)&wf->buffer[0], wf->buffer.size());
#else
        // TODO: get connection parameters from options
        HttpRequest req;

        // Read data from web
        req.method = HTTP_GET;
        req.url = fileName;
        req.scheme = scheme;

        HttpResponse response;
        int result = _client->send(&req, &response);
        if (result != 0)
        {
            OSG_WARN << "[libhv] Failed getting " << fileName << ": " << result << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
        buffer.write((char*)response.body.data(), response.body.size());
#endif

        // Load by other readerwriter
        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->getDatabasePathList().push_front(osgDB::getFilePath(fileName));
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));
        lOptions->setPluginStringData("filename", fileName);

        // TODO: uncompress remote osgz/ivez/gz?
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
        bool usePseudo = (ext == "verse_web");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getFileExtension(fileName);
        }

        if (!osgDB::containsServerAddress(fileName))
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

        // TODO: get connection parameters from options
        HttpRequest req;

        // Post data to web
        req.method = HTTP_POST;
        req.url = fileName;
        req.scheme = scheme;
        req.body = std::string((std::istreambuf_iterator<char>(requestBuffer)),
                               std::istreambuf_iterator<char>());
        
        std::string connection, mimeType;
        if (options)
        {
            connection = options->getPluginStringData("Connection");
            mimeType = options->getPluginStringData("MimeType");
        }
        if (connection.empty()) connection = "keep-alive";
        if (mimeType.empty()) mimeType = "application/octet-stream";
        req.headers["Connection"] = connection;
        req.headers["Content-Type"] = mimeType;

        HttpResponse response; int code = _client->send(&req, &response);
        return (code != 0) ? WriteResult::ERROR_IN_WRITING_FILE : WriteResult::FILE_SAVED;
    }

protected:
    hv::HttpClient* _client;
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_web, ReaderWriterWeb)
