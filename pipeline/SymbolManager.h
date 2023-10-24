#ifndef MANA_PP_SYMBOLMANAGER_HPP
#define MANA_PP_SYMBOLMANAGER_HPP

#include <osg/Version>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/MatrixTransform>

namespace osgVerse
{
    struct Symbol : public osg::Referenced
    {
        enum State { Hidden = 0, FarClustered, FarDistance,
                     MidDistance, NearDistance };
        Symbol() : state(Hidden), id(-1), modelFrame0(0)
        {
            color = osg::Vec3(1.0f, 1.0f, 1.0f);
            rotateAngle = 0.0f; scale = 0.1f;
        }

        osg::observer_ptr<osg::Node> loadedModel;
        std::string name, desciption, fileName;
        osg::Vec3d position; osg::Vec3f color;
        float rotateAngle, scale;
        State state; int id, modelFrame0;
    };

    /** The symbol manager. */
    class SymbolManager : public osg::NodeCallback
    {
    public:
        SymbolManager();
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        enum LodLevel { LOD0 = 0/*->Far->*/, LOD1/*->Mid->*/, LOD2/*->Near->*/ };
        void setLodDistance(LodLevel lv, double d) { _lodDistances[(int)lv] = d; }
        double getLodDistance(LodLevel lv) const { return _lodDistances[(int)lv]; }

        void setInstanceGeometry(osg::Geometry* g) { _instanceGeom = g; }
        osg::Geometry* getInstanceGeometry() { return _instanceGeom.get(); }

        void setMainCamera(osg::Camera* cam) { _camera = cam; }
        osg::Camera* getMainCamera() { return _camera.get(); }

        int updateSymbol(Symbol* sym);
        bool removeSymbol(Symbol* sym);
    
    protected:
        virtual ~SymbolManager() {}
        void initialize(osg::Group* group);
        void update(osg::Group* group, unsigned int frameNo);
        void updateNearDistance(Symbol* sym, osg::Group* group);

        std::map<int, osg::ref_ptr<Symbol>> _symbols;
        osg::ref_ptr<osg::Geometry> _instanceGeom;
        osg::ref_ptr<osg::Texture2D> _posTexture, _dirTexture;
        osg::observer_ptr<osg::Camera> _camera;
        double _lodDistances[3];
        int _idCounter;
        bool _firstRun;
    };
}

#endif
