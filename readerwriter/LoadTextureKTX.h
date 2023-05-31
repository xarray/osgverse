#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>
#include <ktx/ktx.h>

#if defined(VERSE_STATIC_BUILD)
#  define OSGVERSE_RW_EXPORT extern
#elif defined(VERSE_WINDOWS)
#  if defined(VERSE_RW_LIBRARY)
#    define OSGVERSE_RW_EXPORT   __declspec(dllexport)
#  else
#    define OSGVERSE_RW_EXPORT   __declspec(dllimport)
#  endif
#else
#  define OSGVERSE_RW_EXPORT extern
#endif

namespace osgVerse
{
    OSGVERSE_RW_EXPORT std::vector<osg::ref_ptr<osg::Image>> loadKtx(const std::string& file);
    OSGVERSE_RW_EXPORT std::vector<osg::ref_ptr<osg::Image>> loadKtx2(std::istream& in);

    OSGVERSE_RW_EXPORT bool saveKtx(const std::string& file, bool asCubeMap,
                                    const std::vector<osg::Image*>& images);
    OSGVERSE_RW_EXPORT bool saveKtx2(std::ostream& out, bool asCubeMap,
                                     const std::vector<osg::Image*>& images);
}
