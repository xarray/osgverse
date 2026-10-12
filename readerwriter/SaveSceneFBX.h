#ifndef MANA_READERWRITER_SAVESCENE_FBX_HPP
#define MANA_READERWRITER_SAVESCENE_FBX_HPP

#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>

#include "Export.h"

namespace osgVerse
{
    class SaverFBX : public osg::Referenced
    {
    public:
        SaverFBX(const osg::Node& node, std::ostream& out, const std::string& dir,
                 const std::string& imgHint, bool isBinary);
        bool getResult() const { return _done; }

    protected:
        virtual ~SaverFBX();
        bool _done;
    };

    OSGVERSE_RW_EXPORT bool saveFbx(const osg::Node& node, const std::string& file,
                                    const std::string& imgHint, bool isBinary);
    OSGVERSE_RW_EXPORT bool saveFbx2(const osg::Node& node, std::ostream& out, const std::string& dir,
                                     const std::string& imgHint, bool isBinary);
}

#endif
