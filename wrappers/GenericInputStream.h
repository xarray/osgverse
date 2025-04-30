#ifndef MANA_WRAPPERS_GENERICINPUTSTREAM_HPP
#define MANA_WRAPPERS_GENERICINPUTSTREAM_HPP

#include <osgDB/InputStream>
#include <memory>
#include <string>

namespace osgVerse
{
    class InputUserData
    {
    public:
        void add(const char* name, ...);
    };

    typedef osgDB::InputStream InputStream;
}

#endif
