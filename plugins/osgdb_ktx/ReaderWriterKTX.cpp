#include <osg/io_utils>
#include <osg/Version>
#include <osg/Image>
#include <osg/ImageSequence>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <readerwriter/LoadTextureKTX.h>

class ReaderWriterKTX : public osgDB::ReaderWriter
{
public:
    ReaderWriterKTX()
    {
        supportsExtension("verse_ktx", "osgVerse pseudo-loader");
        supportsExtension("ktx", "KTX texture file");
        supportsOption("SavingCubeMap", "Save KTX cubemap data: default=0");
        supportsOption("UseBASISU", "Save with BASISU encoder: default=0");
        supportsOption("UseUASTC", "Save with UASTC (1), or ETC1S (0): default=0");
        supportsOption("ThreadCount", "Number of threads used for compression: default=1");
        supportsOption("CompressLevel", "Encoding speed vs. quality tradeoff [0-5]: default=1");
        supportsOption("QualityLevel", "Compression quality [1,255]: default=128");
    }

    virtual const char* className() const
    {
        return "[osgVerse] KTX texture reader";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_ktx");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        std::vector<osg::ref_ptr<osg::Image>> images = osgVerse::loadKtx(fileName, options);
        if (images.size() > 1)
        {
            osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
            for (size_t i = 0; i < images.size(); ++i) seq->addImage(images[i]);
            return seq.get();
        }
        return images.empty() ? ReadResult::ERROR_IN_READING_FILE : ReadResult(images[0]);
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options = NULL) const
    {
        std::vector<osg::ref_ptr<osg::Image>> images = osgVerse::loadKtx2(fin, options);
        if (images.size() > 1)
        {
            osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
            for (size_t i = 0; i < images.size(); ++i) seq->addImage(images[i]);
            return seq.get();
        }
        return images.empty() ? ReadResult::ERROR_IN_READING_FILE : ReadResult(images[0]);
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_ktx");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        osg::Image* imagePtr = const_cast<osg::Image*>(&image);
        osg::ImageSequence* seq = dynamic_cast<osg::ImageSequence*>(imagePtr);
        std::vector<osg::Image*> imageList;
        if (seq)
        {
            bool useCubemap = false;
            if (options)
            {
                std::string scm = options->getPluginStringData("SavingCubeMap");
                std::transform(scm.begin(), scm.end(), scm.begin(), tolower);
                useCubemap = (scm == "true" || atoi(scm.c_str()) > 0);
            }
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
            for (size_t i = 0; i < seq->getNumImageData(); ++i)
#else
            for (size_t i = 0; i < seq->getNumImages(); ++i)
#endif
                imageList.push_back(seq->getImage(i));

            bool result = osgVerse::saveKtx(fileName, useCubemap, options, imageList);
            return result ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
        }
        else
            imageList.push_back(imagePtr);

        bool result = osgVerse::saveKtx(fileName, false, options, imageList);
        return result ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
    }

    virtual WriteResult writeImage(const osg::Image& image, std::ostream& fout,
                                   const Options* options) const
    {
        osg::Image* imagePtr = const_cast<osg::Image*>(&image);
        osg::ImageSequence* seq = dynamic_cast<osg::ImageSequence*>(imagePtr);
        std::vector<osg::Image*> imageList;
        if (seq)
        {
            bool useCubemap = false;
            if (options)
            {
                std::string scm = options->getPluginStringData("SavingCubeMap");
                std::transform(scm.begin(), scm.end(), scm.begin(), tolower);
                useCubemap = (scm == "true" || atoi(scm.c_str()) > 0);
            }
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
            for (size_t i = 0; i < seq->getNumImageData(); ++i)
#else
            for (size_t i = 0; i < seq->getNumImages(); ++i)
#endif
                imageList.push_back(seq->getImage(i));

            bool result = osgVerse::saveKtx2(fout, useCubemap, options, imageList);
            return result ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
        }
        else
            imageList.push_back(imagePtr);

        bool result = osgVerse::saveKtx2(fout, false, options, imageList);
        return result ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_ktx, ReaderWriterKTX)
