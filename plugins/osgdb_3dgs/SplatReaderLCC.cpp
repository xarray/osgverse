#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include "modeling/GaussianGeometry.h"
#include "spz/load-spz.h"
#include "3rdparty/picojson.h"

spz::GaussianCloud loadSplatFromXGrids(std::istream& in, const std::string& path)
{
    picojson::value document; spz::GaussianCloud result;
    std::string err = picojson::parse(document, in);
    if (!err.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to parse LCC: " << err << std::endl;
        return result;
    }

    /*result.numPoints = numPoints;
    result.shDegree = degreeForDim(shDim);
    result.positions.reserve(numPoints * 3);
    result.scales.reserve(numPoints * 3);
    result.rotations.reserve(numPoints * 4);
    result.alphas.reserve(numPoints * 1);
    result.colors.reserve(numPoints * 3);*/
    //result.convertCoordinates(spz::CoordinateSystem::RDF, to);
    return result;
}
