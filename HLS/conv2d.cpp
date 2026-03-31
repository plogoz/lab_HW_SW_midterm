#include "conv2d.h"
#include <cstdint>

void coeffCaching(TFXP *coeffs, TFXP filter_buffer[NUM_OUTPUT_FILTER][MAX_CHANNELS][CONV_SIZE][CONV_SIZE], uint32_t numChannels, uint32_t iFilter) {
    #pragma HLS INLINE

    parallel_coeffs : for(uint32_t iFilterP = 0; iFilterP < NUM_OUTPUT_FILTER; ++iFilterP) {

        load_coeff : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
            #pragma HLS loop_tripcount max=256

            for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {

                load_coeff_kx : for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
                    #pragma HLS PIPELINE II=1

                    filter_buffer[iFilterP][iChannel][cy][cx] = *(coeffs + (iFilter + iFilterP)*numChannels*CONV_SIZE*CONV_SIZE + iChannel*CONV_SIZE*CONV_SIZE + cy*CONV_SIZE + cx);
                }
            }
        }
    }
}

void rowCaching(TFXP *input, TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE], uint32_t inputWidth, uint32_t y) {
    #pragma HLS INLINE

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

    #pragma HLS INTERFACE m_axi port=input  offset=slave bundle=gmem0 max_widen_bitwidth=128 max_read_burst_length=256
    #pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem0 max_widen_bitwidth=128 max_write_burst_length=256 num_write_outstanding=32
    #pragma HLS INTERFACE m_axi port=coeffs offset=slave bundle=gmem1 max_widen_bitwidth=128 max_read_burst_length=256 num_read_outstanding=32

    TFXP filter_buffer[NUM_OUTPUT_FILTER][MAX_CHANNELS][CONV_SIZE][CONV_SIZE];
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=3
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=4

    TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE]; // cache 3 rows
    #pragma HLS ARRAY_PARTITION variable=rows_buffer complete dim=1
    #pragma HLS ARRAY_PARTITION variable=rows_buffer cyclic factor=3 dim=2

    TFXP acc_buf[NUM_OUTPUT_FILTER][MAX_ROW_SIZE]; // accumulator per output column
    #pragma HLS ARRAY_PARTITION variable=acc_buf complete dim=1

    loop_filters : for (uint32_t iFilter = 0; iFilter < numFilters; iFilter+= NUM_OUTPUT_FILTER) {

        coeffCaching(coeffs, filter_buffer, numChannels, iFilter);

        // For each output row y
        loop_y : for (uint32_t y = 0; y < (inputHeight-2); ++y) {
            #pragma HLS loop_tripcount max=256

            // Initialize accumulators for this row
            for (uint32_t iFilterP = 0; iFilterP < NUM_OUTPUT_FILTER; ++iFilterP) {
                for (uint32_t x = 0; x < (inputWidth-2); ++x) {
                    #pragma HLS loop_tripcount max=256
                    acc_buf[iFilterP][x] = 0;
                }
            }

            // For each channel, load 3 rows and slide across all x positions
            loop_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
                // Load 3 rows for current channel starting at row y
                rowCaching(input + iChannel * inputWidth * inputHeight, rows_buffer, inputWidth, y);

                // Slide 3x3 window across all x positions
                loop_x : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
                    #pragma HLS PIPELINE II=1
                    #pragma HLS loop_tripcount max=254
                    for (uint32_t iFilterP = 0; iFilterP < NUM_OUTPUT_FILTER; ++iFilterP) {
                        #pragma HLS UNROLL
                        TFXP sum = acc_buf[iFilterP][x];
                        for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
                            #pragma HLS UNROLL
                            for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
                                #pragma HLS UNROLL
                                sum += FXP_Mult(filter_buffer[iFilterP][iChannel][cy][cx], rows_buffer[cy][x+cx], DECIMALS);
                            }
                        }
                        acc_buf[iFilterP][x] = sum;
                    }
                }
            }

            // Write output row

            for (uint32_t iFilterP = 0; iFilterP < NUM_OUTPUT_FILTER; ++iFilterP) {

                write_out : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
                    #pragma HLS PIPELINE II=1

                   *(output + (iFilter + iFilterP) * (inputHeight-2)*(inputWidth-2) + y*(inputWidth-2) + x) = acc_buf[iFilterP][x];
                }
            }
        }
    }
}
