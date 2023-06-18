#include <osg/io_utils>
#include <osg/Image>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include <pipeline/Global.h>
#include <openvdb/openvdb.h>
#include <openvdb/io/Stream.h>

class ReaderWriterVDB : public osgDB::ReaderWriter
{
public:
    ReaderWriterVDB()
    {
        supportsExtension("verse_vdb", "osgVerse pseudo-loader");
        supportsExtension("vdb", "VDB point cloud and texture file");
        openvdb::initialize();
    }

    virtual const char* className() const
    {
        return "[osgVerse] VDB point cloud and texture reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_vdb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        std::ifstream ifile(fileName, std::ios_base::in | std::ios_base::binary);
        if (!ifile) return ReadResult::FILE_NOT_FOUND;
        return readNode(ifile, options);
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        openvdb::io::Stream strm(fin);
        openvdb::GridPtrVecPtr grids = strm.getGrids();
        if (!grids) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
        for (size_t i = 0; i < grids->size(); ++i)
        {
            openvdb::FloatGrid::Ptr g0 = openvdb::gridPtrCast<openvdb::FloatGrid>((*grids)[i]);
            if (g0)
            {
                for (openvdb::FloatGrid::ValueOnIter iter = g0->beginValueOn(); iter; ++iter)
                {
                    openvdb::Vec3d coord = iter.getCoord().asVec3d();
                    float value = iter.getValue();
                    va->push_back(osg::Vec3(coord.x(), coord.y(), coord.z()));
                    ca->push_back(osg::Vec4(value, value, value, 1.0f));
                }
                continue;
            }

            openvdb::Int32Grid::Ptr g1 = openvdb::gridPtrCast<openvdb::Int32Grid>((*grids)[i]);
            if (g1)
            {
                for (openvdb::Int32Grid::ValueOnIter iter = g1->beginValueOn(); iter; ++iter)
                {
                    openvdb::Vec3d coord = iter.getCoord().asVec3d();
                    float value = iter.getValue() / 255.0f;
                    va->push_back(osg::Vec3(coord.x(), coord.y(), coord.z()));
                    ca->push_back(osg::Vec4(value, value, value, 1.0f));
                }
                continue;
            }

            openvdb::Vec3fGrid::Ptr g2 = openvdb::gridPtrCast<openvdb::Vec3fGrid>((*grids)[i]);
            if (g2)
            {
                for (openvdb::Vec3fGrid::ValueOnIter iter = g2->beginValueOn(); iter; ++iter)
                {
                    openvdb::Vec3d coord = iter.getCoord().asVec3d();
                    openvdb::Vec3f value = iter.getValue();
                    va->push_back(osg::Vec3(coord.x(), coord.y(), coord.z()));
                    ca->push_back(osg::Vec4(value.x(), value.y(), value.z(), 1.0f));
                }
                continue;
            }

            OSG_WARN << "[ReaderWriterVDB] Unsupported VDB grid "
                     << (*grids)[i]->getName() << std::endl;
        }

        if (!va->empty())
        {
            osg::Geometry* geom = new osg::Geometry;
            geom->setVertexArray(va.get());
            geom->setColorArray(ca.get());
            geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, va->size()));

            osg::Geode* geode = new osg::Geode;
            geode->addDrawable(geom);
            return geode;
        }
        return ReadResult::ERROR_IN_READING_FILE;
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_vdb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        std::ofstream ofile(fileName, std::ios_base::out | std::ios_base::binary);
        if (!ofile) return WriteResult::ERROR_IN_WRITING_FILE;
        return writeImage(image, ofile, options);
    }

    virtual WriteResult writeImage(const osg::Image& image, std::ostream& fout,
                                   const Options* options) const
    {
        openvdb::GridPtrVecPtr grids(new openvdb::GridPtrVec);
        openvdb::Coord coord;

        GLenum dataType = osg::Image::computeFormatDataType(image.getPixelFormat());
        unsigned int numComponents = osg::Image::computeNumComponents(image.getPixelFormat());
        unsigned int pixelSize = osg::Image::computePixelSizeInBits(
            image.getPixelFormat(), image.getDataType());
        pixelSize = pixelSize / numComponents;

#define TRAVERSE_COORD(gridType, name) \
    gridType ::Ptr grid = gridType ::create(); \
    grid->setName( name ); gridType ::Accessor accessor = grid->getAccessor(); \
    for (int z = 0; z < image.r(); ++z) for (int y = 0; y < image.t(); ++y) \
    for (int x = 0; x < image.s(); ++x)
    
        if (numComponents == 1)
        {
            if (dataType == GL_FLOAT && pixelSize == 32)
            {
                TRAVERSE_COORD(openvdb::FloatGrid, "density")
                {
                    float* ptr = (float*)image.data(x, y, z);
                    coord.reset(x, y, z); accessor.setValue(coord, *ptr);
                    if (*ptr == 0.0f) accessor.setValueOff(coord, (int)*ptr);
                }
                grids->push_back(grid);
            }
            else if (dataType == GL_INT || dataType == GL_UNSIGNED_INT)
            {
                TRAVERSE_COORD(openvdb::Int32Grid, "density")
                {
                    unsigned int* ptr = (unsigned int*)image.data(x, y, z);
                    coord.reset(x, y, z); accessor.setValue(coord, *ptr);
                    if (*ptr == 0) accessor.setValueOff(coord, (int)*ptr);
                }
                grids->push_back(grid);
            }
            else if (dataType == GL_BYTE || dataType == GL_UNSIGNED_BYTE)
            {
                TRAVERSE_COORD(openvdb::Int32Grid, "density")
                {
                    unsigned char* ptr = (unsigned char*)image.data(x, y, z);
                    coord.reset(x, y, z); accessor.setValue(coord, (int)*ptr);
                    if (*ptr == 0) accessor.setValueOff(coord, (int)*ptr);
                }
                grids->push_back(grid);
            }
            else
            {
                OSG_NOTICE << "[ReaderWriterVDB] Unsupported data type "
                           << std::hex << dataType << std::dec << ", pixel size = "
                           << pixelSize << std::endl;
            }
        }
        else if (numComponents == 3)
        {
            if (dataType == GL_FLOAT && pixelSize == 32)
            {
                TRAVERSE_COORD(openvdb::Vec3fGrid, "color")
                {
                    osg::Vec3f* ptr = (osg::Vec3f*)image.data(x, y, z);
                    openvdb::Vec3f vec(ptr->x(), ptr->y(), ptr->z());
                    coord.reset(x, y, z); accessor.setValue(coord, vec);
                }
                grids->push_back(grid);
            }
            else if (dataType == GL_INT || dataType == GL_UNSIGNED_INT)
            {
                TRAVERSE_COORD(openvdb::Vec3IGrid, "color")
                {
                    osg::Vec3i* ptr = (osg::Vec3i*)image.data(x, y, z);
                    openvdb::Vec3I vec((*ptr)[0], (*ptr)[1], (*ptr)[2]);
                    coord.reset(x, y, z); accessor.setValue(coord, vec);
                }
                grids->push_back(grid);
            }
            else if (dataType == GL_BYTE || dataType == GL_UNSIGNED_BYTE)
            {
                TRAVERSE_COORD(openvdb::Vec3IGrid, "color")
                {
                    osg::Vec3b* ptr = (osg::Vec3b*)image.data(x, y, z);
                    openvdb::Vec3I vec(ptr->x(), ptr->y(), ptr->z());
                    coord.reset(x, y, z); accessor.setValue(coord, vec);
                }
                grids->push_back(grid);
            }
            else
            {
                OSG_NOTICE << "[ReaderWriterVDB] Unsupported data type "
                           << std::hex << dataType << std::dec << ", pixel size = "
                           << pixelSize << std::endl;
            }
        }
#undef TRAVERSE_COORD

        if (grids->empty()) return WriteResult::NOT_IMPLEMENTED;
        openvdb::io::Stream(fout).write(*grids);
        return WriteResult::FILE_SAVED;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_vdb, ReaderWriterVDB)
