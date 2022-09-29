#include "NodeSelector.h"
#include <osg/io_utils>
#include <osg/AnimationPath>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geode>
#include <algorithm>
#include <iostream>
using namespace osgVerse;

/* NodeSelector::BoundUpdater */

bool NodeSelector::BoundUpdater::removeTarget(osg::Node* t)
{
    osg::NodePath::iterator itr = std::find(_targets.begin(), _targets.end(), t);
    if (itr != _targets.end())
    {
        _targets.erase(itr);
        if (!_targets.size()) return false;
    }
    return true;
}

void NodeSelector::BoundUpdater::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    osg::BoundingBox totalBound;
    osg::Camera* mainCamera = _picker->getMainCamera();
    for (unsigned int i = 0; i < _targets.size(); ++i)
    {
        osg::Node* target = _targets[i];
        if (target && target->referenceCount() > 0)
        {
            osg::Matrix worldMatrix;
            if (target->getNumParents() > 0)
                worldMatrix = target->getParent(0)->getWorldMatrices().front();
            switch (_picker->getBoundType())
            {
            case NodeSelector::BOUND_RECTANGLE: case NodeSelector::BOUND_SQUARE:
                if (mainCamera)
                {
                    // Set the matrix to [-1, 1] for HUD billboards
                    worldMatrix = worldMatrix * mainCamera->getViewMatrix() *
                        mainCamera->getProjectionMatrix();
                }
                break;
            default: break;
            }

            switch (_picker->getComputationMethod())
            {
            case USE_NODE_BBOX:
            {
                osg::ComputeBoundsVisitor cbbv;
                cbbv.pushMatrix(worldMatrix);
                target->accept(cbbv);

                osg::BoundingBox bb = cbbv.getBoundingBox();
                if (bb.valid()) totalBound.expandBy(bb);
                else
                {
                    OSG_INFO << "[NodeSelector::BoundUpdater] cannot compute bound of target "
                        << target->getName() << std::endl;
                }
            }
            break;
            case USE_NODE_BSPHERE:
            {
                osg::Vec3 pt = target->getBound().center();
                osg::BoundingSphere bs(pt * worldMatrix, 0.0f);

                pt = pt + osg::X_AXIS * target->getBound().radius();
                bs.expandRadiusBy(pt * worldMatrix);
                if (bs.valid()) totalBound.expandBy(bs);
                else
                {
                    OSG_INFO << "[NodeSelector::BoundUpdater] cannot compute bound of target "
                        << target->getName() << std::endl;
                }
            }
            break;
            default: break;
            }
        }
        else
        {
            OSG_NOTICE << "[NodeSelector::BoundUpdater] found invalid bound target "
                << target->getName() << std::endl;
        }
    }

    osg::MatrixTransform* mt = static_cast<osg::MatrixTransform*>(node);
    if (totalBound.valid())
    {
        osg::Vec3 center = totalBound.center(), scaleVector = totalBound._max - totalBound._min;
        switch (_picker->getBoundType())
        {
        case NodeSelector::BOUND_RECTANGLE:
            center.z() = 0.0f; scaleVector.z() = 1.0f;
            break;
        case NodeSelector::BOUND_SQUARE:
            center.z() = 0.0f;
            {
                float aspectRatio = (mainCamera && mainCamera->getViewport()) ?
                    mainCamera->getViewport()->aspectRatio() : 1.0f;
                float scaleValue = osg::maximum(scaleVector.x(), scaleVector.y());
                scaleVector.set(scaleValue / aspectRatio, scaleValue, 1.0f);
            }
            break;
        default: break;
        }
        mt->setMatrix(osg::Matrix::scale(scaleVector) * osg::Matrix::translate(center));
    }
    traverse(node, nv);
}

/* NodeSelector */

NodeSelector::NodeSelector()
    : _selectorColor(1.0f, 1.0f, 1.0f, 1.0f), _boundColor(1.0f, 1.0f, 1.0f, 1.0f),
    _boundDeltaLength(0.2f), _computationMethod(USE_NODE_BBOX),
    _selectorType(SINGLE_SELECTOR), _boundType(BOUND_BOX)
{
    _hudRoot = new osg::Group;
    _auxiliaryRoot = new osg::Group;
    _auxiliaryRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    rebuildSelectorGeometry();
    rebuildBoundGeometry();
}

bool NodeSelector::event(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::DRAG:
        moveLastSelectorPoint(ea.getXnormalized(), ea.getYnormalized());
        break;
    case osgGA::GUIEventAdapter::PUSH:
        addSelectorPoint(ea.getXnormalized(), ea.getYnormalized(), true);
        break;
    case osgGA::GUIEventAdapter::RELEASE:
        clearAllSelectorPoints();
        break;
    default: break;
    }
    return false;
}

bool NodeSelector::obtainSelectorData(unsigned int index, osg::Vec3& start, osg::Vec3& end)
{
    osg::Matrix invViewProj;
    if (_mainCamera.valid())
    {
        invViewProj = osg::Matrix::inverse(
            _mainCamera->getViewMatrix() * _mainCamera->getProjectionMatrix());
    }

    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    if (index < va->size())
    {
        const osg::Vec3& v = (*va)[index];
        start = osg::Vec3(v.x(), v.y(), -1.0f) * invViewProj;
        end = osg::Vec3(v.x(), v.y(), 1.0f) * invViewProj;
        return true;
    }
    return false;
}

bool NodeSelector::obtainSelectorData(osg::Polytope& polytope)
{
    osg::Matrix viewProj;
    if (_mainCamera.valid())
    {
        viewProj = _mainCamera->getViewMatrix() * _mainCamera->getProjectionMatrix();
    }

    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    unsigned int size = va->size();
    if (_selectorType == RECTANGLE_SELECTOR || _selectorType == POLYGON_SELECTOR)
    {
        osg::Polytope projPolytope;
        for (unsigned int i = 0; i < size; ++i)
        {
            const osg::Vec3& v1 = (*va)[i];
            const osg::Vec3& v2 = (*va)[(i + 1) % size];
            osg::Vec3 v3 = v2 + osg::Vec3(0.0f, 0.0f, (v1 - v2).length());
            projPolytope.add(osg::Plane(v1, v2, v3));
        }
        projPolytope.add(osg::Plane(0.0, 0.0, 1.0, 1.0));
        polytope.setAndTransformProvidingInverse(projPolytope, viewProj);
        return true;
    }
    return false;
}

bool NodeSelector::obtainSelectorData(osg::Vec3& c1, osg::Vec3& c2, float& r1, float& r2)
{
    osg::Matrix invViewProj;
    if (_mainCamera.valid())
    {
        invViewProj = osg::Matrix::inverse(
            _mainCamera->getViewMatrix() * _mainCamera->getProjectionMatrix());
    }

    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    if (_selectorType == CIRCLE_SELECTOR)
    {
        const osg::Vec3& v0 = (*va)[0];
        c1 = osg::Vec3(v0.x(), v0.y(), -1.0f) * invViewProj;
        c2 = osg::Vec3(v0.x(), v0.y(), 1.0f) * invViewProj;

        const osg::Vec3& v1 = (*va)[1];
        osg::Vec3 p1 = osg::Vec3(v1.x(), v1.y(), -1.0f) * invViewProj;
        osg::Vec3 p2 = osg::Vec3(v1.x(), v1.y(), 1.0f) * invViewProj;
        r1 = (p1 - c1).length();
        r2 = (p2 - c2).length();
        return true;
    }
    return false;
}

bool NodeSelector::addSelectorPoint(float x, float y, bool firstPoint)
{
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    switch (_selectorType)
    {
    case SINGLE_SELECTOR: return false;
    case RECTANGLE_SELECTOR: case CIRCLE_SELECTOR:
        if (firstPoint) { (*va)[0] = osg::Vec3(x, y, 0.0f); (*va)[1] = (*va)[0]; }
        else (*va)[1] = osg::Vec3(x, y, 0.0f);
        break;
    case POLYGON_SELECTOR:
        if (firstPoint) { (*va)[0] = osg::Vec3(x, y, 0.0f); (*va)[1] = (*va)[0]; }
        else va->push_back(osg::Vec3(x, y, 0.0f));
        break;
    default: return false;
    }
    updateSelectionGeometry();
    if (firstPoint) _selectorGeode->setNodeMask(0xffffffff);
    return true;
}

bool NodeSelector::removeLastSelectorPoint()
{
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    switch (_selectorType)
    {
    case SINGLE_SELECTOR: return false;
    case RECTANGLE_SELECTOR: case CIRCLE_SELECTOR:
        (*va)[1] = (*va)[0];
        break;
    case POLYGON_SELECTOR:
        if (va->size() > 2) va->pop_back();
        break;
    default: return false;
    }
    updateSelectionGeometry();
    return true;
}

bool NodeSelector::moveLastSelectorPoint(float x, float y)
{
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    switch (_selectorType)
    {
    case SINGLE_SELECTOR: return false;
    case RECTANGLE_SELECTOR: case CIRCLE_SELECTOR:
        (*va)[1] = osg::Vec3(x, y, 0.0f);
        break;
    case POLYGON_SELECTOR:
        va->back() = osg::Vec3(x, y, 0.0f);
        break;
    default: return false;
    }
    updateSelectionGeometry();
    return true;
}

void NodeSelector::clearAllSelectorPoints()
{
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    switch (_selectorType)
    {
    case SINGLE_SELECTOR: return;
    case RECTANGLE_SELECTOR: case CIRCLE_SELECTOR: case POLYGON_SELECTOR:
        (*va)[1] = (*va)[0];
        break;
    default: return;
    }
    updateSelectionGeometry(true);
    _selectorGeode->setNodeMask(0);
}

bool NodeSelector::addSelectedNode(osg::Node* node)
{
    SelectionMap::iterator itr = _selections.find(node);
    if (node && itr == _selections.end())
    {
        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setName("NodeSelector::Bound");
        mt->setUpdateCallback(new BoundUpdater(node, this));
        mt->addChild(_boundGeode.get());
        _selections[node] = mt.get();

        switch (_boundType)
        {
        case BOUND_BOX: _auxiliaryRoot->addChild(mt.get()); break;
        case BOUND_RECTANGLE: case BOUND_SQUARE: _hudRoot->addChild(mt.get()); break;
        default: break;
        }
        return true;
    }
    return false;
}

bool NodeSelector::addSelectedNodes(const osg::NodePath& nodes, bool useSingleBound)
{
    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    unsigned int numValidNodes = 0;
    for (unsigned int i = 0; i < nodes.size(); ++i)
    {
        osg::Node* node = nodes[i];
        if (!useSingleBound)
        {
            addSelectedNode(node);
            continue;
        }

        if (!node) continue;
        removeSelectedNode(node);  // remove the node's previous bound first
        _selections[node] = mt.get();
        numValidNodes++;
    }

    if (useSingleBound && numValidNodes > 0)
    {
        mt->setName("NodeSelector::MultiBound");
        mt->setUpdateCallback(new BoundUpdater(nodes, this));
        mt->addChild(_boundGeode.get());

        switch (_boundType)
        {
        case BOUND_BOX: _auxiliaryRoot->addChild(mt.get()); break;
        case BOUND_RECTANGLE: case BOUND_SQUARE: _hudRoot->addChild(mt.get()); break;
        default: break;
        }
    }
    return true;
}

bool NodeSelector::removeSelectedNode(osg::Node* node)
{
    SelectionMap::iterator itr = _selections.find(node);
    if (itr != _selections.end())
    {
        BoundUpdater* updater = dynamic_cast<BoundUpdater*>(itr->second->getUpdateCallback());
        if (updater && updater->removeTarget(itr->first))
        {
            // If the updater still has targets, it means the bound should work for the remain
            _selections.erase(itr);
            return true;
        }

        switch (_boundType)
        {
        case BOUND_BOX: _auxiliaryRoot->removeChild(itr->second); break;
        case BOUND_RECTANGLE: case BOUND_SQUARE: _hudRoot->removeChild(itr->second); break;
        default: break;
        }
        _selections.erase(itr);
        return true;
    }
    return false;
}

bool NodeSelector::isNodeSelected(osg::Node* node, osg::Matrix& boundMatrix) const
{
    SelectionMap::const_iterator itr = _selections.find(node);
    if (itr != _selections.end())
    {
        boundMatrix = itr->second->getMatrix();
        return true;
    }
    return false;
}

void NodeSelector::clearAllSelectedNodes()
{
    switch (_boundType)
    {
    case BOUND_BOX:
        for (SelectionMap::iterator itr = _selections.begin(); itr != _selections.end(); ++itr)
            _auxiliaryRoot->removeChild(itr->second);
        break;
    case BOUND_RECTANGLE: case BOUND_SQUARE:
        for (SelectionMap::iterator itr = _selections.begin(); itr != _selections.end(); ++itr)
            _hudRoot->removeChild(itr->second);
        break;
    default: break;
    }
    _selections.clear();
}

void NodeSelector::updateSelectionGeometry(bool rebuilding)
{
    float aspectRatio = (_mainCamera.valid() && _mainCamera->getViewport()) ?
        _mainCamera->getViewport()->aspectRatio() : 1.0f;
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_selectorGeometry->getVertexArray());
    osg::DrawElementsUByte* de = static_cast<osg::DrawElementsUByte*>(_selectorGeometry->getPrimitiveSet(0));
    switch (_selectorType)
    {
    case RECTANGLE_SELECTOR:
        // Use vertex 0 and 1 as the two corners of the rectangle
        if (rebuilding)
        {
            va->resize(4, (*va)[0]);
            de->resize(4, 0);
        }
        (*va)[2] = (*va)[0]; (*va)[2].y() = (*va)[1].y();
        (*va)[3] = (*va)[0]; (*va)[3].x() = (*va)[1].x();
        (*de)[0] = 0; (*de)[1] = 2; (*de)[2] = 1; (*de)[3] = 3;
        break;
    case CIRCLE_SELECTOR:
        // Use vertex 0 as the center, and 1 as the start point on the circle
        if (rebuilding)
        {
            va->resize(33, (*va)[0]);
            de->resize(32, 0);
        }

        {
            osg::Vec3 dir = (*va)[1] - (*va)[0];
            float radius = dir.normalize(), factor = aspectRatio * aspectRatio - 1.0f;
            float cosine = dir * osg::Y_AXIS;
            float a = sqrt((radius * radius) / (1.0f + factor * cosine*cosine));
            for (unsigned int i = 1; i < 33; ++i)
            {
                float angle = osg::PI * 0.0625f * (float)(i - 1);
                (*va)[i] = osg::Vec3(a*sinf(angle), aspectRatio*a*cosf(angle), 0.0f) + (*va)[0];
                (*de)[i - 1] = i;
            }
        }
        break;
    case POLYGON_SELECTOR:
        if (rebuilding) va->resize(2, (*va)[0]);
        de->resize(va->size());
        for (unsigned int i = 0; i < va->size(); ++i) (*de)[i] = i;
        break;
    default: break;
    }
    va->dirty(); de->dirty();
    _selectorGeometry->dirtyBound();
}

void NodeSelector::rebuildSelectorGeometry(bool onlyColors)
{
    if (!_selectorGeometry)
    {
        _selectorGeometry = new osg::Geometry;
        _selectorGeometry->setUseDisplayList(false);
        _selectorGeometry->setUseVertexBufferObjects(true);
        _selectorGeometry->setVertexArray(new osg::Vec3Array(1));
        _selectorGeometry->setColorArray(new osg::Vec4Array(1));
        _selectorGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        _selectorGeometry->addPrimitiveSet(new osg::DrawElementsUByte(GL_LINE_LOOP, 1));

        _selectorGeode = new osg::Geode;
        _selectorGeode->addDrawable(_selectorGeometry.get());
        _hudRoot->addChild(_selectorGeode.get());
    }

    osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(_selectorGeometry->getColorArray());
    if (!onlyColors)
    {
        if (_selectorType == SINGLE_SELECTOR)
        {
            // Don't show the selector in single mode
            _selectorGeode->setNodeMask(0);
        }
        else
        {
            _selectorGeode->setNodeMask(0xffffffff);
            updateSelectionGeometry(true);
        }
    }

    (*ca)[0] = _selectorColor;
    ca->dirty();
}

void NodeSelector::rebuildBoundGeometry(bool onlyColors)
{
    if (!_boundGeometry)
    {
        _boundGeometry = new osg::Geometry;
        _boundGeometry->setUseDisplayList(false);
        _boundGeometry->setUseVertexBufferObjects(true);
        _boundGeometry->setVertexArray(new osg::Vec3Array(1));
        _boundGeometry->setColorArray(new osg::Vec4Array(1));
        _boundGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        _boundGeometry->addPrimitiveSet(new osg::DrawElementsUByte(GL_LINES, 1));

        _boundGeode = new osg::Geode;
        _boundGeode->addDrawable(_boundGeometry.get());
    }

    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_boundGeometry->getVertexArray());
    osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(_boundGeometry->getColorArray());
    osg::DrawElementsUByte* de = static_cast<osg::DrawElementsUByte*>(_boundGeometry->getPrimitiveSet(0));
    if (!onlyColors)
    {
        osg::Vec3 dx(_boundDeltaLength, 0.0f, 0.0f);
        osg::Vec3 dy(0.0f, _boundDeltaLength, 0.0f);
        osg::Vec3 dz(0.0f, 0.0f, _boundDeltaLength);
        switch (_boundType)
        {
        case BOUND_BOX:
            va->resize(32);
            (*va)[0] = osg::Vec3(-0.5f, -0.5f, -0.5f);
            (*va)[1] = (*va)[0] + dx; (*va)[2] = (*va)[0] + dy; (*va)[3] = (*va)[0] + dz;
            (*va)[4] = osg::Vec3(-0.5f, -0.5f, 0.5f);
            (*va)[5] = (*va)[4] + dx; (*va)[6] = (*va)[4] + dy; (*va)[7] = (*va)[4] - dz;
            (*va)[8] = osg::Vec3(-0.5f, 0.5f, -0.5f);
            (*va)[9] = (*va)[8] + dx; (*va)[10] = (*va)[8] - dy; (*va)[11] = (*va)[8] + dz;
            (*va)[12] = osg::Vec3(-0.5f, 0.5f, 0.5f);
            (*va)[13] = (*va)[12] + dx; (*va)[14] = (*va)[12] - dy; (*va)[15] = (*va)[12] - dz;
            (*va)[16] = osg::Vec3(0.5f, 0.5f, -0.5f);
            (*va)[17] = (*va)[16] - dx; (*va)[18] = (*va)[16] - dy; (*va)[19] = (*va)[16] + dz;
            (*va)[20] = osg::Vec3(0.5f, 0.5f, 0.5f);
            (*va)[21] = (*va)[20] - dx; (*va)[22] = (*va)[20] - dy; (*va)[23] = (*va)[20] - dz;
            (*va)[24] = osg::Vec3(0.5f, -0.5f, -0.5f);
            (*va)[25] = (*va)[24] - dx; (*va)[26] = (*va)[24] + dy; (*va)[27] = (*va)[24] + dz;
            (*va)[28] = osg::Vec3(0.5f, -0.5f, 0.5f);
            (*va)[29] = (*va)[28] - dx; (*va)[30] = (*va)[28] + dy; (*va)[31] = (*va)[28] - dz;

            de->resize(48);
            (*de)[0] = 0; (*de)[1] = 1; (*de)[2] = 0; (*de)[3] = 2; (*de)[4] = 0; (*de)[5] = 3;
            (*de)[6] = 4; (*de)[7] = 5; (*de)[8] = 4; (*de)[9] = 6; (*de)[10] = 4; (*de)[11] = 7;
            (*de)[12] = 8; (*de)[13] = 9; (*de)[14] = 8; (*de)[15] = 10; (*de)[16] = 8; (*de)[17] = 11;
            (*de)[18] = 12; (*de)[19] = 13; (*de)[20] = 12; (*de)[21] = 14; (*de)[22] = 12; (*de)[23] = 15;
            (*de)[24] = 16; (*de)[25] = 17; (*de)[26] = 16; (*de)[27] = 18; (*de)[28] = 16; (*de)[29] = 19;
            (*de)[30] = 20; (*de)[31] = 21; (*de)[32] = 20; (*de)[33] = 22; (*de)[34] = 20; (*de)[35] = 23;
            (*de)[36] = 24; (*de)[37] = 25; (*de)[38] = 24; (*de)[39] = 26; (*de)[40] = 24; (*de)[41] = 27;
            (*de)[42] = 28; (*de)[43] = 29; (*de)[44] = 28; (*de)[45] = 30; (*de)[46] = 28; (*de)[47] = 31;
            break;
        case BOUND_RECTANGLE: case BOUND_SQUARE:
            va->resize(12);
            (*va)[0] = osg::Vec3(-0.5f, -0.5f, 0.0f);
            (*va)[1] = (*va)[0] + dx; (*va)[2] = (*va)[0] + dy;
            (*va)[3] = osg::Vec3(0.5f, -0.5f, 0.0f);
            (*va)[4] = (*va)[3] - dx; (*va)[5] = (*va)[3] + dy;
            (*va)[6] = osg::Vec3(0.5f, 0.5f, 0.0f);
            (*va)[7] = (*va)[6] - dx; (*va)[8] = (*va)[6] - dy;
            (*va)[9] = osg::Vec3(-0.5f, 0.5f, 0.0f);
            (*va)[10] = (*va)[9] + dx; (*va)[11] = (*va)[9] - dy;

            de->resize(16);
            (*de)[0] = 0; (*de)[1] = 1; (*de)[2] = 0; (*de)[3] = 2;
            (*de)[4] = 3; (*de)[5] = 4; (*de)[6] = 3; (*de)[7] = 5;
            (*de)[8] = 6; (*de)[9] = 7; (*de)[10] = 6; (*de)[11] = 8;
            (*de)[12] = 9; (*de)[13] = 10; (*de)[14] = 9; (*de)[15] = 11;
            break;
        default: break;
        }
        va->dirty(); de->dirty();
        _boundGeometry->dirtyBound();
    }

    (*ca)[0] = _boundColor;
    ca->dirty();
}
