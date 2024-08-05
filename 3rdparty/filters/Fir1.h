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

/* (C) 2013-2024 Graeme Hattan & Bernd Porr */

#ifndef FIR1_H
#define FIR1_H

#include <stdio.h>
#include <vector>

/**
 * Finite impulse response filter. The precision is double.
 * It takes as an input a file with coefficients or an double
 * array.
 **/
class Fir1 {
public:
    /** 
     * Coefficients as a const double array. Because the array is const
     * the number of taps is identical to the length of the array.
     * \param _coefficients A const double array with the impulse response.
     **/
    template <unsigned nTaps> Fir1(const double (&_coefficients)[nTaps]) :
	coefficients(new double[nTaps]),
	buffer(new double[nTaps]()),
	taps(nTaps) {
	for(unsigned i=0;i<nTaps;i++) {
	    coefficients[i] = _coefficients[i];
	    buffer[i] = 0;
	}
    }

    /**
     * Coefficients as a C++ vector
     * \param _coefficients is a Vector of doubles.
     **/
    Fir1(std::vector<double> _coefficients) {
	initWithVector(_coefficients);
    }

    /**
     * Coefficients as a (non-constant-) double array where the length needs to be specified.
     * \param coefficients Coefficients as double array.
     * \param number_of_taps Number of taps (needs to match the number of coefficients
     **/
    Fir1(const double *coefficients,const unsigned number_of_taps);

    /** Coefficients as a text file (for example from Python)
     * The number of taps is automatically detected
     * when the taps are kept zero.
     * \param coeffFile Patht to textfile where every line contains one coefficient
     * \param number_of_taps Number of taps (0 = autodetect)
     **/
    Fir1(const char* coeffFile, unsigned number_of_taps = 0);

    /** 
     * Inits all coefficients and the buffer to a constant value.
     * This is useful for adaptive filters where we start with
     * zero valued coefficients or moving average filters with
     * value = 1.0/number_of_taps.
     **/
    Fir1(unsigned number_of_taps, double value = 0);

    /**
     * Releases the coefficients and buffer.
     **/
    ~Fir1();

	
    /**
     * The actual filter function operation: it receives one sample
     * and returns one sample.
     * \param input The input sample.
     **/
    inline double filter(double input) {
	const double *coeff     = coefficients;
	const double *const coeff_end = coefficients + taps;
		
	double *buf_val = buffer + offset;
		
	*buf_val = input;
	double output_ = 0;
		
	while(buf_val >= buffer)
	    output_ += *buf_val-- * *coeff++;
		
	buf_val = buffer + taps-1;
		
	while(coeff < coeff_end)
	    output_ += *buf_val-- * *coeff++;
		
	if(++offset >= taps)
	    offset = 0;
		
	return output_;
    }


    /**
     * LMS adaptive filter weight update:
     * Every filter coefficient is updated with:
     * w_k(n+1) = w_k(n) + learning_rate * buffer_k(n) * error(n)
     * \param error Is the term error(n), the error which adjusts the FIR conefficients.
     **/
    inline void lms_update(double error) {
	double *coeff     = coefficients;
	const double *coeff_end = coefficients + taps;
	
	double *buf_val = buffer + offset;
		
	while(buf_val >= buffer) {
	    *coeff++ += *buf_val-- * error * mu;
	}
		
	buf_val = buffer + taps-1;
		
	while(coeff < coeff_end) {
	    *coeff++ += *buf_val-- * error * mu;
	}
    }

    /**
     * Setting the learning rate for the adaptive filter.
     * \param _mu The learning rate (i.e. rate of the change by the error signal)
     **/
    void setLearningRate(double _mu) {mu = _mu;};

    /**
     * Getting the learning rate for the adaptive filter.
     **/
    double getLearningRate() {return mu;};

    /**
     * Resets the buffer (but not the coefficients)
     **/
    void reset();

    /** 
     * Sets all coefficients to zero
     **/
    void zeroCoeff();

    /**
     * Copies the current filter coefficients into a provided array.
     * Useful after an adaptive filter has been trained to query
     * the result of its training.
     * \param coeff_data target where coefficients are copied
     * \param number_of_taps number of doubles to be copied
     * \throws std::out_of_range number_of_taps is less the actual number of taps.
     */
    void getCoeff(double* coeff_data, unsigned number_of_taps) const;

    /**
     * @brief Externally sets the coefficient array. This is useful when the
     * actually running filter is at a different place as where the updating
     * filter is employed.
     *
     * @param coeff_data New coefficients to set.
     * @param number_of_taps Number of taps in the coefficient array. If this is
     * not equal to the number of taps used in this filter, a runtime error is
     * thrown.
     */
    void setCoeff(const double *coeff_data, const unsigned number_of_taps);

    /**
     * Returns the coefficients as a vector
     **/
    std::vector<double> getCoeffVector() const {
	return std::vector<double>(coefficients,coefficients+taps);
    }

    /**
     * Returns the number of taps.
     **/
    unsigned getTaps() {return taps;};

    /**
     * Returns the power of the of the buffer content:
     * sum_k buffer[k]^2
     * which is needed to implement a normalised LMS algorithm.
     **/
    inline double getTapInputPower() {
	double *buf_val = buffer;
		
	double p = 0;
		
	for(unsigned i = 0; i < taps; i++) {
	    p += (*buf_val) * (*buf_val);
	    buf_val++;
	}
	
	return p;
    }

private:
    void initWithVector(std::vector<double> _coefficients);
	
    double        *coefficients;
    double        *buffer;
    unsigned      taps;
    unsigned      offset = 0;
    double        mu = 0;
};

#endif
