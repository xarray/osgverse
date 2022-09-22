#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>
#include <tiny_gltf.h>

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
#  if defined(OSGVERSE_RW_LIBRARY)
#    define OSGVERSE_RW_EXPORT   __declspec(dllexport)
#  else
#    define OSGVERSE_RW_EXPORT   __declspec(dllimport)
#  endif
#else
#  define OSGVERSE_RW_EXPORT extern
#endif

namespace osgVerse
{
    class LoaderGLTF : public osg::Referenced
    {
    public:
        LoaderGLTF(std::istream& in, const std::string& d, bool isBinary);

        osg::Group* getRoot() { return _root.get(); }
        tinygltf::Model& getModelData() { return _modelDef; }

    protected:
        virtual ~LoaderGLTF() {}
        osg::Node* createNode(tinygltf::Node& node);
        bool createMesh(osg::Geode* geode, tinygltf::Mesh mesh);
        void createMaterial(osg::StateSet* ss, tinygltf::Material mat);
        void createTexture(osg::StateSet* ss, int u, const std::string& name, tinygltf::Texture& tex);

        std::map<int, osg::observer_ptr<osg::Texture2D>> _textureMap;
        osg::ref_ptr<osg::Group> _root;
        tinygltf::Model _modelDef;
        std::string _workingDir;
    };

    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadGltf(const std::string& file, bool isBinary);
    OSGVERSE_RW_EXPORT osg::ref_ptr<osg::Group> loadGltf2(std::istream& in, const std::string& dir, bool isBinary);
}
