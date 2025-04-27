#ifndef MANA_PP_RASTERIZER_HPP
#define MANA_PP_RASTERIZER_HPP

#include <osg/Texture2D>
#include <osg/Geometry>
#include <set>
struct Occluder;

namespace osgVerse
{
    class UserOccluder : public osg::Referenced
    {
    public:
        UserOccluder(const std::string& name, const std::vector<osg::Vec3> vertices,
                     const osg::BoundingBoxf& refBound);
        Occluder* getOccluder() { return _privateData.get(); }

        osg::BoundingBoxf getBound() const;
        osg::Vec3 getCenter() const;
        const std::string& getName() const { return _name; }

    protected:
        virtual ~UserOccluder();
        std::unique_ptr<Occluder> _privateData;
        std::string _name;
    };

    class Rasterizer : public osg::Referenced
    {
    public:
        Rasterizer(unsigned int width, unsigned int height);
        void setModelViewProjection(const osg::Matrixd& matrix);
        void render(std::vector<float>& depthData);

        void addOccluder(UserOccluder* o) { _occluders.insert(o); }
        void removeOccluder(UserOccluder* o);
        void removeAllOccluders() { _occluders.clear(); }

        std::set<osg::ref_ptr<UserOccluder>>& getOccluders() { return _occluders; }
        const std::set<osg::ref_ptr<UserOccluder>>& getOccluders() const { return _occluders; }
        unsigned int getNumOccluders() const { return _occluders.size(); }

    protected:
        virtual ~Rasterizer();
        bool queryVisibility(const osg::BoundingBoxf& occluderBound, bool& needsClipping);
        void rasterize(UserOccluder& occluder, bool needsClipping);
        void clear();

        std::set<osg::ref_ptr<UserOccluder>> _occluders;
        std::vector<char> _depthData;
        void* _privateData;
    };
}

#endif
