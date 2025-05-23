#ifndef MANA_PP_RASTERIZER_HPP
#define MANA_PP_RASTERIZER_HPP

#include <osg/Texture2D>
#include <osg/Geometry>
#include <memory>
#include <set>

//#define WITH_SIMD_INPUT 1
#ifdef WITH_SIMD_INPUT
#   include <smmintrin.h>
#endif

namespace osgVerse
{
    class UserOccluder;
    class BatchOccluder : public osg::Referenced
    {
    public:
        BatchOccluder(UserOccluder* u, const std::vector<osg::Vec3> vertices,
                      const osg::BoundingBoxf& refBound);
        BatchOccluder(UserOccluder* u, void* verticesInternal, const osg::BoundingBoxf& refBound);
        void* getOccluder() const { return _privateData; }
        UserOccluder* getOwner() { return _owner; }

        osg::BoundingBoxf getBound() const;
        osg::Vec3 getCenter() const;
        unsigned int getNumVertices() const { return _numVertices; }

    protected:
        virtual ~BatchOccluder();
        void* _privateData;
        UserOccluder* _owner;
        unsigned int _numVertices;
    };

    class UserOccluder : public osg::Referenced
    {
    public:
        UserOccluder(const std::string& name, const std::vector<osg::Vec3> vertices,
                     const std::vector<unsigned int>& indices);
#ifdef WITH_SIMD_INPUT
        UserOccluder(const std::string& name, const std::vector<__m128> vertices,
                     const std::vector<unsigned int>& indices);
#endif
        UserOccluder(osg::Geometry& geom);
        UserOccluder(osg::Node& node);

        std::set<osg::ref_ptr<BatchOccluder>>& getBatches() { return _batches; }
        const std::string& getName() const { return _name; }

    protected:
        virtual ~UserOccluder() {}
        void set(const std::vector<osg::Vec3> v, const std::vector<unsigned int>& i);

        std::set<osg::ref_ptr<BatchOccluder>> _batches;
        std::string _name;
    };

    class UserRasterizer : public osg::Referenced
    {
    public:
        UserRasterizer(unsigned int width, unsigned int height);
        void setModelViewProjection(const osg::Matrix& view, const osg::Matrix& proj);
        void render(const osg::Vec3& cameraPos, std::vector<float>* depthData,
                    std::vector<unsigned short>* hizData = NULL);
        float queryVisibility(UserOccluder* occluder, int* numVisible = NULL);

        void addOccluder(UserOccluder* o) { _occluders.insert(o); }
        void removeOccluder(UserOccluder* o);
        void removeAllOccluders() { _occluders.clear(); }

        std::set<osg::ref_ptr<UserOccluder>>& getOccluders() { return _occluders; }
        const std::set<osg::ref_ptr<UserOccluder>>& getOccluders() const { return _occluders; }
        unsigned int getNumOccluders() const { return _occluders.size(); }

    protected:
        virtual ~UserRasterizer();
        std::set<osg::ref_ptr<UserOccluder>> _occluders;
        unsigned int _blockNumX, _blockNumY;
        void* _privateData;
    };
}

#endif
