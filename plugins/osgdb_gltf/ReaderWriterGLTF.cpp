#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ConvertUTF>
#include <readerwriter/LoadSceneGLTF.h>

class ReaderWriterGLTF : public osgDB::ReaderWriter
{
public:
    ReaderWriterGLTF()
    {
        supportsExtension("verse_gltf", "osgVerse pseudo-loader");
        supportsExtension("gltf", "GLTF ascii scene file");
        supportsExtension("glb", "GLTF binary scene file");
        supportsExtension("b3dm", "Cesium batch 3D model");
        supportsExtension("i3dm", "Cesium instanced 3D model");
        supportsExtension("cmpt", "Cesium cmposite tiles");
        supportsOption("Directory", "Setting the working directory");
        supportsOption("Mode", "Set to 'ascii/binary' to read specific GLTF data");
        supportsOption("DisabledPBR", "Use PBR materials or not");
    }

    virtual const char* className() const
    {
        return "[osgVerse] GLTF scene reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_gltf");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        osg::ref_ptr<osg::Node> group;
        int noPBR = atoi(options->getPluginStringData("DisabledPBR").c_str());

        if (ext == "cmpt")
            group = readCesiumFormatCmpt(fileName, osgDB::getFilePath(fileName));
        else if (ext == "glb" || ext == "b3dm" || ext == "i3dm")
            group = osgVerse::loadGltf(fileName, true, noPBR == 0).get();
        else
            group = osgVerse::loadGltf(fileName, false, noPBR == 0).get();
        if (!group) OSG_WARN << "[ReaderWriterGLTF] Failed to load " << fileName << std::endl;
        return group.get();
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string dir = "", mode; bool noPBR = false, isBinary = false;
        if (options)
        {
            std::string fileName = options->getPluginStringData("filename");
            if (!fileName.empty())
            {
                std::string ext = osgDB::getFileExtension(fileName);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != "gltf") isBinary = true;
            }

            noPBR = (atoi(options->getPluginStringData("DisabledPBR").c_str()) != 0);
            dir = options->getPluginStringData("Directory");
            mode = options->getPluginStringData("Mode");
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            if (mode == "binary") isBinary = true;
        }

        if (dir.empty() && options && !options->getDatabasePathList().empty())
            dir = options->getDatabasePathList().front();
        return osgVerse::loadGltf2(fin, dir, isBinary, !noPBR).get();
    }

protected:
    osg::Group* readCesiumFormatCmpt(const std::string& fileName, const std::string& dir) const
    {
        std::ifstream fin(fileName, std::ios::in | std::ios::binary);
        if (!fin) return NULL;

        unsigned char magic[4]; unsigned int version = 0, bytes = 0, tiles = 0;
        fin.read((char*)magic, sizeof(char) * 4); fin.read((char*)&version, sizeof(int));
        fin.read((char*)&bytes, sizeof(int)); fin.read((char*)&tiles, sizeof(int));
        if (magic[0] != 'c' || magic[1] != 'm' || magic[2] != 'p' || magic[3] != 't') return NULL;

        osg::ref_ptr<osg::Group> group = new osg::Group;
        for (unsigned int t = 0; t < tiles; ++t)
        {
            std::stringstream binaryData;
            fin.read((char*)magic, sizeof(char) * 4); binaryData.write((char*)magic, sizeof(char) * 4);
            fin.read((char*)&version, sizeof(int)); binaryData.write((char*)&version, sizeof(int));
            fin.read((char*)&bytes, sizeof(int)); binaryData.write((char*)&bytes, sizeof(int));

            std::vector<unsigned char> remain(bytes - sizeof(char) * 4 - sizeof(int) * 2);
            if (!remain.empty())
            {
                fin.read((char*)&remain[0], remain.size());
                binaryData.write((char*)&remain[0], remain.size());
            }

            if ((magic[0] == 'b' && magic[1] == '3' && magic[2] == 'd' && magic[3] == 'm') ||
                (magic[0] == 'i' && magic[1] == '3' && magic[2] == 'd' && magic[3] == 'm'))
            {
                osg::ref_ptr<osg::Node> child = osgVerse::loadGltf2(binaryData, dir, true);
                if (child.valid()) group->addChild(child.get());
            }
            else
            {
                OSG_NOTICE << "[ReaderWriterGLTF] Unknown format: " << std::string(magic, magic + 4)
                           << ", found in " << fileName << std::endl;
            }
        }
        group->setName(fileName); return group.release();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_gltf, ReaderWriterGLTF)
