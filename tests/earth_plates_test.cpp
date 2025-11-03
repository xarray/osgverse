#include <osg/io_utils>
#include <osg/ImageUtils>
#include <osg/LOD>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Tessellator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <modeling/GeometryMerger.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Utilities.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/DatabasePager.h>
#include <readerwriter/TileCallback.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

void createVolumeData(const std::string& csvFile, const std::string& raw3dFile)
{
    std::map<size_t, std::string> indexMap;
    std::map<std::string, std::string> valueMap;
    std::string line0, header; unsigned int rowID = 0;
    std::ifstream in(csvFile.c_str());

    typedef std::pair<osg::Vec2, float> DataPair;
    std::map<int, std::vector<DataPair>> layerData;
    while (std::getline(in, line0))
    {
        std::string line = osgVerse::StringAuxiliary::trim(line0); rowID++;
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::vector<std::string> values, vertices;
        osgVerse::StringAuxiliary::split(line, values, ',', false);
        if (!valueMap.empty())
        {
            size_t numColumns = valueMap.size();
            if (numColumns != values.size())
            {
                std::cout << "CSV line " << rowID << " has different values (" << values.size()
                          << ") than " << numColumns << " header columns" << std::endl; continue;
            }

            for (size_t i = 0; i < values.size(); ++i) valueMap[indexMap[i]] = values[i];
            float x = atof(valueMap["x"].c_str()), y = atof(valueMap["y"].c_str()),
                  z = atof(valueMap["z"].c_str()), amp = atof(valueMap["amp"].c_str());
            layerData[(int)z].push_back(DataPair(osg::Vec2(x, y), (amp + 10000.0) / 20000.0));
        }
        else
        {
            header = line;
            for (size_t i = 0; i < values.size(); ++i)
                { indexMap[i] = values[i]; valueMap[values[i]] = ""; }
        }
    }

    std::vector<osg::ref_ptr<osg::Image>> images;
    for (std::map<int, std::vector<DataPair>>::iterator it = layerData.begin();
         it != layerData.end(); ++it)
    {
        std::vector<osg::Vec2> points; std::vector<osg::Vec4> values;
        const std::vector<DataPair>& dataList = it->second;
        for (size_t i = 0; i < dataList.size(); ++i)
        {
            points.push_back(dataList[i].first);
            values.push_back(osg::Vec4(dataList[i].second, 0.0f, 0.0f, 0.0f));
        }

        osg::ref_ptr<osg::Image> image = osgVerse::createInterpolatedMap(points, values, 256, 256, 1, true);
        if (image.valid()) images.push_back(image);
    }

    osg::ref_ptr<osg::Image> image3D = osg::createImage3D(images, GL_LUMINANCE, 256, 256, layerData.size());
    std::ofstream out(raw3dFile.c_str(), std::ios::out | std::ios::binary);
    out.write((char*)image3D->data(), image3D->getTotalSizeInBytes()); out.close();
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();
    if (argc > 2) createVolumeData(argv[1], argv[2]);
    return 0;
}
