#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>
#include "modeling/GaussianGeometry.h"
#include "spz/load-spz.h"

class ReaderWriter3DGS : public osgDB::ReaderWriter
{
public:
    ReaderWriter3DGS()
    {
        supportsExtension("verse_3dgs", "osgVerse pseudo-loader");
        supportsExtension("ply", "PLY point cloud file");
        supportsExtension("splat", "Gaussian splat data file");
        supportsExtension("ksplat", "Mark Kellogg's splat file");
        supportsExtension("spz", "NianticLabs' splat file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] 3D Gaussian Scattering data format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_3dgs");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;

        osg::ref_ptr<Options> localOptions = NULL;
        if (options) localOptions = options->cloneOptions();
        else localOptions = new osgDB::Options();

        localOptions->setPluginStringData("prefix", osgDB::getFilePath(path));
        localOptions->setPluginStringData("extension", ext);
        return readNode(in, localOptions.get());
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        spz::UnpackOptions unpackOpt;  // TODO: convert coordinates
        if (options)
        {
            std::string prefix = options->getPluginStringData("prefix");
            std::string ext = options->getPluginStringData("extension");
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            spz::GaussianCloud cloud;
            if (ext == "ply")
            {
                cloud = spz::loadSplatFromPly(fin, prefix, unpackOpt);
                if (cloud.numPoints > 0) geode->addDrawable(fromSpz(cloud));
            }
            else
            {
                std::string buffer((std::istreambuf_iterator<char>(fin)),
                                   std::istreambuf_iterator<char>());
                if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

                if (ext == "spz")
                {
                    std::vector<uint8_t> dataSrc(buffer.begin(), buffer.end());
                    cloud = spz::loadSpz(dataSrc, unpackOpt);
                    if (cloud.numPoints > 0) geode->addDrawable(fromSpz(cloud));
                }
                else if (ext == "splat")
                {
                    // TODO
                    return ReadResult::NOT_IMPLEMENTED;
                }
                else if (ext == "ksplat")
                {
                    // TODO
                    return ReadResult::NOT_IMPLEMENTED;
                }
            }
        }

        if (geode->getNumDrawables() > 0) return geode.get();
        else return ReadResult::FILE_NOT_HANDLED;
    }

protected:
    osgVerse::GaussianGeometry* fromSpz(spz::GaussianCloud& c) const
    {
        osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array, scale = new osg::Vec3Array;
        for (size_t i = 0; i < c.positions.size(); i += 3)
            pos->push_back(osg::Vec3(c.positions[i], c.positions[i + 1], c.positions[i + 2]));
        for (size_t i = 0; i < c.scales.size(); i += 3)
            scale->push_back(osg::Vec3(c.scales[i], c.scales[i + 1], c.scales[i + 2]));

        osg::ref_ptr<osg::QuatArray> rot = new osg::QuatArray;
        for (size_t i = 0; i < c.rotations.size(); i += 4)
            rot->push_back(osg::Quat(c.rotations[i], c.rotations[i + 1], c.rotations[i + 2], c.rotations[i + 3]));

        osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray;
        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_POINTS);
        for (size_t i = 0; i < c.alphas.size(); i++)
            { alpha->push_back(c.alphas[i]); de->push_back(i); }

        osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array, gD0 = new osg::Vec4Array, bD0 = new osg::Vec4Array;
        osg::ref_ptr<osg::Vec4Array> rD1 = new osg::Vec4Array, gD1 = new osg::Vec4Array, bD1 = new osg::Vec4Array;
        osg::ref_ptr<osg::Vec4Array> rD2 = new osg::Vec4Array, gD2 = new osg::Vec4Array, bD2 = new osg::Vec4Array;
        osg::ref_ptr<osg::Vec4Array> rD3 = new osg::Vec4Array, gD3 = new osg::Vec4Array, bD3 = new osg::Vec4Array;
        size_t numShCoff = c.sh.size() / c.numPoints, shIndex = 0;
        for (size_t i = 0; i < c.colors.size(); i += 3, shIndex += numShCoff)
        {
            rD0->push_back(osg::Vec4(c.colors[i + 0], 0.0f, 0.0f, 0.0f));
            gD0->push_back(osg::Vec4(c.colors[i + 1], 0.0f, 0.0f, 0.0f));
            bD0->push_back(osg::Vec4(c.colors[i + 2], 0.0f, 0.0f, 0.0f));
            if (numShCoff >= 9)  // Degree 1
            {
                rD0->back().set(rD0->back()[0], c.sh[shIndex + 0], c.sh[shIndex + 3], c.sh[shIndex + 6]);
                gD0->back().set(gD0->back()[0], c.sh[shIndex + 1], c.sh[shIndex + 4], c.sh[shIndex + 7]);
                bD0->back().set(bD0->back()[0], c.sh[shIndex + 2], c.sh[shIndex + 5], c.sh[shIndex + 8]);
            }

            if (numShCoff >= 24)  // Degree 2
            {
                rD1->push_back(osg::Vec4(c.sh[shIndex + 9], c.sh[shIndex + 12], c.sh[shIndex + 15], c.sh[shIndex + 18]));
                gD1->push_back(osg::Vec4(c.sh[shIndex + 10], c.sh[shIndex + 13], c.sh[shIndex + 16], c.sh[shIndex + 19]));
                bD1->push_back(osg::Vec4(c.sh[shIndex + 11], c.sh[shIndex + 14], c.sh[shIndex + 17], c.sh[shIndex + 20]));
                rD2->push_back(osg::Vec4(c.sh[shIndex + 21], 0.0f, 0.0f, 0.0f));
                gD2->push_back(osg::Vec4(c.sh[shIndex + 22], 0.0f, 0.0f, 0.0f));
                bD2->push_back(osg::Vec4(c.sh[shIndex + 23], 0.0f, 0.0f, 0.0f));
            }

            if (numShCoff >= 45)  // Degree 3
            {
                rD2->back().set(rD2->back()[0], c.sh[shIndex + 24], c.sh[shIndex + 27], c.sh[shIndex + 30]);
                gD2->back().set(gD2->back()[0], c.sh[shIndex + 25], c.sh[shIndex + 28], c.sh[shIndex + 31]);
                bD2->back().set(bD2->back()[0], c.sh[shIndex + 26], c.sh[shIndex + 29], c.sh[shIndex + 32]);
                rD3->push_back(osg::Vec4(c.sh[shIndex + 33], c.sh[shIndex + 36], c.sh[shIndex + 39], c.sh[shIndex + 42]));
                gD3->push_back(osg::Vec4(c.sh[shIndex + 34], c.sh[shIndex + 37], c.sh[shIndex + 40], c.sh[shIndex + 43]));
                bD3->push_back(osg::Vec4(c.sh[shIndex + 35], c.sh[shIndex + 38], c.sh[shIndex + 41], c.sh[shIndex + 44]));
            }
        }

        osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry;
        geom->setShDegrees(c.shDegree); geom->setPosition(pos.get());
        geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
        geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get());
        if (numShCoff >= 24)
        {
            geom->setShRed(1, rD1.get()); geom->setShGreen(1, gD1.get()); geom->setShBlue(1, bD1.get());
            geom->setShRed(2, rD2.get()); geom->setShGreen(2, gD2.get()); geom->setShBlue(2, bD2.get());
            if (numShCoff >= 45)
                { geom->setShRed(3, rD3.get()); geom->setShGreen(3, gD3.get()); geom->setShBlue(3, bD3.get()); }
        }
        geom->addPrimitiveSet(de.get());
        return geom.release();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_3dgs, ReaderWriter3DGS)
