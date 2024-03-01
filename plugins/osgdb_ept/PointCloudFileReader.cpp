#include <osg/io_utils>
#include <osg/Version>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgDB/ConvertUTF>
#include <osgDB/fstream>

#include "ReaderWriterEPT_Setting.h"
#include "3rdparty/laszip/laszip_api.h"
#include <iostream>

osg::Node* readNodeFromUnityPoint(const std::string& file, const ReadEptSettings& settings)
{
    osgDB::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
    if (!in) return NULL;

    double bounds[6] = { 0.0 }; in.read((char*)bounds, sizeof(double) * 6);
    size_t numPoints = 0; in.read((char*)&numPoints, sizeof(size_t));
    if (numPoints == 0) return NULL;

    std::vector<float> vertices(numPoints * 3), normals(numPoints * 3), colors(numPoints * 4);
    std::vector<unsigned int> idList(numPoints);
    int mode = 0; in.read((char*)&mode, sizeof(int));

    in.read((char*)&(idList[0]), sizeof(unsigned int) * idList.size());
    in.read((char*)&(vertices[0]), sizeof(float) * vertices.size());
    if (mode & 0x2) in.read((char*)&(normals[0]), sizeof(float) * normals.size());
    if (mode & 0x1) in.read((char*)&(colors[0]), sizeof(float) * colors.size());

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(numPoints, (osg::Vec3f*)&(vertices[0]));
    osg::ref_ptr<osg::Vec4Array> ca = (mode & 0x1) ? new osg::Vec4Array(numPoints, (osg::Vec4f*)&(colors[0])) : NULL;
    osg::ref_ptr<osg::Vec3Array> na = (mode & 0x2) ? new osg::Vec3Array(numPoints, (osg::Vec3f*)&(normals[0])) : NULL;
    for (int i = 0; i < numPoints; ++i) if (ca) (*ca)[i] = (*ca)[i] * settings.invR;
    in.close();

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setName(file);
    geom->setVertexArray(va.get());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
    if (ca.get()) { geom->setColorArray(ca.get(), osg::Array::BIND_PER_VERTEX); }
    if (na.get()) { geom->setNormalArray(na.get(), osg::Array::BIND_PER_VERTEX); }
#else
    if (ca.get()) { geom->setColorArray(ca.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }
    if (na.get()) { geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
#endif
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, numPoints));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->addChild(geode.get());
    return mt.release();
}

osg::Node* readNodeFromLaz(const std::string& file, const ReadEptSettings& settings)
{
    laszip_POINTER laszipReader;
    if (laszip_create(&laszipReader))
    {
        OSG_NOTICE << "Can't create laszip reader for " << file << std::endl;
        return NULL;
    }

    laszip_BOOL isCompressed = 0;
    std::string fileUtf8 = osgDB::convertStringFromCurrentCodePageToUTF8(file);
    if (laszip_open_reader(laszipReader, fileUtf8.c_str(), &isCompressed))
    {
        char* msg = NULL; laszip_get_error(laszipReader, &msg);
        OSG_NOTICE << "Can't open reader for " << file << ": " << msg << std::endl;
        return NULL;
    }

    laszip_header* header = NULL;
    if (laszip_get_header_pointer(laszipReader, &header))
    {
        OSG_NOTICE << "Can't get header for " << file << std::endl;
        return NULL;
    }

    laszip_I64 numPoints = (header->number_of_point_records ? header->number_of_point_records : header->extended_number_of_point_records);
    osg::Vec3d offset(header->x_offset, header->y_offset, header->z_offset);
    osg::Vec3d scale(header->x_scale_factor, header->y_scale_factor, header->z_scale_factor);
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(numPoints);
    osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array(numPoints);
    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(numPoints);

    laszip_point* point = NULL;
    for (int i = 0; i < numPoints; ++i)
    {
        laszip_read_point(laszipReader);
        laszip_get_point_pointer(laszipReader, &point);
        if (settings.lazOffsetToVertices)
        {
            (*va)[i] = osg::Vec3(point->X * scale[0] + offset[0], point->Y * scale[1] + offset[1],
                                 point->Z * scale[2] + offset[2]);
        }
        else
            (*va)[i] = osg::Vec3((float)point->X, (float)point->Y, (float)point->Z);
        (*ca)[i] = osg::Vec4((float)point->rgb[0] * settings.invR, (float)point->rgb[1] * settings.invR,
                             (float)point->rgb[2] * settings.invR, 1.0f);
        if (na.valid())
        {
            if (point->extra_bytes != NULL && point->num_extra_bytes >= 12)
            {
                (*na)[i] = osg::Vec3(*(float*)&(point->extra_bytes[0]), *(float*)&(point->extra_bytes[4]),
                                     *(float*)&(point->extra_bytes[8]));
            }
            else na = NULL;
        }
    }
    laszip_close_reader(laszipReader);

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setName(file);
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get());
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
    if (ca.get()) geom->setColorArray(ca.get(), osg::Array::BIND_PER_VERTEX);
    if (na.get()) geom->setNormalArray(na.get(), osg::Array::BIND_PER_VERTEX);
#else
    if (ca.get()) { geom->setColorArray(ca.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX); }
    if (na.get()) { geom->setNormalArray(na.get()); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX); }
#endif
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, numPoints));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    if (!settings.lazOffsetToVertices)
        mt->setMatrix(osg::Matrix::scale(scale) * osg::Matrix::translate(offset));
    mt->addChild(geode.get());
    return mt.release();
}
