############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
############################################################
open_project midterm_vitis
set_top Conv2D_HW
add_files HLS/conv2d.cpp
add_files HLS/conv2d.h
add_files -tb HLS/conv2DTestbench.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg400-1}
create_clock -period 10 -name default
config_export -output /home/plogoz/lab_HW_SW/lab_HW_SW_midterm-main/IP
#source "./midterm_vitis/solution1/directives.tcl"
csim_design -clean
csynth_design
cosim_design
export_design -rtl verilog -format ip_catalog -output /home/plogoz/lab_HW_SW/lab_HW_SW_midterm-main/IP
