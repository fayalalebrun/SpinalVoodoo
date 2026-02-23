# Makefile for building the CoreSim Verilator model
#
# Steps:
#   1. verilog:  Generate Verilog from SpinalHDL CoreSim
#   2. verilate: Run Verilator on generated Verilog
#   3. model:    Build the Verilator C++ model library
#   4. harness:  Build the sim_harness.o wrapper
#
# Usage:
#   make all          # build everything
#   make verilog      # just generate Verilog
#   make clean        # remove build artifacts

.PHONY: all verilog model harness clean

PROJ_ROOT = ../..
RTL_DIR = rtl
OBJ_DIR = obj_dir
HARNESS_DIR = harness

# SpinalHDL sources that generate the Verilog
SCALA_SRCS := $(shell find $(PROJ_ROOT)/src -name '*.scala') $(PROJ_ROOT)/project.scala

# Verilator settings
VERILATOR = verilator
VERILATOR_ROOT ?= $(shell $(VERILATOR) --getenv VERILATOR_ROOT)
VERILATOR_FLAGS = --cc --output-split 20000 --public-flat-rw --x-assign fast --x-initial fast --noassert -Wno-fatal
ifeq ($(TRACE),1)
VERILATOR_FLAGS += --trace-fst --trace-structs --trace-underscore
endif

# 32-bit compilation (must match Glide's 32-bit build)
# CC32/CXX32 come from devenv; derive CXX32 from CC32 if not set
CC32 ?= gcc -m32
CXX32 ?= $(shell d=$$(dirname "$(firstword $(CC32))") && [ -x "$$d/g++" ] && echo "$$d/g++" || echo "g++ -m32")

CXXFLAGS = -O3 -DVL_TIME_CONTEXT
ifeq ($(TRACE),1)
CXXFLAGS += -DVM_TRACE_FST
endif
CXXFLAGS += -I$(OBJ_DIR) -I$(VERILATOR_ROOT)/include -I$(VERILATOR_ROOT)/include/vltstd

# Stamp file: tracks when Verilator was last run successfully.
# Verilator regenerates obj_dir/*.cpp from the Verilog source.
VERILATE_STAMP = $(OBJ_DIR)/.verilate.stamp

# Trace mode sentinel: forces rebuild when TRACE= setting changes.
# Updated at parse time via $(shell) so the timestamp only changes on real transitions.
TRACE_MODE_FILE = $(OBJ_DIR)/.trace_mode
TRACE_MODE_VAL  = $(if $(filter 1,$(TRACE)),trace,notrace)
$(shell mkdir -p $(OBJ_DIR))
$(shell [ -f $(TRACE_MODE_FILE) ] && [ "$$(cat $(TRACE_MODE_FILE))" = "$(TRACE_MODE_VAL)" ] || echo "$(TRACE_MODE_VAL)" > $(TRACE_MODE_FILE))

###############################################################################
# Targets
###############################################################################

all: model harness

# Step 1: Generate Verilog from SpinalHDL (rebuilds when Scala sources change)
# scala-cli run has a LibC native-image bug; use java -cp workaround
$(RTL_DIR)/CoreSim.v: $(SCALA_SRCS)
	cd $(PROJ_ROOT) && java -Xmx2g -cp "$$(scala-cli compile . --print-class-path 2>/dev/null)" voodoo.CoreSimGen

verilog: $(RTL_DIR)/CoreSim.v

# Step 2: Run Verilator (rebuilds when Verilog source or TRACE mode changes)
$(VERILATE_STAMP): $(RTL_DIR)/CoreSim.v $(TRACE_MODE_FILE)
	$(VERILATOR) $(VERILATOR_FLAGS) $(RTL_DIR)/CoreSim.v --Mdir $(OBJ_DIR)
	@touch $@

# Step 3: Build Verilator model library
model: $(OBJ_DIR)/VCoreSim__ALL.a

$(OBJ_DIR)/VCoreSim__ALL.a: $(VERILATE_STAMP)
	$(MAKE) -C $(OBJ_DIR) -f VCoreSim.mk \
		CXX="$(CXX32)" \
		OPT_FAST="-O3" \
		OPT_SLOW="-O1"

# Step 4: Build harness object + verilated runtime
HARNESS_OBJS = $(OBJ_DIR)/sim_harness.o $(OBJ_DIR)/verilated.o
ifeq ($(TRACE),1)
HARNESS_OBJS += $(OBJ_DIR)/verilated_fst_c.o
endif
harness: $(HARNESS_OBJS)

# sim_harness.o only needs generated headers (from verilate), not the compiled model.
# verilated.o / verilated_fst_c.o only need obj_dir to exist.
# Using order-only deps on the stamp so these compile in parallel with the model.
# TRACE_MODE_FILE forces recompilation when TRACE= changes (CXXFLAGS differ).
$(OBJ_DIR)/sim_harness.o: $(HARNESS_DIR)/sim_harness.cpp $(HARNESS_DIR)/sim_harness.h $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(CXX32) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/verilated.o: $(VERILATOR_ROOT)/include/verilated.cpp $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(CXX32) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/verilated_fst_c.o: $(VERILATOR_ROOT)/include/verilated_fst_c.cpp $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(CXX32) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(RTL_DIR)/CoreSim.v
