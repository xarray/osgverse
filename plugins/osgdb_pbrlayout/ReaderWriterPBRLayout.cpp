#include <osg/Version>
#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <pipeline/Global.h>

class TexLayoutVisitor : public osg::NodeVisitor
{
public:
    TexLayoutVisitor(const osgDB::StringList& params)
    : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) { parse(params); }

    virtual void apply(osg::Drawable& drawable)
    {
        if (drawable.getStateSet()) applyStateSet(*(drawable.getStateSet()));
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        traverse(drawable);
#endif
    }
    
    virtual void apply(osg::Node& node)
    {
        if (node.getStateSet()) applyStateSet(*node.getStateSet());
        traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Drawable* d = node.getDrawable(i);
            if (d) apply(*d);
        }
#endif
        if (node.getStateSet()) applyStateSet(*node.getStateSet());
        traverse(node);
    }

    void applyStateSet(osg::StateSet& ss)
    {
        typedef std::pair<osg::ref_ptr<osg::Texture2D>, osg::Vec3i> TextureAndRange;
        std::map<PbrType, TextureAndRange> sourceTexMap;

        // Find current textures
        const osg::StateSet::TextureAttributeList& texList = ss.getTextureAttributeList();
        for (size_t u = 0; u < texList.size(); ++u)
        {
            std::vector<TypeAndComponent>& typeDataList = _sourceMap[u];
            const osg::StateSet::AttributeList& attrList = texList[u];
            if (typeDataList.empty()) continue;

            int componentStart = 0;
            for (osg::StateSet::AttributeList::const_iterator itr = attrList.begin();
                 itr != attrList.end(); ++itr)
            {
                osg::StateAttribute::Type type = itr->first.first;
                if (type != osg::StateAttribute::TEXTURE) continue;

                osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(itr->second.first.get());
                if (tex2D && tex2D->getImage())
                {
                    for (size_t i = 0; i < typeDataList.size(); ++i)
                    {
                        TypeAndComponent& typeAndComp = typeDataList[i];
                        osg::Vec3i range(componentStart, typeAndComp.second, u);
                        sourceTexMap[typeAndComp.first] = TextureAndRange(tex2D, range);
                        componentStart += typeAndComp.second;
                    }
                }
            }
        }

        // Remove old textures from state-set
        for (std::map<PbrType, TextureAndRange>::iterator itr = sourceTexMap.begin();
            itr != sourceTexMap.end(); ++itr)
        {
            osg::Texture2D* tex2D = itr->second.first;
            osg::Vec3i range = itr->second.second;
            ss.removeTextureAttribute(range[2], tex2D);
        }
        
        // Re-apply textures in D4,N3,S4,O1R1M1,A3,E3 layout
        osg::ref_ptr<osg::Image> ormImage = new osg::Image;
        for (std::map<PbrType, TextureAndRange>::iterator itr = sourceTexMap.begin();
             itr != sourceTexMap.end(); ++itr)
        {
            osg::Texture2D* tex2D = itr->second.first.get();
            osg::Vec3i range = itr->second.second;
            OSG_INFO << "[TexLayoutVisitor] Ready to change tex from unit-" << range[2]
                     << " to PBR channel " << (char)itr->first << std::endl;

            switch (itr->first)
            {
            case 'D': ss.setTextureAttributeAndModes(0, getTexture(tex2D, range)); break;
            case 'N': ss.setTextureAttributeAndModes(1, getTexture(tex2D, range)); break;
            case 'S': ss.setTextureAttributeAndModes(2, getTexture(tex2D, range)); break;
            case 'A': ss.setTextureAttributeAndModes(4, getTexture(tex2D, range)); break;
            case 'E': ss.setTextureAttributeAndModes(5, getTexture(tex2D, range)); break;
            case 'O': applyToImageORM(ormImage.get(), 0, tex2D, range); break;
            case 'R': applyToImageORM(ormImage.get(), 1, tex2D, range); break;
            case 'M': applyToImageORM(ormImage.get(), 2, tex2D, range); break;
            default: break;
            }
        }

        if (ormImage->valid())
        {
            osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
            tex2D->setImage(ormImage.get());
            tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
            tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
            tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
            tex2D->setResizeNonPowerOfTwoHint(false);
            ss.setTextureAttributeAndModes(3, tex2D.get());
        }
    }

    osg::Texture2D* getTexture(osg::Texture2D* tex, const osg::Vec3i& range)
    {
        osg::Image* srcImage = tex->getImage();
        int numComp = osg::Image::computeNumComponents(srcImage->getPixelFormat());
        if (range[0] == 0 && range[1] >= numComp) return tex;

        osg::ref_ptr<osg::Image> dstImage = new osg::Image;
        dstImage->allocateImage(srcImage->s(), srcImage->t(), 1,
                                srcImage->getPixelFormat(), srcImage->getDataType());
        dstImage->setInternalTextureFormat(srcImage->getInternalTextureFormat());

        int r0 = range[0], r1 = osg::minimum(range[1] + r0, numComp);
        unsigned char *src = srcImage->data(), *dst = dstImage->data();
        memset(dst, 255, dstImage->getTotalSizeInBytes());
        for (int t = 0; t < dstImage->t(); ++t)
            for (int s = 0; s < dstImage->s(); ++s)
            {
                int idx = (s + t * dstImage->s()) * numComp;
                for (int i = r0; i < r1; ++i) *(dst + idx + (i - r0)) = *(src + idx + i);
            }

        osg::ref_ptr<osg::Texture2D> tex2D = static_cast<osg::Texture2D*>(
            tex->clone(osg::CopyOp::SHALLOW_COPY));
        tex2D->setImage(dstImage.get());
        return tex2D.release();
    }

    void applyToImageORM(osg::Image* image, int c, osg::Texture2D* tex, const osg::Vec3i& range)
    {
        osg::Image* srcImage = tex->getImage();
        if (!image->valid())
        {
            image->allocateImage(srcImage->s(), srcImage->t(), 1, GL_RGB, GL_FLOAT);
            image->setInternalTextureFormat(GL_RGB32F_ARB);
            memset(image->data(), 0, image->getTotalSizeInBytes());
        }

        int numComp = osg::Image::computeNumComponents(srcImage->getPixelFormat());
        int r0 = range[0], r1 = osg::minimum(range[1] + r0, numComp);
        unsigned char *src = srcImage->data(), *dst = image->data();
        for (int t = 0; t < srcImage->t(); ++t)
            for (int s = 0; s < srcImage->s(); ++s)
            {
                int idx = (s + t * srcImage->s()) * numComp;
                *(dst + idx + c) = *(src + idx + r0);  // FIXME: only consider 1 component
            }
    }

protected:
    enum PbrType
    {
        DiffuseType = 'D', SpecularType = 'S', NormalType = 'N',
        MetallicType = 'M', RoughnessType = 'R', OcclusionType = 'O',
        EmissiveType = 'E', AmbientType = 'A', OmittedType = 'X'
    };
    typedef std::pair<PbrType, int> TypeAndComponent;
    std::map<int, std::vector<TypeAndComponent>> _sourceMap;

    void parse(const osgDB::StringList& params)
    {
        _sourceMap.clear();
        for (size_t u = 0; u < params.size(); ++u)
        {
            std::string p = trim(params[u]);
            int maxComponents = 4;

            for (size_t j = 0; j < p.length(); j += 2)
            {
                PbrType type = (PbrType)p[j];
                int num = (int)(p[j + 1] - '0');
                if (num <= 0 || num > 4) continue;

                _sourceMap[u].push_back(TypeAndComponent(type, num));
                maxComponents -= num; if (maxComponents <= 0) break;
            }
        }
    }

    static std::string trim(const std::string& str)
    {
        if (!str.size()) return str;
        std::string::size_type first = str.find_first_not_of(" \t");
        std::string::size_type last = str.find_last_not_of("  \t\r\n");
        if ((first == str.npos) || (last == str.npos)) return std::string("");
        return str.substr(first, last - first + 1);
    }
};

class ReaderWriterPBRLayout : public osgDB::ReaderWriter
{
public:
    ReaderWriterPBRLayout()
    {
        supportsExtension("pbrlayout", "PBR texture layout pseudo-loader");
    }

    virtual const char* className() const
    {
        return "[osgVerse] PBR texture layout pseudo-loader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        std::string tmpName = osgDB::getNameLessExtension(path);
        std::size_t index = tmpName.find_last_of('.');
        if (index == std::string::npos) return osgDB::readNodeFile(tmpName, options);

        std::string fileName = tmpName.substr(0, index);
        std::string params = (index < tmpName.size() - 1) ? tmpName.substr(index + 1) : "";
        osg::ref_ptr<osg::Node> node = osgDB::readRefNodeFile(fileName, options);
        if (!node) return ReadResult::FILE_NOT_FOUND;

        osgDB::StringList texParams; osgDB::split(params, texParams, ',');
        TexLayoutVisitor tlv(texParams); node->accept(tlv);
        return node.get();
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(pbrlayout, ReaderWriterPBRLayout)
