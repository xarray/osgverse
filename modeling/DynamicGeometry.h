#ifndef MANA_MODELING_DYNAMICGEOMETRY
#define MANA_MODELING_DYNAMICGEOMETRY

#include <osg/Version>
#include <osg/Geometry>

namespace osgVerse
{

/** The dynamic geometry base class */
class DynamicGeometry : public osg::Geometry
{
public:
    typedef std::vector<osg::Vec3> Vertices;
    DynamicGeometry();
    DynamicGeometry(const DynamicGeometry& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

    void addPoint(const osg::Vec3& p);
    void popPoint();
    void setPoint(int pos, const osg::Vec3& p);
    void insertPoint(int pos, const osg::Vec3& p);
    void insertPoints(int pos, Vertices& pts);
    void removePoints(int pos, unsigned int numToRemove = 1);

    bool isEditing() const { return _editing; }
    unsigned int size() const;
    unsigned int getPointList(Vertices& pts);
    bool getPoint(int pos, osg::Vec3& p);
    bool getCenter(osg::Vec3& p, osg::Vec3& n);

    virtual void edit();
    virtual void finish();
    virtual void cancel() { removePoints(0, getOrCreateVertexArray()->size()); }
    virtual void updatePrimitiveSet(osg::Vec3Array* va, unsigned int start = 0);

    void setColor(const osg::Vec4& color, unsigned int index = 0);
    osg::Vec4 getColor(unsigned int index = 0) const;

protected:
    virtual ~DynamicGeometry();
    unsigned int rescaleNegativeIndex(int pos, unsigned int size) { return (unsigned int)(size + pos); }

    osg::Vec3Array* getOrCreateVertexArray()
    {
        osg::Vec3Array* va = dynamic_cast<osg::Vec3Array*>(getVertexArray());
        if (!va) { va = new osg::Vec3Array; setVertexArray(va); }
        return va;
    }

    osg::Vec4Array* getOrCreateColorArray()
    {
        osg::Vec4Array* ca = dynamic_cast<osg::Vec4Array*>(getColorArray());
        if (!ca)
        {
            ca = new osg::Vec4Array;
            ca->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
            setColorArray(ca); setColorBinding(BIND_PER_PRIMITIVE_SET);
        }
        return ca;
    }

    void inheritColor(unsigned int num)
    {
        osg::Vec4Array* ca = getOrCreateColorArray();
        if (num > 0)
        {
            ca->resize(num); ca->at(num - 1) = ca->front();
#ifdef OSG_USE_DEPRECATED_API
            if (getUseDisplayList()) dirtyDisplayList(); else ca->dirty();
#endif
        }
    }

    GLenum _mode;
    bool _editing;
};

/** The dynamic point/line class */
class DynamicPointLine : public DynamicGeometry
{
public:
    DynamicPointLine(bool usePoint = false);
    DynamicPointLine(const DynamicPointLine& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

    void setPointMode(bool b) { _mode = (b ? GL_POINTS : GL_LINES); }
    bool getPointMode() const { return _mode == GL_POINTS ? true : false; }
    virtual void updatePrimitiveSet(osg::Vec3Array* va, unsigned int start = 0);

protected:
    virtual ~DynamicPointLine();
};

/** The dynamic polyline class */
class DynamicPolyline : public DynamicGeometry
{
public:
    DynamicPolyline(bool loopMode = false);
    DynamicPolyline(const DynamicPolyline& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

    void setLoopMode(bool b) { _mode = (b ? GL_LINE_LOOP : GL_LINE_STRIP); }
    bool getLoopMode() const { return _mode == GL_LINE_LOOP ? true : false; }

    virtual void finish();
    virtual void updatePrimitiveSet(osg::Vec3Array* va, unsigned int start = 0);

protected:
    virtual ~DynamicPolyline();
};

/** The polygon class */
class DynamicPolygon : public DynamicGeometry
{
public:
    DynamicPolygon();
    DynamicPolygon(const DynamicPolygon& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

    virtual void finish();
    virtual void updatePrimitiveSet(osg::Vec3Array* va, unsigned int start = 0);

protected:
    virtual ~DynamicPolygon();
};

}

#endif
