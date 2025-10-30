#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ConvertUTF>

#include "readerwriter/LoadSceneGLTF.h"
#include "readerwriter/SaveSceneGLTF.h"
#include "3rdparty/picojson.h"

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
        supportsExtension("pnts", "Cesium point-cloud tiles");
        supportsOption("Directory", "Setting the working directory");
        supportsOption("Mode", "Set to 'ascii/binary' to read specific GLTF data");
        supportsOption("DisabledPBR", "Use PBR materials or not");
        supportsOption("ForcedPBR", "Force using PBR materials or not");
        supportsOption("UpAxis", "Set up axis to Y (0) or Z (1) (default = 0)");
    }

    virtual const char* className() const
    {
        return "[osgVerse] GLTF scene reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;
        int noPBR = options ? atoi(options->getPluginStringData("DisabledPBR").c_str()) : 0;
        int forcedPBR = options ? atoi(options->getPluginStringData("ForcedPBR").c_str()) : 0;
        int yUp = options ? atoi(options->getPluginStringData("UpAxis").c_str()) : 0;
        
        osg::ref_ptr<osg::Node> group;
        if (ext == "cmpt")
        {
            std::ifstream fin(fileName, std::ios::in | std::ios::binary);
            group = readCesiumFormatCmpt(fin, osgDB::getFilePath(fileName), yUp == 0);
        }
        else if (ext == "pnts")
        {
            std::ifstream fin(fileName, std::ios::in | std::ios::binary);
            group = readCesiumFormatPnts(fin, osgDB::getFilePath(fileName));
        }
        else if (ext == "glb" || ext == "b3dm" || ext == "i3dm")
            group = osgVerse::loadGltf(fileName, true, (noPBR == 1) ? 0 : (forcedPBR == 0 ? 1 : 2), yUp == 0).get();
        else
            group = osgVerse::loadGltf(fileName, false, (noPBR == 1) ? 0 : (forcedPBR == 0 ? 1 : 2), yUp == 0).get();
        if (!group) OSG_WARN << "[ReaderWriterGLTF] Failed to load " << fileName << std::endl;
        return group.get();
    }

    virtual ReadResult readNode(std::istream& fin, const osgDB::Options* options) const
    {
        std::string dir = "", mode; bool isBinary = false, yUp = true; int pbrMode = 1;
        if (options)
        {
            std::string fileName = options->getPluginStringData("filename");
            dir = options->getPluginStringData("Directory");
            if (dir.empty()) dir = options->getPluginStringData("prefix");

            yUp = (options ? atoi(options->getPluginStringData("UpAxis").c_str()) : 0) == 0;
            if (!fileName.empty())
            {
                std::string ext = osgDB::getFileExtension(fileName);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (dir.empty()) dir = osgDB::getFilePath(fileName);

                if (ext == "cmpt") return readCesiumFormatCmpt(fin, dir, yUp);
                else if (ext == "pnts") return readCesiumFormatPnts(fin, dir);
                else if (ext != "gltf") isBinary = true;
            }

            int noPBR = atoi(options->getPluginStringData("DisabledPBR").c_str());
            int forcedPBR = atoi(options->getPluginStringData("ForcedPBR").c_str());
            pbrMode = (noPBR == 1) ? 0 : (forcedPBR == 0 ? 1 : 2);
            mode = options->getPluginStringData("Mode");
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            if (mode == "binary") isBinary = true;
        }

        if (dir.empty() && options && !options->getDatabasePathList().empty())
            dir = options->getDatabasePathList().front();
        return osgVerse::loadGltf2(fin, dir, isBinary, pbrMode, yUp).get();
    }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& path, const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return WriteResult::FILE_NOT_HANDLED;

        bool success = false;
        if (ext == "glb") success = osgVerse::saveGltf(node, fileName, true);
        else success = osgVerse::saveGltf(node, fileName, false);
        return success ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
    }

    virtual WriteResult writeNode(const osg::Node& node, std::ostream& fout, const osgDB::Options* options) const
    {
        std::string dir = "", mode; bool isBinary = false;
        if (options)
        {
            dir = options->getPluginStringData("Directory");
            if (dir.empty()) dir = options->getPluginStringData("prefix");

            mode = options->getPluginStringData("Mode");
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            if (mode == "binary") isBinary = true;
        }

        if (dir.empty() && options && !options->getDatabasePathList().empty())
            dir = options->getDatabasePathList().front();
        bool success = osgVerse::saveGltf2(node, fout, dir, isBinary);
        return success ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_gltf");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    osg::Group* readCesiumFormatCmpt(std::istream& fin, const std::string& dir, bool yUp) const
    {
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
                osg::ref_ptr<osg::Node> child = osgVerse::loadGltf2(binaryData, dir, true, yUp);
                if (child.valid()) group->addChild(child.get());
            }
            else
            {
                OSG_NOTICE << "[ReaderWriterGLTF] Unknown format: " << std::string(magic, magic + 4)
                           << ", found in " << dir << std::endl;
            }
        }
        return group.release();
    }

    osg::Node* readCesiumFormatPnts(std::istream& fin, const std::string& dir) const
    {
        std::istreambuf_iterator<char> eos;
        std::vector<char> data(std::istreambuf_iterator<char>(fin), eos);
        if (data.empty()) return NULL;

        unsigned int magic = *reinterpret_cast<unsigned int*>(&data[0]);
        unsigned int version = *reinterpret_cast<unsigned int*>(&data[4]);
        unsigned int byteLength = *reinterpret_cast<unsigned int*>(&data[8]);
        unsigned int featureTableJsonByteLength = *reinterpret_cast<unsigned int*>(&data[12]);
        unsigned int featureTableBinaryByteLength = *reinterpret_cast<unsigned int*>(&data[16]);

        std::string json(data.begin() + 28, data.begin() + 28 + featureTableJsonByteLength);
        picojson::value featureTable; std::string err = picojson::parse(featureTable, json);
        if (err.empty())
        {
            int byteOffset = 0, numPoints = featureTable.contains("POINTS_LENGTH")
                                          ? (int)featureTable.get("POINTS_LENGTH").get<double>() : 0;
            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(numPoints);
            osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(numPoints);
            osg::ref_ptr<osg::Vec4ubArray> ca = new osg::Vec4ubArray(numPoints);
            if (numPoints < 2) return NULL;  // FIXME: to make LOD continue traversing

            if (featureTable.contains("POSITION"))
            {
                byteOffset = (int)featureTable.get("POSITION").get("byteOffset").get<double>();
                for (int i = 0; i < numPoints; ++i) (*va)[i].set(
                    *reinterpret_cast<float*>(&data[byteOffset + i * 12 + 0]),
                    *reinterpret_cast<float*>(&data[byteOffset + i * 12 + 4]),
                    *reinterpret_cast<float*>(&data[byteOffset + i * 12 + 8]));
            }
            else if (featureTable.contains("POSITION_QUANTIZED"))
            {
                osg::Vec3 offset, scale(1.0f, 1.0f, 1.0f);
                picojson::array of = featureTable.get("QUANTIZED_VOLUME_OFFSET").get<picojson::array>();
                picojson::array sc = featureTable.get("QUANTIZED_VOLUME_SCALE").get<picojson::array>();
                if (of.size() == 3) offset.set(of[0].get<double>(), of[1].get<double>(), of[2].get<double>());
                if (sc.size() == 3) scale.set(sc[0].get<double>(), sc[1].get<double>(), sc[2].get<double>());

                byteOffset = (int)featureTable.get("POSITION_QUANTIZED").get("byteOffset").get<double>();
                for (int i = 0; i < numPoints; ++i)
                {
                    unsigned short x = *reinterpret_cast<unsigned short*>(&data[byteOffset + i * 6 + 0]);
                    unsigned short y = *reinterpret_cast<unsigned short*>(&data[byteOffset + i * 6 + 2]);
                    unsigned short z = *reinterpret_cast<unsigned short*>(&data[byteOffset + i * 6 + 4]);
                    (*va)[i].set(osg::Vec3(x * scale[0] / 65535.0f, y * scale[1] / 65535.0f,
                                           z * scale[2] / 65535.0f) + offset);
                }
            }

            if (featureTable.contains("RGB"))
            {
                byteOffset = (int)featureTable.get("RGB").get("byteOffset").get<double>();
                for (int i = 0; i < numPoints; ++i) (*ca)[i].set(
                    (unsigned char)data[byteOffset + i * 3 + 0], (unsigned char)data[byteOffset + i * 3 + 1],
                    (unsigned char)data[byteOffset + i * 3 + 2], 255);
            }
            else if (featureTable.contains("RGBA"))
            {
                byteOffset = (int)featureTable.get("RGBA").get("byteOffset").get<double>();
                for (int i = 0; i < numPoints; ++i) (*ca)[i].set(
                    (unsigned char)data[byteOffset + i * 4 + 0], (unsigned char)data[byteOffset + i * 4 + 1],
                    (unsigned char)data[byteOffset + i * 4 + 2], (unsigned char)data[byteOffset + i * 4 + 3]);
            }

            if (featureTable.contains("NORMAL"))
            {
                byteOffset = (int)featureTable.get("NORMAL").get("byteOffset").get<double>();
                for (int i = 0; i < numPoints; ++i) (*na)[i].set(
                    *reinterpret_cast<float*>(&data[byteOffset + i * 12 + 0]),
                    *reinterpret_cast<float*>(&data[byteOffset + i * 12 + 4]),
                    *reinterpret_cast<float*>(&data[byteOffset + i * 12 + 8]));
            }

            osg::Geometry* geom = new osg::Geometry;
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            geom->setName("PNTS"); geom->setVertexArray(va.get());
            geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
            geom->setColorArray(na.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, va->size()));

            osg::Geode* geode = new osg::Geode;
            geode->addDrawable(geom); return geode;
        }
        return NULL;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_gltf, ReaderWriterGLTF)
