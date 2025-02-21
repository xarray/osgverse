#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/Camera>
#include <osgUtil/Simplifier>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include "GeometryMerger.h"
#include "Utilities.h"
#include "Octree.h"
using namespace osgVerse;

struct GeometryMergeData : public osg::Referenced
{
    osg::ref_ptr<osg::Geometry> geometry;
    osg::Matrix matrix;
};

#if OSG_VERSION_GREATER_THAN(3, 4, 1)
template<typename CLS, typename FUNC>
static void acceptMultDrawElementsIndirect(CLS* drawer, FUNC& functor)
{
    osg::IndirectCommandDrawElements* icd =
        const_cast<osg::IndirectCommandDrawElements*>(drawer->getIndirectCommandArray());
    unsigned int firsCmd = drawer->getFirstCommandToDraw(), count = drawer->getNumCommandsToDraw();
    unsigned int maxIndex = (count > 0) ? (firsCmd + count) : (icd->getNumElements() - firsCmd);
    for (unsigned int i = firsCmd; i < maxIndex; ++i)
    {
        unsigned int baseVertex = icd->baseVertex(i);
        unsigned int firstIndex = icd->firstIndex(i), count = icd->count(i);
        typename CLS::vector_type::pointer ptr = (typename CLS::vector_type::pointer)&(*drawer)[firstIndex];

        std::vector<typename CLS::value_type> tmpIndices(count);
        for (unsigned int n = 0; n < count; ++n) tmpIndices[n] = *(ptr + n) + baseVertex;
        functor.drawElements(drawer->getMode(), count, &tmpIndices[0]);
    }
}

#define MULTI_DRAW_ELEMENTS_INDIRECT_ACCEPT(t) \
    void MultiDrawElementsIndirect##t ::accept(osg::PrimitiveFunctor& functor) const \
    { if (!empty()) acceptMultDrawElementsIndirect(this, functor); } \
    void MultiDrawElementsIndirect##t ::accept(osg::PrimitiveIndexFunctor& functor) const \
    { if (!empty()) acceptMultDrawElementsIndirect(this, functor); }
MULTI_DRAW_ELEMENTS_INDIRECT_ACCEPT(UByte)
MULTI_DRAW_ELEMENTS_INDIRECT_ACCEPT(UShort)
MULTI_DRAW_ELEMENTS_INDIRECT_ACCEPT(UInt)
#endif

static void addOctreeNodeToGeometry(const BoundsOctreeNode<GeometryMergeData>& node,
                                    osg::Vec3Array& va, osg::Vec4Array& ca, osg::DrawElementsUInt& de)
{
    if (node.hasAnyObjects())
    {
        osg::BoundingBoxd bb; double half = node.baseLength * 0.5;
        bb._min = node.center - osg::Vec3d(half, half, half);
        bb._max = node.center + osg::Vec3d(half, half, half);

        int maxNum = node.getNumObjectsAllowed();
        size_t v0 = va.size(), numObj = node.getObjects().size();
        osg::Vec4 color(1.0f, 0.0f, 0.0f, 0.4f);
        if (numObj < 1) color.set(0.0f, 0.0f, 0.0f, 0.4f);
        else if (numObj < 2) color.set(0.0f, 0.0f, 0.6f, 0.4f);
        else if (numObj < maxNum / 2) color.set(0.0f, 0.6f, 0.0f, 0.4f);
        else if (numObj < maxNum) color.set(0.6f, 0.6f, 0.0f, 0.4f);

        for (int i = 0; i < 8; ++i) { va.push_back(bb.corner(i)); ca.push_back(color); }
        de.push_back(v0 + 0); de.push_back(v0 + 1); de.push_back(v0 + 1); de.push_back(v0 + 3);
        de.push_back(v0 + 3); de.push_back(v0 + 2); de.push_back(v0 + 2); de.push_back(v0 + 0);
        de.push_back(v0 + 4); de.push_back(v0 + 5); de.push_back(v0 + 5); de.push_back(v0 + 7);
        de.push_back(v0 + 7); de.push_back(v0 + 6); de.push_back(v0 + 6); de.push_back(v0 + 4);
        de.push_back(v0 + 0); de.push_back(v0 + 4); de.push_back(v0 + 1); de.push_back(v0 + 5);
        de.push_back(v0 + 3); de.push_back(v0 + 7); de.push_back(v0 + 2); de.push_back(v0 + 6);
    }

    const std::vector<BoundsOctreeNode<GeometryMergeData>>& children = node.getChildren();
    for (size_t i = 0; i < children.size(); ++i)
        addOctreeNodeToGeometry(children[i], va, ca, de);
}

static void applyOctreeNode(GeometryMerger* merger, osg::Group* group,
                            const BoundsOctreeNode<GeometryMergeData>& node)
{
    const std::vector<BoundsOctreeNode<GeometryMergeData>>& children = node.getChildren();
    osg::ref_ptr<osg::Group> fineGroup = new osg::Group;
    for (size_t i = 0; i < children.size(); ++i)
    {
        const BoundsOctreeNode<GeometryMergeData>& child = children[i];
        if (!child.getChildren().empty())
        {
            osg::ref_ptr<osg::LOD> childLOD = new osg::LOD;
            childLOD->setCenterMode(osg::LOD::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
            childLOD->setCenter(child.center); childLOD->setRadius(child.baseLength * 0.5);
            applyOctreeNode(merger, childLOD.get(), child);
            fineGroup->addChild(childLOD.get());
            
        }
        else if (child.hasAnyObjects())
        {
            osg::ref_ptr<osg::Group> childGroup = new osg::Group;
            applyOctreeNode(merger, childGroup.get(), child);
            fineGroup->addChild(childGroup.get());
        }
    }

    // Create rough level child
    std::vector<BoundsOctreeNode<GeometryMergeData>::OctreeObject> objects = node.getObjects();
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    if (!objects.empty())
    {
        std::vector<GeometryMerger::GeometryPair> geomList;
        for (size_t i = 0; i < objects.size(); ++i) geomList.push_back(
            GeometryMerger::GeometryPair(objects[i].object->geometry, objects[i].object->matrix));
        osg::ref_ptr<osg::Geometry> roughGeom = merger->process(geomList, 0);
        geode->addDrawable(roughGeom.get());

        if (merger->getSimplifierRatio() > 0.0f)
        {
            osgUtil::Simplifier simplifier(merger->getSimplifierRatio());
            geode->accept(simplifier);
        }
    }

    // Add LOD/group children
    osg::LOD* lodGroup = dynamic_cast<osg::LOD*>(group);
    if (lodGroup)
    {
        if (geode->getNumDrawables() > 0) lodGroup->addChild(geode.get(), 0.0f, FLT_MAX);
        lodGroup->addChild(fineGroup.get(), 0.0f, node.baseLength * 5.0f);
    }
    else
    {
        if (geode->getNumDrawables() > 0) group->addChild(geode.get());
        if (fineGroup->getNumChildren() > 0) group->addChild(fineGroup.get());
    }
}

struct ResetTrianglesOperator
{
    ResetTrianglesOperator() : _start(0), _count(0) {}
    osg::observer_ptr<osg::DrawElementsUInt> _de;
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    osg::observer_ptr<osg::DrawElementsIndirectUInt> _mde;
#endif
    unsigned int _start, _count;

    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (i1 == i2 || i2 == i3 || i1 == i3) return; else _count += 3;
        if (_de.valid())
        { _de->push_back(i1 + _start); _de->push_back(i2 + _start); _de->push_back(i3 + _start); }
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        if (_mde.valid())
        { _mde->push_back(i1 + _start); _mde->push_back(i2 + _start); _mde->push_back(i3 + _start); }
#endif
    }
};

GeometryMerger::GeometryMerger(Method m, GpuBaker* baker)
{
    _method = m; _baker = baker;
    _autoSimplifierRatio = 0.0f; _forceColorArray = false;
}

GeometryMerger::~GeometryMerger()
{}

osg::Image* GeometryMerger::processAtlas(const std::vector<GeometryPair>& geomList,
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
    if (images.empty()) return NULL;

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
    return atlas.release();
}

osg::Geometry* GeometryMerger::process(const std::vector<GeometryPair>& geomList,
                                       size_t offset, size_t size, int maxTextureSize)
{
    if (size == 0) size = geomList.size() - offset;
    size_t end = osg::minimum(offset + size, geomList.size());
    osg::ref_ptr<osg::Image> atlas = processAtlas(geomList, offset, size, maxTextureSize);

    osg::ref_ptr<osg::Geometry> resultGeom;
    switch (_method)
    {
    case INDIRECT_COMMANDS:
        resultGeom = createIndirect(geomList, offset, end); break;
    case GPU_BAKING:
        resultGeom = createGpuBaking(geomList, offset, end); break;
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

osg::Node* GeometryMerger::processAsOctree(const std::vector<GeometryPair>& geomList,
                                           size_t offset, size_t size, int maxTextureSize,
                                           osg::Geode* octRoot, int numAllowed, float minSizeInCell)
{
    BoundsOctree<GeometryMergeData> octree(
        osg::Vec3d(), osg::maximum(50.0f, minSizeInCell), minSizeInCell, 1.0f, numAllowed);
    if (size == 0) size = geomList.size() - offset;
    size_t end = osg::minimum(offset + size, geomList.size());
    for (size_t i = offset; i < end; ++i)
    {
        GeometryMergeData* gd = new GeometryMergeData;
        gd->geometry = geomList[i].first;
        gd->matrix = geomList[i].second;

#if OSG_VERSION_GREATER_THAN(3, 2, 3)
        osg::BoundingBoxd bbox1, bbox0 = gd->geometry->getBoundingBox();
#else
        osg::BoundingBoxd bbox1, bbox0 = osg::BoundingBoxd(
            gd->geometry->getBound()._min, gd->geometry->getBound()._max);
#endif
        for (int j = 0; j < 8; ++j) bbox1.expandBy(bbox0.corner(j) * gd->matrix);
        octree.add(gd, bbox1);
    }

    if (octRoot != NULL)
    {
        osg::ref_ptr<osg::Geometry> octreeGeom = new osg::Geometry;
        octreeGeom->setName("GeometryMerger.Octree");
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
        octRoot->addDrawable(octreeGeom.get());
    }

    osg::ref_ptr<osg::LOD> root = new osg::LOD;
    root->setCenterMode(osg::LOD::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
    root->setCenter(octree.getRoot().center);
    root->setRadius(octree.getRoot().baseLength * 0.5);
    applyOctreeNode(this, root.get(), octree.getRoot());

    osg::ref_ptr<osg::Image> atlas = processAtlas(geomList, offset, size, maxTextureSize);
    if (root.valid() && atlas.valid())
    {
        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setResizeNonPowerOfTwoHint(true); tex2D->setImage(atlas.get());
        root->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex2D.get());
    }
    return root.release();
}

osg::Geometry* GeometryMerger::createCombined(const std::vector<GeometryPair>& geomList,
                                              size_t offset, size_t end)
{
    osg::ref_ptr<osg::Vec3Array> vaAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> naAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> caAll = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec2Array> taAll = new osg::Vec2Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);

    osg::ref_ptr<osg::Geometry> resultGeom = new osg::Geometry;
    for (size_t i = offset; i < end; ++i)
    {
        osg::Geometry* geom = geomList[i].first;
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom->getNormalArray());
        osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(geom->getColorArray());
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

        osg::Matrix matrix = geomList[i].second; size_t vS = va->size();
        for (size_t v = 0; v < va->size(); ++v) vaAll->push_back((*va)[v] * matrix);
        if (ta) taAll->insert(taAll->end(), ta->begin(), ta->end());
        if (na)
        {
            if (na->size() < vS) naAll->insert(naAll->end(), vS, na->front());
            else naAll->insert(naAll->end(), na->begin(), na->end());
        }
        if (ca)
        {
            if (ca->size() < vS) caAll->insert(caAll->end(), vS, ca->front());
            else caAll->insert(caAll->end(), ca->begin(), ca->end());
        }
        else if (_forceColorArray)
            caAll->insert(caAll->end(), vS, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    resultGeom->setUseDisplayList(false);
    resultGeom->setUseVertexBufferObjects(true);
    resultGeom->setVertexArray(vaAll.get());
    if (naAll->size() == vaAll->size())
    {
        resultGeom->setNormalArray(naAll.get());
        resultGeom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (caAll->size() == vaAll->size())
    {
        resultGeom->setColorArray(caAll.get());
        resultGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (taAll->size() == vaAll->size()) resultGeom->setTexCoordArray(0, taAll.get());
    resultGeom->addPrimitiveSet(de.get());
    return resultGeom.release();
}

osg::Geometry* GeometryMerger::createIndirect(const std::vector<GeometryPair>& geomList,
                                              size_t offset, size_t end)
{
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    osg::ref_ptr<osg::Vec3Array> vaAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> naAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> caAll = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec2Array> taAll = new osg::Vec2Array;
    osg::ref_ptr<MultiDrawElementsIndirectUInt> mde = new MultiDrawElementsIndirectUInt(GL_TRIANGLES);
    osg::ref_ptr<IndirectCommandDrawElements> icde = new IndirectCommandDrawElements;
    mde->setIndirectCommandArray(icde.get());

    osg::ref_ptr<osg::Geometry> resultGeom = new osg::Geometry; osg::BoundingBoxd bbox;
    for (size_t i = offset; i < end; ++i)
    {
        osg::Geometry* geom = geomList[i].first;
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom->getNormalArray());
        osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(geom->getColorArray());
        osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));
        if (!va || geom->getNumPrimitiveSets() == 0) continue;

        osg::ref_ptr<osg::UserDataContainer> udc = geom->getUserDataContainer();
        if (!udc && !geom->getName().empty()) udc = new osg::DefaultUserDataContainer;
        if (udc.valid()) udc->setName(geom->getName());

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
        icde->push_back(cmd); icde->pushUserData(udc.get());

        osg::Matrix matrix = geomList[i].second; size_t vS = va->size();
        for (size_t v = 0; v < vS; ++v)
        {
            vaAll->push_back((*va)[v] * matrix);
            bbox.expandBy(vaAll->back());
        }

        if (ta) taAll->insert(taAll->end(), ta->begin(), ta->end());
        if (na)
        {
            if (na->size() < vS) naAll->insert(naAll->end(), vS, na->front());
            else naAll->insert(naAll->end(), na->begin(), na->end());
        }
        if (ca)
        {
            if (ca->size() < vS) caAll->insert(caAll->end(), vS, ca->front());
            else caAll->insert(caAll->end(), ca->begin(), ca->end());
        }
        else if (_forceColorArray)
            caAll->insert(caAll->end(), vS, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
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
    if (caAll->size() == vaAll->size())
    {
        resultGeom->setColorArray(caAll.get());
        resultGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    if (taAll->size() == vaAll->size()) resultGeom->setTexCoordArray(0, taAll.get());
    resultGeom->addPrimitiveSet(mde.get());
    return resultGeom.release();
#else
    OSG_FATAL << "[GeometryMerger] createIndirect() not supported" << std::endl;
    return NULL;
#endif
}

osg::Geometry* GeometryMerger::createGpuBaking(const std::vector<GeometryPair>& geomList,
                                               size_t offset, size_t end)
{
    if (!_baker)
    {
        OSG_FATAL << "[GeometryMerger] Please call setBaker() before"
                  << " using GPU baking to merge geometries" << std::endl;
        return NULL;
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    for (size_t i = offset; i < end; ++i)
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geomList[i].first);

        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setMatrix(geomList[i].second); mt->addChild(geode.get());
        root->addChild(mt.get());
    }

    osg::ref_ptr<osg::Geometry> geom = _baker->bakeGeometry(root.get());
    if (geom.valid())
    {
        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        tex2D->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        tex2D->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

        osg::ref_ptr<osg::Image> image = _baker->bakeTextureImage(root.get());
        if (image.valid()) tex2D->setImage(image.get());
        geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex2D.get());
    }
    return geom.release();
}

osg::Image* GeometryMerger::createTextureAtlas(TexturePacker* packer, const std::string& fileName,
                                               int maxTextureSize, int& originW, int& originH)
{
    size_t numImages = 0; osg::ref_ptr<osg::Image> atlas = packer->pack(numImages, true);
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
