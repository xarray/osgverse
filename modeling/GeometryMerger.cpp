#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include "GeometryMerger.h"
#include "Utilities.h"
#include "Octree.h"
using namespace osgVerse;

static void addOctreeNodeToGeometry(const BoundsOctreeNode<osg::Geometry>& node,
                                    osg::Vec3Array& va, osg::Vec4Array& ca, osg::DrawElementsUInt& de)
{
    osg::BoundingBoxd bb; double half = node.baseLength * 0.5;
    bb._min = node.center - osg::Vec3d(half, half, half);
    bb._max = node.center + osg::Vec3d(half, half, half);

    size_t v0 = va.size(); for (int i = 0; i < 8; ++i)
    { va.push_back(bb.corner(i)); ca.push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f)); }
    de.push_back(v0 + 0); de.push_back(v0 + 1); de.push_back(v0 + 1); de.push_back(v0 + 2);
    de.push_back(v0 + 2); de.push_back(v0 + 3); de.push_back(v0 + 3); de.push_back(v0 + 0);
    de.push_back(v0 + 4); de.push_back(v0 + 5); de.push_back(v0 + 5); de.push_back(v0 + 6);
    de.push_back(v0 + 6); de.push_back(v0 + 7); de.push_back(v0 + 7); de.push_back(v0 + 4);
    de.push_back(v0 + 0); de.push_back(v0 + 4); de.push_back(v0 + 1); de.push_back(v0 + 5);
    de.push_back(v0 + 2); de.push_back(v0 + 6); de.push_back(v0 + 3); de.push_back(v0 + 7);

    const std::vector<BoundsOctreeNode<osg::Geometry>>& children = node.getChildren();
    for (size_t i = 0; i < children.size(); ++i)
        addOctreeNodeToGeometry(children[i], va, ca, de);
}

struct ResetTrianglesOperator
{
    ResetTrianglesOperator() : _start(0), _count(0) {}
    osg::observer_ptr<osg::DrawElementsUInt> _de;
    osg::observer_ptr<osg::DrawElementsIndirectUInt> _mde;
    unsigned int _start, _count;

    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (i1 == i2 || i2 == i3 || i1 == i3) return; else _count += 3;
        if (_de.valid())
        { _de->push_back(i1 + _start); _de->push_back(i2 + _start); _de->push_back(i3 + _start); }
        if (_mde.valid())
        { _mde->push_back(i1 + _start); _mde->push_back(i2 + _start); _mde->push_back(i3 + _start); }
    }
};

GeometryMerger::GeometryMerger(Method m)
{ _method = m; }

GeometryMerger::~GeometryMerger()
{}

osg::Geometry* GeometryMerger::process(const std::vector<GeometryPair>& geomList,
                                       size_t offset, size_t size, int maxTextureSize)
{
    if (size == 0) size = geomList.size() - offset;
    if (geomList.empty()) return NULL;

    // Collect textures and make atlas
    osg::ref_ptr<TexturePacker> packer = new TexturePacker(4096, 4096);
    std::vector<osg::ref_ptr<osg::Image>> images;
    std::vector<size_t> gIndices;

    std::map<size_t, size_t> geometryIdMap;
    std::string imageName; int atlasW = 0, atlasH = 0;
    size_t end = osg::minimum(offset + size, geomList.size());
    for (size_t i = offset; i < end; ++i)
    {
        osg::StateSet* ss = geomList[i].first->getStateSet();
        if (!ss) continue; else if (ss->getNumTextureAttributeLists() == 0) continue;

        osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>(
            ss->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
        if (tex && tex->getImage()) { images.push_back(tex->getImage()); gIndices.push_back(i); }
    }

    std::string ext = images.empty() ? "" : osgDB::getFileExtension(images[0]->getFileName());
    for (size_t i = 0; i < images.size(); ++i)
    {
        size_t id = gIndices[i]; geometryIdMap[id] = packer->addElement(images[i].get());
        imageName += osgDB::getStrippedName(images[i]->getFileName()) + ",";
    }
    ext = ".jpg";  // packed image should always be saved to JPG at current time...

    osg::ref_ptr<osg::Image> atlas = createTextureAtlas(
        packer.get(), imageName + "_all." + ext, maxTextureSize, atlasW, atlasH);
    if (atlas.valid())
    {
        // Recompute texture coords
        float totalW = (float)atlasW, totalH = (float)atlasH;
        for (size_t i = offset; i < end; ++i)
        {
            osg::Geometry* geom = geomList[i].first;
            osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));
            if (!ta || geometryIdMap.find(i) == geometryIdMap.end()) continue;

            int x = 0, y = 0, w = 0, h = 0;
            if (!packer->getPackingData(geometryIdMap[i], x, y, w, h)) continue;

            float tx0 = (float)x / totalW, tw = (float)w / totalW;
            float ty0 = (float)y / totalH, th = (float)h / totalH;
            for (size_t j = 0; j < ta->size(); ++j)
            {
                const osg::Vec2& t = (*ta)[j];
                (*ta)[j] = osg::Vec2(t[0] * tw + tx0, t[1] * th + ty0);
            }
        }
    }

    // Concatenate arrays and primitive-sets
    osg::ref_ptr<osg::Geometry> resultGeom;
    switch (_method)
    {
    case INDIRECT_COMMANDS:
        resultGeom = createIndirect(geomList, offset, end); break;
    default:
        resultGeom = createCombined(geomList, offset, end); break;
    }

    if (resultGeom.valid() && atlas.valid())
    {
        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setResizeNonPowerOfTwoHint(true); tex2D->setImage(atlas.get());
        resultGeom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex2D.get());
    }
    return resultGeom.release();
}

osg::Node* GeometryMerger::processAsOctree(const std::vector<GeometryPair>& geomList)
{
    BoundsOctree<osg::Geometry> octree(osg::Vec3d(), 50.0f, 1.0f, 1.0f);
    for (size_t i = 0; i < geomList.size(); ++i)
    {
        osg::Geometry* geom = geomList[i].first;
        const osg::Matrix& matrix = geomList[i].second;

        osg::BoundingBoxd bbox1, bbox0 = geom->getBoundingBox();
        for (int j = 0; j < 8; ++j) bbox1.expandBy(bbox0.corner(j) * matrix);
        octree.add(geom, bbox1);
    }

#if true
    osg::ref_ptr<osg::Geometry> octreeGeom = new osg::Geometry;
    octreeGeom->setUseDisplayList(false);
    octreeGeom->setUseVertexBufferObjects(true);

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_LINES);
    addOctreeNodeToGeometry(octree.getRoot(), *va, *ca, *de);
    octreeGeom->setVertexArray(va.get());
    octreeGeom->setColorArray(ca.get());
    octreeGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    octreeGeom->addPrimitiveSet(de.get());

    osg::ref_ptr<osg::Geode> octreeGeode = new osg::Geode;
    octreeGeode->addDrawable(octreeGeom.get());
    octreeGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    return octreeGeode.release();
#else
    // TODo
#endif
    return NULL;
}

osg::Geometry* GeometryMerger::createCombined(const std::vector<GeometryPair>& geomList,
                                              size_t offset, size_t end)
{
    osg::ref_ptr<osg::Vec3Array> vaAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> naAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> taAll = new osg::Vec2Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);

    osg::ref_ptr<osg::Geometry> resultGeom = new osg::Geometry;
    for (size_t i = offset; i < end; ++i)
    {
        osg::Geometry* geom = geomList[i].first;
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom->getNormalArray());
        osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));
        if (!va || geom->getNumPrimitiveSets() == 0) continue;

        if (!resultGeom->getStateSet() && geom->getStateSet() != NULL)
        {
            resultGeom->setStateSet(static_cast<osg::StateSet*>(
                geom->getStateSet()->clone(osg::CopyOp::DEEP_COPY_ALL)));
        }

        osg::TriangleIndexFunctor<ResetTrianglesOperator> functor;
        functor._de = de.get(); functor._start = vaAll->size();
        geom->accept(functor);

        osg::Matrix matrix = geomList[i].second;
        for (size_t v = 0; v < va->size(); ++v) vaAll->push_back((*va)[v] * matrix);
        if (na) naAll->insert(naAll->end(), na->begin(), na->end());
        if (ta) taAll->insert(taAll->end(), ta->begin(), ta->end());
    }

    resultGeom->setUseDisplayList(false);
    resultGeom->setUseVertexBufferObjects(true);
    resultGeom->setVertexArray(vaAll.get());
    if (naAll->size() == vaAll->size())
    {
        resultGeom->setNormalArray(naAll.get());
        resultGeom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (taAll->size() == vaAll->size()) resultGeom->setTexCoordArray(0, taAll.get());
    resultGeom->addPrimitiveSet(de.get());
    return resultGeom.release();
}

osg::Geometry* GeometryMerger::createIndirect(const std::vector<GeometryPair>& geomList,
                                              size_t offset, size_t end)
{
    osg::ref_ptr<osg::Vec3Array> vaAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> naAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> taAll = new osg::Vec2Array;
    osg::ref_ptr<osg::MultiDrawElementsIndirectUInt> mde = new osg::MultiDrawElementsIndirectUInt(GL_TRIANGLES);
    osg::ref_ptr<IndirectCommandDrawElements> icde = new IndirectCommandDrawElements;
    mde->setIndirectCommandArray(icde.get());

    osg::ref_ptr<osg::Geometry> resultGeom = new osg::Geometry; osg::BoundingBoxd bbox;
    for (size_t i = offset; i < end; ++i)
    {
        osg::Geometry* geom = geomList[i].first;
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom->getNormalArray());
        osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));
        if (!va || geom->getNumPrimitiveSets() == 0) continue;

        if (!resultGeom->getStateSet() && geom->getStateSet() != NULL)
        {
            resultGeom->setStateSet(static_cast<osg::StateSet*>(
                geom->getStateSet()->clone(osg::CopyOp::DEEP_COPY_ALL)));
        }

        osg::DrawElementsIndirectCommand cmd;
        cmd.firstIndex = mde->size();
        cmd.baseVertex = vaAll->size();

        osg::TriangleIndexFunctor<ResetTrianglesOperator> functor;
        functor._mde = mde.get(); geom->accept(functor);
        cmd.count = functor._count; cmd.instanceCount = 1;
        icde->push_back(cmd); icde->pushUserData(geom->getUserDataContainer());

        osg::Matrix matrix = geomList[i].second;
        for (size_t v = 0; v < va->size(); ++v)
        {
            vaAll->push_back((*va)[v] * matrix);
            bbox.expandBy(vaAll->back());
        }
        if (na) naAll->insert(naAll->end(), na->begin(), na->end());
        if (ta) taAll->insert(taAll->end(), ta->begin(), ta->end());
    }

    resultGeom->setInitialBound(bbox);
    resultGeom->setUseDisplayList(false);
    resultGeom->setUseVertexBufferObjects(true);
    resultGeom->setVertexArray(vaAll.get());
    if (naAll->size() == vaAll->size())
    {
        resultGeom->setNormalArray(naAll.get());
        resultGeom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (taAll->size() == vaAll->size()) resultGeom->setTexCoordArray(0, taAll.get());
    resultGeom->addPrimitiveSet(mde.get());
    return resultGeom.release();
}

osg::Image* GeometryMerger::createTextureAtlas(TexturePacker* packer, const std::string& fileName,
                                               int maxTextureSize, int& originW, int& originH)
{
    size_t numImages = 0;
    osg::ref_ptr<osg::Image> atlas = packer->pack(numImages, true);
    if (!atlas) { packer->setMaxSize(8192, 8192); atlas = packer->pack(numImages, true); }
    if (atlas.valid())
    {
        originW = atlas->s(); originH = atlas->t();
        int totalW1 = osg::Image::computeNearestPowerOfTwo(originW);
        int totalH1 = osg::Image::computeNearestPowerOfTwo(originH);
        if (totalW1 > (originW * 1.5)) totalW1 = totalW1 / 2;
        if (totalH1 > (originH * 1.5)) totalH1 = totalH1 / 2;
        if (totalW1 > maxTextureSize) totalW1 = maxTextureSize;
        if (totalH1 > maxTextureSize) totalH1 = maxTextureSize;
        if (totalW1 != originW || totalH1 != originH) atlas->scaleImage(totalW1, totalH1, 1);
        atlas->setFileName(fileName);
    }
    return atlas.release();
}
