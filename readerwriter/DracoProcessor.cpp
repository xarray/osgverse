#include <osg/Version>
#include <osg/TriangleIndexFunctor>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <pipeline/Global.h>
#include "DracoProcessor.h"
using namespace osgVerse;

#if OSG_VERSION_GREATER_THAN(3, 1, 8)

#ifdef VERSE_USE_DRACO
#   include <draco/mesh/mesh.h>
#   include <draco/compression/encode.h>
#   include <draco/compression/decode.h>
#endif

struct CollectFaceOperator
{
    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (i1 == i2 || i2 == i3 || i1 == i3) return;
        triangles.push_back(osg::Vec3i(i1, i2, i3));
    }
    std::vector<osg::Vec3i> triangles;
};

#ifdef VERSE_USE_DRACO
static osg::Array* createDataArray(draco::Mesh* mesh, const draco::PointAttribute* attr)
{
    osg::Array* outArray = NULL;
    switch (attr->data_type())
    {
    case draco::DT_FLOAT32:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::FloatArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2Array(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3Array(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4Array(mesh->num_points()); break;
        }
        break;
    case draco::DT_FLOAT64:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::DoubleArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2dArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3dArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4dArray(mesh->num_points()); break;
        }
        break;
    case draco::DT_INT8:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::ByteArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2bArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3bArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4bArray(mesh->num_points()); break;
        }
        break;
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
    case draco::DT_UINT8:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::UByteArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2ubArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3ubArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4ubArray(mesh->num_points()); break;
        }
        break;
#endif
    case draco::DT_INT16:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::ShortArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2sArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3sArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4sArray(mesh->num_points()); break;
        }
        break;
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
    case draco::DT_UINT16:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::UShortArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2usArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3usArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4usArray(mesh->num_points()); break;
        }
        break;
    case draco::DT_INT32:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::IntArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2iArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3iArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4iArray(mesh->num_points()); break;
        }
        break;
    case draco::DT_UINT32:
        switch (attr->num_components())
        {
        case 1: outArray = new osg::UIntArray(mesh->num_points()); break;
        case 2: outArray = new osg::Vec2uiArray(mesh->num_points()); break;
        case 3: outArray = new osg::Vec3uiArray(mesh->num_points()); break;
        case 4: outArray = new osg::Vec4uiArray(mesh->num_points()); break;
        }
        break;
#endif
    }

    if (outArray)
    {
        unsigned char* data = new unsigned char[attr->byte_stride()];
        const char* dst = (const char*)(outArray->getDataPointer());
        unsigned int elemSize = outArray->getElementSize();
        for (draco::PointIndex i(0); i < mesh->num_points(); ++i)
        {
            const draco::AttributeValueIndex valIndex = attr->mapped_index(i);
            attr->GetValue(valIndex, data);
            memcpy((void*)(dst + i.value() * elemSize), data, attr->byte_stride());
        }
        delete[] data;
    }
    else
        OSG_WARN << "[DracoProcessor] Unsupported Draco data type" << std::endl;
    return outArray;
}

static int addGeometryAttribute(draco::Mesh* mesh, draco::GeometryAttribute::Type type,
                                const osg::Array* arrayPtr)
{
    draco::DataType dt = draco::DT_INVALID;
    switch (arrayPtr->getDataType())
    {
    case GL_BYTE: dt = draco::DT_INT8; break;
    case GL_SHORT: dt = draco::DT_INT16; break;
    case GL_INT: dt = draco::DT_INT32; break;
    case GL_UNSIGNED_BYTE: dt = draco::DT_UINT8; break;
    case GL_UNSIGNED_SHORT: dt = draco::DT_UINT16; break;
    case GL_UNSIGNED_INT: dt = draco::DT_UINT32; break;
    case GL_FLOAT: dt = draco::DT_FLOAT32; break;
    case GL_DOUBLE: dt = draco::DT_FLOAT64; break;
    }

    draco::GeometryAttribute attr;
    attr.Init(type, nullptr, arrayPtr->getDataSize(), dt, false,
              draco::DataTypeLength(dt) * (int64_t)arrayPtr->getDataSize(), 0);
    return mesh->AddAttribute(attr, true, arrayPtr->getNumElements());
}
#endif

osg::Geometry* DracoProcessor::decodeDracoData(std::istream& in)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    if (decodeDracoData(in, geom.get()))
        return geom.release();
    return NULL;
}

bool DracoProcessor::decodeDracoData(std::istream& in, osg::Geometry* geom)
{
#ifdef VERSE_USE_DRACO
    std::string data((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    if (data.empty()) return false;

    draco::DecoderBuffer buffer;
    buffer.Init(data.data(), data.size());

    draco::StatusOr<draco::EncodedGeometryType> statusor =
        draco::Decoder::GetEncodedGeometryType(&buffer);
    if (!statusor.ok())
    {
        OSG_WARN << "[DracoProcessor] Error initing: " << statusor.status() << std::endl;
        return false;
    }
    else if (statusor.value() != draco::TRIANGULAR_MESH)
    {
        OSG_WARN << "[DracoProcessor] Unsupported mesh type: " << statusor.value() << std::endl;
        return false;
    }

    draco::Decoder decoder;
    draco::StatusOr<std::unique_ptr<draco::Mesh>> statusor2 =
        decoder.DecodeMeshFromBuffer(&buffer);
    if (!statusor2.ok())
    {
        OSG_WARN << "[DracoProcessor] Error decoding: " << statusor2.status() << std::endl;
        return false;
    }


    std::unique_ptr<draco::Mesh> mesh = std::move(statusor2).value();
    const draco::PointAttribute* va = mesh->GetNamedAttribute(draco::GeometryAttribute::POSITION);
    const draco::PointAttribute* na = mesh->GetNamedAttribute(draco::GeometryAttribute::NORMAL);
    const draco::PointAttribute* ca = mesh->GetNamedAttribute(draco::GeometryAttribute::COLOR);
    const draco::PointAttribute* ta = mesh->GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);

    if (va) geom->setVertexArray(createDataArray(mesh.get(), va));
    if (ta) geom->setTexCoordArray(0, createDataArray(mesh.get(), ta));
    if (na)
    {
        geom->setNormalArray(createDataArray(mesh.get(), na));
        geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (ca)
    {
        geom->setColorArray(createDataArray(mesh.get(), ca));
        geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }

    unsigned int index = 0;
    osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES, mesh->num_faces() * 3);
    for (draco::FaceIndex f(0); f < mesh->num_faces(); ++f)
    {
        const draco::Mesh::Face& face = mesh->face(f);
        (*de)[index++] = face[0].value();
        (*de)[index++] = face[1].value();
        (*de)[index++] = face[2].value();
    }
    geom->addPrimitiveSet(de);
    return true;
#else
    OSG_WARN << "[DracoProcessor] Dependency not found" << std::endl;
    return false;
#endif
}

bool DracoProcessor::encodeDracoData(std::ostream& out, osg::Geometry* geom)
{
#ifdef VERSE_USE_DRACO
    const int speed = 10 - _compressionLevel;
    draco::Encoder encoder;
    encoder.SetSpeedOptions(speed, speed);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, _posQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, _normalQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, _uvQuantizationBits);

    osg::TriangleIndexFunctor<CollectFaceOperator> functor;
    if (geom) geom->accept(functor);
    if (functor.triangles.empty()) return false;

    std::unique_ptr<draco::Mesh> mesh(new draco::Mesh());
    mesh->SetNumFaces(functor.triangles.size());
    for (size_t i = 0; i < functor.triangles.size(); ++i)
    {
        const osg::Vec3i& tri = functor.triangles[i];
        mesh->SetFace(draco::FaceIndex(i), {
            draco::PointIndex(tri[0]), draco::PointIndex(tri[1]), draco::PointIndex(tri[2]) });
    }

    const osg::Array* va = geom->getVertexArray();
    const osg::Array* na = geom->getNormalArray();
    const osg::Array* ca = geom->getColorArray();
    const osg::Array* ta = geom->getTexCoordArray(0);
    mesh->set_num_points(va->getNumElements());

    int posAttrID = -1, uvAttrID = -1, normalAttrID = -1, colorAttrID = -1;
    posAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::POSITION, va);
    uvAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::TEX_COORD, ta);
    if (geom->getNormalBinding() == osg::Geometry::BIND_PER_VERTEX)
        normalAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::NORMAL, na);
    if (geom->getColorBinding() == osg::Geometry::BIND_PER_VERTEX)
        colorAttrID = addGeometryAttribute(mesh.get(), draco::GeometryAttribute::COLOR, ca);

    if (posAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(posAttrID);
        for (unsigned int i = 0; i < va->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), va->getDataPointer(i));
    }

    if (normalAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(normalAttrID);
        for (unsigned int i = 0; i < na->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), na->getDataPointer(i));
    }

    if (colorAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(colorAttrID);
        for (unsigned int i = 0; i < ca->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), ca->getDataPointer(i));
    }

    if (uvAttrID >= 0)
    {
        draco::GeometryAttribute* attr = mesh->attribute(uvAttrID);
        if (attr->num_components() != 2)
        {
            // disable the predicator as it is causing an issue with decompression
            encoder.SetAttributePredictionScheme(
                draco::GeometryAttribute::Type::TEX_COORD, draco::PREDICTION_NONE);
        }

        for (unsigned int i = 0; i < ta->getNumElements(); ++i)
            attr->SetAttributeValue(draco::AttributeValueIndex(i), ta->getDataPointer(i));
    }

    draco::EncoderBuffer buffer;
    draco::Status status = encoder.EncodeMeshToBuffer(*mesh, &buffer);
    if (status.ok()) out.write(buffer.data(), buffer.size());
    return status.ok();
#else
    OSG_WARN << "[DracoProcessor] Dependency not found" << std::endl;
    return false;
#endif
}
#else  // OSG_VERSION_GREATER_THAN
osg::Geometry* DracoProcessor::decodeDracoData(std::istream& in) { return NULL; }
bool DracoProcessor::decodeDracoData(std::istream& in, osg::Geometry* geom) { return false; }
bool DracoProcessor::encodeDracoData(std::ostream& out, osg::Geometry* geom) { return false; }
#endif  // OSG_VERSION_GREATER_THAN

DracoGeometry::DracoGeometry() : osg::Geometry() {}
DracoGeometry::DracoGeometry(const DracoGeometry& copy, const osg::CopyOp& op)
    : osg::Geometry(copy, op) {}
DracoGeometry::DracoGeometry(const osg::Geometry& copy, const osg::CopyOp& op)
    : osg::Geometry(copy, op) {}

// DracoGeometry Wrappers
struct GeometryFinishedObjectReadCallback : public osgDB::FinishedObjectReadCallback
{
    virtual void objectRead(osgDB::InputStream&, osg::Object& obj)
    {
        osg::Geometry& geometry = static_cast<osg::Geometry&>(obj);
        if (!geometry.getUseVertexBufferObjects())
        {
            geometry.setUseDisplayList(false);
            geometry.setUseVertexBufferObjects(true);
        }
    }
};

static bool checkCompressedData(const osgVerse::DracoGeometry& geom)
{ return true; }

static bool readCompressedData(osgDB::InputStream& is, osgVerse::DracoGeometry& geom)
{
    unsigned int dataSize = 0; is >> dataSize;
    if (dataSize == 0) return true;

    std::vector<char> data(dataSize);
    is.readCharArray(&data[0], dataSize);

    DracoProcessor dp;
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ss.write(&data[0], dataSize);
    return dp.decodeDracoData(ss, &geom);
}

static bool writeCompressedData(osgDB::OutputStream& os, const osgVerse::DracoGeometry& geom)
{
    DracoProcessor dp;
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    if (dp.encodeDracoData(ss, (osg::Geometry*)&geom))
    {
        std::string data = ss.str();
        unsigned int dataSize = data.size();
        os << dataSize; if (dataSize == 0) return true;
        os.writeCharArray(&data[0], dataSize); return true;
    }
    return false;
}

#if OSG_VERSION_GREATER_THAN(3, 4, 1)
REGISTER_OBJECT_WRAPPER(DracoGeometry,
    new osgVerse::DracoGeometry,
    osgVerse::DracoGeometry,  // ignore osg::Geometry to NOT serialize vertices and primitives
    "osg::Object osg::Node osg::Drawable osgVerse::DracoGeometry")
{
    {
        UPDATE_TO_VERSION_SCOPED(154)
        ADDED_ASSOCIATE("osg::Node")
    }
    ADD_USER_SERIALIZER(CompressedData);
    wrapper->addFinishedObjectReadCallback(new GeometryFinishedObjectReadCallback());
}
#else
REGISTER_OBJECT_WRAPPER(DracoGeometry,
    new osgVerse::DracoGeometry,
    osgVerse::DracoGeometry,  // ignore osg::Geometry to NOT serialize vertices and primitives
    "osg::Object osg::Drawable osgVerse::DracoGeometry")
{
    ADD_USER_SERIALIZER(CompressedData);
    wrapper->addFinishedObjectReadCallback(new GeometryFinishedObjectReadCallback());
}
#endif
