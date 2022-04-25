#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <laszip/laszip_api.h>
#include <iostream>
#include <fstream>

#define OPTIMIZED_ARRAYS 0

osg::Node* readNodeFromUnityPoint(const std::string& file, float invR = 1.0f)
{
    std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
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
#if OPTIMIZED_ARRAYS
    osg::ref_ptr<osg::Vec4ubArray> cba = ca.valid() ? new osg::Vec4ubArray(numPoints) : NULL;
#else
    osg::ref_ptr<osg::Vec3Array> na = (mode & 0x2) ? new osg::Vec3Array(numPoints, (osg::Vec3f*)&(normals[0])) : NULL;
    osg::ref_ptr<osg::Vec4Array> ca2 = (mode & 0x1) ? new osg::Vec4Array(numPoints) : NULL;
    osg::ref_ptr<osg::Vec2iArray> ids = new osg::Vec2iArray(numPoints);
#endif
    for (int i = 0; i < numPoints; ++i)
    {
        if (ca) (*ca)[i] = (*ca)[i] * invR;
#if OPTIMIZED_ARRAYS
        if (ca) (*cba)[i] = osg::Vec4ub((unsigned char)((*ca)[i][0] * 255.0f), (unsigned char)((*ca)[i][1] * 255.0f),
                                        (unsigned char)((*ca)[i][2] * 255.0f), (unsigned char)((*ca)[i][3] * 255.0f));
#else
        (*ids)[i] = osg::Vec2i(idList[i] % 65535, idList[i] / 65535);
#endif
    }
    in.close();
    
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setName(file);
    geom->setVertexArray(va.get());
#if OPTIMIZED_ARRAYS
    if (cba.get()) geom->setColorArray(cba.get(), osg::Array::BIND_PER_VERTEX);
#else
    if (ca.get()) geom->setColorArray(ca.get(), osg::Array::BIND_PER_VERTEX);
    if (na.get()) geom->setNormalArray(na.get(), osg::Array::BIND_PER_VERTEX);
    if (ca2.get()) geom->setSecondaryColorArray(ca2.get(), osg::Array::BIND_PER_VERTEX);
    geom->setVertexAttribArray(6, ids.get(), osg::Array::BIND_PER_VERTEX);
#endif
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, numPoints));
    
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());
    
    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->addChild(geode.get());
    return mt.release();
}

osg::Node* readNodeFromLaz(const std::string& file, float invR = 1.0 / 255.0f)
{
    laszip_POINTER laszipReader;
    if (laszip_create(&laszipReader))
    {
        OSG_NOTICE << "Can't create laszip reader for " << file << std::endl;
        return NULL;
    }
    
    laszip_BOOL isCompressed = 0;
    if (laszip_open_reader(laszipReader, file.c_str(), &isCompressed))
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
#if !OPTIMIZED_ARRAYS
    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(numPoints);
    osg::ref_ptr<osg::Vec4Array> ca2 = new osg::Vec4Array(numPoints);
    osg::ref_ptr<osg::Vec2iArray> ids = new osg::Vec2iArray(numPoints);
#endif
    
    laszip_point* point = NULL;
    for (int i = 0; i < numPoints; ++i)
    {
        laszip_read_point(laszipReader);
        laszip_get_point_pointer(laszipReader, &point);
        (*va)[i] = osg::Vec3(point->X * scale[0] + offset[0], point->Y * scale[1] + offset[1], point->Z * scale[2] + offset[2]);
        //(*va)[i] = osg::Vec3((float)point->X, (float)point->Y, (float)point->Z);
        (*ca)[i] = osg::Vec4((float)point->rgb[0] * invR, (float)point->rgb[1] * invR, (float)point->rgb[2] * invR, 1.0f);
#if !OPTIMIZED_ARRAYS
        (*ids)[i] = osg::Vec2i(point->point_source_ID, (point->user_data + 255 * point->classification));
        if (na.valid())
        {
            if (point->extra_bytes != NULL && point->num_extra_bytes >= 12)
            {
                (*na)[i] = osg::Vec3(*(float*)&(point->extra_bytes[0]), *(float*)&(point->extra_bytes[4]),
                                     *(float*)&(point->extra_bytes[8]));
            }
            else na = NULL;
        }
#endif
    }
    laszip_close_reader(laszipReader);
    
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setName(file);
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get());
    if (ca.get()) geom->setColorArray(ca.get(), osg::Array::BIND_PER_VERTEX);
#if !OPTIMIZED_ARRAYS
    if (na.get()) geom->setNormalArray(na.get(), osg::Array::BIND_PER_VERTEX);
    if (ca2.get()) geom->setSecondaryColorArray(ca2.get(), osg::Array::BIND_PER_VERTEX);
    geom->setVertexAttribArray(6, ids.get(), osg::Array::BIND_PER_VERTEX);
#endif
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, numPoints));
    
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());
    
    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    //mt->setMatrix(osg::Matrix::scale(scale) * osg::Matrix::translate(offset));
    mt->addChild(geode.get());
    return mt.release();
}
