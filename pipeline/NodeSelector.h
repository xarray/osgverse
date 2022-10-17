#ifndef MANA_PP_NODESELECTOR_HPP
#define MANA_PP_NODESELECTOR_HPP

#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgGA/GUIEventHandler>

namespace osgVerse
{
    /** Manages gizmos and actions for object and multi-object picking */
    class NodeSelector : public osg::Referenced
    {
    public:
        enum ComputationMethod { USE_NODE_BBOX, USE_NODE_BSPHERE };
        enum SelectorType { SINGLE_SELECTOR, RECTANGLE_SELECTOR, CIRCLE_SELECTOR, POLYGON_SELECTOR };
        enum BoundType { BOUND_BOX, BOUND_RECTANGLE, BOUND_SQUARE };
        NodeSelector();

        /** Set main camera */
        virtual void setMainCamera(osg::Camera* cam) { _mainCamera = cam; }
        osg::Camera* getMainCamera() { return _mainCamera.get(); }

        /** Get root node of all HUD/scene objects */
        osg::Group* getHeadUpDisplayRoot() { return _hudRoot.get(); }
        osg::Group* getAuxiliaryRoot() { return _auxiliaryRoot.get(); }

        /** Set selector line display type */
        void setComputationMethod(ComputationMethod m) { _computationMethod = m; }
        ComputationMethod getComputationMethod() const { return _computationMethod; }

        /** Set selector line display type */
        void setSelectorType(SelectorType t) { _selectorType = t; rebuildSelectorGeometry(); }
        SelectorType getSelectorType() const { return _selectorType; }

        /** Set selection's bound display type */
        void setBoundType(BoundType t) { _boundType = t; rebuildBoundGeometry(); }
        BoundType getBoundType() const { return _boundType; }

        /** Set the selector lines color */
        void setSelectorColor(const osg::Vec4& c) { _selectorColor = c; rebuildSelectorGeometry(true); }
        const osg::Vec4& setSelectorColor() const { return _selectorColor; }

        /** Set the bound color */
        void setBoundColor(const osg::Vec4& c) { _boundColor = c; rebuildBoundGeometry(true); }
        const osg::Vec4& getBoundColor() const { return _boundColor; }

        /** Set the bound's corner line length */
        void setBoundDeltaLength(float l) { _boundDeltaLength = l; rebuildBoundGeometry(); }
        float getBoundDeltaLength() const { return _boundDeltaLength; }

        /** Get the selector geometry */
        osg::Geometry* getSelectorGeometry() { return _selectorGeometry.get(); }
        const osg::Geometry* getSelectorGeometry() const { return _selectorGeometry.get(); }

        /** Get the bound geometry */
        osg::Geometry* getBoundGeometry() { return _boundGeometry.get(); }
        const osg::Geometry* getBoundGeometry() const { return _boundGeometry.get(); }

        /** Call this when user events come to pass them to the picker */
        virtual bool event(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

        /** Get the selector line in world space (for single mode) */
        bool obtainSelectorData(unsigned int index, osg::Vec3& start, osg::Vec3& end);

        /** Get the selector polytope in world space (for rectangle and polygon mode) */
        bool obtainSelectorData(osg::Polytope& polytope);

        /** Get the selector cone in world space (for circle mode) */
        bool obtainSelectorData(osg::Vec3& c1, osg::Vec3& c2, float& r1, float& r2);

        /** Add a new selector point, must be projected (rectangle/circle mode only accept 2 points) */
        bool addSelectorPoint(float x, float y, bool firstPoint);

        /** Remove the last selector point */
        bool removeLastSelectorPoint();

        /** Drag the last selector point to change selector shape, must be projected */
        bool moveLastSelectorPoint(float x, float y);

        /** Clear all selector points */
        void clearAllSelectorPoints();

        /** Mark a node as selected */
        bool addSelectedNode(osg::Node* node);

        /** Mark multiple nodes as selected, using one or more bounds */
        bool addSelectedNodes(const osg::NodePath& nodes, bool useSingleBound);

        /** Mark a node as unselected (node in a multiple selection will be excluded) */
        bool removeSelectedNode(osg::Node* node);

        /** Check if a node is selected and return the bound matrix */
        bool isNodeSelected(osg::Node* node, osg::Matrix& boundMatrix) const;

        /** Clear all selection bounds */
        void clearAllSelectedNodes();

        typedef std::map<osg::Node*, osg::MatrixTransform*> SelectionMap;
        SelectionMap& getSelectionMap() { return _selections; }
        const SelectionMap& getSelectionMap() const { return _selections; }

    protected:
        class BoundUpdater : public osg::NodeCallback
        {
        public:
            BoundUpdater(osg::Node* t, NodeSelector* p) : _picker(p) { _targets.push_back(t); }
            BoundUpdater(const osg::NodePath& t, NodeSelector* p) : _targets(t), _picker(p) {}
            bool removeTarget(osg::Node* t);
            virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        protected:
            osg::NodePath _targets;
            NodeSelector* _picker;
        };

        void updateSelectionGeometry(bool rebuilding = false);
        void rebuildSelectorGeometry(bool onlyColors = false);
        void rebuildBoundGeometry(bool onlyColors = false);

        SelectionMap _selections;
        osg::observer_ptr<osg::Camera> _mainCamera;
        osg::ref_ptr<osg::Group> _hudRoot, _auxiliaryRoot;

        osg::ref_ptr<osg::Geode> _selectorGeode, _boundGeode;
        osg::ref_ptr<osg::Geometry> _selectorGeometry, _boundGeometry;
        osg::ref_ptr<osg::Drawable> _boundMaskBox;
        osg::Vec4 _selectorColor, _boundColor;
        float _boundDeltaLength;
        ComputationMethod _computationMethod;
        SelectorType _selectorType;
        BoundType _boundType;
    };
}

#endif
