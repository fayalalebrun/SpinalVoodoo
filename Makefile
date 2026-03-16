# Top-level Makefile for SpinalVoodoo simulation + DE10 scaffolding
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
#   make dos/help     # DOS guest workflow entrypoints
#   make de10/help    # DE10 workflow entrypoints
#   make clean        # clean all build artifacts

.PHONY: all sim glide tests clean clean-sim clean-glide clean-tests run-all trace-test check-all dos/help dos/sdk dosbox tomb/help tomb/prepare tomb/run tomb/headless tomb/screenshot de10/help de10/plan de10/rtl de10/qsys de10/bitstream de10/program de10/deploy de10/mmio-smoke de10/sync-sysroot de10/glide de10/glide-tests de10/glide-cross de10/glide-tests-cross FORCE

# Derive CXX32 from CC32 for sub-makefiles that need it
CC32 ?= gcc -m32
export CXX32 ?= $(shell d=$$(dirname "$(firstword $(CC32))") && [ -x "$$d/g++" ] && echo "$$d/g++" || echo "g++ -m32")
export CC32
export TRACE
export REF_TRACE
SIM_INTERFACE ?= core
SIM_MODEL = $(if $(filter de10,$(SIM_INTERFACE)),CoreDe10,CoreSim)

SIM_DIR       = emu/sim
GLIDE_SRC_DIR = emu/glide/glide2x/sst1/glide/src
GLIDE_TST_DIR = emu/glide/glide2x/sst1/glide/tests
DOS_SDK_MAKEFILE = Makefile.sdkwat
DOS_TEST_MOUNT_DIR = $(GLIDE_TST_DIR)

# SpinalHDL sources — trigger sim rebuild when RTL changes
SCALA_SRCS := $(shell find src -name '*.scala') project.scala

# Stamp files — real build outputs used to avoid unnecessary sub-make invocations
SIM_STAMP   = $(SIM_DIR)/obj_dir/V$(SIM_MODEL)__ALL.a
SIM_RTL     = $(SIM_DIR)/rtl/$(SIM_MODEL).v
GLIDE_STAMP = emu/glide/glide2x/sst1/lib/sst1/libglide2x.so.2

OUTPUT_DIR = output
DE10_RTL_DIR = hw/de10/rtl
DE10_BUILD_SCRIPT = scripts/build-bitstream-de10
DE10_RTL_SCRIPT = scripts/gen-rtl-de10
DE10_QSYS_SCRIPT = scripts/gen-qsys-de10
DE10_PROGRAM_SCRIPT = scripts/program-de10
DE10_DEPLOY_SCRIPT = scripts/deploy-de10.sh
DE10_SYNC_SYSROOT_SCRIPT = scripts/sync-de10-sysroot
DE10_CROSS_GLIDE_SCRIPT = scripts/build-de10-glide-cross
DE10_MMIO_SMOKE_BIN = tools/de10-mmio-smoke

all: tests

dos/help:
	@echo "DOS targets:"
	@echo "  make dos/sdk                  # build all DOS SDK-style test binaries"
	@echo "  make dos/sdk/df00sdk         # build one DOS SDK-style test binary"
	@echo "  make dos/run/df00sdk         # run one DOS SDK-style test in DOSBox-X"
	@echo "  make dos/run/df00sdk-headless # run one DOS SDK-style test headlessly"
	@echo "  make dosbox ARGS='...'       # launch DOSBox-X with the repo Glide backend"
	@echo "  make tomb/help               # print Tomb Raider setup requirements"
	@echo "  make tomb/prepare ARGS='...' # prepare a Tomb Raider source tree from assets"
	@echo "  make tomb/run                # run Tomb Raider with a window"
	@echo "  make tomb/headless           # run Tomb Raider headlessly"
	@echo "  make tomb/screenshot         # run Tomb Raider and capture a screenshot"

dos/sdk:
	$(MAKE) -C $(GLIDE_TST_DIR) -f $(DOS_SDK_MAKEFILE) all

dos/sdk/%:
	$(MAKE) -C $(GLIDE_TST_DIR) -f $(DOS_SDK_MAKEFILE) $*.exe

dosbox: glide
	bash ./scripts/run-dosboxx32-glide $(ARGS)

dos/run/%: dos/sdk/% glide
	bash ./scripts/run-dosboxx32-glide \
	  -c "mount c $(DOS_TEST_MOUNT_DIR)" \
	  -c "c:" \
	  -c "$*.exe" \
	  $(ARGS)

dos/run/%-headless: dos/sdk/% glide
	DOSBOXX32_HEADLESS=1 bash ./scripts/run-dosboxx32-glide \
	  -c "mount c $(DOS_TEST_MOUNT_DIR)" \
	  -c "c:" \
	  -c "$*.exe" \
	  $(ARGS)

tomb/run: glide
	bash ./scripts/run-tomb-glide-live $(ARGS)

tomb/help:
	@echo "Tomb Raider setup:"
	@echo "  1) Prepare a source tree at DOSBOX_TOMB_SRC (default: /tmp/tr1-3dfx)"
	@echo "  2) Easiest path: make tomb/prepare ARGS='--game-dir /path/to/TOMBRAID --patch /path/to/3dfx-patch.zip --iso /path/to/tr1disc01.iso'"
	@echo "  3) The prepared tree must contain \$$DOSBOX_TOMB_GAME_DIR and usually \$$DOSBOX_TOMB_ISO"
	@echo "  4) The runner copies that tree to DOSBOX_TOMB_STAGE_ROOT and disables bundled glide2x.ovl"
	@echo "  5) Override paths with DOSBOX_TOMB_SRC, DOSBOX_TOMB_STAGE_ROOT, DOSBOX_TOMB_GAME_DIR, DOSBOX_TOMB_ISO, DOSBOX_TOMB_EXE"
	@echo "  6) Pass --output PATH or --force through make tomb/prepare ARGS='...' when needed"

tomb/prepare:
	bash ./scripts/prepare-tomb-glide-tree $(ARGS)

tomb/headless: glide
	bash ./scripts/run-tomb-glide-headless $(ARGS)

tomb/screenshot: glide
	bash ./scripts/capture-tomb-screenshot $(ARGS)

de10/help:
	@echo "DE10 targets:"
	@echo "  make de10/plan         # open goals and bring-up docs"
	@echo "  make de10/rtl          # generate DE10-targeted RTL"
	@echo "  make de10/qsys         # generate Platform Designer system"
	@echo "  make de10/bitstream    # run Quartus bitstream build flow"
	@echo "  make de10/program      # program DE10 FPGA image remotely"
	@echo "  make de10/deploy       # deploy runtime bundle to DE10"
	@echo "  make de10/mmio-smoke   # build board MMIO smoke utility"
	@echo "  make de10/sync-sysroot # fetch DE10 userspace sysroot locally"
	@echo "  make de10/glide        # build DE10-targeted Glide library"
	@echo "  make de10/glide-tests  # build DE10-targeted Glide test binaries"
	@echo "  make de10/glide-cross  # cross-build DE10 Glide library locally"
	@echo "  make de10/glide-tests-cross # cross-build DE10 Glide tests locally"

de10/plan:
	@echo "See docs/DE10_MILESTONES.md"
	@echo "See docs/DE10_BRINGUP_PLAN.md"
	@echo "See docs/DE10_DEPLOYMENT.md"

de10/rtl:
	@mkdir -p $(DE10_RTL_DIR)
	$(DE10_RTL_SCRIPT)

de10/qsys:
	$(DE10_QSYS_SCRIPT)

de10/bitstream:
	$(DE10_BUILD_SCRIPT)

de10/program:
	$(DE10_PROGRAM_SCRIPT) $(ARGS)

de10/deploy:
	$(DE10_DEPLOY_SCRIPT) $(ARGS)

de10/mmio-smoke: $(DE10_MMIO_SMOKE_BIN)

de10/sync-sysroot:
	$(DE10_SYNC_SYSROOT_SCRIPT) $(ARGS)

de10/glide:
	$(MAKE) -C emu/glide/glide2x/sst1/glide/src -f Makefile.de10 $(ARGS)

de10/glide-tests:
	$(MAKE) -C emu/glide/glide2x/sst1/glide/src -f Makefile.de10
	$(MAKE) -C emu/glide/glide2x/sst1/glide/tests -f Makefile.de10 $(ARGS)

de10/glide-cross:
	$(DE10_CROSS_GLIDE_SCRIPT) --glide $(ARGS)

de10/glide-tests-cross:
	$(DE10_CROSS_GLIDE_SCRIPT) --glide-tests $(ARGS)

$(DE10_MMIO_SMOKE_BIN): tools/de10-mmio-smoke.c
	$(CC) -O2 -Wall -Wextra -o $@ $<

# Phony targets: always recurse into sub-make (for explicit builds)
sim:
	$(MAKE) -C $(SIM_DIR) all SIM_INTERFACE=$(SIM_INTERFACE)

glide: sim
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim SIM_INTERFACE=$(SIM_INTERFACE)

tests: glide
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim all SIM_INTERFACE=$(SIM_INTERFACE)

# Trace mode sentinel: force sim+glide rebuild when TRACE= setting changes.
# Updated at parse time via $(shell) so the timestamp only changes on real transitions.
TRACE_MODE_FILE = $(SIM_DIR)/obj_dir/.trace_mode
TRACE_MODE_VAL  = $(if $(filter 1,$(TRACE)),trace,notrace)
$(shell mkdir -p $(dir $(TRACE_MODE_FILE)))
$(shell [ -f $(TRACE_MODE_FILE) ] && [ "$$(cat $(TRACE_MODE_FILE))" = "$(TRACE_MODE_VAL)" ] || echo "$(TRACE_MODE_VAL)" > $(TRACE_MODE_FILE))

# File-based build chain: only recurse when outputs are stale.
# The sub-makes handle fine-grained .c/.o dependency tracking internally.
$(SIM_STAMP): $(SCALA_SRCS) $(TRACE_MODE_FILE)
	$(MAKE) -C $(SIM_DIR) all SIM_INTERFACE=$(SIM_INTERFACE)

# Always recurse so sub-make tracks .c → .o deps for Glide sources
$(GLIDE_STAMP): $(SIM_STAMP) FORCE
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim SIM_INTERFACE=$(SIM_INTERFACE)

# Build a single test exe (always recurse so sub-make tracks .c → .o deps)
$(GLIDE_TST_DIR)/%.exe: $(GLIDE_STAMP) FORCE
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim $*.exe SIM_INTERFACE=$(SIM_INTERFACE)

# Run a single test: make run/test00
# Pass TRACE=1 to enable FST trace output
run/%: $(GLIDE_TST_DIR)/%.exe scripts/srle2png
	@rm -rf $(OUTPUT_DIR)/$*
	@mkdir -p $(OUTPUT_DIR)/$*
	@ln -sfn ../../$(SIM_RTL) $(OUTPUT_DIR)/$*/$(SIM_MODEL).v
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
STATE_ROUNDTRIP_BIN = emu/test/obj_dir/state_roundtrip_test

# Build trace_test binary (64-bit, independent of 32-bit Glide build).
# Always recurse — sub-make tracks Scala → Verilog → Verilator → binary deps.
$(TRACE_TEST_BIN): FORCE
	$(MAKE) -C emu/test

trace-test: $(TRACE_TEST_BIN)

$(STATE_ROUNDTRIP_BIN): FORCE
	$(MAKE) -C emu/test state-roundtrip

check/state-roundtrip: $(STATE_ROUNDTRIP_BIN)
	$(STATE_ROUNDTRIP_BIN)

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
	extra_args=""; \
	if [ "$*" = "screamer2" ]; then \
	  extra_args="--max-mismatches 500"; \
	fi; \
	if [ "$(REF_TRACE)" = "1" ]; then \
	  extra_args="$$extra_args --ref-trace-jsonl test-output/$*/ref_trace.jsonl"; \
	fi; \
	$(if $(filter 1,$(TRACE)),SIM_FST=$(abspath test-output)/$*/trace.fst) \
	$(TRACE_TEST_BIN) "$$src" --output-dir test-output/$* $$extra_args

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
	    extra_args=""; \
	    if [ "$(REF_TRACE)" = "1" ]; then extra_args="--ref-trace-jsonl test-output/$$name/ref_trace.jsonl"; fi; \
	    $(TRACE_TEST_BIN) $$f --output-dir test-output/$$name $$extra_args || exit 1; \
	done
	@for d in traces/*/trace.bin; do \
	    [ -f "$$d" ] || continue; \
	    name=$$(basename $$(dirname $$d)); \
	    echo "=== $$name ==="; \
	    mkdir -p test-output/$$name; \
	    extra_args=""; \
	    if [ "$(REF_TRACE)" = "1" ]; then extra_args="--ref-trace-jsonl test-output/$$name/ref_trace.jsonl"; fi; \
	    $(TRACE_TEST_BIN) traces/$$name/ --output-dir test-output/$$name $$extra_args || exit 1; \
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
