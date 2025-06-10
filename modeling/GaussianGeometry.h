#ifndef MANA_MODELING_GAUSSIANGEOMETRY
#define MANA_MODELING_GAUSSIANGEOMETRY

#include <osg/Version>
#include <osg/Geometry>

namespace osgVerse
{

/* Gaussian Data:
   - Position (vec3): getVertexArray()
   - Scale (vec3): getNormalArray()
   - Rotation (vec4): getVertexAttribArray(2)
   - HintColor + Alpha (vec4): getColorArray()
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

    void setShDegrees(int d) { _degrees = d; }
    int getShDegrees() const { return _degrees; }

    void setPosition(osg::Vec3Array* v) { setVertexArray(v); }
    void setScale(osg::Vec3Array* v) { setNormalArray(v); setNormalBinding(BIND_PER_VERTEX); }
    void setRotation(osg::Vec4Array* v) { setVertexAttribArray(2, v); setVertexAttribBinding(2, BIND_PER_VERTEX); }
    void setColorAndAlpha(osg::Vec4Array* v) { setColorArray(v); setColorBinding(BIND_PER_VERTEX); }
    void setShRed(int i, osg::Vec4Array* v) { setVertexAttribArray(4 + i * 3, v); setVertexAttribBinding(4 + i * 3, BIND_PER_VERTEX); }
    void setShGreen(int i, osg::Vec4Array* v) { setVertexAttribArray(5 + i * 3, v); setVertexAttribBinding(5 + i * 3, BIND_PER_VERTEX); }
    void setShBlue(int i, osg::Vec4Array* v) { setVertexAttribArray(6 + i * 3, v); setVertexAttribBinding(6 + i * 3, BIND_PER_VERTEX); }

    osg::Vec3Array* getPosition() { return static_cast<osg::Vec3Array*>(getVertexArray()); }
    osg::Vec3Array* getScale() { return static_cast<osg::Vec3Array*>(getNormalArray()); }
    osg::Vec4Array* getRotation() { return static_cast<osg::Vec4Array*>(getVertexAttribArray(2)); }
    osg::Vec4Array* getColorAndAlpha() { return static_cast<osg::Vec4Array*>(getColorArray()); }
    osg::Vec4Array* getShRed(int index) { return static_cast<osg::Vec4Array*>(getVertexAttribArray(4 + index * 3)); }
    osg::Vec4Array* getShGreen(int index) { return static_cast<osg::Vec4Array*>(getVertexAttribArray(5 + index * 3)); }
    osg::Vec4Array* getShBlue(int index) { return static_cast<osg::Vec4Array*>(getVertexAttribArray(6 + index * 3)); }

protected:
    virtual ~GaussianGeometry() {}
    int _degrees;
};

}

#endif
