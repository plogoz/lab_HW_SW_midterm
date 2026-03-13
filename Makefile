.PHONY: ip vivado_project clean help
PROJECT_NAME := midterm

help:
	@echo "MAKEFILE targets:"
	@echo "  ip             - Synthesizes C++ to RTL and exports the IP to ./IP/"
	@echo "  vivado_project - Creates the Vivado block design using the generated IP"
	@echo "  clean          - Removes all generated projects, IP folders, and log files"

# Step 1: Generate IP
# This target depends on your C++ source files. If they change, the IP will be rebuilt.
ip: IP/component.xml

IP/component.xml: HLS/conv2d.cpp HLS/conv2d.h $(PROJECT_NAME)_vitis.tcl
	@echo "Running Vitis HLS..."
	rm -rf $(PROJECT_NAME)_vitis IP
	mkdir -p IP
	vitis_hls -f $(PROJECT_NAME)_vitis.tcl

# Step 2: Create Vivado Project
# This target depends on the IP generation step.
vivado_project: $(PROJECT_NAME)_vivado/$(PROJECT_NAME)_vivado.xpr

$(PROJECT_NAME)_vivado/$(PROJECT_NAME)_vivado.xpr: IP/component.xml $(PROJECT_NAME)_vivado.tcl
	@echo "Reconstructing Vivado Project..."
	rm -rf $(PROJECT_NAME)_vivado
	vivado -mode batch -source $(PROJECT_NAME)_vivado.tcl

# Step 3: Clean workspace
clean:
	@echo "Cleaning up generated files..."
	rm -rf $(PROJECT_NAME)_vitis $(PROJECT_NAME)_vivado IP .Xil
	rm -f vivado*.jou vivado*.log vivado*.str vitis_hls.log
