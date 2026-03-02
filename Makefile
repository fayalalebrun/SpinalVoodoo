# Top-level Makefile for SpinalVoodoo simulation
#
# Builds everything in dependency order:
#   1. sim    - SpinalHDL → Verilog → Verilator model
#   2. glide  - Glide2x library linked with Verilator model
#   3. tests  - Test programs linked against Glide2x
#
# Usage:
#   make              # build everything
#   make run/test00   # build only what's needed + run test00
#   make sim          # just the Verilator model
#   make glide        # sim + Glide library
#   make tests        # sim + Glide + all test binaries
#   make clean        # clean all build artifacts

.PHONY: all sim glide tests clean clean-sim clean-glide clean-tests run-all trace-test check-all FORCE

# Derive CXX32 from CC32 for sub-makefiles that need it
CC32 ?= gcc -m32
export CXX32 ?= $(shell d=$$(dirname "$(firstword $(CC32))") && [ -x "$$d/g++" ] && echo "$$d/g++" || echo "g++ -m32")
export CC32
export TRACE

SIM_DIR       = emu/sim
GLIDE_SRC_DIR = emu/glide/glide2x/sst1/glide/src
GLIDE_TST_DIR = emu/glide/glide2x/sst1/glide/tests

# SpinalHDL sources — trigger sim rebuild when RTL changes
SCALA_SRCS := $(shell find src -name '*.scala') project.scala

# Stamp files — real build outputs used to avoid unnecessary sub-make invocations
SIM_STAMP   = $(SIM_DIR)/obj_dir/VCoreSim__ALL.a
GLIDE_STAMP = emu/glide/glide2x/sst1/lib/sst1/libglide2x.so.2

OUTPUT_DIR = output

all: tests

# Phony targets: always recurse into sub-make (for explicit builds)
sim:
	$(MAKE) -C $(SIM_DIR) all

glide: sim
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim

tests: glide
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim all

# Trace mode sentinel: force sim+glide rebuild when TRACE= setting changes.
# Updated at parse time via $(shell) so the timestamp only changes on real transitions.
TRACE_MODE_FILE = $(SIM_DIR)/obj_dir/.trace_mode
TRACE_MODE_VAL  = $(if $(filter 1,$(TRACE)),trace,notrace)
$(shell mkdir -p $(dir $(TRACE_MODE_FILE)))
$(shell [ -f $(TRACE_MODE_FILE) ] && [ "$$(cat $(TRACE_MODE_FILE))" = "$(TRACE_MODE_VAL)" ] || echo "$(TRACE_MODE_VAL)" > $(TRACE_MODE_FILE))

# File-based build chain: only recurse when outputs are stale.
# The sub-makes handle fine-grained .c/.o dependency tracking internally.
$(SIM_STAMP): $(SCALA_SRCS) $(TRACE_MODE_FILE)
	$(MAKE) -C $(SIM_DIR) all

# Always recurse so sub-make tracks .c → .o deps for Glide sources
$(GLIDE_STAMP): $(SIM_STAMP) FORCE
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim

# Build a single test exe (always recurse so sub-make tracks .c → .o deps)
$(GLIDE_TST_DIR)/%.exe: $(GLIDE_STAMP) FORCE
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim $*.exe

# Run a single test: make run/test00
# Pass TRACE=1 to enable FST trace output
run/%: $(GLIDE_TST_DIR)/%.exe scripts/srle2png
	@rm -rf $(OUTPUT_DIR)/$*
	@mkdir -p $(OUTPUT_DIR)/$*
	@ln -sfn ../../emu/sim/rtl/CoreSim.v $(OUTPUT_DIR)/$*/CoreSim.v
	cd $(GLIDE_TST_DIR) && \
	  $(if $(filter 1,$(TRACE)),SIM_FST=$(abspath $(OUTPUT_DIR))/$*/trace.fst) \
	  LD_LIBRARY_PATH=../../lib/sst1 \
	  ./$*.exe -n 1 -d $(abspath $(OUTPUT_DIR))/$*/screenshot.srle < /dev/null
	@for f in $(OUTPUT_DIR)/$*/screenshot*.srle; do \
	  [ -f "$$f" ] || continue; \
	  python3 scripts/srle2png "$$f" "$${f%.srle}.png" && rm -f "$$f"; \
	done

# Run all tests that support -d
run-all: $(patsubst %,run/%,test00 test01 test02 test03 test04 test05 test06 test07 test08 test09 test13 test16 test17 test18 test19)


# --------------------------------------------------------------------------
# Trace-based testing (C++ trace_test binary)
# --------------------------------------------------------------------------

TRACE_TEST_BIN = emu/test/obj_dir/trace_test

# Build trace_test binary (64-bit, independent of 32-bit Glide build).
# Always recurse — sub-make tracks Scala → Verilog → Verilator → binary deps.
$(TRACE_TEST_BIN): FORCE
	$(MAKE) -C emu/test

trace-test: $(TRACE_TEST_BIN)

# Capture trace from Glide test.
# TRACE_CAPTURE=1 swaps the Verilator backend for a lightweight trace writer.
# This overwrites the normal Glide library; next run/% will rebuild it via FORCE.
trace/%:
	$(MAKE) -C $(SIM_DIR) trace-harness
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim TRACE_CAPTURE=1
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim $*.exe TRACE_CAPTURE=1
	@mkdir -p traces
	cd $(GLIDE_TST_DIR) && \
	  SIM_TRACE_FILE=$(abspath traces)/$*.bin \
	  LD_LIBRARY_PATH=../../lib/sst1 \
	  ./$*.exe -n 1 < /dev/null

# Replay a .bin trace through both 86Box ref model and Verilator CoreSim.
# Produces _ref.png, _sim.png, _diff.png in test-output/<name>/.
check/%: $(TRACE_TEST_BIN)
	@mkdir -p test-output/$*
	@if [ -f traces/$*.bin ]; then \
	  src=traces/$*.bin; \
	elif [ -d traces/$* ]; then \
	  src=traces/$*/; \
	else \
	  echo "ERROR: neither traces/$*.bin nor traces/$*/ found. Run 'make trace/$*' first."; exit 1; \
	fi; \
	$(if $(filter 1,$(TRACE)),SIM_FST=$(abspath test-output)/$*/trace.fst) \
	$(TRACE_TEST_BIN) "$$src" --output-dir test-output/$*

# Capture + check in one step (sequential: trace must finish before check starts)
test/%:
	$(MAKE) trace/$*
	$(MAKE) check/$*

# Replay all existing traces
check-all: $(TRACE_TEST_BIN)
	@for f in traces/*.bin; do \
	    [ -f "$$f" ] || continue; \
	    name=$$(basename $$f .bin); \
	    echo "=== $$name ==="; \
	    mkdir -p test-output/$$name; \
	    $(TRACE_TEST_BIN) $$f --output-dir test-output/$$name || exit 1; \
	done
	@for d in traces/*/trace.bin; do \
	    [ -f "$$d" ] || continue; \
	    name=$$(basename $$(dirname $$d)); \
	    echo "=== $$name ==="; \
	    mkdir -p test-output/$$name; \
	    $(TRACE_TEST_BIN) traces/$$name/ --output-dir test-output/$$name || exit 1; \
	done

clean-trace-test:
	$(MAKE) -C emu/test clean

# --------------------------------------------------------------------------

clean: clean-tests clean-glide clean-sim

clean-sim:
	$(MAKE) -C $(SIM_DIR) clean

clean-glide:
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim clean

clean-tests:
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim clean
