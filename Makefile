# Top-level Makefile for SpinalVoodoo simulation + DE10 scaffolding
#
# Canonical command taxonomy:
#   native/<runtime>/<action>[/<name>]
#   dos/<runtime>/<action>[/<name>]
#   tomb/<runtime>/<action>
#   de10/<action>

.PHONY: all clean clean-sim clean-glide clean-tests xls native/help native/trace/build dos/help dos/trace/build dos/xls/build dos/xls/check dos/xls/check-headless dos/xls/dosbox tomb/help tomb/prepare tomb/xls/run tomb/xls/headless tomb/trace/run tomb/trace/headless de10/help de10/setup/program de10/setup/deploy de10/check/mmio de10/check/ddr-stress de10/report/ddr de10/run/tomb de10/run/tomb/vnc de10/trace/build de10/trace/run de10/plan de10/rtl de10/qsys de10/bitstream de10/xls/rtl de10/xls/check de10/xls/replay de10/xls/bitstream de10/xls/setup/program de10/ddrbench/rtl de10/ddrbench/bitstream de10/sync-sysroot de10/glide de10/glide-tests de10/glide-cross de10/glide-tests-cross FORCE
.PHONY: native/xls-de10sim/compare-all
.PRECIOUS: dos/trace/build/% dos/xls/build/%

# Derive CXX32 from CC32 for sub-makefiles that need it
CC32 ?= gcc -m32
export CXX32 ?= $(shell d=$$(dirname "$(firstword $(CC32))") && [ -x "$$d/g++" ] && echo "$$d/g++" || echo "g++ -m32")
export CC32
export TRACE
export REF_TRACE

SIM_DIR       = software/simulation/xls
GLIDE_SRC_DIR = vendor/glide/glide2x/sst1/glide/src
GLIDE_TST_DIR = vendor/glide/glide2x/sst1/glide/tests
DOS_SDK_MAKEFILE = Makefile.sdkwat
DOS_TEST_MOUNT_DIR = $(GLIDE_TST_DIR)

# SpinalHDL sources — trigger sim rebuild when RTL changes
SCALA_SRCS := $(shell find hardware -name '*.scala') project.scala
XLS_STAMP = build/generated/xls/rtl/.stamp

xls: $(XLS_STAMP)

$(XLS_STAMP): $(wildcard hardware/renderer/dslx/*.x) workflows/build/generate-xls-rtl flake.nix flake.lock
	$(MAKE) -C hardware/renderer all

# Stamp files — real build outputs used to avoid unnecessary sub-make invocations

OUTPUT_DIR = output
DE10_RTL_DIR = build/generated/scala/de10
DE10_BUILD_SCRIPT = workflows/build/build-bitstream-de10
DE10_RTL_SCRIPT = workflows/build/gen-rtl-de10
DE10_QSYS_SCRIPT = workflows/build/gen-qsys-de10
DE10_PROGRAM_SCRIPT = workflows/board/program-de10
DE10_DEPLOY_SCRIPT = workflows/board/deploy-de10.sh
DE10_SYNC_SYSROOT_SCRIPT = workflows/board/sync-de10-sysroot
DE10_CROSS_GLIDE_SCRIPT = workflows/build/build-de10-glide-cross
DE10_MMIO_SMOKE_BIN = build/host/board/de10-mmio-smoke
DE10_DDR_STRESS_BIN = build/host/board/de10-ddr-stress
DE10_TRACE_REPLAY_BIN = output/de10/bin/de10-trace-replay
DE10_DDR_RBF = output/de10/bitstream/SpinalVoodoo_de10.rbf
DE10_XLS_RBF = output/de10/xls-bitstream/SpinalVoodoo_de10_xls.rbf
DE10_HOST ?= debian-fpga.local
DE10_USER ?= fpga
DE10_TRACE_BUILD_REMOTE_DIR ?= /home/$(DE10_USER)/trace-replay-build

all: xls

native/help:
	@echo "Native targets:"
	@echo "  runtimes: trace, xls"
	@echo "  make native/trace/build           # build the trace-capture Glide runtime"
	@echo "  make native/trace/run/test00      # capture a trace from one Linux-hosted test"
	@echo "  make native/xls-de10sim/run/screamer2 # pure-XLS replay with a 90% cache-rate gate"
	@echo "  make native/xls-de10sim/compare-all # 86Box/XLS screenshot gallery for every trace"

dos/help:
	@echo "DOS targets:"
	@echo "  runtimes: trace, xls"
	@echo "  make dos/trace/run/df00sdk        # run one DOS SDK-style test with trace runtime to verification/fixtures/traces/dos/df00sdk.bin"
	@echo "  make dos/trace/headless/df00sdk   # run one DOS SDK-style test headlessly with trace runtime"
	@echo "  make dos/xls/run/df00sdk          # run one DOS SDK-style test through pure XLS"
	@echo "  make dos/xls/headless/df00sdk     # run one DOS SDK-style test headlessly through pure XLS"
	@echo "  make dos/xls/dosbox ARGS='...'    # launch DOSBox-X with the pure-XLS Glide backend"
	@echo "  make dos/xls/check                 # smoke-test the pure-XLS bridge protocol"
	@echo "  make tomb/help                    # print Tomb Raider setup requirements"

native/trace/build: build/host/simulation/sim_trace_harness.o
	$(MAKE) _glide_build_trace

dos/trace/build: native/trace/build
	$(MAKE) -C $(GLIDE_TST_DIR) -f $(DOS_SDK_MAKEFILE) all

dos/xls/build:
	$(MAKE) _glide_build_xls
	$(MAKE) -C $(GLIDE_TST_DIR) -f $(DOS_SDK_MAKEFILE) all

dos/xls/check:
	workflows/check/test-pure-xls-bridge

dos/xls/dosbox:
	$(MAKE) _glide_build_xls
	bash ./workflows/run/run-dosboxx32-xls-glide $(ARGS)

tomb/help:
	@echo "Tomb Raider setup:"
	@echo "  1) Prepare a source tree at DOSBOX_TOMB_SRC (default: /tmp/tr1-3dfx)"
	@echo "  2) Easiest path: make tomb/prepare ARGS='--game-dir /path/to/TOMBRAID --patch /path/to/3dfx-patch.zip --iso /path/to/tr1disc01.iso'"
	@echo "  3) The prepared tree must contain \$$DOSBOX_TOMB_GAME_DIR and usually \$$DOSBOX_TOMB_ISO"
	@echo "  4) The runner copies that tree to DOSBOX_TOMB_STAGE_ROOT and disables bundled glide2x.ovl"
	@echo "  5) Override paths with DOSBOX_TOMB_SRC, DOSBOX_TOMB_STAGE_ROOT, DOSBOX_TOMB_GAME_DIR, DOSBOX_TOMB_ISO, DOSBOX_TOMB_EXE"
	@echo "  6) Pass --output PATH or --force through make tomb/prepare ARGS='...' when needed"

tomb/prepare:
	bash ./workflows/run/prepare-tomb-glide-tree $(ARGS)

tomb/xls/run:
	$(MAKE) _glide_build_xls
	bash ./workflows/run/run-tomb-xls-live $(ARGS)

tomb/xls/headless:
	$(MAKE) _glide_build_xls
	bash ./workflows/run/run-tomb-xls-headless $(ARGS)

tomb/trace/run: native/trace/build
	@mkdir -p verification/fixtures/traces/tomb
	SIM_TRACE_FILE=$(abspath verification/fixtures/traces)/tomb/trace.bin \
	bash ./workflows/run/run-tomb-glide-live $(ARGS)

tomb/trace/headless: native/trace/build
	@mkdir -p verification/fixtures/traces/tomb
	SIM_TRACE_FILE=$(abspath verification/fixtures/traces)/tomb/trace.bin \
	bash ./workflows/run/run-tomb-glide-headless $(ARGS)

de10/help:
	@echo "DE10 workflow targets:"
	@echo "  runtime: de10"
	@echo "  make de10/setup/program    # program the XLS FPGA image on the board"
	@echo "  make de10/xls/check        # generate and lint the pure-XLS DE10 RTL"
	@echo "  make de10/xls/replay       # run the CoreSimDe10 test28 RTL replay"
	@echo "  make de10/xls/bitstream    # build the pure-XLS DE10 FPGA image"
	@echo "  make de10/xls/setup/program # program the pure-XLS image on the board"
	@echo "  make de10/setup/deploy     # deploy the board runtime bundle"
	@echo "  make de10/check/mmio       # run the board MMIO smoke utility"
	@echo "  make de10/check/ddr-stress # build the DE10 DDR stress utility"
	@echo "  make de10/report/ddr       # program board and collect DDR statistics tables"
	@echo "  make de10/run/dos/df00sdk  # run one DOS workload from the board runtime dir"
	@echo "  make de10/run/tomb         # run Tomb from a prepared remote Tomb tree"
	@echo "  make de10/run/tomb/vnc     # run Tomb on the board under a VNC X server"
	@echo "  make de10/trace/build      # build the DE10 board trace replay tool"
	@echo "  make de10/trace/run/tomb   # replay verification/fixtures/traces/tomb on the board and dump a screenshot"
	@echo ""
	@echo "Advanced DE10 targets:"
	@echo "  make de10/plan         # open goals and bring-up docs"
	@echo "  make de10/rtl          # generate DE10-targeted RTL"
	@echo "  make de10/qsys         # generate Platform Designer system"
	@echo "  make de10/bitstream    # run Quartus bitstream build flow"
	@echo "  make de10/ddrbench/rtl # generate DDR bench RTL as De10Top.v"
	@echo "  make de10/ddrbench/bitstream # build the DDR bench bitstream"
	@echo "  make de10/sync-sysroot # fetch DE10 userspace sysroot locally"
	@echo "  make de10/glide        # build DE10-targeted Glide library"
	@echo "  make de10/glide-tests  # build DE10-targeted Glide test binaries"
	@echo "  make de10/glide-cross  # cross-build DE10 Glide library locally"
	@echo "  make de10/glide-tests-cross # cross-build DE10 Glide tests locally"

de10/setup/program: de10/xls/setup/program

de10/xls/setup/program: $(DE10_XLS_RBF)
	DE10_RBF=$(DE10_XLS_RBF) $(DE10_PROGRAM_SCRIPT) $(ARGS)

de10/setup/deploy:
	$(DE10_DEPLOY_SCRIPT) $(ARGS)

de10/check/mmio: $(DE10_MMIO_SMOKE_BIN)

de10/check/ddr-stress: $(DE10_DDR_STRESS_BIN)

de10/report/ddr: $(DE10_DDR_STRESS_BIN) $(DE10_DDR_RBF)
	bash ./workflows/board/report-de10-ddr

de10/run/dos/%:
	bash ./workflows/board/run-de10-dos-workload --exe $*.EXE $(ARGS)

de10/run/tomb:
	bash ./workflows/board/run-de10-tomb-workload $(ARGS)

de10/run/tomb/vnc:
	bash ./workflows/board/run-de10-tomb-vnc $(ARGS)

de10/trace/build: $(DE10_TRACE_REPLAY_BIN)

de10/trace/run: $(DE10_TRACE_REPLAY_BIN)
	bash ./workflows/board/run-de10-trace-replay $(ARGS)

de10/trace/run/%: $(DE10_TRACE_REPLAY_BIN)
	bash ./workflows/board/run-de10-trace-replay --input verification/fixtures/traces/$* --output-dir output/de10-board-trace/$* $(ARGS)

de10/plan:
	@echo "See ARCHITECTURE.md"
	@echo "See docs/pure-xls-coresim.md"
	@echo "See docs/MISTER.md"

de10/rtl: de10/xls/rtl

de10/qsys:
	$(DE10_QSYS_SCRIPT)

de10/bitstream: de10/xls/bitstream

de10/xls/rtl:
	./workflows/build/gen-rtl-de10-xls

de10/xls/check:
	./workflows/check/check-de10-xls-rtl

de10/xls/replay:
	./workflows/check/check-de10-xls-replay

de10/xls/bitstream:
	./workflows/build/build-bitstream-de10-xls $(ARGS)

$(DE10_XLS_RBF):
	$(MAKE) de10/xls/bitstream

de10/ddrbench/rtl:
	@mkdir -p $(DE10_RTL_DIR)
	DE10_RTL_MAIN_CLASS=voodoo.de10.De10DdrBenchTopGen $(DE10_RTL_SCRIPT)

de10/ddrbench/bitstream: $(DE10_DDR_RBF)

$(DE10_DDR_RBF): $(SCALA_SRCS) hardware/platforms/de10/quartus/De10PlatformTop.v hardware/platforms/de10/qsys/create_de10_soc.tcl
	DE10_RTL_MAIN_CLASS=voodoo.de10.De10DdrBenchTopGen $(DE10_BUILD_SCRIPT)

de10/sync-sysroot:
	$(DE10_SYNC_SYSROOT_SCRIPT) $(ARGS)

de10/glide:
	$(MAKE) -C vendor/glide/glide2x/sst1/glide/src -f Makefile.de10 $(ARGS)

de10/glide-tests:
	$(MAKE) -C vendor/glide/glide2x/sst1/glide/src -f Makefile.de10
	$(MAKE) -C vendor/glide/glide2x/sst1/glide/tests -f Makefile.de10 $(ARGS)

de10/glide-cross:
	$(DE10_CROSS_GLIDE_SCRIPT) --glide $(ARGS)

de10/glide-tests-cross:
	$(DE10_CROSS_GLIDE_SCRIPT) --glide-tests $(ARGS)

$(DE10_MMIO_SMOKE_BIN): diagnostics/board/de10-mmio-smoke.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Wall -Wextra -o $@ $<

$(DE10_DDR_STRESS_BIN): diagnostics/board/de10-ddr-stress.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Wall -Wextra -o $@ $<

$(DE10_TRACE_REPLAY_BIN): diagnostics/board/de10-trace-replay.cpp \
			software/trace/voodoo_trace_format.h \
			software/trace/trace_replay_backend.h \
			software/trace/trace_replay_image.h \
			software/trace/trace_replay_io.h \
			software/trace/trace_replay_runner.h \
			vendor/stb/stb_image_write.h
	@mkdir -p $(dir $@)
	ssh $(DE10_USER)@$(DE10_HOST) "mkdir -p '$(DE10_TRACE_BUILD_REMOTE_DIR)'"
	ssh $(DE10_USER)@$(DE10_HOST) "rm -f '$(DE10_TRACE_BUILD_REMOTE_DIR)/de10-trace-replay' '$(DE10_TRACE_BUILD_REMOTE_DIR)/de10-trace-replay.cpp' '$(DE10_TRACE_BUILD_REMOTE_DIR)/voodoo_trace_format.h' '$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_backend.h' '$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_image.h' '$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_io.h' '$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_runner.h' '$(DE10_TRACE_BUILD_REMOTE_DIR)/stb_image_write.h'"
	scp diagnostics/board/de10-trace-replay.cpp $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/de10-trace-replay.cpp
	scp software/trace/voodoo_trace_format.h $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/voodoo_trace_format.h
	scp software/trace/trace_replay_backend.h $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_backend.h
	scp software/trace/trace_replay_image.h $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_image.h
	scp software/trace/trace_replay_io.h $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_io.h
	scp software/trace/trace_replay_runner.h $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/trace_replay_runner.h
	scp vendor/stb/stb_image_write.h $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/stb_image_write.h
	ssh $(DE10_USER)@$(DE10_HOST) "g++ -O2 -std=c++17 -I'$(DE10_TRACE_BUILD_REMOTE_DIR)' -o '$(DE10_TRACE_BUILD_REMOTE_DIR)/de10-trace-replay' '$(DE10_TRACE_BUILD_REMOTE_DIR)/de10-trace-replay.cpp'"
	scp $(DE10_USER)@$(DE10_HOST):$(DE10_TRACE_BUILD_REMOTE_DIR)/de10-trace-replay $@

_glide_build_trace: build/host/simulation/sim_trace_harness.o
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim clean
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim TRACE_CAPTURE=1

_glide_build_xls:
	$(MAKE) -C $(SIM_DIR) xls-harness
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim clean
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim PURE_XLS=1

build/host/simulation/sim_trace_harness.o: FORCE
	$(MAKE) -C $(SIM_DIR) trace-harness

# --------------------------------------------------------------------------
# Trace-based testing (C++ trace_test binary)
# --------------------------------------------------------------------------

TRACE_TEST_BIN = build/host/reference/trace_test
XLS_DE10SIM_ARGS ?=
XLS_DE10SIM_REFERENCE ?=
XLS_DE10SIM_MAX_REPLAY_SECONDS ?= 22.3
STATE_ROUNDTRIP_BIN = build/host/reference/state_roundtrip_test

# Build trace_test binary (64-bit, independent of 32-bit Glide build).
$(TRACE_TEST_BIN): FORCE
	$(MAKE) -C software/reference

$(STATE_ROUNDTRIP_BIN): FORCE
	$(MAKE) -C software/reference state-roundtrip

native/trace/check/state-roundtrip: $(STATE_ROUNDTRIP_BIN)
	$(STATE_ROUNDTRIP_BIN)

native/xls-de10sim/compare-all: $(TRACE_TEST_BIN)
	nix develop path:. -c workflows/check/compare-emulation-xls-traces

native/xls-de10sim/run/%: FORCE
	@if [ -f verification/fixtures/traces/$*.bin ]; then \
	  src=verification/fixtures/traces/$*.bin; \
	elif [ -f verification/fixtures/traces/$*/trace.bin ]; then \
	  src=verification/fixtures/traces/$*; \
	else \
	  echo "ERROR: neither verification/fixtures/traces/$*.bin nor verification/fixtures/traces/$*/trace.bin found"; exit 1; \
	fi; \
	cache_gate=""; \
	if [ "$*" = "screamer2" ]; then cache_gate="--minimum-cache-hit-rate=0.9"; fi; \
	nix develop path:. -c workflows/run/replay-pure-xls-proc-trace --de10 \
	  --max-replay-seconds=$(XLS_DE10SIM_MAX_REPLAY_SECONDS) $$cache_gate \
	  $(XLS_DE10SIM_ARGS) "$$src"

native/xls-de10sim/check/%: FORCE
	@ref="$(XLS_DE10SIM_REFERENCE)"; \
	if [ -z "$$ref" ]; then ref="build/test-results/replay/$*/$*_ref.png"; fi; \
	test -f "$$ref" || { echo "ERROR: reference framebuffer $$ref not found"; exit 1; }; \
	$(MAKE) native/xls-de10sim/run/$* XLS_DE10SIM_ARGS='--framebuffer-reference '"$$ref"' $(XLS_DE10SIM_ARGS)'

# Capture trace from a Linux-hosted Glide test.
native/trace/run/%: native/trace/build
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim $*.exe TRACE_CAPTURE=1
	@mkdir -p verification/fixtures/traces
	cd $(GLIDE_TST_DIR) && \
	  SIM_TRACE_FILE=$(abspath verification/fixtures/traces)/$*.bin \
	  LD_LIBRARY_PATH=../../lib/sst1 \
	  ./$*.exe -n 1 < /dev/null

dos/xls/build/%:
	$(MAKE) _glide_build_xls
	$(MAKE) -C $(GLIDE_TST_DIR) -f $(DOS_SDK_MAKEFILE) $*.exe

dos/xls/run/%: dos/xls/build/%
	bash ./workflows/run/run-dosboxx32-xls-glide \
	  -c "mount c $(DOS_TEST_MOUNT_DIR)" \
	  -c "c:" \
	  -c "$*.exe" \
	  $(ARGS)

dos/xls/headless/%: dos/xls/build/%
	DOSBOXX32_HEADLESS=1 bash ./workflows/run/run-dosboxx32-xls-glide \
	  -c "mount c $(DOS_TEST_MOUNT_DIR)" \
	  -c "c:" \
	  -c "$*.exe" \
	  $(ARGS)

dos/xls/check-headless: dos/xls/build/df00sdk
	DOSBOXX32_HEADLESS=1 timeout 180 bash ./workflows/run/run-dosboxx32-xls-glide \
	  -c "mount c $(DOS_TEST_MOUNT_DIR)" \
	  -c "c:" \
	  -c "df00sdk.exe -n 10" \
	  -c "exit"

dos/trace/build/%: native/trace/build
	$(MAKE) -C $(GLIDE_TST_DIR) -f $(DOS_SDK_MAKEFILE) $*.exe

dos/trace/run/%: dos/trace/build/%
	@mkdir -p verification/fixtures/traces/dos
	SIM_TRACE_FILE=$(abspath verification/fixtures/traces)/dos/$*.bin \
	bash ./workflows/run/run-dosboxx32-glide \
	  -c "mount c $(DOS_TEST_MOUNT_DIR)" \
	  -c "c:" \
	  -c "$*.exe" \
	  $(ARGS)

dos/trace/headless/%: dos/trace/build/%
	@mkdir -p verification/fixtures/traces/dos
	DOSBOXX32_HEADLESS=1 SIM_TRACE_FILE=$(abspath verification/fixtures/traces)/dos/$*.bin \
	bash ./workflows/run/run-dosboxx32-glide \
	  -c "mount c $(DOS_TEST_MOUNT_DIR)" \
	  -c "c:" \
	  -c "$*.exe" \
	  $(ARGS)

clean-trace-test:
	$(MAKE) -C software/reference clean

# --------------------------------------------------------------------------

clean: clean-tests clean-glide clean-sim

clean-sim:
	$(MAKE) -C $(SIM_DIR) clean

clean-glide:
	$(MAKE) -C $(GLIDE_SRC_DIR) -f Makefile.sim clean

clean-tests:
	$(MAKE) -C $(GLIDE_TST_DIR) -f Makefile.sim clean

.PHONY: check check-python
check-python:
	python3 -B -m unittest discover -s workflows/tests -p 'test_*.py'
	python3 -B -m unittest discover -s diagnostics/profiling/tests -p 'test_*.py'
	python3 -B -m unittest discover -s diagnostics/repository/tests -p 'test_*.py'

check: check-python
	$(MAKE) -C software/runtime/mister check check-input check-output check-presenter
	$(MAKE) -C hardware/renderer test
	scala-cli --power test project.scala hardware --server=false
