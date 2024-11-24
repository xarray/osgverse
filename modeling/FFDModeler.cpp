#include "FFDModeler.h"
#include <osg/Version>
using namespace osgVerse;

/* UniformBSpline */

UniformBSpline::UniformBSpline()
    : _numCtrlPoints(0), _degree(0), _isClosed(false)
{
}

void UniformBSpline::create(int numCtrl, int deg, bool closed)
{
    _numCtrlPoints = numCtrl;
    _degree = deg;
    _isClosed = closed;

    _knots.resize(numCtrl + deg + 1);
    _derivatives.resize(deg + 1);
    for (int i = 0; i <= deg; ++i)
    {
        DerivativeData& data = _derivatives[i];
        data.d0.resize(numCtrl + deg);
        data.d1.resize(numCtrl + deg);
        data.d2.resize(numCtrl + deg);
        data.d3.resize(numCtrl + deg);
    }

    double factor = 1.0 / (numCtrl - deg);
    if (!closed)
    {
        int i = 0;
        for (; i <= deg; ++i) _knots[i] = 0.0;
        for (; i < numCtrl; ++i) _knots[i] = factor * (double)(i - deg);
        for (; i < (int)_knots.size(); ++i) _knots[i] = 1.0;
    }
    else
    {
        for (unsigned int i = 0; i < _knots.size(); ++i)
            _knots[i] = factor * (double)(i - deg);
    }
}

void UniformBSpline::compute(double time, unsigned int order, int& minIndex, int& maxIndex)
{
    int i = getKey(time);
    _derivatives[0].d0[i] = 1.0;
    if (order >= 1)
    {
        _derivatives[0].d1[i] = 0.0;
        if (order >= 2)
        {
            _derivatives[0].d2[i] = 0.0;
            if (order >= 3) _derivatives[0].d3[i] = 0.0;
        }
    }

    double N0 = time - _knots[i], N1 = _knots[i + 1] - time;
    double invD0, invD1;
    for (int j = 1; j <= _degree; ++j)
    {
        invD0 = 1.0 / (_knots[i + j] - _knots[i]);
        invD1 = 1.0 / (_knots[i + 1] - _knots[i - j + 1]);
        _derivatives[j].d0[i] = N0 * _derivatives[j - 1].d0[i] * invD0;
        _derivatives[j].d0[i - j] = N1 * _derivatives[j - 1].d0[i - j + 1] * invD1;

        if (order >= 1)
        {
            _derivatives[j].d1[i] = (N0 * _derivatives[j - 1].d1[i] + _derivatives[j - 1].d0[i]) * invD0;
            _derivatives[j].d1[i - j] = (N1 * _derivatives[j - 1].d1[i - j + 1]
                                      - _derivatives[j - 1].d0[i - j + 1]) * invD1;
            if (order >= 2)
            {
                _derivatives[j].d2[i] =
                    (N0 * _derivatives[j - 1].d2[i] + 2.0 * _derivatives[j - 1].d1[i]) * invD0;
                _derivatives[j].d2[i - j] =
                    (N1 * _derivatives[j - 1].d2[i - j + 1] - 2.0 * _derivatives[j - 1].d1[i - j + 1]) * invD1;
                if (order >= 3)
                {
                    _derivatives[j].d3[i] =
                        (N0 * _derivatives[j - 1].d3[i] + 3.0 * _derivatives[j - 1].d2[i]) * invD0;
                    _derivatives[j].d3[i - j] =
                        (N1 * _derivatives[j - 1].d3[i - j + 1] - 3.0 * _derivatives[j - 1].d2[i - j + 1]) * invD1;
                }
            }
        }
    }

    for (int j = 2; j <= _degree; ++j)
    {
        for (int k = i - j + 1; k < i; ++k)
        {
            N0 = time - _knots[k];
            N1 = _knots[k + j + 1] - time;
            invD0 = 1.0 / (_knots[k + j] - _knots[k]);
            invD1 = 1.0 / (_knots[k + j + 1] - _knots[k + 1]);

            _derivatives[j].d0[k] = N0 * _derivatives[j - 1].d0[k] * invD0 +
                N1 * _derivatives[j - 1].d0[k + 1] * invD1;
            if (order >= 1)
            {
                _derivatives[j].d1[k] = (N0 * _derivatives[j - 1].d1[k] + _derivatives[j - 1].d0[k]) * invD0
                                      + (N1 * _derivatives[j - 1].d1[k + 1] - _derivatives[j - 1].d0[k + 1]) * invD1;
                if (order >= 2)
                {
                    _derivatives[j].d2[k] =
                        (N0 * _derivatives[j - 1].d2[k] + 2.0 * _derivatives[j - 1].d1[k]) * invD0 +
                        (N1 * _derivatives[j - 1].d2[k + 1] - 2.0 * _derivatives[j - 1].d1[k + 1]) * invD1;
                    if (order >= 3)
                    {
                        _derivatives[j].d3[k] =
                            (N0 * _derivatives[j - 1].d3[k] + 3.0 * _derivatives[j - 1].d2[k]) * invD0 +
                            (N1 * _derivatives[j - 1].d3[k + 1] - 3.0 * _derivatives[j - 1].d2[k + 1]) * invD1;
                    }
                }
            }
        }
    }
    minIndex = i - _degree;
    maxIndex = i;
}

int UniformBSpline::getKey(double& time) const
{
    if (!_isClosed)
    {
        if (time <= 0.0) { time = 0.0; return _degree; }
        else if (time >= 1.0) { time = 1.0; return _numCtrlPoints - 1; }
    }
    else
        { if (time < 0.0 || time >= 1.0) time -= floor(time); }
    return _degree + (int)((float)(_numCtrlPoints - _degree) * time);
}

/* BSplineVolume */

BSplineVolume::BSplineVolume(int numUCtrl, int numVCtrl, int numWCtrl, int uDeg, int vDeg, int wDeg)
{
    _basis[0].create(numUCtrl, uDeg, false);
    _basis[1].create(numVCtrl, vDeg, false);
    _basis[2].create(numWCtrl, wDeg, false);
}

BSplineVolume::~BSplineVolume()
{
}

osg::Vec3 BSplineVolume::getControlPoint(const VolumeIndex& index) const
{
    std::map<VolumeIndex, osg::Vec3>::const_iterator itr = _ctrlPoints.find(index);
    if (itr != _ctrlPoints.end()) return itr->second;
    return osg::Vec3();
}

osg::Vec3 BSplineVolume::getPosition(float u, float v, float w)
{
    int umin, umax, vmin, vmax, wmin, wmax;
    _basis[0].compute(u, 0, umin, umax);
    _basis[1].compute(v, 0, vmin, vmax);
    _basis[2].compute(w, 0, wmin, wmax);

    osg::Vec3 pos;
    for (int iu = umin; iu <= umax; ++iu)
    {
        double tmp0 = _basis[0].getD0(iu);
        for (int iv = vmin; iv <= vmax; ++iv)
        {
            double tmp1 = _basis[1].getD0(iv);
            for (int iw = wmin; iw <= wmax; ++iw)
            {
                double tmp2 = _basis[2].getD0(iw);
                double prod = tmp0 * tmp1 * tmp2;
                pos += getControlPoint(VolumeIndex(iu, iv, iw)) * prod;
            }
        }
    }
    return pos;
}

osg::Vec3 BSplineVolume::getDerivativeU(float u, float v, float w)
{
    int umin, umax, vmin, vmax, wmin, wmax;
    _basis[0].compute(u, 1, umin, umax);
    _basis[1].compute(v, 0, vmin, vmax);
    _basis[2].compute(w, 0, wmin, wmax);

    osg::Vec3 pos;
    for (int iu = umin; iu <= umax; ++iu)
    {
        double tmp0 = _basis[0].getD1(iu);
        for (int iv = vmin; iv <= vmax; ++iv)
        {
            double tmp1 = _basis[1].getD0(iv);
            for (int iw = wmin; iw <= wmax; ++iw)
            {
                double tmp2 = _basis[2].getD0(iw);
                double prod = tmp0 * tmp1 * tmp2;
                pos += getControlPoint(VolumeIndex(iu, iv, iw)) * prod;
            }
        }
    }
    return pos;
}

osg::Vec3 BSplineVolume::getDerivativeV(float u, float v, float w)
{
    int umin, umax, vmin, vmax, wmin, wmax;
    _basis[0].compute(u, 0, umin, umax);
    _basis[1].compute(v, 1, vmin, vmax);
    _basis[2].compute(w, 0, wmin, wmax);

    osg::Vec3 pos;
    for (int iu = umin; iu <= umax; ++iu)
    {
        double tmp0 = _basis[0].getD0(iu);
        for (int iv = vmin; iv <= vmax; ++iv)
        {
            double tmp1 = _basis[1].getD1(iv);
            for (int iw = wmin; iw <= wmax; ++iw)
            {
                double tmp2 = _basis[2].getD0(iw);
                double prod = tmp0 * tmp1 * tmp2;
                pos += getControlPoint(VolumeIndex(iu, iv, iw)) * prod;
            }
        }
    }
    return pos;
}

osg::Vec3 BSplineVolume::getDerivativeW(float u, float v, float w)
{
    int umin, umax, vmin, vmax, wmin, wmax;
    _basis[0].compute(u, 0, umin, umax);
    _basis[1].compute(v, 0, vmin, vmax);
    _basis[2].compute(w, 1, wmin, wmax);

    osg::Vec3 pos;
    for (int iu = umin; iu <= umax; ++iu)
    {
        double tmp0 = _basis[0].getD0(iu);
        for (int iv = vmin; iv <= vmax; ++iv)
        {
            double tmp1 = _basis[1].getD0(iv);
            for (int iw = wmin; iw <= wmax; ++iw)
            {
                double tmp2 = _basis[2].getD1(iw);
                double prod = tmp0 * tmp1 * tmp2;
                pos += getControlPoint(VolumeIndex(iu, iv, iw)) * prod;
            }
        }
    }
    return pos;
}

/* ApplyUserNodeVisitor */

void ApplyUserNodeVisitor::apply(osg::Transform& node)
{
    osg::Matrix matrix = _matrixStack.back();
    node.computeLocalToWorldMatrix(matrix, this);

    _matrixStack.push_back(matrix);
    traverse(node);
    _matrixStack.pop_back();
}

void ApplyUserNodeVisitor::apply(osg::Geode& node)
{
    for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
    { applyDrawable(node.getDrawable(i)); }
    traverse(node);
}

void ApplyUserNodeVisitor::applyDrawable(osg::Drawable* drawable)
{
    osg::Geometry* geometry = drawable->asGeometry();
    if (!geometry) return;

    switch (_mode)
    {
    case REQ_BOUND: computeBoundBox(geometry); break;
    case REQ_NORMV: computeNormalizedVertex(geometry); break;
    case REQ_SETV: computeNewVertex(geometry); break;
    default: break;
    }
};

void ApplyUserNodeVisitor::reset(bool resetBB, bool resetVM)
{
    if (resetBB) _bb.init();
    if (resetVM) _normVertexMap.clear();
    _matrixStack.clear();
    _matrixStack.push_back(osg::Matrix::identity());
}

void ApplyUserNodeVisitor::start(ApplyMode mode, osg::Node* node)
{
    setMode(mode);
    if (node && node->getNumParents())
    {
        _upperMatrix = osg::computeLocalToWorld(node->getParent(0)->getParentalNodePaths()[0]);
        node->accept(*this);
    }
}

void ApplyUserNodeVisitor::computeBoundBox(osg::Geometry* geometry)
{
#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
    const osg::BoundingBox& dbb = geometry->getBoundingBox();
#else
    const osg::BoundingBox& dbb = geometry->getBound();
#endif
    if (dbb.valid())
    {
        const osg::Matrix& matrix = _matrixStack.back();
        _bb.expandBy(dbb.corner(0) * matrix);
        _bb.expandBy(dbb.corner(1) * matrix);
        _bb.expandBy(dbb.corner(2) * matrix);
        _bb.expandBy(dbb.corner(3) * matrix);
        _bb.expandBy(dbb.corner(4) * matrix);
        _bb.expandBy(dbb.corner(5) * matrix);
        _bb.expandBy(dbb.corner(6) * matrix);
        _bb.expandBy(dbb.corner(7) * matrix);
    }
}

void ApplyUserNodeVisitor::computeNormalizedVertex(osg::Geometry* geometry)
{
    osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    if (!va) return;

    double xMax = _bb.xMax(), yMax = _bb.yMax(), zMax = _bb.zMax();
    double xMin = _bb.xMin(), yMin = _bb.yMin(), zMin = _bb.zMin();
    double xRangeInv = osg::equivalent(xMax, xMin) ? 0.0 : 1.0 / (xMax - xMin);
    double yRangeInv = osg::equivalent(yMax, yMin) ? 0.0 : 1.0 / (yMax - yMin);
    double zRangeInv = osg::equivalent(zMax, zMin) ? 0.0 : 1.0 / (zMax - zMin);

    osg::Matrix matrix = _matrixStack.back();
    VertexList& list = _normVertexMap[geometry];
    list.resize(va->size());
    for (unsigned int i = 0; i < va->size(); ++i)
    {
        osg::Vec3 pt = (*va)[i] * matrix;
        list[i] = osg::Vec3((pt[0] - xMin) * xRangeInv,
                            (pt[1] - yMin) * yRangeInv, (pt[2] - zMin) * zRangeInv);
    }
}

void ApplyUserNodeVisitor::computeNewVertex(osg::Geometry* geometry)
{
    osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    if (!va || !_volume) return;

    osg::Matrix invMatrix = osg::Matrix::inverse(_upperMatrix * _matrixStack.back());
    VertexList& list = _normVertexMap[geometry];

    unsigned int minSize = osg::minimum(va->size(), list.size());
    for (unsigned int i = 0; i < minSize; ++i)
    {
        const osg::Vec3& param = list[i];
        (*va)[i] = _volume->getPosition(param[0], param[1], param[2]) * invMatrix;
    }

    if (geometry->getUseVertexBufferObjects()) { va->dirty(); }
#ifdef OSG_USE_DEPRECATED_API
    if (geometry->getUseDisplayList()) { geometry->dirtyDisplayList(); }
#endif
    geometry->dirtyBound();
}

/* FFDModeler */

FFDModeler::FFDModeler()
{
    _gridColor.set(1.0f, 1.0f, 1.0f, 1.0f);
    _ffdGeom = new osg::Geometry;
    _ffdGeom->setUseDisplayList(false);
    _ffdGeom->setUseVertexBufferObjects(true);
}

FFDModeler::~FFDModeler()
{
}

void FFDModeler::setQuantity(int u, int v, int w)
{
    if (u < 2 || v < 2 || w < 2) return;
    _quantity[0] = u; _quantity[1] = v; _quantity[2] = w;
    _volume = new BSplineVolume(u, v, w, u - 1, v - 1, w - 1);
    _userNodeVisitor.setBSplineVolume(_volume.get());
    allocateFFDGeometry(u, v, w);
}

void FFDModeler::setNode(osg::Node* node)
{
    _node = node;
    if (node && node->getNumParents())
    {
        _userNodeVisitor.reset(true, true);
        _userNodeVisitor.start(ApplyUserNodeVisitor::REQ_BOUND, node);
        _userNodeVisitor.reset(false, false);
        _userNodeVisitor.start(ApplyUserNodeVisitor::REQ_NORMV, node);

        osg::BoundingBox localBB = _userNodeVisitor.getBoundingBox(), bb;
        osg::Matrix localToWorld = osg::computeLocalToWorld(node->getParent(0)->getParentalNodePaths()[0]);
        for (unsigned int i = 0; i < 8; ++i) bb.expandBy(localBB.corner(i) * localToWorld);
        if (_volume.valid() && _ffdGeom.valid()) comupteFFDBox(bb);
    }
}

void FFDModeler::setCtrlPoint(int u, int v, int w, const osg::Vec3& pt)
{
    if (_volume.valid())
        _volume->setControlPoint(BSplineVolume::VolumeIndex(u, v, w), pt);
    if (_ffdGeom.valid())
    {
        osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(_ffdGeom->getVertexArray());
        if (vertices)
        {
            (*vertices)[w * _quantity[1] * _quantity[0] + v * _quantity[0] + u] = pt;
            vertices->dirty();
            _ffdGeom->dirtyBound();
        }
    }

    if (_node.valid())
    {
        _userNodeVisitor.reset(false, false);
        _userNodeVisitor.start(ApplyUserNodeVisitor::REQ_SETV, _node.get());
    }
}

osg::Vec3 FFDModeler::getCtrlPoint(int u, int v, int w) const
{
    if (_volume.valid())
        return _volume->getControlPoint(BSplineVolume::VolumeIndex(u, v, w));
    return osg::Vec3();
}

float FFDModeler::selectOnCtrlBox(float mx, float my, const osg::Matrix& vpw, const osg::Vec4& color,
                                  int& u, int& v, int& w, float precision)
{
    osg::Vec2 mouse(mx, my);
    if (!_ffdGeom) return -1.0f;

    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_ffdGeom->getVertexArray());
    if (!va) return -1.0f;

    bool hasResult = false;
    float bestDistance = precision, bestZ = FLT_MAX;
    for (int i0 = 0; i0 < _quantity[0]; ++i0)
    {
        for (int i1 = 0; i1 < _quantity[1]; ++i1)
        {
            for (int i2 = 0; i2 < _quantity[2]; ++i2)
            {
                unsigned int index = i2 * _quantity[1] * _quantity[0] + i1 * _quantity[0] + i0;
                osg::Vec3 win = (*va)[index] * vpw;
                float distance = (osg::Vec2(win[0], win[1]) - mouse).length();
                if (distance <= bestDistance && win[2] <= bestZ)
                {
                    w = i2; v = i1; u = i0; bestDistance = distance;
                    bestZ = win[2]; hasResult = true;
                }
            }
        }
    }

    if (hasResult)
    {
        osg::ref_ptr<osg::Vec4Array> colors = static_cast<osg::Vec4Array*>(_ffdGeom->getColorArray());
        if (colors)
        {
            (*colors)[w * _quantity[1] * _quantity[0] + v * _quantity[0] + u] = color;
            colors->dirty();
        }
        return bestZ;
    }
    return -1.0f;
}

void FFDModeler::setFFDGridColor(const osg::Vec4& color)
{
    osg::ref_ptr<osg::Vec4Array> colors = static_cast<osg::Vec4Array*>(_ffdGeom->getColorArray());
    if (colors)
    {
        for (unsigned int i = 0; i < colors->size(); ++i) (*colors)[i] = color;
        colors->dirty(); _gridColor = color;
    }
}

void FFDModeler::comupteFFDBox(const osg::BoundingBox& bb)
{
    double xRange = bb.xMax() - bb.xMin();
    double yRange = bb.yMax() - bb.yMin();
    double zRange = bb.zMax() - bb.zMin();
    double dx = xRange / (double)(_quantity[0] - 1);
    double dy = yRange / (double)(_quantity[1] - 1);
    double dz = zRange / (double)(_quantity[2] - 1);

    osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(_ffdGeom->getVertexArray());
    if (!vertices) return;

    osg::Vec3 vertex;
    for (int i0 = 0; i0 < _quantity[0]; ++i0)
    {
        vertex[0] = bb.xMin() + dx * (double)i0;
        for (int i1 = 0; i1 < _quantity[1]; ++i1)
        {
            vertex[1] = bb.yMin() + dy * (double)i1;
            for (int i2 = 0; i2 < _quantity[2]; ++i2)
            {
                vertex[2] = bb.zMin() + dz * (double)i2;
                _volume->setControlPoint(BSplineVolume::VolumeIndex(i0, i1, i2), vertex);
                (*vertices)[i2 * _quantity[1] * _quantity[0] + i1 * _quantity[0] + i0] = vertex;
            }
        }
    }
    vertices->dirty();
    _ffdGeom->dirtyBound();
}

void FFDModeler::allocateFFDGeometry(int u, int v, int w)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(u * v * w);
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(u * v * w);
    for (unsigned int i = 0; i < colors->size(); ++i) (*colors)[i] = _gridColor;

    _ffdGeom->removePrimitiveSet(0, _ffdGeom->getNumPrimitiveSets());
    _ffdGeom->setVertexArray(vertices.get());
    _ffdGeom->setColorArray(colors.get());
    _ffdGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    std::set< std::pair<int, int> > linePairs;
    for (int i2 = 0; i2 < w; ++i2)
    {
        for (int i1 = 0; i1 < v; ++i1)
        {
            for (int i0 = 0; i0 < u; ++i0)
            {
                int index0 = i2 * u * v + i1 * u + i0;
                if (i0 < u - 1)
                {
                    int index1 = i2 * u * v + i1 * u + (i0 + 1);
                    linePairs.insert(std::pair<int, int>(index0, index1));
                }
                if (i1 < v - 1)
                {
                    int index2 = i2 * u * v + (i1 + 1) * u + i0;
                    linePairs.insert(std::pair<int, int>(index0, index2));
                }
                if (i2 < w - 1)
                {
                    int index3 = (i2 + 1) * u * v + i1 * u + i0;
                    linePairs.insert(std::pair<int, int>(index0, index3));
                }
            }
        }
    }

    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_LINES);
    for (std::set< std::pair<int, int> >::iterator itr = linePairs.begin();
        itr != linePairs.end(); ++itr)
    {
        de->push_back(itr->first);
        de->push_back(itr->second);
    }
    _ffdGeom->addPrimitiveSet(de.get());
    _ffdGeom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
}
