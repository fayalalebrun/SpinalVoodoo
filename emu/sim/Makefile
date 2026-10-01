.PHONY: all trace-harness xls-harness clean
.DEFAULT_GOAL := all

PROJ_ROOT = ../..
OBJ_DIR = obj_dir
HARNESS_DIR = harness
TRACE_DIR = ../trace

CC32 ?= gcc -m32
CXX32 ?= $(shell d=$$(dirname "$(firstword $(CC32))") && [ -x "$$d/g++" ] && echo "$$d/g++" || echo "g++ -m32")

# Use ccache if available (speeds up incremental rebuilds)
CCACHE := $(shell command -v ccache 2>/dev/null)
ifdef CCACHE
  OBJCACHE = ccache
endif

CXXFLAGS = -O3 -I$(TRACE_DIR)
TRACE_IO_FLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

all: xls-harness

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/sim_harness_xls.o $(OBJ_DIR)/sim_trace_harness.o $(OBJ_DIR)/voodoo_trace_writer.o: | $(OBJ_DIR)

trace-harness: $(OBJ_DIR)/sim_trace_harness.o $(OBJ_DIR)/voodoo_trace_writer.o

xls-harness: $(OBJ_DIR)/sim_harness_xls.o

$(OBJ_DIR)/sim_harness_xls.o: $(HARNESS_DIR)/sim_harness_xls.c $(HARNESS_DIR)/sim_harness.h $(PROJ_ROOT)/nix/xls-bridge-protocol.h
	$(OBJCACHE) $(CC32) -O2 -g -mstackrealign -I$(HARNESS_DIR) -I$(PROJ_ROOT)/nix -c -o $@ $<

$(OBJ_DIR)/voodoo_trace_writer.o: $(TRACE_DIR)/voodoo_trace_writer.c $(TRACE_DIR)/voodoo_trace_writer.h $(TRACE_DIR)/voodoo_trace_format.h
	$(OBJCACHE) $(CXX32) $(CXXFLAGS) $(TRACE_IO_FLAGS) -I$(TRACE_DIR) -c -o $@ $<

$(OBJ_DIR)/sim_trace_harness.o: $(HARNESS_DIR)/sim_trace_harness.c $(HARNESS_DIR)/sim_harness.h $(TRACE_DIR)/voodoo_trace_writer.h $(TRACE_DIR)/voodoo_trace_format.h
	$(OBJCACHE) $(CXX32) $(CXXFLAGS) $(TRACE_IO_FLAGS) -I$(TRACE_DIR) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR)
