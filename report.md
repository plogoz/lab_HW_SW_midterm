# midterm task report:

## How to use the system: 

0. clone the repo in a directory where the Makefile will be able to create a new folder
1. Find the paths of you vivado and vitis binaries (unless you're running on a docker VM like me; PS: everything has been done on linux)
2. open a terminal in the git repo and do a "make" command will give you all the options of the makefile
```bash
MAKEFILE targets:
  ip             - Build/export IP (no C simulation)
  ip_sim         - Build/export IP with C simulation
  vivado - Creates the Vivado block design + runs synth, impl & bitstream
  clean          - Removes generated projects, IP folders, and logs

Tool overrides:
  make ip VITIS_HLS_BIN=/path/to/vitis_hls VIVADO_BIN=/path/to/vivado
```
3. Run make vivado with you vivado and vitis binaries paths (as written in the makefile) to create everything right to the bitstream
4. Copy via scp these files and folders : *CNN_Dogs_Cats_HW_version* , *SW* which contains the drivers, *midterm.bit*, *midterm.hwh* and *programOverlay.py*.
5. flash the bitstream with the python file
6. build the convolution, by running make inside the *CNN_Dogs_Cats_HW_version* folder.
7. run as root the executable and give it an image 


## Table:

| Name | Description | Time (s) | Frequency (MHz) | LUTs | FFs | BRAMs | DSPs | Cost | Pareto ? (Yes / No) |
|-----------------|-----------------|---------|---------|---------|---------|---------|---------|---------|---------|
| SW | macOS M1 Max software version | 0.457 | n/a | n/a | n/a | n/a | n/a | 0 | Yes |
| SW | SW only on PYNQ | 28.079 | x | n/a | n/a | n/a | n/a | 0 | yes |
| HW | Software verison directly rrun into Vitis and Vivado | 87.635 | 100 | 5785 | 6938 | 0 | 45 | 9,46 | no |
| HW | task 2 | 38.21 | 100 | 3293 | 5076 | 5 | 22 | 6,1 | yes |
|HW | task 3 | 8.6 | 100 | 19058 | 6227 | 125 | 46 | 37,9 | no |
|HW | task 4 | 4.4 | 50 | 9627 | 27724 | 62.5 | 171 | 41,6| yes |

## Design report:

### Task 1 (on the main branch):

It's basically the software verison with a few pragmas for creating the AXI buses. The drivers in the SW folder needed some additions to allow the hardware mapping of the buses.

```C++
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
```

### Task 2 (branch task2):

I added a coefficient buffer to store the coefficients, so that they're not read thousand of times form the memory. I've computed that the biggest buffer for the layer was 256 * 3 * 3 as expected, and then implemented it as a 1D array (which is useless to make any arry partitionning, and thus will be reimplemented as 3D array in task 3). 

```C++
TFXP filter_bufffer[CONV_SIZE * CONV_SIZE * MAX_CHANNELS];
```

```C++
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
```

The cacing of these filters, and unrolling of some of the loops with compile time known iterations allowed a drastic speed-up from 87s down to 38.

### Task 3 (branch task 3):

The next step to increas throughput was to cache the rows in a buffer. I wrote a function to do so (it was actually only for readabilty), 

```C++
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
```
placed the coefficient caching in a function and rewrote the filter_buffer as a 3D array to make array partitionning.

```C++
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
```

```C++

TFXP filter_buffer[MAX_CHANNELS][CONV_SIZE][CONV_SIZE];
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=2
    #pragma HLS ARRAY_PARTITION variable=filter_buffer complete dim=3

    TFXP rows_buffer[CONV_SIZE][MAX_ROW_SIZE]; // cache 3 rows
    TFXP acc_buf[MAX_ROW_SIZE]; // accumulator per output column
```

Due to my will to also paralleize the filter computations (and I know it's a point that will come later), I added acc_buffer to be able to accumulate the parallel operations and then write everythin in the output.

### task 4 (branch task4):

So, after task 3, where I indeed managed to also fulfill task 4, to speed up a bit the computations, I added a second AXI bus for the coefficients (so that inputs and coeffs can be read at the same time). In the Xilinx doc, I found somewhere where they spoke about the write and read bursts for specific cases (i.e. read only or write only), and use their configs for my buses. After that, due to negative slack issues in my design, and without wanting to rewrtie everything, I passed all my functions to HLS INLINE and passed the computation loops to datastream, mainly due to the lower frequency I needed anyway, and also to have freerunning pipelines (the style=frp), also found in the Xilinx doc. By doing this I could also speedup the output writes (and added a nested loop, again !). All this lead to just less than 4 seconds for the convolution part only 🥳.

### Tests and debugg: 

Due to the fact I have an apple sillicon mac, I don't have any ways of running the tools natively on the hardware. This exact point renderd the use of the vitis simulation impossible (due to the combination of the ubuntu version of my docker container and the Vivado an Vitis edition). Instead, I just used a bash script (HLS/build.sh) and needed to comment the imports not recognized by clang.

For the debugging insde the pynq, I didn't use any system ILAs or Vivado simulations, albeit the ILA would have saved me some time. I relied on vitis (csynth.rpt reports and GUI reports) to have infos about the burst trasfers. When I encounterd AXI related issues, I could deduce them mainly because the code passed the testbench on my computer, but was giving constant results in the PYNQ board (which is onyl possible if a constant, non-memory related issue is hanging around).

About the design procedures, I followed an iterative approach, mainly to observe what Vitis what doing with which pragmas, thus learning by trials and errors.

### AI disclosure:

I did use gen AI (github copilot and claude free tier) to help me write the synthesis part of the vivado TCL script (and to make sure it would be usable outside of my computer), the *HLS/build.sh* file and the Makefile in *CNN_Dogs_Cats_HW_version*, to be able to link the drivers from the SW folder, without needing to copy the files. As I used Zed and VScode as code editors, I did use their respective inline suggestions (Zed AI and github copilot). It only helped to write the some repetitive parts faster (like brackets oft loops beeing automatically inserted when I was messing arround whith nested loops). It just helped me have cleaner formatted code. Any remotely related code about the implementation was useless, none of the code the sugestions where correct.
