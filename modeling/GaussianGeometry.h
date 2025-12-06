#ifndef MANA_MODELING_GAUSSIANGEOMETRY
#define MANA_MODELING_GAUSSIANGEOMETRY

#include <osg/Version>
#include <osg/Geometry>
#include <set>

namespace osgVerse
{

/** Gaussian Data:
   - Position (vec3): getVertexArray()
   - Scale + Rotation (CovMatrix): getVertexAttribArray(1,2,3)
   - Alpha (float): getVertexAttribArray(1).a()
   - Spherical harmonics coefficients
     - R-channel (dc + 15 rests): getVertexAttribArray(4,7,10,13)
     - G-channel (dc + 15 rests): getVertexAttribArray(5,8,11,14)
     - B-channel (dc + 15 rests): getVertexAttribArray(6,9,12,15)
*/
class GaussianGeometry : public osg::Geometry
{
public:
    using osg::Geometry::AttributeBinding;
    GaussianGeometry();
    GaussianGeometry(const GaussianGeometry& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
    virtual osg::BoundingSphere computeBound() const;
    virtual osg::BoundingBox computeBoundingBox() const;
#else
    virtual osg::BoundingBox computeBound() const;
#endif

    static osg::Program* createProgram(osg::Shader* vs, osg::Shader* gs, osg::Shader* fs);
    static osg::NodeCallback* createUniformCallback();

    void setShDegrees(int d) { _degrees = d; }
    int getShDegrees() const { return _degrees; }

    void setPosition(osg::Vec3Array* v) { setVertexArray(v); }
    void setScaleAndRotation(osg::Vec3Array* v, osg::Vec4Array* q, osg::FloatArray* a);
    void setShRed(int i, osg::Vec4Array* v) { setVertexAttribArray(4 + i * 3, v); setVertexAttribBinding(4 + i * 3, BIND_PER_VERTEX); }
    void setShGreen(int i, osg::Vec4Array* v) { setVertexAttribArray(5 + i * 3, v); setVertexAttribBinding(5 + i * 3, BIND_PER_VERTEX); }
    void setShBlue(int i, osg::Vec4Array* v) { setVertexAttribArray(6 + i * 3, v); setVertexAttribBinding(6 + i * 3, BIND_PER_VERTEX); }

    osg::Vec3Array* getPosition() { return static_cast<osg::Vec3Array*>(getVertexArray()); }
    osg::Vec3Array* getCovariance0() { return static_cast<osg::Vec3Array*>(getVertexAttribArray(1)); }
    osg::Vec3Array* getCovariance1() { return static_cast<osg::Vec3Array*>(getVertexAttribArray(2)); }
    osg::Vec3Array* getCovariance2() { return static_cast<osg::Vec3Array*>(getVertexAttribArray(3)); }
    osg::Vec4Array* getShRed(int index) { return static_cast<osg::Vec4Array*>(getVertexAttribArray(4 + index * 3)); }
    osg::Vec4Array* getShGreen(int index) { return static_cast<osg::Vec4Array*>(getVertexAttribArray(5 + index * 3)); }
    osg::Vec4Array* getShBlue(int index) { return static_cast<osg::Vec4Array*>(getVertexAttribArray(6 + index * 3)); }

protected:
    virtual ~GaussianGeometry() {}
    int _degrees;
};

/** Gaussian sorter */
class GaussianSorter : public osg::Referenced
{
public:
    GaussianSorter(int numThreads = 1) : _method(CPU_SORT), _firstFrame(true)
    { configureThreads(numThreads); }

    void cull(const osg::Matrix& view);
    void configureThreads(int numThreads);

    enum Method
    {
        CPU_SORT, GL46_RADIX_SORT, USER_SORT
    };
    void setMethod(Method m) { _method = m; }
    Method getMethod() const { return _method; }

    struct Sorter : public osg::Referenced
    {
        virtual void sort(osg::DrawElementsUInt* indices, osg::Vec3Array* pos,
                          const osg::Matrix& model, const osg::Matrix& view) = 0;
    };
    void setSortCallback(Sorter* s) { _sortCallback = s; }
    Sorter* getSortCallback() { return _sortCallback.get(); }

    void addGeometry(GaussianGeometry* geom);
    void removeGeometry(GaussianGeometry* geom);
    void clear() { _geometries.clear(); }

protected:
    virtual ~GaussianSorter() { configureThreads(0); }
    virtual void cull(GaussianGeometry* geom, const osg::Matrix& model, const osg::Matrix& view);

    std::set<osg::ref_ptr<GaussianGeometry>> _geometries;
    std::vector<OpenThreads::Thread*> _sortThreads;
    osg::ref_ptr<Sorter> _sortCallback;
    Method _method; bool _firstFrame;
};

}

#endif
