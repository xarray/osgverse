#ifndef MANA_READERWRITER_EPT_SETTINGS_HPP
#define MANA_READERWRITER_EPT_SETTINGS_HPP

#include <osg/Geode>
#include <osg/PagedLOD>

struct ReadEptSettings : public osg::Referenced
{
    bool lazOffsetToVertices;
    float minimumExpiryTime, invR;
    osg::LOD::RangeMode rangeMode;
    std::map<int, float> levelToLodRangeMin;
    std::map<int, float> levelToLodRangeMax;

    ReadEptSettings() : lazOffsetToVertices(true), minimumExpiryTime(0.0f)
    {
        invR = 1.0 / 255.0f; rangeMode = osg::LOD::PIXEL_SIZE_ON_SCREEN;
        levelToLodRangeMin = { {0, 5.0f}, {1, 114.87f}, {2, 124.573f}, {3, 131.951f}, {4, 137.973f},
                               {5, 143.097f}, {6, 147.577f}, {7, 151.572f}, {8, 155.185f}, {9, 158.489f},
                               {10, 161.539f}, {11, 164.375f}, {12, 167.028f}, {13, 169.522f}, {14, 171.877f} };
        levelToLodRangeMax = { {0, 250.0f}, {1, 217.638f}, {2, 200.685f}, {3, 189.465f}, {4, 181.195f},
                               {5, 174.707f}, {6, 169.403f}, {7, 164.938f}, {8, 161.099f}, {9, 157.739f},
                               {10, 154.761f}, {11, 152.091f}, {12, 149.676f}, {13, 147.474f}, {14, 145.453f} };
    }
};

extern osg::Node* readNodeFromUnityPoint(const std::string& file, const ReadEptSettings& settings);
extern osg::Node* readNodeFromLaz(const std::string& file, const ReadEptSettings& settings);

#endif
