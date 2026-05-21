#include <osg/io_utils>
#include <osg/Version>
#include <osg/BlendFunc>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#define TINYOBJLOADER_IMPLEMENTATION
#include "3rdparty/tiny_obj_loader.h"
#include "modeling/Utilities.h"
#include <unordered_map>

class ReaderWriterMesh : public osgDB::ReaderWriter
{
public:
    ReaderWriterMesh()
    {
        supportsExtension("verse_mesh", "osgVerse pseudo-loader");
        supportsExtension("obj", "OBJ format");
        supportsExtension("stl", "STL format");
    }

    virtual const char* className() const
    {
        return "[osgVerse] Common mesh format reader";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->getDatabasePathList().push_front(osgDB::getFilePath(fileName));
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));
        
        ReadResult rr = readNode(in, lOptions);
        lOptions->getDatabasePathList().pop_front(); return rr;
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        std::string filename, dir("."), ext("obj");
        if (options)
        {
            if (!options->getDatabasePathList().empty())
                dir = options->getDatabasePathList().front();
            filename = options->getPluginStringData("STREAM_FILENAME");
            ext = osgDB::getLowerCaseFileExtension(filename);
        }

        if (ext == "stl")
        {
            // TODO
        }
        else
            return readSceneOBJ(fin, dir, options);
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_mesh");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    osg::Node* readSceneOBJ(std::istream& fin, const std::string& dir, const Options* options) const
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        tinyobj::MaterialFileReader mtlReader(dir);
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                                    &fin, &mtlReader, true);
        if (!err.empty()) OSG_WARN << "[ReaderWriterMesh] read OBJ failed: " << err << "\n";
        if (!warn.empty()) OSG_NOTICE << "[ReaderWriterMesh] read OBJ warning: " << warn << "\n";
        if (!ret) return NULL; else if (shapes.empty()) return new osg::Node;

        std::unordered_map<int, osg::ref_ptr<osg::StateSet>> matCache;
        std::unordered_map<int, osg::ref_ptr<osg::Geode>> matGeodes;
        for (size_t i = 0; i < materials.size(); ++i)
            matCache[static_cast<int>(i)] = createMaterialOBJ(materials[i], dir);

        osg::ref_ptr<osg::Group> root = new osg::Group;
        for (int i = 0, matID = 0; i < (int)shapes.size(); ++i)
        {
            osg::ref_ptr<osg::Geometry> geom = createShapeOBJ(attrib, shapes[i], matID);
            osg::ref_ptr<osg::StateSet> ss = (matID >= 0 && matCache.count(matID))
                                            ? matCache[matID] : NULL;
            if (!geom) continue; else if (ss.valid()) geom->setStateSet(ss.get());

            if (!matGeodes.count(matID)) matGeodes[matID] = new osg::Geode;
            matGeodes[matID]->addDrawable(geom.get());
        }

        for (std::unordered_map<int, osg::ref_ptr<osg::Geode>>::iterator it = matGeodes.begin();
             it != matGeodes.end(); ++it) root->addChild(it->second.get());
        return root.release();
    }

    osg::Geometry* createShapeOBJ(const tinyobj::attrib_t& attrib,
                                  const tinyobj::shape_t& shape, int& matID) const
    {
        osg::Vec3Array* vertices = new osg::Vec3Array;
        osg::Vec3Array* normals = new osg::Vec3Array;
        osg::Vec2Array* texcoords = new osg::Vec2Array;
        bool hasNormals = !attrib.normals.empty();
        bool hasTexcoords = !attrib.texcoords.empty();
        for (const auto& idx : shape.mesh.indices)
        {
            osg::Vec3 v(attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]);
            vertices->push_back(v);

            if (hasNormals && idx.normal_index >= 0)
            {
                osg::Vec3 n(attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]);
                normals->push_back(n);
            }
            else
                normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));

            if (hasTexcoords && idx.texcoord_index >= 0)
            {
                texcoords->push_back(osg::Vec2(
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    attrib.texcoords[2 * idx.texcoord_index + 1]));
            }
            else
                texcoords->push_back(osg::Vec2(0.0f, 0.0f));
        }
        
        osg::DrawArrays* p = new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size());
        if (!shape.mesh.material_ids.empty()) matID = shape.mesh.material_ids[0];
        if (hasTexcoords) return osgVerse::createGeometry(vertices, normals, texcoords, p, !hasNormals);
        else return osgVerse::createGeometry(vertices, normals, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), p, !hasNormals);
    }

    osg::StateSet* createMaterialOBJ(const tinyobj::material_t& mat, const std::string& dir) const
    {
        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setAmbient(osg::Material::FRONT_AND_BACK,
                             osg::Vec4(mat.ambient[0], mat.ambient[1], mat.ambient[2], 1.0f));
        material->setDiffuse(osg::Material::FRONT_AND_BACK,
                             osg::Vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f));
        material->setSpecular(osg::Material::FRONT_AND_BACK,
                              osg::Vec4(mat.specular[0], mat.specular[1], mat.specular[2], 1.0f));
        material->setShininess(osg::Material::FRONT_AND_BACK, mat.shininess);

        osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
        ss->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
        if (mat.dissolve < 1.0f)
        {
            ss->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
            ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            material->setDiffuse(osg::Material::FRONT_AND_BACK,
                                 osg::Vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], mat.dissolve));
        }

        if (!mat.diffuse_texname.empty())
            ss->setTextureAttributeAndModes(0, createTexture2D(mat.diffuse_texname, dir));
        if (!mat.bump_texname.empty())
            ss->setTextureAttributeAndModes(1, createTexture2D(mat.bump_texname, dir));
        if (!mat.specular_texname.empty())
            ss->setTextureAttributeAndModes(2, createTexture2D(mat.specular_texname, dir));
        if (!mat.specular_highlight_texname.empty())
            ss->setTextureAttributeAndModes(3, createTexture2D(mat.specular_highlight_texname, dir));
        if (!mat.reflection_texname.empty())
            ss->setTextureAttributeAndModes(6, createTexture2D(mat.reflection_texname, dir));
        return ss.release();
    }

    osg::Texture* createTexture2D(const std::string& file, const std::string& dir) const
    {
        std::string texPath = dir + "/" + file;
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(texPath);
        if (image.valid())
        {
            osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D(image.get());
            tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
            tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT); return tex.release();
        }
        else
            OSG_NOTICE << "[ReaderWriterMesh] failed to load texture: " << texPath << std::endl;
        return NULL;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_mesh, ReaderWriterMesh)
