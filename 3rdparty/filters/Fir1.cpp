/*
License: MIT License (http://www.opensource.org/licenses/mit-license.php)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* (C) 2013 Graeme Hattan & Bernd Porr */
/* (C) 2018-2024 Bernd Porr */

#include "Fir1.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdexcept>

// give the filter an array of doubles for the coefficients
Fir1::Fir1(const double *_coefficients,const unsigned number_of_taps) :
	coefficients(new double[number_of_taps]),
	buffer(new double[number_of_taps]()),
	taps(number_of_taps) {
	for(unsigned int i=0;i<number_of_taps;i++) {
		coefficients[i] = _coefficients[i];
		buffer[i] = 0;
	}
}

// init all coefficients and the buffer to zero
Fir1::Fir1(unsigned number_of_taps, double value) :
	coefficients(new double[number_of_taps]),
	buffer(new double[number_of_taps]),  
	taps(number_of_taps) {
	for(unsigned int i=0;i<number_of_taps;i++) {
	        coefficients[i] = value;
		buffer[i] = 0;
	}
}

void Fir1::initWithVector(std::vector<double> _coefficients) {
	coefficients = new double[_coefficients.size()];
	buffer = new double[_coefficients.size()]();
	taps = ((unsigned int)_coefficients.size());
	for(unsigned long i=0;i<_coefficients.size();i++) {
		coefficients[i] = _coefficients[i];
		buffer[i] = 0;
	}
}	

// one coefficient per line
Fir1::Fir1(const char* coeffFile, unsigned number_of_taps) {

	std::vector<double> tmpCoefficients;

	FILE* f=fopen(coeffFile,"rt");
	if (!f) {
		throw std::invalid_argument("Could not open file.");
	}
	for(unsigned int i=0;(i<number_of_taps)||(number_of_taps==0);i++) {
		double v = 0;
		int r = fscanf(f,"%lf\n",&v);
		if (r < 1) break;
		tmpCoefficients.push_back(v);
	}
	fclose(f);
	initWithVector(tmpCoefficients);
}


Fir1::~Fir1()
{
	delete[] buffer;
	delete[] coefficients;
}


void Fir1::reset()
{
	memset(buffer, 0, sizeof(double)*taps);
	offset = 0;
}

void Fir1::zeroCoeff() {
	memset(coefficients, 0, sizeof(double)*taps);
	offset = 0;
}

void Fir1::getCoeff(double* coeff_data, unsigned number_of_taps) const {
	
	if (number_of_taps < taps)
		throw std::out_of_range("Fir1: target of getCoeff: too many weights to copy into target");
 
	memcpy(coeff_data, coefficients, taps * sizeof(double));
	if (number_of_taps > taps)
		memset(&coeff_data[taps], 0, (number_of_taps - taps)*sizeof(double));
}
void Fir1::setCoeff(const double* coeff_data, const unsigned number_of_taps) {

	if (number_of_taps != taps) {
		throw std::runtime_error("Invalid number of taps in new coefficient array");
	}
	for (unsigned int i = 0; i < number_of_taps; i++) {
		coefficients[i] = coeff_data[i];
	}
}

