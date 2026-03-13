############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
############################################################
set RUN_CSIM 0
if { [llength $argv] > 0 } {
    set RUN_CSIM [lindex $argv 0]
}

open_project midterm_vitis
set_top Conv2D_HW
add_files HLS/conv2d.cpp
add_files HLS/conv2d.h
add_files -tb HLS/conv2DTestbench.cpp

open_solution solution1 -flow_target vivado
set_part xc7z020clg400-1
create_clock -period 10 -name default
config_export -output ./IP

if { $RUN_CSIM } {
    csim_design -clean
}

csynth_design
export_design -rtl verilog -format ip_catalog -output ./IP
exit
