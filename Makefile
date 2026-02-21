# Top-level Makefile for SpinalVoodoo simulation
#
# Builds everything in dependency order:
#   1. sim    - SpinalHDL → Verilog → Verilator model
#   2. glide  - Glide2x library linked with Verilator model
#   3. tests  - Test programs linked against Glide2x
#
# Usage:
#   make              # build everything
#   make test00       # build everything + run test00
#   make sim          # just the Verilator model
#   make glide        # sim + Glide library
#   make tests        # sim + Glide + all test binaries
#   make clean        # clean all build artifacts

.PHONY: all sim glide tests clean clean-sim clean-glide clean-tests test00 run-all

# Derive CXX32 from CC32 for sub-makefiles that need it
CC32 ?= gcc -m32
export CXX32 ?= $(shell d=$$(dirname "$(firstword $(CC32))") && [ -x "$$d/g++" ] && echo "$$d/g++" || echo "g++ -m32")
export CC32

SIM_DIR       = emu/sim
GLIDE_SRC_DIR = emu/glide/glide2x/sst1/glide/src
GLIDE_TST_DIR = emu/glide/glide2x/sst1/glide/tests

# Sentinel files to track what's been built
SIM_STAMP   = $(SIM_DIR)/obj_dir/VCoreSim__ALL.a
GLIDE_STAMP = emu/glide/glide2x/sst1/lib/sst1/libglide2x.so

OUTPUT_DIR = output

all: tests

sim:
	$(MAKE) -C $(SIM_DIR) all

glide: sim
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim

tests: glide
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim all

# Run a single test: make run/test00
run/%: tests scripts/srle2png
	@mkdir -p $(OUTPUT_DIR)/$*
	cd $(GLIDE_TST_DIR) && \
	  SIM_FST=$(abspath $(OUTPUT_DIR))/$*/trace.fst \
	  LD_LIBRARY_PATH=../../lib/sst1 \
	  timeout 300 ./$*.exe -n 1 -d $(abspath $(OUTPUT_DIR))/$*/screenshot.srle
	python3 scripts/srle2png $(OUTPUT_DIR)/$*/screenshot.srle $(OUTPUT_DIR)/$*/screenshot.png
	@rm -f $(OUTPUT_DIR)/$*/screenshot.srle
	@ln -sfn ../emu/sim/rtl/CoreSim.v $(OUTPUT_DIR)/CoreSim.v

# Run all tests that support -d
run-all: $(patsubst %,run/%,test00 test01 test02 test03 test04 test05 test06 test07 test08 test13 test16 test17 test18 test19)


clean: clean-tests clean-glide clean-sim

clean-sim:
	$(MAKE) -C $(SIM_DIR) clean

clean-glide:
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim clean

clean-tests:
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim clean
