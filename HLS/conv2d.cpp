#include "conv2d.h"
#include <cstdint>

#define NUM_OUTPUT_FILTER 4 // nécessairement 2^n

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

void rowCaching(TFXP *input, uint32_t inputWidth, uint32_t inputHeight, uint32_t numChannels, TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE][MAX_CHANNELS], uint32_t y) {
    #pragma HLS PIPELINE II=1

    if (y == 0) {
        row : for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
            #pragma HLS UNROLL

            col : for (uint32_t cx = 0; cx < inputWidth; ++cx) {
                #pragma HLS loop_tripcount max=256
                #pragma HLS loop_merge force

                loop_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
                    #pragma HLS loop_tripcount max=256
                    #pragma HLS PIPELINE II=1

                    TFXP *ptr_in = input + iChannel * inputWidth * inputHeight;

                    rows_buffer[cy][cx][iChannel] = *(ptr_in + (y + cy) * inputWidth + cx);
                }
            }
        }
    }
    else {
        mov_rows_y : for (uint32_t cy = 1; cy < CONV_SIZE; ++cy) {
            #pragma HLS UNROLL

            mov_row_x : for (uint32_t cx = 0; cx < inputWidth; ++cx) {
                #pragma HLS loop_tripcount max=256
                #pragma HLS loop_merge force

                shift_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
                    #pragma HLS loop_tripcount max=256
                    #pragma HLS PIPELINE II=1

                    rows_buffer[cy - 1][cx][iChannel] = rows_buffer[cy][cx][iChannel];
                }
            }
        }

        load_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
            #pragma HLS loop_tripcount max=256

            TFXP *ptr_in = input + iChannel * inputWidth * inputHeight + (y + CONV_SIZE - 1) * inputWidth;

            load_new_row : for (uint32_t cx = 0; cx < inputWidth; ++cx) {
                #pragma HLS loop_tripcount max=256
                #pragma HLS PIPELINE II=1
                #pragma HLS loop_flatten off

                rows_buffer[CONV_SIZE - 1][cx][iChannel] = *(ptr_in + cx);
            }
        }
    }
}

void gridComputation(TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE][MAX_CHANNELS], TFXP filter_buffer[MAX_CHANNELS][CONV_SIZE][CONV_SIZE], TFXP acc_buffer[MAX_ROW_SIZE], uint32_t inputWidth, uint32_t numChannels) {
    #pragma HLS dependence variable=rows_buffer class=aray type=inter direction=WAR distance=CONV_SIZE

    loop_cy : for (uint32_t cy = 0; cy < CONV_SIZE; ++cy) {
        #pragma HLS UNROLL

        loop_cx : for (uint32_t cx = 0; cx < CONV_SIZE; ++cx) {
            #pragma HLS UNROLL

            loop_channel : for (uint32_t iChannel = 0; iChannel < numChannels; ++iChannel) {
                #pragma HLS loop_tripcount max=256
                #pragma HLS loop_merge force

                loop_x : for (uint32_t x = 0; x < (inputWidth-2); ++x) {
                    #pragma HLS PIPELINE II=1
                    #pragma HLS loop_tripcount max=256

                    acc_buffer[x] += FXP_Mult(filter_buffer[iChannel][cy][cx], rows_buffer[cy][x+cx][iChannel], DECIMALS);
                }
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

    TFXP filter_buffer[NUM_OUTPUT_FILTER][MAX_CHANNELS][CONV_SIZE][CONV_SIZE];
    #pragma HLS ARRAY_PARTITION variable=filter_buffer cyclic factor=4 dim=3
    #pragma HLS ARRAY_PARTITION variable=filter_buffer cyclic factor=4 dim=4

    static TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE][MAX_CHANNELS]; // cache 3 rows
    #pragma HLS ARRAY_PARTITION variable=rows_buffer complete dim=1
    // cyclic factor=4 on dim=2: for any x, accesses [x],[x+1],[x+2] hit banks x%4,(x+1)%4,(x+2)%4

    #pragma HLS ARRAY_RESHAPE variable=rows_buffer cyclic factor=4 dim=2
    #pragma HLS ARRAY_RESHAPE variable=rows_buffer cyclic factor=4 dim=3 // un peu un test pour dim 3
    #pragma HLS BIND_STORAGE variable=rows_buffer type=RAM_2P impl=BRAM


    TFXP acc_buffer[NUM_OUTPUT_FILTER][MAX_ROW_SIZE]; // accumulator per output column
    #pragma HLS ARRAY_PARTITION variable=acc_buffer cyclic factor=4 dim=1
    #pragma HLS BIND_STORAGE variable=acc_buffer type=RAM_2P impl=LUTRAM


    loop_filters : for (uint32_t iFilter = 0; iFilter < numFilters; iFilter += NUM_OUTPUT_FILTER) {
        #pragma HLS DEPENDENCE variable=filter_buffer intra false
        #pragma HLS PIPELINE II=1

        filter_caching : for(uint32_t iFilterP = 0; iFilterP < NUM_OUTPUT_FILTER; ++iFilterP) {
            coeffCaching(coeffs, filter_buffer[iFilterP], numChannels, iFilter + iFilterP);
        }
        // For each output row y

        parallel_comp : for(uint32_t iFilterP = 0; iFilterP < NUM_OUTPUT_FILTER; ++iFilterP) {
            #pragma HLS UNROLL

            loop_y : for (uint32_t y = 0; y < (inputHeight-2); ++y) {
                #pragma HLS loop_tripcount max=256
                #pragma HLS PIPELINE II=1

                accInitialization(acc_buffer[iFilterP], inputWidth);

                rowCaching(input, inputWidth, inputHeight, numChannels, rows_buffer, y);

                gridComputation(rows_buffer, filter_buffer[iFilterP], acc_buffer[iFilterP], inputWidth, numChannels);

                writeOutput(output, acc_buffer[iFilterP], iFilter + iFilterP, inputHeight, inputWidth, y);
            }
        }
    }
}
