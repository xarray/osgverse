#ifndef MANA_MODELING_OCTREE_HPP
#define MANA_MODELING_OCTREE_HPP

#include <osg/io_utils>
#include <osg/Math>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

namespace osgVerse
{
    template<typename T>
    class BoundsOctreeNode
    {
        struct OctreeObject
        {
            osg::ref_ptr<T> object;
            osg::BoundingBoxd bounds;
        };

    public:
        BoundsOctreeNode(float bLength = 1.0f, float mSize = 1.0f, float lVal = 1.0f,
                         const osg::Vec3d& cVal = osg::Vec3d())
        {
            _numObjectsAllowed = 8;
            setValues(bLength, mSize, lVal, cVal);
        }

        BoundsOctreeNode& operator=(const BoundsOctreeNode& rhs)
        {
            center = rhs.center; baseLength = rhs.baseLength;
            _objects = rhs._objects; _children = rhs._children;
            _childBounds = rhs._childBounds; _bounds = rhs._bounds;
            _looseness = rhs._looseness; _minSize = rhs._minSize;
            _adjLength = rhs._adjLength; return *this;
        }

        void setValues(float baseLen, float minSizeVal, float lVal, const osg::Vec3d& centerVal)
        {
            center = centerVal; baseLength = baseLen;
            _minSize = minSizeVal; _looseness = lVal;
            _adjLength = _looseness * baseLength;

            // Create the bounding box
            osg::Vec3d size = osg::Vec3d(_adjLength, _adjLength, _adjLength);
            setBoundData(_bounds, center, size); _childBounds.resize(8);

            float quarter = baseLength / 4.f, actualLength = (baseLength / 2.f) * _looseness;
            osg::Vec3d childActualSize = osg::Vec3d(actualLength, actualLength, actualLength);
            setBoundData(_childBounds[0], center + osg::Vec3d(-quarter, quarter, -quarter), childActualSize);
            setBoundData(_childBounds[1], center + osg::Vec3d(quarter, quarter, -quarter), childActualSize);
            setBoundData(_childBounds[2], center + osg::Vec3d(-quarter, quarter, quarter), childActualSize);
            setBoundData(_childBounds[3], center + osg::Vec3d(quarter, quarter, quarter), childActualSize);
            setBoundData(_childBounds[4], center + osg::Vec3d(-quarter, -quarter, -quarter), childActualSize);
            setBoundData(_childBounds[5], center + osg::Vec3d(quarter, -quarter, -quarter), childActualSize);
            setBoundData(_childBounds[6], center + osg::Vec3d(-quarter, -quarter, quarter), childActualSize);
            setBoundData(_childBounds[7], center + osg::Vec3d(quarter, -quarter, quarter), childActualSize);
        }

        void getChildBounds(std::vector<osg::BoundingBoxd>& bounds) const
        {
            for (size_t i = 0; i < _children.size(); ++i)
                _children[i].getChildBounds(bounds);
            bounds.push_back(_bounds);
        }

        bool add(T* obj, const osg::BoundingBoxd& objBounds)
        {
            if (!encapsulates(_bounds, objBounds)) return false;
            subAdd(obj, objBounds); return true;
        }

        bool remove(T* obj)
        {
            bool removed = false;
            for (size_t i = 0; i < _objects.size(); ++i)
            {
                if (_objects[i].object == obj)
                {
                    _objects.erase(_objects.begin() + i);
                    removed = true; break;
                }
            }

            if (!removed && !_children.empty())
            {
                for (size_t i = 0; i < _children.size(); ++i)
                {
                    removed = _children[i].remove(obj);
                    if (removed) break;
                }
            }
            if (removed && !_children.empty())
                if (shouldMerge()) merge();
            return removed;
        }

        bool remove(T* obj, const osg::BoundingBoxd& objBounds)
        {
            if (!encapsulates(_bounds, objBounds)) return false;
            return subRemove(obj, objBounds);
        }

        bool isColliding(const osg::BoundingBoxd& checkBounds) const
        {
            if (!_bounds.intersects(checkBounds)) return false;
            for (size_t i = 0; i < _objects.size(); ++i)
            { if (_objects[i].bounds.intersects(checkBounds)) return true; }

            for (size_t i = 0; i < _children.size(); ++i)
            { if (_children[i].isColliding(checkBounds)) return true; }
            return false;
        }

        void getColliding(const osg::BoundingBoxd& checkBounds,
                          std::vector<osg::ref_ptr<T>>& result) const
        {
            if (!_bounds.intersects(checkBounds)) return;
            for (size_t i = 0; i < _objects.size(); ++i)
            {
                if (_objects[i].bounds.intersects(checkBounds))
                    result.push_back(_objects[i].object);
            }
            for (size_t i = 0; i < _children.size(); ++i)
                _children[i].getColliding(checkBounds, result);
        }

        void setChildren(const std::vector<BoundsOctreeNode<T>>& childOctrees)
        { if (childOctrees.size() == 8) _children = childOctrees; }

        BoundsOctreeNode<T> shrinkIfPossible(float minLength)
        {
            if (baseLength < (2 * minLength)) return this;
            if (_objects.size() == 0 && _children.size() == 0) return this;

            int bestFit = -1, newBestFit = -1;
            for (size_t i = 0; i < _objects.size(); ++i)
            {
                OctreeObject curObj = _objects[i];
                newBestFit = bestFitChild(curObj.bounds.center());
                if (i == 0 || newBestFit == bestFit)
                {   // In same octant as the other(s). Does it fit completely inside that octant?
                    if (!encapsulates(_childBounds[newBestFit], curObj.bounds)) return this;
                    else { if (bestFit < 0) bestFit = newBestFit; }
                }
                else return this;
            }

            // Check objects in children if there are any
            bool childHadContent = false;
            for (size_t i = 0; i < _children.size(); ++i)
            {
                if (_children[i].hasAnyObjects())
                {
                    if (childHadContent) return this;
                    if (bestFit >= 0 && bestFit != i) return this;
                    childHadContent = true; bestFit = i;
                }
            }

            if (_children.empty())
            {   // We don't have any children, so just shrink this node to the new size
                // We already know that everything will still fit in it
                setValues(baseLength / 2, _minSize, _looseness, _childBounds[bestFit].center());
                return this;
            }
            if (bestFit == -1) return this;  // No objects in entire octree
            return _children[bestFit];
        }

        int bestFitChild(const osg::Vec3d& objBoundsCenter) const
        {
            return (objBoundsCenter.x() <= center.x() ? 0 : 1)
                 + (objBoundsCenter.y() >= center.y() ? 0 : 4)
                 + (objBoundsCenter.z() <= center.z() ? 0 : 2);
        }

        bool hasAnyObjects() const
        {
            if (_objects.size() > 0) return true;
            for (size_t i = 0; i < _children.size(); ++i)
            { if (_children[i].hasAnyObjects()) return true; }
            return false;
        }

        const std::vector<BoundsOctreeNode<T>>& getChildren() const { return _children; }
        std::vector<OctreeObject>& getObjects() { return _objects; }
        osg::Vec3d center;
        float baseLength;

    private:
        void subAdd(T* obj, const osg::BoundingBoxd& objBounds)
        {   // We always put things in the deepest possible child
            // So we can skip some checks if there are children already
            if (!hasChildren())
            {   // Just add if few objects are here, or children would be below min size
                if (_objects.size() < _numObjectsAllowed || (baseLength / 2.f) < _minSize)
                {
                    OctreeObject newObj; newObj.object = obj, newObj.bounds = objBounds;
                    _objects.push_back(newObj); return;
                }

                // Fits at this level, but we can go deeper. Would it fit there? Create the 8 children
                if (_children.empty())
                {
                    split(); if (_children.empty()) return;
                    for (int i = (int)_objects.size() - 1; i >= 0; i--)
                    {   // Now with new children, see if this node's existing objects would fit
                        OctreeObject existingObj = _objects[i];

                        // Find which child the object is closest to based on where the
                        // object's center is located in relation to the octree's center
                        int bestFitID = bestFitChild(existingObj.bounds.center());
                        if (encapsulates(_children[bestFitID]._bounds, existingObj.bounds))
                        {
                            _children[bestFitID].subAdd(existingObj.object, existingObj.bounds);
                            _objects.erase(_objects.begin() + i); // Remove from here
                        }
                    }
                }
            }

            // Handle the new object we're adding now
            int bestFit = bestFitChild(objBounds.center());
            if (encapsulates(_children[bestFit]._bounds, objBounds))
                _children[bestFit].subAdd(obj, objBounds);
            else
            {   // Didn't fit in a child. We'll have to it to this node instead
                OctreeObject newObj; newObj.object = obj, newObj.bounds = objBounds;
                _objects.push_back(newObj);
            }
        }

        bool subRemove(T* obj, const osg::BoundingBoxd& objBounds)
        {
            bool removed = false;
            for (size_t i = 0; i < _objects.size(); ++i)
            {
                if (_objects[i].object == obj)
                {
                    _objects.erase(_objects.begin() + i);
                    removed = true; break;
                }
            }

            if (!removed && !_children.empty())
            {
                int bestFitID = bestFitChild(objBounds.center());
                removed = _children[bestFitID].subRemove(obj, objBounds);
            }

            if (removed && !_children.empty())
            {   // Check if we should merge nodes now that we've removed an item
                if (shouldMerge()) merge();
            }
            return removed;
        }

        void split()
        {
            float quarter = baseLength / 4.f;
            float newLength = baseLength / 2.f;
            _children.resize(8);
            _children[0] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(-quarter, quarter, -quarter));
            _children[1] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(quarter, quarter, -quarter));
            _children[2] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(-quarter, quarter, quarter));
            _children[3] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(quarter, quarter, quarter));
            _children[4] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(-quarter, -quarter, -quarter));
            _children[5] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(quarter, -quarter, -quarter));
            _children[6] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(-quarter, -quarter, quarter));
            _children[7] = BoundsOctreeNode<T>(
                newLength, _minSize, _looseness,
                center + osg::Vec3d(quarter, -quarter, quarter));
        }

        void merge()
        {   // Note: We know children != null or we wouldn't be merging
            for (size_t i = 0; i < _children.size(); ++i)
            {
                BoundsOctreeNode<T>& curChild = _children[i];
                int numObjects = (int)curChild._objects.size();
                for (int j = numObjects - 1; j >= 0; j--)
                {
                    OctreeObject curObj = curChild._objects[j];
                    _objects.push_back(curObj);
                }
            }
            _children.clear();
        }

        bool shouldMerge() const
        {
            size_t totalObjects = _objects.size();
            for (size_t i = 0; i < _children.size(); ++i)
            {
                BoundsOctreeNode<T>& child = _children[i];
                if (!child._children.empty())
                {
                    // If any of the *children* have children, there are definitely too many to merge,
                    // or the child would have been merged already
                    return false;
                }
                totalObjects += child._objects.size();
            }
            return totalObjects <= _numObjectsAllowed;
        }
        
        void setBoundData(osg::BoundingBoxd& bb, const osg::Vec3d& c, const osg::Vec3d& s)
        { osg::Vec3d half = s * 0.5; bb._min = c - half; bb._max = c + half; }

        bool encapsulates(const osg::BoundingBoxd& outBounds,
                          const osg::BoundingBoxd& inBounds) const
        { return outBounds.contains(inBounds._min) && outBounds.contains(inBounds._max); }

        bool hasChildren() const
        { return !_children.empty(); }

        std::vector<OctreeObject> _objects;
        std::vector<BoundsOctreeNode<T>> _children;
        std::vector<osg::BoundingBoxd> _childBounds;
        osg::BoundingBoxd _bounds;
        float _looseness, _minSize, _adjLength;
        int _numObjectsAllowed;
    };

    template<typename T>
    class BoundsOctree
    {
    public:
        BoundsOctree(const osg::Vec3d& initialWorldPos = osg::Vec3d(), float initialWorldSize = 50.0f,
                     float minNodeSize = 1.0f, float loosenessVal = 1.0f)
        {
            if (minNodeSize > initialWorldSize)
            {
                OSG_WARN << "[BoundsOctree] Minimum node size must be at least "
                         << "as big as the initial world size." << std::endl;
            }
            _count = 0; _initialSize = initialWorldSize; _minSize = minNodeSize;
            _looseness = osg::clampBetween(loosenessVal, 1.0f, 2.0f);
            _rootNode = BoundsOctreeNode<T>(_initialSize, _minSize, _looseness, initialWorldPos);
        }

        void add(T* obj, const osg::BoundingBoxd& objBounds)
        {   // Add object or expand the octree until it can be added
            int count = 0;
            while (!_rootNode.add(obj, objBounds))
            {
                grow(objBounds.center() - _rootNode.center);
                if (++count > 20) break;  // deadlock??
            }
            _count++;
        }

        bool remove(T* obj)
        {
            bool removed = _rootNode.remove(obj);
            if (removed) { _count--; shrink(); }
            return removed;
        }

        bool remove(T* obj, const osg::BoundingBoxd& objBounds)
        {
            bool removed = _rootNode.remove(obj, objBounds);
            if (removed) { _count--; shrink(); }
            return removed;
        }

        bool isColliding(const osg::BoundingBoxd& checkBounds) const
        { return _rootNode.isColliding(checkBounds); }

        std::vector<osg::ref_ptr<T>> getColliding(const osg::BoundingBoxd& checkBounds) const
        {
            std::vector<osg::ref_ptr<T>> colliding;
            _rootNode.GetColliding(checkBounds, colliding);
            return colliding;
        }

        const BoundsOctreeNode<T>& getRoot() const { return _rootNode; }
        BoundsOctreeNode<T>& getRoot() { return _rootNode; }
        osg::BoundingBoxd getMaxBounds() const { return _rootNode.bound; }
        int getMaxCount() const { return _count; }

        std::vector<osg::BoundingBoxd> getChildBounds() const
        {
            std::vector<osg::BoundingBoxd> boundsList;
            _rootNode.getChildBounds(boundsList); return boundsList;
        }

    private:
        void grow(const osg::Vec3d& direction)
        {
            BoundsOctreeNode<T> oldRoot = _rootNode;
            int xDirection = direction.x() >= 0 ? 1 : -1;
            int yDirection = direction.y() >= 0 ? 1 : -1;
            int zDirection = direction.z() >= 0 ? 1 : -1;
            float half = _rootNode.baseLength / 2;
            float newLength = _rootNode.baseLength * 2;
            osg::Vec3d newCenter = _rootNode.center + osg::Vec3d(
                xDirection * half, yDirection * half, zDirection * half);

            _rootNode = BoundsOctreeNode<T>(newLength, _minSize, _looseness, newCenter);
            if (oldRoot.hasAnyObjects())
            {   // Create 7 new octree children to go with the old root as children of the new root
                int rootPos = _rootNode.bestFitChild(oldRoot.center);
                std::vector<BoundsOctreeNode<T>> children(8);
                for (int i = 0; i < 8; ++i)
                {
                    if (i == rootPos)
                        children[i] = oldRoot;
                    else
                    {
                        xDirection = i % 2 == 0 ? -1 : 1;
                        yDirection = i > 3 ? -1 : 1;
                        zDirection = (i < 2 || (i > 3 && i < 6)) ? -1 : 1;
                        children[i].setValues(oldRoot.baseLength, _minSize, _looseness,
                            newCenter + osg::Vec3d(xDirection * half, yDirection * half, zDirection * half));
                    }
                }
                _rootNode.setChildren(children);
            }
        }

        void shrink() { _rootNode = _rootNode.shrinkIfPossible(_initialSize); }
        BoundsOctreeNode<T> _rootNode;
        float _looseness, _initialSize, _minSize;
        int _count;
    };
}

#endif
