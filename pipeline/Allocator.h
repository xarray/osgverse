#ifndef MANA_PP_ALLOCATOR_HPP
#define MANA_PP_ALLOCATOR_HPP

#ifdef VERSE_USE_MIMALLOC
#   define MIMALLOC_VERBOSE 1
#   include <mimalloc/mimalloc-override.h>
#   include <mimalloc/mimalloc-new-delete.h>
#   include <osg/Notify>
#   include <osg/Version>

struct AllocatorProxy
{
    AllocatorProxy()
    {
        OSG_NOTICE << "[osgVerse] Mimalloc v" << mi_version()
                   << " is used for memory allocation." << std::endl;
    }
};
static AllocatorProxy s_allocator;
#endif

#include <backward.hpp>  // for better debug info
namespace backward { static backward::SignalHandling sh; }

#endif
