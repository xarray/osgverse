#ifndef MANA_PP_SYMBOLMANAGER_HPP
#define MANA_PP_SYMBOLMANAGER_HPP

#include <osg/Version>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "Drawer2D.h"

namespace osgVerse
{
    struct Symbol : public osg::Referenced
    {
        enum State { Hidden = 0, FarClustered, FarDistance,
                     MidDistance, NearDistance };
        Symbol() : state(Hidden), id(-1), modelFrame0(0)
        {
            texTiling = osg::Vec3(0.0f, 0.0f, 1.0f);
            rotateAngle = 0.0f; scale = 0.1f; dirtyDesc = false;
        }

        void setDesciption(const std::string& d)
        { desciption = d; dirtyDesc = true; }

        osg::observer_ptr<osg::Node> loadedModel;
        osg::observer_ptr<osg::Texture2D> loadedModelBoard;
        std::string name, desciption, fileName;
        osg::Vec3d position; osg::Vec3f texTiling;
        float rotateAngle, scale; bool dirtyDesc;
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

        void setLodScaleFactor(const osg::Vec3& factor) { _lodIconScaleFactor = factor; }
        const osg::Vec3& getLodScaleFactor() const { return _lodIconScaleFactor; }

        void setInstanceGeometry(osg::Geometry* g) { _instanceGeom = g; }
        osg::Geometry* getInstanceGeometry() { return _instanceGeom.get(); }

        void setInstanceBillboard(osg::Geometry* g) { _instanceBoard = g; }
        osg::Geometry* getInstanceBillboard() { return _instanceBoard.get(); }

        void setIconAtlasImage(osg::Image* image) { _iconTexture->setImage(image); }
        osg::Image* getIconAtlasImage() { return _iconTexture->getImage(); }

        void setMainCamera(osg::Camera* cam) { _camera = cam; }
        osg::Camera* getMainCamera() { return _camera.get(); }

        void setFontFileName(const std::string& file)
        { _drawer->loadFont("def", file); }

        void setMidDistanceTextOffset(const osg::Vec3& oIn)
        {
            osg::Vec3 o = getMidDistanceTextOffset(), s = getMidDistanceTextScale();
            o.x() = oIn.x() / s.x(); o.y() = oIn.y() / s.y(); o.z() = oIn.z(); _midDistanceOffset->set(o);
        }

        void setMidDistanceTextScale(const osg::Vec2& scale, int numTiles = 10)
        {
            osg::Vec3 s = getMidDistanceTextScale(); s.x() = scale.x(); s.y() = scale.y();
            if (numTiles > 0) s.z() = 1.0f / (float)numTiles; _midDistanceScale->set(s);
        }

        osg::Vec3 getMidDistanceTextOffset() const
        { osg::Vec3 offset; _midDistanceOffset->get(offset); return offset; }

        osg::Vec3 getMidDistanceTextScale() const
        { osg::Vec3 offset; _midDistanceScale->get(offset); return offset; }

        /** Add or update symbol data to manager */
        int updateSymbol(Symbol* sym);

        /** Remove symbol data from manager */
        bool removeSymbol(Symbol* sym);

        /** Get symbol by ID */
        Symbol* getSymbol(int id);
        const Symbol* getSymbol(int id) const;

        /** Query symbols by position / polytope */
        std::vector<Symbol*> querySymbols(const osg::Vec3d& pos, double radius) const;
        std::vector<Symbol*> querySymbols(const osg::Polytope& polytope) const;
        std::vector<Symbol*> querySymbols(const osg::Vec2d& proj, double eplsion) const;

        std::map<int, osg::ref_ptr<Symbol>>& getSymols() { return _symbols; }
        const std::map<int, osg::ref_ptr<Symbol>>& getSymols() const { return _symbols; }

        struct DrawTextGridCallback : public osg::Referenced
        {
            // Draw atlased text-grid image for mid-distance symbols
            virtual osg::Image* create(Drawer2D* drawer, const std::vector<Symbol*>& texts) = 0;
        };
        void setDrawTextGridCallback(DrawTextGridCallback* cb) { _drawGridCallback = cb; }
        DrawTextGridCallback* getDrawTextGridCallback() { return _drawGridCallback.get(); }
    
    protected:
        virtual ~SymbolManager() {}
        void initialize(osg::Group* group);
        void update(osg::Group* group, unsigned int frameNo);
        void updateNearDistance(Symbol* sym, osg::Group* group);

        osg::Image* createLabel(int w, int h, const std::string& text,
                                const osg::Vec4& color = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
        osg::Image* createGrid(int w, int h, int grid, const std::vector<Symbol*>& texts,
                               const osg::Vec4& color = osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f));

        std::map<int, osg::ref_ptr<Symbol>> _symbols;
        osg::ref_ptr<osg::Geometry> _instanceGeom, _instanceBoard;
        osg::ref_ptr<osg::Texture2D> _posTexture, _posTexture2;
        osg::ref_ptr<osg::Texture2D> _dirTexture, _iconTexture, _textTexture;
        osg::ref_ptr<osg::Uniform> _midDistanceOffset, _midDistanceScale;
        osg::ref_ptr<DrawTextGridCallback> _drawGridCallback;
        osg::ref_ptr<Drawer2D> _drawer;
        osg::observer_ptr<osg::Camera> _camera;
        osg::Vec3 _lodIconScaleFactor;
        double _lodDistances[3];
        int _idCounter;
        bool _firstRun;
    };
}

#endif
