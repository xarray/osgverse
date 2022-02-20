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

#include "specularmapgenerator.h"
#include <osg/Vec3ub>
#include <osg/Vec4ub>

SpecularmapGenerator::SpecularmapGenerator(IntensityMap::Mode mode, double redMultiplier,
                                           double greenMultiplier, double blueMultiplier, double alphaMultiplier)
{
    this->mode = mode;
    this->redMultiplier = redMultiplier;
    this->greenMultiplier = greenMultiplier;
    this->blueMultiplier = blueMultiplier;
    this->alphaMultiplier = alphaMultiplier;
}

osg::Image* SpecularmapGenerator::calculateSpecmap(osg::Image* input, double scale, double contrast) {
    osg::ref_ptr<osg::Image> result = new osg::Image;
    result->allocateImage(input->s(), input->t(), 1, GL_RGBA, GL_UNSIGNED_BYTE);

    //generate contrast lookup table
    double newValue = 0;
    unsigned short contrastLookup[256];
    for(int i = 0; i < 256; i++) {
        newValue = (double)i;
        newValue /= 255.0;
        newValue -= 0.5;
        newValue *= contrast;
        newValue += 0.5;
        newValue *= 255;
    
        if(newValue < 0) newValue = 0;
        if(newValue > 255) newValue = 255;
        contrastLookup[i] = (unsigned short)newValue;
    }
    
    // This is outside of the loop because the multipliers are the same for every pixel
    osg::Vec3ub* ptr3 = (osg::Vec3ub*)input->data();
    osg::Vec4ub* ptr4 = (osg::Vec4ub*)input->data();
    osg::Vec4ub* ptrR = (osg::Vec4ub*)result->data();
    int use3 = (input->getPixelFormat() == GL_RGB);
    double multiplierSum = ((redMultiplier != 0.0) + (greenMultiplier != 0.0) +
                          (blueMultiplier != 0.0) + (alphaMultiplier != 0.0));
    if(multiplierSum == 0.0) multiplierSum = 1.0;

    #pragma omp parallel for  // OpenMP
    //for every row of the image
    for(int y = 0; y < result->t(); y++) {
        //for every column of the image
        for(int x = 0; x < result->s(); x++) {
            double intensity = 0.0, r, g, b, a = 1.0;
            if (use3)
            {
                osg::Vec3ub pxColor = *(ptr3 + result->s() * y + x);
                r = pxColor.x() * redMultiplier;
                g = pxColor.y() * greenMultiplier;
                b = pxColor.z() * blueMultiplier;
            }
            else
            {
                osg::Vec4ub pxColor = *(ptr4 + result->s() * y + x);
                r = pxColor.x() * redMultiplier;
                g = pxColor.y() * greenMultiplier;
                b = pxColor.z() * blueMultiplier;
                a = pxColor.a() * alphaMultiplier;
            }

            if(mode == IntensityMap::AVERAGE) {
                //take the average out of all selected channels
                intensity = (r + g + b + a) / multiplierSum;
            }
            else if(mode == IntensityMap::MAX) {
                //take the maximum out of all selected channels
                const double tempMaxRG = osg::maximum(r, g);
                const double tempMaxBA = osg::maximum(b, a);
                intensity = osg::maximum(tempMaxRG, tempMaxBA);
            }

            //apply scale (brightness)
            intensity *= scale;
            if(intensity > 1.0) intensity = 1.0;

            //convert intensity to the 0-255 range
            int c = (int)(255.0 * intensity);
            
            //apply contrast
            c = (int)contrastLookup[c];
            
            //write color into image pixel
            *(ptrR + result->s() * y + x) = osg::Vec4ub(c, c, c, a / alphaMultiplier);
        }
    }
    return result.release();
}
