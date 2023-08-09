#ifndef MANA_READERWRITER_EXPORT_HPP
#define MANA_READERWRITER_EXPORT_HPP

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

#endif
