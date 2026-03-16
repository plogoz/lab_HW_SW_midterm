.PHONY: ip ip_sim vivado_project clean help check_tools

PROJECT_NAME := midterm

# ============================================================================
# ENVIRONMENT DETECTION
# ============================================================================

# Detect if running in Docker
IN_DOCKER := $(shell [ -f /.dockerenv ] && echo 1 || echo 0)

# Set tool paths based on environment
ifeq ($(IN_DOCKER),1)
    # Docker container paths
    VITIS_HLS_BIN ?= /home/user/Xilinx/Vitis_HLS/2022.2/bin/vitis_hls
    VIVADO_BIN    ?= /home/user/Xilinx/Vivado/2022.2/bin/vivado
    ENV_NAME := Docker
else
    # Linux course environment paths
    VITIS_HLS_BIN ?= /scrap/users/Xilinx2022.2/Vitis_HLS/2022.2/bin/vitis_hls
    VIVADO_BIN    ?= /scrap/users/Xilinx2022.2/Vivado/2022.2/bin/vivado
    ENV_NAME := Linux
endif

# HLS flow option: 0 = no C simulation, 1 = run csim
CSIM ?= 0

# HLS flow option: 0 = no C simulation, 1 = run csim
CSIM ?= 0

# ============================================================================
# HELP & CONFIGURATION
# ============================================================================

help:
	@echo "MAKEFILE targets:"
	@echo "  ip             - Build/export IP (no C simulation)"
	@echo "  ip_sim         - Build/export IP with C simulation"
	@echo "  vivado_project - Creates the Vivado block design + runs synth, impl & bitstream"
	@echo "  clean          - Removes generated projects, IP folders, and logs"
	@echo ""
	@echo "Tool overrides:"
	@echo "  make ip VITIS_HLS_BIN=/path/to/vitis_hls VIVADO_BIN=/path/to/vivado"

check_tools:
	@test -x "$(VITIS_HLS_BIN)" || (echo "ERROR: VITIS_HLS_BIN not executable: $(VITIS_HLS_BIN)"; exit 1)
	@test -x "$(VIVADO_BIN)"    || (echo "ERROR: VIVADO_BIN not executable: $(VIVADO_BIN)"; exit 1)
	@command -v unzip >/dev/null 2>&1 || (echo "ERROR: 'unzip' not found in PATH. Please install it."; exit 1)

# Step 1a: Generate IP without simulation (default)
ip: CSIM=0
ip: IP/component.xml

# Step 1b: Generate IP with C simulation
ip_sim: CSIM=1
ip_sim: IP/component.xml

# Vitis HLS exports the IP as a .zip archive inside ./IP.
# The TCL script (midterm_vitis.tcl) runs unzip after export_design so that
# Vivado can find component.xml directly under ./IP via ip_repo_paths.
IP/component.xml: HLS/conv2d.cpp HLS/conv2d.h $(PROJECT_NAME)_vitis.tcl | check_tools
	@echo "Running Vitis HLS... (CSIM=$(CSIM))"
	rm -rf $(PROJECT_NAME)_vitis IP
	mkdir -p IP
	"$(VITIS_HLS_BIN)" -f $(PROJECT_NAME)_vitis.tcl -tclargs $(CSIM)
	# @test -f IP/component.xml || (echo "ERROR: IP/component.xml not found after Vitis HLS run. Check vitis_hls.log for errors."; exit 1)

# Step 2: Create Vivado Project, run synthesis, implementation, and generate bitstream
BITSTREAM := $(PROJECT_NAME)_vivado/$(PROJECT_NAME)_vivado.runs/impl_1/design_1_wrapper.bit

vivado_project: $(BITSTREAM)

$(BITSTREAM): IP/component.xml $(PROJECT_NAME)_vivado.tcl | check_tools
	@echo "Reconstructing Vivado project and running full build flow..."
	rm -rf $(PROJECT_NAME)_vivado
	"$(VIVADO_BIN)" -mode batch -source $(PROJECT_NAME)_vivado.tcl
	@test -f $@ || (echo "ERROR: Bitstream not found at $@. Check vivado*.log for details."; exit 1)
	@echo "SUCCESS: Bitstream generated at $@"

# Step 3: Clean workspace
clean:
	@echo "Cleaning up generated files..."
	rm -rf $(PROJECT_NAME)_vitis $(PROJECT_NAME)_vivado IP .Xil
	rm -f vivado*.jou vivado*.log vivado*.str vitis_hls.log
