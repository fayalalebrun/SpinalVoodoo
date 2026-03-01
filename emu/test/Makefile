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

.PHONY: all clean

PROJ_ROOT  = ../..
SIM_DIR    = ../sim
OBJ_DIR    = obj_dir
STUBS_DIR  = 86box_stubs

# 86Box paths
EBOX_SRC   = ../86Box/src/video
EBOX_INC   = ../86Box/src/include

# Verilator
VERILATOR       = verilator
VERILATOR_ROOT ?= $(shell $(VERILATOR) --getenv VERILATOR_ROOT)
VERILATOR_FLAGS = --cc --output-split 20000 --output-split-cfuncs 5000 --decorations none --public-flat-rw --x-assign fast --x-initial fast --noassert -Wno-WIDTHTRUNC -Wno-WIDTHEXPAND -Wno-CMPCONST -Wno-UNSIGNED

# Native 64-bit compiler — prefer clang if available
CXX ?= $(shell command -v clang++ 2>/dev/null || echo g++)
CC  ?= $(shell command -v clang 2>/dev/null || echo gcc)

# Use ccache if available (speeds up incremental rebuilds)
CCACHE := $(shell command -v ccache 2>/dev/null)
ifdef CCACHE
  OBJCACHE = ccache
endif

CFLAGS   = -O2 -fwrapv -w -I$(STUBS_DIR) -I$(EBOX_INC)
CXXFLAGS = -O2 -std=c++17 -I$(OBJ_DIR) -I$(VERILATOR_ROOT)/include -I$(VERILATOR_ROOT)/include/vltstd \
           -I$(SIM_DIR)/harness -I. -DVL_TIME_CONTEXT

# SpinalHDL Verilog source
VERILOG = $(SIM_DIR)/rtl/CoreSim.v
SCALA_SRCS := $(shell find $(PROJ_ROOT)/src -name '*.scala') $(PROJ_ROOT)/project.scala

# Stamps
VERILATE_STAMP = $(OBJ_DIR)/.verilate.stamp

###############################################################################
# Targets
###############################################################################

all: $(OBJ_DIR)/trace_test

# Step 1: Generate Verilog (reuse existing or rebuild)
$(VERILOG): $(SCALA_SRCS)
	@rm -f $(dir $(VERILOG))*.bin
	cd $(PROJ_ROOT) && java -Xmx2g -cp "$$(scala-cli compile . --print-class-path 2>/dev/null)" voodoo.CoreSimGen
	@sed -i 's|\$$readmemb("\([^/"]*\.bin\)"|\$$readmemb("$(abspath $(dir $(VERILOG)))/\1"|g' $@

# Step 2: Verilate (64-bit obj_dir, separate from 32-bit Glide build)
$(VERILATE_STAMP): $(VERILOG)
	@mkdir -p $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) $(VERILOG) --Mdir $(OBJ_DIR)
	@touch $@

# Step 3: Build Verilator model library (64-bit)
$(OBJ_DIR)/VCoreSim__ALL.a: $(VERILATE_STAMP)
	$(MAKE) -j$(shell nproc) -C $(OBJ_DIR) -f VCoreSim.mk \
		CXX="$(CXX)" \
		OBJCACHE="$(OBJCACHE)" \
		OPT_FAST="-O3" \
		OPT_SLOW=""

# Step 4: 86Box reference objects (compiled as C)
$(OBJ_DIR)/vid_voodoo_render.o: $(EBOX_SRC)/vid_voodoo_render.c $(VERILATE_STAMP)
	$(OBJCACHE) $(CC) $(CFLAGS) -Dvoodoo_queue_triangle=_voodoo_queue_triangle_86box \
		-c -o $@ $<

$(OBJ_DIR)/vid_voodoo_texture.o: $(EBOX_SRC)/vid_voodoo_texture.c $(VERILATE_STAMP)
	$(OBJCACHE) $(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/vid_voodoo_reg.o: $(EBOX_SRC)/vid_voodoo_reg.c $(VERILATE_STAMP)
	$(OBJCACHE) $(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/vid_voodoo_fb.o: $(EBOX_SRC)/vid_voodoo_fb.c $(VERILATE_STAMP)
	$(OBJCACHE) $(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/vid_voodoo_blitter.o: $(EBOX_SRC)/vid_voodoo_blitter.c $(VERILATE_STAMP)
	$(OBJCACHE) $(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/ref_model.o: ref_model.c ref_model.h $(VERILATE_STAMP)
	$(OBJCACHE) $(CC) $(CFLAGS) -c -o $@ $<

# Step 5: C++ objects
$(OBJ_DIR)/sim_harness.o: $(SIM_DIR)/harness/sim_harness.cpp $(SIM_DIR)/harness/sim_harness.h Makefile | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/verilated.o: $(VERILATOR_ROOT)/include/verilated.cpp Makefile | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX) $(CXXFLAGS) -DVL_USER_WARN -c -o $@ $<

$(OBJ_DIR)/verilated_threads.o: $(VERILATOR_ROOT)/include/verilated_threads.cpp Makefile | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/trace_test.o: trace_test.cpp ref_model.h $(SIM_DIR)/harness/sim_harness.h stb_image_write.h | $(VERILATE_STAMP)
	$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CFLAGS) -c -o $@ $<

# Step 6: Link
OBJS = $(OBJ_DIR)/trace_test.o \
       $(OBJ_DIR)/ref_model.o \
       $(OBJ_DIR)/vid_voodoo_render.o \
       $(OBJ_DIR)/vid_voodoo_texture.o \
       $(OBJ_DIR)/vid_voodoo_reg.o \
       $(OBJ_DIR)/vid_voodoo_fb.o \
       $(OBJ_DIR)/vid_voodoo_blitter.o \
       $(OBJ_DIR)/sim_harness.o \
       $(OBJ_DIR)/verilated.o \
       $(OBJ_DIR)/verilated_threads.o

$(OBJ_DIR)/trace_test: $(OBJS) $(OBJ_DIR)/VCoreSim__ALL.a
	$(CXX) -o $@ $(OBJS) $(OBJ_DIR)/VCoreSim__ALL.a -lm -lz -lpthread

clean:
	rm -rf $(OBJ_DIR)
