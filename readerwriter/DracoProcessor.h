#ifndef MANA_READERWRITER_DRACOPROCESSOR_HPP
#define MANA_READERWRITER_DRACOPROCESSOR_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osgDB/ReaderWriter>

namespace osgVerse
{
    class DracoProcessor : public osg::Referenced
    {
    public:
        DracoProcessor();

        void setPosQuantizationBits(int bits) { _posQuantizationBits = bits; }
        void setUvQuantizationBits(int bits) { _uvQuantizationBits = bits; }
        void setNormalQuantizationBits(int bits) { _normalQuantizationBits = bits; }
        void setCompressionLevel(int lv) { _compressionLevel = lv; }

        int getPosQuantizationBits() const { return _posQuantizationBits; }
        int getUvQuantizationBits() const { return _uvQuantizationBits; }
        int getNormalQuantizationBits() const { return _normalQuantizationBits; }
        int getCompressionLevel() const { return _compressionLevel; }

        osg::Geometry* decodeDracoData(std::istream& in);
        bool encodeDracoData(std::ostream& out, osg::Geometry* geom);

    protected:
        int _posQuantizationBits, _uvQuantizationBits;
        int _normalQuantizationBits, _compressionLevel;
    };

}

#endif
