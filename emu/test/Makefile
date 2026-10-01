.PHONY: all clean state-roundtrip
.DEFAULT_GOAL := all

PROJ_ROOT = ../..
SIM_DIR = ../sim
OBJ_DIR = obj_dir
STUBS_DIR = 86box_stubs
EBOX_SRC = ../86Box/src/video
EBOX_INC = ../86Box/src/include
TRACE_DIR = ../trace

CXX ?= g++
CC ?= gcc
OBJCACHE := $(shell command -v ccache 2>/dev/null)
CFLAGS = -O2 -fwrapv -w -I$(STUBS_DIR) -I$(EBOX_INC) -I$(TRACE_DIR)
CXXFLAGS = -O2 -std=c++17 -I$(SIM_DIR)/harness -I$(TRACE_DIR) -I.
DEPFLAGS = -MMD -MP

all: $(OBJ_DIR)/trace_test
state-roundtrip: $(OBJ_DIR)/state_roundtrip_test

$(OBJ_DIR):
	mkdir -p $@

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

$(OBJ_DIR)/voodoo_trace_writer.o: $(TRACE_DIR)/voodoo_trace_writer.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/ref_model.o: ref_model.c
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/sim_harness_xls.o: $(SIM_DIR)/harness/sim_harness_xls.c $(SIM_DIR)/harness/sim_harness.h $(PROJ_ROOT)/nix/xls-bridge-protocol.h Makefile
	$(OBJCACHE) $(CC) $(CFLAGS) $(DEPFLAGS) -I$(PROJ_ROOT)/nix -c -o $@ $<

$(OBJ_DIR)/trace_test.o: trace_test.cpp Makefile
	$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/state_roundtrip_test.o: state_roundtrip_test.cpp Makefile
	$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

ROUNDTRIP_OBJS = $(OBJ_DIR)/state_roundtrip_test.o \
                 $(OBJ_DIR)/ref_model.o \
                 $(OBJ_DIR)/vid_voodoo_render.o \
                 $(OBJ_DIR)/vid_voodoo_texture.o \
                 $(OBJ_DIR)/vid_voodoo_reg.o \
                 $(OBJ_DIR)/vid_voodoo_fb.o \
                 $(OBJ_DIR)/vid_voodoo_blitter.o \
                 $(OBJ_DIR)/voodoo_trace_writer.o

$(OBJ_DIR)/state_roundtrip_test: $(ROUNDTRIP_OBJS)
	$(CXX) -no-pie -o $@ $(ROUNDTRIP_OBJS) -lm -lpthread

OBJS = $(filter-out $(OBJ_DIR)/state_roundtrip_test.o,$(ROUNDTRIP_OBJS)) $(OBJ_DIR)/trace_test.o $(OBJ_DIR)/sim_harness_xls.o

$(sort $(OBJS) $(ROUNDTRIP_OBJS)): | $(OBJ_DIR)

$(OBJ_DIR)/trace_test: $(OBJS) Makefile
	$(CXX) -no-pie -o $@ $(OBJS) -lm -lz -lpthread

clean:
	rm -rf $(OBJ_DIR)

-include $(wildcard $(OBJ_DIR)/*.d)
