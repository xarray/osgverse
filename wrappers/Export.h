#ifndef MANA_WRAPPERS_EXPORT_HPP
#define MANA_WRAPPERS_EXPORT_HPP

#if defined(VERSE_STATIC_BUILD)
#  define OSGVERSE_WRAPPERS_EXPORT
#elif defined(VERSE_WINDOWS)
#  if defined(VERSE_WRAPPERS_LIBRARY)
#    define OSGVERSE_WRAPPERS_EXPORT __declspec(dllexport)
#  else
#    define OSGVERSE_WRAPPERS_EXPORT __declspec(dllimport)
#  endif
#else
#  define OSGVERSE_WRAPPERS_EXPORT
#endif

#include <osg/Version>
#include <string>

namespace osgVerse
{
    class RewrapperManager;

#if OSG_VERSION_LESS_THAN(3, 5, 0)
    inline bool updateOsgBinaryWrappers(const std::string& libName = "osg") { return false; }
    inline bool fixOsgBinaryWrappers(const std::string& libName = "osg") { return false; }
#else
    /** Add necessary methods to OSG class wrappers */
    OSGVERSE_WRAPPERS_EXPORT bool updateOsgBinaryWrappers(const std::string& libName = "osg");

    /** A quick function to help fix .osgb dead lock problem */
    OSGVERSE_WRAPPERS_EXPORT bool fixOsgBinaryWrappers(const std::string& libName = "osg");

    /** Load library and rewrapper manager instance used in wrapper classes */
    OSGVERSE_WRAPPERS_EXPORT RewrapperManager* loadRewrappers();
#endif

    /** Setup draco encoding parameters */
    enum EncodingDracoFlag {
        COMPRESS_LEVEL = 0, POSITION_QUANTIZATION = 1,
        UV_QUANTIZATION = 2, NORMAL_QUANTIZATION = 3
    };
    OSGVERSE_WRAPPERS_EXPORT void setEncodingDracoFlag(EncodingDracoFlag flag, int value);

}

#endif
