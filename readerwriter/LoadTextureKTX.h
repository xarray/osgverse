#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>
#include <ktx/ktx.h>
#include "Export.h"

namespace osgVerse
{
    OSGVERSE_RW_EXPORT std::vector<osg::ref_ptr<osg::Image>> loadKtx(const std::string& file);
    OSGVERSE_RW_EXPORT std::vector<osg::ref_ptr<osg::Image>> loadKtx2(std::istream& in);

    OSGVERSE_RW_EXPORT bool saveKtx(const std::string& file, bool asCubeMap,
                                    const std::vector<osg::Image*>& images);
    OSGVERSE_RW_EXPORT bool saveKtx2(std::ostream& out, bool asCubeMap,
                                     const std::vector<osg::Image*>& images);
}
