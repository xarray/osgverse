#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>
#include "3rdparty/ktx/ktx.h"
#include "Export.h"

namespace osgVerse
{
    OSGVERSE_RW_EXPORT std::vector<osg::ref_ptr<osg::Image>> loadKtx(const std::string& file, const osgDB::Options* opt);
    OSGVERSE_RW_EXPORT std::vector<osg::ref_ptr<osg::Image>> loadKtx2(std::istream& in, const osgDB::Options* opt);

    OSGVERSE_RW_EXPORT bool saveKtx(const std::string& file, bool asCubeMap, const osgDB::Options* opt,
                                    const std::vector<osg::Image*>& images);
    OSGVERSE_RW_EXPORT bool saveKtx2(std::ostream& out, bool asCubeMap, const osgDB::Options* opt,
                                     const std::vector<osg::Image*>& images);
}
