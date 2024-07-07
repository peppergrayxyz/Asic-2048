# Makefile for running Verilog simulation with Icarus Verilog (iverilog)
 
# Set SILENT to 's' if --quiet/-s set, otherwise ''.
ifeq ("ws", "w$(findstring s,$(word 1, $(MAKEFLAGS)))")
	SILENT = true
endif

ifdef SILENT
    VVP_DEV = /dev/null
else
    VVP_DEV = /dev/stdout
endif

# If the first argument is "view"
ifeq (view,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif

# Default simulator (Icarus Verilog)
SIMULATOR ?= iverilog -g2012

# Directories
SRC_DIR = ../src/modules
TB_DIR = tb
VCD_DIR = vcd

# Set the name of the output VCD file (waveform)
# VCD_FILE = $(VCD_DIR)/$(TESTBENCH).vcd

# Source files (Verilog files with the extension .sv in tb directory)
TB_FILES := $(wildcard $(TB_DIR)/*_tb.sv)
TESTBENCHES := $(notdir $(TB_FILES))

# Find corresponding design files in SRC_DIR
# DESIGN_FILES := $(patsubst $(TB_DIR)/%_tb.sv,$(SRC_DIR)/%.sv,$(TB_FILES))

# Default target: Run all testbenches
all: $(TESTBENCHES)

# Rule to run each testbench
$(TESTBENCHES): %_tb.sv: $(SRC_DIR)/%.sv $(TB_DIR)/%_tb.sv
	@printf "\n[$*]\n"
	$(SIMULATOR) -grelative-include -D'DUMP_FILE_NAME="$(patsubst %.sv,$(VCD_DIR)/%.vcd,$@)"' -o $(VCD_DIR)/$*.vpp $^
	vvp -N $(VCD_DIR)/$*.vpp > $(VVP_DEV)
	
# View the waveform using gtkwave for a specific testbench
view:
	gtkwave $(VCD_DIR)/$(RUN_ARGS)_tb.vcd &

# Clean up generated files
clean:
	rm -f $(VCD_DIR)/*.vcd $(VCD_DIR)/*.vpp

.PHONY: all $(TESTBENCHES) view clean
