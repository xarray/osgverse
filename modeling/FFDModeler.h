#ifndef MANA_MODELING_FFDMODELER
#define MANA_MODELING_FFDMODELER

#include <osg/Geometry>
#include <osg/Geode>

namespace osgVerse
{

    class UniformBSpline
    {
    public:
        struct DerivativeData
        {
            std::vector<double> d0;  // number = cp + deg
            std::vector<double> d1;
            std::vector<double> d2;
            std::vector<double> d3;
        };

        UniformBSpline();
        void create(int numCtrl, int deg, bool closed);

        int getNumCtrlPoints() const { return _numCtrlPoints; }
        int getDegree() const { return _degree; }
        bool isClosed() const { return _isClosed; }

        /** Access basis functions and their derivatives */
        double getD0(int i) const { return _derivatives[_degree].d0[i]; }
        double getD1(int i) const { return _derivatives[_degree].d1[i]; }
        double getD2(int i) const { return _derivatives[_degree].d2[i]; }
        double getD3(int i) const { return _derivatives[_degree].d3[i]; }

        /** Evaluate basis functions and their derivatives */
        void compute(double time, unsigned int order, int& minIndex, int& maxIndex);

    protected:
        int getKey(double& time) const;

        std::vector<DerivativeData> _derivatives;  // number = deg + 1
        std::vector<double> _knots;  // number = cp + deg + 1
        int _numCtrlPoints;
        int _degree;
        bool _isClosed;
    };

    class BSplineVolume : public osg::Referenced
    {
    public:
        struct VolumeIndex
        {
            VolumeIndex(int uu = 0, int vv = 0, int ww = 0) : u(uu), v(vv), w(ww) {}
            int u, v, w;

            bool operator<(const VolumeIndex& rhs) const
            {
                if (u < rhs.u) return true; else if (u > rhs.u) return false;
                if (v < rhs.v) return true; else if (v > rhs.v) return false;
                return w < rhs.w;
            }

            bool operator==(const VolumeIndex& rhs) const
            { return (u == rhs.u) && (v == rhs.v) && (w == rhs.w); }
        };

        /** Create a BSpline volume: ctrl points number >= 2, 1 <= degree <= cp - 1 */
        BSplineVolume(int numUCtrl = 2, int numVCtrl = 2, int numWCtrl = 2,
                      int uDeg = 1, int vDeg = 1, int wDeg = 1);
        int getNumCtrlPoints(int dim) const { return _basis[dim].getNumCtrlPoints(); }
        int getDegree(int dim) const { return _basis[dim].getDegree(); }

        /** Set the control points at specific index */
        void setControlPoint(const VolumeIndex& index, const osg::Vec3& cp) { _ctrlPoints[index] = cp; }
        osg::Vec3 getControlPoint(const VolumeIndex& index) const;
        const std::map<VolumeIndex, osg::Vec3>& getAllControlPoints() const { return _ctrlPoints; }

        /** Get position at specific domain (0<=u<=1, etc.) */
        osg::Vec3 getPosition(float u, float v, float w);
        osg::Vec3 getDerivativeU(float u, float v, float w);
        osg::Vec3 getDerivativeV(float u, float v, float w);
        osg::Vec3 getDerivativeW(float u, float v, float w);

    protected:
        virtual ~BSplineVolume();

        std::map<VolumeIndex, osg::Vec3> _ctrlPoints;
        UniformBSpline _basis[3];
    };

    /** The node visitor for computing node data controlled by the FFD modeler */
    class ApplyUserNodeVisitor : public osg::NodeVisitor
    {
    public:
        ApplyUserNodeVisitor(TraversalMode mode = TRAVERSE_ALL_CHILDREN)
            : osg::NodeVisitor(mode), _mode(REQ_BOUND) { reset(true, true); }

        void setBSplineVolume(BSplineVolume* bv) { _volume = bv; }
        BSplineVolume* getBSplineVolume() { return _volume.get(); }
        const BSplineVolume* getBSplineVolume() const { return _volume.get(); }

        enum ApplyMode { REQ_BOUND, REQ_NORMV, REQ_SETV };
        void setMode(ApplyMode mode) { _mode = mode; }
        ApplyMode getMode() const { return _mode; }

        osg::BoundingBox& getBoundingBox() { return _bb; }
        const osg::BoundingBox& getBoundingBox() const { return _bb; }

        virtual void apply(osg::Transform& node);
        virtual void apply(osg::Geode& node);

        void applyDrawable(osg::Drawable* drawable);
        void reset(bool resetBB, bool resetVM);
        void start(ApplyMode mode, osg::Node* node);

    protected:
        void computeBoundBox(osg::Geometry* geometry);
        void computeNormalizedVertex(osg::Geometry* geometry);
        void computeNewVertex(osg::Geometry* geometry);

        osg::observer_ptr<BSplineVolume> _volume;

        typedef std::vector<osg::Matrix> MatrixStack;
        MatrixStack _matrixStack;
        osg::Matrix _upperMatrix;

        typedef std::vector<osg::Vec3> VertexList;
        typedef std::map<osg::Geometry*, VertexList> VertexMap;
        VertexMap _normVertexMap;
        osg::BoundingBox _bb;
        ApplyMode _mode;
    };

    /** The FFD modeler to control an input node */
    class FFDModeler : public osg::Referenced
    {
    public:
        FFDModeler();

        /** Set the FFD quantity along U/V/W. Will reset the volume */
        void setQuantity(int u, int v, int w);
        void getQuantity(int& u, int& v, int& w) const
        { u = _quantity[0]; v = _quantity[1]; w = _quantity[2]; }

        /** Set the node to modify. Will reset the volume */
        void setNode(osg::Node* node);
        osg::Node* getNode() { return _node.get(); }
        const osg::Node* getNode() const { return _node.get(); }

        /** Set a control point in the world coordinate to change the node geometries */
        void setCtrlPoint(int u, int v, int w, const osg::Vec3& pt);
        osg::Vec3 getCtrlPoint(int u, int v, int w) const;

        /** Try to select a control point from the mouse picking and return the point's depth value */
        float selectOnCtrlBox(float mx, float my, const osg::Matrix& vpw, const osg::Vec4& color,
                              int& u, int& v, int& w, float precision = 10.0f);

        /** Set grid color */
        void setFFDGridColor(const osg::Vec4& color);
        osg::Vec4 getFFDGridColor() const { return _gridColor; }

        /** FFD grid geometry to be outputted. Don't add it to other transform nodes */
        osg::Geometry* getFFDGridResult() { return _ffdGeom.get(); }

    protected:
        virtual ~FFDModeler();
        void comupteFFDBox(const osg::BoundingBox& bb);
        void allocateFFDGeometry(int u, int v, int w);

        osg::observer_ptr<osg::Node> _node;
        osg::ref_ptr<osg::Geometry> _ffdGeom;
        osg::ref_ptr<BSplineVolume> _volume;
        osg::Vec4 _gridColor;
        ApplyUserNodeVisitor _userNodeVisitor;
        int _quantity[3];
    };

}

#endif
