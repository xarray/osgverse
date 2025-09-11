#include <GenericReserializer.h>
#include "Export.h"

namespace osgVerse
{
    RewrapperManager* loadRewrappers()
    {
        // Must use loadRewrappers() instead of instance() in external libraries and executables
        // This is the real manager used in rewrapper classes!
        return RewrapperManager::instance().get();
    }
}
