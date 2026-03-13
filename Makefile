.PHONY: ip ip_sim vivado_project clean help check_tools

PROJECT_NAME := midterm

# Tool binaries (override per machine via env vars or CLI)
VITIS_HLS_BIN ?= /scrap/users/Xilinx2022.2/Vitis_HLS/2022.2/bin/vitis_hls
VIVADO_BIN    ?= /scrap/users/Xilinx2022.2/Vivado/2022.2/bin/vivado

# HLS flow option: 0 = no C simulation, 1 = run csim
CSIM ?= 0

help:
	@echo "MAKEFILE targets:"
	@echo "  ip             - Build/export IP (no C simulation)"
	@echo "  ip_sim         - Build/export IP with C simulation"
	@echo "  vivado_project - Creates the Vivado block design using the generated IP"
	@echo "  clean          - Removes generated projects, IP folders, and logs"
	@echo ""
	@echo "Tool overrides:"
	@echo "  make ip VITIS_HLS_BIN=/path/to/vitis_hls VIVADO_BIN=/path/to/vivado"

check_tools:
	@test -x "$(VITIS_HLS_BIN)" || (echo "ERROR: VITIS_HLS_BIN not executable: $(VITIS_HLS_BIN)"; exit 1)
	@test -x "$(VIVADO_BIN)"    || (echo "ERROR: VIVADO_BIN not executable: $(VIVADO_BIN)"; exit 1)

# Step 1a: Generate IP without simulation (default)
ip: CSIM=0
ip: IP/component.xml

# Step 1b: Generate IP with C simulation
ip_sim: CSIM=1
ip_sim: IP/component.xml

IP/component.xml: HLS/conv2d.cpp HLS/conv2d.h $(PROJECT_NAME)_vitis.tcl | check_tools
	@echo "Running Vitis HLS... (CSIM=$(CSIM))"
	rm -rf $(PROJECT_NAME)_vitis IP
	mkdir -p IP
	"$(VITIS_HLS_BIN)" -f $(PROJECT_NAME)_vitis.tcl -tclargs $(CSIM)

# Step 2: Create Vivado Project
vivado_project: $(PROJECT_NAME)_vivado/$(PROJECT_NAME)_vivado.xpr

$(PROJECT_NAME)_vivado/$(PROJECT_NAME)_vivado.xpr: IP/component.xml $(PROJECT_NAME)_vivado.tcl | check_tools
	@echo "Reconstructing Vivado Project..."
	rm -rf $(PROJECT_NAME)_vivado
	"$(VIVADO_BIN)" -mode batch -source $(PROJECT_NAME)_vivado.tcl

# Step 3: Clean workspace
clean:
	@echo "Cleaning up generated files..."
	rm -rf $(PROJECT_NAME)_vitis $(PROJECT_NAME)_vivado IP .Xil
	rm -f vivado*.jou vivado*.log vivado*.str vitis_hls.log
