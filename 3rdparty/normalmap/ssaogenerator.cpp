/********************************************************************************
 *   Copyright (C) 2015 by Simon Wendsche                                       *
 *                                                                              *
 *   This file is part of NormalmapGenerator.                                   *
 *                                                                              *
 *   NormalmapGenerator is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by       *
 *   the Free Software Foundation; either version 3 of the License, or          *
 *   (at your option) any later version.                                        *
 *                                                                              *
 *   NormalmapGenerator is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 *   GNU General Public License for more details.                               *
 *                                                                              *
 *   You should have received a copy of the GNU General Public License          *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 *                                                                              *
 *   Sourcecode: https://github.com/Theverat/NormalmapGenerator                 *
 ********************************************************************************/

#include "ssaogenerator.h"
#include <osg/Matrix>
#include <osg/Vec4ub>
#include <cmath>

SsaoGenerator::SsaoGenerator() {
}

osg::Image* SsaoGenerator::calculateSsaomap(osg::Image* normalmap, osg::Image* depthmap,
                                            float radius, unsigned int kernelSamples, unsigned int noiseSize) {
    osg::ref_ptr<osg::Image> result = new osg::Image;
    result->allocateImage(normalmap->s(), normalmap->t(), 1, GL_RGBA, GL_UNSIGNED_BYTE);

    std::vector<osg::Vec3d> kernel = generateKernel(kernelSamples);
    std::vector<osg::Vec3d> noiseTexture = generateNoise(noiseSize);
    osg::Vec4ub* ptr = (osg::Vec4ub*)result->data();
    osg::Vec4ub* ptrN = (osg::Vec4ub*)normalmap->data();
    osg::Vec4ub* ptrD = (osg::Vec4ub*)depthmap->data();

    #pragma omp parallel for  // OpenMP
    for(int y = 0; y < normalmap->t(); y++) {
        for(int x = 0; x < normalmap->s(); x++) {
            osg::Vec3d origin(x, y, 1.0);
            osg::Vec4ub color = *(ptrN + y * normalmap->s() + x);

            float r = color[0], g = color[1], b = color[2];
            osg::Vec3d normal(r, g, b);

            //reorient the kernel along the normal
            //get random vector from noise texture
            osg::Vec3d randVec = noiseTexture.at((int)random(0, noiseTexture.size() - 1));

            osg::Vec3d tangent = (randVec - normal * (randVec * normal)); tangent.normalize();
            osg::Vec3d bitangent = normal ^ tangent;
            osg::Matrix transformMatrix = osg::Matrix(tangent.x(), bitangent.x(), normal.x(), 0,
                                                      tangent.y(), bitangent.y(), normal.y(), 0,
                                                      tangent.z(), bitangent.z(), normal.z(), 0,
                                                      0, 0, 0, 0);

            float occlusion = 0.0;
            for(unsigned int i = 0; i < kernel.size(); i++) {
                //get sample position
                osg::Vec3d sample = transformMatrix * kernel[i];
                sample = (sample * radius) + origin;

                //get sample depth
                osg::Vec4ub depth = *(ptrD + y * depthmap->s() + x);
                float sampleDepth = depth[0];

                //range check and accumulate
                float rangeCheck = fabs(origin.z() - sampleDepth) < radius ? 1.0 : 0.0;
                occlusion += (sampleDepth <= sample.z() ? 1.0 : 0.0) * rangeCheck;
            }

            //normalize and invert occlusion factor
            occlusion = occlusion / kernel.size();

            //convert occlusion to the 0-255 range
            int c = (int)(255.0 * occlusion);
            //write result
            *(ptr + y * result->s() + x) = osg::Vec4ub(c, c, c, 255);
        }
    }
    return result.release();
}

std::vector<osg::Vec3d> SsaoGenerator::generateKernel(unsigned int size) {
    std::vector<osg::Vec3d> kernel = std::vector<osg::Vec3d>(size, osg::Vec3d());

    //generate hemisphere
    for (unsigned int i = 0; i < size; i++) {
        //points on surface of a hemisphere
        kernel[i] = osg::Vec3d(random(-1.0, 1.0), random(-1.0, 1.0), random(0.0, 1.0));
        kernel[i].normalize();
        //scale into the hemisphere
        kernel[i] *= random(0.0, 1.0);
        //generate falloff
        float scale = float(i) / float(size);
        scale = lerp(0.1f, 1.0f, scale * scale);
        kernel[i] *= scale;
    }
    return kernel;
}

std::vector<osg::Vec3d> SsaoGenerator::generateNoise(unsigned int size) {
    std::vector<osg::Vec3d> noise = std::vector<osg::Vec3d>(size, osg::Vec3d());
    for (unsigned int i = 0; i < size; i++) {
        noise[i] = osg::Vec3d(random(-1.0, 1.0), random(-1.0, 1.0), 0.0);
        noise[i].normalize();
    }
    return noise;
}

double SsaoGenerator::random(double min, double max) {
    double f = (double)rand() / RAND_MAX;
    return min + f * (max - min);
}

float SsaoGenerator::lerp(float v0, float v1, float t) {
    return (1 - t) * v0 + t * v1;
}
