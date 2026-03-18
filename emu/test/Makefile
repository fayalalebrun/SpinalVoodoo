# Makefile for trace_test — 64-bit native build
#
# Builds:
#   1. Verilator model of CoreSim (64-bit, separate from Glide's 32-bit build)
#   2. 86Box reference model (vid_voodoo_render.c + vid_voodoo_texture.c)
#   3. trace_test binary linking everything together
#
# Usage:
#   make              # build trace_test
#   make clean        # remove build artifacts

.PHONY: all clean state-roundtrip

PROJ_ROOT  = ../..
SIM_DIR    = ../sim
OBJ_DIR    = obj_dir
STUBS_DIR  = 86box_stubs

# 86Box paths
EBOX_SRC   = ../86Box/src/video
EBOX_INC   = ../86Box/src/include
TRACE_DIR  = ../trace

# Verilator
VERILATOR       = verilator
VERILATOR_ROOT ?= $(shell $(VERILATOR) --getenv VERILATOR_ROOT)
VERILATOR_FLAGS = --cc --output-split 20000 --output-split-cfuncs 5000 --decorations none --public-flat-rw --x-assign fast --x-initial fast --noassert -Wno-WIDTHTRUNC -Wno-WIDTHEXPAND -Wno-CMPCONST -Wno-UNSIGNED
SCALA_CLI ?= scala-cli
SCALA_CLI_FLAGS ?= --power
CORE_SIM_GEN_ARGS ?=
ifeq ($(TRACE),1)
VERILATOR_FLAGS += --trace-fst --trace-structs --trace-underscore
endif

# Native 64-bit compiler — prefer clang if available
CXX ?= $(shell command -v clang++ 2>/dev/null || echo g++)
CC  ?= $(shell command -v clang 2>/dev/null || echo gcc)

# Use ccache if available (speeds up incremental rebuilds)
CCACHE := $(shell command -v ccache 2>/dev/null)
ifdef CCACHE
  OBJCACHE = ccache
endif

CFLAGS   = -O2 -fwrapv -w -I$(STUBS_DIR) -I$(EBOX_INC) -I$(TRACE_DIR)
CXXFLAGS = -O2 -std=c++17 -I$(OBJ_DIR) -I$(VERILATOR_ROOT)/include -I$(VERILATOR_ROOT)/include/vltstd \
           -I$(SIM_DIR)/harness -I$(TRACE_DIR) -I. -DVL_TIME_CONTEXT
DEPFLAGS = -MMD -MP
ifeq ($(TRACE),1)
CXXFLAGS += -DVM_TRACE_FST
endif

# SpinalHDL Verilog source
VERILOG = $(SIM_DIR)/rtl/CoreSim.v
SCALA_SRCS := $(shell find $(PROJ_ROOT)/src -name '*.scala') $(PROJ_ROOT)/project.scala

# Stamps
VERILATE_STAMP = $(OBJ_DIR)/.verilate.stamp

# Trace mode sentinel: forces rebuild when TRACE= setting changes.
TRACE_MODE_FILE = $(OBJ_DIR)/.trace_mode
TRACE_MODE_VAL  = $(if $(filter 1,$(TRACE)),trace,notrace)
CORE_SIM_GEN_ARGS_FILE = $(OBJ_DIR)/.core_sim_gen_args
$(shell mkdir -p $(OBJ_DIR))
$(shell [ -f $(TRACE_MODE_FILE) ] && [ "$$(cat $(TRACE_MODE_FILE))" = "$(TRACE_MODE_VAL)" ] || echo "$(TRACE_MODE_VAL)" > $(TRACE_MODE_FILE))
$(shell [ -f $(CORE_SIM_GEN_ARGS_FILE) ] && [ "$$(cat $(CORE_SIM_GEN_ARGS_FILE))" = "$(CORE_SIM_GEN_ARGS)" ] || printf '%s\n' "$(CORE_SIM_GEN_ARGS)" > $(CORE_SIM_GEN_ARGS_FILE))

###############################################################################
# Targets
###############################################################################

all: $(OBJ_DIR)/trace_test
state-roundtrip: $(OBJ_DIR)/state_roundtrip_test

# Step 1: Generate Verilog (reuse existing or rebuild)
$(VERILOG): $(SCALA_SRCS) $(CORE_SIM_GEN_ARGS_FILE)
	@rm -f $(dir $(VERILOG))*.bin
	cd $(PROJ_ROOT) && java -Xmx2g -cp "$$( $(SCALA_CLI) compile project.scala src --print-class-path $(SCALA_CLI_FLAGS) )" voodoo.CoreSimGen $(if $(filter 1,$(TRACE)),--trace-pipeline) $(CORE_SIM_GEN_ARGS)
	@sed -i 's|\$$readmemb("\([^/"]*\.bin\)"|\$$readmemb("$(abspath $(dir $(VERILOG)))/\1"|g' $@

# Step 2: Verilate (64-bit obj_dir, separate from 32-bit Glide build)
$(VERILATE_STAMP): $(VERILOG) $(TRACE_MODE_FILE)
	@mkdir -p $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) $(VERILOG) --Mdir $(OBJ_DIR)
	@touch $@

# Step 3: Build Verilator model + runtime libraries (64-bit)
# OPT_GLOBAL passes -DVL_USER_WARN to the verilated runtime compilation,
# suppressing its default vl_warn so our override in sim_harness.cpp wins.
# Use a stamp file so parallel make doesn't invoke VCoreSim.mk twice.
VERILATOR_LIBS_STAMP = $(OBJ_DIR)/.verilator_libs.stamp
$(VERILATOR_LIBS_STAMP): $(VERILATE_STAMP)
	$(MAKE) -j$(shell nproc) -C $(OBJ_DIR) -f VCoreSim.mk \
		CXX="$(CXX)" \
		OBJCACHE="$(OBJCACHE)" \
		OPT_FAST="-O3" \
		OPT_SLOW="" \
		OPT_GLOBAL="-DVL_USER_WARN"
	@touch $@
$(OBJ_DIR)/VCoreSim__ALL.a: $(VERILATOR_LIBS_STAMP)
$(OBJ_DIR)/libverilated.a: $(VERILATOR_LIBS_STAMP)

# Step 4: 86Box reference objects (compiled as C)
$(OBJ_DIR)/vid_voodoo_render.o: $(EBOX_SRC)/vid_voodoo_render.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -Dvoodoo_queue_triangle=_voodoo_queue_triangle_86box -DREF_TRIANGLE_COVERAGE_HOOK -DREF_TRACE_EXPORT_HOOK \
		-c -o $@ $<

$(OBJ_DIR)/vid_voodoo_texture.o: $(EBOX_SRC)/vid_voodoo_texture.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/vid_voodoo_reg.o: $(EBOX_SRC)/vid_voodoo_reg.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/vid_voodoo_fb.o: $(EBOX_SRC)/vid_voodoo_fb.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/vid_voodoo_blitter.o: $(EBOX_SRC)/vid_voodoo_blitter.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/vid_voodoo_trace.o: $(EBOX_SRC)/vid_voodoo_trace.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/voodoo_trace_writer.o: $(TRACE_DIR)/voodoo_trace_writer.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/ref_model.o: ref_model.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

# Step 5: C++ objects
$(OBJ_DIR)/sim_harness.o: $(SIM_DIR)/harness/sim_harness.cpp Makefile $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX) $(CXXFLAGS) $(DEPFLAGS) -DVL_USER_WARN -c -o $@ $<

$(OBJ_DIR)/trace_test.o: trace_test.cpp Makefile $(TRACE_MODE_FILE) | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/state_roundtrip_test.o: state_roundtrip_test.cpp Makefile
	$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

# Step 6: Link
# Verilator runtime (verilated.o, verilated_threads.o, verilated_dpi.o) is in
# libverilated.a, built by VCoreSim.mk with OPT_GLOBAL="-DVL_USER_WARN" so
# the default vl_warn is excluded and our override in sim_harness.cpp takes effect.
OBJS = $(OBJ_DIR)/trace_test.o \
       $(OBJ_DIR)/ref_model.o \
       $(OBJ_DIR)/vid_voodoo_render.o \
       $(OBJ_DIR)/vid_voodoo_texture.o \
       $(OBJ_DIR)/vid_voodoo_reg.o \
       $(OBJ_DIR)/vid_voodoo_fb.o \
       $(OBJ_DIR)/vid_voodoo_blitter.o \
       $(OBJ_DIR)/vid_voodoo_trace.o \
       $(OBJ_DIR)/voodoo_trace_writer.o \
       $(OBJ_DIR)/sim_harness.o

$(OBJ_DIR)/trace_test: $(OBJS) $(OBJ_DIR)/VCoreSim__ALL.a $(OBJ_DIR)/libverilated.a
	$(CXX) -o $@ $(OBJS) $(OBJ_DIR)/VCoreSim__ALL.a $(OBJ_DIR)/libverilated.a -lm -lz -lpthread

ROUNDTRIP_OBJS = $(OBJ_DIR)/state_roundtrip_test.o \
                 $(OBJ_DIR)/ref_model.o \
                 $(OBJ_DIR)/vid_voodoo_render.o \
                 $(OBJ_DIR)/vid_voodoo_texture.o \
                 $(OBJ_DIR)/vid_voodoo_reg.o \
                 $(OBJ_DIR)/vid_voodoo_fb.o \
                 $(OBJ_DIR)/vid_voodoo_blitter.o \
                 $(OBJ_DIR)/vid_voodoo_trace.o \
                 $(OBJ_DIR)/voodoo_trace_writer.o

$(OBJ_DIR)/state_roundtrip_test: $(ROUNDTRIP_OBJS)
	$(CXX) -o $@ $(ROUNDTRIP_OBJS) -lm -lpthread

clean:
	rm -rf $(OBJ_DIR)

# Auto-generated header dependencies (created by -MMD -MP)
-include $(wildcard $(OBJ_DIR)/*.d)
