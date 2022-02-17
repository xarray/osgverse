#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>
#include <tiny_gltf.h>

namespace osgVerse
{
    class LoaderGLTF : public osg::Referenced
    {
    public:
        LoaderGLTF(std::istream& in, const std::string& d, bool isBinary);

        osg::Group* getRoot() { return _root.get(); }
        tinygltf::Model& getScene() { return _scene; }

    protected:
        virtual ~LoaderGLTF() {}
        osg::Node* createNode(tinygltf::Node& node);
        bool createMesh(osg::Geode* geode, tinygltf::Mesh mesh);
        void createMaterial(osg::StateSet* ss, tinygltf::Material mat);
        void createTexture(osg::StateSet* ss, int u, const std::string& name, tinygltf::Texture& tex);

        std::map<int, osg::observer_ptr<osg::Texture2D>> _textureMap;
        osg::ref_ptr<osg::Group> _root;
        tinygltf::Model _scene;
        std::string _workingDir;
    };

    extern osg::ref_ptr<osg::Group> loadGltf(const std::string& file, bool isBinary);
    extern osg::ref_ptr<osg::Group> loadGltf2(std::istream& in, const std::string& dir, bool isBinary);
}
