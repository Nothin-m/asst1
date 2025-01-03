#include <stdio.h>
#include <algorithm>
#include <math.h>
#include "CMU418intrin.h"
#include "logger.h"
using namespace std;


void absSerial(float* values, float* output, int N) {
    for (int i=0; i<N; i++) {
	float x = values[i];
	if (x < 0) {
	    output[i] = -x;
	} else {
	    output[i] = x;
	}
    }
}

// implementation of absolute value using 15418 instrinsics
void absVector(float* values, float* output, int N) {
    __cmu418_vec_float x;
    __cmu418_vec_float result;
    __cmu418_vec_float zero = _cmu418_vset_float(0.f);
    __cmu418_mask maskAll, maskIsNegative, maskIsNotNegative;

    //  Note: Take a careful look at this loop indexing.  This example
    //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
    //  Why is that the case?
    for (int i=0; i<N; i+=VECTOR_WIDTH) {

	// All ones
	maskAll = _cmu418_init_ones();

	// All zeros
	maskIsNegative = _cmu418_init_ones(0);

	// Load vector of values from contiguous memory addresses
	_cmu418_vload_float(x, values+i, maskAll);               // x = values[i];

	// Set mask according to predicate
	_cmu418_vlt_float(maskIsNegative, x, zero, maskAll);     // if (x < 0) {

	// Execute instruction using mask ("if" clause)
	_cmu418_vsub_float(result, zero, x, maskIsNegative);      //   output[i] = -x;

	// Inverse maskIsNegative to generate "else" mask
	maskIsNotNegative = _cmu418_mask_not(maskIsNegative);     // } else {

	// Execute instruction ("else" clause)
	_cmu418_vload_float(result, values+i, maskIsNotNegative); //   output[i] = x; }

	// Write results back to memory
	_cmu418_vstore_float(output+i, result, maskAll);
    }
}

// Accepts an array of values and an array of exponents
// For each element, compute values[i]^exponents[i] and clamp value to
// 4.18.  Store result in outputs.
// Uses iterative squaring, so that total iterations is proportional
// to the log_2 of the exponent
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
    for (int i=0; i<N; i++) {
	float x = values[i];
	float result = 1.f;
	int y = exponents[i];
	float xpower = x;
	while (y > 0) {
	    if (y & 0x1) {
			result *= xpower;
		}
	    xpower = xpower * xpower;
	    y >>= 1;
	}
	if (result > 4.18f) {
	    result = 4.18f;
	}
	output[i] = result;
    }
}

void clampedExpVector(float* values, int* exponents, float* output, int N) {
    // Implement your vectorized version of clampedExpSerial here
    //  ...
    __cmu418_vec_int intConstZero = _cmu418_vset_int(0);
    __cmu418_vec_int intConstOne = _cmu418_vset_int(1);
    __cmu418_vec_float clampMaxVector = _cmu418_vset_float(4.18f);

    __cmu418_vec_float valuesVector, xpowerVector, resultVector;
    __cmu418_vec_int exponentsVector;
    __cmu418_mask maskAll, maskClamp, maskNonZeroExponent;

    for (int i = 0; i < N; i += VECTOR_WIDTH) {
        maskAll = _cmu418_init_ones(min(N - i, VECTOR_WIDTH));

        _cmu418_vload_float(valuesVector, values + i, maskAll);
        _cmu418_vload_int(exponentsVector, exponents + i, maskAll);

        _cmu418_vset_float(resultVector, 1.0f, maskAll);
        _cmu418_vmove_float(xpowerVector, valuesVector, maskAll);

        _cmu418_vgt_int(maskNonZeroExponent, exponentsVector, intConstZero, maskAll);

        while (_cmu418_cntbits(maskNonZeroExponent)) {
            __cmu418_vec_int oddExponentFlag;
            __cmu418_mask oddExponentMask, oddExponentMaskAndAll;
            _cmu418_vbitand_int(oddExponentFlag, exponentsVector, intConstOne, maskAll);
            _cmu418_veq_int(oddExponentMask, oddExponentFlag, intConstOne, maskAll);
            oddExponentMaskAndAll = _cmu418_mask_and(oddExponentMask, maskAll);
            _cmu418_vmult_float(resultVector, resultVector, xpowerVector, oddExponentMaskAndAll);
            _cmu418_vmult_float(xpowerVector, xpowerVector, xpowerVector, maskAll);
            _cmu418_vshiftright_int(exponentsVector, exponentsVector, intConstOne, maskAll);
            _cmu418_vgt_int(maskNonZeroExponent, exponentsVector, intConstZero, maskAll);
        }

        _cmu418_vgt_float(maskClamp, resultVector, clampMaxVector, maskAll);
        _cmu418_vmove_float(resultVector, clampMaxVector, maskClamp);
        _cmu418_vstore_float(output + i, resultVector, maskAll);
    }
}


float arraySumSerial(float* values, int N) {
    float sum = 0;
    for (int i=0; i<N; i++) {
	sum += values[i];
    }

    return sum;
}

// Assume N % VECTOR_WIDTH == 0
// Assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
    // Implement your vectorized version here
    //  ...
    __cmu418_vec_float sumVector = _cmu418_vset_float(0.0f);
    __cmu418_vec_float valuesVector;
    __cmu418_mask maskAll = _cmu418_init_ones();

    for (int i = 0; i < N; i += VECTOR_WIDTH) {
        _cmu418_vload_float(valuesVector, values + i, maskAll);
        _cmu418_vadd_float(sumVector, sumVector, valuesVector, maskAll);
    }

    int spread = VECTOR_WIDTH;
    while (spread > 1) {
        _cmu418_hadd_float(sumVector, sumVector);
        _cmu418_interleave_float(sumVector, sumVector);
        spread = spread / 2;
    }

    float result;
    _cmu418_vstore_float(&result, sumVector, maskAll);
    return result;
}
