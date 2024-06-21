#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include "GeometryMerger.h"
#include "Utilities.h"
using namespace osgVerse;

struct ResetTrianglesOperator
{
    ResetTrianglesOperator() : _start(0) {}
    osg::observer_ptr<osg::DrawElementsUInt> _de;
    unsigned int _start;

    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (i1 == i2 || i2 == i3 || i1 == i3) return;
        _de->push_back(i1 + _start); _de->push_back(i2 + _start); _de->push_back(i3 + _start);
    }
};

GeometryMerger::GeometryMerger()
{}

GeometryMerger::~GeometryMerger()
{}

osg::Geometry* GeometryMerger::process(const std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList,
                                       size_t offset, size_t size, int maxTextureSize)
{
    if (size == 0) size = geomList.size() - offset;
    if (geomList.empty()) return NULL;

    // Collect textures and make atlas
    osg::ref_ptr<TexturePacker> packer = new TexturePacker(4096, 4096);
    std::vector<osg::ref_ptr<osg::Image>> images;
    std::vector<size_t> gIndices;

    std::map<size_t, size_t> geometryIdMap;
    std::string imageName; size_t numImages = 0;
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

    // Recompute texture coords
    osg::ref_ptr<osg::Image> atlas = packer->pack(numImages, true);
    if (!atlas) { packer->setMaxSize(8192, 8192); atlas = packer->pack(numImages, true); }
    if (atlas.valid())
    {
        float totalW = atlas->s(), totalH = atlas->t();
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

        int totalW1 = osg::Image::computeNearestPowerOfTwo(totalW);
        int totalH1 = osg::Image::computeNearestPowerOfTwo(totalH);
        if (totalW1 > (totalW * 1.5)) totalW1 = totalW1 / 2;
        if (totalH1 > (totalH * 1.5)) totalH1 = totalH1 / 2;
        if (totalW1 > maxTextureSize) totalW1 = maxTextureSize;
        if (totalH1 > maxTextureSize) totalH1 = maxTextureSize;
        if (totalW1 != totalW || totalH1 != totalH) atlas->scaleImage(totalW1, totalH1, 1);

        ext = ".jpg";  // packed image should always be saved to JPG at current time...
        atlas->setFileName(imageName + "_all." + ext);
    }

    // Concatenate arrays and primitive-sets
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
    if (taAll->size() == vaAll->size())
        resultGeom->setTexCoordArray(0, taAll.get());
    resultGeom->addPrimitiveSet(de.get());

    if (atlas.valid())
    {
        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setResizeNonPowerOfTwoHint(true); tex2D->setImage(atlas.get());
        resultGeom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex2D.get());
    }
    return resultGeom.release();
}
