#include <osg/io_utils>
#include <osg/ImageUtils>
#include <osg/Image>
#include <osg/ImageSequence>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <animation/ParticleEngine.h>
#include <pipeline/Global.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <openvdb/openvdb.h>
#include <openvdb/io/Stream.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/Interpolation.h>

class ReaderWriterVDB : public osgDB::ReaderWriter
{
public:
    ReaderWriterVDB()
    {
        supportsExtension("verse_vdb", "osgVerse pseudo-loader");
        supportsExtension("vdb", "VDB point cloud and texture file");
        supportsOption("ReadDataType=<hint>", "Read option: <Mesh/Points>");
        supportsOption("DimensionScale=<hint>", "Read option: volume image size scale, default is 1.0");
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

        std::string hintString = options ? options->getPluginStringData("ReadDataType") : "";
        int hint = 0; if (hintString == "Mesh") hint = 1;

        osg::Geode* geode = new osg::Geode;
        for (size_t i = 0; i < grids->size(); ++i)
        {
            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
            osg::ref_ptr<osg::DrawElementsUInt> de;

            if (hint == 1)
            {
                openvdb::FloatGrid::Ptr g0 = openvdb::gridPtrCast<openvdb::FloatGrid>((*grids)[i]);
                if (g0) { de = createMesh(*g0, va.get(), na.get(), ca.get()); }

                openvdb::Int32Grid::Ptr g1 = openvdb::gridPtrCast<openvdb::Int32Grid>((*grids)[i]);
                if (g1) { de = createMesh(*g1, va.get(), na.get(), ca.get()); }

                openvdb::DoubleGrid::Ptr g2 = openvdb::gridPtrCast<openvdb::DoubleGrid>((*grids)[i]);
                if (g2) { de = createMesh(*g2, va.get(), na.get(), ca.get()); }
            }
            else
            {
                openvdb::FloatGrid::Ptr g0 = openvdb::gridPtrCast<openvdb::FloatGrid>((*grids)[i]);
                if (g0)
                {
                    for (openvdb::FloatGrid::ValueOnIter iter = g0->beginValueOn(); iter; ++iter)
                    {
                        openvdb::Vec3d coord = iter.getCoord().asVec3d(); float value = iter.getValue();
                        va->push_back(osg::Vec3(coord.x(), coord.y(), coord.z()));
                        ca->push_back(osg::Vec4(value, value, value, 1.0f));
                    }
                }

                openvdb::Int32Grid::Ptr g1 = openvdb::gridPtrCast<openvdb::Int32Grid>((*grids)[i]);
                if (g1)
                {
                    for (openvdb::Int32Grid::ValueOnIter iter = g1->beginValueOn(); iter; ++iter)
                    {
                        openvdb::Vec3d coord = iter.getCoord().asVec3d(); float value = iter.getValue() / 255.0f;
                        va->push_back(osg::Vec3(coord.x(), coord.y(), coord.z()));
                        ca->push_back(osg::Vec4(value, value, value, 1.0f));
                    }
                }

                openvdb::DoubleGrid::Ptr g2 = openvdb::gridPtrCast<openvdb::DoubleGrid>((*grids)[i]);
                if (g2)
                {
                    for (openvdb::DoubleGrid::ValueOnIter iter = g2->beginValueOn(); iter; ++iter)
                    {
                        openvdb::Vec3d coord = iter.getCoord().asVec3d(); float value = iter.getValue() / 255.0f;
                        va->push_back(osg::Vec3(coord.x(), coord.y(), coord.z()));
                        ca->push_back(osg::Vec4(value, value, value, 1.0f));
                    }
                }

                openvdb::Vec3fGrid::Ptr g3 = openvdb::gridPtrCast<openvdb::Vec3fGrid>((*grids)[i]);
                if (g3)
                {
                    for (openvdb::Vec3fGrid::ValueOnIter iter = g3->beginValueOn(); iter; ++iter)
                    {
                        openvdb::Vec3d coord = iter.getCoord().asVec3d(); openvdb::Vec3f value = iter.getValue();
                        va->push_back(osg::Vec3(coord.x(), coord.y(), coord.z()));
                        ca->push_back(osg::Vec4(value.x(), value.y(), value.z(), 1.0f));
                    }
                }
            }

            if (!va->empty())
            {
                osg::Geometry* geom = new osg::Geometry;
                geom->setUseDisplayList(false);
                geom->setUseVertexBufferObjects(true);
                geom->setVertexArray(va.get());
                if (na->size() == va->size())
                    { geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
                if (ca->size() == va->size())
                    { geom->setColorArray(ca.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }

                if (de.valid()) geom->addPrimitiveSet(de.get());
                else geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, va->size()));
                geom->setName((*grids)[i]->getName()); geode->addDrawable(geom);
            }
            else
            {
                OSG_WARN << "[ReaderWriterVDB] Unsupported VDB grid "
                         << (*grids)[i]->getName() << std::endl;
            }
        }
        if (geode->getNumDrawables() > 0) return geode;
        else return ReadResult::ERROR_IN_READING_FILE;
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream ifile(fileName, std::ios_base::in | std::ios_base::binary);
        if (!ifile) return ReadResult::FILE_NOT_FOUND;
        return readImage(ifile, options);
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        openvdb::io::Stream strm(fin);
        openvdb::GridPtrVecPtr grids = strm.getGrids();
        if (!grids) return ReadResult::ERROR_IN_READING_FILE;

        std::vector<osg::ref_ptr<osg::Image>> images;
        std::string hintStr = options ? options->getPluginStringData("DimensionScale") : "0.1";
        float scale = atof(hintStr.c_str()); if (scale <= 0.0f) scale = 1.0f;
        for (size_t i = 0; i < grids->size(); ++i)
        {
            osg::ref_ptr<osg::Image> img = new osg::Image;
            openvdb::CoordBBox bbox = (*grids)[i]->evalActiveVoxelBoundingBox();
            osg::Vec3d res(osg::Image::computeNearestPowerOfTwo(bbox.dim().x() * scale),
                           osg::Image::computeNearestPowerOfTwo(bbox.dim().y() * scale),
                           osg::Image::computeNearestPowerOfTwo(bbox.dim().z() * scale));

            openvdb::FloatGrid::Ptr g0 = openvdb::gridPtrCast<openvdb::FloatGrid>((*grids)[i]);
            if (g0)
            {
                img->allocateImage(res[0], res[1], res[2], GL_LUMINANCE, GL_UNSIGNED_BYTE);
                img->setInternalTextureFormat(GL_LUMINANCE8);
                createImage<openvdb::FloatGrid, float>(*g0, img.get(), res, 1);
                if (img->valid()) images.push_back(img);
            }

            openvdb::Int32Grid::Ptr g1 = openvdb::gridPtrCast<openvdb::Int32Grid>((*grids)[i]);
            if (g1)
            {
                img->allocateImage(res[0], res[1], res[2], GL_LUMINANCE, GL_UNSIGNED_BYTE);
                img->setInternalTextureFormat(GL_LUMINANCE8);
                createImage<openvdb::Int32Grid, int32_t>(*g1, img.get(), res, 1);
                if (img->valid()) images.push_back(img);
            }

            openvdb::DoubleGrid::Ptr g2 = openvdb::gridPtrCast<openvdb::DoubleGrid>((*grids)[i]);
            if (g2)
            {
                img->allocateImage(res[0], res[1], res[2], GL_LUMINANCE, GL_UNSIGNED_BYTE);
                img->setInternalTextureFormat(GL_LUMINANCE8);
                createImage<openvdb::DoubleGrid, double>(*g2, img.get(), res, 1);
                if (img->valid()) images.push_back(img);
            }

#if 0
            for (int r = 0; r < img->r(); ++r)
            {
                osg::ref_ptr<osg::Image> cp = new osg::Image;
                cp->allocateImage(img->s(), img->t(), 1, img->getPixelFormat(), img->getDataType());
                osg::copyImage(img.get(), 0, 0, r, img->s(), img->t(), 1, cp.get(), 0, 0, 0);
                osgDB::writeImageFile(*cp, std::to_string(r) + "___.bmp");
            }
#endif
        }
        if (images.empty()) return ReadResult::FILE_LOADED;
        else if (images.size() == 1) return images.front().get();

        osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
        for (size_t i = 0; i < images.size(); ++i) seq->addImage(images[i]);
        return seq.get();
    }

    virtual WriteResult writeImage(const osg::Image& image, const std::string& path,
                                   const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ofstream ofile(fileName, std::ios_base::out | std::ios_base::binary);
        if (!ofile) return WriteResult::ERROR_IN_WRITING_FILE;
        return writeImage(image, ofile, options);
    }
    
    virtual WriteResult writeObject(const osg::Object& obj, const std::string& path,
                                    const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ofstream ofile(fileName, std::ios_base::out | std::ios_base::binary);
        if (!ofile) return WriteResult::ERROR_IN_WRITING_FILE;
        return writeObject(obj, ofile, options);
    }

    virtual WriteResult writeImage(const osg::Image& image, std::ostream& fout,
                                   const Options* options) const
    {
        openvdb::GridPtrVecPtr grids(new openvdb::GridPtrVec);
        openvdb::Coord coord;

        GLenum dataType = osg::Image::computeFormatDataType(image.getPixelFormat());
        unsigned int numComponents = osg::Image::computeNumComponents(image.getPixelFormat());
        unsigned int pixelSize = osg::Image::computePixelSizeInBits(image.getPixelFormat(), image.getDataType());
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

    virtual WriteResult writeObject(const osg::Object& obj, std::ostream& fout,
                                    const Options* options) const
    {
        const osgVerse::ParticleCloud* cloud = dynamic_cast<const osgVerse::ParticleCloud*>(&obj);
        if (!cloud) return WriteResult::NOT_IMPLEMENTED;

        const osg::Vec4Array* positions = cloud->getData(0);
        const osg::Vec4Array* attributes = cloud->getData(3);
        openvdb::GridPtrVecPtr grids(new openvdb::GridPtrVec);
        openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create();
        openvdb::FloatGrid::Accessor accessor = grid->getAccessor();

        openvdb::Coord coord; grid->setName("CloudValue");
        unsigned int size = positions->size();
        for (unsigned int i = 0; i < size; ++i)
        {
            const osg::Vec4& pos = positions->at(i);
            const osg::Vec4& attr = attributes->at(i);
            coord.reset(pos[0], pos[1], pos[2]);
            accessor.setValue(coord, attr[0]);  // FIXME: only for Zhijiang csv...
        }
        grids->push_back(grid);

        if (grids->empty()) return WriteResult::NOT_IMPLEMENTED;
        openvdb::io::Stream(fout).write(*grids);
        return WriteResult::FILE_SAVED;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_vdb");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    template<typename T> struct ValueRange
    {
        ValueRange() : _min(std::numeric_limits<T>::max()), _max(std::numeric_limits<T>::min()) {}
        ValueRange(T min_v, T max_v) : m_min(min_v), m_max(max_v) {}
        void addValue(T value) { _min = std::min(_min, value); _max = std::max(_max, value); }
        T _min, _max;
    };

    template<typename GridType, typename T>
    void createImage(GridType& grid, osg::Image* image, const osg::Vec3d& res, int comp) const
    {
        openvdb::tools::GridSampler<GridType, openvdb::tools::BoxSampler> sampler(grid);
        openvdb::BBoxd worldBox = grid.transform().indexToWorld(getIndexSpaceBoundingBox(grid));
        openvdb::CoordBBox domain(
            openvdb::Coord(0, 0, 0), openvdb::Coord(res[0] - 1, res[1] - 1, res[2] - 1));
        const openvdb::Vec3i stride = { 1, (int)res[0], (int)(res[0] * res[1]) };

        long long num_voxels = domain.volume();
        std::vector<T> dataArray(num_voxels * comp);
        tbb::enumerable_thread_specific<ValueRange<T>> ranges;
        tbb::parallel_for(domain,
            [&sampler, &worldBox, &res, &comp, &stride, &ranges, &dataArray](const openvdb::CoordBBox& bbox)
        {
            tbb::enumerable_thread_specific<ValueRange<T>>::reference this_range = ranges.local();
            for (int z = bbox.min().z(); z <= bbox.max().z(); ++z)
                for (int y = bbox.min().y(); y <= bbox.max().y(); ++y)
                    for (int x = bbox.min().x(); x <= bbox.max().x(); ++x)
                    {
                        const openvdb::Vec3i domain_index(x, y, z);
                        const int linear_index = domain_index.dot(stride) * comp;
                        openvdb::Vec3d pos(res[0], res[1], res[2]), ext = worldBox.extents();
                        pos = worldBox.min() + ((openvdb::Vec3d(domain_index) + 0.5) / pos) * ext;

                        T value = sampler.wsSample(pos);
                        for (int i = 0; i < comp; ++i) dataArray[linear_index + i] = value;
                        this_range.addValue(value);
                    }
        });

        // Merge per-thread value ranges
        ValueRange<T> out_value_range;
        for (size_t i = 0; i < ranges.size(); ++i)
        {
            const ValueRange<T>& per_thread_range = *(ranges.begin() + i);
            out_value_range.addValue(per_thread_range._min);
            out_value_range.addValue(per_thread_range._max);
        }

        // Remap sample values to [0, 1]
        unsigned char* ptr = (unsigned char*)image->data();
        tbb::parallel_for(tbb::blocked_range<size_t>(0, num_voxels * comp),
            [&ptr, &dataArray, &out_value_range](const tbb::blocked_range<size_t>& range)
        {
            double inv = 1.0 / (double)(out_value_range._max - out_value_range._min);
            for (size_t i = range.begin(); i < range.end(); ++i)
            {
                double v = (double)(dataArray[i] - out_value_range._min) * inv;
                *(ptr + i) = (unsigned char)(v * 255.0);
            }
        });
    }

    osg::DrawElementsUInt* createTreeTopology(openvdb::FloatGrid& grid, osg::Vec3Array* va,
                                              osg::Vec3Array* na, osg::Vec4Array* ca) const
    {
        for (typename openvdb::FloatGrid::TreeType::NodeCIter itr = grid.tree().cbeginNode(); itr; ++itr)
        {
            openvdb::CoordBBox bbox; itr.getBoundingBox(bbox);
            // TODO
        }
    }

    template<typename GridType>
    osg::DrawElementsUInt* createMesh(GridType& grid, osg::Vec3Array* va,
                                      osg::Vec3Array* na, osg::Vec4Array* ca) const
    {
        openvdb::tools::VolumeToMesh mesher(
            grid.getGridClass() == openvdb::GRID_LEVEL_SET ? 0.0 : 0.01);
        mesher(grid);

        openvdb::tools::PointList& pointList = mesher.pointList();
        for (openvdb::Index64 n = 0, i = 0, N = mesher.pointListSize(); n < N; ++n)
        {
            const openvdb::Vec3s& p = pointList[n];
            va->push_back(osg::Vec3(p[0], p[1], p[2]));
        }

        osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES);
        na->resize(va->size());

        openvdb::tools::PolygonPoolList& polygonPoolList = mesher.polygonPoolList();
        for (openvdb::Index64 n = 0, N = mesher.polygonPoolListSize(); n < N; ++n)
        {
            const openvdb::tools::PolygonPool& polygons = polygonPoolList[n];
            for (openvdb::Index64 i = 0, I = polygons.numQuads(); i < I; ++i)
            {
                const openvdb::Vec4I& quad = polygons.quad(i);
                de->push_back(quad[0]); de->push_back(quad[1]); de->push_back(quad[2]);
                de->push_back(quad[0]); de->push_back(quad[2]); de->push_back(quad[3]);

                openvdb::Vec3d e1 = pointList[quad[1]] - pointList[quad[0]];
                openvdb::Vec3d e2 = pointList[quad[2]] - pointList[quad[1]];
                openvdb::Vec3d n0 = e1.cross(e2);
                osg::Vec3 normal(n0[0], n0[1], n0[2]); normal.normalize();
                for (int j = 0; j < 4; ++j) (*na)[quad[j]] = -normal;
            }
        }
        return de;
    }

    openvdb::CoordBBox getIndexSpaceBoundingBox(const openvdb::GridBase& grid) const
    {
        try
        {
            const openvdb::Coord file_bbox_min(grid.metaValue<openvdb::Vec3i>("file_bbox_min"));
            const openvdb::Coord file_bbox_max(grid.metaValue<openvdb::Vec3i>("file_bbox_max"));
            if (file_bbox_min.x() == std::numeric_limits<int>::max() ||
                file_bbox_min.y() == std::numeric_limits<int>::max() ||
                file_bbox_min.z() == std::numeric_limits<int>::max()) return openvdb::CoordBBox();
            if (file_bbox_max.x() == std::numeric_limits<int>::min() ||
                file_bbox_max.y() == std::numeric_limits<int>::min() ||
                file_bbox_max.z() == std::numeric_limits<int>::min()) return openvdb::CoordBBox();
            return openvdb::CoordBBox(file_bbox_min, file_bbox_max);
        }
        catch (openvdb::Exception e) { return openvdb::CoordBBox(); }
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_vdb, ReaderWriterVDB)
