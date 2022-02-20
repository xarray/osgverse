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

#include "normalmapgenerator.h"
#include <osg/Vec4ub>

NormalmapGenerator::NormalmapGenerator(IntensityMap::Mode mode, double redMultiplier, double greenMultiplier, double blueMultiplier, double alphaMultiplier)
    : tileable(false), redMultiplier(redMultiplier), greenMultiplier(greenMultiplier), blueMultiplier(blueMultiplier), alphaMultiplier(alphaMultiplier), mode(mode)
{}

const IntensityMap& NormalmapGenerator::getIntensityMap() const {
    return this->intensity;
}

osg::Image* NormalmapGenerator::calculateNormalmap(osg::Image* input, Kernel kernel, double strength, bool invert, bool tileable,
                                                   bool keepLargeDetail, int largeDetailScale, double largeDetailHeight) {
    this->tileable = tileable;
    this->intensity = IntensityMap(input, mode, redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier);
    if(!invert) {
        // The default "non-inverted" normalmap looks wrong in renderers,
        // so I use inversion by default
        intensity.invert();
	}

    osg::ref_ptr<osg::Image> result = new osg::Image;
    result->allocateImage(input->s(), input->t(), 1, GL_RGBA, GL_UNSIGNED_BYTE);
    
    // optimization
    double strengthInv = 1.0 / strength;
    int srcW = input->s(), srcH = input->t();
    osg::Vec4ub* ptr = (osg::Vec4ub*)result->data();

    #pragma omp parallel for  // OpenMP
    //code from http://stackoverflow.com/a/2368794
    for(int y = 0; y < srcH; y++) {
        for(int x = 0; x < srcW; x++) {

            const double topLeft      = intensity.at(handleEdges(x - 1, srcW), handleEdges(y - 1, srcH));
            const double top          = intensity.at(handleEdges(x - 1, srcW), handleEdges(y, srcH));
            const double topRight     = intensity.at(handleEdges(x - 1, srcW), handleEdges(y + 1, srcH));
            const double right        = intensity.at(handleEdges(x, srcW), handleEdges(y + 1, srcH));
            const double bottomRight  = intensity.at(handleEdges(x + 1, srcW), handleEdges(y + 1, srcH));
            const double bottom       = intensity.at(handleEdges(x + 1, srcW), handleEdges(y, srcH));
            const double bottomLeft   = intensity.at(handleEdges(x + 1, srcW), handleEdges(y - 1, srcH));
            const double left         = intensity.at(handleEdges(x, srcW), handleEdges(y - 1, srcH));

            const double convolution_kernel[3][3] = {{topLeft, top, topRight},
                                                     {left, 0.0, right},
                                                     {bottomLeft, bottom, bottomRight}};
            osg::Vec3d normal(0, 0, 0);
            if(kernel == SOBEL) normal = sobel(convolution_kernel, strengthInv);
            else if(kernel == PREWITT) normal = prewitt(convolution_kernel, strengthInv);
            *(ptr + y * srcW + x) = osg::Vec4ub(
                (GLubyte)mapComponent(normal.x()), (GLubyte)mapComponent(normal.y()),
                (GLubyte)mapComponent(normal.z()), 255);
        }
    }
    
    if (keepLargeDetail) {
        //generate a second normalmap from a downscaled input image, then mix both normalmaps
        int largeDetailMapWidth = (int) (((double)srcW / 100.0) * largeDetailScale);
        int largeDetailMapHeight = (int) (((double)srcH / 100.0) * largeDetailScale);
        
        //create downscaled version of input
        osg::ref_ptr<osg::Image> inputScaled = static_cast<osg::Image*>(input->clone(osg::CopyOp::DEEP_COPY_ALL));
        inputScaled->scaleImage(largeDetailMapWidth, largeDetailMapHeight, 1);
        //compute downscaled normalmap
        osg::ref_ptr<osg::Image> largeDetailMap = calculateNormalmap(
            inputScaled, kernel, largeDetailHeight, invert, tileable, false, 0, 0.0);
        //scale map up
        largeDetailMap = static_cast<osg::Image*>(largeDetailMap->clone(osg::CopyOp::DEEP_COPY_ALL));
        largeDetailMap->scaleImage(srcW, srcH, 1);
        
        osg::Vec4ub* ptrR = (osg::Vec4ub*)result->data();
        osg::Vec4ub* ptrL = (osg::Vec4ub*)largeDetailMap->data();
        #pragma omp parallel for  // OpenMP
        //mix the normalmaps
        for(int y = 0; y < srcH; y++) {
            for(int x = 0; x < srcW; x++) {
                osg::Vec4ub colorResult = *(ptrR + y * srcW + x);
                osg::Vec4ub colorLargeDetail = *(ptrL + y * srcW + x);

                const int r = blendSoftLight(colorResult[0], colorLargeDetail[0]);
                const int g = blendSoftLight(colorResult[1], colorLargeDetail[1]);
                const int b = blendSoftLight(colorResult[2], colorLargeDetail[2]);
                *(ptrR + y * srcW + x) = osg::Vec4ub((GLubyte)r, (GLubyte)g, (GLubyte)b, 255);
            }
        }
    }
    return result.release();
}

osg::Vec3d NormalmapGenerator::sobel(const double convolution_kernel[3][3], double strengthInv) const {
    const double top_side    = convolution_kernel[0][0] + 2.0 * convolution_kernel[0][1] + convolution_kernel[0][2];
    const double bottom_side = convolution_kernel[2][0] + 2.0 * convolution_kernel[2][1] + convolution_kernel[2][2];
    const double right_side  = convolution_kernel[0][2] + 2.0 * convolution_kernel[1][2] + convolution_kernel[2][2];
    const double left_side   = convolution_kernel[0][0] + 2.0 * convolution_kernel[1][0] + convolution_kernel[2][0];

    const double dY = right_side - left_side;
    const double dX = bottom_side - top_side;
    const double dZ = strengthInv;
    osg::Vec3d n(dX, dY, dZ); n.normalize(); return n;
}

osg::Vec3d NormalmapGenerator::prewitt(const double convolution_kernel[3][3], double strengthInv) const {
    const double top_side    = convolution_kernel[0][0] + convolution_kernel[0][1] + convolution_kernel[0][2];
    const double bottom_side = convolution_kernel[2][0] + convolution_kernel[2][1] + convolution_kernel[2][2];
    const double right_side  = convolution_kernel[0][2] + convolution_kernel[1][2] + convolution_kernel[2][2];
    const double left_side   = convolution_kernel[0][0] + convolution_kernel[1][0] + convolution_kernel[2][0];

    const double dY = right_side - left_side;
    const double dX = top_side - bottom_side;
    const double dZ = strengthInv;
    osg::Vec3d n(dX, dY, dZ); n.normalize(); return n;
}

int NormalmapGenerator::handleEdges(int iterator, int maxValue) const {
    if(iterator >= maxValue) {
        //move iterator from end to beginning + overhead
        if(tileable)
            return maxValue - iterator;
        else
            return maxValue - 1;
    }
    else if(iterator < 0) {
        //move iterator from beginning to end - overhead
        if(tileable)
            return maxValue + iterator;
        else
            return 0;
    }
    else {
        return iterator;
    }
}

//transform -1..1 to 0..255
int NormalmapGenerator::mapComponent(double value) const {
    return (value + 1.0) * (255.0 / 2.0);
}

//uses a similar algorithm like "soft light" in PS, 
//see http://www.michael-kreil.de/algorithmen/photoshop-layer-blending-equations/index.php
int NormalmapGenerator::blendSoftLight(int color1, int color2) const {
    const float a = color1;
    const float b = color2;
    
    if(2.0f * b < 255.0f) {
        return (int) (((a + 127.5f) * b) / 255.0f);
    }
    else {
        return (int) (255.0f - (((382.5f - a) * (255.0f - b)) / 255.0f));
    }
}
