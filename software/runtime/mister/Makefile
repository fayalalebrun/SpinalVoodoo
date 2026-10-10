CC ?= cc
CXX ?= c++
DOSBOX_SOURCE ?= ../../../build/deps/mister/dosbox-x
BUILD_DIR ?= ../../../build/host/mister
CFLAGS ?= -O2 -g
CPPFLAGS += -I. -I../../integrations/glide -I../../integrations/dosbox
CHECK_FLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -pthread
SOURCES = mister_mailbox.c mister_mailbox_test.c
GLIDE_BUILD_DIR ?= ../../../build/host/mister-arm-glide
DEMO_INCLUDES = -I../../../vendor/glide/glide2x/sst1/glide/src -I../../../vendor/glide/swlibs/fxmisc -I../../../vendor/glide/glide2x/sst1/init

.PHONY: all check check-input check-output check-presenter check-demo demo
all: $(BUILD_DIR)/libmister_runtime.so

demo: $(BUILD_DIR)/glide_demo

$(BUILD_DIR)/glide_demo: ../../../software/integrations/glide/glide_demo.c $(BUILD_DIR)/libmister_runtime.so $(GLIDE_BUILD_DIR)/libglide2x.so
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) $(DEMO_INCLUDES) ../../../software/integrations/glide/glide_demo.c $(LDFLAGS) \
		-L$(GLIDE_BUILD_DIR) -L$(BUILD_DIR) -lglide2x -lmister_runtime -lm -o "$@"

$(BUILD_DIR)/glide_demo_test: ../../../software/integrations/glide/glide_demo.c ../../../software/integrations/glide/glide_demo_test.c mister_runtime.h mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) $(DEMO_INCLUDES) ../../../software/integrations/glide/glide_demo.c ../../../software/integrations/glide/glide_demo_test.c $(LDFLAGS) -o "$@"

check-demo: $(BUILD_DIR)/glide_demo_test
	$(BUILD_DIR)/glide_demo_test 1
	$(BUILD_DIR)/glide_demo_test 61
	@set -e; for value in 0 -1 abc 10001 ''; do \
		status=0; $(BUILD_DIR)/glide_demo_test "$$value" || status=$$?; test "$$status" = 2; done
	@set -e; for phase in query open lock unlock exchange pixels; do \
		status=0; MISTER_DEMO_TEST_FAIL="$$phase" $(BUILD_DIR)/glide_demo_test 1 || status=$$?; test "$$status" = 1; done

$(BUILD_DIR)/libmister_runtime.so: mister_runtime.c mister_memory.c mister_mailbox.c mister_runtime.h mister_memory.h mister_mailbox.h Makefile ../../../workflows/build/build-mister-dosbox
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) -fPIC -shared mister_runtime.c mister_memory.c mister_mailbox.c \
		$(LDFLAGS) -Wl,-soname,libmister_runtime.so,-z,defs -o "$@"

check: check-demo $(BUILD_DIR)/mister_runtime_palette_test $(BUILD_DIR)/mister_runtime_cache_test $(BUILD_DIR)/mister_mailbox_async_test $(BUILD_DIR)/mister_mailbox_wait_test $(BUILD_DIR)/mister_mailbox_test $(BUILD_DIR)/mister_memory_test $(BUILD_DIR)/mister_runtime_test $(BUILD_DIR)/glide_harness_test $(BUILD_DIR)/glide_guest_test $(BUILD_DIR)/dosbox_glide_test
	$(BUILD_DIR)/mister_runtime_palette_test
	$(BUILD_DIR)/mister_runtime_cache_test
	$(BUILD_DIR)/mister_mailbox_async_test
	$(BUILD_DIR)/mister_mailbox_wait_test
	$(BUILD_DIR)/dosbox_glide_test
	$(BUILD_DIR)/glide_guest_test
	$(BUILD_DIR)/mister_mailbox_test
	cd "$(BUILD_DIR)" && ./mister_memory_test
	$(BUILD_DIR)/mister_runtime_test
	$(BUILD_DIR)/glide_harness_test
	python3 ../../../software/integrations/glide/glide_idle_test.py

check-presenter: $(BUILD_DIR)/glide_presenter_test
	$(BUILD_DIR)/glide_presenter_test

$(BUILD_DIR)/dosbox_glide_test: ../../../software/integrations/dosbox/dosbox_glide_test.cpp ../../../software/integrations/dosbox/dosbox_glide.h ../../../software/integrations/glide/glide_guest.h
	mkdir -p "$(BUILD_DIR)"
	$(CXX) $(CFLAGS) $(CPPFLAGS) -std=c++17 -Wall -Wextra -Werror -pedantic ../../../software/integrations/dosbox/dosbox_glide_test.cpp $(LDFLAGS) -o "$@"

$(BUILD_DIR)/glide_guest_test: ../../../software/integrations/glide/glide_guest.c ../../../software/integrations/glide/glide_guest_test.c ../../../software/integrations/glide/glide_guest.h mister_runtime.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) $(DEMO_INCLUDES) ../../../software/integrations/glide/glide_guest.c ../../../software/integrations/glide/glide_guest_test.c $(LDFLAGS) -o "$@"

$(BUILD_DIR)/glide_presenter_test: ../../../software/integrations/glide/glide_presenter.c ../../../software/integrations/glide/glide_presenter_test.c mister_runtime.h mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) -I../../../vendor/glide/glide2x/sst1/glide/src -I../../../vendor/glide/swlibs/fxmisc -I../../../vendor/glide/glide2x/sst1/init \
		../../../software/integrations/glide/glide_presenter.c ../../../software/integrations/glide/glide_presenter_test.c $(LDFLAGS) -Wl,--wrap=mister_runtime_present,--wrap=mister_runtime_exchange \
		-Wl,--wrap=mister_runtime_present_renderer,--wrap=mister_runtime_stop_presenting -o "$@"

check-output: $(BUILD_DIR)/dosbox_output_test
	$(BUILD_DIR)/dosbox_output_test

$(BUILD_DIR)/dosbox_output_test: ../../../software/integrations/dosbox/dosbox_output.cpp ../../../software/integrations/dosbox/dosbox_output_test.cpp mister_runtime.h mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CXX) $(CFLAGS) $(CPPFLAGS) -std=c++17 -Wall -Wextra -Werror -pedantic -pthread \
		../../../software/integrations/dosbox/dosbox_output.cpp ../../../software/integrations/dosbox/dosbox_output_test.cpp $$(pkg-config --cflags sdl2 | sed 's/-I/-isystem /g') $$(pkg-config --libs sdl2) $(LDFLAGS) \
		-Wl,--wrap=mister_runtime_present -o "$@"

check-input: $(BUILD_DIR)/dosbox_input_test
	$(BUILD_DIR)/dosbox_input_test

$(BUILD_DIR)/dosbox_input_test: ../../../software/integrations/dosbox/dosbox_input.cpp ../../../software/integrations/dosbox/dosbox_media.cpp ../../../software/integrations/dosbox/dosbox_input_test.cpp mister_runtime.h mister_mailbox.h $(DOSBOX_SOURCE)/include/keyboard.h
	mkdir -p "$(BUILD_DIR)"
	$(CXX) $(CFLAGS) $(CPPFLAGS) -std=c++17 -Wall -Wextra -Werror -pedantic -pthread -I"$(DOSBOX_SOURCE)/include" \
		../../../software/integrations/dosbox/dosbox_input.cpp ../../../software/integrations/dosbox/dosbox_media.cpp ../../../software/integrations/dosbox/dosbox_input_test.cpp $(LDFLAGS) \
		-Wl,--wrap=mister_runtime_acquire,--wrap=mister_runtime_release,--wrap=mister_runtime_exchange,--wrap=clock_gettime -o "$@"

$(BUILD_DIR)/glide_harness_test: ../../../software/integrations/glide/glide_harness.c ../../../software/integrations/glide/glide_harness_test.c mister_runtime.h mister_mailbox.h ../../simulation/xls/harness/sim_harness.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) ../../../software/integrations/glide/glide_harness.c ../../../software/integrations/glide/glide_harness_test.c mister_mailbox.c $(LDFLAGS) \
		-Wl,--wrap=mister_runtime_acquire,--wrap=mister_runtime_release,--wrap=mister_runtime_exchange \
		-Wl,--wrap=mister_runtime_stop_presenting,--wrap=mister_runtime_submit_writes \
		-Wl,--wrap=mister_runtime_upload_palette -o "$@"

$(BUILD_DIR)/mister_runtime_cache_test: mister_runtime.c mister_mailbox.c mister_runtime_cache_test.c mister_runtime.h mister_memory.h mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) mister_runtime.c mister_mailbox.c mister_runtime_cache_test.c $(LDFLAGS) \
		-Wl,--wrap=mister_memory_open,--wrap=mister_memory_close \
		-Wl,--wrap=mister_mailbox_attach,--wrap=mister_mailbox_detach,--wrap=mister_mailbox_exchange \
		-Wl,--wrap=mister_mailbox_submit_writes,--wrap=mister_mailbox_drain -o "$@"

$(BUILD_DIR)/mister_runtime_palette_test: mister_runtime.c mister_mailbox.c mister_runtime_palette_test.c mister_runtime.h mister_memory.h mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) mister_runtime.c mister_mailbox.c mister_runtime_palette_test.c $(LDFLAGS) \
		-Wl,--wrap=mister_memory_open,--wrap=mister_memory_close \
		-Wl,--wrap=mister_mailbox_attach,--wrap=mister_mailbox_detach,--wrap=mister_mailbox_exchange \
		-Wl,--wrap=mister_mailbox_submit_writes,--wrap=mister_mailbox_drain -o "$@"

$(BUILD_DIR)/mister_runtime_test: mister_runtime.c mister_runtime_test.c mister_runtime.h mister_memory.h mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) mister_runtime.c mister_runtime_test.c $(LDFLAGS) \
		-Wl,--wrap=mister_memory_open,--wrap=mister_memory_close \
		-Wl,--wrap=mister_mailbox_attach,--wrap=mister_mailbox_detach,--wrap=mister_mailbox_exchange \
		-Wl,--wrap=mister_mailbox_submit_writes,--wrap=mister_mailbox_drain -o "$@"

$(BUILD_DIR)/mister_memory_test: mister_memory.c mister_memory_test.c mister_memory.h mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) mister_memory.c mister_memory_test.c $(LDFLAGS) \
		-Wl,--wrap=open,--wrap=mmap,--wrap=munmap,--wrap=close -o "$@"

$(BUILD_DIR)/mister_mailbox_async_test: mister_mailbox_async_test.c mister_mailbox_wait_test.c mister_mailbox.c mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) mister_mailbox_async_test.c $(LDFLAGS) -o "$@"

$(BUILD_DIR)/mister_mailbox_wait_test: mister_mailbox_wait_test.c mister_mailbox.c mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) mister_mailbox_wait_test.c $(LDFLAGS) -o "$@"

$(BUILD_DIR)/mister_mailbox_test: $(SOURCES) mister_mailbox.h
	mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CHECK_FLAGS) $(SOURCES) $(LDFLAGS) -o "$@"
