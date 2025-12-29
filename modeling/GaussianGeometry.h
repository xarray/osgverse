#ifndef MANA_MODELING_GAUSSIANGEOMETRY
#define MANA_MODELING_GAUSSIANGEOMETRY

#include <osg/Version>
#if OSG_VERSION_GREATER_THAN(3, 3, 3)
#   include <osg/VertexAttribDivisor>
#endif
#include <osg/Texture2DArray>
#include <osg/Geometry>
#include <set>

namespace osgVerse
{

/** Gaussian Data:
   - As mesh instances:
     - Original shape vertices (vec3): getVertexArray()
     - Instance Indices (uint): getVertexAttribArray(1)
     - SSBO-0 (4 floats): Position.xyz + Alpha
     - SSBO-1 (4 floats): CovMatrix.c0 + Color.r
     - SSBO-2 (4 floats): CovMatrix.c1 + Color.g
     - SSBO-3 (4 floats): CovMatrix.c2 + Color.b
     - SSBO-4 (optional, 60 floats): Spherical harmonics coefficients (alpha is ignored)

   - As mesh instances and texture look-up tables (not recommend):
     - Original shape vertices (vec3): getVertexArray()
     - Instance Indices (uint): getVertexAttribArray(1)
     - Tex2DArray-0 (4 layers): "CoreParameters"
       - Position + Alpha (vec4): layer 0
       - Scale + Rotation + Color (CovMatrix + vec3): layer 1,2,3
     - Tex2DArray-1 (optional, 15 layers): "ShParameters"
       - Spherical harmonics coefficients (vec3 * 15)

   - As vertex attributes:
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
    enum RenderMethod { INSTANCING, INSTANCING_TEXTURE, GEOMETRY_SHADER };
    GaussianGeometry(RenderMethod m = INSTANCING);
    GaussianGeometry(const GaussianGeometry& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
    virtual osg::BoundingSphere computeBound() const;
    virtual osg::BoundingBox computeBoundingBox() const;
#else
    virtual osg::BoundingBox computeBound() const;
#endif

    static osg::Program* createProgram(osg::Shader* vs, osg::Shader* gs, osg::Shader* fs, RenderMethod m = INSTANCING);
    static osg::NodeCallback* createUniformCallback();

    bool finalize();  // only run this after setting all attributes
    RenderMethod getRenderMethod() const { return _method; }
    int getNumSplats() const { return _numSplats; }

    void setShDegrees(int d) { _degrees = d; checkShaderFlag(); }
    int getShDegrees() const { return _degrees; }

    void setPosition(osg::Vec3Array* v);
    void setScaleAndRotation(osg::Vec3Array* v, osg::Vec4Array* q, osg::FloatArray* a);
    void setShRed(int i, osg::Vec4Array* v);
    void setShGreen(int i, osg::Vec4Array* v);
    void setShBlue(int i, osg::Vec4Array* v);

    osg::Vec3* getPosition3();
    osg::Vec4* getPosition4();
    osg::ref_ptr<osg::Vec3Array> getCovariance0();
    osg::ref_ptr<osg::Vec3Array> getCovariance1();
    osg::ref_ptr<osg::Vec3Array> getCovariance2();
    osg::ref_ptr<osg::Vec4Array> getShRed(int index);
    osg::ref_ptr<osg::Vec4Array> getShGreen(int index);
    osg::ref_ptr<osg::Vec4Array> getShBlue(int index);

protected:
    virtual ~GaussianGeometry() {}
    osg::BoundingBox getBounding(osg::Vec4* va) const;
    void checkShaderFlag();

    std::map<std::string, std::vector<osg::Vec4>> _preDataMap;
    std::map<std::string, std::vector<osg::Vec3>> _preDataMap2;
    osg::ref_ptr<osg::FloatArray> _coreBuffer, _shcoefBuffer;
    osg::ref_ptr<osg::Texture2DArray> _core, _shcoef;
    RenderMethod _method;
    int _degrees, _numSplats;
};

/** Gaussian sorter */
class GaussianSorter : public osg::Referenced
{
public:
    GaussianSorter(int numThreads = 1) : _method(CPU_SORT), _firstFrame(true), _onDemand(true)
    { configureThreads(numThreads); }

    void cull(const osg::Matrix& view);
    void configureThreads(int numThreads);

    enum Method { CPU_SORT, GL46_RADIX_SORT, USER_SORT };
    void setMethod(Method m) { _method = m; }
    Method getMethod() const { return _method; }

    void setOnDemand(bool b) { _onDemand = b; }
    bool getOnDemand() const { return _onDemand; }

    struct Sorter : public osg::Referenced
    {
        virtual bool sort(osg::VectorGLuint* indices, osg::Vec3* pos, size_t size,
                          const osg::Matrix& model, const osg::Matrix& view) = 0;
        virtual bool sort(osg::VectorGLuint* indices, osg::Vec4* pos, size_t size,
                          const osg::Matrix& model, const osg::Matrix& view) = 0;
    };
    void setSortCallback(Sorter* s) { _sortCallback = s; }
    Sorter* getSortCallback() { return _sortCallback.get(); }

    void addGeometry(GaussianGeometry* geom);
    void removeGeometry(GaussianGeometry* geom);
    void clear() { _geometries.clear(); _geometryMatrices.clear(); }

protected:
    virtual ~GaussianSorter() { configureThreads(0); }
    virtual void cull(GaussianGeometry* geom, const osg::Matrix& model, const osg::Matrix& view);

    std::set<osg::ref_ptr<GaussianGeometry>> _geometries;
    std::map<GaussianGeometry*, osg::Matrix> _geometryMatrices;
    std::vector<OpenThreads::Thread*> _sortThreads;
    osg::ref_ptr<Sorter> _sortCallback;
    Method _method; bool _firstFrame, _onDemand;
};

}

#endif
