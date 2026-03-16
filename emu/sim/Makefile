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

.PHONY: all verilog model harness trace-harness clean

PROJ_ROOT = ../..
RTL_DIR = rtl
OBJ_DIR = obj_dir
HARNESS_DIR = harness
TRACE_DIR = ../trace
SIM_INTERFACE ?= core
SIM_TOP = $(if $(filter de10,$(SIM_INTERFACE)),CoreDe10,CoreSim)
SIM_GEN_CLASS = $(if $(filter de10,$(SIM_INTERFACE)),voodoo.de10.CoreDe10SimGen,voodoo.CoreSimGen)
RTL_FILE = $(RTL_DIR)/$(SIM_TOP).v
SIM_LIB = $(OBJ_DIR)/V$(SIM_TOP)__ALL.a
SIM_MK = V$(SIM_TOP).mk
HARNESS_SRC = $(if $(filter de10,$(SIM_INTERFACE)),$(HARNESS_DIR)/sim_harness_de10.cpp,$(HARNESS_DIR)/sim_harness.cpp)

# SpinalHDL sources that generate the Verilog
SCALA_SRCS := $(shell find $(PROJ_ROOT)/src -name '*.scala' ! -name '.#*') $(PROJ_ROOT)/project.scala
SCALA_CLI_SRCS = project.scala src

# Verilator settings
VERILATOR = verilator
VERILATOR_ROOT ?= $(shell $(VERILATOR) --getenv VERILATOR_ROOT)
VERILATOR_FLAGS = --cc --output-split 20000 --output-split-cfuncs 5000 --decorations none --public-flat-rw --x-assign fast --x-initial fast --noassert -Wno-WIDTHTRUNC -Wno-WIDTHEXPAND -Wno-CMPCONST -Wno-UNSIGNED
SCALA_CLI ?= scala-cli
SCALA_CLI_FLAGS ?= --power
CORE_SIM_GEN_ARGS ?=
ifeq ($(TRACE),1)
VERILATOR_FLAGS += --trace-fst --trace-structs --trace-underscore
endif

# 32-bit compilation (must match Glide's 32-bit build)
# CC32/CXX32 come from devenv; derive CXX32 from CC32 if not set
CC32 ?= gcc -m32
CXX32 ?= $(shell d=$$(dirname "$(firstword $(CC32))") && [ -x "$$d/g++" ] && echo "$$d/g++" || echo "g++ -m32")

# Use ccache if available (speeds up incremental rebuilds)
CCACHE := $(shell command -v ccache 2>/dev/null)
ifdef CCACHE
  OBJCACHE = ccache
endif

CXXFLAGS = -O3 -DVL_TIME_CONTEXT
ifeq ($(TRACE),1)
CXXFLAGS += -DVM_TRACE_FST
endif
CXXFLAGS += -I$(OBJ_DIR) -I$(VERILATOR_ROOT)/include -I$(VERILATOR_ROOT)/include/vltstd

TRACE_IO_FLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

# Stamp file: tracks when Verilator was last run successfully.
# Verilator regenerates obj_dir/*.cpp from the Verilog source.
VERILATE_STAMP = $(OBJ_DIR)/.$(SIM_TOP).verilate.stamp

# Trace mode sentinel: forces rebuild when TRACE= setting changes.
# Updated at parse time via $(shell) so the timestamp only changes on real transitions.
TRACE_MODE_FILE = $(OBJ_DIR)/.trace_mode
TRACE_MODE_VAL  = $(if $(filter 1,$(TRACE)),trace,notrace)
CORE_SIM_GEN_ARGS_FILE = $(OBJ_DIR)/.core_sim_gen_args
$(shell mkdir -p $(OBJ_DIR))
$(shell [ -f $(TRACE_MODE_FILE) ] && [ "$$(cat $(TRACE_MODE_FILE))" = "$(TRACE_MODE_VAL)" ] || echo "$(TRACE_MODE_VAL)" > $(TRACE_MODE_FILE))
$(shell [ -f $(CORE_SIM_GEN_ARGS_FILE) ] && [ "$$(cat $(CORE_SIM_GEN_ARGS_FILE))" = "$(CORE_SIM_GEN_ARGS)" ] || printf '%s\n' "$(CORE_SIM_GEN_ARGS)" > $(CORE_SIM_GEN_ARGS_FILE))

###############################################################################
# Targets
###############################################################################

all: model harness

# Step 1: Generate Verilog from SpinalHDL (rebuilds when Scala sources or TRACE mode change)
# scala-cli run has a LibC native-image bug; use java -cp workaround
$(RTL_FILE): $(SCALA_SRCS) $(TRACE_MODE_FILE) $(CORE_SIM_GEN_ARGS_FILE)
	@rm -f $(RTL_DIR)/*.bin
	cd $(PROJ_ROOT) && java -Xmx2g -cp "$$( $(SCALA_CLI) compile project.scala src --print-class-path $(SCALA_CLI_FLAGS) )" $(SIM_GEN_CLASS) $(if $(filter 1,$(TRACE)),--trace-pipeline) $(CORE_SIM_GEN_ARGS)
	@# Fix $readmemb paths: SpinalHDL emits bare filenames resolved relative to
	@# the process cwd.  Prepend the absolute RTL path so the Verilator model
	@# finds the .bin files regardless of where the binary runs from.
	@sed -i 's|\$$readmemb("\([^/"]*\.bin\)"|\$$readmemb("$(abspath $(RTL_DIR))/\1"|g' $@

verilog: $(RTL_FILE)

# Step 2: Run Verilator (rebuilds when Verilog source or TRACE mode changes)
$(VERILATE_STAMP): $(RTL_FILE) $(TRACE_MODE_FILE)
	$(VERILATOR) $(VERILATOR_FLAGS) $(RTL_FILE) --Mdir $(OBJ_DIR)
	@touch $@

# Step 3: Build Verilator model library
model: $(SIM_LIB)

$(SIM_LIB): $(VERILATE_STAMP)
	$(MAKE) -j$(shell nproc) -C $(OBJ_DIR) -f $(SIM_MK) \
		CXX="$(CXX32)" \
		OBJCACHE="$(OBJCACHE)" \
		OPT_FAST="-O3" \
		OPT_SLOW="" \
		OPT_GLOBAL="-DVL_USER_WARN"

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
$(OBJ_DIR)/sim_harness.o: $(HARNESS_SRC) $(HARNESS_DIR)/sim_harness.h $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX32) $(CXXFLAGS) -DVL_USER_WARN -c -o $@ $<

$(OBJ_DIR)/verilated.o: $(VERILATOR_ROOT)/include/verilated.cpp $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX32) $(CXXFLAGS) -DVL_USER_WARN -c -o $@ $<

$(OBJ_DIR)/verilated_fst_c.o: $(VERILATOR_ROOT)/include/verilated_fst_c.cpp $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX32) $(CXXFLAGS) -c -o $@ $<

# Step 5: Trace-capture harness (no Verilator dependency)
trace-harness: $(OBJ_DIR)/sim_trace_harness.o $(OBJ_DIR)/voodoo_trace_writer.o

$(OBJ_DIR)/voodoo_trace_writer.o: $(TRACE_DIR)/voodoo_trace_writer.c $(TRACE_DIR)/voodoo_trace_writer.h $(TRACE_DIR)/voodoo_trace_format.h
	$(OBJCACHE) $(CXX32) $(CXXFLAGS) $(TRACE_IO_FLAGS) -I$(TRACE_DIR) -c -o $@ $<

$(OBJ_DIR)/sim_trace_harness.o: $(HARNESS_DIR)/sim_trace_harness.c $(HARNESS_DIR)/sim_harness.h $(TRACE_DIR)/voodoo_trace_writer.h $(TRACE_DIR)/voodoo_trace_format.h
	$(OBJCACHE) $(CXX32) $(CXXFLAGS) $(TRACE_IO_FLAGS) -I$(TRACE_DIR) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(RTL_DIR)/CoreSim.v $(RTL_DIR)/CoreDe10.v $(RTL_DIR)/*.bin
