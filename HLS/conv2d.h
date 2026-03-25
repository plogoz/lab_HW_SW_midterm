#ifndef CONV2D_H
#define CONV2D_H

#include <stdint.h>
// #include <ap_fixed.h>
// #include <ap_int.h>

#define CONV_SIZE 3
#define MAX_CHANNELS 256
#define MAX_INPUT_SIZE 2304
#define MAX_ROW_SIZE 127 * 32 // Convolution layer 1

const uint32_t DECIMALS = 20;
typedef int32_t TFXP;     // Parameters and activations
typedef int64_t TFXP_MULT;// Intermmediate results of multiplications

inline TFXP FXP_Mult(TFXP a, TFXP b, uint32_t decimalBits = DECIMALS)
{
  TFXP_MULT res = (TFXP_MULT)a * (TFXP_MULT)b;
  res = res >> decimalBits;
  return res;
}

void Conv2D_HW(TFXP *input, TFXP * output, TFXP * coeffs,
      uint32_t numChannels, uint32_t numFilters,
      uint32_t inputWidth, uint32_t inputHeight,
      uint32_t convWidth = 3, uint32_t convHeight = 3);

#endif // CONV2D_H
