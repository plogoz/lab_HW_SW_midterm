#include "conv2d.h"
#include <cstdint>

void coeffCaching(TFXP *coeffs, TFXP filter_buffer[MAX_CHANNELS][CONV_SIZE][CONV_SIZE], uint32_t numChannels, uint32_t iFilter) {

    load_coeff : for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
        #pragma HLS UNROLL

        for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
            #pragma HLS UNROLL

            for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
                #pragma HLS loop_tripcount max=256
                #pragma HLS PIPELINE II=1

              filter_buffer[iChannel][cy][cx] = *(coeffs + iFilter*numChannels*CONV_SIZE*CONV_SIZE + iChannel*CONV_SIZE*CONV_SIZE + cy*CONV_SIZE + cx);
            }
        }
    }
}

void rowCaching(TFXP *input, TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE], uint32_t inputWidth, uint32_t y) {
    // cy outer + UNROLL → HLS issues one burst per row (3 bursts total), AXI-efficient
    row : for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
        #pragma HLS UNROLL

        col : for (uint32_t cx = 0; cx < inputWidth; ++cx) {
            #pragma HLS loop_tripcount max=256
            #pragma HLS PIPELINE II=1

            rows_buffer[cy][cx] = *(input + (y + cy) * inputWidth + cx);
        }
    }
}

void gridComputation(TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE], TFXP filter_buffer[MAX_CHANNELS][CONV_SIZE][CONV_SIZE], TFXP acc_buffer[MAX_ROW_SIZE], uint32_t iChannel, uint32_t inputWidth) {
    #pragma HLS dependence variable=rows_buffer class=aray type=inter direction=WAR distance=CONV_SIZE

    loop_x : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
        #pragma HLS PIPELINE II=1
        #pragma HLS loop_tripcount max=256

        loop_cy : for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
            #pragma HLS UNROLL

            loop_cx : for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
                #pragma HLS UNROLL

                acc_buffer[x] += FXP_Mult(filter_buffer[iChannel][cy][cx], rows_buffer[cy][x+cx], DECIMALS);
            }
        }
    }
}

void writeOutput(TFXP *output, TFXP acc_buffer[MAX_ROW_SIZE], uint32_t iFilter, uint32_t inputHeight, uint32_t inputWidth, uint32_t y) {
    write_out : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
        #pragma HLS PIPELINE II=1
        #pragma loop_tripcount max=256

        *(output + iFilter * (inputHeight-2)*(inputWidth-2) + y*(inputWidth-2) + x) = acc_buffer[x];
    }
}

void accInitialization(TFXP acc_buffer[MAX_ROW_SIZE], uint32_t inputWidth) {
    acc_init : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
        #pragma HLS PIPELINE II=1
        #pragma HLS loop_tripcount max=256

        acc_buffer[x] = 0;
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

    #pragma HLS INTERFACE m_axi port=input offset=slave max_read_burst_length=256 max_write_burst_length=256 bundle=gmem0
    #pragma HLS INTERFACE m_axi port=output offset=slave max_write_burst_length=256 max_read_burst_length=256 bundle=gmem1
    #pragma HLS INTERFACE m_axi port=coeffs offset=slave max_read_burst_length=256 max_write_burst_length=256 bundle=gmem1

    TFXP filter_buffer[MAX_CHANNELS][CONV_SIZE][CONV_SIZE];
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=2
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=3

    TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE]; // cache 3 rows
    #pragma HLS ARRAY_PARTITION variable=rows_buffer complete dim=1
    // cyclic factor=4 on dim=2: for any x, accesses [x],[x+1],[x+2] hit banks x%4,(x+1)%4,(x+2)%4
    // which are always 3 distinct banks → no conflict → II=1 achievable
    #pragma HLS ARRAY_RESHAPE variable=rows_buffer cyclic factor=4 dim=2

    TFXP acc_buffer[MAX_ROW_SIZE]; // accumulator per output column
    // PARTITION (not RESHAPE): creates separate memories, no wide MUX in critical path → fixes timing
    #pragma HLS ARRAY_PARTITION variable=acc_buffer cyclic factor=4
    // #pragma HLS BIND_STORAGE variable=acc_buffer type=RAM_2P impl=BRAM

    loop_filters : for (uint32_t iFilter = 0; iFilter < numFilters; ++iFilter) {
        #pragma HLS DEPENDENCE variable=filter_buffer intra false


        coeffCaching(coeffs, filter_buffer, numChannels, iFilter);

        // For each output row y
        loop_y : for (uint32_t y = 0; y < (inputHeight-2); ++y) {
            #pragma HLS loop_tripcount max=256

            // Initialize accumulators for this row
            accInitialization(acc_buffer, inputWidth);

            // For each channel, load 3 rows and slide across all x positions
            loop_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
                #pragma HLS loop_tripcount max=256

                // Load 3 rows for current channel starting at row y
                rowCaching(input + iChannel * inputWidth * inputHeight, rows_buffer, inputWidth, y);

                // Slide 3x3 window across all x positions
                gridComputation(rows_buffer, filter_buffer, acc_buffer, iChannel, inputWidth);
            }

            // Write output row
            writeOutput(output, acc_buffer, iFilter,  inputHeight, inputWidth, y);
        }
    }
}
