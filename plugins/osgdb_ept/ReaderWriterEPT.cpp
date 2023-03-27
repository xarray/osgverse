#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <picojson.h>
#include <iostream>
#include <fstream>
#include <sstream>

extern osg::Node* readNodeFromUnityPoint(const std::string& file, float invR = 1.0f);
extern osg::Node* readNodeFromLaz(const std::string& file, float invR = 1.0 / 255.0f);

static std::vector<std::string> split(const std::string& src, const char* seperator, bool ignoreEmpty)
{
    std::vector<std::string> slist;
    std::string sep = (seperator == NULL) ? " " : std::string(seperator);
    std::string::size_type start = src.find_first_not_of(sep);
    while (start != std::string::npos)
    {
        std::string::size_type end = src.find_first_of(sep, start);
        if (end != std::string::npos)
        {
            slist.push_back(std::string(src, start, end - start));
            if (ignoreEmpty) start = src.find_first_not_of(sep, end);
            else start = end + 1;
        }
        else
        {
            slist.push_back(std::string(src, start, src.size() - start));
            start = end;
        }
    }
    return slist;
}

class EptBuilder
{
public:
    EptBuilder(const std::string& dir, const std::string& ext, osgDB::Options* op = NULL)
        : _dataFilePath(dir), _dataFileExtIncludingDot(ext)
    {
        for (int i = 0; i < 15; ++i)
        {
            _levelToLodRangeMin[i] = 200.0f * powf((float)(i + 1), 0.2f);
            _levelToLodRangeMax[i] = 1000.0f / powf((float)(i + 1), 0.2f);
        }
        loadDataFromOptions(op);
    }

    void loadDataFromOptions(osgDB::Options* op)
    {
        if (!op) return; else _options = op;
        _minTotalBound[0] = atof(op->getPluginStringData("MinTotalBoundX").c_str());
        _minTotalBound[1] = atof(op->getPluginStringData("MinTotalBoundY").c_str());
        _minTotalBound[2] = atof(op->getPluginStringData("MinTotalBoundZ").c_str());
        _maxTotalBound[0] = atof(op->getPluginStringData("MaxTotalBoundX").c_str());
        _maxTotalBound[1] = atof(op->getPluginStringData("MaxTotalBoundY").c_str());
        _maxTotalBound[2] = atof(op->getPluginStringData("MaxTotalBoundZ").c_str());
    }

    osg::Node* createEptScene(picojson::value& eptRootJson, picojson::value& hierarchyJson,
        const std::string& hPath, osgDB::Options* globalOptions)
    {
        std::map<std::string, int> dataFileMap;
        retrieveTotalBounds(eptRootJson.get<picojson::object>());
        retrieveDataFileMap(dataFileMap, hierarchyJson.get<picojson::object>(), hPath);
        for (std::map<std::string, int>::iterator itr = dataFileMap.begin(); itr != dataFileMap.end(); ++itr)
            globalOptions->setPluginStringData(itr->first, std::to_string(itr->second));
        _options = globalOptions;

        globalOptions->setPluginStringData("MinTotalBoundX", std::to_string(_minTotalBound[0]));
        globalOptions->setPluginStringData("MinTotalBoundY", std::to_string(_minTotalBound[1]));
        globalOptions->setPluginStringData("MinTotalBoundZ", std::to_string(_minTotalBound[2]));
        globalOptions->setPluginStringData("MaxTotalBoundX", std::to_string(_maxTotalBound[0]));
        globalOptions->setPluginStringData("MaxTotalBoundY", std::to_string(_maxTotalBound[1]));
        globalOptions->setPluginStringData("MaxTotalBoundZ", std::to_string(_maxTotalBound[2]));
        return createPagedNode("0-0-0-0");
    }

    osg::Node* createPagedNode(const std::string& hierarchyName)
    {
        std::vector<std::string> loc = split(hierarchyName, "-", false);
        if (loc.size() < 4) return NULL;

        int level = atoi(loc[0].c_str()), locX = atoi(loc[1].c_str()),
            locY = atoi(loc[2].c_str()), locZ = atoi(loc[3].c_str());
        osg::BoundingBoxd bb = computeBound(level, locX, locY, locZ);

        osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
        plod->setName(hierarchyName);
        plod->setCenter(bb.center());
        plod->setRadius(bb.radius());
        plod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);

        osg::Node* child = (_dataFileExtIncludingDot.find("unitypoint") != std::string::npos)
            ? readNodeFromUnityPoint(_dataFilePath + hierarchyName + _dataFileExtIncludingDot)
            : readNodeFromLaz(_dataFilePath + hierarchyName + _dataFileExtIncludingDot);
        plod->addChild(child, _levelToLodRangeMin[level], FLT_MAX);

        int index = plod->getNumChildren();
        for (int z = 0; z <= 1; ++z)
            for (int y = 0; y <= 1; ++y)
                for (int x = 0; x <= 1; ++x)
                {
                    std::stringstream ss;
                    ss << (level + 1) << "-" << (locX * 2 + x) << "-" << (locY * 2 + y) << "-" << (locZ * 2 + z);
                    if (_options.valid() && _options->getPluginStringData(ss.str()).empty()) continue;

                    plod->setFileName(index, _dataFilePath + ss.str() + _dataFileExtIncludingDot + ".eptile");
                    plod->setRange(index, _levelToLodRangeMax[level], FLT_MAX);
                    index++;
                }
        plod->setDatabaseOptions(_options.get());
        return plod.release();
    }

protected:
    void retrieveTotalBounds(picojson::object& jsonMap)
    {
        picojson::array& bounds = jsonMap["bounds"].get<picojson::array>();
        if (bounds.size() == 6)
        {
            _minTotalBound[0] = bounds[0].get<double>();
            _minTotalBound[1] = bounds[1].get<double>();
            _minTotalBound[2] = bounds[2].get<double>();
            _maxTotalBound[0] = bounds[3].get<double>();
            _maxTotalBound[1] = bounds[4].get<double>();
            _maxTotalBound[2] = bounds[5].get<double>();
        }
    }

    void retrieveDataFileMap(std::map<std::string, int>& dataFileMap, picojson::object& jsonMap, const std::string& hPath)
    {
        for (picojson::value::object::const_iterator i = jsonMap.begin(); i != jsonMap.end(); ++i)
        {
            int count = atoi(i->second.to_str().c_str());
            if (count <= 1)
            {
                // Try to get and save data from another file
                std::string subHierarchy(hPath + i->first + ".json");
                std::ifstream subHierarchyStream(subHierarchy.c_str());
                if (!subHierarchyStream)
                {
                    OSG_NOTICE << "Failed to found file " << subHierarchy << std::endl;
                    continue;
                }
                else
                {
                    typedef std::istreambuf_iterator<char> sbuf_iterator;
                    picojson::value subHierarchyJson;
                    std::string stat = picojson::parse(
                        subHierarchyJson, std::string((sbuf_iterator(subHierarchyStream)), sbuf_iterator()));
                    if (!stat.empty())
                    {
                        OSG_NOTICE << "Failed to parse " << subHierarchy << ": " << stat << std::endl;
                        continue;
                    }

                    // Read hierarchy data from this file
                    retrieveDataFileMap(dataFileMap, subHierarchyJson.get<picojson::object>(), hPath);
                }
            }
            dataFileMap[i->first] = count;
        }
    }

    osg::BoundingBoxd computeBound(int level, int locX, int locY, int locZ)
    {
        osg::Vec3d cellSize = (_maxTotalBound - _minTotalBound) / pow(2.0, (double)level);
        osg::Vec3d minBound = _minTotalBound + osg::Vec3d(
            (double)locX * cellSize[0], (double)locY * cellSize[1], (double)locZ * cellSize[2]);
        return osg::BoundingBoxd(minBound, minBound + cellSize);
    }

    osg::ref_ptr<osgDB::Options> _options;
    osg::Vec3d _minTotalBound, _maxTotalBound;
    std::string _dataFilePath, _dataFileExtIncludingDot;
    std::map<int, float> _levelToLodRangeMin, _levelToLodRangeMax;
};

class ReaderWriterEPT : public osgDB::ReaderWriter
{
public:
    ReaderWriterEPT()
    {
        supportsExtension("verse_ept", "Entwine point cloud");
        supportsExtension("eptile", "Entwine point cloud tile file");
        supportsExtension("las", "Standard LAS format");
        supportsExtension("laz", "Compressed LAS format");
    }

    virtual const char* className() const
    {
        return "[osgVerse] EPT Point Cloud Reader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        std::string eptPath = osgDB::getNameLessExtension(path);
        std::string ext2 = osgDB::getLowerCaseFileExtension(eptPath), pathKey = eptPath;
        size_t eptDataIndex = eptPath.find("/ept-data/");
        if (eptDataIndex != eptPath.npos) pathKey = eptPath.substr(0, eptDataIndex);

        if (ext == "eptile")
        {
            std::string eptTileFile = osgDB::findDataFile(eptPath, options);
            if (eptTileFile.empty()) return ReadResult::FILE_NOT_FOUND;

            std::string tileDir = osgDB::getFilePath(eptTileFile) + "/";
            if (!_globalOptions[pathKey])
            {
                OSG_NOTICE << "Tile file " << eptTileFile << " lost its options" << std::endl;
                return ReadResult::ERROR_IN_READING_FILE;
            }

            EptBuilder builder(tileDir, osgDB::getFileExtensionIncludingDot(eptTileFile),
                               _globalOptions[pathKey].get());
            return builder.createPagedNode(osgDB::getStrippedName(eptTileFile));
        }
        else if (ext == "verse_ept")
        {
            osgDB::DirectoryContents eptRootDataFile = osgDB::expandWildcardsInFilename(eptPath + "/ept-data/0-0-0-0.*");
            if (!eptRootDataFile.empty())
            {
                return readEptScene(eptPath, eptRootDataFile, pathKey, options);
            }
            else  // load .ept as a list file
            {
                std::ifstream in(path); std::string line;
                if (!in)
                {
                    if (ext2 == "las" || ext2 == "laz")
                    {
                        std::string lasFile = osgDB::findDataFile(eptPath, options);
                        if (lasFile.empty()) return ReadResult::FILE_NOT_FOUND;
                        return readNodeFromLaz(lasFile);
                    }
                    return ReadResult::FILE_NOT_FOUND;
                }

                osg::ref_ptr<osg::Group> group = new osg::Group;
                osg::ref_ptr<osg::MatrixTransform> mt;
                int lineNum = 0;
                while (std::getline(in, line))
                {
                    if (lineNum % 2)
                    {
                        std::string subEptPath = osgDB::getNameLessExtension(line);
                        osgDB::DirectoryContents subRootDataFile = osgDB::expandWildcardsInFilename(
                            subEptPath + "/ept-data/0-0-0-0.*");
                        if (!subRootDataFile.empty())
                        {
                            ReadResult rr = readEptScene(subEptPath, subRootDataFile, subEptPath, options);
                            if (mt.valid() && rr.getNode()) mt->addChild(rr.getNode());
                        }
                    }
                    else
                    {
                        osg::Vec3d position, euler;
                        osgDB::StringList values; osgDB::split(line, values, ' ');
                        if (values.size() > 2)
                            position.set(atof(values[0].data()), atof(values[1].data()), atof(values[2].data()));
                        if (values.size() > 5)
                            euler.set(atof(values[3].data()), atof(values[4].data()), atof(values[5].data()));

                        mt = new osg::MatrixTransform;
                        mt->setMatrix(osg::Matrix::rotate(osg::inDegrees(euler[2]), osg::Z_AXIS)
                                    * osg::Matrix::rotate(osg::inDegrees(euler[0]), osg::X_AXIS)
                                    * osg::Matrix::rotate(osg::inDegrees(euler[1]), osg::Y_AXIS)
                                    * osg::Matrix::translate(position));
                        group->addChild(mt.get());
                    }
                    lineNum++;
                }
                return group;
            }
        }
        return ReadResult::FILE_NOT_HANDLED;
    }

protected:
    ReadResult readEptScene(const std::string& eptPath, const osgDB::DirectoryContents& eptRootDataFile,
                            const std::string& pathKey, const osgDB::Options* options) const
    {
        std::string eptRootFile = osgDB::findDataFile(eptPath + "/ept.json", options);
        std::string hierarchyFile = osgDB::findDataFile(eptPath + "/ept-hierarchy/0-0-0-0.json", options);
        if (eptRootFile.empty() || hierarchyFile.empty()) return ReadResult::FILE_NOT_FOUND;

        picojson::value eptRootJson, hierarchyJson;
        std::ifstream eptRootStream(eptRootFile.c_str()), hierarchyStream(hierarchyFile.c_str());
        if (!eptRootStream || !hierarchyStream)
            return ReadResult::ERROR_IN_READING_FILE;
        else
        {
            typedef std::istreambuf_iterator<char> sbuf_iterator;
            std::string stat1 = picojson::parse(eptRootJson, std::string((sbuf_iterator(eptRootStream)), sbuf_iterator()));
            std::string stat2 = picojson::parse(hierarchyJson, std::string((sbuf_iterator(hierarchyStream)), sbuf_iterator()));
            if (!stat1.empty()) OSG_NOTICE << "Failed to parse ept.json: " << stat1 << std::endl;
            if (!stat2.empty()) OSG_NOTICE << "Failed to parse ept-hierarchy/0-0-0-0.json: " << stat2 << std::endl;
            if (!stat1.empty() || !stat2.empty()) return ReadResult::ERROR_IN_READING_FILE;
        }

        EptBuilder builder(eptPath + "/ept-data/", osgDB::getFileExtensionIncludingDot(eptRootDataFile[0]));
        std::string subDirName = "/ept-hierarchy/"; _globalOptions[pathKey] = new osgDB::Options;
        return builder.createEptScene(eptRootJson, hierarchyJson, eptPath + subDirName,
                                      _globalOptions[pathKey].get());
    }

    mutable std::map<std::string, osg::ref_ptr<osgDB::Options>> _globalOptions;
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_ept, ReaderWriterEPT)
