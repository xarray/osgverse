#include "GaussianGeometry.h"
using namespace osgVerse;

GaussianGeometry::GaussianGeometry()
:   osg::Geometry(), _degrees(0)
{
    setUseDisplayList(false);
    setUseVertexBufferObjects(true);
}

GaussianGeometry::GaussianGeometry(const GaussianGeometry& copy, const osg::CopyOp& copyop)
: osg::Geometry(copy, copyop), _degrees(copy._degrees) {}
