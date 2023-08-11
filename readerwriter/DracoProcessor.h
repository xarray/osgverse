#ifndef MANA_READERWRITER_DRACOPROCESSOR_HPP
#define MANA_READERWRITER_DRACOPROCESSOR_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osgDB/ReaderWriter>
#include "Export.h"

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT DracoProcessor : public osg::Referenced
    {
    public:
        DracoProcessor()
        {
            _posQuantizationBits = 11;
            _uvQuantizationBits = 10;
            _normalQuantizationBits = 8;
            _compressionLevel = 7;
        }

        void setPosQuantizationBits(int bits) { _posQuantizationBits = bits; }
        void setUvQuantizationBits(int bits) { _uvQuantizationBits = bits; }
        void setNormalQuantizationBits(int bits) { _normalQuantizationBits = bits; }
        void setCompressionLevel(int lv) { _compressionLevel = lv; }

        int getPosQuantizationBits() const { return _posQuantizationBits; }
        int getUvQuantizationBits() const { return _uvQuantizationBits; }
        int getNormalQuantizationBits() const { return _normalQuantizationBits; }
        int getCompressionLevel() const { return _compressionLevel; }

        osg::Geometry* decodeDracoData(std::istream& in);
        bool decodeDracoData(std::istream& in, osg::Geometry* geom);
        bool encodeDracoData(std::ostream& out, osg::Geometry* geom);

    protected:
        int _posQuantizationBits, _uvQuantizationBits;
        int _normalQuantizationBits, _compressionLevel;
    };

    class OSGVERSE_RW_EXPORT DracoGeometry : public osg::Geometry
    {
    public:
        DracoGeometry();
        DracoGeometry(const DracoGeometry& copy,
                      const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
        DracoGeometry(const osg::Geometry& copy,
                      const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
        META_Object(osgVerse, DracoGeometry)
    };

}

#endif
