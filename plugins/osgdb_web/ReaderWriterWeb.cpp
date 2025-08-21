#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <algorithm>

#include "3rdparty/libhv/all/client/requests.h"
#include <readerwriter/Utilities.h>
#ifdef WITH_ZLIB
#   include <zlib.h>
static size_t readGZip(const char* in, size_t in_size, char* out, size_t out_size)
{
    z_stream zs = {}; memset(&zs, 0, sizeof(zs));
    inflateInit2(&zs, 16 + MAX_WBITS);
    zs.next_in = (Bytef*)in;
    zs.avail_in = in_size;

    char buffer[16384];
    std::string result;
    do {
        zs.next_out = reinterpret_cast<Bytef*>(buffer);
        zs.avail_out = sizeof(buffer);
        inflate(&zs, Z_NO_FLUSH);
        result.append(buffer, sizeof(buffer) - zs.avail_out);
    } while (zs.avail_out == 0);
    inflateEnd(&zs);
    memcpy(out, result.data(), result.size());
    return result.size();
}
#else
static size_t readGZip(const char* in, size_t in_size, char* out, size_t out_size)
{ return 0; }
#endif

class ReaderWriterWeb : public osgDB::ReaderWriter
{
public:
    enum ObjectType { OBJECT, ARCHIVE, IMAGE, HEIGHTFIELD, NODE };
    
    ReaderWriterWeb()
    {
        //_client = new hv::HttpClient;

        supportsProtocol("http", "Read from http port using libhv.");
        supportsProtocol("https", "Read from https port using libhv.");
        supportsProtocol("ftp", "Read from ftp port using libhv.");
        supportsProtocol("ftps", "Read from ftps port using libhv.");

        // Examples:
        // osgviewer --image https://www.baidu.com/img/PCtm_d9c8750bed0b3c7d089fa7d55720d6cf.png.verse_web
        // osgviewer --image ftp://ftp.techtrade.si/SLIKE/0002133.jpg.verse_web
        supportsExtension("verse_web", "Pseudo file extension, used to select libhv plugin.");
        supportsExtension("*", "Passes all read files to other plugins to handle actual model loading.");
        supportsOption("Extension", "Set another pseudo extension for loaded file");
    }

    virtual ~ReaderWriterWeb()
    {
        //delete _client;
    }

    bool acceptsProtocol(const std::string& protocol) const
    {
        std::string lowercase_protocol = osgDB::convertToLowerCase(protocol);
        return (_supportedProtocols.count(lowercase_protocol) != 0);
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
        return ReadResult::NOT_IMPLEMENTED;
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
        std::string fileName(fullFileName), ext2;
        std::string ext = osgDB::getLowerCaseFileExtension(fullFileName);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
        bool usePseudo = (ext == "verse_web");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }
        else if (ext.find("verse_") != std::string::npos)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext2 = ext; ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        if (!acceptsProtocol(scheme))
        {
            if (options && !options->getDatabasePathList().empty())
            {
                scheme = osgDB::getServerProtocol(options->getDatabasePathList().front());
                if (acceptsProtocol(scheme))
                {
                    std::string newFileName = options->getDatabasePathList().front() + "/"
                                            + osgDB::getSimpleFileName(fileName);
                    return readFile(objectType, newFileName, options);
                }
            }
            return ReadResult::FILE_NOT_HANDLED;
        }

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));
        lOptions->setPluginStringData("simple_name", osgDB::getStrippedName(fileName));
        lOptions->setPluginStringData("prefix", osgDB::getFilePath(fileName));
        lOptions->setPluginStringData("filename", fileName);
        if (ext2 == "verse_tiles" && ext == "children")
        {
            osgDB::ReaderWriter* reader = getReaderWriter(ext2, true);
            if (reader) return reader->readNode(fileName, lOptions.get());
        }

        // TODO: get connection parameters from options
        std::string contentType = "image/jpeg", encoding = "";
        std::vector<unsigned char> content =
            osgVerse::loadFileData(fileName, contentType, encoding);
        if (content.empty()) return ReadResult::FILE_NOT_FOUND;

        std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
        buffer.write((char*)content.data(), content.size());

        size_t queryInExt = ext.find("?");  // remove query string if mixed with extension
        if (queryInExt != std::string::npos) ext = ext.substr(0, queryInExt);
        if (encoding.find("gzip") != std::string::npos)
        {
            size_t bufferSize = buffer.str().size();
            std::vector<char> inData(bufferSize), outData(bufferSize * 10);

            buffer.read((char*)&inData[0], bufferSize); buffer.str("");
            bufferSize = readGZip(inData.data(), bufferSize, outData.data(), outData.size());
            if (bufferSize > 0) buffer.write(outData.data(), bufferSize);
        }
        else if (!encoding.empty())
        {
            OSG_WARN << "[ReaderWriterWeb] Encoding method " << encoding << " not supported" << std::endl;
        }

        // Load by other readerwriter
        osgDB::ReaderWriter* reader = getReaderWriter(ext, true);
        lOptions->getDatabasePathList().push_front(osgDB::getFilePath(fileName));
        if (!reader && options)
        {
            if (ext2.empty()) ext2 = options->getPluginStringData("Extension");
            if (!ext2.empty()) reader = getReaderWriter(ext2, true);
        }

        if (!reader) reader = getReaderWriter(contentType, false);
        if (!reader)
        {
            OSG_WARN << "[ReaderWriterWeb] No reader/writer plugin for " << fileName
                     << " (content-type: " << contentType << ")" << std::endl;
            return ReadResult::FILE_NOT_HANDLED;
        }

        // TODO: uncompress remote osgz/ivez/gz?
        ReadResult readResult = readFile(objectType, reader, buffer, lOptions.get());
        lOptions->getDatabasePathList().pop_front();
        return readResult;
    }

    virtual WriteResult writeFile(const osg::Object& obj, const std::string& fullFileName,
                                  const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(fullFileName, ext);
        std::string scheme = osgDB::getServerProtocol(fullFileName);
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
        else if (fileName.empty()) return WriteResult::FILE_NOT_HANDLED;

        osgDB::ReaderWriter* writer = getReaderWriter(ext, true);
        if (!writer) return WriteResult::FILE_NOT_HANDLED;

        std::stringstream requestBuffer;
        osgDB::ReaderWriter::WriteResult result = writeFile(obj, writer, requestBuffer, options);
        if (!result.success()) return result;

        // TODO: get connection parameters from options
        HttpRequest req;
        hv::HttpClient* _client = new hv::HttpClient;

        // Post data to web
        req.method = HTTP_POST;
        req.url = osgVerse::normalizeUrl(fileName); req.scheme = scheme;
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

        HttpResponse response; int code = _client->send(&req, &response); delete _client;
        return (code != 0) ? WriteResult::ERROR_IN_WRITING_FILE : WriteResult::FILE_SAVED;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_web");
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
        if (rw) const_cast<ReaderWriterWeb*>(this)->_cachedReaderWriters[extOrMime] = rw; return rw;
    }

    std::map<std::string, osg::observer_ptr<osgDB::ReaderWriter>> _cachedReaderWriters;
    //hv::HttpClient* _client;
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_web, ReaderWriterWeb)
