#include "conv2d.h"
#include <cstdint>

void Conv2D_HW(TFXP *input, TFXP * output, TFXP * coeffs,
      uint32_t numChannels, uint32_t numFilters,
      uint32_t inputWidth, uint32_t inputHeight,
      uint32_t convWidth, uint32_t convHeight)
{
    #pragma HLS INTERFACE s_axilite port=numChannels
    #pragma HLS INTERFACE s_axilite port=numFilters
    #pragma HLS INTERFACE s_axilite port=inputWidth
    #pragma HLS INTERFACE s_axilite port=inputHeight
    #pragma HLS INTERFACE s_axilite port=convWidth
    #pragma HLS INTERFACE s_axilite port=convHeight
    #pragma HLS INTERFACE s_axilite port=return

    #pragma HLS INTERFACE m_axi port=input offset=slave
    #pragma HLS INTERFACE m_axi port=output offset=slave
    #pragma HLS INTERFACE m_axi port=coeffs offset=slave

    TFXP filter_bufffer[CONV_SIZE * CONV_SIZE * MAX_CHANNELS]; // same as 3D array (is stored flat without any precisions, and Vitis can do whatever it wants)

    TFXP rows_buffer[MAX_ROW_SIZE];

  loop_filters : for (uint32_t iFilter = 0; iFilter < numFilters; ++iFilter) {

      // filters cache
      load_coeff : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
        for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
          for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
            #pragma HLS UNROLL

            TFXP filterValue;
            filterValue = *(coeffs + iFilter*numChannels*CONV_SIZE*CONV_SIZE + iChannel*CONV_SIZE*CONV_SIZE + cy*CONV_SIZE + cx);
            filter_bufffer[iFilter*numChannels*CONV_SIZE*CONV_SIZE + iChannel*CONV_SIZE*CONV_SIZE + cy*CONV_SIZE + cx] = filterValue;
          }
        }
      }

    loop_convolve_x : for (uint32_t y = 0; y < (inputHeight-2); ++y) {

        // row caching
        loop_row : for (uint32_t cx = 0; cx < inputWidth-2; ++cx) {
            for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
                #pragma HLS UNROLL

                TFXP pixelValue = *(input + cy*inputWidth + cx);
                rows_buffer[cy*inputWidth + cx] = pixelValue;
            }
        }

      loop_convolve_y : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
        TFXP acc;
        acc = 0;

        loop_acc_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
          loop_acc_x : for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
            loop_acc_y : for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
                #pragma HLS UNROLL

              TFXP pixelValue;
              // filterValue = *(coeffs + iFilter*numChannels*CONV_SIZE*CONV_SIZE + iChannel*CONV_SIZE*CONV_SIZE + cy*CONV_SIZE + cx);
              pixelValue = rows_buffer[(cy+1)*inputWidth + (cx+1)];
              acc += FXP_Mult(filter_bufffer[iFilter*numChannels*CONV_SIZE*CONV_SIZE + iChannel*CONV_SIZE*CONV_SIZE + cy*CONV_SIZE + cx], pixelValue, DECIMALS);
            }
          }
        }
        //output[iFilter][y][x] = acc;
        *(output + iFilter * (inputHeight-2)*(inputWidth-2) + y*(inputWidth-2) + x) = acc;
      }
    }
  }
}
