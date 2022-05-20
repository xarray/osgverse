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

#include "intensitymap.h"
#include <osg/Vec3b>
#include <osg/Vec4ub>
#include <iostream>

IntensityMap::IntensityMap() {

}

IntensityMap::IntensityMap(int width, int height) {
    map = std::vector< std::vector<double> >(height, std::vector<double>(width, 0.0));
}

IntensityMap::IntensityMap(osg::Image* rgbImage, Mode mode, double redMultiplier, double greenMultiplier,
                           double blueMultiplier, double alphaMultiplier)
{
    osg::Vec3b* ptr3 = (osg::Vec3b*)rgbImage->data();
    osg::Vec4ub* ptr4 = (osg::Vec4ub*)rgbImage->data();
    int srcW = rgbImage->s(), srcH = rgbImage->t(), use3 = (rgbImage->getPixelFormat() == GL_RGB);
    map = std::vector< std::vector<double> >(srcH, std::vector<double>(srcW, 0.0));

    #pragma omp parallel for
    //for every row of the image
    for(int y = 0; y < rgbImage->t(); y++) {
        //for every column of the image
        for(int x = 0; x < rgbImage->s(); x++) {
            double intensity = 0.0, r, g, b, a = 1.0;
            if (use3)
            {
                osg::Vec3b color = *(ptr3 + srcW * y + x);
                r = (unsigned char)color.x() * redMultiplier;
                g = (unsigned char)color.y() * greenMultiplier;
                b = (unsigned char)color.z() * blueMultiplier;
            }
            else
            {
                osg::Vec4ub color = *(ptr4 + srcW * y + x);
                r = color[0] * redMultiplier;
                g = color[1] * greenMultiplier;
                b = color[2] * blueMultiplier;
                a = color[3] * alphaMultiplier;
            }

            if(mode == AVERAGE) {
                //take the average out of all selected channels
                int num_channels = 0;
                if(redMultiplier != 0.0) {
                    intensity += r;
                    num_channels++;
                }
                if(greenMultiplier != 0.0) {
                    intensity += g;
                    num_channels++;
                }
                if(blueMultiplier != 0.0) {
                    intensity += b;
                    num_channels++;
                }
                if(alphaMultiplier != 0.0) {
                    intensity += a;
                    num_channels++;
                }

                if(num_channels != 0)
                    intensity /= num_channels;
                else
                    intensity = 0.0;
            }
            else if(mode == MAX) {
                //take the maximum out of all selected channels
                const double tempMaxRG = osg::maximum(r, g);
                const double tempMaxBA = osg::maximum(b, a);
                intensity = osg::maximum(tempMaxRG, tempMaxBA);
            }

            //add resulting pixel intensity to intensity map
            this->map.at(y).at(x) = intensity;
        }
    }
}

double IntensityMap::at(int x, int y) const {
    return this->map.at(y).at(x);
}

double IntensityMap::at(int pos) const {
    const int x = pos % this->getWidth();
    const int y = pos / this->getWidth();

    return this->at(x, y);
}

void IntensityMap::setValue(int x, int y, double value) {
    this->map.at(y).at(x) = value;
}

void IntensityMap::setValue(int pos, double value) {
    const int x = pos % this->getWidth();
    const int y = pos / this->getWidth();

    this->map.at(y).at(x) = value;
}

size_t IntensityMap::getWidth() const {
    return this->map.at(0).size();
}

size_t IntensityMap::getHeight() const {
    return this->map.size();
}

void IntensityMap::invert() {
    #pragma omp parallel for
    for(int y = 0; y < (int)this->getHeight(); y++) {
        for(int x = 0; x < (int)this->getWidth(); x++) {
            const double inverted = 1.0 - this->map.at(y).at(x);
            this->map.at(y).at(x) = inverted;
        }
    }
}

osg::Image* IntensityMap::convertToQImage() const {
    osg::ref_ptr<osg::Image> result = new osg::Image;
    result->allocateImage(this->getWidth(), this->getHeight(), 1, GL_RGBA, GL_UNSIGNED_BYTE);

    osg::Vec4ub* ptr = (osg::Vec4ub*)result->data();
    for(size_t y = 0; y < this->getHeight(); y++) {
        for(size_t x = 0; x < this->getWidth(); x++) {
            const int c = 255 * map.at(y).at(x);
            *(ptr + y * this->getWidth() + x) = osg::Vec4ub(c, c, c, 255);
        }
    }
    return result.release();
}
