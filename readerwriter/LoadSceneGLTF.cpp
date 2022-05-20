#include <osg/io_utils>
#include <osg/Version>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include "pipeline/Utilities.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "LoadSceneGLTF.h"

#ifndef GL_ARB_texture_rg
    #define GL_RG                             0x8227
#endif

namespace osgVerse
{
    LoaderGLTF::LoaderGLTF(std::istream& in, const std::string& d, bool isBinary)
    {
        std::string err, warn; bool loaded = false;
        std::istreambuf_iterator<char> eos;
        std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
        if (data.empty()) { OSG_WARN << "[LoaderFBX] Unable to read from stream\n"; return; }

        tinygltf::TinyGLTF loader;
        if (isBinary)
        {
            loaded = loader.LoadBinaryFromMemory(
                &_scene, &err, &warn, (unsigned char*)&data[0], data.size(), d);
        }
        else
            loaded = loader.LoadASCIIFromString(&_scene, &err, &warn, &data[0], data.size(), d);
        
        if (!err.empty()) OSG_WARN << "[LoaderFBX] Errors found: " << err << "\n";
        if (!warn.empty()) OSG_WARN << "[LoaderFBX] Warnings found: " << warn << "\n";
        if (!loaded) { OSG_WARN << "[LoaderFBX] Unable to load GLTF scene\n"; return; }
        _root = new osg::Group;

        const tinygltf::Scene& defScene = _scene.scenes[_scene.defaultScene];
        for (size_t i = 0; i < defScene.nodes.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createNode(_scene.nodes[defScene.nodes[i]]);
            if (child.valid()) _root->addChild(child.get());
        }
    }

    osg::Node* LoaderGLTF::createNode(tinygltf::Node& node)
    {
        osg::ref_ptr<osg::Geode> geode = (node.mesh >= 0) ? new osg::Geode : NULL;
        if (geode.valid()) createMesh(geode.get(), _scene.meshes[node.mesh]);  // TODO: skin
        if (node.matrix.empty() && node.children.empty()) return geode.release();

        osg::ref_ptr<osg::MatrixTransform> group = new osg::MatrixTransform;
        for (size_t i = 0; i < node.children.size(); ++i)
        {
            osg::ref_ptr<osg::Node> child = createNode(_scene.nodes[node.children[i]]);
            if (child.valid()) group->addChild(child.get());
        }

        if (geode.valid()) group->addChild(geode.get());
        if (!node.matrix.empty()) group->setMatrix(osg::Matrix(&node.matrix[0]));
        return group.release();
    }

    bool LoaderGLTF::createMesh(osg::Geode* geode, tinygltf::Mesh mesh)
    {
        for (size_t i = 0; i < mesh.primitives.size(); ++i)
        {
            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);

            tinygltf::Primitive primitive = mesh.primitives[i];
            for (auto& attrib : primitive.attributes)
            {
                tinygltf::Accessor attrAccessor = _scene.accessors[attrib.second];
                const tinygltf::BufferView& attrView = _scene.bufferViews[attrAccessor.bufferView];
                if (attrView.buffer < 0) continue;

                const tinygltf::Buffer& buffer = _scene.buffers[attrView.buffer];
                int compNum = (attrAccessor.type != TINYGLTF_TYPE_SCALAR) ? attrAccessor.type : 1;
                int compSize = tinygltf::GetComponentSizeInBytes(attrAccessor.componentType);
                int size = attrAccessor.count; if (!size) continue;

                int copySize = size * (compSize * compNum);
                if (attrib.first.compare("POSITION") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* va = new osg::Vec3Array(size);
                    memcpy(&(*va)[0], &buffer.data[attrView.byteOffset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    va->setNormalize(attrAccessor.normalized);
#endif
                    geom->setVertexArray(va);
                }
                else if (attrib.first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::Vec3Array* na = new osg::Vec3Array(size);
                    memcpy(&(*na)[0], &buffer.data[attrView.byteOffset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    na->setNormalize(attrAccessor.normalized);
                    geom->setNormalArray(na, osg::Array::BIND_PER_VERTEX);
#else
                    geom->setNormalArray(na); geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
#endif
                }
                else if (attrib.first.compare("TANGENT") == 0 && compSize == 4 && compNum == 4)
                {   // Do nothing as we calculate tangent/binormal by ourselves
#if 0
                    osg::Vec4Array* ta = new osg::Vec4Array(size);
                    memcpy(&(*ta)[0], &buffer.data[attrView.byteOffset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
                    geom->setVertexAttribArray(6, ta, osg::Array::BIND_PER_VERTEX);
#endif
                    geom->setVertexAttribArray(6, ta);
                    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
#endif
                }
                else if (attrib.first.find("TEXCOORD_") != std::string::npos && compSize == 4 && compNum == 2)
                {
                    osg::Vec2Array* ta = new osg::Vec2Array(size);
                    memcpy(&(*ta)[0], &buffer.data[attrView.byteOffset], copySize);
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                    ta->setNormalize(attrAccessor.normalized);
#endif
                    geom->setTexCoordArray(atoi(attrib.first.substr(9).c_str()), ta);
                }
                else
                    OSG_WARN << "[LoaderGLTF] Unsupported primitive " << attrib.first << " with "
                             << compNum << "-components and dataSize=" << compSize << std::endl;
            }

            tinygltf::Accessor indexAccessor = _scene.accessors[primitive.indices];
            const tinygltf::BufferView& indexView = _scene.bufferViews[indexAccessor.bufferView];
            osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
            if (!va || (va && va->empty())) continue;

            osg::ref_ptr<osg::PrimitiveSet> p;
            if (indexView.target == 0)
                p = new osg::DrawArrays(GL_POINTS, 0, va->size());
            else
            {
                const tinygltf::Buffer& indexBuffer = _scene.buffers[indexView.buffer];
                int compSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
                int size = indexAccessor.count; if (!size) continue;

                switch (compSize)
                {
                case 1:
                    {
                        osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[indexView.byteOffset], size * compSize);
                    }
                    break;
                case 2:
                    {
                        osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[indexView.byteOffset], size * compSize);
                    }
                    break;
                case 4:
                    {
                        osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_POINTS, size); p = de;
                        memcpy(&(*de)[0], &indexBuffer.data[indexView.byteOffset], size * compSize);
                    }
                    break;
                default: continue;
                }
            }

            switch (primitive.mode)
            {
            case TINYGLTF_MODE_POINTS: p->setMode(GL_POINTS); break;
            case TINYGLTF_MODE_LINE: p->setMode(GL_LINES); break;
            case TINYGLTF_MODE_LINE_LOOP: p->setMode(GL_LINE_LOOP); break;
            case TINYGLTF_MODE_LINE_STRIP: p->setMode(GL_LINE_STRIP); break;
            case TINYGLTF_MODE_TRIANGLES: p->setMode(GL_TRIANGLES); break;
            case TINYGLTF_MODE_TRIANGLE_STRIP: p->setMode(GL_TRIANGLE_STRIP); break;
            case TINYGLTF_MODE_TRIANGLE_FAN: p->setMode(GL_TRIANGLE_FAN); break;
            default: continue;
            }
            geom->addPrimitiveSet(p.get());

            if (primitive.material >= 0)
            {
                tinygltf::Material& material = _scene.materials[primitive.material];
                createMaterial(geom->getOrCreateStateSet(), material);
            }
            geode->addDrawable(geom.get());
        }
        return true;
    }

    void LoaderGLTF::createMaterial(osg::StateSet* ss, tinygltf::Material material)
    {
        // FIXME: Shininess = inv(Roughness), Ambient = Occlusion...
        int baseID = material.pbrMetallicRoughness.baseColorTexture.index;
        int roughnessID = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        int normalID = material.normalTexture.index;
        int emissiveID = material.emissiveTexture.index;
        int occlusionID = material.occlusionTexture.index;

        if (baseID >= 0) createTexture(ss, 0, uniformNames[0], _scene.textures[baseID]);
        if (normalID >= 0) createTexture(ss, 1, uniformNames[1], _scene.textures[normalID]);
        if (roughnessID >= 0) createTexture(ss, 3, uniformNames[3], _scene.textures[roughnessID]);
        if (occlusionID >= 0) createTexture(ss, 4, uniformNames[4], _scene.textures[occlusionID]);
        if (emissiveID >= 0) createTexture(ss, 5, uniformNames[5], _scene.textures[emissiveID]);

#if 0
        if (material.alphaMode.compare("OPAQUE") == 0)  // FIXME: handle transparent
            ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
        else
            ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
#endif
    }

    void LoaderGLTF::createTexture(osg::StateSet* ss, int u,
                                   const std::string& name, tinygltf::Texture& tex)
    {
        tinygltf::Image& imageSrc = _scene.images[tex.source];
        if (imageSrc.image.empty()) return;

        GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
        if (imageSrc.bits == 16) type = GL_UNSIGNED_SHORT;
        if (imageSrc.component == 1) format = GL_RED;
        else if (imageSrc.component == 2) format = GL_RG;
        else if (imageSrc.component == 3) format = GL_RGB;

        osg::Texture2D* tex2D = _textureMap[tex.source].get();
        if (!tex2D || u == 1)  // FIXME: dont know why but normal-maps can't be shared?
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->allocateImage(imageSrc.width, imageSrc.height, 1, format, type);
            image->setInternalTextureFormat(imageSrc.component);
            memcpy(image->data(), &imageSrc.image[0], image->getTotalSizeInBytes());

            tex2D = new osg::Texture2D;
            tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
            tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
            tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
            tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            tex2D->setImage(image.get()); tex2D->setName(imageSrc.uri);

            _textureMap[tex.source] = tex2D;
            OSG_NOTICE << "[LoaderGLTF] " << imageSrc.uri << " loaded for " << name << std::endl;
        }
        ss->setTextureAttributeAndModes(u, tex2D);
        ss->addUniform(new osg::Uniform(name.c_str(), u));
    }

    osg::ref_ptr<osg::Group> loadGltf(const std::string& file, bool isBinary)
    {
        std::string workDir = osgDB::getFilePath(file);
        std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, workDir, isBinary);
        return loader->getRoot();
    }

    osg::ref_ptr<osg::Group> loadGltf2(std::istream& in, const std::string& dir, bool isBinary)
    {
        osg::ref_ptr<LoaderGLTF> loader = new LoaderGLTF(in, dir, isBinary);
        return loader->getRoot();
    }
}
