#include "conv2d.h"
#include <cstdint>

void coeffCaching(TFXP *coeffs, TFXP filter_buffer[MAX_CHANNELS][CONV_SIZE][CONV_SIZE], uint32_t numChannels, uint32_t iFilter) {
    load_coeff : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
        #pragma HLS loop_tripcount max=256
        #pragma HLS PIPELINE II=1

      for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
        #pragma HLS UNROLL

        for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
          #pragma HLS UNROLL

              filter_buffer[iChannel][cy][cx] = *(coeffs + iFilter*numChannels*CONV_SIZE*CONV_SIZE + iChannel*CONV_SIZE*CONV_SIZE + cy*CONV_SIZE + cx);
        }
      }
    }
}

void rowCaching(TFXP *input, TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE], uint32_t inputWidth, uint32_t y) {
    // Load 3 rows starting at y into rows_buffer[0..2]
    for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
        #pragma HLS UNROLL
        for (uint32_t cx = 0; cx < inputWidth; ++cx) {
            #pragma HLS loop_tripcount max=256
            #pragma HLS PIPELINE II=1
            rows_buffer[cy][cx] = *(input + (y + cy) * inputWidth + cx);
        }
    }
}

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

    #pragma HLS INTERFACE m_axi port=input offset=slave max_read_burst_length=256
    #pragma HLS INTERFACE m_axi port=output offset=slave max_read_burst_length=256
    #pragma HLS INTERFACE m_axi port=coeffs offset=slave max_read_burst_length=256

    TFXP filter_buffer[MAX_CHANNELS][CONV_SIZE][CONV_SIZE];
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=2
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=3

    TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE]; // cache 3 rows
    TFXP acc_buf[MAX_ROW_SIZE]; // accumulator per output column

    loop_filters : for (uint32_t iFilter = 0; iFilter < numFilters; ++iFilter) {

        coeffCaching(coeffs, filter_buffer, numChannels, iFilter);

        // For each output row y
        loop_y : for (uint32_t y = 0; y < (inputHeight-2); ++y) {
            // Initialize accumulators for this row
            for (uint32_t x = 0; x < (inputWidth-2); ++x) {
                acc_buf[x] = 0;
            }

            // For each channel, load 3 rows and slide across all x positions
            loop_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
                // Load 3 rows for current channel starting at row y
                rowCaching(input + iChannel * inputWidth * inputHeight, rows_buffer, inputWidth, y);

                // Slide 3x3 window across all x positions
                loop_x : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
                    for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
                        for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
                            TFXP pixelValue = rows_buffer[cy][x+cx];
                            acc_buf[x] += FXP_Mult(filter_buffer[iChannel][cy][cx], pixelValue, DECIMALS);
                        }
                    }
                }
            }

            // Write output row
            for (uint32_t x = 0; x < (inputWidth-2); ++x) {
                *(output + iFilter * (inputHeight-2)*(inputWidth-2) + y*(inputWidth-2) + x) = acc_buf[x];
            }
        }
    }
}
