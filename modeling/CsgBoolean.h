#ifndef MANA_MODELING_CSGBOOLEAN_HPP
#define MANA_MODELING_CSGBOOLEAN_HPP

#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <map>
#include <vector>
#include <iostream>

namespace osgVerse
{
    class CsgBoolean
    {
    public:
        enum Operation { A_NOT_B, B_NOT_A, UNION, INTERSECTION };
        CsgBoolean(Operation op);
        
        void setMesh(osg::Node* node, bool AorB);
        osg::Node* generate();
        
    protected:
        struct MeshData
        {
            std::vector<uint32_t> faceSizesArray;
            std::vector<uint32_t> faceIndicesArray;
            std::vector<double> vertexCoordsArray;
        };
        MeshData _meshA, _meshB;
        Operation _operation;
    };
}

#endif
