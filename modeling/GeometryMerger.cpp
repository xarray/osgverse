#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osgDB/FileNameUtils>
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

osg::Geometry* GeometryMerger::process(const std::vector<osg::Geometry*>& geomList)
{
    if (geomList.empty()) return NULL;
    else if (geomList.size() == 1) return geomList[0];

    // Collect textures and make atlas
    osg::ref_ptr<TexturePacker> packer = new TexturePacker(4096, 4096);
    std::map<size_t, size_t> geometryIdMap;
    std::string imageName; size_t numImages = 0;
    for (size_t i = 0; i < geomList.size(); ++i)
    {
        osg::StateSet* ss = geomList[i]->getStateSet();
        if (!ss) continue;

        osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>(
            ss->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
        if (tex && tex->getImage())
        {
            geometryIdMap[i] = packer->addElement(tex->getImage());
            imageName = tex->getImage()->getFileName();
        }
    }

    // Recompute texture coords
    osg::ref_ptr<osg::Image> atlas = packer->pack(numImages, true);
    if (atlas.valid())
    {
        float totalW = atlas->s(), totalH = atlas->t();
        for (size_t i = 0; i < geomList.size(); ++i)
        {
            osg::Geometry* geom = geomList[i];
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
        if (totalW1 != totalW || totalH1 != totalH) atlas->scaleImage(totalW1, totalH1, 1);

        size_t index0 = imageName.find("_L");
        size_t index1 = imageName.find("_", index0 + 2);
        std::string ext = osgDB::getFileExtension(imageName);
        if (index0 == std::string::npos || index1 == std::string::npos)
            index1 = imageName.find_last_of('_');
        atlas->setFileName(imageName.substr(0, index1) + "_all." + ext);

    }

    // Concatenate arrays and primitive-sets
    osg::ref_ptr<osg::Vec3Array> vaAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> naAll = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> taAll = new osg::Vec2Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (size_t i = 0; i < geomList.size(); ++i)
    {
        osg::Geometry* geom = geomList[i];
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(geom->getNormalArray());
        osg::Vec2Array* ta = static_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));
        if (!va || geom->getNumPrimitiveSets() == 0) continue;

        osg::TriangleIndexFunctor<ResetTrianglesOperator> functor;
        functor._de = de.get(); functor._start = vaAll->size();
        geom->accept(functor);

        vaAll->insert(vaAll->end(), va->begin(), va->end());
        if (na) naAll->insert(naAll->end(), na->begin(), na->end());
        if (ta) taAll->insert(taAll->end(), ta->begin(), ta->end());
    }

    osg::ref_ptr<osg::Geometry> resultGeom = new osg::Geometry;
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
