/*
* Copyright Altera Corporation (C) 2012-2014. All rights reserved
*
* SPDX-License-Identifier:  BSD-3-Clause
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  * Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the distribution.
*  * Neither the name of Altera Corporation nor the
*  names of its contributors may be used to endorse or promote products
*  derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL ALTERA CORPORATION BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "sequencer_defines.h"

#include "alt_types.h"
#include "system.h"
#if HPS_HW
#include "sdram_io.h"
#else
#include "io.h"
#endif
#include "sequencer.h"
#include "tclrpt.h"
#include "sequencer_auto.h"

#if HHP_HPS_SIMULATION
#include "hps_controller.h"
#endif


/******************************************************************************
 ******************************************************************************
 ** NOTE: Special Rules for Globale Variables                                **
 **                                                                          **
 ** All global variables that are explicitly initialized (including          **
 ** explicitly initialized to zero), are only initialized once, during       **
 ** configuration time, and not again on reset.  This means that they        **
 ** preserve their current contents across resets, which is needed for some  **
 ** special cases involving communication with external modules.  In         **
 ** addition, this avoids paying the price to have the memory initialized,   **
 ** even for zeroed data, provided it is explicitly set to zero in the code, **
 ** and doesn't rely on implicit initialization.                             **
 ******************************************************************************
 ******************************************************************************/

#ifndef ARMCOMPILER
#if ARRIAV
// Temporary workaround to place the initial stack pointer at a safe offset from end
#define STRINGIFY(s)		STRINGIFY_STR(s)
#define STRINGIFY_STR(s)	#s
asm(".global __alt_stack_pointer");
asm("__alt_stack_pointer = " STRINGIFY(STACK_POINTER));
#endif

#if CYCLONEV
// Temporary workaround to place the initial stack pointer at a safe offset from end
#define STRINGIFY(s)		STRINGIFY_STR(s)
#define STRINGIFY_STR(s)	#s
asm(".global __alt_stack_pointer");
asm("__alt_stack_pointer = " STRINGIFY(STACK_POINTER));
#endif
#endif

#if ENABLE_PRINTF_LOG
#include <stdio.h>
#include <string.h>

typedef struct {
	alt_u32 v;
	alt_u32 p;
	alt_u32 d;
	alt_u32 ps;
} dqs_pos_t;

/*
The parameters that were previously here are now supplied by generation, until the new data manager is working.
*/

struct {
	const char *stage;

	alt_u32 vfifo_idx;

	dqs_pos_t gwrite_pos[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];

	dqs_pos_t dqs_enable_left_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH];
	dqs_pos_t dqs_enable_right_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH];
	dqs_pos_t dqs_enable_mid[RW_MGR_MEM_IF_READ_DQS_WIDTH];

	dqs_pos_t dqs_wlevel_left_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	dqs_pos_t dqs_wlevel_right_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	dqs_pos_t dqs_wlevel_mid[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];

	alt_32 dq_read_left_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH][RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 dq_read_right_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH][RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 dq_write_left_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 dq_write_right_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 dm_left_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_NUM_DM_PER_WRITE_GROUP];
	alt_32 dm_right_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_NUM_DM_PER_WRITE_GROUP];
} bfm_gbl;

#endif

#if HPS_HW
#include <sdram.h>
#endif // HPS_HW

#if BFM_MODE
#include <stdio.h>

// DPI access function via library
extern long long get_sim_time(void);

typedef struct {
	alt_u32 v;
	alt_u32 p;
	alt_u32 d;
	alt_u32 ps;
} dqs_pos_t;

/*
The parameters that were previously here are now supplied by generation, until the new data manager is working.
*/

struct {
	FILE *outfp;
	int bfm_skip_guaranteed_write;
	int trk_sample_count;
	int trk_long_idle_updates;
	int lfifo_margin;
	const char *stage;

	alt_u32 vfifo_idx;

	dqs_pos_t gwrite_pos[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	
	dqs_pos_t dqs_enable_left_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH];
	dqs_pos_t dqs_enable_right_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH];
	dqs_pos_t dqs_enable_mid[RW_MGR_MEM_IF_READ_DQS_WIDTH];

	dqs_pos_t dqs_wlevel_left_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	dqs_pos_t dqs_wlevel_right_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	dqs_pos_t dqs_wlevel_mid[RW_MGR_MEM_IF_WRITE_DQS_WIDTH];

	alt_32 dq_read_left_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH][RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 dq_read_right_edge[RW_MGR_MEM_IF_READ_DQS_WIDTH][RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 dq_write_left_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_MEM_DQ_PER_WRITE_DQS];
	alt_32 dq_write_right_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_MEM_DQ_PER_WRITE_DQS];
	alt_32 dm_left_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_NUM_DM_PER_WRITE_GROUP];
	alt_32 dm_right_edge[RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_NUM_DM_PER_WRITE_GROUP];
} bfm_gbl;


#endif

#if ENABLE_TCL_DEBUG
debug_data_t my_debug_data;
#endif

#define NEWVERSION_RDDESKEW 1
#define NEWVERSION_WRDESKEW 1
#define NEWVERSION_GW 1
#define NEWVERSION_WL 1
#define NEWVERSION_DQSEN 1

// Just to make the debugging code more uniform
#ifndef RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM
#define RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM 0
#endif

#if HALF_RATE
#define HALF_RATE_MODE 1
#else
#define HALF_RATE_MODE 0
#endif

#if QUARTER_RATE
#define QUARTER_RATE_MODE 1
#else
#define QUARTER_RATE_MODE 0
#endif
#define DELTA_D 1

// case:56390
// VFIFO_CONTROL_WIDTH_PER_DQS is the number of VFIFOs actually instantiated per DQS. This is always one except:
// AV QDRII where it is 2 for x18 and x18w2, and 4 for x36 and x36w2
// RLDRAMII x36 and x36w2 where it is 2.
// In 12.0sp1 we set this to 4 for all of the special cases above to keep it simple.
// In 12.0sp2 or 12.1 this should get moved to generation and unified with the same constant used in the phy mgr

#define VFIFO_CONTROL_WIDTH_PER_DQS 1

#if ARRIAV

#if QDRII 
 #if RW_MGR_MEM_DQ_PER_READ_DQS > 9
  #undef VFIFO_CONTROL_WIDTH_PER_DQS
  #define VFIFO_CONTROL_WIDTH_PER_DQS 4
 #endif
#endif // protocol check

#if RLDRAMII
 #if RW_MGR_MEM_DQ_PER_READ_DQS > 9
  #undef VFIFO_CONTROL_WIDTH_PER_DQS
  #define VFIFO_CONTROL_WIDTH_PER_DQS 2
 #endif
#endif // protocol check

#endif // family check

// In order to reduce ROM size, most of the selectable calibration steps are
// decided at compile time based on the user's calibration mode selection,
// as captured by the STATIC_CALIB_STEPS selection below.
//
// However, to support simulation-time selection of fast simulation mode, where
// we skip everything except the bare minimum, we need a few of the steps to
// be dynamic.  In those cases, we either use the DYNAMIC_CALIB_STEPS for the
// check, which is based on the rtl-supplied value, or we dynamically compute the
// value to use based on the dynamically-chosen calibration mode

#if QDRII
#define BTFLD_FMT "%llx"
#else
#define BTFLD_FMT "%lx"
#endif

#if BFM_MODE // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// TODO: should make this configurable; could even have it read from config file or env at startup
#define DLEVEL 2
// space around comma is required for varargs macro to remove comma if args is empty
#define DPRINT(level, fmt, args...) 	if (DLEVEL >= (level)) printf("[%lld] SEQ.C: " fmt "\n" , get_sim_time(), ## args)
#define IPRINT(fmt, args...) 	printf("[%lld] SEQ.C: " fmt "\n" , get_sim_time(), ## args)
#define BFM_GBL_SET(field,value)	bfm_gbl.field = value
#define BFM_GBL_GET(field)		bfm_gbl.field
#define BFM_STAGE(label)		BFM_GBL_SET(stage,label)
#define BFM_INC_VFIFO			bfm_gbl.vfifo_idx = (bfm_gbl.vfifo_idx + 1) % VFIFO_SIZE
#define COV(label)			getpid() /* no-op marker for coverage */

#elif ENABLE_PRINTF_LOG // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define DLEVEL 2

void wait_printf_queue()
{
	alt_u32 next_entry;

	while (debug_printf_output->count == PRINTF_READ_BUFFER_FIFO_WORDS || debug_printf_output->slave_lock != 0)
	{}

	debug_printf_output->master_lock = 1;
	next_entry = (debug_printf_output->head + debug_printf_output->count) % PRINTF_READ_BUFFER_FIFO_WORDS;
	strcpy((char*)(&(debug_printf_output->read_buffer[next_entry])), (char*)(debug_printf_output->active_word));
	debug_printf_output->count++;
	debug_printf_output->master_lock = 0;
}
#define DPRINT(level, fmt, args...) \
	if (DLEVEL >= (level)) { \
		snprintf((char*)(debug_printf_output->active_word), PRINTF_READ_BUFFER_SIZE*4, "DEBUG:" fmt, ## args); \
		wait_printf_queue(); \
	}
#define IPRINT(fmt, args...) \
		snprintf((char*)(debug_printf_output->active_word), PRINTF_READ_BUFFER_SIZE*4, "INFO:" fmt, ## args); \
		wait_printf_queue();

#define BFM_GBL_SET(field,value)	bfm_gbl.field = value
#define BFM_GBL_GET(field)		bfm_gbl.field
#define BFM_STAGE(label)		BFM_GBL_SET(stage,label)
#define BFM_INC_VFIFO			bfm_gbl.vfifo_idx = (bfm_gbl.vfifo_idx + 1) % VFIFO_SIZE
#define COV(label)

#elif HPS_HW // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// For HPS running on actual hardware

#define DLEVEL 0
#ifdef HPS_HW_SERIAL_SUPPORT
// space around comma is required for varargs macro to remove comma if args is empty
#define DPRINT(level, fmt, args...) 	if (DLEVEL >= (level)) printf("SEQ.C: " fmt "\n" , ## args)
#define IPRINT(fmt, args...) 	        printf("SEQ.C: " fmt "\n" , ## args)
#if RUNTIME_CAL_REPORT
#define RPRINT(fmt, args...)            printf("SEQ.C: " fmt "\n" , ## args)
#endif 
#else
#define DPRINT(level, fmt, args...)
#define IPRINT(fmt, args...)
#endif
#define BFM_GBL_SET(field,value)
#define BFM_GBL_GET(field) 		((long unsigned int)0)
#define BFM_STAGE(stage)	
#define BFM_INC_VFIFO
#define COV(label)

#else // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-----------------------------------

// Default mode
#define DPRINT(level, fmt, args...) 
#define IPRINT(fmt, args...)
#define BFM_GBL_SET(field,value)
#define BFM_GBL_GET(field) 0
#define BFM_STAGE(stage)	
#define BFM_INC_VFIFO
#define COV(label)

#endif // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~----------------------------------

#if BFM_MODE
#define TRACE_FUNC(fmt, args...) DPRINT(1, "%s[%ld]: " fmt, __func__, __LINE__ , ## args)
#else
#define TRACE_FUNC(fmt, args...) DPRINT(1, "%s[%d]: " fmt, __func__, __LINE__ , ## args)
#endif

#if BFM_MODE
// In BFM mode, we do full calibration as for real-rtl
#define DYNAMIC_CALIB_STEPS STATIC_CALIB_STEPS
#else
#define DYNAMIC_CALIB_STEPS (dyn_calib_steps)
#endif

#if STATIC_SIM_FILESET
#define STATIC_IN_RTL_SIM CALIB_IN_RTL_SIM
#else
#define STATIC_IN_RTL_SIM 0
#endif

#if STATIC_SKIP_MEM_INIT
#define STATIC_SKIP_DELAY_LOOPS CALIB_SKIP_DELAY_LOOPS
#else
#define STATIC_SKIP_DELAY_LOOPS 0
#endif

#if STATIC_FULL_CALIBRATION
#define STATIC_CALIB_STEPS (STATIC_IN_RTL_SIM | CALIB_SKIP_FULL_TEST | STATIC_SKIP_DELAY_LOOPS)
#elif STATIC_QUICK_CALIBRATION
#define STATIC_CALIB_STEPS (STATIC_IN_RTL_SIM | CALIB_SKIP_FULL_TEST | CALIB_SKIP_WRITES | CALIB_SKIP_DELAY_SWEEPS | CALIB_SKIP_ALL_BITS_CHK | STATIC_SKIP_DELAY_LOOPS)
#elif STATIC_SKIP_CALIBRATION
#define STATIC_CALIB_STEPS (STATIC_IN_RTL_SIM | CALIB_SKIP_FULL_TEST | CALIB_SKIP_WRITES | CALIB_SKIP_WLEVEL | CALIB_SKIP_LFIFO | CALIB_SKIP_VFIFO | CALIB_SKIP_DELAY_SWEEPS | CALIB_SKIP_ALL_BITS_CHK | STATIC_SKIP_DELAY_LOOPS)
#else
#undef STATIC_CALIB_STEPS
// This should force an error
#endif

// calibration steps requested by the rtl
alt_u16 dyn_calib_steps = 0;

// To make CALIB_SKIP_DELAY_LOOPS a dynamic conditional option
// instead of static, we use boolean logic to select between
// non-skip and skip values
//
// The mask is set to include all bits when not-skipping, but is
// zero when skipping

alt_u16 skip_delay_mask = 0;	// mask off bits when skipping/not-skipping

#define SKIP_DELAY_LOOP_VALUE_OR_ZERO(non_skip_value) \
	((non_skip_value) & skip_delay_mask)


// TODO: The skip group strategy is completely missing

gbl_t *gbl = 0;
param_t *param = 0;

alt_u32 curr_shadow_reg = 0;

#if ENABLE_DELAY_CHAIN_WRITE
alt_u32 vfifo_settings[RW_MGR_MEM_IF_READ_DQS_WIDTH];
#endif // ENABLE_DELAY_CHAIN_WRITE

#if ENABLE_NON_DESTRUCTIVE_CALIB
// Technically, the use of these variables could be separated from ENABLE_NON_DESTRUCTIVE_CALIB
// but currently they are part of a single feature which is not fully validated, so we're keeping
// them together

// These variables can be modified by external rtl modules, and hence are "volatile"
volatile alt_u32 no_init = 0;
volatile alt_u32 abort_cal = 0;
#endif

alt_u32 rw_mgr_mem_calibrate_write_test (alt_u32 rank_bgn, alt_u32 write_group, alt_u32 use_dm, alt_u32 all_correct, t_btfld *bit_chk, alt_u32 all_ranks);

#if ENABLE_BRINGUP_DEBUGGING

#define DI_BUFFER_DEBUG_SIZE   64

alt_u8 di_buf_gbl[DI_BUFFER_DEBUG_SIZE*4] = {0};

void load_di_buf_gbl(void)
{
	int i;
	int j;

	for (i = 0; i < DI_BUFFER_DEBUG_SIZE; i++) {
		alt_u32 val = IORD_32DIRECT(RW_MGR_DI_BASE + i*4, 0);
		for (j = 0; j < 4; j++) {
			alt_u8 byte = (val >> (8*j)) & 0xff;
			di_buf_gbl[i*4 + j] = byte;
		}
	}
}

#endif	/* ENABLE_BRINGUP_DEBUGGING */


#if ENABLE_DQSEN_SWEEP
void init_di_buffer(void)
{
	alt_u32 i;

	debug_data->di_report.flags = 0;
	debug_data->di_report.cur_samples = 0;

	for (i = 0; i < NUM_DI_SAMPLE; i++)
	{
		debug_data->di_report.di_buffer[i].bit_chk = 0;
		debug_data->di_report.di_buffer[i].delay = 0;
		debug_data->di_report.di_buffer[i].d = 0;
		debug_data->di_report.di_buffer[i].v = 0;
		debug_data->di_report.di_buffer[i].p = 0;
		debug_data->di_report.di_buffer[i].di_buffer_0a = 0;
		debug_data->di_report.di_buffer[i].di_buffer_0b = 0;
		debug_data->di_report.di_buffer[i].di_buffer_1a = 0;
		debug_data->di_report.di_buffer[i].di_buffer_1b = 0;
		debug_data->di_report.di_buffer[i].di_buffer_2a = 0;
		debug_data->di_report.di_buffer[i].di_buffer_2b = 0;
		debug_data->di_report.di_buffer[i].di_buffer_3a = 0;
		debug_data->di_report.di_buffer[i].di_buffer_3b = 0;
		debug_data->di_report.di_buffer[i].di_buffer_4a = 0;
		debug_data->di_report.di_buffer[i].di_buffer_4b = 0;
	}
}

inline void flag_di_buffer_ready()
{
	debug_data->di_report.flags |= DI_REPORT_FLAGS_READY;
}

inline void flag_di_buffer_done()
{
	debug_data->di_report.flags |= DI_REPORT_FLAGS_READY;
	debug_data->di_report.flags |= DI_REPORT_FLAGS_DONE;
}

void wait_di_buffer(void)
{
	if (debug_data->di_report.cur_samples == NUM_DI_SAMPLE)
	{
		flag_di_buffer_ready();
		while (debug_data->di_report.cur_samples != 0)
		{
		}
		debug_data->di_report.flags = 0;
	}
}

void sample_di_data(alt_u32 bit_chk, alt_u32 delay, alt_u32 d, alt_u32 v, alt_u32 p)
{
	alt_u32 k;
	alt_u32 di_status_word;
	alt_u32 di_word_avail;
	alt_u32 di_write_to_read_ratio;
	alt_u32 di_write_to_read_ratio_2_exp;

	wait_di_buffer();

	k = debug_data->di_report.cur_samples;

	debug_data->di_report.di_buffer[k].bit_chk = bit_chk;
	debug_data->di_report.di_buffer[k].delay = delay;
	debug_data->di_report.di_buffer[k].d = d;
	debug_data->di_report.di_buffer[k].v = v;
	debug_data->di_report.di_buffer[k].p = p;

	di_status_word = IORD_32DIRECT(BASE_RW_MGR + 8, 0);
	di_word_avail = di_status_word & 0x0000FFFF;
	di_write_to_read_ratio = (di_status_word & 0x00FF0000) >> 16;
	di_write_to_read_ratio_2_exp = (di_status_word & 0xFF000000) >> 24;

	debug_data->di_report.di_buffer[k].di_buffer_0a = IORD_32DIRECT(BASE_RW_MGR + 16 + 0*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_0b = IORD_32DIRECT(BASE_RW_MGR + 16 + 1*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_1a = IORD_32DIRECT(BASE_RW_MGR + 16 + 2*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_1b = IORD_32DIRECT(BASE_RW_MGR + 16 + 3*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_2a = IORD_32DIRECT(BASE_RW_MGR + 16 + 4*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_2b = IORD_32DIRECT(BASE_RW_MGR + 16 + 5*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_3a = IORD_32DIRECT(BASE_RW_MGR + 16 + 6*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_3b = IORD_32DIRECT(BASE_RW_MGR + 16 + 7*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_4a = IORD_32DIRECT(BASE_RW_MGR + 16 + 8*4, 0);
	debug_data->di_report.di_buffer[k].di_buffer_4b = IORD_32DIRECT(BASE_RW_MGR + 16 + 9*4, 0);

	debug_data->di_report.cur_samples = debug_data->di_report.cur_samples + 1;
}
#endif

// This (TEST_SIZE) is used to test handling of large roms, to make
// sure we are sizing things correctly
// Note, the initialized data takes up twice the space in rom, since
// there needs to be a copy with the initial value and a copy that is
// written too, since on soft-reset, it needs to have the initial values
// without reloading the memory from external sources

// #define TEST_SIZE	(6*1024)

#ifdef TEST_SIZE

#define PRE_POST_TEST_SIZE 3

unsigned int pre_test_size_mem[PRE_POST_TEST_SIZE] = { 1, 2, 3};

unsigned int test_size_mem[TEST_SIZE/sizeof(unsigned int)] = { 100, 200, 300 };

unsigned int post_test_size_mem[PRE_POST_TEST_SIZE] = {10, 20, 30};

void write_test_mem(void)
{
	int i;

	for (i = 0; i < PRE_POST_TEST_SIZE; i++) {
		pre_test_size_mem[i] = (i+1)*10;
		post_test_size_mem[i] = (i+1);
	}
	
	for (i = 0; i < sizeof(test_size_mem)/sizeof(unsigned int); i++) {
		test_size_mem[i] = i;
	}
	
}

int check_test_mem(int start)
{
	int i;

	for (i = 0; i < PRE_POST_TEST_SIZE; i++) {
		if (start) {
			if (pre_test_size_mem[i] != (i+1)) {
				return 0;
			}
			if (post_test_size_mem[i] != (i+1)*10) {
				return 0;
			}
		} else {
			if (pre_test_size_mem[i] != (i+1)*10) {
				return 0;
			}
			if (post_test_size_mem[i] != (i+1)) {
				return 0;
			}
		}
	}
		
	for (i = 0; i < sizeof(test_size_mem)/sizeof(unsigned int); i++) {
		if (start) {
			if (i < 3) {
				if (test_size_mem[i] != (i+1)*100) {
					return 0;
				}
			} else {
				if (test_size_mem[i] != 0) {
					return 0;
				}
			}
		} else {
			if (test_size_mem[i] != i) {
				return 0;
			}
		}
	}

	return 1;
}

#endif // TEST_SIZE

static void set_failing_group_stage(alt_u32 group, alt_u32 stage, alt_u32 substage)
{
	ALTERA_ASSERT(group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	// Only set the global stage if there was not been any other failing group
	if (gbl->error_stage == CAL_STAGE_NIL)
	{
		gbl->error_substage = substage;
		gbl->error_stage = stage;
		gbl->error_group = group;
		TCLRPT_SET(debug_summary_report->error_sub_stage, substage);
		TCLRPT_SET(debug_summary_report->error_stage, stage);
		TCLRPT_SET(debug_summary_report->error_group, group);

	}

	// Always set the group specific errors
	TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][group].error_stage, stage);
	TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][group].error_sub_stage, substage);

}

static inline void reg_file_set_group(alt_u32 set_group)
{
	// Read the current group and stage
	alt_u32 cur_stage_group = IORD_32DIRECT (REG_FILE_CUR_STAGE, 0);

	// Clear the group
	cur_stage_group &= 0x0000FFFF;

	// Set the group
	cur_stage_group |= (set_group << 16);

	// Write the data back
	IOWR_32DIRECT (REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static inline void reg_file_set_stage(alt_u32 set_stage)
{
	// Read the current group and stage
	alt_u32 cur_stage_group = IORD_32DIRECT (REG_FILE_CUR_STAGE, 0);

	// Clear the stage and substage
	cur_stage_group &= 0xFFFF0000;

	// Set the stage
	cur_stage_group |= (set_stage & 0x000000FF);

	// Write the data back
	IOWR_32DIRECT (REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static inline void reg_file_set_sub_stage(alt_u32 set_sub_stage)
{
	// Read the current group and stage
	alt_u32 cur_stage_group = IORD_32DIRECT (REG_FILE_CUR_STAGE, 0);

	// Clear the substage
	cur_stage_group &= 0xFFFF00FF;

	// Set the sub stage
	cur_stage_group |= ((set_sub_stage << 8) & 0x0000FF00);

	// Write the data back
	IOWR_32DIRECT (REG_FILE_CUR_STAGE, 0, cur_stage_group);
}

static inline alt_u32 is_write_group_enabled_for_dm(alt_u32 write_group)
{
#if DM_PINS_ENABLED
 #if RLDRAMII
	alt_32 decrement_counter = write_group + 1;

	while (decrement_counter > 0)
	{
		decrement_counter -= RW_MGR_MEM_IF_WRITE_DQS_WIDTH/RW_MGR_MEM_DATA_MASK_WIDTH;
	}

	if (decrement_counter == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
 #else
	return 1;
 #endif
#else
	return 0;
#endif
}

static inline void select_curr_shadow_reg_using_rank(alt_u32 rank)
{
#if USE_SHADOW_REGS
	//USER Map the rank to its shadow reg and set the global variable
	curr_shadow_reg = (rank >> (NUM_RANKS_PER_SHADOW_REG - 1));
#endif
}

void initialize(void)
{
	TRACE_FUNC();

	//USER calibration has control over path to memory 

#if HARD_PHY
	// In Hard PHY this is a 2-bit control:
	// 0: AFI Mux Select
	// 1: DDIO Mux Select
	IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 0x3);
#else
	IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 1);
#endif

	//USER memory clock is not stable we begin initialization 

	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 0);

	//USER calibration status all set to zero 

	IOWR_32DIRECT (PHY_MGR_CAL_STATUS, 0, 0);
	IOWR_32DIRECT (PHY_MGR_CAL_DEBUG_INFO, 0, 0);

	if (((DYNAMIC_CALIB_STEPS) & CALIB_SKIP_ALL) != CALIB_SKIP_ALL) {
		param->read_correct_mask_vg  = ((t_btfld)1 << (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS)) - 1;
		param->write_correct_mask_vg = ((t_btfld)1 << (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS)) - 1;
		param->read_correct_mask     = ((t_btfld)1 << RW_MGR_MEM_DQ_PER_READ_DQS) - 1;
		param->write_correct_mask    = ((t_btfld)1 << RW_MGR_MEM_DQ_PER_WRITE_DQS) - 1;
		param->dm_correct_mask       = ((t_btfld)1 << (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH)) - 1;
	}
}


#if MRS_MIRROR_PING_PONG_ATSO
// This code is specific to the ATSO setup.  There are two ways to set
// the cs/odt mask:
// 1. the normal way (set_rank_and_odt_mask)
//  This method will be used in general.  The behavior will be to unmask
//  BOTH CS (i.e. broadcast to both sides as if calibrating one large interface).
// 2. this function
//  This method will be used for MRS settings only.  This allows us to do settings
//  on a per-side basis.  This is needed because Slot 1 Rank 1 needs a mirrored MRS.
// This function is specific to our setup ONLY.
void set_rank_and_odt_mask_for_ping_pong_atso(alt_u32 side, alt_u32 odt_mode)
{
	alt_u32 odt_mask_0 = 0;
	alt_u32 odt_mask_1 = 0;
	alt_u32 cs_and_odt_mask;
	
	if(odt_mode == RW_MGR_ODT_MODE_READ_WRITE)
	{
		//USER 1 Rank
		//USER Read: ODT = 0
		//USER Write: ODT = 1
		odt_mask_0 = 0x0;
		odt_mask_1 = 0x1;
	}
	else
	{
		odt_mask_0 = 0x0;
		odt_mask_1 = 0x0;
	}

	cs_and_odt_mask = 
		(0xFF & ~(1 << side)) |
		((0xFF & odt_mask_0) << 8) |
		((0xFF & odt_mask_1) << 16);

	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, cs_and_odt_mask);
}
#endif

#if DDR3
void set_rank_and_odt_mask(alt_u32 rank, alt_u32 odt_mode)
{
	alt_u32 odt_mask_0 = 0;
	alt_u32 odt_mask_1 = 0;
	alt_u32 cs_and_odt_mask;
	
	if(odt_mode == RW_MGR_ODT_MODE_READ_WRITE)
	{
#if USE_SHADOW_REGS	
		alt_u32 rank_one_hot = (0xFF & (1 << rank));
		select_curr_shadow_reg_using_rank(rank);
	
		//USER Assert afi_rrank and afi_wrank. These signals ultimately drive
		//USER the read/write rank select signals which select the shadow register.
		IOWR_32DIRECT (RW_MGR_SET_ACTIVE_RANK, 0, rank_one_hot);
#endif	

		if ( LRDIMM ) {
			// USER LRDIMMs have two cases to consider: single-slot and dual-slot.
			// USER In single-slot, assert ODT for write only.
			// USER In dual-slot, assert ODT for both slots for write,
			// USER and on the opposite slot only for reads.
			// USER
			// USER Further complicating this is that both DIMMs have either 1 or 2 ODT
			// USER inputs, which do the same thing (only one is actually required).
			if ((RW_MGR_MEM_CHIP_SELECT_WIDTH/RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM) == 1) {
				// USER Single-slot case
				if (RW_MGR_MEM_ODT_WIDTH == 1) {
					// USER Read = 0, Write = 1
					odt_mask_0 = 0x0;
					odt_mask_1 = 0x1;
				} else if (RW_MGR_MEM_ODT_WIDTH == 2) {
					// USER Read = 00, Write = 11
					odt_mask_0 = 0x0;
					odt_mask_1 = 0x3;
				}
			} else if ((RW_MGR_MEM_CHIP_SELECT_WIDTH/RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM) == 2) {
				// USER Dual-slot case
				if (RW_MGR_MEM_ODT_WIDTH == 2) {
					// USER Read: asserted for opposite slot, Write: asserted for both
					odt_mask_0 = (rank < 2) ? 0x2 : 0x1;
					odt_mask_1 = 0x3;
				} else if (RW_MGR_MEM_ODT_WIDTH == 4) {
					// USER Read: asserted for opposite slot, Write: asserted for both
					odt_mask_0 = (rank < 2) ? 0xC : 0x3;
					odt_mask_1 = 0xF;
				}
			}
		} else if(RW_MGR_MEM_NUMBER_OF_RANKS == 1) { 
			//USER 1 Rank
			//USER Read: ODT = 0
			//USER Write: ODT = 1
			odt_mask_0 = 0x0;
			odt_mask_1 = 0x1;
		} else if(RW_MGR_MEM_NUMBER_OF_RANKS == 2) { 
			//USER 2 Ranks
			if(RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 1 ||
			   (RDIMM && RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 2 
			   && RW_MGR_MEM_CHIP_SELECT_WIDTH == 4)) {
				//USER - Dual-Slot , Single-Rank (1 chip-select per DIMM)
				//USER OR
				//USER - RDIMM, 4 total CS (2 CS per DIMM) means 2 DIMM
				//USER Since MEM_NUMBER_OF_RANKS is 2 they are both single rank
				//USER with 2 CS each (special for RDIMM)
				//USER Read: Turn on ODT on the opposite rank
				//USER Write: Turn on ODT on all ranks
				odt_mask_0 = 0x3 & ~(1 << rank);
				odt_mask_1 = 0x3;
			} else {
				//USER - Single-Slot , Dual-rank DIMMs (2 chip-selects per DIMM)
				//USER Read: Turn on ODT off on all ranks
				//USER Write: Turn on ODT on active rank
				odt_mask_0 = 0x0;
				odt_mask_1 = 0x3 & (1 << rank);
			}
				} else {
			//USER 4 Ranks
			//USER Read:
			//USER ----------+-----------------------+
			//USER           |                       |
			//USER           |         ODT           |
			//USER Read From +-----------------------+
			//USER   Rank    |  3  |  2  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
			//USER     0     |  0  |  1  |  0  |  0  |
			//USER     1     |  1  |  0  |  0  |  0  |
			//USER     2     |  0  |  0  |  0  |  1  |
			//USER     3     |  0  |  0  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
			//USER
			//USER Write:
			//USER ----------+-----------------------+
			//USER           |                       |
			//USER           |         ODT           |
			//USER Write To  +-----------------------+
			//USER   Rank    |  3  |  2  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
			//USER     0     |  0  |  1  |  0  |  1  |
			//USER     1     |  1  |  0  |  1  |  0  |
			//USER     2     |  0  |  1  |  0  |  1  |
			//USER     3     |  1  |  0  |  1  |  0  |
			//USER ----------+-----+-----+-----+-----+
			switch(rank)
			{
				case 0:
					odt_mask_0 = 0x4;
					odt_mask_1 = 0x5;
				break;
				case 1:
					odt_mask_0 = 0x8;
					odt_mask_1 = 0xA;
				break;
				case 2:
					odt_mask_0 = 0x1;
					odt_mask_1 = 0x5;
				break;
				case 3:
					odt_mask_0 = 0x2;
					odt_mask_1 = 0xA;
				break;
			}
		}
	}
	else
	{
		odt_mask_0 = 0x0;
		odt_mask_1 = 0x0;
	}

#if ADVANCED_ODT_CONTROL
	// odt_mask_0 = read
	// odt_mask_1 = write
	odt_mask_0  = (CFG_READ_ODT_CHIP  >> (RW_MGR_MEM_ODT_WIDTH * rank));
	odt_mask_1  = (CFG_WRITE_ODT_CHIP >> (RW_MGR_MEM_ODT_WIDTH * rank));
	odt_mask_0 &= ((1 << RW_MGR_MEM_ODT_WIDTH) - 1);
	odt_mask_1 &= ((1 << RW_MGR_MEM_ODT_WIDTH) - 1);
#endif

#if MRS_MIRROR_PING_PONG_ATSO
	// See set_cs_and_odt_mask_for_ping_pong_atso
	cs_and_odt_mask = 
			(0xFC) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
#else
	if(RDIMM && RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 2 
	   && RW_MGR_MEM_CHIP_SELECT_WIDTH == 4 && RW_MGR_MEM_NUMBER_OF_RANKS == 2) {
		//USER See RDIMM special case above
		cs_and_odt_mask = 
			(0xFF & ~(1 << (2*rank))) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
	} else if (LRDIMM) {
#if LRDIMM
		// USER LRDIMM special cases - When RM=2, CS[2] is remapped to A[16] so skip it,
		// USER and when RM=4, CS[3:2] are remapped to A[17:16] so skip them both.
		alt_u32 lrdimm_rank = 0;
		alt_u32 lrdimm_rank_mask = 0;

		//USER When rank multiplication is active, the remapped CS pins must be forced low
		//USER instead of high for proper targetted RTT_NOM programming.
		if (LRDIMM_RANK_MULTIPLICATION_FACTOR == 2) {
			// USER Mask = CS[5:0] = 011011
			lrdimm_rank_mask = (0x3 | (0x3 << 3));
		} else if (LRDIMM_RANK_MULTIPLICATION_FACTOR == 4) {
			// USER Mask = CS[7:0] = 00110011
			lrdimm_rank_mask = (0x3 | (0x3 << 4));
		}

		// USER Handle LRDIMM cases where Rank multiplication may be active
		if (((RW_MGR_MEM_CHIP_SELECT_WIDTH/RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM) == 1)) {
			// USER Single-DIMM case
			lrdimm_rank = ~(1 << rank);
		} else if ((RW_MGR_MEM_CHIP_SELECT_WIDTH/RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM) == 2) {
			if (rank < (RW_MGR_MEM_NUMBER_OF_RANKS >> 1)) {
			// USER Dual-DIMM case, accessing first slot
				lrdimm_rank = ~(1 << rank);
			} else {
			// USER Dual-DIMM case, accessing second slot
				lrdimm_rank = ~(1 << (rank + RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM - (RW_MGR_MEM_NUMBER_OF_RANKS>>1)));
			}
		}
		cs_and_odt_mask = 
			(lrdimm_rank_mask & lrdimm_rank) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
#endif // LRDIMM
	} else {
		cs_and_odt_mask = 
			(0xFF & ~(1 << rank)) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
	}
#endif

	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, cs_and_odt_mask);
}
#else
#if DDR2
void set_rank_and_odt_mask(alt_u32 rank, alt_u32 odt_mode)
{
	alt_u32 odt_mask_0 = 0;
	alt_u32 odt_mask_1 = 0;
	alt_u32 cs_and_odt_mask;

	if(odt_mode == RW_MGR_ODT_MODE_READ_WRITE)
	{
		if(RW_MGR_MEM_NUMBER_OF_RANKS == 1) { 
			//USER 1 Rank
			//USER Read: ODT = 0
			//USER Write: ODT = 1
			odt_mask_0 = 0x0;
			odt_mask_1 = 0x1;
		} else if(RW_MGR_MEM_NUMBER_OF_RANKS == 2) { 
			//USER 2 Ranks
			if(RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 1 ||
			   (RDIMM && RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 2 
			   && RW_MGR_MEM_CHIP_SELECT_WIDTH == 4)) {
				//USER - Dual-Slot , Single-Rank (1 chip-select per DIMM)
				//USER OR
				//USER - RDIMM, 4 total CS (2 CS per DIMM) means 2 DIMM
				//USER Since MEM_NUMBER_OF_RANKS is 2 they are both single rank
				//USER with 2 CS each (special for RDIMM)
				//USER Read/Write: Turn on ODT on the opposite rank
				odt_mask_0 = 0x3 & ~(1 << rank);
				odt_mask_1 = 0x3 & ~(1 << rank);
			} else {
				//USER - Single-Slot , Dual-rank DIMMs (2 chip-selects per DIMM)
				//USER Read: Turn on ODT off on all ranks
				//USER Write: Turn on ODT on active rank
				odt_mask_0 = 0x0;
				odt_mask_1 = 0x3 & (1 << rank);
			}
		} else { 
			//USER 4 Ranks
			//USER Read/Write:
			//USER -----------+-----------------------+
			//USER            |                       |
			//USER            |         ODT           |
			//USER Read/Write |                       |
			//USER   From     +-----------------------+
			//USER   Rank     |  3  |  2  |  1  |  0  |
			//USER -----------+-----+-----+-----+-----+
			//USER     0      |  0  |  1  |  0  |  0  |
			//USER     1      |  1  |  0  |  0  |  0  |
			//USER     2      |  0  |  0  |  0  |  1  |
			//USER     3      |  0  |  0  |  1  |  0  |
			//USER -----------+-----+-----+-----+-----+
			switch(rank)
			{
				case 0:
					odt_mask_0 = 0x4;
					odt_mask_1 = 0x4;
				break;
				case 1:
					odt_mask_0 = 0x8;
					odt_mask_1 = 0x8;
				break;
				case 2:
					odt_mask_0 = 0x1;
					odt_mask_1 = 0x1;
				break;
				case 3:
					odt_mask_0 = 0x2;
					odt_mask_1 = 0x2;
				break;
			}
		}
	}
	else
	{
		odt_mask_0 = 0x0;
		odt_mask_1 = 0x0;
	}

	if(RDIMM && RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM == 2 
	   && RW_MGR_MEM_CHIP_SELECT_WIDTH == 4 && RW_MGR_MEM_NUMBER_OF_RANKS == 2) {
		//USER See RDIMM/LRDIMM special case above
		cs_and_odt_mask = 
			(0xFF & ~(1 << (2*rank))) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
	} else {
		cs_and_odt_mask = 
			(0xFF & ~(1 << rank)) |
			((0xFF & odt_mask_0) << 8) |
			((0xFF & odt_mask_1) << 16);
	}

	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, cs_and_odt_mask);
}
#else // QDRII and RLDRAMx
void set_rank_and_odt_mask(alt_u32 rank, alt_u32 odt_mode)
{
	alt_u32 cs_and_odt_mask = 
		(0xFF & ~(1 << rank));

	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, cs_and_odt_mask);
}
#endif
#endif

//USER Given a rank, select the set of shadow registers that is responsible for the
//USER delays of such rank, so that subsequent SCC updates will go to those shadow
//USER registers. 
void select_shadow_regs_for_update (alt_u32 rank, alt_u32 group, alt_u32 update_scan_chains)
{
#if USE_SHADOW_REGS
	alt_u32 rank_one_hot = (0xFF & (1 << rank));
	
	//USER Assert afi_rrank and afi_wrank. These signals ultimately drive
	//USER the read/write rank select signals which select the shadow register.
	IOWR_32DIRECT (RW_MGR_SET_ACTIVE_RANK, 0, rank_one_hot);
	
	//USER Cause the SCC manager to switch its register file, which is used as
	//USER local cache of the various dtap/ptap settings. There's one register file
	//USER per shadow register set.
	IOWR_32DIRECT (SCC_MGR_ACTIVE_RANK, 0, rank_one_hot);
		
	if (update_scan_chains) {
		alt_u32 i;
	
		//USER On the read side, a memory read is required because the read rank
		//USER select signal (as well as the postamble delay chain settings) is clocked
		//USER into the periphery by the postamble signal. Simply asserting afi_rrank
		//USER is not enough. If update_scc_regfile is not set, we assume there'll be a 
		//USER subsequent read that'll handle this.
		for (i = 0; i < RW_MGR_MEM_NUMBER_OF_RANKS; ++i) {
			
			//USER The dummy read can go to any non-skipped rank.
			//USER Skipped ranks are uninitialized and their banks are un-activated.
			//USER Accessing skipped ranks can lead to bad behavior.
			if (! param->skip_ranks[i]) {
			
				set_rank_and_odt_mask(i, RW_MGR_ODT_MODE_READ_WRITE);
				
				// must re-assert afi_wrank/afi_rrank prior to issuing read 
				// because set_rank_and_odt_mask may have changed the signals.
				IOWR_32DIRECT (RW_MGR_SET_ACTIVE_RANK, 0, rank_one_hot);
				
				IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x10);
				IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_READ_B2B_WAIT1);

				IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x10);
				IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_READ_B2B_WAIT2);
				
				IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x0);
				IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_READ_B2B);
			
				IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x0);
				IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_READ_B2B);
			
				IOWR_32DIRECT (RW_MGR_RUN_ALL_GROUPS, 0, __RW_MGR_READ_B2B);
				
				//USER The dummy read above may cause the DQS enable signal to be stuck high.
				//USER The following corrects this.
				IOWR_32DIRECT (RW_MGR_RUN_ALL_GROUPS, 0, __RW_MGR_CLEAR_DQS_ENABLE);				
				
				set_rank_and_odt_mask(i, RW_MGR_ODT_MODE_OFF);
				
				break;
			}
		}
		
		//USER Reset the fifos to get pointers to known state 
		IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);
		IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);	
	
		//USER On the write side the afi_wrank signal eventually propagates to the I/O 
		//USER through the write datapath. We need to make sure we wait long enough for
		//USER this to happen. The operations above should be enough, hence no extra delay
		//USER inserted here.
	
		//USER Make sure the data in the I/O scan chains are in-sync with the register
		//USER file inside the SCC manager. If we don't do this, a subsequent SCC_UPDATE
		//USER may cause stale data for the other shadow register to be loaded. This must
		//USER be done for every scan chain of the current group. Note that in shadow
		//USER register mode, the SCC_UPDATE signal is per-group.
		IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
		
		IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, group);
		IOWR_32DIRECT (SCC_MGR_DQS_IO_ENA, 0, 0);
		
		for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
			IOWR_32DIRECT (SCC_MGR_DQ_ENA, 0, i);
		}
		for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
			IOWR_32DIRECT (SCC_MGR_DM_ENA, 0, i);
		}
	}

	//USER Map the rank to its shadow reg
	select_curr_shadow_reg_using_rank(rank);
#endif
}

#if HHP_HPS
void scc_mgr_initialize(void)
{
	// Clear register file for HPS
	// 16 (2^4) is the size of the full register file in the scc mgr:
	//	RFILE_DEPTH = log2(MEM_DQ_PER_DQS + 1 + MEM_DM_PER_DQS + MEM_IF_READ_DQS_WIDTH - 1) + 1;
	alt_u32 i;
	for (i = 0; i < 16; i++) {
		DPRINT(1, "Clearing SCC RFILE index %lu", i);
		IOWR_32DIRECT(SCC_MGR_HHP_RFILE, i << 2, 0);
	}
}
#endif

inline void scc_mgr_set_dqs_bus_in_delay(alt_u32 read_group, alt_u32 delay)
{
	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQS_IN_DELAY(read_group, delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][read_group].dqs_bus_in_delay, delay);

}

static inline void scc_mgr_set_dqs_io_in_delay(alt_u32 write_group, alt_u32 delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQS_IO_IN_DELAY(delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].dqs_io_in_delay, delay);

}

static inline void scc_mgr_set_dqs_en_phase(alt_u32 read_group, alt_u32 phase)
{
	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQS_EN_PHASE(read_group, phase);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][read_group].dqs_en_phase, phase);

}

void scc_mgr_set_dqs_en_phase_all_ranks (alt_u32 read_group, alt_u32 phase)
{
	alt_u32 r;
	alt_u32 update_scan_chains;
		
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		//USER although the h/w doesn't support different phases per shadow register,
		//USER for simplicity our scc manager modeling keeps different phase settings per 
		//USER shadow reg, and it's important for us to keep them in sync to match h/w.
		//USER for efficiency, the scan chain update should occur only once to sr0.
		update_scan_chains = (r == 0) ? 1 : 0;
		
		select_shadow_regs_for_update(r, read_group, update_scan_chains);
		scc_mgr_set_dqs_en_phase(read_group, phase);
		
		if (update_scan_chains) {
			IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, read_group);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		}
	}
}


static inline void scc_mgr_set_dqdqs_output_phase(alt_u32 write_group, alt_u32 phase)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	#if CALIBRATE_BIT_SLIPS
	alt_u32 num_fr_slips = 0;
	while (phase > IO_DQDQS_OUT_PHASE_MAX) {
		phase -= IO_DLL_CHAIN_LENGTH;
		num_fr_slips++;
	}
	IOWR_32DIRECT (PHY_MGR_FR_SHIFT, write_group*4, num_fr_slips);
#endif
	
	// Load the setting in the SCC manager
	WRITE_SCC_DQDQS_OUT_PHASE(write_group, phase);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].dqdqs_out_phase, phase);

}

void scc_mgr_set_dqdqs_output_phase_all_ranks (alt_u32 write_group, alt_u32 phase)
{
	alt_u32 r;
	alt_u32 update_scan_chains;
		
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		//USER although the h/w doesn't support different phases per shadow register,
		//USER for simplicity our scc manager modeling keeps different phase settings per 
		//USER shadow reg, and it's important for us to keep them in sync to match h/w.
		//USER for efficiency, the scan chain update should occur only once to sr0.
		update_scan_chains = (r == 0) ? 1 : 0;
		
		select_shadow_regs_for_update(r, write_group, update_scan_chains);
		scc_mgr_set_dqdqs_output_phase(write_group, phase);
		
		if (update_scan_chains) {
			IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, write_group);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		}
	}
}


static inline void scc_mgr_set_dqs_en_delay(alt_u32 read_group, alt_u32 delay)
{
	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQS_EN_DELAY(read_group, delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][read_group].dqs_en_delay, delay);

}

void scc_mgr_set_dqs_en_delay_all_ranks (alt_u32 read_group, alt_u32 delay)
{
	alt_u32 r;
		
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
	
		select_shadow_regs_for_update(r, read_group, 0);
		
		scc_mgr_set_dqs_en_delay(read_group, delay);

		IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, read_group);
		
#if !USE_SHADOW_REGS		
		// In shadow register mode, the T11 settings are stored in registers
		// in the core, which are updated by the DQS_ENA signals. Not issuing
		// the SCC_MGR_UPD command allows us to save lots of rank switching
		// overhead, by calling select_shadow_regs_for_update with update_scan_chains
		// set to 0.
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
#endif		
	}	
}

static void scc_mgr_set_oct_out1_delay(alt_u32 write_group, alt_u32 delay)
{
	alt_u32 read_group;

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	// Load the setting in the SCC manager
	// Although OCT affects only write data, the OCT delay is controlled by the DQS logic block
	// which is instantiated once per read group. For protocols where a write group consists
	// of multiple read groups, the setting must be set multiple times.
	for (read_group = write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
		 read_group < (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
		 ++read_group) {
		 
		WRITE_SCC_OCT_OUT1_DELAY(read_group, delay);
	}

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].oct_out_delay1, delay);

}

static void scc_mgr_set_oct_out2_delay(alt_u32 write_group, alt_u32 delay)
{
	alt_u32 read_group;

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	// Load the setting in the SCC manager
	// Although OCT affects only write data, the OCT delay is controlled by the DQS logic block
	// which is instantiated once per read group. For protocols where a write group consists
	// of multiple read groups, the setting must be set multiple times.
	for (read_group = write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
		 read_group < (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
		 ++read_group) {
	
		WRITE_SCC_OCT_OUT2_DELAY(read_group, delay);
	}

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].oct_out_delay2, delay);

}

static inline void scc_mgr_set_dqs_bypass(alt_u32 write_group, alt_u32 bypass)
{
	// Load the setting in the SCC manager
	WRITE_SCC_DQS_BYPASS(write_group, bypass);
}

inline void scc_mgr_set_dq_out1_delay(alt_u32 write_group, alt_u32 dq_in_group, alt_u32 delay)
{
#if ENABLE_TCL_DEBUG || ENABLE_ASSERT
	alt_u32 dq = write_group*RW_MGR_MEM_DQ_PER_WRITE_DQS + dq_in_group;
#endif

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQ_OUT1_DELAY(dq_in_group, delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dq_settings[curr_shadow_reg][dq].dq_out_delay1, delay);

}

inline void scc_mgr_set_dq_out2_delay(alt_u32 write_group, alt_u32 dq_in_group, alt_u32 delay)
{
#if ENABLE_TCL_DEBUG || ENABLE_ASSERT
	alt_u32 dq = write_group*RW_MGR_MEM_DQ_PER_WRITE_DQS + dq_in_group;
#endif

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQ_OUT2_DELAY(dq_in_group, delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dq_settings[curr_shadow_reg][dq].dq_out_delay2, delay);

}

inline void scc_mgr_set_dq_in_delay(alt_u32 write_group, alt_u32 dq_in_group, alt_u32 delay)
{
#if ENABLE_TCL_DEBUG || ENABLE_ASSERT
	alt_u32 dq = write_group*RW_MGR_MEM_DQ_PER_WRITE_DQS + dq_in_group;
#endif

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQ_IN_DELAY(dq_in_group, delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dq_settings[curr_shadow_reg][dq].dq_in_delay, delay);

}

static inline void scc_mgr_set_dq_bypass(alt_u32 write_group, alt_u32 dq_in_group, alt_u32 bypass)
{
	// Load the setting in the SCC manager
	WRITE_SCC_DQ_BYPASS(dq_in_group, bypass);
}

static inline void scc_mgr_set_rfifo_mode(alt_u32 write_group, alt_u32 dq_in_group, alt_u32 mode)
{
	// Load the setting in the SCC manager
	WRITE_SCC_RFIFO_MODE(dq_in_group, mode);
}

static inline void scc_mgr_set_hhp_extras(void)
{
	// Load the fixed setting in the SCC manager
	// bits: 0:0 = 1'b1   - dqs bypass
	// bits: 1:1 = 1'b1   - dq bypass
	// bits: 4:2 = 3'b001   - rfifo_mode
	// bits: 6:5 = 2'b01  - rfifo clock_select
	// bits: 7:7 = 1'b0  - separate gating from ungating setting
	// bits: 8:8 = 1'b0  - separate OE from Output delay setting
	alt_u32 value = (0<<8) | (0<<7) | (1<<5) | (1<<2) | (1<<1) | (1<<0);
	WRITE_SCC_HHP_EXTRAS(value);
}

static inline void scc_mgr_set_hhp_dqse_map(void)
{
	// Load the fixed setting in the SCC manager
	WRITE_SCC_HHP_DQSE_MAP(0);
}

static inline void scc_mgr_set_dqs_out1_delay(alt_u32 write_group, alt_u32 delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQS_IO_OUT1_DELAY(delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].dqs_out_delay1, delay);

}

static inline void scc_mgr_set_dqs_out2_delay(alt_u32 write_group, alt_u32 delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	// Load the setting in the SCC manager
	WRITE_SCC_DQS_IO_OUT2_DELAY(delay);

	// Make the setting in the TCL report
	TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].dqs_out_delay2, delay);

}

inline void scc_mgr_set_dm_out1_delay(alt_u32 write_group, alt_u32 dm, alt_u32 delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dm < RW_MGR_NUM_DM_PER_WRITE_GROUP);

	// Load the setting in the SCC manager
	WRITE_SCC_DM_IO_OUT1_DELAY(dm, delay);

	// Make the setting in the TCL report

	if (RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP > 0)
	{
		TCLRPT_SET(debug_cal_report->cal_dm_settings[curr_shadow_reg][write_group][dm].dm_out_delay1, delay);
	}
}

inline void scc_mgr_set_dm_out2_delay(alt_u32 write_group, alt_u32 dm, alt_u32 delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dm < RW_MGR_NUM_DM_PER_WRITE_GROUP);

	// Load the setting in the SCC manager
	WRITE_SCC_DM_IO_OUT2_DELAY(dm, delay);

	// Make the setting in the TCL report

	if (RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP > 0)
	{
		TCLRPT_SET(debug_cal_report->cal_dm_settings[curr_shadow_reg][write_group][dm].dm_out_delay2, delay);
	}
}

static inline void scc_mgr_set_dm_in_delay(alt_u32 write_group, alt_u32 dm, alt_u32 delay)
{
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	ALTERA_ASSERT(dm < RW_MGR_NUM_DM_PER_WRITE_GROUP);

	// Load the setting in the SCC manager
	WRITE_SCC_DM_IO_IN_DELAY(dm, delay);

	// Make the setting in the TCL report

	if (RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP > 0)
	{
		TCLRPT_SET(debug_cal_report->cal_dm_settings[curr_shadow_reg][write_group][dm].dm_in_delay, delay);
	}
}

static inline void scc_mgr_set_dm_bypass(alt_u32 write_group, alt_u32 dm, alt_u32 bypass)
{
	// Load the setting in the SCC manager
	WRITE_SCC_DM_BYPASS(dm, bypass);
}

//USER Zero all DQS config
// TODO: maybe rename to scc_mgr_zero_dqs_config (or something)
void scc_mgr_zero_all (void)
{
	alt_u32 i, r;
		
	//USER Zero all DQS config settings, across all groups and all shadow registers
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
	
		// Strictly speaking this should be called once per group to make
		// sure each group's delay chain is refreshed from the SCC register file,
		// but since we're resetting all delay chains anyway, we can save some
		// runtime by calling select_shadow_regs_for_update just once to switch
		// rank.
		select_shadow_regs_for_update(r, 0, 1);

		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			// The phases actually don't exist on a per-rank basis, but there's
			// no harm updating them several times, so let's keep the code simple.
			scc_mgr_set_dqs_bus_in_delay(i, IO_DQS_IN_RESERVE);
			scc_mgr_set_dqs_en_phase(i, 0);
			scc_mgr_set_dqs_en_delay(i, 0);
		}

		for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
			scc_mgr_set_dqdqs_output_phase(i, 0);
#if ARRIAV || CYCLONEV
			// av/cv don't have out2
			scc_mgr_set_oct_out1_delay(i, IO_DQS_OUT_RESERVE);
#else
			scc_mgr_set_oct_out1_delay(i, 0);
			scc_mgr_set_oct_out2_delay(i, IO_DQS_OUT_RESERVE);
#endif
		}

		//USER multicast to all DQS group enables
		IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, 0xff);
		
#if USE_SHADOW_REGS		
		//USER in shadow-register mode, SCC_UPDATE is done on a per-group basis
		//USER unless we explicitly ask for a multicast via the group counter
		IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, 0xFF);
#endif		
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}
}

void scc_set_bypass_mode(alt_u32 write_group, alt_u32 mode)
{
	// mode = 0 : Do NOT bypass - Half Rate Mode
	// mode = 1 : Bypass - Full Rate Mode

#if !HHP_HPS
	alt_u32 i;
#endif

#if HHP_HPS
	// only need to set once for all groups, pins, dq, dqs, dm
	if (write_group == 0) {
		DPRINT(1, "Setting HHP Extras");
		scc_mgr_set_hhp_extras();
		DPRINT(1, "Done Setting HHP Extras");
	}
#endif
	
#if !HHP_HPS
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++)
	{
		scc_mgr_set_dq_bypass(write_group, i, mode);
		scc_mgr_set_rfifo_mode(write_group, i, mode);
	}
#endif

	//USER multicast to all DQ enables 
	IOWR_32DIRECT (SCC_MGR_DQ_ENA, 0, 0xff);

#if !HHP_HPS
	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++)
	{
		scc_mgr_set_dm_bypass(write_group, i, mode);
	}
#endif

	IOWR_32DIRECT (SCC_MGR_DM_ENA, 0, 0xff);

#if !HHP_HPS
	scc_mgr_set_dqs_bypass(write_group, mode);
#endif

	//USER update current DQS IO enable
	IOWR_32DIRECT (SCC_MGR_DQS_IO_ENA, 0, 0);

	//USER update the DQS logic
	IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, write_group);

	//USER hit update
	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
}

// Moving up to avoid warnings
void scc_mgr_load_dqs_for_write_group (alt_u32 write_group)
{
	alt_u32 read_group;
	
	// Although OCT affects only write data, the OCT delay is controlled by the DQS logic block
	// which is instantiated once per read group. For protocols where a write group consists
	// of multiple read groups, the setting must be scanned multiple times.
	for (read_group = write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
		 read_group < (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
		 ++read_group) {
		 
		IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, read_group);
	}
}

void scc_mgr_zero_group (alt_u32 write_group, alt_u32 test_begin, alt_32 out_only)
{
	alt_u32 i, r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {	
		
		select_shadow_regs_for_update(r, write_group, 1);

		//USER Zero all DQ config settings
		for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++)
		{
			scc_mgr_set_dq_out1_delay(write_group, i, 0);
			scc_mgr_set_dq_out2_delay(write_group, i, IO_DQ_OUT_RESERVE);
			if (!out_only) {
				scc_mgr_set_dq_in_delay(write_group, i, 0);
			}
		}

		//USER multicast to all DQ enables
		IOWR_32DIRECT (SCC_MGR_DQ_ENA, 0, 0xff);

		//USER Zero all DM config settings 
		for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++)
		{
			if (!out_only) {
				// Do we really need this?
				scc_mgr_set_dm_in_delay(write_group, i, 0);
			}
			scc_mgr_set_dm_out1_delay(write_group, i, 0);
			scc_mgr_set_dm_out2_delay(write_group, i, IO_DM_OUT_RESERVE);
		}

		//USER multicast to all DM enables
		IOWR_32DIRECT (SCC_MGR_DM_ENA, 0, 0xff);

		//USER zero all DQS io settings 
		if (!out_only) {
			scc_mgr_set_dqs_io_in_delay(write_group, 0);
		}
#if ARRIAV || CYCLONEV
		// av/cv don't have out2
		scc_mgr_set_dqs_out1_delay(write_group, IO_DQS_OUT_RESERVE);
		scc_mgr_set_oct_out1_delay(write_group, IO_DQS_OUT_RESERVE);
		scc_mgr_load_dqs_for_write_group (write_group);
#else
		scc_mgr_set_dqs_out1_delay(write_group, 0);
		scc_mgr_set_dqs_out2_delay(write_group, IO_DQS_OUT_RESERVE);
		scc_mgr_set_oct_out1_delay(write_group, 0);
		scc_mgr_set_oct_out2_delay(write_group, IO_DQS_OUT_RESERVE);
		scc_mgr_load_dqs_for_write_group (write_group);
#endif

		//USER multicast to all DQS IO enables (only 1)
		IOWR_32DIRECT (SCC_MGR_DQS_IO_ENA, 0, 0);

#if USE_SHADOW_REGS		
		//USER in shadow-register mode, SCC_UPDATE is done on a per-group basis
		//USER unless we explicitly ask for a multicast via the group counter
		IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, 0xFF);
#endif				
		//USER hit update to zero everything 
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}
}

//USER load up dqs config settings 

void scc_mgr_load_dqs (alt_u32 dqs)
{
	IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, dqs);
}


//USER load up dqs io config settings 

void scc_mgr_load_dqs_io (void)
{
	IOWR_32DIRECT (SCC_MGR_DQS_IO_ENA, 0, 0);
}

//USER load up dq config settings 

void scc_mgr_load_dq (alt_u32 dq_in_group)
{
	IOWR_32DIRECT (SCC_MGR_DQ_ENA, 0, dq_in_group);
}

//USER load up dm config settings 

void scc_mgr_load_dm (alt_u32 dm)
{
	IOWR_32DIRECT (SCC_MGR_DM_ENA, 0, dm);
}

//USER apply and load a particular input delay for the DQ pins in a group
//USER group_bgn is the index of the first dq pin (in the write group)

void scc_mgr_apply_group_dq_in_delay (alt_u32 write_group, alt_u32 group_bgn, alt_u32 delay)
{
	alt_u32 i, p;

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		scc_mgr_set_dq_in_delay(write_group, p, delay);
		scc_mgr_load_dq (p);
	}
}

//USER apply and load a particular output delay for the DQ pins in a group

void scc_mgr_apply_group_dq_out1_delay (alt_u32 write_group, alt_u32 group_bgn, alt_u32 delay1)
{
	alt_u32 i, p;

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		scc_mgr_set_dq_out1_delay(write_group, i, delay1);
		scc_mgr_load_dq (i);
	}
}

void scc_mgr_apply_group_dq_out2_delay (alt_u32 write_group, alt_u32 group_bgn, alt_u32 delay2)
{
	alt_u32 i, p;

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		scc_mgr_set_dq_out2_delay(write_group, i, delay2);
		scc_mgr_load_dq (i);
	}
}

//USER apply and load a particular output delay for the DM pins in a group

void scc_mgr_apply_group_dm_out1_delay (alt_u32 write_group, alt_u32 delay1)
{
	alt_u32 i;

	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		scc_mgr_set_dm_out1_delay(write_group, i, delay1);
		scc_mgr_load_dm (i);
	}
}


//USER apply and load delay on both DQS and OCT out1
void scc_mgr_apply_group_dqs_io_and_oct_out1 (alt_u32 write_group, alt_u32 delay)
{
	scc_mgr_set_dqs_out1_delay(write_group, delay);
	scc_mgr_load_dqs_io ();

	scc_mgr_set_oct_out1_delay(write_group, delay);
	scc_mgr_load_dqs_for_write_group (write_group);
}

//USER apply and load delay on both DQS and OCT out2
void scc_mgr_apply_group_dqs_io_and_oct_out2 (alt_u32 write_group, alt_u32 delay)
{
	scc_mgr_set_dqs_out2_delay(write_group, delay);
	scc_mgr_load_dqs_io ();

	scc_mgr_set_oct_out2_delay(write_group, delay);
	scc_mgr_load_dqs_for_write_group (write_group);
}

//USER set delay on both DQS and OCT out1 by incrementally changing
//USER the settings one dtap at a time towards the target value, to avoid
//USER breaking the lock of the DLL/PLL on the memory device.
void scc_mgr_set_group_dqs_io_and_oct_out1_gradual (alt_u32 write_group, alt_u32 delay)
{
	alt_u32 d = READ_SCC_DQS_IO_OUT1_DELAY();
	
	while (d > delay) {
		--d;
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, d);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		if (QDRII)
		{
			rw_mgr_mem_dll_lock_wait();
		}
	}
	while (d < delay) {
		++d;
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, d);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		if (QDRII)
		{
			rw_mgr_mem_dll_lock_wait();
		}
	}	
}

//USER set delay on both DQS and OCT out2 by incrementally changing
//USER the settings one dtap at a time towards the target value, to avoid
//USER breaking the lock of the DLL/PLL on the memory device.
void scc_mgr_set_group_dqs_io_and_oct_out2_gradual (alt_u32 write_group, alt_u32 delay)
{
	alt_u32 d = READ_SCC_DQS_IO_OUT2_DELAY();
	
	while (d > delay) {
		--d;
		scc_mgr_apply_group_dqs_io_and_oct_out2 (write_group, d);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		if (QDRII)
		{
			rw_mgr_mem_dll_lock_wait();
		}
	}
	while (d < delay) {
		++d;
		scc_mgr_apply_group_dqs_io_and_oct_out2 (write_group, d);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		if (QDRII)
		{
			rw_mgr_mem_dll_lock_wait();
		}
	}	
}

//USER apply a delay to the entire output side: DQ, DM, DQS, OCT 

void scc_mgr_apply_group_all_out_delay (alt_u32 write_group, alt_u32 group_bgn, alt_u32 delay)
{
	//USER dq shift 

	scc_mgr_apply_group_dq_out1_delay (write_group, group_bgn, delay);

	//USER dm shift 

	scc_mgr_apply_group_dm_out1_delay (write_group, delay);

	//USER dqs and oct shift 

	scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, delay);
}

//USER apply a delay to the entire output side (DQ, DM, DQS, OCT) and to all ranks
void scc_mgr_apply_group_all_out_delay_all_ranks (alt_u32 write_group, alt_u32 group_bgn, alt_u32 delay)
{
	alt_u32 r;
		
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
	
		select_shadow_regs_for_update(r, write_group, 1);

		scc_mgr_apply_group_all_out_delay (write_group, group_bgn, delay);
		
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}
}

//USER apply a delay to the entire output side: DQ, DM, DQS, OCT 

void scc_mgr_apply_group_all_out_delay_add (alt_u32 write_group, alt_u32 group_bgn, alt_u32 delay)
{
	alt_u32 i, p, new_delay;

	//USER dq shift 

	for (i = 0, p = group_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {

		new_delay = READ_SCC_DQ_OUT2_DELAY(i);
		new_delay += delay;

		if (new_delay > IO_IO_OUT2_DELAY_MAX) {
			DPRINT(1, "%s(%lu, %lu, %lu) DQ[%lu,%lu]: %lu > %lu => %lu",
			       __func__, write_group, group_bgn, delay, i, p,
			       new_delay, (long unsigned int)IO_IO_OUT2_DELAY_MAX, (long unsigned int)IO_IO_OUT2_DELAY_MAX);
			new_delay = IO_IO_OUT2_DELAY_MAX;
		}

		scc_mgr_set_dq_out2_delay(write_group, i, new_delay);
		scc_mgr_load_dq (i);
	}

	//USER dm shift 

	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		new_delay = READ_SCC_DM_IO_OUT2_DELAY(i);
		new_delay += delay;

		if (new_delay > IO_IO_OUT2_DELAY_MAX) {
			DPRINT(1, "%s(%lu, %lu, %lu) DM[%lu]: %lu > %lu => %lu",
			       __func__, write_group, group_bgn, delay, i, 
			       new_delay, (long unsigned int)IO_IO_OUT2_DELAY_MAX, (long unsigned int)IO_IO_OUT2_DELAY_MAX);
			new_delay = IO_IO_OUT2_DELAY_MAX;
		}

		scc_mgr_set_dm_out2_delay(write_group, i, new_delay);
		scc_mgr_load_dm (i);
	}

	//USER dqs shift 

	new_delay = READ_SCC_DQS_IO_OUT2_DELAY();
	new_delay += delay;

	if (new_delay > IO_IO_OUT2_DELAY_MAX) {
		DPRINT(1, "%s(%lu, %lu, %lu) DQS: %lu > %d => %d; adding %lu to OUT1",
		       __func__, write_group, group_bgn, delay,
		       new_delay, IO_IO_OUT2_DELAY_MAX, IO_IO_OUT2_DELAY_MAX,
			new_delay - IO_IO_OUT2_DELAY_MAX);
		scc_mgr_set_dqs_out1_delay(write_group, new_delay - IO_IO_OUT2_DELAY_MAX);
		new_delay = IO_IO_OUT2_DELAY_MAX;
	}

	scc_mgr_set_dqs_out2_delay(write_group, new_delay);
	scc_mgr_load_dqs_io ();

	//USER oct shift 

	new_delay = READ_SCC_OCT_OUT2_DELAY(write_group);
	new_delay += delay;

	if (new_delay > IO_IO_OUT2_DELAY_MAX) {
		DPRINT(1, "%s(%lu, %lu, %lu) DQS: %lu > %d => %d; adding %lu to OUT1",
		       __func__, write_group, group_bgn, delay,
		       new_delay, IO_IO_OUT2_DELAY_MAX, IO_IO_OUT2_DELAY_MAX,
			new_delay - IO_IO_OUT2_DELAY_MAX);
		scc_mgr_set_oct_out1_delay(write_group, new_delay - IO_IO_OUT2_DELAY_MAX);
		new_delay = IO_IO_OUT2_DELAY_MAX;
	}

	scc_mgr_set_oct_out2_delay(write_group, new_delay);
	scc_mgr_load_dqs_for_write_group (write_group);
}

//USER apply a delay to the entire output side (DQ, DM, DQS, OCT) and to all ranks
void scc_mgr_apply_group_all_out_delay_add_all_ranks (alt_u32 write_group, alt_u32 group_bgn, alt_u32 delay)
{
	alt_u32 r;
		
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
	
		select_shadow_regs_for_update(r, write_group, 1);
		
		scc_mgr_apply_group_all_out_delay_add (write_group, group_bgn, delay);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}
}

static inline void scc_mgr_spread_out2_delay_all_ranks (alt_u32 write_group, alt_u32 test_bgn)
{
#if STRATIXV || ARRIAVGZ
	alt_u32 found;
	alt_u32 i;
	alt_u32 p;
	alt_u32 d;
	alt_u32 r;
		
	const alt_u32 delay_step = IO_IO_OUT2_DELAY_MAX/(RW_MGR_MEM_DQ_PER_WRITE_DQS-1); /* we start at zero, so have one less dq to devide among */
	
	TRACE_FUNC("(%lu,%lu)", write_group, test_bgn);

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		select_shadow_regs_for_update(r, write_group, 1);
		for (i = 0, p = test_bgn, d = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++, d += delay_step) {
			DPRINT(1, "rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay: g=%lu r=%lu, i=%lu p=%lu d=%lu",
			       write_group, r, i, p, d);
			scc_mgr_set_dq_out2_delay(write_group, i, d);
			scc_mgr_load_dq (i);
		}
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}
#endif
}

#if DDR3
// optimization used to recover some slots in ddr3 inst_rom
// could be applied to other protocols if we wanted to
void set_jump_as_return(void)
{
	// to save space, we replace return with jump to special shared RETURN instruction
	// so we set the counter to large value so that we always jump
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0xFF);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_RETURN);

}
#endif

// should always use constants as argument to ensure all computations are performed at compile time
static inline void delay_for_n_mem_clocks(const alt_u32 clocks)
{
	alt_u32 afi_clocks;
	alt_u8 inner;
	alt_u8 outer;
	alt_u16 c_loop;

	TRACE_FUNC("clocks=%lu ... start", clocks);

	afi_clocks = (clocks + AFI_RATE_RATIO-1) / AFI_RATE_RATIO; /* scale (rounding up) to get afi clocks */

	// Note, we don't bother accounting for being off a little bit because of a few extra instructions in outer loops
	// Note, the loops have a test at the end, and do the test before the decrement, and so always perform the loop
	// 1 time more than the counter value
	if (afi_clocks == 0) {
		inner = outer = c_loop = 0;
	} else if (afi_clocks <= 0x100) {
		inner = afi_clocks-1;
		outer = 0;
		c_loop = 0;
	} else if (afi_clocks <= 0x10000) {
		inner = 0xff;
		outer = (afi_clocks-1) >> 8;
		c_loop = 0;
	} else {
		inner = 0xff;
		outer = 0xff;
		c_loop = (afi_clocks-1) >> 16;
	}

	// rom instructions are structured as follows:
	//
	//    IDLE_LOOP2: jnz cntr0, TARGET_A
	//    IDLE_LOOP1: jnz cntr1, TARGET_B
	//                return
	//
	// so, when doing nested loops, TARGET_A is set to IDLE_LOOP2, and TARGET_B is
	// set to IDLE_LOOP2 as well
	//
	// if we have no outer loop, though, then we can use IDLE_LOOP1 only, and set
	// TARGET_B to IDLE_LOOP1 and we skip IDLE_LOOP2 entirely
	//
	// a little confusing, but it helps save precious space in the inst_rom and sequencer rom
	// and keeps the delays more accurate and reduces overhead
	if (afi_clocks <= 0x100) {

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(inner));
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_IDLE_LOOP1);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE_LOOP1);

	} else {
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(inner));
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(outer));
	
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_IDLE_LOOP2);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_IDLE_LOOP2);

		// hack to get around compiler not being smart enough
		if (afi_clocks <= 0x10000) {
			// only need to run once
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE_LOOP2);
		} else {
			do {
				IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE_LOOP2);
			} while (c_loop-- != 0);
		}
	}

	TRACE_FUNC("clocks=%lu ... end", clocks);
}

// should always use constants as argument to ensure all computations are performed at compile time
static inline void delay_for_n_ns(const alt_u32 nanoseconds)
{
	TRACE_FUNC("nanoseconds=%lu ... end", nanoseconds);
	delay_for_n_mem_clocks((1000*nanoseconds) / (1000000/AFI_CLK_FREQ) * AFI_RATE_RATIO);
}

#if RLDRAM3
// Special routine to recover memory device from illegal state after
// ck/dk relationship is potentially violated.
static inline void recover_mem_device_after_ck_dqs_violation(void)
{
	//USER Issue MRS0 command. For some reason this is required once we
	//USER violate tCKDK. Without this all subsequent write tests will fail
	//USER even with known good delays.
   
   //USER Load MR0
	if ( RW_MGR_MEM_NUMBER_OF_RANKS == 1 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 2 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFC);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 4 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFC);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
		//USER Wait MRSC
		delay_for_n_mem_clocks(12);
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xF3);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_QUAD_RANK);
	}
	else {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
	}

	//USER Wait MRSC
	delay_for_n_mem_clocks(12);
}
#else
// Special routine to recover memory device from illegal state after
// ck/dqs relationship is violated.
static inline void recover_mem_device_after_ck_dqs_violation(void)
{
	// Current protocol doesn't require any special recovery
}
#endif

#if (LRDIMM && DDR3)
// Routine to program specific LRDIMM control words.
static void rw_mgr_lrdimm_rc_program(alt_u32 fscw, alt_u32 rc_addr, alt_u32 rc_val)
{
	alt_u32 i;
	const alt_u32 AC_BASE_CONTENT = __RW_MGR_CONTENT_ac_rdimm;
	//USER These values should be dynamically loaded instead of hard-coded
	const alt_u32 AC_ADDRESS_POSITION = 0x0;
	const alt_u32 AC_BANK_ADDRESS_POSITION = 0xD;
	alt_u32 ac_content;
	alt_u32 lrdimm_cs_msk = RW_MGR_RANK_NONE;

	TRACE_FUNC();

	//USER Turn on only CS0 and CS1 for each DIMM.
	for (i = 0; i < RW_MGR_MEM_CHIP_SELECT_WIDTH; i+= RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM)
	{
		lrdimm_cs_msk &= (~(3 << i));
	}

	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, lrdimm_cs_msk);

	// Program the fscw first (RC7), followed by the actual value
	for (i = 0; i < 2; i++)
	{
		alt_u32 addr;
		alt_u32 val;

		addr = (i == 0) ? 7 : rc_addr;
		val = (i == 0) ? fscw : rc_val;

		ac_content =
			AC_BASE_CONTENT |
			//USER Word address
			((addr & 0x7) << AC_ADDRESS_POSITION) |
			(((addr >> 3) & 0x1) << (AC_BANK_ADDRESS_POSITION + 2)) |
			//USER Configuration Word
			(((val >> 2) & 0x3) << (AC_BANK_ADDRESS_POSITION)) |
			((val & 0x3) << (AC_ADDRESS_POSITION + 3));

		//USER Override the AC row with the RDIMM command
		IOWR_32DIRECT(BASE_RW_MGR, 0x1C00 + (__RW_MGR_ac_rdimm << 2), ac_content);

		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_RDIMM_CMD);
	}

	// USER The following registers require a delay of tSTAB (6us) for proper functionality.
	// USER F0RC2, F0RC10, F0RC11, F1RC8, F1RC11-F1RC15
	// USER Note that it is only necessary to wait tSTAB after all of these
	// USER control words have been written, not after each one. Only F0RC0-F0RC15
	// USER are guaranteed to be written (and in order), but F1* are not so
	// USER wait after each.
	if (    ((fscw == 0) && ((rc_addr==2) || (rc_addr==10) || (rc_addr==11)))
		  || ((fscw == 1) && (rc_addr >= 8)))
	{
		delay_for_n_ns(6000);
	}
}
#endif
#if (RDIMM || LRDIMM) && DDR3
void rw_mgr_rdimm_initialize(void)
{
	alt_u32 i;
	alt_u32 conf_word;
#if RDIMM
	const alt_u32 AC_BASE_CONTENT = __RW_MGR_CONTENT_ac_rdimm;
	//USER These values should be dynamically loaded instead of hard-coded
	const alt_u32 AC_ADDRESS_POSITION = 0x0;
	const alt_u32 AC_BANK_ADDRESS_POSITION = 0xD;
	alt_u32 ac_content;
#endif

	TRACE_FUNC();
	
	//USER RDIMM registers are programmed by writing 16 configuration words
	//USER 1. An RDIMM command is a NOP with all CS asserted
	//USER 2. The 4-bit address of the configuration words is 
	//USER    * { mem_ba[2] , mem_a[2] , mem_a[1] , mem_a[0] }
	//USER 3. The 4-bit configuration word is
	//USER    * { mem_ba[1] , mem_ba[0] , mem_a[4] , mem_a[3] }

#if RDIMM
	//USER Turn on all ranks
	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, RW_MGR_RANK_ALL);
#endif

	for(i = 0; i < 16; i++)
	{
	

		if(i < 8)
		{
#if ENABLE_TCL_DEBUG && USE_USER_RDIMM_VALUE
			conf_word = (my_debug_data.command_parameters[0] >> (i * 4)) & 0xF;		
#else			
			conf_word = (RDIMM_CONFIG_WORD_LOW >> (i * 4)) & 0xF;	
#endif			
		}
		else
		{
#if ENABLE_TCL_DEBUG && USE_USER_RDIMM_VALUE	
			conf_word = (my_debug_data.command_parameters[1] >> ((i - 8) * 4)) & 0xF;	
#else			
			conf_word = (RDIMM_CONFIG_WORD_HIGH >> ((i - 8) * 4)) & 0xF;			
#endif		
		}	

#if RDIMM
		ac_content = 
			AC_BASE_CONTENT | 
			//USER Word address
			((i & 0x7) << AC_ADDRESS_POSITION) |
			(((i >> 3) & 0x1) << (AC_BANK_ADDRESS_POSITION + 2)) |
			//USER Configuration Word
			(((conf_word >> 2) & 0x3) << (AC_BANK_ADDRESS_POSITION)) |
			((conf_word & 0x3) << (AC_ADDRESS_POSITION + 3));

		//USER Override the AC row with the RDIMM command
		IOWR_32DIRECT(BASE_RW_MGR, 0x1C00 + (__RW_MGR_ac_rdimm << 2), ac_content);

		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_RDIMM_CMD);
		//USER When sending the RC2 or RC10 word, tSTAB time must elapse before the next command
		//USER is sent out. tSTAB is currently hard-coded to 6us.
		if((i == 2) || (i == 10))
		{
			//USER tSTAB = 6 us
			delay_for_n_ns(6000);
		}

#endif
#if LRDIMM
		// USER Program configuration word with FSCW set to zero.
		rw_mgr_lrdimm_rc_program(0, i, conf_word);
#endif
	}
}
#else
void rw_mgr_rdimm_initialize(void) { }
#endif

#if DDR3

#if (ADVANCED_ODT_CONTROL || LRDIMM)
alt_u32 ddr3_mirror_mrs_cmd(alt_u32 bit_vector) {
	// This function performs address mirroring of an AC ROM command, which
	// requires swapping the following DDR3 bits:
	//     A[3] <=> A[4]
	//     A[5] <=> A[6]
	//     A[7] <=> A[8]
	//    BA[0] <=>BA[1]
	// We assume AC_ROM_ENTRY = {BA[2:0], A[15:0]}.
	alt_u32 unchanged_bits;
	alt_u32 mask_a;
	alt_u32 mask_b;
	alt_u32 retval;

	unchanged_bits = (~(DDR3_AC_MIRR_MASK | (DDR3_AC_MIRR_MASK << 1))) & bit_vector;
	mask_a = DDR3_AC_MIRR_MASK & bit_vector;
	mask_b = (DDR3_AC_MIRR_MASK << 1) & bit_vector;

	retval = unchanged_bits | (mask_a << 1) | (mask_b >> 1);

	return retval;
}

void rtt_change_MRS1_MRS2_NOM_WR (alt_u32 prev_ac_mr , alt_u32 odt_ac_mr, alt_u32 mirr_on, alt_u32 mr_cmd ) {
	// This function updates the ODT-specific Mode Register bits (MRS1 or MRS2) in the AC ROM.
	// Parameters:  prev_ac_mr - Original, *un-mirrored* AC ROM Entry
	//              odt_ac_mr  - ODT bits to update (un-mirrored)
	//              mirr_on    - boolean flag indicating if the regular or mirrored entry is updated
	//              mr_cmd     - Mode register command (only MR1 and MR2 are supported for DDR3)
	alt_u32 new_ac_mr;
	alt_u32 ac_rom_entry = 0;
	alt_u32 ac_rom_mask;

	switch (mr_cmd) {
		case 1: {
			// USER MRS1 = RTT_NOM, RTT_DRV
			ac_rom_mask = DDR3_MR1_ODT_MASK;
			ac_rom_entry = mirr_on ? (0x1C00 | (__RW_MGR_ac_mrs1_mirr << 2))
			                       : (0x1C00 | (__RW_MGR_ac_mrs1 << 2));
		} break;
		case 2: {
			// USER MRS2 = RTT_WR
			ac_rom_mask = DDR3_MR2_ODT_MASK;
			ac_rom_entry = mirr_on ? (0x1C00 | (__RW_MGR_ac_mrs2_mirr << 2))
			                       : (0x1C00 | (__RW_MGR_ac_mrs2 << 2));
		} break;
	}

	// USER calculate new AC values and update ROM
	new_ac_mr  = odt_ac_mr;
	new_ac_mr |= (prev_ac_mr & ac_rom_mask);
	if (mirr_on) {
		new_ac_mr = ddr3_mirror_mrs_cmd(new_ac_mr);
	}
	IOWR_32DIRECT(BASE_RW_MGR, ac_rom_entry, new_ac_mr);
}
#endif //(ADVANCED_ODT_CONTROL || LRDIMM)

void rw_mgr_mem_initialize (void)
{
	alt_u32 r;

#if LRDIMM
	alt_u32 rtt_nom;
	alt_u32 rtt_drv;
	alt_u32 rtt_wr;
#endif // LRDIMM

	TRACE_FUNC();

	//USER The reset / cke part of initialization is broadcasted to all ranks
	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, RW_MGR_RANK_ALL);

	// Here's how you load register for a loop
	//USER Counters are located @ 0x800
	//USER Jump address are located @ 0xC00
	//USER For both, registers 0 to 3 are selected using bits 3 and 2, like in
	//USER 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	// I know this ain't pretty, but Avalon bus throws away the 2 least significant bits
	
	//USER start with memory RESET activated

	//USER tINIT is typically 200us (but can be adjusted in the GUI)
	//USER The total number of cycles required for this nested counter structure to
	//USER complete is defined by:
	//USER        num_cycles = (CTR2 + 1) * [(CTR1 + 1) * (2 * (CTR0 + 1) + 1) + 1] + 1

	//USER Load counters
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR0_VAL));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR1_VAL));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR2_VAL));
	
	//USER Load jump address
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_RESET_0_CKE_0);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_RESET_0_CKE_0);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_INIT_RESET_0_CKE_0);

	//USER Execute count instruction
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_RESET_0_CKE_0);

	//USER indicate that memory is stable
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);

	//USER transition the RESET to high 
	//USER Wait for 500us
	//USER        num_cycles = (CTR2 + 1) * [(CTR1 + 1) * (2 * (CTR0 + 1) + 1) + 1] + 1
	//USER Load counters
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TRESET_CNTR0_VAL));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TRESET_CNTR1_VAL));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TRESET_CNTR2_VAL));

	//USER Load jump address
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_RESET_1_CKE_0);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_RESET_1_CKE_0);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_INIT_RESET_1_CKE_0);

	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_RESET_1_CKE_0);

	//USER bring up clock enable 

	//USER tXRP < 250 ck cycles
	delay_for_n_mem_clocks(250);

#ifdef RDIMM
	// USER initialize RDIMM buffer so MRS and RZQ Calibrate commands will be 
	// USER propagated to discrete memory devices
	rw_mgr_rdimm_initialize();
#endif

#if LRDIMM
	// USER initialize LRDIMM MB so MRS and RZQ Calibrate commands will be 
	// USER propagated to all sub-ranks.  Per LRDIMM spec, all LRDIMM ranks must have
	// USER RTT_WR set, but only physical ranks 0 and 1 should have RTT_NOM set.
	// USER Therefore RTT_NOM=0 is broadcast to all ranks, and the non-zero value is 
	// USER programmed directly into Ranks 0 and 1 using physical MRS targetting.
	rw_mgr_rdimm_initialize();

	rtt_nom = LRDIMM_SPD_MR_RTT_NOM(LRDIMM_SPD_MR);
	rtt_drv = LRDIMM_SPD_MR_RTT_DRV(LRDIMM_SPD_MR);
	rtt_wr  = LRDIMM_SPD_MR_RTT_WR(LRDIMM_SPD_MR);

	// USER Configure LRDIMM to broadcast LRDIMM MRS commands to all ranks
	rw_mgr_lrdimm_rc_program(0, 14, (((RDIMM_CONFIG_WORD_HIGH >> 24) & 0xF) & (~0x4)));

	// USER Update contents of AC ROM with new RTT WR, DRV values only (NOM = Off)
	rtt_change_MRS1_MRS2_NOM_WR(__RW_MGR_CONTENT_ac_mrs1, rtt_drv, 0, 1);
	rtt_change_MRS1_MRS2_NOM_WR(__RW_MGR_CONTENT_ac_mrs1, rtt_drv, 1, 1);
	rtt_change_MRS1_MRS2_NOM_WR(__RW_MGR_CONTENT_ac_mrs2, rtt_wr,  0, 2);
	rtt_change_MRS1_MRS2_NOM_WR(__RW_MGR_CONTENT_ac_mrs2, rtt_wr,  1, 2);
#endif
#if RDIMM
	// USER initialize RDIMM buffer so MRS and RZQ Calibrate commands will be 
	// USER propagated to discrete memory devices
	rw_mgr_rdimm_initialize();
#endif

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

#if ADVANCED_ODT_CONTROL
		alt_u32 rtt_nom = 0;
		alt_u32 rtt_wr  = 0;
		alt_u32 rtt_drv = 0;

		switch (r) {
			case 0: {
				rtt_nom = MR1_RTT_RANK0;
				rtt_wr  = MR2_RTT_WR_RANK0;
				rtt_drv = MR1_RTT_DRV_RANK0;
			} break;
			case 1: {
				rtt_nom = MR1_RTT_RANK1;
				rtt_wr  = MR2_RTT_WR_RANK1;
				rtt_drv = MR1_RTT_DRV_RANK1;
			} break;
			case 2: {
				rtt_nom = MR1_RTT_RANK2;
				rtt_wr  = MR2_RTT_WR_RANK2;
				rtt_drv = MR1_RTT_DRV_RANK2;
			} break;
			case 3: {
				rtt_nom = MR1_RTT_RANK3;
				rtt_wr  = MR2_RTT_WR_RANK3;
				rtt_drv = MR1_RTT_DRV_RANK3;
			} break;
		}
		rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs1, (rtt_nom|rtt_drv),
		                             ((RW_MGR_MEM_ADDRESS_MIRRORING>>r)&0x1), 1);
		rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs2, rtt_wr,
		                             ((RW_MGR_MEM_ADDRESS_MIRRORING>>r)&0x1), 2);
#endif //ADVANCED_ODT_CONTROL

		//USER set rank 
#if MRS_MIRROR_PING_PONG_ATSO
		// Special case
		// SIDE 0
		set_rank_and_odt_mask_for_ping_pong_atso(0, RW_MGR_ODT_MODE_OFF);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET);

		// SIDE 1
		set_rank_and_odt_mask_for_ping_pong_atso(1, RW_MGR_ODT_MODE_OFF);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2_MIRR);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3_MIRR);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET_MIRR);

		// Unmask all CS
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);
#else
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER Use Mirror-ed commands for odd ranks if address mirrorring is on
		if((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET_MIRR);
		} else {
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET);
		}
#endif

		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_ZQCL);

		//USER tZQinit = tDLLK = 512 ck cycles
		delay_for_n_mem_clocks(512);
	}
#if LRDIMM
	// USER Configure LRDIMM to target physical ranks decoded by RM bits only (ranks 0,1 only)
	// USER Set bit F0RC14.DBA0 to '1' so MRS commands target physical ranks only
	rw_mgr_lrdimm_rc_program(0, 14, (((RDIMM_CONFIG_WORD_HIGH >> 24) & 0xF) | 0x4));
	// USER update AC ROM MR1 entry to include RTT_NOM
	rtt_change_MRS1_MRS2_NOM_WR(__RW_MGR_CONTENT_ac_mrs1, (rtt_drv|rtt_nom), 0, 1);
	rtt_change_MRS1_MRS2_NOM_WR(__RW_MGR_CONTENT_ac_mrs1, (rtt_drv|rtt_nom), 1, 1);
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank
			continue;
		}

		//USER set rank 
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER Use Mirror-ed commands for odd ranks if address mirrorring is on
		if((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
		} else {
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
			delay_for_n_mem_clocks(4);
		}
	}

	// USER Initiate LRDIMM MB->Physical Rank training here
	// USER -> Set minimum skew mode for levelling - F3RC6 = 0001
	rw_mgr_lrdimm_rc_program(3, 6, 0x1);
	// USER -> Set error status output in register F2RC3 for debugging purposes
	rw_mgr_lrdimm_rc_program(2, 3, 0x8);

#ifdef LRDIMM_EXT_CONFIG_ARRAY
	// USER Configure LRDIMM ODT/Drive parameters using SPD information
	{
		static const alt_u8 lrdimm_cfg_array[][3] = LRDIMM_EXT_CONFIG_ARRAY;
		alt_u32 cfg_reg_ctr;

		for (cfg_reg_ctr = 0; cfg_reg_ctr < (sizeof(lrdimm_cfg_array)/sizeof(lrdimm_cfg_array[0])); cfg_reg_ctr++)
		{
			alt_u32 lrdimm_fp  = (alt_u32)lrdimm_cfg_array[cfg_reg_ctr][0];
			alt_u32 lrdimm_rc  = (alt_u32)lrdimm_cfg_array[cfg_reg_ctr][1];
			alt_u32 lrdimm_val = (alt_u32)lrdimm_cfg_array[cfg_reg_ctr][2];

			rw_mgr_lrdimm_rc_program(lrdimm_fp, lrdimm_rc, lrdimm_val);
		}
	}
#endif // LRDIMM_EXT_CONFIG_ARRAY

	// USER -> Initiate MB->DIMM training on the LRDIMM
	rw_mgr_lrdimm_rc_program(0, 12, 0x2);
#if (!STATIC_SKIP_DELAY_LOOPS)
	// USER Wait for max(tcal) * number of physical ranks. Tcal is approx. 10ms.
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS * RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM; r++)
	{
		delay_for_n_ns(80000000UL);
	}
#endif // !STATIC_SKIP_DELAY_LOOPS
	// USER Place MB back in normal operating mode
	rw_mgr_lrdimm_rc_program(0, 12, 0x0);
#endif // LRDIMM
}


#if (ENABLE_NON_DESTRUCTIVE_CALIB || ENABLE_NON_DES_CAL)
void rw_mgr_mem_initialize_no_init (void)
{
	alt_u32 r;
	alt_u32 mem_refresh_all_ranks(alt_u32 no_validate);
	TRACE_FUNC();
	rw_mgr_rdimm_initialize();
	IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, RW_MGR_RANK_ALL);
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_RETURN);
	delay_for_n_mem_clocks(512);
	mem_refresh_all_ranks(1);
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			continue;
		}
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);
		set_jump_as_return();
		if((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET_MIRR);
		} else {
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_DLL_RESET);
		}

// Reprogramming these is not really required but....
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);



		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_ZQCL);
		delay_for_n_mem_clocks(512);
	}

	IOWR_32DIRECT (RW_MGR_ENABLE_REFRESH, 0, 1);  // Enable refresh engine  

}
#endif
#endif // DDR3

#if DDR2
void rw_mgr_mem_initialize (void)
{
	alt_u32 r;

	TRACE_FUNC();
	
	//USER *** NOTE ***
	//USER The following STAGE (n) notation refers to the corresponding stage in the Micron datasheet

	// Here's how you load register for a loop
	//USER Counters are located @ 0x800
	//USER Jump address are located @ 0xC00
	//USER For both, registers 0 to 3 are selected using bits 3 and 2, like in
	//USER 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	// I know this ain't pretty, but Avalon bus throws away the 2 least significant bits
	
	//USER *** STAGE (1, 2, 3) ***

	//USER start with CKE low 

	//USER tINIT is typically 200us (but can be adjusted in the GUI)
	//USER The total number of cycles required for this nested counter structure to
	//USER complete is defined by:
	//USER        num_cycles = (CTR0 + 1) * [(CTR1 + 1) * (2 * (CTR2 + 1) + 1) + 1] + 1

	//TODO: Need to manage multi-rank

	//USER Load counters
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR0_VAL));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR1_VAL));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(SEQ_TINIT_CNTR2_VAL));
	
	//USER Load jump address
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_CKE_0);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_CKE_0);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_INIT_CKE_0);

	//USER Execute count instruction
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_CKE_0);

	//USER indicate that memory is stable 
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);

	//USER Bring up CKE 
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_NOP);

	//USER *** STAGE (4)

	//USER Wait for 400ns 
	delay_for_n_ns(400);

	//USER Multi-rank section begins here
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank 
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER * **** *
		//USER * NOTE *
		//USER * **** *
		//USER The following commands must be spaced by tMRD or tRPA which are in the order
		//USER of 2 to 4 full rate cycles. This is peanuts in the NIOS domain, so for now
		//USER we can avoid redundant wait loops

		// Possible FIXME BEN: for HHP, we need to add delay loops to be sure
		// although, the sequencer write interface by itself likely has enough delay

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER *** STAGE (5)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR2);

		//USER *** STAGE (6)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR3);

		//USER *** STAGE (7)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);

		//USER *** STAGE (8)
		//USER DLL reset
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR_DLL_RESET);

		//USER *** STAGE (9)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER *** STAGE (10)

		//USER Issue 2 refresh commands spaced by tREF 

		//USER First REFRESH
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REFRESH);

		//USER tREF = 200ns
		delay_for_n_ns(200);

		//USER Second REFRESH
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REFRESH);

		//USER Second idle loop
		delay_for_n_ns(200);

		//USER *** STAGE (11)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR_CALIB);

		//USER *** STAGE (12)
		//USER OCD defaults
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR_OCD_ENABLE);

		//USER *** STAGE (13)
		//USER OCD exit
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);

		//USER *** STAGE (14)

		//USER The memory is now initialized. Before being able to use it, we must still
		//USER wait for the DLL to lock, 200 clock cycles after it was reset @ STAGE (8).
		//USER Since we cannot keep track of time in any other way, let's start counting from now
		delay_for_n_mem_clocks(200);
	}
}
#endif // DDR2 

#if LPDDR2
void rw_mgr_mem_initialize (void)
{
	alt_u32 r;

	//USER *** NOTE ***
	//USER The following STAGE (n) notation refers to the corresponding stage in the Micron datasheet

	// Here's how you load register for a loop
	//USER Counters are located @ 0x800
	//USER Jump address are located @ 0xC00
	//USER For both, registers 0 to 3 are selected using bits 3 and 2, like in
	//USER 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	// I know this ain't pretty, but Avalon bus throws away the 2 least significant bits
	
	//USER *** STAGE (1, 2, 3) ***

	//USER start with CKE low 

	//USER tINIT1 = 100ns

	//USER 100ns @ 300MHz (3.333 ns) ~ 30 cycles
	//USER If a is the number of iteration in a loop
	//USER it takes the following number of cycles to complete the operation:
	//USER number_of_cycles = (2 + n) * a
	//USER where n is the number of instruction in the inner loop
	//USER One possible solution is n = 0 , a = 15 => a = 0x10

	//USER Load counter
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0x10));
	
	//USER Load jump address
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_CKE_0);

	//USER Execute count instruction
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_CKE_0);

	//USER tINIT3 = 200us
	delay_for_n_ns(200000);

	//USER indicate that memory is stable 
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);

	//USER Multi-rank section begins here
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank 
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER MRW RESET
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR63_RESET);
	}

	//USER tINIT5 = 10us
	delay_for_n_ns(10000);

	//USER Multi-rank section begins here
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank 
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER MRW ZQC
		// Note: We cannot calibrate other ranks when the current rank is calibrating for tZQINIT
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR10_ZQC);

		//USER tZQINIT = 1us
		delay_for_n_ns(1000);

		//USER * **** *
		//USER * NOTE *
		//USER * **** *
		//USER The following commands must be spaced by tMRW which is in the order
		//USER of 3 to 5 full rate cycles. This is peanuts in the NIOS domain, so for now
		//USER we can avoid redundant wait loops

		//USER MRW MR1
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR1_CALIB);

		//USER MRW MR2
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR2);

		//USER MRW MR3
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR3);
	}
}
#endif // LPDDR2 

#if LPDDR1
void rw_mgr_mem_initialize (void)
{
	alt_u32 r;

	TRACE_FUNC();
	
	//USER *** NOTE ***
	//USER The following STAGE (n) notation refers to the corresponding stage in the Micron datasheet

	// Here's how you load register for a loop
	//USER Counters are located @ 0x800
	//USER Jump address are located @ 0xC00
	//USER For both, registers 0 to 3 are selected using bits 3 and 2, like in
	//USER 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	// I know this ain't pretty, but Avalon bus throws away the 2 least significant bits
	
	//USER *** STAGE (1, 2, 3) ***

	//USER start with CKE high 

	//USER tINIT = 200us

	//USER 200us @ 300MHz (3.33 ns) ~ 60000 clock cycles
	//USER If a and b are the number of iteration in 2 nested loops
	//USER it takes the following number of cycles to complete the operation:
	//USER number_of_cycles = ((2 + n) * b + 2) * a
	//USER where n is the number of instruction in the inner loop
	//USER One possible solution is n = 0 , a = 256 , b = 118 => a = FF, b = 76

	//TODO: Need to manage multi-rank

	//USER Load counters
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0xFF));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0x76));
	
	//USER Load jump address
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_CKE_1);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_CKE_1_inloop);

	//USER Execute count instruction and bring up CKE
	//USER IOWR_32DIRECT (BASE_RW_MGR, 0, __RW_MGR_COUNT_REG_0);
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_CKE_1);

	//USER indicate that memory is stable 
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);

	//USER Multi-rank section begins here
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank 
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER * **** *
		//USER * NOTE *
		//USER * **** *
		//USER The following commands must be spaced by tMRD or tRPA which are in the order
		//USER of 2 to 4 full rate cycles. This is peanuts in the NIOS domain, so for now
		//USER we can avoid redundant wait loops

		// Possible FIXME BEN: for HHP, we need to add delay loops to be sure
		// although, the sequencer write interface by itself likely has enough delay

		//USER *** STAGE (9)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER *** STAGE (10)

		//USER Issue 2 refresh commands spaced by tREF 

		//USER First REFRESH
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REFRESH);

		//USER tREF = 200ns
		delay_for_n_ns(200);

		//USER Second REFRESH
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REFRESH);

		//USER Second idle loop
		delay_for_n_ns(200);

		//USER *** STAGE (11)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR_CALIB);

		//USER *** STAGE (13)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);
	
	}
}
#endif // LPDDR1

#if QDRII
void rw_mgr_mem_initialize (void)
{
	TRACE_FUNC();

	//USER Turn off QDRII DLL to reset it
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_IDLE);

	//USER Turn on QDRII DLL and wait 25us for it to lock
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_NOP);
	delay_for_n_ns(25000);
	
	//USER indicate that memory is stable
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);
}
#endif

#if QDRII
void rw_mgr_mem_dll_lock_wait (void)
{
	//USER The DLL in QDR requires 25us to lock
	delay_for_n_ns(25000);
}
#else
void rw_mgr_mem_dll_lock_wait (void) { }
#endif

#if RLDRAMII
void rw_mgr_mem_initialize (void)
{
	TRACE_FUNC();
	
	//USER start with memory RESET activated

	//USER tINIT = 200us
	delay_for_n_ns(200000);

	//USER indicate that memory is stable 
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);

	//USER Dummy MRS, followed by valid MRS commands to reset the DLL on memory device
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS_INIT);

	//USER 8192 memory cycles for DLL to lock. 
	// 8192 cycles are required by Renesas LLDRAM-II, though we don't officially support it
	delay_for_n_mem_clocks(8192);
	
	//USER Refresh all banks
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REF_X8);

	//USER 1024 memory cycles
	delay_for_n_mem_clocks(1024);
}
#endif

#if RLDRAM3
void rw_mgr_mem_initialize (void)
{
	TRACE_FUNC();
	alt_u32 r;
	
	// Here's how you load register for a loop
	//USER Counters are located @ 0x800
	//USER Jump address are located @ 0xC00
	//USER For both, registers 0 to 3 are selected using bits 3 and 2, like in
	//USER 0x800, 0x804, 0x808, 0x80C and 0xC00, 0xC04, 0xC08, 0xC0C
	// I know this ain't pretty, but Avalon bus throws away the 2 least significant bits
	
	//USER start with memory RESET activated

	//USER tINIT = 200us

	//USER 200us @ 266MHz (3.75 ns) ~ 54000 clock cycles
	//USER If a and b are the number of iteration in 2 nested loops
	//USER it takes the following number of cycles to complete the operation:
	//USER number_of_cycles = ((2 + n) * a + 2) * b
	//USER where n is the number of instruction in the inner loop
	//USER One possible solution is n = 0 , a = 256 , b = 106 => a = FF, b = 6A

	//USER Load counters
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0xFF));
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, SKIP_DELAY_LOOP_VALUE_OR_ZERO(0x6A));
	
	//USER Load jump address
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_INIT_RESET_0);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_INIT_RESET_0_inloop);

	//USER Execute count instruction
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_INIT_RESET_0);

	//USER indicate that memory is stable
	IOWR_32DIRECT (PHY_MGR_RESET_MEM_STBL, 0, 1);
	
	//USER transition the RESET to high 
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_NOP);
	
	//USER Wait for 10000 cycles
	delay_for_n_mem_clocks(10000);

	//USER Load MR0
	if ( RW_MGR_MEM_NUMBER_OF_RANKS == 1 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 2 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFC);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 4 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFC);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
		//USER Wait MRSC
		delay_for_n_mem_clocks(12);
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xF3);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_QUAD_RANK);
	}
	else {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0);
	}
	
	
	//USER Wait MRSC
	delay_for_n_mem_clocks(12);

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank
			continue;
		}
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);
		//USER Load MR1 (reset DLL reset and kick off long ZQ calibration)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_CALIB);
      
      //USER Wait 512 cycles for DLL to reset and for ZQ calibration to complete
      delay_for_n_mem_clocks(512);
	}
		
	//USER Load MR2 (set write protocol to Single Bank)
	if ( RW_MGR_MEM_NUMBER_OF_RANKS == 1 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 2 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFC);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 4 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xF0);
	}
	else {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
	}
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2_CALIB);
	
	//USER Wait MRSC and a bit more
	delay_for_n_mem_clocks(64);
}
#endif

//USER  At the end of calibration we have to program the user settings in, and
//USER  hand off the memory to the user.

#if DDR3
void rw_mgr_mem_handoff (void)
{
	alt_u32 r;
#if LRDIMM
	alt_u32 rtt_nom;
	alt_u32 rtt_drv;
	alt_u32 rtt_wr;
#endif // LRDIMM

	TRACE_FUNC();

#if LRDIMM
	rtt_nom = LRDIMM_SPD_MR_RTT_NOM(LRDIMM_SPD_MR);
	rtt_drv = LRDIMM_SPD_MR_RTT_DRV(LRDIMM_SPD_MR);
	rtt_wr  = LRDIMM_SPD_MR_RTT_WR(LRDIMM_SPD_MR);

	// USER Configure LRDIMM to broadcast LRDIMM MRS commands to all ranks
	// USER Set bit F0RC14.DBA0 to '0' so MRS commands target all physical ranks in a logical rank
	rw_mgr_lrdimm_rc_program(0, 14, (((RDIMM_CONFIG_WORD_HIGH >> 24) & 0xF) & (~0x4)));

	// USER Update contents of AC ROM with new RTT WR, DRV values
	rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs1, rtt_drv, 0, 1);
	rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs1, rtt_drv, 1, 1);
	rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs2, rtt_wr,  0, 2);
	rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs2, rtt_wr,  1, 2);
#endif // LRDIMM

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

#if MRS_MIRROR_PING_PONG_ATSO
		// Side 0
		set_rank_and_odt_mask_for_ping_pong_atso(0, RW_MGR_ODT_MODE_OFF);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_USER);

		// Side 1
		set_rank_and_odt_mask_for_ping_pong_atso(1, RW_MGR_ODT_MODE_OFF);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2_MIRR);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3_MIRR);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
		delay_for_n_mem_clocks(4);
		set_jump_as_return();
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_USER_MIRR);

		// Unmask all CS
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);
#else
		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER precharge all banks ... 

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER load up MR settings specified by user 

		//USER Use Mirror-ed commands for odd ranks if address mirrorring is on
		if((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_USER_MIRR);
		} else {
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS3);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS0_USER);
		}
#endif
		//USER  need to wait tMOD (12CK or 15ns) time before issuing other commands,
		//USER  but we will have plenty of NIOS cycles before actual handoff so its okay.
	}
	

#if LRDIMM	
	delay_for_n_mem_clocks(12);
	// USER Set up targetted MRS commands
	rw_mgr_lrdimm_rc_program(0, 14, (((RDIMM_CONFIG_WORD_HIGH >> 24) & 0xF) | 0x4));
	// USER update AC ROM MR1 entry to include RTT_NOM for physical ranks 0,1 only
	rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs1, (rtt_drv|rtt_nom), 0, 1);
	rtt_change_MRS1_MRS2_NOM_WR (__RW_MGR_CONTENT_ac_mrs1, (rtt_drv|rtt_nom), 1, 1);

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank
			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER Use Mirror-ed commands for odd ranks if address mirrorring is on
		if((RW_MGR_MEM_ADDRESS_MIRRORING >> r) & 0x1) {
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1_MIRR);
			delay_for_n_mem_clocks(4);
		} else {
			delay_for_n_mem_clocks(4);
			set_jump_as_return();
			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
			delay_for_n_mem_clocks(4);
		}
	}
#endif // LRDIMM
}
#endif // DDR3

#if DDR2
void rw_mgr_mem_handoff (void)
{
	alt_u32 r;

	TRACE_FUNC();
	
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER precharge all banks ... 

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER load up MR settings specified by user 

		// FIXME BEN: for HHP, we need to add delay loops to be sure
		// We can check this with BFM perhaps
		// Likely enough delay in RW_MGR though

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR2);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR3);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR_USER);

		//USER need to wait tMOD (12CK or 15ns) time before issuing other commands,
		//USER but we will have plenty of NIOS cycles before actual handoff so its okay.
	}
}
#endif //USER DDR2

#if LPDDR2
void rw_mgr_mem_handoff (void)
{
	alt_u32 r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER precharge all banks...

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER load up MR settings specified by user

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR1_USER);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR2);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR3);
	}
}
#endif //USER LPDDR2

#if LPDDR1
void rw_mgr_mem_handoff (void)
{
	alt_u32 r;

	TRACE_FUNC();
	
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER precharge all banks ... 

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		//USER load up MR settings specified by user 

		// FIXME BEN: for HHP, we need to add delay loops to be sure
		// We can check this with BFM perhaps
		// Likely enough delay in RW_MGR though

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_EMR);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MR_USER);

		//USER need to wait tMOD (12CK or 15ns) time before issuing other commands,
		//USER but we will have plenty of NIOS cycles before actual handoff so its okay.
	}
}
#endif //USER LPDDR1

#if RLDRAMII
void rw_mgr_mem_handoff (void)
{
	TRACE_FUNC();
	
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS);
}
#endif

#if RLDRAM3
void rw_mgr_mem_handoff (void)
{
	TRACE_FUNC();
	alt_u32 r;
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank
			continue;
		}
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);
		
		//USER Load user requested MR1
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS1);
	}
	
	if ( RW_MGR_MEM_NUMBER_OF_RANKS == 1 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 2 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFC);
	} else if ( RW_MGR_MEM_NUMBER_OF_RANKS == 4 ) {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xF0);
	}
	else {
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, 0xFE);
	}
	//USER Load user requested MR2
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_MRS2);
	
	//USER Wait MRSC and a bit more
	delay_for_n_mem_clocks(64);
}
#endif

#if QDRII
void rw_mgr_mem_handoff (void)
{
	TRACE_FUNC();
}
#endif

#if DDRX
//USER performs a guaranteed read on the patterns we are going to use during a read test to ensure memory works
alt_u32 rw_mgr_mem_calibrate_read_test_patterns (alt_u32 rank_bgn, alt_u32 group, alt_u32 num_tries, t_btfld *bit_chk, alt_u32 all_ranks)
{
	alt_u32 r, vg;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	alt_u32 rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	*bit_chk = param->read_correct_mask;
	correct_mask_vg = param->read_correct_mask_vg;
	
	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		//USER Load up a constant bursts of read commands
		
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_GUARANTEED_READ);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_GUARANTEED_READ_CONT);

		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS-1; ; vg--)
		{
			//USER reset the fifos to get pointers to known state

			IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);
			IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);

			tmp_bit_chk = tmp_bit_chk << (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS);

			IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, ((group*RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS+vg) << 2), __RW_MGR_GUARANTEED_READ);
			tmp_bit_chk = tmp_bit_chk | (correct_mask_vg & ~(IORD_32DIRECT(BASE_RW_MGR, 0)));

			if (vg == 0) {
				break;
			}
		}
		*bit_chk &= tmp_bit_chk;
	}

	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, (group << 2), __RW_MGR_CLEAR_DQS_ENABLE);

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
	DPRINT(2, "test_load_patterns(%lu,ALL) => (%lu == %lu) => %lu", group, *bit_chk, param->read_correct_mask, (long unsigned int)(*bit_chk == param->read_correct_mask));
	return (*bit_chk == param->read_correct_mask);
}

alt_u32 rw_mgr_mem_calibrate_read_test_patterns_all_ranks (alt_u32 group, alt_u32 num_tries, t_btfld *bit_chk)
{
	if (rw_mgr_mem_calibrate_read_test_patterns (0, group, num_tries, bit_chk, 1))
	{
		return 1;
	}
	else
	{
		// case:139851 - if guaranteed read fails, we can retry using different dqs enable phases.
		// It is possible that with the initial phase, dqs enable is asserted/deasserted too close 
		// to an dqs edge, truncating the read burst.
		alt_u32 p;
		for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++) {
			scc_mgr_set_dqs_en_phase_all_ranks (group, p);
			if (rw_mgr_mem_calibrate_read_test_patterns (0, group, num_tries, bit_chk, 1))
			{
				return 1;
			}
		}
		return 0;
	}
}
#endif

//USER load up the patterns we are going to use during a read test 
#if DDRX
void rw_mgr_mem_calibrate_read_load_patterns (alt_u32 rank_bgn, alt_u32 all_ranks)
{
	alt_u32 r;
	alt_u32 rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	TRACE_FUNC();
			
	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		//USER Load up a constant bursts

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_GUARANTEED_WRITE_WAIT0);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_GUARANTEED_WRITE_WAIT1);

#if QUARTER_RATE
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x01);
#endif		
#if HALF_RATE
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x02);
#endif		
#if FULL_RATE
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x04);
#endif
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_GUARANTEED_WRITE_WAIT2);

#if QUARTER_RATE
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x01);
#endif		
#if HALF_RATE
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x02);
#endif		
#if FULL_RATE
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x04);
#endif
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_GUARANTEED_WRITE_WAIT3);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_GUARANTEED_WRITE);
	}

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
}
#endif

#if QDRII
void rw_mgr_mem_calibrate_read_load_patterns (alt_u32 rank_bgn, alt_u32 all_ranks)
{
	TRACE_FUNC();
	
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x20);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_GUARANTEED_WRITE_WAIT0);
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x20);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_GUARANTEED_WRITE_WAIT1);
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_GUARANTEED_WRITE);
}
#endif

#if RLDRAMX
void rw_mgr_mem_calibrate_read_load_patterns (alt_u32 rank_bgn, alt_u32 all_ranks)
{
	TRACE_FUNC();
	alt_u32 r;
	alt_u32 rank_end = RW_MGR_MEM_NUMBER_OF_RANKS;//all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);
#if QUARTER_RATE	
	alt_u32 write_data_cycles = 0x10;
#else
	alt_u32 write_data_cycles = 0x20;
#endif
	
	for (r = rank_bgn; r < rank_end; r++) {
	if (param->skip_ranks[r]) {
		//USER request to skip the rank

		continue;
	}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);
			
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, write_data_cycles);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_GUARANTEED_WRITE_WAIT0);
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, write_data_cycles);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_GUARANTEED_WRITE_WAIT1);
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, write_data_cycles);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_GUARANTEED_WRITE_WAIT2);
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, write_data_cycles);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_GUARANTEED_WRITE_WAIT3);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_GUARANTEED_WRITE);
	}
	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
}
#endif

static inline void rw_mgr_mem_calibrate_read_load_patterns_all_ranks (void)
{
	rw_mgr_mem_calibrate_read_load_patterns (0, 1);
}


// pe checkout pattern for harden managers
//void pe_checkout_pattern (void)
//{
//    // test RW manager
//    
//    // do some reads to check load buffer
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_READ_B2B_WAIT1);
//
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_READ_B2B_WAIT2);
//		
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_READ_B2B);
//	
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_READ_B2B);
//	
//	// clear error word
//	IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
//	
//	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_READ_B2B);
//	
//	alt_u32 readdata;
//	
//	// read error word
//	readdata = IORD_32DIRECT(BASE_RW_MGR, 0);
//	
//	// read DI buffer
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 0*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 1*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 2*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 3*4, 0);
//	
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_READ_B2B_WAIT1);
//
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_READ_B2B_WAIT2);
//		
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_READ_B2B);
//	
//	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x0);
//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_READ_B2B);
//	
//	// clear error word
//	IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
//	
//	// do read
//	IOWR_32DIRECT (RW_MGR_LOOPBACK_MODE, 0, __RW_MGR_READ_B2B);
//	
//	// read error word
//	readdata = IORD_32DIRECT(BASE_RW_MGR, 0);
//	
//	// error word should be 0x00
//	
//	// read DI buffer
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 0*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 1*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 2*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 3*4, 0);
//	
//	// clear error word
//	IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
//	
//	// do dm read	
//	IOWR_32DIRECT (RW_MGR_LOOPBACK_MODE, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_WL_1);
//	
//	// read error word
//	readdata = IORD_32DIRECT(BASE_RW_MGR, 0);
//	
//	// error word should be ff
//	
//	// read DI buffer
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 0*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 1*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 2*4, 0);
//	readdata = IORD_32DIRECT(RW_MGR_DI_BASE + 3*4, 0);
//	
//	// exit loopback mode
//	IOWR_32DIRECT (BASE_RW_MGR, 0, __RW_MGR_IDLE_LOOP2);
//	
//	// start of phy manager access
//	
//	readdata = IORD_32DIRECT (PHY_MGR_MAX_RLAT_WIDTH, 0);
//	readdata = IORD_32DIRECT (PHY_MGR_MAX_AFI_WLAT_WIDTH, 0);
//	readdata = IORD_32DIRECT (PHY_MGR_MAX_AFI_RLAT_WIDTH, 0);
//	readdata = IORD_32DIRECT (PHY_MGR_CALIB_SKIP_STEPS, 0);
//	readdata = IORD_32DIRECT (PHY_MGR_CALIB_VFIFO_OFFSET, 0);	
//	readdata = IORD_32DIRECT (PHY_MGR_CALIB_LFIFO_OFFSET, 0);
//	
//	// start of data manager test
//	
//	readdata = IORD_32DIRECT (DATA_MGR_DRAM_CFG	    , 0);
//	readdata = IORD_32DIRECT (DATA_MGR_MEM_T_WL	    , 0);
//	readdata = IORD_32DIRECT (DATA_MGR_MEM_T_ADD	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_MEM_T_RL	    , 0);
//	readdata = IORD_32DIRECT (DATA_MGR_MEM_T_RFC	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_MEM_T_REFI	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_MEM_T_WR	    , 0);
//	readdata = IORD_32DIRECT (DATA_MGR_MEM_T_MRD	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_COL_WIDTH	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_ROW_WIDTH	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_BANK_WIDTH	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_CS_WIDTH	    , 0);
//	readdata = IORD_32DIRECT (DATA_MGR_ITF_WIDTH	, 0);
//	readdata = IORD_32DIRECT (DATA_MGR_DVC_WIDTH	, 0);
//	
//}

//USER  try a read and see if it returns correct data back. has dummy reads inserted into the mix
//USER  used to align dqs enable. has more thorough checks than the regular read test.

alt_u32 rw_mgr_mem_calibrate_read_test (alt_u32 rank_bgn, alt_u32 group, alt_u32 num_tries, alt_u32 all_correct, t_btfld *bit_chk, alt_u32 all_groups, alt_u32 all_ranks)
{
	alt_u32 r, vg;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	alt_u32 rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);

#if LRDIMM
	// USER Disable MB Write-levelling mode and enter normal operation
	rw_mgr_lrdimm_rc_program(0,12,0x0);
#endif

	*bit_chk = param->read_correct_mask;
	correct_mask_vg = param->read_correct_mask_vg;
	
	alt_u32 quick_read_mode = (((STATIC_CALIB_STEPS) & CALIB_SKIP_DELAY_SWEEPS) && ENABLE_SUPER_QUICK_CALIBRATION) || BFM_MODE;

	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x10);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_READ_B2B_WAIT1);
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x10);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_READ_B2B_WAIT2);
		
		if(quick_read_mode) {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x1); /* need at least two (1+1) reads to capture failures */
		} else if (all_groups) {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x06);
		} else {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x32);
		}
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_READ_B2B);
		if(all_groups) {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, RW_MGR_MEM_IF_READ_DQS_WIDTH * RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS - 1);
		} else {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x0);
		}
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_READ_B2B);
		
		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS-1; ; vg--)
		{
			//USER reset the fifos to get pointers to known state 

			IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);
			IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);	

			tmp_bit_chk = tmp_bit_chk << (RW_MGR_MEM_DQ_PER_READ_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS);

			IOWR_32DIRECT (all_groups ? RW_MGR_RUN_ALL_GROUPS : RW_MGR_RUN_SINGLE_GROUP, ((group*RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS+vg) << 2), __RW_MGR_READ_B2B);
			tmp_bit_chk = tmp_bit_chk | (correct_mask_vg & ~(IORD_32DIRECT(BASE_RW_MGR, 0)));

			if (vg == 0) {
				break;
			}
		}
		*bit_chk &= tmp_bit_chk;
	}

#if ENABLE_BRINGUP_DEBUGGING
	load_di_buf_gbl();
#endif
	
	#if DDRX
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, (group << 2), __RW_MGR_CLEAR_DQS_ENABLE);
	#endif
	
	if (all_correct)
	{
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "read_test(%lu,ALL,%lu) => (%lu == %lu) => %lu", group, all_groups, *bit_chk, param->read_correct_mask, (long unsigned int)(*bit_chk == param->read_correct_mask));
		return (*bit_chk == param->read_correct_mask);
	}
	else
	{
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "read_test(%lu,ONE,%lu) => (%lu != %lu) => %lu", group, all_groups, *bit_chk, (long unsigned int)0, (long unsigned int)(*bit_chk != 0x00));
		return (*bit_chk != 0x00);
	}
}

static inline alt_u32 rw_mgr_mem_calibrate_read_test_all_ranks (alt_u32 group, alt_u32 num_tries, alt_u32 all_correct, t_btfld *bit_chk, alt_u32 all_groups)
{
    alt_u32 success = 1;
    alt_u32 one_test_success;
    alt_u32 i = 0;
    if (num_tries <= 0) num_tries = 1;
    for (i = 0; i < num_tries; i++)
    {
        one_test_success = rw_mgr_mem_calibrate_read_test (0, group, 1, all_correct, bit_chk, all_groups, 1);
        success = success & one_test_success;
        if (success == 0)
        {
            break;
        }
    }
    
    return success;
}

#if ENABLE_DELAY_CHAIN_WRITE
void rw_mgr_incr_vfifo_auto(alt_u32 grp) {
	alt_u32 v;
	v = vfifo_settings[grp]%VFIFO_SIZE;
	rw_mgr_incr_vfifo(grp, &v);
	vfifo_settings[grp] = v;
}

void rw_mgr_decr_vfifo_auto(alt_u32 grp) {
	alt_u32 v;
	v = vfifo_settings[grp]%VFIFO_SIZE;
	rw_mgr_decr_vfifo(grp, &v);
	vfifo_settings[grp] = v;
}
#endif // ENABLE_DELAY_CHAIN_WRITE

void rw_mgr_incr_vfifo(alt_u32 grp, alt_u32 *v) {
	//USER fiddle with FIFO 
	if(HARD_PHY) {
		IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_HARD_PHY, 0, grp);
	} else if (QUARTER_RATE_MODE && !HARD_VFIFO) {
		if ((*v & 3) == 3) {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_QR, 0, grp);
		} else if ((*v & 2) == 2) {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR_HR, 0, grp);
		} else if ((*v & 1) == 1) {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_HR, 0, grp);
		} else {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
		}
	} else if (HARD_VFIFO) {
		// Arria V & Cyclone V have a hard full-rate VFIFO that only has a single incr signal
		IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
	}
	else {
		if (!HALF_RATE_MODE || (*v & 1) == 1) {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_HR, 0, grp);
		} else {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR, 0, grp);
		}
	}
	
	(*v)++;
#if USE_DQS_TRACKING && !HHP_HPS
	IOWR_32DIRECT (TRK_V_POINTER, (grp << 2), *v);
#endif
	BFM_INC_VFIFO;
}

//Used in quick cal to properly loop through the duplicated VFIFOs in AV QDRII/RLDRAM
static inline void rw_mgr_incr_vfifo_all(alt_u32 grp, alt_u32 *v) {
#if VFIFO_CONTROL_WIDTH_PER_DQS == 1	
	rw_mgr_incr_vfifo(grp, v);
#else
	alt_u32 i;
	for(i = 0; i < VFIFO_CONTROL_WIDTH_PER_DQS; i++) {
		rw_mgr_incr_vfifo(grp*VFIFO_CONTROL_WIDTH_PER_DQS+i, v);
		if(i != 0) {
			(*v)--;
		}
	}
#endif
}

void rw_mgr_decr_vfifo(alt_u32 grp, alt_u32 *v) {

	alt_u32 i;

	for (i = 0; i < VFIFO_SIZE-1; i++) {
		rw_mgr_incr_vfifo(grp, v);
 	}
}

//USER find a good dqs enable to use 

#if QDRII || RLDRAMX
alt_u32 rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase (alt_u32 grp)
{
	alt_u32 v;
	alt_u32 found;
    alt_u32 dtaps_per_ptap, tmp_delay;
	t_btfld bit_chk;

	TRACE_FUNC("%lu", grp);
	
	reg_file_set_sub_stage(CAL_SUBSTAGE_DQS_EN_PHASE);

	found = 0;

	//USER first push vfifo until we get a passing read 
	for (v = 0; v < VFIFO_SIZE && found == 0;) {
		DPRINT(2, "find_dqs_en_phase: vfifo %lu", BFM_GBL_GET(vfifo_idx));
		if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			found = 1;
		}

		if (!found) {
			//USER fiddle with FIFO
#if (VFIFO_CONTROL_WIDTH_PER_DQS != 1)
			alt_u32 i;
			for (i = 0; i < VFIFO_CONTROL_WIDTH_PER_DQS; i++) {
				rw_mgr_incr_vfifo(grp*VFIFO_CONTROL_WIDTH_PER_DQS+i, &v);
				v--; // undo increment of v in rw_mgr_incr_vfifo
			}
			v++;	// add back single increment
#else
			rw_mgr_incr_vfifo(grp, &v);
#endif
		}
	}

#if (VFIFO_CONTROL_WIDTH_PER_DQS != 1)
	if (found) {
		// we found a vfifo setting that works for at least one vfifo "group"
		// Some groups may need next vfifo setting, so check each one to
		// see if we get new bits passing by increment the vfifo
		alt_u32 i;
		t_btfld best_bit_chk_inv;
		alt_u8 found_on_first_check = (v == 1);

		best_bit_chk_inv = ~bit_chk;
		
		for (i = 0; i < VFIFO_CONTROL_WIDTH_PER_DQS; i++) {
			rw_mgr_incr_vfifo(grp*VFIFO_CONTROL_WIDTH_PER_DQS+i, &v);
			v--; // undo increment of v in rw_mgr_incr_vfifo, just in case it matters for next check
			rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0);
			if ((bit_chk & best_bit_chk_inv) != 0) {
				// found some new bits
				best_bit_chk_inv = ~bit_chk;
			} else {
				// no improvement, so put back
				rw_mgr_decr_vfifo(grp*VFIFO_CONTROL_WIDTH_PER_DQS+i, &v);
				v++;
				if (found_on_first_check) {
					// found on first vfifo check, so we also need to check earlier vfifo values
					rw_mgr_decr_vfifo(grp*VFIFO_CONTROL_WIDTH_PER_DQS+i, &v);
					v++; // undo decrement of v in rw_mgr_incr_vfifo, just in case it matters for next check
					rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0);
					if ((bit_chk & best_bit_chk_inv) != 0) {
						// found some new bits
						best_bit_chk_inv = ~bit_chk;
					} else {
						// no improvement, so put back
						rw_mgr_incr_vfifo(grp*VFIFO_CONTROL_WIDTH_PER_DQS+i, &v);
						v--;
					}
				} // found_on_first_check
			} // check for new bits
		} // loop over all vfifo control bits
	}
#endif	

	if (found) {
		DPRINT(2, "find_dqs_en_phase: found vfifo=%lu", BFM_GBL_GET(vfifo_idx));
		// Not really dqs_enable left/right edge, but close enough for testing purposes
		BFM_GBL_SET(dqs_enable_left_edge[grp].v,BFM_GBL_GET(vfifo_idx));
		BFM_GBL_SET(dqs_enable_right_edge[grp].v,BFM_GBL_GET(vfifo_idx));
		BFM_GBL_SET(dqs_enable_mid[grp].v,BFM_GBL_GET(vfifo_idx));
	} else {
		DPRINT(2, "find_dqs_en_phase: no valid vfifo found");
	}

#if ENABLE_TCL_DEBUG
	// FIXME: Not a dynamically calculated value for dtaps_per_ptap
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}
	dtaps_per_ptap--;
	ALTERA_ASSERT(dtaps_per_ptap <= IO_DQS_EN_DELAY_MAX);
	
    TCLRPT_SET(debug_summary_report->computed_dtap_per_ptap, dtaps_per_ptap);
#endif

	return found;
}
#endif

#if DDRX
#if NEWVERSION_DQSEN
	
// Navid's version 
	
alt_u32 rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase (alt_u32 grp)
{
	alt_u32 i, d, v, p, sr, j;
	alt_u32 max_working_cnt;
	alt_u32 fail_cnt;
	t_btfld bit_chk;
	alt_u32 dtaps_per_ptap;
	alt_u32 found_begin, found_end;
	alt_u32 work_bgn, work_mid, work_end, tmp_delay;
	alt_u32 test_status;
	alt_u32 found_passing_read, found_failing_read, initial_failing_dtap;
#if RUNTIME_CAL_REPORT
	alt_u32 start_v[NUM_SHADOW_REGS], start_p[NUM_SHADOW_REGS], start_d[NUM_SHADOW_REGS];
	alt_u32 end_v[NUM_SHADOW_REGS], end_p[NUM_SHADOW_REGS], end_d[NUM_SHADOW_REGS];
	for(sr = 0; sr < NUM_SHADOW_REGS; sr++)	{
		start_v[sr] = 0;
		start_p[sr] = 0;
		start_d[sr] = 0;
	}
#endif	

	TRACE_FUNC("%lu", grp);
	BFM_STAGE("find_dqs_en_phase");
	ALTERA_ASSERT(grp < RW_MGR_MEM_IF_READ_DQS_WIDTH);

	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);
	
	scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
#if SKIP_PTAP_0_DQS_EN_CAL
        scc_mgr_set_dqs_en_phase_all_ranks(grp, 1);
#else
	scc_mgr_set_dqs_en_phase_all_ranks(grp, 0);
#endif

	fail_cnt = 0;
	
	//USER **************************************************************
	//USER * Step 0 : Determine number of delay taps for each phase tap *
	
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}
	dtaps_per_ptap--;
	ALTERA_ASSERT(dtaps_per_ptap <= IO_DQS_EN_DELAY_MAX);
	tmp_delay = 0;	
	TCLRPT_SET(debug_summary_report->computed_dtap_per_ptap, dtaps_per_ptap);

	// VFIFO sweep
#if ENABLE_DQSEN_SWEEP
	init_di_buffer();
	work_bgn = 0;
	for (d = 0; d <= dtaps_per_ptap; d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {
		work_bgn = tmp_delay;
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		for (i = 0; i < VFIFO_SIZE; i++) {
			for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
				DPRINT(2, "find_dqs_en_phase: begin: vfifo=%lu ptap=%lu dtap=%lu", BFM_GBL_GET(vfifo_idx), p, d);
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				test_status = rw_mgr_mem_calibrate_read_test_all_ranks (grp, 5, PASS_ONE_BIT, &bit_chk, 0);

				//if (p ==0 && d == 0)
				sample_di_data(bit_chk, work_bgn, d, i, p);
			}
			//Increment FIFO
			rw_mgr_incr_vfifo(grp, &v);
		}

		work_bgn++;
	}
	flag_di_buffer_done();
#endif

	//USER *********************************************************
	//USER * Step 1 : First push vfifo until we get a failing read *
	for (v = 0; v < VFIFO_SIZE; ) {
		DPRINT(2, "find_dqs_en_phase: vfifo %lu", BFM_GBL_GET(vfifo_idx));
		test_status = rw_mgr_mem_calibrate_read_test_all_ranks (grp, 5, PASS_ONE_BIT, &bit_chk, 0);
		if (!test_status) {
			fail_cnt++;

			if (fail_cnt == 2) {
				break;
			}
		}

		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (v >= VFIFO_SIZE) {
		//USER no failing read found!! Something must have gone wrong
		DPRINT(2, "find_dqs_en_phase: vfifo failed");
		return 0;
	}

	max_working_cnt = 0;
	
	//USER ********************************************************
	//USER * step 2: find first working phase, increment in ptaps *
	found_begin = 0;
	work_bgn = 0;
	for (d = 0; d <= dtaps_per_ptap; d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {
		work_bgn = tmp_delay;
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);
				
		for (i = 0; i < VFIFO_SIZE; i++) {
			for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
#if SKIP_PTAP_0_DQS_EN_CAL
				// Skip p == 0 setting for HARD PHY
				if (p == 0) {
					continue;
				}
#endif				
				DPRINT(2, "find_dqs_en_phase: begin: vfifo=%lu ptap=%lu dtap=%lu", BFM_GBL_GET(vfifo_idx), p, d);
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				test_status = rw_mgr_mem_calibrate_read_test_all_ranks (grp, 5, PASS_ONE_BIT, &bit_chk, 0);

				if (test_status) {
					max_working_cnt = 1;
					found_begin = 1;
					break;
				}
			}
			
			if (found_begin) {
				break;
			}
			
			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
			}
		}
		
		if (found_begin) {
			break;
		}
	}
	
	if (i >= VFIFO_SIZE) {
		//USER cannot find working solution 
		DPRINT(2, "find_dqs_en_phase: no vfifo/ptap/dtap");
		return 0;
	}
	
	work_end = work_bgn;

	//USER  If d is 0 then the working window covers a phase tap and we can follow the old procedure
	//USER 	otherwise, we've found the beginning, and we need to increment the dtaps until we find the end 
	if (d == 0) {
		//USER ********************************************************************
		//USER * step 3a: if we have room, back off by one and increment in dtaps *
		COV(EN_PHASE_PTAP_OVERLAP);
			
		//USER Special case code for backing up a phase 
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX ;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}
		tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;
		
		// For HARD EMIF we increase the phase if p == 0 as we can't set that value
#if SKIP_PTAP_0_DQS_EN_CAL	
		if (p == 0) {
			p = 1;
			tmp_delay = work_bgn;
		}
#endif
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
			
		found_begin = 0;
		for (d = 0; d <= IO_DQS_EN_DELAY_MAX && tmp_delay < work_bgn; d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {

			DPRINT(2, "find_dqs_en_phase: begin-2: vfifo=%lu ptap=%lu dtap=%lu", BFM_GBL_GET(vfifo_idx), p, d);
			
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);
				
			if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 5, PASS_ONE_BIT, &bit_chk, 0)) {
				found_begin = 1;
				work_bgn = tmp_delay;
				break;
			}
		}

#if BFM_MODE
		{
			alt_32 p2, v2, d2;

			// print out where the actual beginning is
			if (found_begin) {
				v2 = BFM_GBL_GET(vfifo_idx);
				p2 = p;
				d2 = d;
			} else if (p == IO_DQS_EN_PHASE_MAX) {
				v2 = (BFM_GBL_GET(vfifo_idx) + 1) % VFIFO_SIZE;
#if SKIP_PTAP_0_DQS_EN_CAL
				p2 = 1;
#else
				p2 = 0;
#endif
				d2 = 0;
			} else {
				v2 = BFM_GBL_GET(vfifo_idx);
				p2 = p + 1;
				d2 = 0;
			}

			DPRINT(2, "find_dqs_en_phase: begin found: vfifo=%lu ptap=%lu dtap=%lu begin=%lu",
			       v2, p2, d2, work_bgn);
			BFM_GBL_SET(dqs_enable_left_edge[grp].v,v2);
			BFM_GBL_SET(dqs_enable_left_edge[grp].p,p2);
			BFM_GBL_SET(dqs_enable_left_edge[grp].d,d2);
			BFM_GBL_SET(dqs_enable_left_edge[grp].ps,work_bgn);
		}
#endif
		// Record the debug data
		// Currently dqsen is the same for all ranks
		for (sr = 0; sr < NUM_SHADOW_REGS; sr++)
		{
			TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].work_begin, work_bgn);
		if (found_begin)
		{
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].phase_begin, p);
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].delay_begin, d);
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].vfifo_begin, v % VFIFO_SIZE);
#if RUNTIME_CAL_REPORT
				start_v[sr] = v % VFIFO_SIZE;
				start_p[sr] = p;
				start_d[sr] = d;
#endif
		}
		else if (p == IO_DQS_EN_PHASE_MAX)
		{
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].phase_begin, 0);
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].delay_begin, 0);
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].vfifo_begin, (v+1) % VFIFO_SIZE);
#if RUNTIME_CAL_REPORT
				start_v[sr] = (v+1) % VFIFO_SIZE;
				start_p[sr] = p;
				start_d[sr] = d;
#endif
		}
		else
		{
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].phase_begin, p+1);
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].delay_begin, 0);
				TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].vfifo_begin, v % VFIFO_SIZE);
#if RUNTIME_CAL_REPORT
				start_v[sr] = v % VFIFO_SIZE;
				start_p[sr] = p+1;
				start_d[sr] = d;
#endif
			}
		}
		
		//USER We have found a working dtap before the ptap found above 
		if (found_begin == 1) {
			max_working_cnt++;
		} 
			
		//USER Restore VFIFO to old state before we decremented it (if needed)
		p = p + 1;
		if (p > IO_DQS_EN_PHASE_MAX) {
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}
			
		scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
		
		//USER ***********************************************************************************
		//USER * step 4a: go forward from working phase to non working phase, increment in ptaps *
		p = p + 1;
		work_end += IO_DELAY_PER_OPA_TAP;
		if (p > IO_DQS_EN_PHASE_MAX) {
			//USER fiddle with FIFO
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}

		j = 0;
		
		found_end = 0;
		for (; i < VFIFO_SIZE + 1; i++) {
			for (; p <= IO_DQS_EN_PHASE_MAX; p++, work_end += IO_DELAY_PER_OPA_TAP) {
				DPRINT(2, "find_dqs_en_phase: end: vfifo=%lu ptap=%lu dtap=%lu", BFM_GBL_GET(vfifo_idx), p, (long unsigned int)0);
				j++;
#if SKIP_PTAP_0_DQS_EN_CAL
				if ( p == 0 ) {
					max_working_cnt++;
					continue;
				}
#endif
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				test_status = rw_mgr_mem_calibrate_read_test_all_ranks (grp, 5, PASS_ONE_BIT, &bit_chk, 0);
				
				// Check if the first edge we try fails
				// This indicates that the begin edge that we found was fuzzy, so we adjust the begin edge
				if (!test_status && (j == 1))
				{
					work_bgn = work_end;
				}
				else if (!test_status)
				{
					found_end = 1;
					break;
				} else {
					max_working_cnt++;
				}
			}
			
			if (found_end) {
				break;
			}
			
			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
				p = 0;
			}		
		}
		
		if (i >= VFIFO_SIZE + 1) {
			//USER cannot see edge of failing read 
			DPRINT(2, "find_dqs_en_phase: end: failed");
			return 0;
		}
		
		//USER *********************************************************
		//USER * step 5a:  back off one from last, increment in dtaps  *
			
		//USER Special case code for backing up a phase 
#if SKIP_PTAP_0_DQS_EN_CAL	
		if (p == 1) {
			p = 0;
			work_end -= IO_DELAY_PER_OPA_TAP;
			max_working_cnt--;
		}
#endif
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}
		
		work_end -= IO_DELAY_PER_OPA_TAP;
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
		
		//USER * The actual increment of dtaps is done outside of the if/else loop to share code
		d = 0;
	
		DPRINT(2, "find_dqs_en_phase: found end v/p: vfifo=%lu ptap=%lu", BFM_GBL_GET(vfifo_idx), p);
	} else {
	
		// We should not be hitting this case as the window should be around one clock cycle wide

		//USER ********************************************************************
		//USER * step 3-5b:  Find the right edge of the window using delay taps   *		
		COV(EN_PHASE_PTAP_NO_OVERLAP);
		
		DPRINT(2, "find_dqs_en_phase: begin found: vfifo=%lu ptap=%lu dtap=%lu begin=%lu", BFM_GBL_GET(vfifo_idx), p, d, work_bgn);
		BFM_GBL_SET(dqs_enable_left_edge[grp].v,BFM_GBL_GET(vfifo_idx));
		BFM_GBL_SET(dqs_enable_left_edge[grp].p,p);
		BFM_GBL_SET(dqs_enable_left_edge[grp].d,d);
		BFM_GBL_SET(dqs_enable_left_edge[grp].ps,work_bgn);

		work_end = work_bgn;
		
		//USER * The actual increment of dtaps is done outside of the if/else loop to share code
		
		//USER Only here to counterbalance a subtract later on which is not needed if this branch
		//USER  of the algorithm is taken 
		max_working_cnt++;
	}

	//USER The dtap increment to find the failing edge is done here
	for (; d <= IO_DQS_EN_DELAY_MAX; d++, work_end += IO_DELAY_PER_DQS_EN_DCHAIN_TAP) {

			DPRINT(2, "find_dqs_en_phase: end-2: dtap=%lu", d);
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 5, PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}

	//USER Go back to working dtap 
	if (d != 0) {
		work_end -= IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	} 
		
	DPRINT(2, "find_dqs_en_phase: found end v/p/d: vfifo=%lu ptap=%lu dtap=%lu end=%lu", BFM_GBL_GET(vfifo_idx), p, d-1, work_end);
	BFM_GBL_SET(dqs_enable_right_edge[grp].v,BFM_GBL_GET(vfifo_idx));
	BFM_GBL_SET(dqs_enable_right_edge[grp].p,p);
	BFM_GBL_SET(dqs_enable_right_edge[grp].d,d-1);
	BFM_GBL_SET(dqs_enable_right_edge[grp].ps,work_end);

	// Record the debug data
	for (sr = 0; sr < NUM_SHADOW_REGS; sr++)
	{
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].work_end, work_end);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].phase_end, p);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].delay_end, d-1);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][grp].vfifo_end, v % VFIFO_SIZE);
#if RUNTIME_CAL_REPORT
		end_v[sr] = v % VFIFO_SIZE;
		end_p[sr] = p;
		end_d[sr] = d-1;
#endif
	}

	if (work_end >= work_bgn) {
		//USER we have a working range 
	} else {
		//USER nil range 
		DPRINT(2, "find_dqs_en_phase: end-2: failed");
		return 0;
	}
	
	DPRINT(2, "find_dqs_en_phase: found range [%lu,%lu]", work_bgn, work_end);

#if USE_DQS_TRACKING
	// ***************************************************************
	//USER * We need to calculate the number of dtaps that equal a ptap
	//USER * To do that we'll back up a ptap and re-find the edge of the 
	//USER * window using dtaps

	DPRINT(2, "find_dqs_en_phase: calculate dtaps_per_ptap for tracking");
	
	//USER Special case code for backing up a phase 
	
#if SKIP_PTAP_0_DQS_EN_CAL	
	if (p == 0 || p == 1) {
#else
	if (p == 0) {
#endif
		p = IO_DQS_EN_PHASE_MAX;
		rw_mgr_decr_vfifo(grp, &v);
		DPRINT(2, "find_dqs_en_phase: backed up cycle/phase: v=%lu p=%lu", BFM_GBL_GET(vfifo_idx), p);
	} else {
		p = p - 1;
		DPRINT(2, "find_dqs_en_phase: backed up phase only: v=%lu p=%lu", BFM_GBL_GET(vfifo_idx), p);
	}
	
	scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
	
	//USER Increase dtap until we first see a passing read (in case the window is smaller than a ptap),
	//USER and then a failing read to mark the edge of the window again
	
	//USER Find a passing read
	DPRINT(2, "find_dqs_en_phase: find passing read");
	found_passing_read = 0;
   	found_failing_read = 0;
	initial_failing_dtap = d;
	for (; d <= IO_DQS_EN_DELAY_MAX; d++) {
		DPRINT(2, "find_dqs_en_phase: testing read d=%lu", d);
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

		if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			found_passing_read = 1;
			break;
		}
	}
	
	if (found_passing_read) {
	   //USER Find a failing read 
	   DPRINT(2, "find_dqs_en_phase: find failing read");
	   for (d = d + 1; d <= IO_DQS_EN_DELAY_MAX; d++) {
	   	DPRINT(2, "find_dqs_en_phase: testing read d=%lu", d);
	   	scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

	   	if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
	   		found_failing_read = 1;
	   		break;
	   	}
	   }	
   } else {
      DPRINT(1, "find_dqs_en_phase: failed to calculate dtaps per ptap. Fall back on static value");
	}

	//USER The dynamically calculated dtaps_per_ptap is only valid if we found a passing/failing read
	//USER If we didn't, it means d hit the max (IO_DQS_EN_DELAY_MAX).
   //USER Otherwise, dtaps_per_ptap retains its statically calculated value.
	if(found_passing_read && found_failing_read) {
      dtaps_per_ptap = d - initial_failing_dtap;
   }

	ALTERA_ASSERT(dtaps_per_ptap <= IO_DQS_EN_DELAY_MAX);
#if HHP_HPS
	IOWR_32DIRECT (REG_FILE_DTAPS_PER_PTAP, 0, dtaps_per_ptap);
#else
	IOWR_32DIRECT (TRK_DTAPS_PER_PTAP, 0, dtaps_per_ptap);
#endif

	DPRINT(2, "find_dqs_en_phase: dtaps_per_ptap=%lu - %lu = %lu", d, initial_failing_dtap, dtaps_per_ptap);
#endif
	
	//USER ********************************************
	//USER * step 6:  Find the centre of the window   *
		
	work_mid = (work_bgn + work_end) / 2;
	tmp_delay = 0;

	DPRINT(2, "work_bgn=%ld work_end=%ld work_mid=%ld", work_bgn, work_end, work_mid);
	//USER Get the middle delay to be less than a VFIFO delay 
	for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++, tmp_delay += IO_DELAY_PER_OPA_TAP);
	DPRINT(2, "vfifo ptap delay %ld", tmp_delay);
	while(work_mid > tmp_delay) work_mid -= tmp_delay;
	DPRINT(2, "new work_mid %ld", work_mid);
	tmp_delay = 0;
	for (p = 0; p <= IO_DQS_EN_PHASE_MAX && tmp_delay < work_mid; p++, tmp_delay += IO_DELAY_PER_OPA_TAP);
	tmp_delay -= IO_DELAY_PER_OPA_TAP;
	DPRINT(2, "new p %ld, tmp_delay=%ld", p-1, tmp_delay);
	for (d = 0; d <= IO_DQS_EN_DELAY_MAX && tmp_delay < work_mid; d++, tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP);
	DPRINT(2, "new d %ld, tmp_delay=%ld", d, tmp_delay);
	
	// DQSEN same for all shadow reg
	for(sr = 0; sr < NUM_SHADOW_REGS; sr++) {
		TCLRPT_SET(debug_cal_report->cal_dqs_in_margins[sr][grp].dqsen_margin, max_working_cnt -1);
	}
#if SKIP_PTAP_0_DQS_EN_CAL	
		if (p == 1) {
			// If center lies at p=0 and d=d, then the safest choice is to set the center at p=1 and d=0
			p = 2;
			d = 0;
		}
#endif
	scc_mgr_set_dqs_en_phase_all_ranks(grp, p-1);
	scc_mgr_set_dqs_en_delay_all_ranks(grp, d);
	
	//USER push vfifo until we can successfully calibrate. We can do this because
	//USER the largest possible margin in 1 VFIFO cycle

	for (i = 0; i < VFIFO_SIZE; i++) {
		DPRINT(2, "find_dqs_en_phase: center: vfifo=%lu", BFM_GBL_GET(vfifo_idx));
		if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}

		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		DPRINT(2, "find_dqs_en_phase: center: failed");
		return 0;
	}
#if RUNTIME_CAL_REPORT
	for(sr = 0; sr < NUM_SHADOW_REGS; sr++) {
		RPRINT("DQS Enable ; Group %lu ; Rank %lu ; Start  VFIFO %2li ; Phase %li ; Delay %2li", grp, sr, start_v[sr], start_p[sr], start_d[sr]);
		RPRINT("DQS Enable ; Group %lu ; Rank %lu ; End    VFIFO %2li ; Phase %li ; Delay %2li", grp, sr, end_v[sr], end_p[sr], end_d[sr]);
      // Case 174276: Normalizing VFIFO center
		RPRINT("DQS Enable ; Group %lu ; Rank %lu ; Center VFIFO %2li ; Phase %li ; Delay %2li", grp, sr, (v % VFIFO_SIZE), p-1, d);
	}
#endif
	DPRINT(2, "find_dqs_en_phase: center found: vfifo=%li ptap=%lu dtap=%lu", BFM_GBL_GET(vfifo_idx), p-1, d);
	#if ENABLE_DELAY_CHAIN_WRITE
	vfifo_settings[grp] = v;
	#endif // ENABLE_DELAY_CHAIN_WRITE
	BFM_GBL_SET(dqs_enable_mid[grp].v,BFM_GBL_GET(vfifo_idx));
	BFM_GBL_SET(dqs_enable_mid[grp].p,p-1);
	BFM_GBL_SET(dqs_enable_mid[grp].d,d);
	BFM_GBL_SET(dqs_enable_mid[grp].ps,work_mid);
	return 1;
}

#if 0
// Ryan's algorithm 

alt_u32 rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase (alt_u32 grp)
{
	alt_u32 i, d, v, p;
	alt_u32 min_working_p, max_working_p, min_working_d, max_working_d, max_working_cnt;
	alt_u32 fail_cnt;
	t_btfld bit_chk;
	alt_u32 dtaps_per_ptap;
	alt_u32 found_begin, found_end;
	alt_u32 tmp_delay;

	TRACE_FUNC("%lu", grp);
	
	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);

	scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
	scc_mgr_set_dqs_en_phase_all_ranks(grp, 0);

	fail_cnt = 0;
	
	//USER **************************************************************
	//USER * Step 0 : Determine number of delay taps for each phase tap *
	
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
	}
	dtaps_per_ptap--;

	//USER *********************************************************
	//USER * Step 1 : First push vfifo until we get a failing read *
	for (v = 0; v < VFIFO_SIZE; ) {
		if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			fail_cnt++;

			if (fail_cnt == 2) {
				break;
			}
		}

		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		//USER no failing read found!! Something must have gone wrong
		return 0;
	}

	max_working_cnt = 0;
	min_working_p = 0;
	
	//USER ********************************************************
	//USER * step 2: find first working phase, increment in ptaps *
	found_begin = 0;
	for (d = 0; d <= dtaps_per_ptap; d++) {
		scc_mgr_set_dqs_en_delay_all_ranks(grp, d);
				
		for (i = 0; i < VFIFO_SIZE; i++) {
			for (p = 0; p <= IO_DQS_EN_PHASE_MAX; p++) {
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);

				if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
					max_working_cnt = 1;
					found_begin = 1;
					break;
				}
			}
			
			if (found_begin) {
				break;
			}
			
			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
			}
		}
		
		if (found_begin) {
			break;
		}
	}
	
	if (i >= VFIFO_SIZE) {
		//USER cannot find working solution 
		return 0;
	}
		
	min_working_p = p;

	//USER  If d is 0 then the working window covers a phase tap and we can follow the old procedure
	//USER 	otherwise, we've found the beginning, and we need to increment the dtaps until we find the end 
	if (d == 0) {
		//USER ********************************************************************
		//USER * step 3a: if we have room, back off by one and increment in dtaps *
		min_working_d = 0;

		//USER Special case code for backing up a phase 
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX ;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
		
		found_begin = 0;
		for (d = 0; d <= dtaps_per_ptap; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);
			
			if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				found_begin = 1;
				min_working_d = d;
				break;
			}
		}
		
		//USER We have found a working dtap before the ptap found above 
		if (found_begin == 1) {
			min_working_p = p;
			max_working_cnt++;
		} 
		
		//USER Restore VFIFO to old state before we decremented it 
		p = p + 1;
		if (p > IO_DQS_EN_PHASE_MAX) {
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}
		
		scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);

		
		//USER ***********************************************************************************
		//USER * step 4a: go forward from working phase to non working phase, increment in ptaps *
		p = p + 1;
		if (p > IO_DQS_EN_PHASE_MAX) {
			//USER fiddle with FIFO
			p = 0;
			rw_mgr_incr_vfifo(grp, &v);
		}
		
		found_end = 0;
		for (; i < VFIFO_SIZE+1; i++) {
			for (; p <= IO_DQS_EN_PHASE_MAX; p++) {
				scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
				
				if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
					found_end = 1;
					break;
				} else {
					max_working_cnt++;
				}
			}
			
			if (found_end) {
				break;
			}
			
			if (p > IO_DQS_EN_PHASE_MAX) {
				//USER fiddle with FIFO
				rw_mgr_incr_vfifo(grp, &v);
				p = 0;
			}		
		}
		
		if (i >= VFIFO_SIZE+1) {
			//USER cannot see edge of failing read 
			return 0;
		}
		
		//USER *********************************************************
		//USER * step 5a:  back off one from last, increment in dtaps  *
		max_working_d = 0;
			
		//USER Special case code for backing up a phase 
		if (p == 0) {
			p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			p = p - 1;
		}
		
		max_working_p = p;
		scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
		
		for (d = 0; d <= IO_DQS_EN_DELAY_MAX; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);
			
			if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}
		
		//USER Go back to working dtap 
		if (d != 0) {
			max_working_d = d - 1;
		} 
	
	} else {

		//USER ********************************************************************
		//USER * step 3-5b:  Find the right edge of the window using delay taps   *		
		
		max_working_p = min_working_p;
		min_working_d = d;
		
		for (; d <= IO_DQS_EN_DELAY_MAX; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}

		//USER Go back to working dtap 
		if (d != 0) {
			max_working_d = d - 1;
		} 
		
		//USER Only here to counterbalance a subtract later on which is not needed if this branch
		//USER of the algorithm is taken 
		max_working_cnt++;		
	}

	//USER ********************************************
	//USER * step 6:  Find the centre of the window   *

	//USER If the number of working phases is even we will step back a phase and find the
	//USER 	edge with a larger delay chain tap 
	if ((max_working_cnt & 1) == 0) {
		p = min_working_p + (max_working_cnt-1)/2;
		
		//USER Special case code for backing up a phase 
		if (max_working_p == 0) {
			max_working_p = IO_DQS_EN_PHASE_MAX;
			rw_mgr_decr_vfifo(grp, &v);
		} else {
			max_working_p = max_working_p - 1;
		}
		
		scc_mgr_set_dqs_en_phase_all_ranks(grp, max_working_p);
		
		//USER Code to determine at which dtap we should start searching again for a failure
		//USER If we've moved back such that the max and min p are the same, we should start searching
		//USER from where the window actually exists
		if (max_working_p == min_working_p) {
			d = min_working_d;
		} else {
			d = max_working_d;
		}
		
		for (; d <= IO_DQS_EN_DELAY_MAX; d++) {
			scc_mgr_set_dqs_en_delay_all_ranks(grp, d);

			if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				break;
			}
		}

		//USER Go back to working dtap 
		if (d != 0) {
			max_working_d = d - 1;
		}
	} else {
		p = min_working_p + (max_working_cnt)/2;
	}
	
	while (p > IO_DQS_EN_PHASE_MAX) {
		p -= (IO_DQS_EN_PHASE_MAX + 1);
	}	
		
	d = (min_working_d + max_working_d)/2;
	
	scc_mgr_set_dqs_en_phase_all_ranks(grp, p);
	scc_mgr_set_dqs_en_delay_all_ranks(grp, d);
	
	//USER push vfifo until we can successfully calibrate 

	for (i = 0; i < VFIFO_SIZE; i++) {
		if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}

		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		return 0;
	}

	return 1;
}

#endif

#else
// Val's original version 

alt_u32 rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase (alt_u32 grp)
{
	alt_u32 i, j, v, d;
	alt_u32 min_working_d, max_working_cnt;
	alt_u32 fail_cnt;
	t_btfld bit_chk;
	alt_u32 delay_per_ptap_mid;

	TRACE_FUNC("%lu", grp);
	
	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);
	
	scc_mgr_set_dqs_en_delay_all_ranks(grp, 0);
	scc_mgr_set_dqs_en_phase_all_ranks(grp, 0);

	fail_cnt = 0;

	//USER first push vfifo until we get a failing read 
	v = 0;
	for (i = 0; i < VFIFO_SIZE; i++) {
		if (!rw_mgr_mem_calibrate_read_test_all_ranks (grp, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			fail_cnt++;

			if (fail_cnt == 2) {
				break;
			}
		}

		//USER fiddle with FIFO
		rw_mgr_incr_vfifo(grp, &v);
	}

	if (v >= VFIFO_SIZE) {
		//USER no failing read found!! Something must have gone wrong

		return 0;
	}

	max_working_cnt = 0;
	min_working_d = 0;

	for (i = 0; i < VFIFO_SIZE+1; i++) {
		for (d = 0; d <= IO_DQS_EN_PHASE_MAX; d++) {
			scc_mgr_set_dqs_en_phase_all_ranks(grp, d);

			rw_mgr_mem_calibrate_read_test_all_ranks (grp, NUM_READ_PB_TESTS, PASS_ONE_BIT, &bit_chk, 0);
			if (bit_chk) {
				//USER passing read 

				if (max_working_cnt == 0) {
					min_working_d = d;
				}

				max_working_cnt++;
			} else {
				if (max_working_cnt > 0) {
					//USER already have one working value 
					break;
				}
			}
		}

		if (d > IO_DQS_EN_PHASE_MAX) {
			//USER fiddle with FIFO
			rw_mgr_incr_vfifo(grp, &v);
		} else {
			//USER found working solution! 

			d = min_working_d + (max_working_cnt - 1) / 2;

			while (d > IO_DQS_EN_PHASE_MAX) {
				d -= (IO_DQS_EN_PHASE_MAX + 1);
			}

			break;
		}
	}

	if (i >= VFIFO_SIZE+1) {
		//USER cannot find working solution or cannot see edge of failing read 

		return 0;
	}

	//USER in the case the number of working steps is even, use 50ps taps to further center the window 

	if ((max_working_cnt & 1) == 0) {
		delay_per_ptap_mid = IO_DELAY_PER_OPA_TAP / 2;

		//USER increment in 50ps taps until we reach the required amount 

		for (i = 0, j = 0; i <= IO_DQS_EN_DELAY_MAX && j < delay_per_ptap_mid; i++, j += IO_DELAY_PER_DQS_EN_DCHAIN_TAP);

		scc_mgr_set_dqs_en_delay_all_ranks(grp, i - 1);
	}

	scc_mgr_set_dqs_en_phase_all_ranks(grp, d);

	//USER push vfifo until we can successfully calibrate 

	for (i = 0; i < VFIFO_SIZE; i++) {
		if (rw_mgr_mem_calibrate_read_test_all_ranks (grp, NUM_READ_PB_TESTS, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		}

		//USER fiddle with FIFO
		rw_mgr_incr_vfifo (grp, &v);
	}

	if (i >= VFIFO_SIZE) {
		return 0;
	}

	return 1;
}

#endif
#endif


// Try rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase across different dq_in_delay values
static inline alt_u32 rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay (alt_u32 write_group, alt_u32 read_group, alt_u32 test_bgn)
{
#if STRATIXV || ARRIAV || CYCLONEV || ARRIAVGZ
	alt_u32 found;
	alt_u32 i;
	alt_u32 p;
	alt_u32 d;
	alt_u32 r;
		
	const alt_u32 delay_step = IO_IO_IN_DELAY_MAX/(RW_MGR_MEM_DQ_PER_READ_DQS-1); /* we start at zero, so have one less dq to devide among */
	
	TRACE_FUNC("(%lu,%lu,%lu)", write_group, read_group, test_bgn);

	// try different dq_in_delays since the dq path is shorter than dqs

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		select_shadow_regs_for_update(r, write_group, 1);
		for (i = 0, p = test_bgn, d = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++, d += delay_step) {
			DPRINT(1, "rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay: g=%lu/%lu r=%lu, i=%lu p=%lu d=%lu",
			       write_group, read_group, r, i, p, d);
			scc_mgr_set_dq_in_delay(write_group, p, d);
			scc_mgr_load_dq (p);
		}
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}

	found = rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(read_group);

	DPRINT(1, "rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay: g=%lu/%lu found=%lu; Reseting delay chain to zero",
	       write_group, read_group, found);

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {
		select_shadow_regs_for_update(r, write_group, 1);
		for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
			scc_mgr_set_dq_in_delay(write_group, p, 0);
			scc_mgr_load_dq (p);
		}
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}

	return found;
#else
	return rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase(read_group);
#endif
}

//USER per-bit deskew DQ and center 

#if NEWVERSION_RDDESKEW

alt_u32 rw_mgr_mem_calibrate_vfifo_center (alt_u32 rank_bgn, alt_u32 write_group, alt_u32 read_group, alt_u32 test_bgn, alt_u32 use_read_test, alt_u32 update_fom)
{
	alt_u32 i, p, d, min_index;
	//USER Store these as signed since there are comparisons with signed numbers
	t_btfld bit_chk;
	t_btfld sticky_bit_chk;
	alt_32 left_edge[RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 right_edge[RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 final_dq[RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_32 mid;
	alt_32 orig_mid_min, mid_min;
	alt_32 new_dqs, start_dqs, start_dqs_en, shift_dq, final_dqs, final_dqs_en;
	alt_32 dq_margin, dqs_margin;
	alt_u32 stop;

	TRACE_FUNC("%lu %lu", read_group, test_bgn);
#if BFM_MODE	
	if (use_read_test) {
		BFM_STAGE("vfifo_center");
	} else {
		BFM_STAGE("vfifo_center_after_writes");
	}
#endif	
	
	ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	start_dqs = READ_SCC_DQS_IN_DELAY(read_group);
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		start_dqs_en = READ_SCC_DQS_EN_DELAY(read_group);
	}
	
	select_curr_shadow_reg_using_rank(rank_bgn);

	//USER per-bit deskew 
		
	//USER set the left and right edge of each bit to an illegal value 
	//USER use (IO_IO_IN_DELAY_MAX + 1) as an illegal value 
	sticky_bit_chk = 0;
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		left_edge[i]  = IO_IO_IN_DELAY_MAX + 1;
		right_edge[i] = IO_IO_IN_DELAY_MAX + 1;
	}
	
	//USER Search for the left edge of the window for each bit
	for (d = 0; d <= IO_IO_IN_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_in_delay (write_group, test_bgn, d);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit
		if (use_read_test) {
			stop = !rw_mgr_mem_calibrate_read_test (rank_bgn, read_group, NUM_READ_PB_TESTS, PASS_ONE_BIT, &bit_chk, 0, 0);
		} else {
			rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ONE_BIT, &bit_chk, 0);    
			bit_chk = bit_chk >> (RW_MGR_MEM_DQ_PER_READ_DQS * (read_group - (write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH)));
			stop = (bit_chk == 0);                                      
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->read_correct_mask);
		DPRINT(2, "vfifo_center(left): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT " && %lu", d, sticky_bit_chk, param->read_correct_mask, stop);
		
		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the left_edge
					left_edge[i] = d;
				} else {
					//USER If a left edge has not been seen yet, then a future passing test will mark this edge as the right edge 
					if (left_edge[i] == IO_IO_IN_DELAY_MAX + 1) {
						right_edge[i] = -(d + 1);
					}
				}
				DPRINT(2, "vfifo_center[l,d=%lu]: bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	//USER Reset DQ delay chains to 0 
	scc_mgr_apply_group_dq_in_delay (write_group, test_bgn, 0);
	sticky_bit_chk = 0;
	for (i = RW_MGR_MEM_DQ_PER_READ_DQS - 1;; i--) {

		DPRINT(2, "vfifo_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i], i, right_edge[i]);

		//USER Check for cases where we haven't found the left edge, which makes our assignment of the the 
		//USER right edge invalid.  Reset it to the illegal value. 
		if ((left_edge[i] == IO_IO_IN_DELAY_MAX + 1) && (right_edge[i] != IO_IO_IN_DELAY_MAX + 1)) {
			right_edge[i] = IO_IO_IN_DELAY_MAX + 1;
			DPRINT(2, "vfifo_center: reset right_edge[%lu]: %ld", i, right_edge[i]);
		}
		
		//USER Reset sticky bit (except for bits where we have seen both the left and right edge) 
		sticky_bit_chk = sticky_bit_chk << 1;
		if ((left_edge[i] != IO_IO_IN_DELAY_MAX + 1) && (right_edge[i] != IO_IO_IN_DELAY_MAX + 1)) {
			sticky_bit_chk = sticky_bit_chk | 1;
		}

		if (i == 0)
		{
			break;
		}
	}
	
	//USER Search for the right edge of the window for each bit 
	for (d = 0; d <= IO_DQS_IN_DELAY_MAX - start_dqs; d++) {
		scc_mgr_set_dqs_bus_in_delay(read_group, d + start_dqs);
		if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
			alt_u32 delay = d + start_dqs_en;
			if (delay > IO_DQS_EN_DELAY_MAX) {
				delay = IO_DQS_EN_DELAY_MAX;
			}
			scc_mgr_set_dqs_en_delay(read_group, delay);
		}
		scc_mgr_load_dqs (read_group);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit 
		if (use_read_test) {
			stop = !rw_mgr_mem_calibrate_read_test (rank_bgn, read_group, NUM_READ_PB_TESTS, PASS_ONE_BIT, &bit_chk, 0, 0);
		} else {
			rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ONE_BIT, &bit_chk, 0);    
			bit_chk = bit_chk >> (RW_MGR_MEM_DQ_PER_READ_DQS * (read_group - (write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH)));
			stop = (bit_chk == 0);   
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->read_correct_mask);

		DPRINT(2, "vfifo_center(right): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT " && %lu", d, sticky_bit_chk, param->read_correct_mask, stop);
		
		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the right_edge 
					right_edge[i] = d;
				} else {
					if (d != 0) {
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge 
						if (right_edge[i] == IO_IO_IN_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
					} else {
						//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
						if (right_edge[i] == IO_IO_IN_DELAY_MAX + 1 && left_edge[i] != IO_IO_IN_DELAY_MAX + 1) {
							right_edge[i] = -1;
						}
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge 
						else if (right_edge[i] == IO_IO_IN_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
						
					}	
				}
				
				DPRINT(2, "vfifo_center[r,d=%lu]: bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	// Store all observed margins
#if ENABLE_TCL_DEBUG
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		alt_u32 dq = read_group*RW_MGR_MEM_DQ_PER_READ_DQS + i;

		ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

		TCLRPT_SET(debug_cal_report->cal_dq_in_margins[curr_shadow_reg][dq].left_edge, left_edge[i]);
		TCLRPT_SET(debug_cal_report->cal_dq_in_margins[curr_shadow_reg][dq].right_edge, right_edge[i]);
	}
#endif

	//USER Check that all bits have a window
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		DPRINT(2, "vfifo_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i], i, right_edge[i]);
		BFM_GBL_SET(dq_read_left_edge[read_group][i],left_edge[i]);
		BFM_GBL_SET(dq_read_right_edge[read_group][i],right_edge[i]);
		if ((left_edge[i] == IO_IO_IN_DELAY_MAX + 1) || (right_edge[i] == IO_IO_IN_DELAY_MAX + 1)) {
		
			//USER Restore delay chain settings before letting the loop in 
			//USER rw_mgr_mem_calibrate_vfifo to retry different dqs/ck relationships
			scc_mgr_set_dqs_bus_in_delay(read_group, start_dqs);
			if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
				scc_mgr_set_dqs_en_delay(read_group, start_dqs_en);
			}	
			scc_mgr_load_dqs (read_group);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		
			DPRINT(1, "vfifo_center: failed to find edge [%lu]: %ld %ld", i, left_edge[i], right_edge[i]);
			if (use_read_test) {
				set_failing_group_stage(read_group*RW_MGR_MEM_DQ_PER_READ_DQS + i, CAL_STAGE_VFIFO, CAL_SUBSTAGE_VFIFO_CENTER);
			} else {
				set_failing_group_stage(read_group*RW_MGR_MEM_DQ_PER_READ_DQS + i, CAL_STAGE_VFIFO_AFTER_WRITES, CAL_SUBSTAGE_VFIFO_CENTER);
			}
			return 0;
		}
	}
	
	//USER Find middle of window for each DQ bit 
	mid_min = left_edge[0] - right_edge[0];
	min_index = 0;
	for (i = 1; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		mid = left_edge[i] - right_edge[i];
		if (mid < mid_min) {
			mid_min = mid;
			min_index = i;
		}
	}

	//USER  -mid_min/2 represents the amount that we need to move DQS.  If mid_min is odd and positive we'll need to add one to
	//USER make sure the rounding in further calculations is correct (always bias to the right), so just add 1 for all positive values
	if (mid_min > 0) {
		mid_min++;
	}
	mid_min = mid_min / 2;

	DPRINT(1, "vfifo_center: mid_min=%ld (index=%lu)", mid_min, min_index);
	
	//USER Determine the amount we can change DQS (which is -mid_min)
	orig_mid_min = mid_min;
#if ENABLE_DQS_IN_CENTERING
	new_dqs = start_dqs - mid_min;
	if (new_dqs > IO_DQS_IN_DELAY_MAX) {
		new_dqs = IO_DQS_IN_DELAY_MAX;
	} else if (new_dqs < 0) {
		new_dqs = 0;
	} 
	mid_min = start_dqs - new_dqs;
	DPRINT(1, "vfifo_center: new mid_min=%ld new_dqs=%ld", mid_min, new_dqs);
	
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		if (start_dqs_en - mid_min > IO_DQS_EN_DELAY_MAX) {
			mid_min += start_dqs_en - mid_min - IO_DQS_EN_DELAY_MAX;
		} else if (start_dqs_en - mid_min < 0) {
			mid_min += start_dqs_en - mid_min;
		}
	}
	new_dqs = start_dqs - mid_min;
#else
	new_dqs = start_dqs;
	mid_min = 0;
#endif

	DPRINT(1, "vfifo_center: start_dqs=%ld start_dqs_en=%ld new_dqs=%ld mid_min=%ld",
	       start_dqs, IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS ? start_dqs_en : -1, new_dqs, mid_min);
	
	//USER Initialize data for export structures 
	dqs_margin = IO_IO_IN_DELAY_MAX + 1;
	dq_margin  = IO_IO_IN_DELAY_MAX + 1;
	
	//USER add delay to bring centre of all DQ windows to the same "level" 
	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		//USER Use values before divide by 2 to reduce round off error 
		shift_dq = (left_edge[i] - right_edge[i] - (left_edge[min_index] - right_edge[min_index]))/2  + (orig_mid_min - mid_min);

		DPRINT(2, "vfifo_center: before: shift_dq[%lu]=%ld", i, shift_dq);
		
		if (shift_dq + (alt_32)READ_SCC_DQ_IN_DELAY(p) > (alt_32)IO_IO_IN_DELAY_MAX) {
			shift_dq = (alt_32)IO_IO_IN_DELAY_MAX - READ_SCC_DQ_IN_DELAY(i);
		} else if (shift_dq + (alt_32)READ_SCC_DQ_IN_DELAY(p) < 0) {
			shift_dq = -(alt_32)READ_SCC_DQ_IN_DELAY(p);
		} 
		DPRINT(2, "vfifo_center: after: shift_dq[%lu]=%ld", i, shift_dq);
		final_dq[i] = READ_SCC_DQ_IN_DELAY(p) + shift_dq;
		scc_mgr_set_dq_in_delay(write_group, p, final_dq[i]);
		scc_mgr_load_dq (p);
		
		DPRINT(2, "vfifo_center: margin[%lu]=[%ld,%ld]", i,
		       left_edge[i] - shift_dq + (-mid_min),
		       right_edge[i] + shift_dq - (-mid_min));
		//USER To determine values for export structures 
		if (left_edge[i] - shift_dq + (-mid_min) < dq_margin) {
			dq_margin = left_edge[i] - shift_dq + (-mid_min);
		}
		if (right_edge[i] + shift_dq - (-mid_min) < dqs_margin) {
			dqs_margin = right_edge[i] + shift_dq - (-mid_min);
		}
	}

#if ENABLE_DQS_IN_CENTERING	
	final_dqs = new_dqs;
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		final_dqs_en = start_dqs_en - mid_min;
	}
#else
	final_dqs = start_dqs;
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		final_dqs_en = start_dqs_en;
	}
#endif	

	//USER Move DQS-en
	if (IO_SHIFT_DQS_EN_WHEN_SHIFT_DQS) {
		scc_mgr_set_dqs_en_delay(read_group, final_dqs_en);
		scc_mgr_load_dqs (read_group);
	}
	
#if QDRII || RLDRAMX	
	//USER Move DQS. Do it gradually to minimize the chance of causing a timing
	//USER failure in core FPGA logic driven by an input-strobe-derived clock
	d = READ_SCC_DQS_IN_DELAY(read_group);
	while (d != final_dqs) {
		if (d > final_dqs) {
			--d;
		} else {
			++d;
		}
		scc_mgr_set_dqs_bus_in_delay(read_group, d);
		scc_mgr_load_dqs (read_group);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}
#else
	//USER Move DQS
	scc_mgr_set_dqs_bus_in_delay(read_group, final_dqs);
	scc_mgr_load_dqs (read_group);
#endif

    if(update_fom) {
	//USER Export values 
	gbl->fom_in += (dq_margin + dqs_margin)/(RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
    	TCLRPT_SET(debug_summary_report->fom_in, debug_summary_report->fom_in + (dq_margin + dqs_margin)/(RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH));
	    TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][write_group].fom_in, debug_cal_report->cal_status_per_group[curr_shadow_reg][write_group].fom_in + (dq_margin + dqs_margin)/(RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH));
    }

	TCLRPT_SET(debug_cal_report->cal_dqs_in_margins[curr_shadow_reg][read_group].dqs_margin, dqs_margin);
	TCLRPT_SET(debug_cal_report->cal_dqs_in_margins[curr_shadow_reg][read_group].dq_margin, dq_margin);

	DPRINT(2, "vfifo_center: dq_margin=%ld dqs_margin=%ld", dq_margin, dqs_margin);
	
#if RUNTIME_CAL_REPORT
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		if (use_read_test) {
			RPRINT("Read Deskew ; DQ %2lu ; Rank %lu ; Left edge %3li ; Right edge %3li ; DQ delay %2li ; DQS delay %2li", read_group*RW_MGR_MEM_DQ_PER_READ_DQS + i, curr_shadow_reg, left_edge[i],  right_edge[i], final_dq[i], final_dqs);
		} else {
			RPRINT("Read after Write ; DQ %2lu ; Rank %lu ; Left edge %3li ; Right edge %3li ; DQ delay %2li ; DQS delay %2li", read_group*RW_MGR_MEM_DQ_PER_READ_DQS + i, curr_shadow_reg, left_edge[i],  right_edge[i], final_dq[i], final_dqs);
		}
	}
#endif

	//USER Do not remove this line as it makes sure all of our decisions have been applied
	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);	
	return (dq_margin >= 0) && (dqs_margin >= 0);
}

#else

alt_u32 rw_mgr_mem_calibrate_vfifo_center (alt_u32 rank_bgn, alt_u32 grp, alt_u32 test_bgn, alt_u32 use_read_test)
{
	alt_u32 i, p, d;
	alt_u32 mid;
	t_btfld bit_chk;
	alt_u32 max_working_dq[RW_MGR_MEM_DQ_PER_READ_DQS];
	alt_u32 dq_margin, dqs_margin;
	alt_u32 start_dqs;

	TRACE_FUNC("%lu %lu", grp, test_bgn);
	
	//USER per-bit deskew.
	//USER start of the per-bit sweep with the minimum working delay setting for
	//USER all bits.

	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		max_working_dq[i] = 0;
	}

	for (d = 1; d <= IO_IO_IN_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_in_delay (write_group, test_bgn, d);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		if (!rw_mgr_mem_calibrate_read_test (rank_bgn, grp, NUM_READ_PB_TESTS, PASS_ONE_BIT, &bit_chk, 0, 0)) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
				if (bit_chk & 1) {
					max_working_dq[i] = d;
				}
				bit_chk = bit_chk >> 1;
			}
		}
	}

	//USER determine minimum working value for DQ 

	dq_margin = IO_IO_IN_DELAY_MAX;

	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		if (max_working_dq[i] < dq_margin) {
			dq_margin = max_working_dq[i];
		}
	}

	//USER add delay to bring all DQ windows to the same "level" 

	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
		if (max_working_dq[i] > dq_margin) {
			scc_mgr_set_dq_in_delay(write_group, i, max_working_dq[i] - dq_margin);
		} else {
			scc_mgr_set_dq_in_delay(write_group, i, 0);
		}

		scc_mgr_load_dq (p, p);
	}

	//USER sweep DQS window, may potentially have more window due to per-bit-deskew that was done
	//USER in the previous step.

	start_dqs = READ_SCC_DQS_IN_DELAY(grp);

	for (d = start_dqs + 1; d <= IO_DQS_IN_DELAY_MAX; d++) {
		scc_mgr_set_dqs_bus_in_delay(grp, d);
		scc_mgr_load_dqs (grp);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		if (!rw_mgr_mem_calibrate_read_test (rank_bgn, grp, NUM_READ_TESTS, PASS_ALL_BITS, &bit_chk, 0, 0)) {
			break;
		}
	}

	scc_mgr_set_dqs_bus_in_delay(grp, start_dqs);

	//USER margin on the DQS pin 

	dqs_margin = d - start_dqs - 1;

	//USER find mid point, +1 so that we don't go crazy pushing DQ 

	mid = (dq_margin + dqs_margin + 1) / 2;

	gbl->fom_in += dq_margin + dqs_margin;
//	TCLRPT_SET(debug_summary_report->fom_in, debug_summary_report->fom_in + (dq_margin + dqs_margin));
//	TCLRPT_SET(debug_cal_report->cal_status_per_group[grp].fom_in, (dq_margin + dqs_margin));

	
	

#if ENABLE_DQS_IN_CENTERING
	//USER center DQS ... if the headroom is setup properly we shouldn't need to 

	if (dqs_margin > mid) {
		scc_mgr_set_dqs_bus_in_delay(grp, READ_SCC_DQS_IN_DELAY(grp) + dqs_margin - mid);

		if (DDRX) {
			alt_u32 delay = READ_SCC_DQS_EN_DELAY(grp) + dqs_margin - mid;

			if (delay > IO_DQS_EN_DELAY_MAX) {
				delay = IO_DQS_EN_DELAY_MAX;
			}

			scc_mgr_set_dqs_en_delay(grp, delay);
		}
	}
#endif

	scc_mgr_load_dqs (grp);

	//USER center DQ 

	if (dq_margin > mid) {
		for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++, p++) {
			scc_mgr_set_dq_in_delay(write_group, i, READ_SCC_DQ_IN_DELAY(i) + dq_margin - mid);
			scc_mgr_load_dq (p, p);
		}

		dqs_margin += dq_margin - mid;
		dq_margin  -= dq_margin - mid;
	}

	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

	return (dq_margin + dqs_margin) > 0;
}

#endif

//USER calibrate the read valid prediction FIFO.
//USER 
//USER  - read valid prediction will consist of finding a good DQS enable phase, DQS enable delay, DQS input phase, and DQS input delay.
//USER  - we also do a per-bit deskew on the DQ lines.

#if DYNAMIC_CALIBRATION_MODE || STATIC_QUICK_CALIBRATION

#if !ENABLE_SUPER_QUICK_CALIBRATION

//USER VFIFO Calibration -- Quick Calibration
alt_u32 rw_mgr_mem_calibrate_vfifo (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 v, d, i;
	alt_u32 found;
	t_btfld bit_chk;

	TRACE_FUNC("%lu %lu", grp, test_bgn);
	
	//USER update info for sims 

	reg_file_set_stage(CAL_STAGE_VFIFO);

	//USER Load up the patterns used by read calibration 

	rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();
	
	//USER maximum phase values for the sweep 


	//USER update info for sims 

	reg_file_set_group(g);

	found = 0;
	v = 0;
	for (i = 0; i < VFIFO_SIZE && found == 0; i++) {
		for (d = 0; d <= IO_DQS_EN_PHASE_MAX && found == 0; d++) {
			if (DDRX)
			{
				scc_mgr_set_dqs_en_phase_all_ranks(g, d);
			}
			
			//USER calibrate the vfifo with the current dqs enable phase setting 

			if (rw_mgr_mem_calibrate_read_test_all_ranks (g, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				found = 1;
			}
		}

		if (found) {
			break;
		} else {
			rw_mgr_incr_vfifo_all (g, &v);
		}
	}

	return found;
}

#else

//USER VFIFO Calibration -- Super Quick Calibration
alt_u32 rw_mgr_mem_calibrate_vfifo (alt_u32 grp, alt_u32 test_bgn2)
{
	alt_u32 g, v, d, i;
	alt_u32 test_bgn;
	alt_u32 found;
	t_btfld bit_chk;
	alt_u32 phase_increment;
	alt_u32 final_v_setting = 0;
	alt_u32 final_d_setting = 0;
	
	TRACE_FUNC("%lu %lu", grp, test_bgn2);
	
	#if ARRIAV || CYCLONEV
	    // Compensate for simulation model behaviour 
	    for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
	    	scc_mgr_set_dqs_bus_in_delay(i, 10);
	    	scc_mgr_load_dqs (i);
	    }
	    IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	#endif
	
	//USER The first call to this function will calibrate all groups
	if (grp !=0) {
		return 1;
	}

	//USER update info for sims 

	reg_file_set_stage(CAL_STAGE_VFIFO);

	//USER Load up the patterns used by read calibration 

	rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();

	//USER maximum phase values for the sweep 

	//USER Calibrate group 0
	g = 0;
	test_bgn = 0;

	//USER update info for sims

	reg_file_set_group(g);

	found = 0;

	//USER In behavioral simulation only phases 0 and IO_DQS_EN_PHASE_MAX/2 are relevant
	//USER All other values produces the same results as those 2, so there's really no
	//USER point in sweeping them all
	phase_increment = (IO_DQS_EN_PHASE_MAX + 1) / 2;
	//USER Make sure phase_increment is > 0 to prevent infinite loop
	if (phase_increment == 0) phase_increment++;

	v = 0;
	for (i = 0; i < VFIFO_SIZE && found == 0; i++) {
		for (d = 0; d <= IO_DQS_EN_PHASE_MAX && found == 0; d += phase_increment) {

			scc_mgr_set_dqs_en_phase_all_ranks(g, d);

			//USER calibrate the vfifo with the current dqs enable phase setting 

			if (rw_mgr_mem_calibrate_read_test_all_ranks (g, 1, PASS_ONE_BIT, &bit_chk, 0)) {
				found = 1;
				final_v_setting = v;
				final_d_setting = d;
			}
		}

		if (!found) {
			rw_mgr_incr_vfifo_all (g, &v);
		} else {
			break;
		}
	}

	if (!found) return 0;

	//USER Now copy the calibration settings to all other groups
	for (g = 1, test_bgn = RW_MGR_MEM_DQ_PER_READ_DQS; (g < RW_MGR_MEM_IF_READ_DQS_WIDTH) && found; g++, test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS) {
		//USER Set the VFIFO
		v = 0;
		for (i = 0; i < final_v_setting; i++) {
			rw_mgr_incr_vfifo_all (g, &v);
		}

		//USER Set the proper phase
		IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, g);
		scc_mgr_set_dqs_en_phase_all_ranks(g, final_d_setting);

		//USER Verify that things worked as expected
		if(!rw_mgr_mem_calibrate_read_test_all_ranks (g, 1, PASS_ONE_BIT, &bit_chk, 0)) {
			//USER Fail
			found = 0;
		}
	}

	IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, 0);
	return found;
}

#endif
#endif

#if DYNAMIC_CALIBRATION_MODE || STATIC_FULL_CALIBRATION

#if NEWVERSION_GW

//USER VFIFO Calibration -- Full Calibration
alt_u32 rw_mgr_mem_calibrate_vfifo (alt_u32 read_group, alt_u32 test_bgn)
{
	alt_u32 p, d, rank_bgn, sr;
	alt_u32 dtaps_per_ptap;
	alt_u32 tmp_delay;
	t_btfld bit_chk;
	alt_u32 grp_calibrated;
	alt_u32 write_group, write_test_bgn;
	alt_u32 failed_substage;
	alt_u32 dqs_in_dtaps, orig_start_dqs;

	TRACE_FUNC("%lu %lu", read_group, test_bgn);
	
	//USER update info for sims 

	reg_file_set_stage(CAL_STAGE_VFIFO);

	if (DDRX) {
		write_group = read_group;
		write_test_bgn = test_bgn;
	} else {
		write_group = read_group / (RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
		write_test_bgn = read_group * RW_MGR_MEM_DQ_PER_READ_DQS;
	}
	
	// USER Determine number of delay taps for each phase tap
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	if (!QDRII) {
		while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
			dtaps_per_ptap++;
			tmp_delay += IO_DELAY_PER_DQS_EN_DCHAIN_TAP;
		}
		dtaps_per_ptap--;
		tmp_delay = 0;
	}

	//USER update info for sims 

	reg_file_set_group(read_group);

	grp_calibrated = 0;
	
	reg_file_set_sub_stage(CAL_SUBSTAGE_GUARANTEED_READ);
	failed_substage = CAL_SUBSTAGE_GUARANTEED_READ;

	for (d = 0; d <= dtaps_per_ptap && grp_calibrated == 0; d+=2) {

		if (DDRX || RLDRAMX) {
			// In RLDRAMX we may be messing the delay of pins in the same write group but outside of
			// the current read group, but that's ok because we haven't calibrated the output side yet.
			if (d > 0) {
				scc_mgr_apply_group_all_out_delay_add_all_ranks (write_group, write_test_bgn, d);
			}
		}

		for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX && grp_calibrated == 0; p++) {
			//USER set a particular dqdqs phase 
			if (DDRX) {
				scc_mgr_set_dqdqs_output_phase_all_ranks(read_group, p);
			}
		
			//USER Previous iteration may have failed as a result of ck/dqs or ck/dk violation,
			//USER in which case the device may require special recovery.
			if (DDRX || RLDRAMX) {
				if (d != 0 || p != 0) {
					recover_mem_device_after_ck_dqs_violation();
				}
			}

			DPRINT(1, "calibrate_vfifo: g=%lu p=%lu d=%lu", read_group, p, d);
			BFM_GBL_SET(gwrite_pos[read_group].p, p);
			BFM_GBL_SET(gwrite_pos[read_group].d, d);

			//USER Load up the patterns used by read calibration using current DQDQS phase 

#if BFM_MODE
			// handled by pre-initializing memory if skipping
			if (bfm_gbl.bfm_skip_guaranteed_write == 0) {
				rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();
			}
#else
			rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();
			
#if DDRX
#if !AP_MODE
			if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_DISABLE_GUARANTEED_READ)) {
				if (!rw_mgr_mem_calibrate_read_test_patterns_all_ranks (read_group, 1, &bit_chk)) {
					DPRINT(1, "Guaranteed read test failed: g=%lu p=%lu d=%lu", read_group, p, d);
					break;
				}
			}
#endif
#endif
#endif

#if ARRIAV || CYCLONEV
			///////
			// To make DQS bypass able to calibrate more often
			///////
			// Loop over different DQS in delay chains for the purpose of DQS Enable calibration finding one bit working
			orig_start_dqs = READ_SCC_DQS_IN_DELAY(read_group);	
			for (dqs_in_dtaps = orig_start_dqs; dqs_in_dtaps <= IO_DQS_IN_DELAY_MAX && grp_calibrated == 0; dqs_in_dtaps++) {
			
				for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS; rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {

					if (! param->skip_shadow_regs[sr]) {
						
						//USER Select shadow register set
						select_shadow_regs_for_update(rank_bgn, read_group, 1);

						WRITE_SCC_DQS_IN_DELAY(read_group, dqs_in_dtaps);
						scc_mgr_load_dqs (read_group);
						IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
					}
				}
#endif				
				
// case:56390
#if 0 && ARRIAV && QDRII
				// Note, much of this counts on the fact that we don't need to keep track
				// of what vfifo offset we are at because incr_vfifo doesn't use it
				// We also assume only a single group, and that the vfifo incrementers start at offset zero

#define BIT(w,b) (((w) >> (b)) & 1)
				{
					alt_u32 prev;
					alt_u32 vbase;
					alt_u32 i;

					grp_calibrated = 0;

					// check every combination of vfifo relative settings
					for (prev = vbase = 0; vbase < (1 << VFIFO_CONTROL_WIDTH_PER_DQS); prev=vbase, vbase++ ) {
						// check each bit to see if we need to increment, decrement, or leave the corresponding vfifo alone
						for (i = 0; i < VFIFO_CONTROL_WIDTH_PER_DQS; i++) {
							if (BIT(vbase,i) > BIT(prev,i)) {
								rw_mgr_incr_vfifo(read_group*VFIFO_CONTROL_WIDTH_PER_DQS + i,0);
							} else if (BIT(vbase,i) < BIT(prev,i)) {
								rw_mgr_decr_vfifo(read_group*VFIFO_CONTROL_WIDTH_PER_DQS + i,0);
							}
						}
						if (rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay (write_group, read_group, test_bgn)) {
						
#if ARRIAV || CYCLONEV						
							///////
							// To make DQS bypass able to calibrate more often
							///////
							// Before doing read deskew, set DQS in back to the reserve value
							WRITE_SCC_DQS_IN_DELAY(read_group, orig_start_dqs);
							scc_mgr_load_dqs (read_group);
							IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);						
#endif
						
							if (! rw_mgr_mem_calibrate_vfifo_center (0, write_group, read_group, test_bgn, 1)) {
								// remember last failed stage
								failed_substage = CAL_SUBSTAGE_VFIFO_CENTER;
							} else {
								grp_calibrated = 1;
							}
						} else {
							failed_substage = CAL_SUBSTAGE_DQS_EN_PHASE;
						}
						if (grp_calibrated) {
							break;
						}

						break; // comment out for fix

					}
				}
#else				
				grp_calibrated = 1;
				if (rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay (write_group, read_group, test_bgn)) {
					// USER Read per-bit deskew can be done on a per shadow register basis
					for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS; rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {
#if RUNTIME_CAL_REPORT		
						//Report print can cause a delay at each instance of rw_mgr_mem_calibrate_vfifo_center, need to re-issue guaranteed write to ensure no refresh violation
						rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();
#endif				
						//USER Determine if this set of ranks should be skipped entirely
						if (! param->skip_shadow_regs[sr]) {
						
							//USER Select shadow register set
							select_shadow_regs_for_update(rank_bgn, read_group, 1);
							
#if ARRIAV || CYCLONEV
							///////
							// To make DQS bypass able to calibrate more often
							///////
							// Before doing read deskew, set DQS in back to the reserve value
							WRITE_SCC_DQS_IN_DELAY(read_group, orig_start_dqs);
							scc_mgr_load_dqs (read_group);
							IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
#endif
					
							// If doing read after write calibration, do not update FOM now - do it then
#if READ_AFTER_WRITE_CALIBRATION
							if (! rw_mgr_mem_calibrate_vfifo_center (rank_bgn, write_group, read_group, test_bgn, 1, 0)) {
#else
							if (! rw_mgr_mem_calibrate_vfifo_center (rank_bgn, write_group, read_group, test_bgn, 1, 1)) {
#endif
								grp_calibrated = 0;
								failed_substage = CAL_SUBSTAGE_VFIFO_CENTER;
							}
						}
					}
				} else {
					grp_calibrated = 0;
					failed_substage = CAL_SUBSTAGE_DQS_EN_PHASE;
				}
#endif
#if BFM_MODE
				if (bfm_gbl.bfm_skip_guaranteed_write > 0 && !grp_calibrated) {
					// This should never happen with pre-initialized guaranteed write load pattern
					// unless calibration was always going to fail
					DPRINT(0, "calibrate_vfifo: skip guaranteed write calibration failed");
					break;
				} else if (bfm_gbl.bfm_skip_guaranteed_write == -1) {
					// if skip value is -1, then we expect to fail, but we want to use
					// the regular guaranteed write next time
					if (grp_calibrated) {
						// We shouldn't be succeeding for this test, so this is an error
						DPRINT(0, "calibrate_vfifo: ERROR: skip guaranteed write == -1, but calibration passed");
						grp_calibrated = 0;
						break;
					} else {
						DPRINT(0, "calibrate_vfifo: skip guaranteed write == -1, expected failure, trying again with no skip");
						bfm_gbl.bfm_skip_guaranteed_write = 0;
					}
				}

#endif
#if ARRIAV || CYCLONEV
			///////
			// To make DQS bypass able to calibrate more often
			///////
			}
#endif			
		
		}
#if BFM_MODE
		if (bfm_gbl.bfm_skip_guaranteed_write && !grp_calibrated) break;
#endif
	}

	if (grp_calibrated == 0) {
		set_failing_group_stage(write_group, CAL_STAGE_VFIFO, failed_substage);

		return 0;
	}

	//USER Reset the delay chains back to zero if they have moved > 1 (check for > 1 because loop will increase d even when pass in first case)
	if (DDRX || RLDRAMII) {
		if (d > 2) {
			scc_mgr_zero_group(write_group, write_test_bgn, 1);
		}
	}


	return 1;
}

#else

//USER VFIFO Calibration -- Full Calibration
alt_u32 rw_mgr_mem_calibrate_vfifo (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 p, rank_bgn, sr;
	alt_u32 grp_calibrated;
	alt_u32 failed_substage;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	//USER update info for sims 
	
	reg_file_set_stage(CAL_STAGE_VFIFO);

	reg_file_set_sub_stage(CAL_SUBSTAGE_GUARANTEED_READ);

	failed_substage = CAL_SUBSTAGE_GUARANTEED_READ;

	//USER update info for sims 

	reg_file_set_group(g);

	grp_calibrated = 0;

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX && grp_calibrated == 0; p++) {
		//USER set a particular dqdqs phase 
		if (DDRX) {
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);
		}

		//USER Load up the patterns used by read calibration using current DQDQS phase 

		rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();
#if DDRX
		if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_DISABLE_GUARANTEED_READ)) {
			if (!rw_mgr_mem_calibrate_read_test_patterns_all_ranks (read_group, 1, &bit_chk)) {
				break;
			}
		}
#endif

		grp_calibrated = 1;
		if (rw_mgr_mem_calibrate_vfifo_find_dqs_en_phase_sweep_dq_in_delay (g, g, test_bgn)) {
			// USER Read per-bit deskew can be done on a per shadow register basis
			for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS; rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {
			
				//USER Determine if this set of ranks should be skipped entirely
				if (! param->skip_shadow_regs[sr]) {
				
					//USER Select shadow register set
					select_shadow_regs_for_update(rank_bgn, read_group, 1);
			
					if (! rw_mgr_mem_calibrate_vfifo_center (rank_bgn, g, test_bgn, 1)) {
						grp_calibrated = 0;
						failed_substage = CAL_SUBSTAGE_VFIFO_CENTER;
					}
				}
			}
		} else {
			grp_calibrated = 0;
			failed_substage = CAL_SUBSTAGE_DQS_EN_PHASE;
		}
	}

	if (grp_calibrated == 0) {
		set_failing_group_stage(g, CAL_STAGE_VFIFO, failed_substage);
		return 0;
	}


	return 1;
}

#endif

#endif

#if READ_AFTER_WRITE_CALIBRATION
//USER VFIFO Calibration -- Read Deskew Calibration after write deskew
alt_u32 rw_mgr_mem_calibrate_vfifo_end (alt_u32 read_group, alt_u32 test_bgn)
{
	alt_u32 rank_bgn, sr;
	alt_u32 grp_calibrated;
	alt_u32 write_group;

	TRACE_FUNC("%lu %lu", read_group, test_bgn);
	
	//USER update info for sims 

	reg_file_set_stage(CAL_STAGE_VFIFO_AFTER_WRITES);
	reg_file_set_sub_stage(CAL_SUBSTAGE_VFIFO_CENTER);	

	if (DDRX) {
		write_group = read_group;
	} else {
		write_group = read_group / (RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	}
	
	//USER update info for sims 
	reg_file_set_group(read_group);

	grp_calibrated = 1;
	// USER Read per-bit deskew can be done on a per shadow register basis
	for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS; rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {
	
		//USER Determine if this set of ranks should be skipped entirely
		if (! param->skip_shadow_regs[sr]) {
		
			//USER Select shadow register set
			select_shadow_regs_for_update(rank_bgn, read_group, 1);
	
            // This is the last calibration round, update FOM here
			if (! rw_mgr_mem_calibrate_vfifo_center (rank_bgn, write_group, read_group, test_bgn, 0, 1)) {
				grp_calibrated = 0;
			}
		}
	}


	if (grp_calibrated == 0) {
		set_failing_group_stage(write_group, CAL_STAGE_VFIFO_AFTER_WRITES, CAL_SUBSTAGE_VFIFO_CENTER);
		return 0;
	}

	return 1;
}
#endif


//USER Calibrate LFIFO to find smallest read latency

alt_u32 rw_mgr_mem_calibrate_lfifo (void)
{
	alt_u32 found_one;
	t_btfld bit_chk;
	alt_u32 g;

	TRACE_FUNC();
	BFM_STAGE("lfifo");
	
	//USER update info for sims 

	reg_file_set_stage(CAL_STAGE_LFIFO);
	reg_file_set_sub_stage(CAL_SUBSTAGE_READ_LATENCY);

	//USER Load up the patterns used by read calibration for all ranks

	rw_mgr_mem_calibrate_read_load_patterns_all_ranks ();

	found_one = 0;

	do {
		IOWR_32DIRECT (PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
		DPRINT(2, "lfifo: read_lat=%lu", gbl->curr_read_lat);

		if (!rw_mgr_mem_calibrate_read_test_all_ranks (0, NUM_READ_TESTS, PASS_ALL_BITS, &bit_chk, 1)) {
			break;
		}

		found_one = 1;
		
		//USER reduce read latency and see if things are working
		//USER correctly

		gbl->curr_read_lat--;
	} while (gbl->curr_read_lat > 0);

	//USER reset the fifos to get pointers to known state 

	IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);
   
#if SET_FIX_READ_LATENCY_ENABLE      	 
   if(gbl->curr_read_lat < (FIX_READ_LATENCY -1) ) {
      gbl->curr_read_lat = FIX_READ_LATENCY -2;  	
   } else {
     //Mark as fail by changing found_one back to 0
      found_one = 0 ;
   }
#endif 

	if (found_one) {
		//USER add a fudge factor to the read latency that was determined 
		gbl->curr_read_lat += 2;
#if BFM_MODE
		gbl->curr_read_lat += BFM_GBL_GET(lfifo_margin);
#endif
		IOWR_32DIRECT (PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
#if RUNTIME_CAL_REPORT		
		RPRINT("LFIFO Calibration ; PHY Read Latency %li", gbl->curr_read_lat);
#endif

		DPRINT(2, "lfifo: success: using read_lat=%lu", gbl->curr_read_lat);

		return 1;
	} else {
		set_failing_group_stage(0xff, CAL_STAGE_LFIFO, CAL_SUBSTAGE_READ_LATENCY);

		for (g = 0; g < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; g++)
		{
			TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][g].error_stage, CAL_STAGE_LFIFO);
			TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][g].error_sub_stage, CAL_SUBSTAGE_READ_LATENCY);
		}

		DPRINT(2, "lfifo: failed at initial read_lat=%lu", gbl->curr_read_lat);
		
		return 0;
	}
}

//USER issue write test command.
//USER two variants are provided. one that just tests a write pattern and another that
//USER tests datamask functionality.

#if QDRII
void rw_mgr_mem_calibrate_write_test_issue (alt_u32 group, alt_u32 test_dm)
{
	alt_u32 quick_write_mode = (((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES) && ENABLE_SUPER_QUICK_CALIBRATION) || BFM_MODE;

	//USER CNTR 1 - This is used to ensure enough time elapses for read data to come back.
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x30);

	if (test_dm) {
		IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
		if(quick_write_mode) {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x08);
		} else {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x40);
		}
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_WAIT);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, (group) << 2, __RW_MGR_LFSR_WR_RD_DM_BANK_0);
	} else {
		IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);
		if(quick_write_mode) {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x08);
		} else {
			IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x40);
		}
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_LFSR_WR_RD_BANK_0);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_LFSR_WR_RD_BANK_0_WAIT);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, (group) << 2, __RW_MGR_LFSR_WR_RD_BANK_0);
	}
}
#else
void rw_mgr_mem_calibrate_write_test_issue (alt_u32 group, alt_u32 test_dm)
{
	alt_u32 mcc_instruction;
	alt_u32 quick_write_mode = (((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES) && ENABLE_SUPER_QUICK_CALIBRATION) || BFM_MODE;
	alt_u32 rw_wl_nop_cycles;

	//USER Set counter and jump addresses for the right
	//USER number of NOP cycles.
	//USER The number of supported NOP cycles can range from -1 to infinity
	//USER Three different cases are handled:
	//USER
	//USER 1. For a number of NOP cycles greater than 0, the RW Mgr looping
	//USER    mechanism will be used to insert the right number of NOPs
	//USER
	//USER 2. For a number of NOP cycles equals to 0, the micro-instruction
	//USER    issuing the write command will jump straight to the micro-instruction
	//USER    that turns on DQS (for DDRx), or outputs write data (for RLD), skipping
	//USER    the NOP micro-instruction all together
	//USER
	//USER 3. A number of NOP cycles equal to -1 indicates that DQS must be turned
	//USER    on in the same micro-instruction that issues the write command. Then we need
	//USER    to directly jump to the micro-instruction that sends out the data
	//USER
	//USER NOTE: Implementing this mechanism uses 2 RW Mgr jump-counters (2 and 3). One
	//USER       jump-counter (0) is used to perform multiple write-read operations.
	//USER       one counter left to issue this command in "multiple-group" mode.
	
#if MULTIPLE_AFI_WLAT
	rw_wl_nop_cycles = gbl->rw_wl_nop_cycles_per_group[group];
#else
	rw_wl_nop_cycles = gbl->rw_wl_nop_cycles;
#endif	

	if(rw_wl_nop_cycles == -1)
	{
		#if DDRX
		//USER CNTR 2 - We want to execute the special write operation that
		//USER turns on DQS right away and then skip directly to the instruction that
		//USER sends out the data. We set the counter to a large number so that the
		//USER jump is always taken
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0xFF);

		//USER CNTR 3 - Not used
		if(test_dm)
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0_WL_1;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_DATA);
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_NOP);
		}
		else
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0_WL_1;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_BANK_0_DATA);
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_BANK_0_NOP);
		}
		
		#endif
	} 
	else if(rw_wl_nop_cycles == 0)
	{
		#if DDRX
		//USER CNTR 2 - We want to skip the NOP operation and go straight to
		//USER the DQS enable instruction. We set the counter to a large number so that the
		//USER jump is always taken
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0xFF);

		//USER CNTR 3 - Not used
		if(test_dm)
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_DQS);
		}
		else
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_BANK_0_DQS);
		}
		#endif
		
		#if RLDRAMX
		//USER CNTR 2 - We want to skip the NOP operation and go straight to
		//USER the write data instruction. We set the counter to a large number so that the
		//USER jump is always taken
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0xFF);

		//USER CNTR 3 - Not used
		if(test_dm)
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_DATA);
		}
		else
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_LFSR_WR_RD_BANK_0_DATA);
		}		
		#endif
	}
	else
	{
		//USER CNTR 2 - In this case we want to execute the next instruction and NOT
		//USER take the jump. So we set the counter to 0. The jump address doesn't count
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x0);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, 0x0);

		//USER CNTR 3 - Set the nop counter to the number of cycles we need to loop for, minus 1
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, rw_wl_nop_cycles - 1);
		if(test_dm)
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_DM_BANK_0;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_NOP);
		}
		else
		{
			mcc_instruction = __RW_MGR_LFSR_WR_RD_BANK_0;
			IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_LFSR_WR_RD_BANK_0_NOP);
		}
	}

	IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);

	if(quick_write_mode) {
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x08);
	} else {
#if ENABLE_NON_DES_CAL
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x08); // Break this up for refresh purposes
#else
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x40);
#endif
	}
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, mcc_instruction);

	//USER CNTR 1 - This is used to ensure enough time elapses for read data to come back.
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x30);

	if(test_dm)
	{
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_LFSR_WR_RD_DM_BANK_0_WAIT);
	} else {
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_LFSR_WR_RD_BANK_0_WAIT);
	}

	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, (group << 2), mcc_instruction);

#if ENABLE_NON_DES_CAL	
	alt_u32 i = 0;
	for (i=0; i < 8; i++)
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, (group << 2), mcc_instruction);
#endif	

	





}
#endif

//USER Test writes, can check for a single bit pass or multiple bit pass

alt_u32 rw_mgr_mem_calibrate_write_test (alt_u32 rank_bgn, alt_u32 write_group, alt_u32 use_dm, alt_u32 all_correct, t_btfld *bit_chk, alt_u32 all_ranks)
{
	alt_u32 r;
	t_btfld correct_mask_vg;
	t_btfld tmp_bit_chk;
	alt_u32 vg;
	alt_u32 rank_end = all_ranks ? RW_MGR_MEM_NUMBER_OF_RANKS : (rank_bgn + NUM_RANKS_PER_SHADOW_REG);

	*bit_chk = param->write_correct_mask;
	correct_mask_vg = param->write_correct_mask_vg;

	for (r = rank_bgn; r < rank_end; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank 
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		tmp_bit_chk = 0;
		for (vg = RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS-1; ; vg--) {

			//USER reset the fifos to get pointers to known state 
			IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);			

			tmp_bit_chk = tmp_bit_chk << (RW_MGR_MEM_DQ_PER_WRITE_DQS / RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS);
			rw_mgr_mem_calibrate_write_test_issue (write_group*RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS+vg, use_dm);

			tmp_bit_chk = tmp_bit_chk | (correct_mask_vg & ~(IORD_32DIRECT(BASE_RW_MGR, 0)));
			DPRINT(2, "write_test(%lu,%lu,%lu) :[%lu,%lu] " BTFLD_FMT " & ~%x => " BTFLD_FMT " => " BTFLD_FMT,
			       write_group, use_dm, all_correct, r, vg,
			       correct_mask_vg, IORD_32DIRECT(BASE_RW_MGR, 0), correct_mask_vg & ~IORD_32DIRECT(BASE_RW_MGR, 0),
			       tmp_bit_chk);

			if (vg == 0) {
				break;
			}
		}
		*bit_chk &= tmp_bit_chk;
	}

	if (all_correct)
	{
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "write_test(%lu,%lu,ALL) : " BTFLD_FMT " == " BTFLD_FMT " => %lu", write_group, use_dm,
		       *bit_chk, param->write_correct_mask, (long unsigned int)(*bit_chk == param->write_correct_mask));
		return (*bit_chk == param->write_correct_mask);
	}
	else
	{
		set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		DPRINT(2, "write_test(%lu,%lu,ONE) : " BTFLD_FMT " != " BTFLD_FMT " => %lu", write_group, use_dm,
		       *bit_chk, (long unsigned int)0, (long unsigned int)(*bit_chk != 0));
		return (*bit_chk != 0x00);
	}
}

static inline alt_u32 rw_mgr_mem_calibrate_write_test_all_ranks (alt_u32 write_group, alt_u32 use_dm, alt_u32 all_correct, t_btfld *bit_chk)
{
	return rw_mgr_mem_calibrate_write_test (0, write_group, use_dm, all_correct, bit_chk, 1);
}


//USER level the write operations

#if DYNAMIC_CALIBRATION_MODE || STATIC_QUICK_CALIBRATION

#if QDRII

//USER Write Levelling -- Quick Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	return 0;
}

#endif

#if RLDRAMX
#if !ENABLE_SUPER_QUICK_CALIBRATION

//USER Write Levelling -- Quick Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 d;
	t_btfld bit_chk;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);
	reg_file_set_group(g);

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	if (d > IO_IO_OUT1_DELAY_MAX) {
		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	return 1;
}

#else

//USER Write Levelling -- Super Quick Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 d;
	t_btfld bit_chk;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	//USER The first call to this function will calibrate all groups
	if (g != 0) {
		return 1;
	}

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);
	reg_file_set_group(g);

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	if (d > IO_IO_OUT1_DELAY_MAX) {
		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	reg_file_set_sub_stage(CAL_SUBSTAGE_WLEVEL_COPY);

	//USER Now copy the calibration settings to all other groups
	for (g = 1, test_bgn = RW_MGR_MEM_DQ_PER_WRITE_DQS; g < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; g++, test_bgn += RW_MGR_MEM_DQ_PER_WRITE_DQS) {
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		//USER Verify that things worked as expected
		if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WLEVEL_COPY);

			return 0;
		}
	}

	return 1;
}

#endif
#endif

#if DDRX
#if !ENABLE_SUPER_QUICK_CALIBRATION

//USER Write Levelling -- Quick Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 p;
	t_btfld bit_chk;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	//USER update info for sims 

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	//USER maximum phases for the sweep 

	//USER starting phases 

	//USER update info for sims

	reg_file_set_group(g);

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX; p++) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	if (p > IO_DQDQS_OUT_PHASE_MAX) {
		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	return 1;
}

#else

//USER Write Levelling -- Super Quick Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 p;
	t_btfld bit_chk;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	//USER The first call to this function will calibrate all groups
	if (g != 0) {
		return 1;
	}

	//USER update info for sims 

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	//USER maximum phases for the sweep 

	//USER starting phases 

	//USER update info for sims

	reg_file_set_group(g);

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX; p++) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	if (p > IO_DQDQS_OUT_PHASE_MAX) {
		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	reg_file_set_sub_stage(CAL_SUBSTAGE_WLEVEL_COPY);

	//USER Now copy the calibration settings to all other groups
	for (g = 1, test_bgn = RW_MGR_MEM_DQ_PER_READ_DQS; (g < RW_MGR_MEM_IF_READ_DQS_WIDTH); g++, test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS) {
		IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, g);
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		//USER Verify that things worked as expected
		if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WLEVEL_COPY);

			IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, 0);
			return 0;
		}
	}

	IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, 0);
	return 1;
}

#endif
#endif

#endif

#if DYNAMIC_CALIBRATION_MODE || STATIC_FULL_CALIBRATION

#if QDRII 
//USER Write Levelling -- Full Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	return 0;
}
#endif

#if RLDRAMX
//USER Write Levelling -- Full Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 d;
	t_btfld bit_chk;
	alt_u32 work_bgn, work_end;
	alt_u32 d_bgn, d_end;
	alt_u32 found_begin;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	BFM_STAGE("wlevel");
	
	ALTERA_ASSERT(g < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	//USER maximum delays for the sweep 

	//USER update info for sims

	reg_file_set_group(g);

	//USER starting and end range where writes work

	scc_mgr_spread_out2_delay_all_ranks (g,test_bgn);

	work_bgn = 0;
	work_end = 0;

	//USER step 1: find first working dtap, increment in dtaps
	found_begin = 0;
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++, work_bgn += IO_DELAY_PER_DCHAIN_TAP) {
		DPRINT(2, "wlevel: begin: d=%lu", d);
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);
		
		if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			found_begin = 1;
			d_bgn = d;
			break;
		} else {
			recover_mem_device_after_ck_dqs_violation();
		}
	}

	if (!found_begin) {
		//USER fail, cannot find first working delay

		DPRINT(2, "wlevel: failed to find first working delay", d);
		
		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}
	
	DPRINT(2, "wlevel: found begin d=%lu work_bgn=%lu", d_bgn, work_bgn);
	BFM_GBL_SET(dqs_wlevel_left_edge[g].d,d_bgn);
	BFM_GBL_SET(dqs_wlevel_left_edge[g].ps,work_bgn);
	
	reg_file_set_sub_stage(CAL_SUBSTAGE_LAST_WORKING_DELAY);

	//USER step 2 : find first non-working dtap, increment in dtaps
	work_end = work_bgn;
	d = d + 1;
	for (; d <= IO_IO_OUT1_DELAY_MAX; d++, work_end += IO_DELAY_PER_DCHAIN_TAP) {
		DPRINT(2, "wlevel: end: d=%lu", d);
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			recover_mem_device_after_ck_dqs_violation();
			break;
		}
	}
	d_end = d - 1;

	if (d_end >= d_bgn) {
		//USER we have a working range 
	} else {
		//USER nil range
		//Note: don't think this is possible

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_LAST_WORKING_DELAY);

		return 0;
	}

	DPRINT(2, "wlevel: found end: d=%lu work_end=%lu", d_end, work_end);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].d,d_end);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].ps,work_end);
	
	TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[curr_shadow_reg][g].dqdqs_start, work_bgn);
	TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[curr_shadow_reg][g].dqdqs_end, work_end);


	//USER center 

	d = (d_end + d_bgn) / 2;

	DPRINT(2, "wlevel: found middle: d=%lu work_mid=%lu", d, (work_end + work_bgn)/2);
	BFM_GBL_SET(dqs_wlevel_mid[g].d,d);
	BFM_GBL_SET(dqs_wlevel_mid[g].ps,(work_end + work_bgn)/2);

	scc_mgr_zero_group (g, test_bgn, 1);
	scc_mgr_apply_group_all_out_delay_add_all_ranks (g, test_bgn, d);

	return 1;
}
#endif


#if DDRX
#if NEWVERSION_WL

//USER Write Levelling -- Full Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 p, d, sr;
	
#if CALIBRATE_BIT_SLIPS
#if QUARTER_RATE_MODE	
	alt_32 num_additional_fr_cycles = 3;
#elif HALF_RATE_MODE	
	alt_32 num_additional_fr_cycles = 1;
#else
	alt_32 num_additional_fr_cycles = 0;
#endif
#if MULTIPLE_AFI_WLAT
	num_additional_fr_cycles++;
#endif
#else	
	alt_u32 num_additional_fr_cycles = 0;
#endif
	
	t_btfld bit_chk;
	alt_u32 work_bgn, work_end, work_mid;
	alt_u32 tmp_delay;
	alt_u32 found_begin;
	alt_u32 dtaps_per_ptap;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	BFM_STAGE("wlevel");
	
	
	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	//USER maximum phases for the sweep 

#if USE_DQS_TRACKING
#if HHP_HPS
	dtaps_per_ptap = IORD_32DIRECT(REG_FILE_DTAPS_PER_PTAP, 0);
#else
	dtaps_per_ptap = IORD_32DIRECT(TRK_DTAPS_PER_PTAP, 0);
#endif
#else
	dtaps_per_ptap = 0;
	tmp_delay = 0;
	while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
		dtaps_per_ptap++;
		tmp_delay += IO_DELAY_PER_DCHAIN_TAP;
	}
	dtaps_per_ptap--;
	tmp_delay = 0;
#endif

	//USER starting phases 

	//USER update info for sims

	reg_file_set_group(g);

	//USER starting and end range where writes work 

	scc_mgr_spread_out2_delay_all_ranks (g,test_bgn);
	
	work_bgn = 0;
	work_end = 0;

	//USER step 1: find first working phase, increment in ptaps, and then in dtaps if ptaps doesn't find a working phase 
	found_begin = 0;
	tmp_delay = 0;
	for (d = 0; d <= dtaps_per_ptap; d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) {
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);
		
		work_bgn = tmp_delay;
		
		for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles*IO_DLL_CHAIN_LENGTH; p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
			DPRINT(2, "wlevel: begin-1: p=%lu d=%lu", p, d);
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

			if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
				found_begin = 1;
				break;
			}
		}
		
		if (found_begin) {
			break;
		}
	}

	if (p > IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles*IO_DLL_CHAIN_LENGTH) {
		//USER fail, cannot find first working phase 

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	DPRINT(2, "wlevel: first valid p=%lu d=%lu", p, d);
	
	reg_file_set_sub_stage(CAL_SUBSTAGE_LAST_WORKING_DELAY);

	//USER If d is 0 then the working window covers a phase tap and we can follow the old procedure
	//USER 	otherwise, we've found the beginning, and we need to increment the dtaps until we find the end 
	if (d == 0) {
		COV(WLEVEL_PHASE_PTAP_OVERLAP);
		work_end = work_bgn + IO_DELAY_PER_OPA_TAP;

		//USER step 2: if we have room, back off by one and increment in dtaps 
		
		if (p > 0) {
#ifdef BFM_MODE
			int found = 0;
#endif
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);
			
			tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;

			for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_bgn; d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) {
				DPRINT(2, "wlevel: begin-2: p=%lu d=%lu", (p-1), d);
				scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

				if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
#ifdef BFM_MODE
					found = 1;
#endif
					work_bgn = tmp_delay;
					break;
				}
			}

#ifdef BFM_MODE
			{
				alt_u32 d2;
				alt_u32 p2;
				if (found) {
					d2 = d;
					p2 = p - 1;
				} else {
					d2 = 0;
					p2 = p;
				}

				DPRINT(2, "wlevel: found begin-A: p=%lu d=%lu ps=%lu", p2, d2, work_bgn);

				BFM_GBL_SET(dqs_wlevel_left_edge[g].p,p2);
				BFM_GBL_SET(dqs_wlevel_left_edge[g].d,d2);
				BFM_GBL_SET(dqs_wlevel_left_edge[g].ps,work_bgn);
			}
#endif

			scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, 0);
		} else {
			DPRINT(2, "wlevel: found begin-B: p=%lu d=%lu ps=%lu", p, d, work_bgn);

			BFM_GBL_SET(dqs_wlevel_left_edge[g].p,p);
			BFM_GBL_SET(dqs_wlevel_left_edge[g].d,d);
			BFM_GBL_SET(dqs_wlevel_left_edge[g].ps,work_bgn);
		}

		//USER step 3: go forward from working phase to non working phase, increment in ptaps 

		for (p = p + 1; p <= IO_DQDQS_OUT_PHASE_MAX + num_additional_fr_cycles*IO_DLL_CHAIN_LENGTH; p++, work_end += IO_DELAY_PER_OPA_TAP) {
			DPRINT(2, "wlevel: end-0: p=%lu d=%lu", p, (long unsigned int)0);
			scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

			if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
				break;
			}
		}

		//USER step 4: back off one from last, increment in dtaps 
		//USER The actual increment is done outside the if/else statement since it is shared with other code

		p = p - 1;

		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		work_end -= IO_DELAY_PER_OPA_TAP;
		d = 0;

	} else {
		//USER step 5: Window doesn't cover phase tap, just increment dtaps until failure
		//USER The actual increment is done outside the if/else statement since it is shared with other code
		COV(WLEVEL_PHASE_PTAP_NO_OVERLAP);
		work_end = work_bgn;
		DPRINT(2, "wlevel: found begin-C: p=%lu d=%lu ps=%lu", p, d, work_bgn);
		BFM_GBL_SET(dqs_wlevel_left_edge[g].p,p);
		BFM_GBL_SET(dqs_wlevel_left_edge[g].d,d);
		BFM_GBL_SET(dqs_wlevel_left_edge[g].ps,work_bgn);

	}
	
	//USER The actual increment until failure
	for (; d <= IO_IO_OUT1_DELAY_MAX; d++, work_end += IO_DELAY_PER_DCHAIN_TAP) {
		DPRINT(2, "wlevel: end: p=%lu d=%lu", p, d);
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}
	scc_mgr_zero_group (g, test_bgn, 1);

	work_end -= IO_DELAY_PER_DCHAIN_TAP;

	if (work_end >= work_bgn) {
		//USER we have a working range 
	} else {
		//USER nil range 

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_LAST_WORKING_DELAY);

		return 0;
	}

	DPRINT(2, "wlevel: found end: p=%lu d=%lu; range: [%lu,%lu]", p, d-1, work_bgn, work_end);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].p,p);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].d,d-1);
	BFM_GBL_SET(dqs_wlevel_right_edge[g].ps,work_end);
	
	for(sr = 0; sr < NUM_SHADOW_REGS; sr++) {
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[sr][g].dqdqs_start, work_bgn);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[sr][g].dqdqs_end, work_end);
	}

	//USER center 

	work_mid = (work_bgn + work_end) / 2;

	DPRINT(2, "wlevel: work_mid=%ld", work_mid);

	tmp_delay = 0;

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX  + num_additional_fr_cycles*IO_DLL_CHAIN_LENGTH && tmp_delay < work_mid; p++, tmp_delay += IO_DELAY_PER_OPA_TAP);

	if (tmp_delay > work_mid) {
		tmp_delay -= IO_DELAY_PER_OPA_TAP;
		p--;
	}
	
	while (p > IO_DQDQS_OUT_PHASE_MAX) {
		tmp_delay -= IO_DELAY_PER_OPA_TAP;
		p--;
	}

	scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);
	
	DPRINT(2, "wlevel: p=%lu tmp_delay=%lu left=%lu", p, tmp_delay, work_mid - tmp_delay);
	
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_mid; d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP);

	if (tmp_delay > work_mid) {
		tmp_delay -= IO_DELAY_PER_DCHAIN_TAP;
		d--;
	}

	DPRINT(2, "wlevel: p=%lu d=%lu tmp_delay=%lu left=%lu", p, d, tmp_delay, work_mid - tmp_delay);

	scc_mgr_apply_group_all_out_delay_add_all_ranks (g, test_bgn, d);

	DPRINT(2, "wlevel: found middle: p=%lu d=%lu", p, d);
	BFM_GBL_SET(dqs_wlevel_mid[g].p,p);
	BFM_GBL_SET(dqs_wlevel_mid[g].d,d);
	BFM_GBL_SET(dqs_wlevel_mid[g].ps,work_mid);

	return 1;
}


#else

//USER Write Levelling -- Full Calibration
alt_u32 rw_mgr_mem_calibrate_wlevel (alt_u32 g, alt_u32 test_bgn)
{
	alt_u32 p, d;
	t_btfld bit_chk;
	alt_u32 work_bgn, work_end, work_mid;
	alt_u32 tmp_delay;

	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	//USER update info for sims

	reg_file_set_stage(CAL_STAGE_WLEVEL);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WORKING_DELAY);

	//USER maximum phases for the sweep 

	//USER starting phases 

	//USER update info for sims

	reg_file_set_group(g);

	//USER starting and end range where writes work 

	work_bgn = 0;
	work_end = 0;

	//USER step 1: find first working phase, increment in ptaps 

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX; p++, work_bgn += IO_DELAY_PER_OPA_TAP) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	if (p > IO_DQDQS_OUT_PHASE_MAX) {
		//USER fail, cannot find first working phase 

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_WORKING_DELAY);

		return 0;
	}

	work_end = work_bgn + IO_DELAY_PER_OPA_TAP;

	reg_file_set_sub_stage(CAL_SUBSTAGE_LAST_WORKING_DELAY);

	//USER step 2: if we have room, back off by one and increment in dtaps 

	if (p > 0) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

		tmp_delay = work_bgn - IO_DELAY_PER_OPA_TAP;

		for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_bgn; d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP) {
			scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

			if (rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
				work_bgn = tmp_delay;
				break;
			}
		}

		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, 0);
	}

	//USER step 3: go forward from working phase to non working phase, increment in ptaps 

	for (p = p + 1; p <= IO_DQDQS_OUT_PHASE_MAX; p++, work_end += IO_DELAY_PER_OPA_TAP) {
		scc_mgr_set_dqdqs_output_phase_all_ranks(g, p);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	//USER step 4: back off one from last, increment in dtaps 

	scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

	work_end -= IO_DELAY_PER_OPA_TAP;

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++, work_end += IO_DELAY_PER_DCHAIN_TAP) {
		scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, d);

		if (!rw_mgr_mem_calibrate_write_test_all_ranks (g, 0, PASS_ONE_BIT, &bit_chk)) {
			break;
		}
	}

	scc_mgr_apply_group_all_out_delay_all_ranks (g, test_bgn, 0);

	if (work_end > work_bgn) {
		//USER we have a working range 
	} else {
		//USER nil range 

		set_failing_group_stage(g, CAL_STAGE_WLEVEL, CAL_SUBSTAGE_LAST_WORKING_DELAY);

		return 0;
	}

	//USER center 

	work_mid = (work_bgn + work_end) / 2;

	tmp_delay = 0;

	for (p = 0; p <= IO_DQDQS_OUT_PHASE_MAX && tmp_delay < work_mid; p++, tmp_delay += IO_DELAY_PER_OPA_TAP);

	tmp_delay -= IO_DELAY_PER_OPA_TAP;

	scc_mgr_set_dqdqs_output_phase_all_ranks(g, p - 1);

	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX && tmp_delay < work_mid; d++, tmp_delay += IO_DELAY_PER_DCHAIN_TAP);

	scc_mgr_apply_group_all_out_delay_add_all_ranks (g, test_bgn, d - 1);


	return 1;
}

#endif
#endif
#endif

//USER center all windows. do per-bit-deskew to possibly increase size of certain windows

#if NEWVERSION_WRDESKEW

alt_u32 rw_mgr_mem_calibrate_writes_center (alt_u32 rank_bgn, alt_u32 write_group, alt_u32 test_bgn)
{
	alt_u32 i, p, min_index;
	alt_32 d;
	//USER Store these as signed since there are comparisons with signed numbers
	t_btfld bit_chk;
#if QDRII
	t_btfld tmp_bit_chk;
	t_btfld tmp_mask;
	t_btfld mask;
#endif
	t_btfld sticky_bit_chk;
	alt_32 left_edge[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	alt_32 right_edge[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	alt_32 mid;
	alt_32 mid_min, orig_mid_min;
	alt_32 new_dqs, start_dqs, shift_dq;
#if RUNTIME_CAL_REPORT	
	alt_32 new_dq[RW_MGR_MEM_DQ_PER_WRITE_DQS];
#endif
	alt_32 dq_margin, dqs_margin, dm_margin;
	alt_u32 stop;

	TRACE_FUNC("%lu %lu", write_group, test_bgn);
	BFM_STAGE("writes_center");

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	dm_margin = 0;
	
	start_dqs = READ_SCC_DQS_IO_OUT1_DELAY();

	select_curr_shadow_reg_using_rank(rank_bgn);

	//USER per-bit deskew 
		
	//USER set the left and right edge of each bit to an illegal value 
	//USER use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value
	sticky_bit_chk = 0;
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		left_edge[i]  = IO_IO_OUT1_DELAY_MAX + 1;
		right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
	}
	
	//USER Search for the left edge of the window for each bit
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_out1_delay (write_group, test_bgn, d);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit 
		stop = !rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ONE_BIT, &bit_chk, 0);
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->write_correct_mask);
		DPRINT(2, "write_center(left): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT " && %lu [bit_chk=" BTFLD_FMT "]",
		       d, sticky_bit_chk, param->write_correct_mask, stop, bit_chk);
		
		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the left_edge
					left_edge[i] = d;
				} else {
					//USER If a left edge has not been seen yet, then a future passing test will mark this edge as the right edge 
					if (left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -(d + 1);
					}
				}
				DPRINT(2, "write_center[l,d=%lu): bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

	//USER Reset DQ delay chains to 0 
	scc_mgr_apply_group_dq_out1_delay (write_group, test_bgn, 0);
	sticky_bit_chk = 0;
	for (i = RW_MGR_MEM_DQ_PER_WRITE_DQS - 1;; i--) {

		DPRINT(2, "write_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i], i, right_edge[i]);
		
		//USER Check for cases where we haven't found the left edge, which makes our assignment of the the 
		//USER right edge invalid.  Reset it to the illegal value. 
		if ((left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) && (right_edge[i] != IO_IO_OUT1_DELAY_MAX + 1)) {
			right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
			DPRINT(2, "write_center: reset right_edge[%lu]: %ld", i, right_edge[i]);
		}
		
		//USER Reset sticky bit (except for bits where we have seen the left edge) 
		sticky_bit_chk = sticky_bit_chk << 1;
		if ((left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1)) {
			sticky_bit_chk = sticky_bit_chk | 1;
		}

		if (i == 0)
		{
			break;
		}
	}
	
	//USER Search for the right edge of the window for each bit 
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - start_dqs; d++) {
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, d + start_dqs);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		if (QDRII)
		{
			rw_mgr_mem_dll_lock_wait();
		}

		//USER Stop searching when the read test doesn't pass AND when we've seen a passing read on every bit 
		stop = !rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ONE_BIT, &bit_chk, 0);
		if (stop) {
			recover_mem_device_after_ck_dqs_violation();
		}
		sticky_bit_chk = sticky_bit_chk | bit_chk;
		stop = stop && (sticky_bit_chk == param->write_correct_mask);
		
		DPRINT(2, "write_center (right): dtap=%lu => " BTFLD_FMT " == " BTFLD_FMT " && %lu", d, sticky_bit_chk, param->write_correct_mask, stop);

		if (stop == 1) {
			if (d == 0) {
				for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
					//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
					if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1 && left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -1;
					}
				}
			}
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					//USER Remember a passing test as the right_edge 
					right_edge[i] = d;
				} else {
					if (d != 0) {
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge 
						if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
					} else {
						//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
						if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1 && left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1) {
							right_edge[i] = -1;
						}
						//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge 
						else if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
							left_edge[i] = -(d + 1);
						}
					}
				}
				DPRINT(2, "write_center[r,d=%lu): bit_chk_test=%d left_edge[%lu]: %ld right_edge[%lu]: %ld",
				       d, (int)(bit_chk & 1), i, left_edge[i], i, right_edge[i]);
				bit_chk = bit_chk >> 1;
			}
		}
	}

#if ENABLE_TCL_DEBUG
	// Store all observed margins
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		alt_u32 dq = write_group*RW_MGR_MEM_DQ_PER_WRITE_DQS + i;

		ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

		TCLRPT_SET(debug_cal_report->cal_dq_out_margins[curr_shadow_reg][dq].left_edge, left_edge[i]);
		TCLRPT_SET(debug_cal_report->cal_dq_out_margins[curr_shadow_reg][dq].right_edge, right_edge[i]);
	}
#endif

	//USER Check that all bits have a window
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		DPRINT(2, "write_center: left_edge[%lu]: %ld right_edge[%lu]: %ld", i, left_edge[i], i, right_edge[i]);
		BFM_GBL_SET(dq_write_left_edge[write_group][i],left_edge[i]);
		BFM_GBL_SET(dq_write_right_edge[write_group][i],right_edge[i]);
		if ((left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) || (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1)) {
			set_failing_group_stage(test_bgn + i, CAL_STAGE_WRITES, CAL_SUBSTAGE_WRITES_CENTER);
			return 0;
		}
	}		
	
	//USER Find middle of window for each DQ bit 
	mid_min = left_edge[0] - right_edge[0];
	min_index = 0;
	for (i = 1; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		mid = left_edge[i] - right_edge[i];
		if (mid < mid_min) {
			mid_min = mid;
			min_index = i;
		}
	}

	//USER  -mid_min/2 represents the amount that we need to move DQS.  If mid_min is odd and positive we'll need to add one to
	//USER make sure the rounding in further calculations is correct (always bias to the right), so just add 1 for all positive values
	if (mid_min > 0) {
		mid_min++;
	}
	mid_min = mid_min / 2;

	DPRINT(1, "write_center: mid_min=%ld", mid_min);
	
	//USER Determine the amount we can change DQS (which is -mid_min)
	orig_mid_min = mid_min;		
#if ENABLE_DQS_OUT_CENTERING
	if (DDRX || RLDRAMX) {
		new_dqs = start_dqs - mid_min;
		DPRINT(2, "write_center: new_dqs(1)=%ld", new_dqs);
		if (new_dqs > IO_IO_OUT1_DELAY_MAX) {
			new_dqs = IO_IO_OUT1_DELAY_MAX;
		} else if (new_dqs < 0) {
			new_dqs = 0;
		} 
		mid_min = start_dqs - new_dqs;

		new_dqs = start_dqs - mid_min;
	} else {
		new_dqs = start_dqs;
		mid_min = 0;
	}
#else
	new_dqs = start_dqs;
	mid_min = 0;
#endif
	
	DPRINT(1, "write_center: start_dqs=%ld new_dqs=%ld mid_min=%ld", start_dqs, new_dqs, mid_min);

	//USER Initialize data for export structures 
	dqs_margin = IO_IO_OUT1_DELAY_MAX + 1;
	dq_margin  = IO_IO_OUT1_DELAY_MAX + 1;
	
	//USER add delay to bring centre of all DQ windows to the same "level" 
	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		//USER Use values before divide by 2 to reduce round off error 
		shift_dq = (left_edge[i] - right_edge[i] - (left_edge[min_index] - right_edge[min_index]))/2  + (orig_mid_min - mid_min);
		
		DPRINT(2, "write_center: before: shift_dq[%lu]=%ld", i, shift_dq);

		if (shift_dq + (alt_32)READ_SCC_DQ_OUT1_DELAY(i) > (alt_32)IO_IO_OUT1_DELAY_MAX) {
			shift_dq = (alt_32)IO_IO_OUT1_DELAY_MAX - READ_SCC_DQ_OUT1_DELAY(i);
		} else if (shift_dq + (alt_32)READ_SCC_DQ_OUT1_DELAY(i) < 0) {
			shift_dq = -(alt_32)READ_SCC_DQ_OUT1_DELAY(i);
		} 
#if RUNTIME_CAL_REPORT
		new_dq[i] = shift_dq;
#endif
		DPRINT(2, "write_center: after: shift_dq[%lu]=%ld", i, shift_dq);
		scc_mgr_set_dq_out1_delay(write_group, i, READ_SCC_DQ_OUT1_DELAY(i) + shift_dq);
		scc_mgr_load_dq (i);
		
		DPRINT(2, "write_center: margin[%lu]=[%ld,%ld]", i,
		       left_edge[i] - shift_dq + (-mid_min),
		       right_edge[i] + shift_dq - (-mid_min));
		//USER To determine values for export structures 
		if (left_edge[i] - shift_dq + (-mid_min) < dq_margin) {
			dq_margin = left_edge[i] - shift_dq + (-mid_min);
		}
		if (right_edge[i] + shift_dq - (-mid_min) < dqs_margin) {
			dqs_margin = right_edge[i] + shift_dq - (-mid_min);
		}
	}

	//USER Move DQS 
	if (QDRII) {
		scc_mgr_set_group_dqs_io_and_oct_out1_gradual (write_group, new_dqs);
	} else {
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, new_dqs);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}
	
#if RUNTIME_CAL_REPORT
	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		RPRINT("Write Deskew ; DQ %2lu ; Rank %lu ; Left edge %3li ; Right edge %3li ; DQ delay %2li ; DQS delay %2li", write_group*RW_MGR_MEM_DQ_PER_WRITE_DQS + i, rank_bgn, left_edge[i], right_edge[i], new_dq[i], new_dqs);
	}
#endif


	//////////////////////
	//////////////////////
	//USER Centre DM 
	//////////////////////
	//////////////////////

	BFM_STAGE("dm_center");

	DPRINT(2, "write_center: DM");

#if RLDRAMX

	//Note: this is essentially the same as DDR with the exception of the dm_ global accounting
	
	//USER Determine if first group in device to initialize left and right edges
	if (!is_write_group_enabled_for_dm(write_group))
	{
		DPRINT(2, "dm_calib: skipping since not last in group");
	}
	else
	{

		// last in the group, so we need to do DM
		DPRINT(2, "dm_calib: calibrating DM since last in group");

		//USER set the left and right edge of each bit to an illegal value 
		//USER use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value
		left_edge[0]  = IO_IO_OUT1_DELAY_MAX + 1;
		right_edge[0] = IO_IO_OUT1_DELAY_MAX + 1;
	
		sticky_bit_chk = 0;
		//USER Search for the left edge of the window for the DM bit
		for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++) {
			scc_mgr_apply_group_dm_out1_delay (write_group, d);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			//USER Stop searching when the write test doesn't pass AND when we've seen a passing write before
			if (rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {
				DPRINT(2, "dm_calib: left=%lu passed", d);
				left_edge[0] = d;
			} else {
				DPRINT(2, "dm_calib: left=%lu failed", d);
				//USER If a left edge has not been seen yet, then a future passing test will mark this edge as the right edge 
				if (left_edge[0] == IO_IO_OUT1_DELAY_MAX + 1) {
					right_edge[0] = -(d + 1);
				} else {
					//USER left edge has been seen, so this failure marks the left edge, and we are done
					break;
				}
			}
			DPRINT(2, "dm_calib[l,d=%lu]: left_edge: %ld right_edge: %ld",
			       d, left_edge[0], right_edge[0]);
		}

		DPRINT(2, "dm_calib left done: left_edge: %ld right_edge: %ld",
		       left_edge[0], right_edge[0]);
	
		//USER Reset DM delay chains to 0
		scc_mgr_apply_group_dm_out1_delay (write_group, 0);

		//USER Check for cases where we haven't found the left edge, which makes our assignment of the the 
		//USER right edge invalid.  Reset it to the illegal value. 
		if ((left_edge[0] == IO_IO_OUT1_DELAY_MAX + 1) && (right_edge[0] != IO_IO_OUT1_DELAY_MAX + 1)) {
			right_edge[0] = IO_IO_OUT1_DELAY_MAX + 1;
			DPRINT(2, "dm_calib: reset right_edge: %ld", right_edge[0]);
		}
		
		//USER Search for the right edge of the window for the DM bit
		for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - new_dqs; d++) {
			// Note: This only shifts DQS, so are we limiting ourselve to
			// width of DQ unnecessarily
			scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, d + new_dqs);

			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			//USER Stop searching when the test fails and we've seen passing test already
			if (rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {
				DPRINT(2, "dm_calib: right=%lu passed", d);
				right_edge[0] = d;
			} else {
				recover_mem_device_after_ck_dqs_violation();
			
				DPRINT(2, "dm_calib: right=%lu failed", d);
				if (d != 0) {
					//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge 
					if (right_edge[0] == IO_IO_OUT1_DELAY_MAX + 1) {
						left_edge[0] = -(d + 1);
					} else {
						break;
					}
				} else {
					//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
					if (right_edge[0] == IO_IO_OUT1_DELAY_MAX + 1 && left_edge[0] != IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[0] = -1;
						// we're done
						break;
					}
					//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge 
					else if (right_edge[0] == IO_IO_OUT1_DELAY_MAX + 1) {
						left_edge[0] = -(d + 1);
					}
				}
			}
			DPRINT(2, "dm_calib[l,d=%lu]: left_edge: %ld right_edge: %ld",
			       d, left_edge[0], right_edge[0]);
		}

		DPRINT(2, "dm_calib: left=%ld right=%ld", left_edge[0], right_edge[0]);
#if BFM_MODE
		// need to update for all groups covered by this dm
		for (i = write_group+1-(RW_MGR_MEM_IF_WRITE_DQS_WIDTH/RW_MGR_MEM_DATA_MASK_WIDTH); i <= write_group; i++)
		{
			DPRINT(3, "dm_calib: left[%d]=%ld right[%d]=%ld", i, left_edge[0], i, right_edge[0]);
			BFM_GBL_SET(dm_left_edge[i][0],left_edge[0]);
			BFM_GBL_SET(dm_right_edge[i][0],right_edge[0]);
		}
#endif
	
		//USER Move DQS (back to orig)
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, new_dqs);

		//USER move DM
		//USER Find middle of window for the DM bit
		mid = (left_edge[0] - right_edge[0]) / 2;
		if (mid < 0) {
			mid = 0;
		}
		scc_mgr_apply_group_dm_out1_delay (write_group, mid);

		dm_margin = left_edge[0];
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		DPRINT(2, "dm_calib: left=%ld right=%ld mid=%ld dm_margin=%ld",
		       left_edge[0], right_edge[0], mid, dm_margin);
	} // end of DM calibration
#endif


#if DDRX
	//USER set the left and right edge of each bit to an illegal value 
	//USER use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value
	left_edge[0]  = IO_IO_OUT1_DELAY_MAX + 1;
	right_edge[0] = IO_IO_OUT1_DELAY_MAX + 1;
	alt_32 bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
	alt_32 end_curr = IO_IO_OUT1_DELAY_MAX + 1;
	alt_32 bgn_best = IO_IO_OUT1_DELAY_MAX + 1;
	alt_32 end_best = IO_IO_OUT1_DELAY_MAX + 1;
	alt_32 win_best = 0;
	
	//USER Search for the/part of the window with DM shift
	for (d = IO_IO_OUT1_DELAY_MAX; d >= 0; d-=DELTA_D) {
		scc_mgr_apply_group_dm_out1_delay (write_group, d);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		if (rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {
			
			//USE Set current end of the window
			end_curr = -d;
			//USER If a starting edge of our window has not been seen this is our current start of the DM window
			if(bgn_curr == IO_IO_OUT1_DELAY_MAX + 1){
				bgn_curr = -d;
			}

			//USER If current window is bigger than best seen. Set best seen to be current window 
			if((end_curr-bgn_curr+1) > win_best ){
				win_best = end_curr-bgn_curr+1;
				bgn_best = bgn_curr;
				end_best = end_curr;
			}
		} else {
			//USER We just saw a failing test. Reset temp edge
			bgn_curr=IO_IO_OUT1_DELAY_MAX + 1;
			end_curr=IO_IO_OUT1_DELAY_MAX + 1;
			}
		
	
		}

	
	//USER Reset DM delay chains to 0
	scc_mgr_apply_group_dm_out1_delay (write_group, 0);

	//USER Check to see if the current window nudges up aganist 0 delay. If so we need to continue the search by shifting DQS otherwise DQS search begins as a new search
	if(end_curr!=0) {
		bgn_curr=IO_IO_OUT1_DELAY_MAX + 1;
		end_curr=IO_IO_OUT1_DELAY_MAX + 1;
	}
		
	//USER Search for the/part of the window with DQS shifts
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - new_dqs; d+=DELTA_D) {
		// Note: This only shifts DQS, so are we limiting ourselve to
		// width of DQ unnecessarily
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, d + new_dqs);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		if (rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {
			
			//USE Set current end of the window
			end_curr = d;
			//USER If a beginning edge of our window has not been seen this is our current begin of the DM window
			if(bgn_curr == IO_IO_OUT1_DELAY_MAX + 1){
				bgn_curr = d;
			}
				
			//USER If current window is bigger than best seen. Set best seen to be current window
			if((end_curr-bgn_curr+1) > win_best){
				win_best = end_curr-bgn_curr+1;
				bgn_best = bgn_curr;
				end_best = end_curr;
			}
		} else {
			//USER We just saw a failing test. Reset temp edge
			recover_mem_device_after_ck_dqs_violation();
			bgn_curr = IO_IO_OUT1_DELAY_MAX + 1;
			end_curr = IO_IO_OUT1_DELAY_MAX + 1;
			
			//USER Early exit optimization: if ther remaining delay chain space is less than already seen largest window we can exit
			if((win_best-1) > (IO_IO_OUT1_DELAY_MAX - new_dqs - d)){				
					break;
				}
		
				}
				}

	//USER assign left and right edge for cal and reporting;
	left_edge[0] = -1*bgn_best;
	right_edge[0] = end_best;

	DPRINT(2, "dm_calib: left=%ld right=%ld", left_edge[0], right_edge[0]);
	BFM_GBL_SET(dm_left_edge[write_group][0],left_edge[0]);
	BFM_GBL_SET(dm_right_edge[write_group][0],right_edge[0]);
	
	//USER Move DQS (back to orig)
	scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, new_dqs);

	//USER Move DM

	//USER Find middle of window for the DM bit
	mid = (left_edge[0] - right_edge[0]) / 2;
	
	//USER only move right, since we are not moving DQS/DQ
	if (mid < 0) {
		mid = 0;
	}
	
	//dm_marign should fail if we never find a window
	if(win_best==0){ 
		dm_margin = -1;
	}else{
		dm_margin = left_edge[0] - mid;
	}
	
	
	
	scc_mgr_apply_group_dm_out1_delay(write_group, mid);
	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	
	DPRINT(2, "dm_calib: left=%ld right=%ld mid=%ld dm_margin=%ld",
	       left_edge[0], right_edge[0], mid, dm_margin);
#endif

#if QDRII
	sticky_bit_chk = 0;

	//USER set the left and right edge of each bit to an illegal value
	//USER use (IO_IO_OUT1_DELAY_MAX + 1) as an illegal value
	for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
		left_edge[i] = right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
	}

	mask = param->dm_correct_mask;
	
	//USER Search for the left edge of the window for the DM bit
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dm_out1_delay (write_group, d);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		//USER Stop searching when the read test doesn't pass for all bits (as they've already been calibrated)
		stop = !rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ONE_BIT, &bit_chk, 0);
		DPRINT(2, "dm_calib[l,d=%lu] stop=%ld bit_chk=%llx sticky_bit_chk=%llx mask=%llx",
		       d, stop, bit_chk, sticky_bit_chk, param->write_correct_mask);
		tmp_bit_chk = bit_chk;
		tmp_mask = mask;
		for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
			if ( (tmp_bit_chk & mask) == mask ) {
				sticky_bit_chk = sticky_bit_chk | tmp_mask;
			}
			tmp_bit_chk = tmp_bit_chk >> (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH);
			tmp_mask = tmp_mask << (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH);
		}
		stop = stop && (sticky_bit_chk == param->write_correct_mask);

		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
				DPRINT(2, "dm_calib[l,i=%lu] d=%lu bit_chk&dm_mask=" BTFLD_FMT " == " BTFLD_FMT, i, d,
				       bit_chk & mask, mask);
				if ((bit_chk & mask) == mask) {
					DPRINT(2, "dm_calib: left[%lu]=%lu", i, d);
					left_edge[i] = d;
				} else {
					//USER If a left edge has not been seen yet, then a future passing test will mark this edge as the right edge
					if (left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -(d + 1);
					}
				}
				bit_chk = bit_chk >> (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH);
			}
		}
	}

	//USER Reset DM delay chains to 0
	scc_mgr_apply_group_dm_out1_delay (write_group, 0);

	//USER Check for cases where we haven't found the left edge, which makes our assignment of the the
	//USER right edge invalid.  Reset it to the illegal value.
	for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
		if ((left_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) && (right_edge[i] != IO_IO_OUT1_DELAY_MAX + 1)) {
			right_edge[i] = IO_IO_OUT1_DELAY_MAX + 1;
			DPRINT(2, "dm_calib: reset right_edge: %d", right_edge[i]);
		}
	}

	//USER Search for the right edge of the window for the DM bit
	for (d = 0; d <= IO_IO_OUT1_DELAY_MAX - new_dqs; d++) {
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, d + new_dqs);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		rw_mgr_mem_dll_lock_wait();

		//USER Stop searching when the read test doesn't pass for all bits (as they've already been calibrated)
		stop = !rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ONE_BIT, &bit_chk, 0);
		DPRINT(2, "dm_calib[l,d=%lu] stop=%ld bit_chk=%llx sticky_bit_chk=%llx mask=%llx",
		       d, stop, bit_chk, sticky_bit_chk, param->write_correct_mask);
		tmp_bit_chk = bit_chk;
		tmp_mask = mask;
		for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
			if ( (tmp_bit_chk & mask) == mask ) {
				sticky_bit_chk = sticky_bit_chk | tmp_mask;
			}
			tmp_bit_chk = tmp_bit_chk >> (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH);
			tmp_mask = tmp_mask << (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH);
		}
		stop = stop && (sticky_bit_chk == param->write_correct_mask);

		if (stop == 1) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
				DPRINT(2, "dm_calib[r,i=%lu] d=%lu bit_chk&dm_mask=" BTFLD_FMT " == " BTFLD_FMT, i, d,
				       bit_chk & mask, mask);
				if ((bit_chk & mask) == mask) {
					right_edge[i] = d;
				} else {
					//USER d = 0 failed, but it passed when testing the left edge, so it must be marginal, set it to -1
					if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1 && left_edge[i] != IO_IO_OUT1_DELAY_MAX + 1) {
						right_edge[i] = -1;
						// we're done
						break;
					}
					//USER If a right edge has not been seen yet, then a future passing test will mark this edge as the left edge
					else if (right_edge[i] == IO_IO_OUT1_DELAY_MAX + 1) {
						left_edge[i] = -(d + 1);
					}
				}
				bit_chk = bit_chk >> (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH);
			}
		}
	}

	//USER Move DQS (back to orig)
	scc_mgr_set_group_dqs_io_and_oct_out1_gradual (write_group, new_dqs);	

	//USER Move DM
	dm_margin = IO_IO_OUT1_DELAY_MAX;
	for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
		//USER Find middle of window for the DM bit
		mid = (left_edge[i] - right_edge[i]) / 2;
		DPRINT(2, "dm_calib[mid,i=%lu] left=%ld right=%ld mid=%ld", i, left_edge[i], right_edge[i], mid);
		BFM_GBL_SET(dm_left_edge[write_group][i],left_edge[i]);
		BFM_GBL_SET(dm_right_edge[write_group][i],right_edge[i]);
		if (mid < 0) {
			mid = 0;
		}
		scc_mgr_set_dm_out1_delay(write_group, i, mid);
		scc_mgr_load_dm (i);
		if ((left_edge[i] - mid) < dm_margin) {
			dm_margin = left_edge[i] - mid;
		}
	}
#endif

	// Store observed DM margins
#if RLDRAMX
	if (is_write_group_enabled_for_dm(write_group))
	{
		TCLRPT_SET(debug_cal_report->cal_dm_margins[curr_shadow_reg][write_group][0].left_edge, left_edge[0]);
		TCLRPT_SET(debug_cal_report->cal_dm_margins[curr_shadow_reg][write_group][0].right_edge, right_edge[0]);
	}
#else
	for (i = 0; i < RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP; i++) {
		TCLRPT_SET(debug_cal_report->cal_dm_margins[curr_shadow_reg][write_group][i].left_edge, left_edge[i]);
		TCLRPT_SET(debug_cal_report->cal_dm_margins[curr_shadow_reg][write_group][i].right_edge, right_edge[i]);
	}
#endif

#if RUNTIME_CAL_REPORT
	for (i = 0; i < RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP; i++) {
		RPRINT("DM Deskew ; Group %lu ; Left edge %3li; Right edge %3li; DM delay %2li", write_group, left_edge[i], right_edge[i], mid);
	}
#endif

	//USER Export values 
	gbl->fom_out += dq_margin + dqs_margin;

	TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[curr_shadow_reg][write_group].dqs_margin, dqs_margin);
	TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[curr_shadow_reg][write_group].dq_margin, dq_margin);

#if RLDRAMX
	if (is_write_group_enabled_for_dm(write_group))
	{
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[curr_shadow_reg][write_group].dm_margin, dm_margin);
	}
#else
	TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[curr_shadow_reg][write_group].dm_margin, dm_margin);
#endif
	TCLRPT_SET(debug_summary_report->fom_out, debug_summary_report->fom_out + (dq_margin + dqs_margin));
	TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][write_group].fom_out, (dq_margin + dqs_margin));

	DPRINT(2, "write_center: dq_margin=%ld dqs_margin=%ld dm_margin=%ld", dq_margin, dqs_margin, dm_margin);

	//USER Do not remove this line as it makes sure all of our decisions have been applied
	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);	
	return (dq_margin >= 0) && (dqs_margin >= 0) && (dm_margin >= 0);
}

#else // !NEWVERSION_WRDESKEW

alt_u32 rw_mgr_mem_calibrate_writes_center (alt_u32 rank_bgn, alt_u32 write_group, alt_u32 test_bgn)
{
	alt_u32 i, p, d;
	alt_u32 mid;
	t_btfld bit_chk, sticky_bit_chk;
	alt_u32 max_working_dq[RW_MGR_MEM_DQ_PER_WRITE_DQS];
	alt_u32 max_working_dm[RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	alt_u32 dq_margin, dqs_margin, dm_margin;
	alt_u32 start_dqs;
	alt_u32 stop;

	TRACE_FUNC("%lu %lu", write_group, test_bgn);
	
	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	//USER per-bit deskew 

	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		max_working_dq[i] = 0;
	}

	for (d = 1; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dq_out1_delay (write_group, test_bgn, d);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		if (!rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ONE_BIT, &bit_chk, 0)) {
			break;
		} else {
			for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
				if (bit_chk & 1) {
					max_working_dq[i] = d;
				}
				bit_chk = bit_chk >> 1;
			}
		}
	}

	scc_mgr_apply_group_dq_out1_delay (write_group, test_bgn, 0);

	//USER determine minimum of maximums 

	dq_margin = IO_IO_OUT1_DELAY_MAX;

	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		if (max_working_dq[i] < dq_margin) {
			dq_margin = max_working_dq[i];
		}
	}

	//USER add delay to center DQ windows 

	for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
		if (max_working_dq[i] > dq_margin) {
			scc_mgr_set_dq_out1_delay(write_group, i, max_working_dq[i] - dq_margin);
		} else {
			scc_mgr_set_dq_out1_delay(write_group, i, 0);
		}

		scc_mgr_load_dq (p, i);
	}

	//USER sweep DQS window, may potentially have more window due to per-bit-deskew

	start_dqs = READ_SCC_DQS_IO_OUT1_DELAY();

	for (d = start_dqs + 1; d <= IO_IO_OUT1_DELAY_MAX; d++) {
		scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, d);

		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		if (QDRII)
		{
			rw_mgr_mem_dll_lock_wait();
		}		

		if (!rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ALL_BITS, &bit_chk, 0)) {
			break;
		}
	}

	scc_mgr_set_dqs_out1_delay(write_group, start_dqs);
	scc_mgr_set_oct_out1_delay(write_group, start_dqs);

	dqs_margin = d - start_dqs - 1;

	//USER time to center, +1 so that we don't go crazy centering DQ 

	mid = (dq_margin + dqs_margin + 1) / 2;

	gbl->fom_out += dq_margin + dqs_margin;
	TCLRPT_SET(debug_summary_report->fom_out, debug_summary_report->fom_out + (dq_margin + dqs_margin));
	TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][grp].fom_out, (dq_margin + dqs_margin));

#if ENABLE_DQS_OUT_CENTERING
	//USER center DQS ... if the headroom is setup properly we shouldn't need to 
	if (DDRX) {
		if (dqs_margin > mid) {
			scc_mgr_set_dqs_out1_delay(write_group, READ_SCC_DQS_IO_OUT1_DELAY() + dqs_margin - mid);
			scc_mgr_set_oct_out1_delay(write_group, READ_SCC_OCT_OUT1_DELAY(write_group) + dqs_margin - mid);
		}
	}
#endif

	scc_mgr_load_dqs_io ();
	scc_mgr_load_dqs_for_write_group (write_group);

	//USER center dq 

	if (dq_margin > mid) {
		for (i = 0, p = test_bgn; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++, p++) {
			scc_mgr_set_dq_out1_delay(write_group, i, READ_SCC_DQ_OUT1_DELAY(i) + dq_margin - mid);
			scc_mgr_load_dq (p, i);
		}
		dqs_margin += dq_margin - mid;
		dq_margin  -= dq_margin - mid;
	}

	//USER do dm centering 

	if (!RLDRAMX) {
		dm_margin = IO_IO_OUT1_DELAY_MAX;

		if (QDRII) {
			sticky_bit_chk = 0;
			for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
				max_working_dm[i] = 0;
			}
		}

		for (d = 1; d <= IO_IO_OUT1_DELAY_MAX; d++) {
			scc_mgr_apply_group_dm_out1_delay (write_group, d);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			if (DDRX) {
				if (rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0)) {
					max_working_dm[0] = d;
				} else {
					break;
				}
			} else {
				stop = !rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0);
				sticky_bit_chk = sticky_bit_chk | bit_chk;
				stop = stop && (sticky_bit_chk == param->read_correct_mask);

				if (stop == 1) {
					break;
				} else {
					for (i = 0; i < RW_MGR_MEM_DATA_MASK_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
						if ((bit_chk & param->dm_correct_mask) == param->dm_correct_mask) {
							max_working_dm[i] = d;
						}
						bit_chk = bit_chk >> (RW_MGR_MEM_DATA_WIDTH / RW_MGR_MEM_DATA_MASK_WIDTH);
					}
				}
			}
		}

		i = 0;
		for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
			if (max_working_dm[i] > mid) {
				scc_mgr_set_dm_out1_delay(write_group, i, max_working_dm[i] - mid);
			} else {
				scc_mgr_set_dm_out1_delay(write_group, i, 0);
			}

			scc_mgr_load_dm (i);

			if (max_working_dm[i] < dm_margin) {
				dm_margin = max_working_dm[i];
			}
		}
	} else {
		dm_margin = 0;
	}

	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

	return (dq_margin + dqs_margin) > 0;
}

#endif

//USER calibrate the write operations

alt_u32 rw_mgr_mem_calibrate_writes (alt_u32 rank_bgn, alt_u32 g, alt_u32 test_bgn)
{
	//USER update info for sims

	TRACE_FUNC("%lu %lu", g, test_bgn);
	
	reg_file_set_stage(CAL_STAGE_WRITES);
	reg_file_set_sub_stage(CAL_SUBSTAGE_WRITES_CENTER);

	//USER starting phases 

	//USER update info for sims

	reg_file_set_group(g);

	if (!rw_mgr_mem_calibrate_writes_center (rank_bgn, g, test_bgn)) {
		set_failing_group_stage(g, CAL_STAGE_WRITES, CAL_SUBSTAGE_WRITES_CENTER);
		return 0;
	}


	return 1;
}

// helpful for creating eye diagrams 
// TODO: This is for the TCL DBG... but obviously it serves no purpose...
// Decide what to do with it!

void rw_mgr_mem_calibrate_eye_diag_aid (void)
{
	// no longer exists
}

// TODO: This needs to be update to properly handle the number of failures
// Right now it only checks if the write test was successful or not
alt_u32 rw_mgr_mem_calibrate_full_test (alt_u32 min_correct, t_btfld *bit_chk, alt_u32 test_dm)
{
	alt_u32 g;
	alt_u32 success = 0;
	alt_u32 run_groups = ~param->skip_groups;

	TRACE_FUNC("%lu %lu", min_correct, test_dm);
	
	for (g = 0; g < RW_MGR_MEM_IF_READ_DQS_WIDTH; g++) {
		if (run_groups & ((1 << RW_MGR_NUM_DQS_PER_WRITE_GROUP) - 1))
		{
			success = rw_mgr_mem_calibrate_write_test_all_ranks (g, test_dm, PASS_ALL_BITS, bit_chk);
		}
		run_groups = run_groups >> RW_MGR_NUM_DQS_PER_WRITE_GROUP;
	}

	return success;
}

#if ENABLE_TCL_DEBUG
// see how far we can push a particular DQ pin before complete failure on input and output sides
// NOTE: if ever executing a run_*_margining function outside of calibration context you must first issue IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 1);
void run_dq_margining (alt_u32 rank_bgn, alt_u32 write_group)
{
	alt_u32 test_num;
	alt_u32 read_group;
	alt_u32 read_test_bgn;
	alt_u32 subdq;
	alt_u32 dq;
	alt_u32 delay;
	alt_u32 calibrated_delay;
	alt_u32 working_cnt;
	t_btfld bit_chk;
	t_btfld bit_chk_test = 0;
	t_btfld bit_chk_mask;

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	select_curr_shadow_reg_using_rank(rank_bgn);

	// Load the read patterns
	rw_mgr_mem_calibrate_read_load_patterns (rank_bgn, 0);

	// sweep input delays
	for (read_group = write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH, read_test_bgn = 0;
		 read_group < (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
		 read_group++, read_test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS)
	{

		ALTERA_ASSERT(read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH);

		for (subdq = 0; subdq < RW_MGR_MEM_DQ_PER_READ_DQS; subdq++)
		{
			dq = read_group*RW_MGR_MEM_DQ_PER_READ_DQS + subdq;

			ALTERA_ASSERT(dq < RW_MGR_MEM_DATA_WIDTH);

			calibrated_delay =  debug_cal_report->cal_dq_settings[curr_shadow_reg][dq].dq_in_delay;

			working_cnt = 0;

			bit_chk_test = 0;

			// Find the left edge
			for (delay = calibrated_delay; delay <= IO_IO_IN_DELAY_MAX; delay++)
			{
				WRITE_SCC_DQ_IN_DELAY((subdq + read_test_bgn), delay);
				scc_mgr_load_dq (subdq + read_test_bgn);
				IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

				for (test_num = 0; test_num < NUM_READ_TESTS; test_num++)
				{
					rw_mgr_mem_calibrate_read_test (rank_bgn, read_group, 1, PASS_ONE_BIT, &bit_chk, 0, 0);
					if (test_num == 0)
					{
						bit_chk_test = bit_chk;
					}
					else
					{
						bit_chk_test &= bit_chk;
					}
				}

				// Check only the bit we are testing
				bit_chk_mask = (bit_chk_test & (((t_btfld) 1) << ((t_btfld) subdq)));
				if (bit_chk_mask == 0)
				{
					break;
				}

				working_cnt++;
			}

			// Restore the settings
			WRITE_SCC_DQ_IN_DELAY((subdq + read_test_bgn), calibrated_delay);
			scc_mgr_load_dq (subdq + read_test_bgn);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			// Store the setting
			TCLRPT_SET(debug_margin_report->margin_dq_in_margins[curr_shadow_reg][dq].min_working_setting, working_cnt);

			// Find the right edge
			calibrated_delay = debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][read_group].dqs_bus_in_delay;

			working_cnt = 0;
			for (delay = calibrated_delay; delay <= IO_DQS_IN_DELAY_MAX; delay++)
			{
				WRITE_SCC_DQS_IN_DELAY(read_group, delay);
				scc_mgr_load_dqs(read_group);
				IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

				for (test_num = 0; test_num < NUM_READ_TESTS; test_num++)
				{
					rw_mgr_mem_calibrate_read_test (rank_bgn, read_group, 1, PASS_ONE_BIT, &bit_chk, 0, 0);
					if (test_num == 0)
					{
						bit_chk_test = bit_chk;
					}
					else
					{
						bit_chk_test &= bit_chk;
					}
				}

				// Check only the bit we are testing
				bit_chk_mask = (bit_chk_test & (((t_btfld)1) << ((t_btfld)(subdq))));
				if (bit_chk_mask == 0)
				{
					break;
				}

				working_cnt++;
			}

			// Restore the settings
			WRITE_SCC_DQS_IN_DELAY(read_group, calibrated_delay);
			scc_mgr_load_dqs(read_group);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			// Store the setting
			TCLRPT_SET(debug_margin_report->margin_dq_in_margins[curr_shadow_reg][dq].max_working_setting, working_cnt);

		}
	}

	// sweep output delays
	for (subdq = 0; subdq < RW_MGR_MEM_DQ_PER_WRITE_DQS; subdq++)
	{
		dq = write_group*RW_MGR_MEM_DQ_PER_WRITE_DQS + subdq;

		calibrated_delay = debug_cal_report->cal_dq_settings[curr_shadow_reg][dq].dq_out_delay1;
		working_cnt = 0;

		// Find the left edge
		for (delay = calibrated_delay; delay <= IO_IO_OUT1_DELAY_MAX; delay++)
		{
			WRITE_SCC_DQ_OUT1_DELAY(subdq, delay);
			scc_mgr_load_dq (subdq);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			for (test_num = 0; test_num < NUM_WRITE_TESTS; test_num++)
			{
				rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ALL_BITS, &bit_chk, 0);
				if (test_num == 0)
				{
					bit_chk_test = bit_chk;
				}
				else
				{
					bit_chk_test &= bit_chk;
				}
			}

			// Check only the bit we are testing
			bit_chk_mask = (bit_chk_test & (((t_btfld)1) << ((t_btfld)subdq)));
			if (bit_chk_mask == 0)
			{
				break;
			}

			working_cnt++;
		}

		// Restore the settings
		WRITE_SCC_DQ_OUT1_DELAY(subdq, calibrated_delay);
		scc_mgr_load_dq (subdq);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		// Store the setting
		TCLRPT_SET(debug_margin_report->margin_dq_out_margins[curr_shadow_reg][dq].min_working_setting, working_cnt);

		// Find the right edge
		calibrated_delay = debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].dqs_out_delay1;

		working_cnt = 0;
		for (delay = calibrated_delay; delay <= IO_IO_OUT1_DELAY_MAX; delay++)
		{
			WRITE_SCC_DQS_IO_OUT1_DELAY(delay);
			scc_mgr_load_dqs_io();
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
			if (QDRII)
			{
				rw_mgr_mem_dll_lock_wait();
			}

			for (test_num = 0; test_num < NUM_WRITE_TESTS; test_num++)
			{
				rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 0, PASS_ONE_BIT, &bit_chk, 0);
				if (test_num == 0)
				{
					bit_chk_test = bit_chk;
				}
				else
				{
					bit_chk_test &= bit_chk;
				}
			}

			// Check only the bit we are testing
			bit_chk_mask = (bit_chk_test & (((t_btfld)1) << ((t_btfld)subdq)));
			if (bit_chk_mask == 0)
			{
				break;
			}

			working_cnt++;
		}

		//USER Restore the settings
		if (QDRII) {
			scc_mgr_set_group_dqs_io_and_oct_out1_gradual (write_group, calibrated_delay);
		} else {
			scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, calibrated_delay);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		}

		// Store the setting
		TCLRPT_SET(debug_margin_report->margin_dq_out_margins[curr_shadow_reg][dq].max_working_setting, working_cnt);
	}
}
#endif

#if ENABLE_TCL_DEBUG
// NOTE: if ever executing a run_*_margining function outside of calibration context you must first issue IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 1);
void run_dm_margining (alt_u32 rank_bgn, alt_u32 write_group)
{
	alt_u32 test_status;
	alt_u32 test_num;
	alt_u32 dm;
	alt_u32 delay;
	alt_u32 calibrated_delay;
	alt_u32 working_cnt;
	t_btfld bit_chk;

	ALTERA_ASSERT(write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH);

	select_curr_shadow_reg_using_rank(rank_bgn);

	// sweep output delays
	for (dm = 0; dm < RW_MGR_NUM_DM_PER_WRITE_GROUP; dm++)
	{

		calibrated_delay = debug_cal_report->cal_dm_settings[curr_shadow_reg][write_group][dm].dm_out_delay1;
		working_cnt = 0;

		// Find the left edge
		for (delay = calibrated_delay; delay <= IO_IO_OUT1_DELAY_MAX; delay++)
		{
			WRITE_SCC_DM_IO_OUT1_DELAY(dm, delay);
			scc_mgr_load_dm (dm);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			test_status = 1;
			for (test_num = 0; test_num < NUM_WRITE_TESTS; test_num++)
			{
				if (!rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0))
				{
					test_status = 0;
					break;
				}
			}

			if (test_status == 0)
			{
				break;
			}

			working_cnt++;
		}

		// Restore the settings
		WRITE_SCC_DM_IO_OUT1_DELAY(dm, calibrated_delay);
		scc_mgr_load_dm (dm);
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

		// Store the setting
		TCLRPT_SET(debug_margin_report->margin_dm_margins[curr_shadow_reg][write_group][dm].min_working_setting, working_cnt);

		// Find the right edge
		calibrated_delay = debug_cal_report->cal_dqs_out_settings[curr_shadow_reg][write_group].dqs_out_delay1;

		working_cnt = 0;
		for (delay = calibrated_delay; delay <= IO_IO_OUT1_DELAY_MAX; delay++)
		{
			WRITE_SCC_DQS_IO_OUT1_DELAY(delay);
			scc_mgr_load_dqs_io();
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
			if (QDRII)
			{
				rw_mgr_mem_dll_lock_wait();
			}

			test_status = 1;
			for (test_num = 0; test_num < NUM_WRITE_TESTS; test_num++)
			{
				if (!rw_mgr_mem_calibrate_write_test (rank_bgn, write_group, 1, PASS_ALL_BITS, &bit_chk, 0))
				{
					test_status = 0;
					break;
				}
			}

			if (test_status == 0)
			{
				break;
			}

			working_cnt++;
		}

		//USER Restore the settings
		if (QDRII) {
			scc_mgr_set_group_dqs_io_and_oct_out1_gradual (write_group, calibrated_delay);
		} else {
			scc_mgr_apply_group_dqs_io_and_oct_out1 (write_group, calibrated_delay);
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
		}

		// Store the setting
		TCLRPT_SET(debug_margin_report->margin_dm_margins[curr_shadow_reg][write_group][dm].max_working_setting, working_cnt);

	}
}
#endif


//USER precharge all banks and activate row 0 in bank "000..." and bank "111..." 
#if DDRX
void mem_precharge_and_activate (void)
{
	alt_u32 r;

	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_OFF);

		//USER precharge all banks ... 
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x0F);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_ACTIVATE_0_AND_1_WAIT1);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x0F);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_ACTIVATE_0_AND_1_WAIT2);

		//USER activate rows 
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_ACTIVATE_0_AND_1);
	}
}
#endif

#if QDRII || RLDRAMX
void mem_precharge_and_activate (void) {}
#endif

//USER perform all refreshes necessary over all ranks
#if (ENABLE_NON_DESTRUCTIVE_CALIB || ENABLE_NON_DES_CAL)
// Only have DDR3 version for now
#if DDR3
alt_u32 mem_refresh_all_ranks (alt_u32 no_validate)
{
	const alt_u32 T_REFI_NS = 3900;                      // JEDEC spec refresh interval in ns (industrial temp)
//	const alt_u32 T_RFC_NS = 350;                        // Worst case REFRESH-REFRESH or REFRESH-ACTIVATE wait time in ns
	                                                     // Alternatively, we could extract T_RFC from uniphy_gen.tcl
	const alt_u32 T_RFC_AFI = 350 * AFI_CLK_FREQ / 1000; // T_RFC expressed in mem clk cycles (will be less than 256)
#if (ENABLE_NON_DESTRUCTIVE_CALIB)	
	const alt_u32 NUM_REFRESH_POSTING = 8192;            // Number of consecutive refresh commands supported by Micron DDR3 devices
#else
	const alt_u32 NUM_REFRESH_POSTING = 8;  
#endif
	alt_u32 i;
	alt_u32 elapsed_time;  // In AVL clock cycles
	
#if (ENABLE_NON_DESTRUCTIVE_CALIB)	
	//USER Reset the refresh interval timer
	elapsed_time = IORD_32DIRECT (BASE_TIMER, 0);
	IOWR_32DIRECT (BASE_TIMER, 0, 0x00);

	//USER Validate that maximum refresh interval is not exceeded
	if ( !no_validate ) {
		if (!(~elapsed_time) || elapsed_time > (NUM_REFRESH_POSTING * T_REFI_NS * AVL_CLK_FREQ / 1000) ) {
			// Non-destructive calibration failure
			return 0; 
		}
	}		
	
#endif
	//USER set CS and ODT mask
	if ( RDIMM || LRDIMM ) {
		if (RW_MGR_MEM_NUMBER_OF_RANKS == 1) {
			set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
		}
		else {
			// Only single-rank DIMM supported for non-destructive cal
			return 0;
		}
	}
	else { // UDIMM
		// Issue refreshes to all ranks simultaneously
		IOWR_32DIRECT (RW_MGR_SET_CS_AND_ODT_MASK, 0, RW_MGR_RANK_ALL);
	}
	
	//USER Precharge all banks
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_PRECHARGE_ALL);
	// Wait for tRP = 15ns before issuing REFRESH commands
	// No need to insert explicit delay; simulation shows more than 1000 ns between PRECHARGE and first REFRESH
	
	//USER Issue refreshes
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_REFRESH_ALL);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_REFRESH_DELAY);
	for (i = 0; i < NUM_REFRESH_POSTING; i += 256) {
		// Issue 256 REFRESH commands, waiting t_RFC between consecutive refreshes

#if (ENABLE_NON_DESTRUCTIVE_CALIB)			
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0xFF);
#else
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x07);
#endif
		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, T_RFC_AFI);
		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_REFRESH_ALL);
	}

	//USER Re-activate all banks
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x00); // No need to wait between commands to activate different banks (since ACTIVATE is preceded by tRFC wait)
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x0F); // Wait for ACTIVATE to complete
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_ACTIVATE_0_AND_1_WAIT1);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_ACTIVATE_0_AND_1_WAIT2);
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_ACTIVATE_0_AND_1);
	
	return 1;
}
#else
alt_u32 mem_refresh_all_ranks (alt_u32 no_validate)
{
	return 1;
}
#endif
#endif

//USER Configure various memory related parameters.
 
#if DDRX
void mem_config (void)
{
	alt_u32 rlat, wlat;
	alt_u32 rw_wl_nop_cycles;
	alt_u32 max_latency;
#if CALIBRATE_BIT_SLIPS
	alt_u32 i;
#endif	

	TRACE_FUNC();
	
	//USER read in write and read latency 

	wlat = IORD_32DIRECT (MEM_T_WL_ADD, 0);
#if HARD_PHY
	wlat += IORD_32DIRECT (DATA_MGR_MEM_T_ADD, 0); /* WL for hard phy does not include additive latency */
	
	#if DDR3 || DDR2
		// YYONG: add addtional write latency to offset the address/command extra clock cycle
		// YYONG: We change the AC mux setting causing AC to be delayed by one mem clock cycle
		// YYONG: only do this for DDR3
		wlat = wlat + 1;
	#endif
#endif
	
	rlat = IORD_32DIRECT (MEM_T_RL_ADD, 0);

	if (QUARTER_RATE_MODE) {
		//USER In Quarter-Rate the WL-to-nop-cycles works like this
		//USER 0,1     -> 0
		//USER 2,3,4,5 -> 1
		//USER 6,7,8,9 -> 2
		//USER etc...
		rw_wl_nop_cycles = (wlat + 6) / 4 - 1;
	} 
	else if(HALF_RATE_MODE)	{
		//USER In Half-Rate the WL-to-nop-cycles works like this
		//USER 0,1 -> -1
		//USER 2,3 -> 0
		//USER 4,5 -> 1
		//USER etc...
		if(wlat % 2)
		{
			rw_wl_nop_cycles = ((wlat - 1) / 2) - 1;
		}
		else
		{
			rw_wl_nop_cycles = (wlat / 2) - 1;
		}
	}
	else {
		rw_wl_nop_cycles = wlat - 2;
#if LPDDR2
		rw_wl_nop_cycles = rw_wl_nop_cycles + 1;
#endif
	}
#if MULTIPLE_AFI_WLAT
	for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
		gbl->rw_wl_nop_cycles_per_group[i] = rw_wl_nop_cycles;
	}	
#endif	
	gbl->rw_wl_nop_cycles = rw_wl_nop_cycles;

#if ARRIAV || CYCLONEV
	//USER For AV/CV, lfifo is hardened and always runs at full rate
	//USER so max latency in AFI clocks, used here, is correspondingly smaller
	if (QUARTER_RATE_MODE) {
		max_latency = (1<<MAX_LATENCY_COUNT_WIDTH)/4 - 1;
	} else if (HALF_RATE_MODE) {
		max_latency = (1<<MAX_LATENCY_COUNT_WIDTH)/2 - 1;
	} else {
		max_latency = (1<<MAX_LATENCY_COUNT_WIDTH)/1 - 1;
	}
#else
	max_latency = (1<<MAX_LATENCY_COUNT_WIDTH) - 1;
#endif		
	//USER configure for a burst length of 8

	if (QUARTER_RATE_MODE) {
		//USER write latency 
		wlat = (wlat + 5) / 4 + 1;
		
		//USER set a pretty high read latency initially
		gbl->curr_read_lat = (rlat + 1) / 4 + 8;
	} else if (HALF_RATE_MODE) {
		//USER write latency 
		wlat = (wlat - 1) / 2 + 1;

		//USER set a pretty high read latency initially 
		gbl->curr_read_lat = (rlat + 1) / 2 + 8;
	} else {
		//USER write latency 
#if HARD_PHY
		// Adjust Write Latency for Hard PHY
		wlat = wlat + 1;
#if LPDDR2
		// Add another one in hard for LPDDR2 since this value is raw from controller
		// assume tdqss is one
		wlat = wlat + 1;
#endif
#endif

		//USER set a pretty high read latency initially 
		gbl->curr_read_lat = rlat + 16;
	}

	if (gbl->curr_read_lat > max_latency) {
		gbl->curr_read_lat = max_latency;
	}
	IOWR_32DIRECT (PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
	
	//USER advertise write latency 
	gbl->curr_write_lat = wlat;
#if MULTIPLE_AFI_WLAT
	for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
#if HARD_PHY		
		IOWR_32DIRECT (PHY_MGR_AFI_WLAT, i*4, wlat - 2);
#else
		IOWR_32DIRECT (PHY_MGR_AFI_WLAT, i*4, wlat - 1);
#endif
	}
#else
#if HARD_PHY
	IOWR_32DIRECT (PHY_MGR_AFI_WLAT, 0, wlat - 2);
#else
	IOWR_32DIRECT (PHY_MGR_AFI_WLAT, 0, wlat - 1);
#endif
#endif
	
	//USER initialize bit slips
#if CALIBRATE_BIT_SLIPS
	for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
		IOWR_32DIRECT (PHY_MGR_FR_SHIFT, i*4, 0);
	}
#endif	
	
	
	mem_precharge_and_activate ();
}
#endif

#if QDRII || RLDRAMX
void mem_config (void)
{
	alt_u32 wlat, nop_cycles, max_latency;

	TRACE_FUNC();

	max_latency = (1<<MAX_LATENCY_COUNT_WIDTH) - 1;
	
	if (QUARTER_RATE_MODE) {
		// TODO_JCHOI: verify confirm
		gbl->curr_read_lat = (IORD_32DIRECT (MEM_T_RL_ADD, 0) + 1) / 4 + 8;
	} else if (HALF_RATE_MODE) {
		gbl->curr_read_lat = (IORD_32DIRECT (MEM_T_RL_ADD, 0) + 1) / 2 + 8;
	} else {
		gbl->curr_read_lat = IORD_32DIRECT (MEM_T_RL_ADD, 0) + 16;
	}
	if (gbl->curr_read_lat > max_latency) {
		gbl->curr_read_lat = max_latency;
	}
	IOWR_32DIRECT (PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
	
	if (RLDRAMX)
	{
		//USER read in write and read latency 
		wlat = IORD_32DIRECT (MEM_T_WL_ADD, 0);
		
		if (QUARTER_RATE_MODE) 
		{
			// TODO_JCHOI Verify
			nop_cycles = ((wlat - 1) / 4) - 1;
		}
		else if (HALF_RATE_MODE)
		{
#if HR_DDIO_OUT_HAS_THREE_REGS
			nop_cycles = (wlat / 2) - 2;
#else		
	#if RLDRAM3
			// RLDRAM3 uses all AFI phases to issue commands
			nop_cycles = (wlat / 2) - 2;	
	#else
			nop_cycles = ((wlat + 1) / 2) - 2;	
	#endif
#endif			
		}
		else
		{
			nop_cycles = wlat - 1;
		}
		gbl->rw_wl_nop_cycles = nop_cycles;
	}
}
#endif

//USER Set VFIFO and LFIFO to instant-on settings in skip calibration mode

void mem_skip_calibrate (void)
{
	alt_u32 vfifo_offset;
	alt_u32 i, j, r;
#if HCX_COMPAT_MODE && DDR3
	alt_u32 v;
#if (RDIMM || LRDIMM)
	alt_u32 increment = 2;
#else
	alt_u32 wlat = IORD_32DIRECT (PHY_MGR_MEM_T_WL, 0);
	alt_u32 rlat = IORD_32DIRECT (PHY_MGR_MEM_T_RL, 0);
	alt_u32 increment = rlat - wlat*2 + 1;
#endif
#endif

	TRACE_FUNC();

	// Need to update every shadow register set used by the interface	
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r += NUM_RANKS_PER_SHADOW_REG) {

		// Strictly speaking this should be called once per group to make
		// sure each group's delay chains are refreshed from the SCC register file,
		// but since we're resetting all delay chains anyway, we can save some
		// runtime by calling select_shadow_regs_for_update just once to switch rank.
		select_shadow_regs_for_update(r, 0, 1);
	
		//USER Set output phase alignment settings appropriate for skip calibration
		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {

#if STRATIXV || ARRIAV || CYCLONEV || ARRIAVGZ
			scc_mgr_set_dqs_en_phase(i, 0);
#else
#if IO_DLL_CHAIN_LENGTH == 6
			scc_mgr_set_dqs_en_phase(i, (IO_DLL_CHAIN_LENGTH >> 1) - 1);
#else
			scc_mgr_set_dqs_en_phase(i, (IO_DLL_CHAIN_LENGTH >> 1));
#endif
#endif
#if HCX_COMPAT_MODE && DDR3
			v = 0;
			for (j = 0; j < increment; j++) {
				rw_mgr_incr_vfifo(i, &v);
			}

#if IO_DLL_CHAIN_LENGTH == 6
			scc_mgr_set_dqdqs_output_phase(i, 6);
#else
			scc_mgr_set_dqdqs_output_phase(i, 7);
#endif
#else
	#if HCX_COMPAT_MODE
			// in this mode, write_clk doesn't always lead mem_ck by 90 deg, and so
			// the enhancement in case:33398 can't be applied.
			scc_mgr_set_dqdqs_output_phase(i, (IO_DLL_CHAIN_LENGTH - IO_DLL_CHAIN_LENGTH / 3));
	#else
			// Case:33398
			//
			// Write data arrives to the I/O two cycles before write latency is reached (720 deg).
			//   -> due to bit-slip in a/c bus
			//   -> to allow board skew where dqs is longer than ck 
			//      -> how often can this happen!?
			//      -> can claim back some ptaps for high freq support if we can relax this, but i digress...
			//
			// The write_clk leads mem_ck by 90 deg
			// The minimum ptap of the OPA is 180 deg
			// Each ptap has (360 / IO_DLL_CHAIN_LENGH) deg of delay
			// The write_clk is always delayed by 2 ptaps
			//
			// Hence, to make DQS aligned to CK, we need to delay DQS by:
			//    (720 - 90 - 180 - 2 * (360 / IO_DLL_CHAIN_LENGTH))
			//
			// Dividing the above by (360 / IO_DLL_CHAIN_LENGTH) gives us the number of ptaps, which simplies to:
			//
			//    (1.25 * IO_DLL_CHAIN_LENGTH - 2)
			scc_mgr_set_dqdqs_output_phase(i, (1.25 * IO_DLL_CHAIN_LENGTH - 2));
	#endif
#endif	
		}

		IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, 0xff);
		IOWR_32DIRECT (SCC_MGR_DQS_IO_ENA, 0, 0xff);

		for (i = 0; i < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; i++) {
			IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, i);
			IOWR_32DIRECT (SCC_MGR_DQ_ENA, 0, 0xff);
			IOWR_32DIRECT (SCC_MGR_DM_ENA, 0, 0xff);
		}

#if USE_SHADOW_REGS		
		//USER in shadow-register mode, SCC_UPDATE is done on a per-group basis
		//USER unless we explicitly ask for a multicast via the group counter
		IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, 0xFF);
#endif		
		IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	}

#if ARRIAV || CYCLONEV
	// Compensate for simulation model behaviour 
	for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
		scc_mgr_set_dqs_bus_in_delay(i, 10);
		scc_mgr_load_dqs (i);
	}
	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
#endif

#if ARRIAV || CYCLONEV
	//ArriaV has hard FIFOs that can only be initialized by incrementing in sequencer
	vfifo_offset = CALIB_VFIFO_OFFSET;
	for (j = 0; j < vfifo_offset; j++) {
		if(HARD_PHY) {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_HARD_PHY, 0, 0xff);
		} else {
			IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR, 0, 0xff);
		}
	}
#else
// Note, this is not currently supported; changing this might significantly
// increase the size of the ROM
#if SUPPORT_DYNAMIC_SKIP_CALIBRATE_ACTIONS
	if ((DYNAMIC_CALIB_STEPS) & CALIB_IN_RTL_SIM) {
		//USER VFIFO is reset to the correct settings in RTL simulation 
	} else {
		vfifo_offset = IORD_32DIRECT (PHY_MGR_CALIB_VFIFO_OFFSET, 0);

		if (QUARTER_RATE_MODE) {
			while (vfifo_offset > 3) {
				IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_QR, 0, 0xff);
				vfifo_offset -= 4;
			}

			if (vfifo_offset == 3) {
				IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR_HR, 0, 0xff);
			} else if (vfifo_offset == 2) {
				IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_HR, 0, 0xff);
			} else if (vfifo_offset == 1) {
				IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR, 0, 0xff);
			}
		} else {
			while (vfifo_offset > 1) {
				IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_HR, 0, 0xff);
				vfifo_offset -= 2;
			}
	
			if (vfifo_offset == 1) {
				IOWR_32DIRECT (PHY_MGR_CMD_INC_VFIFO_FR, 0, 0xff);
			}
		}
	}
#endif
#endif


	IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);

#if ARRIAV || CYCLONEV
	// For ACV with hard lfifo, we get the skip-cal setting from generation-time constant
	gbl->curr_read_lat = CALIB_LFIFO_OFFSET;
#else
	gbl->curr_read_lat = IORD_32DIRECT (PHY_MGR_CALIB_LFIFO_OFFSET, 0);
#endif
	IOWR_32DIRECT (PHY_MGR_PHY_RLAT, 0, gbl->curr_read_lat);
}


#if BFM_MODE
void print_group_settings(alt_u32 group, alt_u32 dq_begin)
{
	int i;
	
	fprintf(bfm_gbl.outfp, "Group %lu (offset %lu)\n", group, dq_begin);

	fprintf(bfm_gbl.outfp, "Output:\n");
	fprintf(bfm_gbl.outfp, "dqdqs_out_phase: %2u\n", READ_SCC_DQDQS_OUT_PHASE(group));
	fprintf(bfm_gbl.outfp, "dqs_out1_delay:  %2u\n", READ_SCC_DQS_IO_OUT1_DELAY());
	fprintf(bfm_gbl.outfp, "dqs_out2_delay:  %2u\n", READ_SCC_DQS_IO_OUT2_DELAY());
	fprintf(bfm_gbl.outfp, "oct_out1_delay:  %2u\n", READ_SCC_OCT_OUT1_DELAY(group));
	fprintf(bfm_gbl.outfp, "oct_out2_delay:  %2u\n", READ_SCC_OCT_OUT2_DELAY(group));
	fprintf(bfm_gbl.outfp, "dm_out1:         ");
	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		fprintf(bfm_gbl.outfp, "%2u ", READ_SCC_DM_IO_OUT1_DELAY(i));
	}
	fprintf(bfm_gbl.outfp, "\n");
	fprintf(bfm_gbl.outfp, "dm_out2:         ");
	for (i = 0; i < RW_MGR_NUM_DM_PER_WRITE_GROUP; i++) {
		fprintf(bfm_gbl.outfp, "%2u ", READ_SCC_DM_IO_OUT2_DELAY(i));
	}
	fprintf(bfm_gbl.outfp, "\n");
	fprintf(bfm_gbl.outfp, "dq_out1:         ");
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		fprintf(bfm_gbl.outfp, "%2u ", READ_SCC_DQ_OUT1_DELAY(i));
	}
	fprintf(bfm_gbl.outfp, "\n");
	fprintf(bfm_gbl.outfp, "dq_out2:         ");
	for (i = 0; i < RW_MGR_MEM_DQ_PER_WRITE_DQS; i++) {
		fprintf(bfm_gbl.outfp, "%2u ", READ_SCC_DQ_OUT2_DELAY(i));
	}
	fprintf(bfm_gbl.outfp, "\n");

	fprintf(bfm_gbl.outfp, "Input:\n");
	fprintf(bfm_gbl.outfp, "dqs_en_phase:    %2u\n", READ_SCC_DQS_EN_PHASE(group));
	fprintf(bfm_gbl.outfp, "dqs_en_delay:    %2u\n", READ_SCC_DQS_EN_DELAY(group));
	fprintf(bfm_gbl.outfp, "dqs_in_delay:    %2u\n", READ_SCC_DQS_IN_DELAY(group));
	fprintf(bfm_gbl.outfp, "dq_in:           ");
	for (i = 0; i < RW_MGR_MEM_DQ_PER_READ_DQS; i++) {
		fprintf(bfm_gbl.outfp, "%2u ", READ_SCC_DQ_IN_DELAY(i));
	}
	fprintf(bfm_gbl.outfp, "\n");

	fprintf(bfm_gbl.outfp, "\n");

	fflush(bfm_gbl.outfp);
}

#endif

#if RUNTIME_CAL_REPORT
void print_report(alt_u32 pass)
{
	RPRINT("Calibration Summary");
	char *stage_name, *substage_name;
	
	if(pass) {
		RPRINT("Calibration Passed");
		RPRINT("FOM IN  = %lu", gbl->fom_in);
		RPRINT("FOM OUT = %lu", gbl->fom_out);
	} else {
		RPRINT("Calibration Failed");
		switch (gbl->error_stage) {
			case CAL_STAGE_NIL:
				stage_name = "NIL";
				substage_name = "NIL";
			case CAL_STAGE_VFIFO:
				stage_name = "VFIFO";
				switch (gbl->error_substage) {
					case CAL_SUBSTAGE_GUARANTEED_READ:
						substage_name = "GUARANTEED READ";
						break;
					case CAL_SUBSTAGE_DQS_EN_PHASE:
						substage_name = "DQS ENABLE PHASE";
						break;
					case CAL_SUBSTAGE_VFIFO_CENTER:
						substage_name = "Read Per-Bit Deskew";
						break;
					default:
						substage_name = "NIL";
				}
				break;
			case CAL_STAGE_WLEVEL:
				stage_name = "WRITE LEVELING";
				switch (gbl->error_substage) {
					case CAL_SUBSTAGE_WORKING_DELAY:
						substage_name = "DQS Window Left Edge"; //need a more descriptive name
						break;
					case CAL_SUBSTAGE_LAST_WORKING_DELAY:
						substage_name = "DQS Window Right Edge";
						break;
					case CAL_SUBSTAGE_WLEVEL_COPY:
						substage_name = "WRITE LEVEL COPY";
						break;
					default:
						substage_name = "NIL";
				}
				break;
			case CAL_STAGE_LFIFO:
				stage_name = "LFIFO";
				substage_name = "READ LATENCY";
				break;
			case CAL_STAGE_WRITES:
				stage_name = "WRITES";
				substage_name = "Write Per-Bit Deskew";
				break;
			case CAL_STAGE_FULLTEST:
				stage_name = "FULL TEST";
				substage_name = "FULL TEST";
				break;
			case CAL_STAGE_REFRESH:
				stage_name = "REFRESH";
				substage_name = "REFRESH";
				break;
			case CAL_STAGE_CAL_SKIPPED:
				stage_name = "SKIP CALIBRATION"; //hw: is this needed
				substage_name = "SKIP CALIBRATION";
				break;
			case CAL_STAGE_CAL_ABORTED:
				stage_name = "ABORTED CALIBRATION"; //hw: hum???
				substage_name = "ABORTED CALIBRATION";
				break;
			case CAL_STAGE_VFIFO_AFTER_WRITES:
				stage_name = "READ Fine-tuning";
				switch (gbl->error_substage) {
					case CAL_SUBSTAGE_GUARANTEED_READ:
						substage_name = "GUARANTEED READ";
						break;
					case CAL_SUBSTAGE_DQS_EN_PHASE:
						substage_name = "DQS ENABLE PHASE";
						break;
					case CAL_SUBSTAGE_VFIFO_CENTER:
						substage_name = "VFIFO CENTER";
						break;
					default:
						substage_name = "NIL";
				}
				break;
			default:
				stage_name = "NIL";
				substage_name = "NIL";
		}
		RPRINT("Error Stage   : %lu - %s", gbl->error_stage, stage_name);
		RPRINT("Error Substage: %lu - %s", gbl->error_substage, substage_name);
		RPRINT("Error Group   : %lu", gbl->error_group);
	}
}
#endif //RUNTIME_CAL_REPORT

//USER Memory calibration entry point
 
alt_u32 mem_calibrate (void)
{
	alt_u32 i;
	alt_u32 rank_bgn, sr;
	alt_u32 write_group, write_test_bgn;
	alt_u32 read_group, read_test_bgn;
	alt_u32 run_groups, current_run;
	alt_u32 failing_groups = 0;
	alt_u32 group_failed = 0;
	alt_u32 sr_failed = 0;

	TRACE_FUNC();
	
	// Initialize the data settings
	DPRINT(1, "Preparing to init data");
#if ENABLE_TCL_DEBUG
	tclrpt_initialize_data();
#endif
	DPRINT(1, "Init complete");

	gbl->error_substage = CAL_SUBSTAGE_NIL;
	gbl->error_stage = CAL_STAGE_NIL;
	gbl->error_group = 0xff;
	gbl->fom_in = 0;
	gbl->fom_out = 0;

	TCLRPT_SET(debug_summary_report->cal_read_latency, 0);
	TCLRPT_SET(debug_summary_report->cal_write_latency, 0);

	mem_config ();

	if(ARRIAV || CYCLONEV) {
		alt_u32 bypass_mode = (HARD_PHY) ? 0x1 : 0x0;
		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, i);
			scc_set_bypass_mode (i, bypass_mode);
		}
	}
	
	if (((DYNAMIC_CALIB_STEPS) & CALIB_SKIP_ALL) == CALIB_SKIP_ALL) {
		//USER Set VFIFO and LFIFO to instant-on settings in skip calibration mode 

		mem_skip_calibrate ();
	} else {
		for (i = 0; i < NUM_CALIB_REPEAT; i++) {
		
			//USER Zero all delay chain/phase settings for all groups and all shadow register sets
			scc_mgr_zero_all ();

#if ENABLE_SUPER_QUICK_CALIBRATION
			for (write_group = 0, write_test_bgn = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++, write_test_bgn += RW_MGR_MEM_DQ_PER_WRITE_DQS)
			{
				IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, write_group);
				scc_mgr_zero_group (write_group, write_test_bgn, 0);
			}
#endif

			run_groups = ~param->skip_groups;

			for (write_group = 0, write_test_bgn = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++, write_test_bgn += RW_MGR_MEM_DQ_PER_WRITE_DQS)
			{
				// Initialized the group failure
				group_failed = 0;

				// Mark the group as being attempted for calibration
#if ENABLE_TCL_DEBUG
				tclrpt_set_group_as_calibration_attempted(write_group);
#endif

#if RLDRAMX || QDRII
				//Note:
				//  It seems that with rldram and qdr vfifo starts at max (not sure for ddr)
				//  also not sure if max is really vfifo_size-1 or vfifo_size
				BFM_GBL_SET(vfifo_idx,VFIFO_SIZE-1);
#else
				BFM_GBL_SET(vfifo_idx,0);
#endif
				current_run = run_groups & ((1 << RW_MGR_NUM_DQS_PER_WRITE_GROUP) - 1);
				run_groups = run_groups >> RW_MGR_NUM_DQS_PER_WRITE_GROUP;

				if (current_run == 0)
				{
					continue;
				}

				IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, write_group);
#if !ENABLE_SUPER_QUICK_CALIBRATION
				scc_mgr_zero_group (write_group, write_test_bgn, 0);
#endif

				for (read_group = write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH, read_test_bgn = 0;
				     read_group < (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH && group_failed == 0;
				     read_group++, read_test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS) {

					//USER Calibrate the VFIFO 
					if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_VFIFO)) {
						if (!rw_mgr_mem_calibrate_vfifo (read_group, read_test_bgn)) {
							group_failed = 1;
							
							if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_SWEEP_ALL_GROUPS)) {
								return 0;
							}
						}
					}
				}

				//USER level writes (or align DK with CK for RLDRAMX) 
				if (group_failed == 0)
				{
					if ((DDRX || RLDRAMII) && !(ARRIAV || CYCLONEV))
					{
						if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WLEVEL)) {
							if (!rw_mgr_mem_calibrate_wlevel (write_group, write_test_bgn)) {
								group_failed = 1;
								
								if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_SWEEP_ALL_GROUPS)) {
									return 0;
								}
							}
						}
					}
				}

				//USER Calibrate the output side 
				if (group_failed == 0)
				{
					for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS; rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {
						sr_failed = 0;
						if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)) {
							if ((STATIC_CALIB_STEPS) & CALIB_SKIP_DELAY_SWEEPS) {
								//USER not needed in quick mode!
							} else {
								//USER Determine if this set of ranks should be skipped entirely
								if (! param->skip_shadow_regs[sr]) {
														
									//USER Select shadow register set
									select_shadow_regs_for_update(rank_bgn, write_group, 1);
							
									if (!rw_mgr_mem_calibrate_writes (rank_bgn, write_group, write_test_bgn)) {
										sr_failed = 1;
										if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_SWEEP_ALL_GROUPS)) {
											return 0;
										}
									}
								}
							}
						}
						if(sr_failed == 0) {
							TCLRPT_SET(debug_cal_report->cal_status_per_group[sr][write_group].error_stage, CAL_STAGE_NIL);
						} else {
							group_failed = 1;
						}
					}
				}
				
#if READ_AFTER_WRITE_CALIBRATION				
				if (group_failed == 0)
				{
					for (read_group = write_group * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH, read_test_bgn = 0;
						 read_group < (write_group + 1) * RW_MGR_MEM_IF_READ_DQS_WIDTH / RW_MGR_MEM_IF_WRITE_DQS_WIDTH && group_failed == 0;
						 read_group++, read_test_bgn += RW_MGR_MEM_DQ_PER_READ_DQS) {

						if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_WRITES)) {
							if (!rw_mgr_mem_calibrate_vfifo_end (read_group, read_test_bgn)) {
								group_failed = 1;
								
								if (!(gbl->phy_debug_mode_flags & PHY_DEBUG_SWEEP_ALL_GROUPS)) {
									return 0;
								}
							}
						}
					}
				}
#endif				
				
				if (group_failed == 0)
				{

#if BFM_MODE

					// TODO: should just update global BFM structure with all data
					// and print all out at the end
					print_group_settings(write_group, write_test_bgn);
#endif

#if STATIC_IN_RTL_SIM
#if ENABLE_TCL_DEBUG && BFM_MODE
	tclrpt_populate_fake_margin_data();
#endif
#else
#if ENABLE_TCL_DEBUG
				if (gbl->phy_debug_mode_flags & PHY_DEBUG_ENABLE_MARGIN_RPT)
				{
					// Run margining
					for (rank_bgn = 0, sr = 0; rank_bgn < RW_MGR_MEM_NUMBER_OF_RANKS; rank_bgn += NUM_RANKS_PER_SHADOW_REG, ++sr) {
					
						//USER Determine if this set of ranks should be skipped entirely
						if (! param->skip_shadow_regs[sr]) {
					
							//USER Select shadow register set
							select_shadow_regs_for_update(rank_bgn, write_group, 1);
											
							run_dq_margining(rank_bgn, write_group);
#if DDRX
								if (RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP > 0)
								{
									run_dm_margining(rank_bgn, write_group);
								}
#endif
#if QDRII
								run_dm_margining(rank_bgn, write_group);
#endif
#if RLDRAMX
								if (is_write_group_enabled_for_dm(write_group))
								{
									run_dm_margining(rank_bgn, write_group);
								}
#endif
							}
						}
					}
#endif
#endif
				}

				if (group_failed != 0)
				{
					failing_groups++;
				}

#if ENABLE_NON_DESTRUCTIVE_CALIB
				if (gbl->phy_debug_mode_flags & PHY_DEBUG_ENABLE_NON_DESTRUCTIVE_CALIBRATION) {
				  // USER Refresh the memory
				  if (!mem_refresh_all_ranks(0)) {
					set_failing_group_stage(write_group, CAL_STAGE_REFRESH, CAL_SUBSTAGE_REFRESH);
					TCLRPT_SET(debug_cal_report->cal_status_per_group[curr_shadow_reg][write_group].error_stage, CAL_STAGE_REFRESH);
					return 0;
				  }
				}
#endif

#if ENABLE_NON_DESTRUCTIVE_CALIB
				// USER Check if synchronous abort has been asserted
				if (abort_cal) {
					set_failing_group_stage(write_group, CAL_STAGE_CAL_ABORTED, CAL_SUBSTAGE_NIL);
					return 0;
				}
#endif
			}

			// USER If there are any failing groups then report the failure
			if (failing_groups != 0)
			{
				return 0;
			}

			//USER Calibrate the LFIFO 
			if (!((STATIC_CALIB_STEPS) & CALIB_SKIP_LFIFO)) {
				//USER If we're skipping groups as part of debug, don't calibrate LFIFO
				if (param->skip_groups == 0)
				{
					if (!rw_mgr_mem_calibrate_lfifo ()) {
						return 0;
					}
				}
			}
		}
	}

	TCLRPT_SET(debug_summary_report->cal_write_latency, IORD_32DIRECT (MEM_T_WL_ADD, 0));
	if (QUARTER_RATE == 1) {
		// The read latency is in terms of AFI cycles so we multiply by 4 in quarter
		// rate to get the memory cycles.
		TCLRPT_SET(debug_summary_report->cal_read_latency, gbl->curr_read_lat * 4);
	}
	else if (HALF_RATE == 1) {
		// The read latency is in terms of AFI cycles so we multiply by 2 in half
		// rate to get the memory cycles.
		TCLRPT_SET(debug_summary_report->cal_read_latency, gbl->curr_read_lat * 2);
	}
	else {
		TCLRPT_SET(debug_summary_report->cal_read_latency, gbl->curr_read_lat);
	}


	//USER Do not remove this line as it makes sure all of our decisions have been applied
	IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
	return 1;
}
#if ENABLE_NON_DES_CAL
alt_u32 run_mem_calibrate(alt_u32 non_des_mode) {

#else
alt_u32 run_mem_calibrate(void) {
#endif

	alt_u32 pass;
	alt_u32 debug_info;

	// Initialize the debug status to show that calibration has started.
	// This should occur before anything else
#if ENABLE_TCL_DEBUG
	tclrpt_initialize_debug_status();

	// Set that calibration has started
	debug_data->status |= 1 << DEBUG_STATUS_CALIBRATION_STARTED;
#endif
   // Reset pass/fail status shown on afi_cal_success/fail
   IOWR_32DIRECT (PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_RESET);

   TRACE_FUNC();

	BFM_STAGE("calibrate");
#if USE_DQS_TRACKING
#if HHP_HPS
	//stop tracking manger
	alt_u32 ctrlcfg = IORD_32DIRECT(CTRL_CONFIG_REG,0);

	IOWR_32DIRECT(CTRL_CONFIG_REG, 0, ctrlcfg & 0xFFBFFFFF); 
#else
	// we need to stall tracking
	IOWR_32DIRECT (TRK_STALL, 0, TRK_STALL_REQ_VAL);
	// busy wait for tracking manager to ack stall request
	while (IORD_32DIRECT (TRK_STALL, 0) != TRK_STALL_ACKED_VAL) {
	}
#endif
#endif

    initialize();
	
#if ENABLE_NON_DESTRUCTIVE_CALIB
	if (gbl->phy_debug_mode_flags & PHY_DEBUG_ENABLE_NON_DESTRUCTIVE_CALIBRATION) {
		if (no_init) {
			rw_mgr_mem_initialize_no_init();
			// refresh is done as part of rw_mgr_mem_initialize_no_init()
		} else {
			rw_mgr_mem_initialize ();
			mem_refresh_all_ranks(1);
		}
	} else {
		rw_mgr_mem_initialize ();
	}
#else
#if ENABLE_NON_DES_CAL
	if (non_des_mode)
		rw_mgr_mem_initialize_no_init();	
	else
		rw_mgr_mem_initialize ();

#else
	rw_mgr_mem_initialize ();
#endif	
#endif


#if ENABLE_BRINGUP_DEBUGGING
	do_bringup_test();
#endif

	pass = mem_calibrate ();

#if ENABLE_NON_DESTRUCTIVE_CALIB
	if( (gbl->phy_debug_mode_flags & PHY_DEBUG_ENABLE_NON_DESTRUCTIVE_CALIBRATION) ) {
	  if (!mem_refresh_all_ranks(0)) {
		set_failing_group_stage(RW_MGR_MEM_IF_WRITE_DQS_WIDTH, CAL_STAGE_REFRESH, CAL_SUBSTAGE_REFRESH);
		pass = 0;
	  } 
	} else {
	  mem_precharge_and_activate ();
	}
#else
	mem_precharge_and_activate ();
#endif
	
	//pe_checkout_pattern();

	IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);

	if (pass) {
		TCLRPT_SET(debug_summary_report->error_stage, CAL_STAGE_NIL);
		
		
		BFM_STAGE("handoff");

#ifdef TEST_SIZE
		if (!check_test_mem(0)) {
			gbl->error_stage = 0x92;
			gbl->error_group = 0x92;
		}
#endif
	}

#if TRACKING_ERROR_TEST
	if (IORD_32DIRECT(REG_FILE_TRK_SAMPLE_CHECK, 0) == 0xFE) {
		poll_for_sample_check();
	}
#endif

	//USER Handoff 
#if ENABLE_NON_DES_CAL

	if (non_des_mode)
	{
		alt_u32 took_too_long = 0;
		IOWR_32DIRECT (RW_MGR_ENABLE_REFRESH, 0, 0);  // Disable refresh engine  	
		took_too_long = IORD_32DIRECT (RW_MGR_ENABLE_REFRESH, 0);  

		if (took_too_long != 0)
		{
			pass = 0;  // force a failure  	 
			set_failing_group_stage(RW_MGR_MEM_IF_WRITE_DQS_WIDTH, CAL_STAGE_REFRESH, CAL_SUBSTAGE_REFRESH);
		}
	}
#endif	


	//USER Don't return control of the PHY back to AFI when in debug mode
	if ((gbl->phy_debug_mode_flags & PHY_DEBUG_IN_DEBUG_MODE) == 0) {
		rw_mgr_mem_handoff ();

#if HARD_PHY
		// In Hard PHY this is a 2-bit control:
		// 0: AFI Mux Select
		// 1: DDIO Mux Select
		IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 0x2);
#else
		IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 0);
#endif
	}
#if USE_DQS_TRACKING
#if HHP_HPS
	IOWR_32DIRECT(CTRL_CONFIG_REG, 0, ctrlcfg); 
#else
	// clear tracking stall flags
	IOWR_32DIRECT (TRK_STALL, 0, 0);
#endif	
#endif

#if FAKE_CAL_FAIL
   if (0) {
#else
	if (pass) {
#endif
		IPRINT("CALIBRATION PASSED");
		
		gbl->fom_in /= 2;
		gbl->fom_out /= 2;

		if (gbl->fom_in > 0xff) {
			gbl->fom_in = 0xff;
		}

		if (gbl->fom_out > 0xff) {
			gbl->fom_out = 0xff;
		}

#if BFM_MODE
		// duplicated because we want it after updating gbl, but before phy
		// is informed that calibration has completed
		print_gbl();
		fini_outfile();
#endif

		// Update the FOM in the register file
		debug_info = gbl->fom_in;
		debug_info |= gbl->fom_out << 8;
		IOWR_32DIRECT (REG_FILE_FOM, 0, debug_info);

		IOWR_32DIRECT (PHY_MGR_CAL_DEBUG_INFO, 0, debug_info);
		IOWR_32DIRECT (PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_SUCCESS);

	} else {
		
		IPRINT("CALIBRATION FAILED");
		
		debug_info = gbl->error_stage;
		debug_info |= gbl->error_substage << 8;
		debug_info |= gbl->error_group << 16;

#if BFM_MODE
		// duplicated because we want it after updating gbl, but before phy
		// is informed that calibration has completed
		print_gbl();
		fini_outfile();
#endif

		IOWR_32DIRECT (REG_FILE_FAILING_STAGE, 0, debug_info);
		IOWR_32DIRECT (PHY_MGR_CAL_DEBUG_INFO, 0, debug_info);
		IOWR_32DIRECT (PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_FAIL);

		// Update the failing group/stage in the register file
		debug_info = gbl->error_stage;
		debug_info |= gbl->error_substage << 8;
		debug_info |= gbl->error_group << 16;
		IOWR_32DIRECT (REG_FILE_FAILING_STAGE, 0, debug_info);

	}

#if RUNTIME_CAL_REPORT
	print_report(pass);
#endif


	// Mark the reports as being ready to read
	TCLRPT_SET(debug_summary_report->report_flags, debug_summary_report->report_flags |= DEBUG_REPORT_STATUS_REPORT_READY);
	TCLRPT_SET(debug_cal_report->report_flags, debug_cal_report->report_flags |= DEBUG_REPORT_STATUS_REPORT_READY);
	TCLRPT_SET(debug_margin_report->report_flags, debug_margin_report->report_flags |= DEBUG_REPORT_STATUS_REPORT_READY);

	// Set the debug status to show that calibration has ended.
	// This should occur after everything else
#if ENABLE_TCL_DEBUG
		debug_data->status |= 1 << DEBUG_STATUS_CALIBRATION_ENDED;
#endif
	return pass;

}

#if HCX_COMPAT_MODE || ENABLE_INST_ROM_WRITE
void hc_initialize_rom_data(void)
{
	alt_u32 i;

	for(i = 0; i < inst_rom_init_size; i++)
	{
		alt_u32 data = inst_rom_init[i];
		IOWR_32DIRECT (RW_MGR_INST_ROM_WRITE, (i << 2), data);
	}

	for(i = 0; i < ac_rom_init_size; i++)
	{
		alt_u32 data = ac_rom_init[i];
		IOWR_32DIRECT (RW_MGR_AC_ROM_WRITE, (i << 2), data);
	}
}
#endif

#if BFM_MODE
void init_outfile(void)
{
	const char *filename = getenv("SEQ_OUT_FILE");

	if (filename == NULL) {
		filename = "sequencer.out";
	}

	if ((bfm_gbl.outfp = fopen(filename, "w")) == NULL) {
		printf("ERROR: Failed to open %s for writing; using stdout\n", filename);
		bfm_gbl.outfp = stdout;
	}

	fprintf(bfm_gbl.outfp, "%s%s %s ranks=%lu cs/dimm=%lu dq/dqs=%lu,%lu vg/dqs=%lu,%lu dqs=%lu,%lu dq=%lu dm=%lu "
		"ptap_delay=%lu dtap_delay=%lu dtap_dqsen_delay=%lu dll=%lu\n",
		RDIMM ? "r" : (LRDIMM ? "l" : ""),
		DDR2 ? "DDR2" : (DDR3 ? "DDR3" : (QDRII ? "QDRII" : (RLDRAMII ? "RLDRAMII" : (RLDRAM3 ? "RLDRAM3" : "??PROTO??")))),
		FULL_RATE ? "FR" : (HALF_RATE ? "HR" : (QUARTER_RATE ? "QR" : "??RATE??")),
		RW_MGR_MEM_NUMBER_OF_RANKS,
		RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM,
		RW_MGR_MEM_DQ_PER_READ_DQS,
		RW_MGR_MEM_DQ_PER_WRITE_DQS,
		RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS,
		RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS,
		RW_MGR_MEM_IF_READ_DQS_WIDTH,
		RW_MGR_MEM_IF_WRITE_DQS_WIDTH,
		RW_MGR_MEM_DATA_WIDTH,
		RW_MGR_MEM_DATA_MASK_WIDTH,
		IO_DELAY_PER_OPA_TAP,
		IO_DELAY_PER_DCHAIN_TAP,
		IO_DELAY_PER_DQS_EN_DCHAIN_TAP,
		IO_DLL_CHAIN_LENGTH);
	fprintf(bfm_gbl.outfp, "max values: en_p=%lu dqdqs_p=%lu en_d=%lu dqs_in_d=%lu io_in_d=%lu io_out1_d=%lu io_out2_d=%lu "
		"dqs_in_reserve=%lu dqs_out_reserve=%lu\n",
		IO_DQS_EN_PHASE_MAX,
		IO_DQDQS_OUT_PHASE_MAX,
		IO_DQS_EN_DELAY_MAX,
		IO_DQS_IN_DELAY_MAX,
		IO_IO_IN_DELAY_MAX,
		IO_IO_OUT1_DELAY_MAX,
		IO_IO_OUT2_DELAY_MAX,
		IO_DQS_IN_RESERVE,
		IO_DQS_OUT_RESERVE);
	fprintf(bfm_gbl.outfp, "\n");
	// repeat these in a format that can be easily parsed
	fprintf(bfm_gbl.outfp, "ptap_delay: %lu\n", IO_DELAY_PER_OPA_TAP);
	fprintf(bfm_gbl.outfp, "dtap_delay: %lu\n", IO_DELAY_PER_DCHAIN_TAP);
	fprintf(bfm_gbl.outfp, "ptap_per_cycle: %lu\n", IO_DLL_CHAIN_LENGTH);
	fprintf(bfm_gbl.outfp, "ptap_max: %lu\n", IO_DQDQS_OUT_PHASE_MAX);
	fprintf(bfm_gbl.outfp, "dtap_max: %lu\n", IO_IO_OUT1_DELAY_MAX);
	fprintf(bfm_gbl.outfp, "vfifo_size: %lu\n", VFIFO_SIZE);
}

void fini_outfile(void)
{
	if (bfm_gbl.outfp != stdout && bfm_gbl.outfp != NULL) {
		// just flush, in case we calibrate again
		fflush(bfm_gbl.outfp);
	}
}

void print_u32_array(const char *label, alt_u32 *val, alt_u32 size)
{
	int i;

	fprintf(bfm_gbl.outfp, "%s", label);
	for (i = 0; i < size; i++) {
		fprintf(bfm_gbl.outfp, "%lu ", val[i]);
	}
	fprintf(bfm_gbl.outfp, "\n");
}

void print_s32_array(const char *label, alt_32 *val, alt_u32 size)
{
	int i;

	fprintf(bfm_gbl.outfp, "%s", label);
	for (i = 0; i < size; i++) {
		fprintf(bfm_gbl.outfp, "%ld ", val[i]);
	}
	fprintf(bfm_gbl.outfp, "\n");
}

void print_dqs_array(const char *label, alt_u32 *dqs)
{
	print_u32_array(label, dqs, MAX_DQS);
}

void print_read_dq_array(const char *label, alt_32 *dq)
{
	print_s32_array(label, dq, RW_MGR_MEM_IF_READ_DQS_WIDTH*RW_MGR_MEM_DQ_PER_READ_DQS);
}

void print_write_dq_array(const char *label, alt_32 *dq)
{
	print_s32_array(label, dq, RW_MGR_MEM_IF_WRITE_DQS_WIDTH*RW_MGR_MEM_DQ_PER_WRITE_DQS);
}

void print_dm_array(const char *label, alt_32 *dq)
{
	print_s32_array(label, dq, RW_MGR_MEM_IF_WRITE_DQS_WIDTH*RW_MGR_NUM_DM_PER_WRITE_GROUP);
}

void print_dqs_pos_array(const char *fmt, dqs_pos_t *dqs, int has_v, int has_ps)
{
	int i;

	if (has_v) {
		fprintf(bfm_gbl.outfp, fmt, "_v:  ");
		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			fprintf(bfm_gbl.outfp, "%lu ", dqs[i].v);
		}
		fprintf(bfm_gbl.outfp, "\n");
	}
	fprintf(bfm_gbl.outfp, fmt, "_p:  ");
	for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
		fprintf(bfm_gbl.outfp, "%lu ", dqs[i].p);
	}
	fprintf(bfm_gbl.outfp, "\n");
	fprintf(bfm_gbl.outfp, fmt, "_d:  ");
	for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
		fprintf(bfm_gbl.outfp, "%lu ", dqs[i].d);
	}
	fprintf(bfm_gbl.outfp, "\n");
	if (has_ps) {
		fprintf(bfm_gbl.outfp, fmt, "_ps: ");
		for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
			fprintf(bfm_gbl.outfp, "%lu ", dqs[i].ps);
		}
		fprintf(bfm_gbl.outfp, "\n");
	}
}

void print_gbl(void)
{
	int i;

	fprintf(bfm_gbl.outfp, "Globals\n");
	fprintf(bfm_gbl.outfp, "bfm_stage:     %s\n", BFM_GBL_GET(stage));
	// TODO: may want to do this per group, like other values
	print_dqs_pos_array(   "dqse_left%s  ", BFM_GBL_GET(dqs_enable_left_edge), 1, 1);
	print_dqs_pos_array(   "dqse_right%s ", BFM_GBL_GET(dqs_enable_right_edge), 1, 1);
	print_dqs_pos_array(   "dqse_mid%s   ", BFM_GBL_GET(dqs_enable_mid), 1, 1);
	print_dqs_pos_array(   "gwrite_pos%s ", BFM_GBL_GET(gwrite_pos), 0, 0);
	print_dqs_pos_array(   "dqswl_left%s ", BFM_GBL_GET(dqs_wlevel_left_edge), 0, 1);
	print_dqs_pos_array(   "dqswl_right%s", BFM_GBL_GET(dqs_wlevel_right_edge), 0, 1);
	print_dqs_pos_array(   "dqswl_mid%s  ", BFM_GBL_GET(dqs_wlevel_mid), 0, 1);
	print_read_dq_array(   "dq_read_l:     ", BFM_GBL_GET(dq_read_left_edge));
	print_read_dq_array(   "dq_read_r:     ", BFM_GBL_GET(dq_read_right_edge));
	print_write_dq_array(  "dq_write_l:    ", BFM_GBL_GET(dq_write_left_edge));
	print_write_dq_array(  "dq_write_r:    ", BFM_GBL_GET(dq_write_right_edge));
	print_dm_array(  "dm_l:          ", BFM_GBL_GET(dm_left_edge));
	print_dm_array(  "dm_r:          ", BFM_GBL_GET(dm_right_edge));
	
	fprintf(bfm_gbl.outfp, "curr_read_lat: %lu\n", gbl->curr_read_lat);
	fprintf(bfm_gbl.outfp, "error_stage:   %lu\n", gbl->error_stage);
	fprintf(bfm_gbl.outfp, "error_group:   %lu\n", gbl->error_group);
	fprintf(bfm_gbl.outfp, "fom_in:        %lu\n", gbl->fom_in);
	fprintf(bfm_gbl.outfp, "fom_out:       %lu\n", gbl->fom_out);
	fflush(bfm_gbl.outfp);
};

void bfm_set_globals_from_config()
{
	const char *filename = "board_delay_config.txt";
	const char *seq_c_prefix = "seq_c_";
	FILE *fp;
	char line[1024];
	char name[64];
	int value;

	if ((fp = fopen(filename, "r")) == NULL) {
		DPRINT(0, "Failed to open %s for reading; skipping config\n", filename);
		return;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (sscanf(line, "%s %ld", name, &value) != 2) {
			continue;
		}
		// for some unknown reason, sscanf of 'name' doesn't seem to work when linked into modelsim,
		// so we take a different approach for the name part, by just looking at the original line
		if (strncmp(line, seq_c_prefix, strlen(seq_c_prefix)) != 0) {
			// not a line targetted for us
			continue;
		}

		if (strncmp(line, "seq_c_skip_guaranteed_write", strlen("seq_c_skip_guaranteed_write")) == 0) {
			BFM_GBL_SET(bfm_skip_guaranteed_write,value);
			DPRINT(0, "bfm_skip_guaranteed_write => %ld", value);
		} else if (strncmp(line, "seq_c_trk_sample_count", strlen("seq_c_trk_sample_count")) == 0) {
			BFM_GBL_SET(trk_sample_count,value);
			DPRINT(0, "trk_sample_count => %ld", value);
		} else if (strncmp(line, "seq_c_trk_long_idle_updates", strlen("seq_c_trk_long_idle_updates")) == 0) {
			BFM_GBL_SET(trk_long_idle_updates,value);
			DPRINT(0, "trk_long_idle_updates => %ld", value);
		} else if (strncmp(line, "seq_c_lfifo_margin", strlen("seq_c_lfifo_margin")) == 0) {
			BFM_GBL_SET(lfifo_margin,value/AFI_RATE_RATIO);
			DPRINT(0, "lfifo_margin => %ld", value);
		} else {
			DPRINT(0, "Unknown Sequencer setting in line: %s\n", line);
		}
	}

	fclose(fp);
}
#endif

void initialize_reg_file(void)
{
	// Initialize the register file with the correct data
	IOWR_32DIRECT (REG_FILE_SIGNATURE, 0, REG_FILE_INIT_SEQ_SIGNATURE);
	IOWR_32DIRECT (REG_FILE_DEBUG_DATA_ADDR, 0, 0);
	IOWR_32DIRECT (REG_FILE_CUR_STAGE, 0, 0);
	IOWR_32DIRECT (REG_FILE_FOM, 0, 0);
	IOWR_32DIRECT (REG_FILE_FAILING_STAGE, 0, 0);
	IOWR_32DIRECT (REG_FILE_DEBUG1, 0, 0);
	IOWR_32DIRECT (REG_FILE_DEBUG2, 0, 0);
}

#if HPS_HW
void initialize_hps_phy(void)
{
	// These may need to be included also:
	// wrap_back_en (false)
	// atpg_en (false)
	// pipelineglobalenable (true) 
	
	alt_u32 reg;
	// Tracking also gets configured here because it's in the same register
	alt_u32 trk_sample_count = 7500;
	alt_u32 trk_long_idle_sample_count = (10 << 16) | 100; // Format is number of outer loops in the 16 MSB, sample count in 16 LSB.
	
	reg = 0;
#if DDR3 || DDR2
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ACDELAYEN_SET(2);
#else
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ACDELAYEN_SET(1);
#endif
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSLOGICDELAYEN_SET(1);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_RESETDELAYEN_SET(0);
#if LPDDR2
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_SET(0);
#else
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_SET(1);
#endif
	// Fix for long latency VFIFO
	// This field selects the intrinsic latency to RDATA_EN/FULL path. 00-bypass, 01- add 5 cycles, 10- add 10 cycles, 11- add 15 cycles.
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ADDLATSEL_SET(0);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_SET(trk_sample_count);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_OFFSET, reg);
	
	reg = 0;
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_SAMPLECOUNT_31_20_SET(trk_sample_count >> SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_WIDTH);
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_SET(trk_long_idle_sample_count);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_OFFSET, reg);
	
	reg = 0;
	reg |= SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_LONGIDLESAMPLECOUNT_31_20_SET(trk_long_idle_sample_count >> SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_WIDTH);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_OFFSET, reg);
}
#endif

#if USE_DQS_TRACKING

#if HHP_HPS
void initialize_tracking(void)
{
    alt_u32 concatenated_longidle = 0x0;
    alt_u32 concatenated_delays = 0x0;
    alt_u32 concatenated_rw_addr = 0x0;
    alt_u32 concatenated_refresh = 0x0;
    alt_u32 dtaps_per_ptap;
    alt_u32 tmp_delay;

    // compute usable version of value in case we skip full computation later
    dtaps_per_ptap = 0;
    tmp_delay = 0;
    while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
        dtaps_per_ptap++;
        tmp_delay += IO_DELAY_PER_DCHAIN_TAP;
    }
    dtaps_per_ptap--;
    
#if BFM_MODE
    concatenated_longidle = concatenated_longidle ^ (bfm_gbl.trk_long_idle_updates > 0 ? bfm_gbl.trk_long_idle_updates : 10); //longidle outer loop
    concatenated_longidle = concatenated_longidle << 16;
    concatenated_longidle = concatenated_longidle ^ (bfm_gbl.trk_sample_count > 0 ? bfm_gbl.trk_sample_count : 100); //longidle sample count
#else
    concatenated_longidle = concatenated_longidle ^ 10; //longidle outer loop
    concatenated_longidle = concatenated_longidle << 16;
    concatenated_longidle = concatenated_longidle ^ 100; //longidle sample count
#endif
    
    concatenated_delays = concatenated_delays ^ 243; // trfc, worst case of 933Mhz 4Gb
    concatenated_delays = concatenated_delays << 8;
    concatenated_delays = concatenated_delays ^ 14; // trcd, worst case
    concatenated_delays = concatenated_delays << 8;
    concatenated_delays = concatenated_delays ^ 10; // vfifo wait
    concatenated_delays = concatenated_delays << 8;
    concatenated_delays = concatenated_delays ^ 4; // mux delay

#if DDR3 || LPDDR2
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_IDLE;
    concatenated_rw_addr = concatenated_rw_addr << 8;
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_ACTIVATE_1;
    concatenated_rw_addr = concatenated_rw_addr << 8;
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_SGLE_READ;
    concatenated_rw_addr = concatenated_rw_addr << 8;
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_PRECHARGE_ALL;
#endif

#if DDR3 || LPDDR2
    concatenated_refresh = concatenated_refresh ^ __RW_MGR_REFRESH_ALL;
#else
    concatenated_refresh = concatenated_refresh ^ 0;
#endif
    concatenated_refresh = concatenated_refresh << 24;
    concatenated_refresh = concatenated_refresh ^ 1000; // trefi
    
	// Initialize the register file with the correct data
	IOWR_32DIRECT (REG_FILE_DTAPS_PER_PTAP, 0, dtaps_per_ptap);
#if BFM_MODE
	IOWR_32DIRECT (REG_FILE_TRK_SAMPLE_COUNT, 0, bfm_gbl.trk_sample_count > 0 ? bfm_gbl.trk_sample_count : 7500);
#else
	IOWR_32DIRECT (REG_FILE_TRK_SAMPLE_COUNT, 0, 7500);
#endif
	IOWR_32DIRECT (REG_FILE_TRK_LONGIDLE, 0, concatenated_longidle);
	IOWR_32DIRECT (REG_FILE_DELAYS, 0, concatenated_delays);
	IOWR_32DIRECT (REG_FILE_TRK_RW_MGR_ADDR, 0, concatenated_rw_addr);
	IOWR_32DIRECT (REG_FILE_TRK_READ_DQS_WIDTH, 0, RW_MGR_MEM_IF_READ_DQS_WIDTH);
	IOWR_32DIRECT (REG_FILE_TRK_RFSH, 0, concatenated_refresh);
}

#else

void initialize_tracking(void)
{
    alt_u32 concatenated_longidle = 0x0;
    alt_u32 concatenated_delays = 0x0;
    alt_u32 concatenated_rw_addr = 0x0;
    alt_u32 concatenated_refresh = 0x0;
    alt_u32 dtaps_per_ptap;
    alt_u32 tmp_delay;

    // compute usable version of value in case we skip full computation later
    dtaps_per_ptap = 0;
    tmp_delay = 0;
    while (tmp_delay < IO_DELAY_PER_OPA_TAP) {
        dtaps_per_ptap++;
        tmp_delay += IO_DELAY_PER_DCHAIN_TAP;
    }
    dtaps_per_ptap--;
     
#if BFM_MODE
    concatenated_longidle = concatenated_longidle ^ (bfm_gbl.trk_long_idle_updates > 0 ? bfm_gbl.trk_long_idle_updates : 10); //longidle outer loop
    concatenated_longidle = concatenated_longidle << 16;
    concatenated_longidle = concatenated_longidle ^ (bfm_gbl.trk_sample_count > 0 ? bfm_gbl.trk_sample_count : 100); //longidle sample count
#else
    concatenated_longidle = concatenated_longidle ^ 10; //longidle outer loop
    concatenated_longidle = concatenated_longidle << 16;
    concatenated_longidle = concatenated_longidle ^ 100; //longidle sample count
#endif
    
#if FULL_RATE
    concatenated_delays = concatenated_delays ^ 60; // trfc
#endif
#if HALF_RATE
    concatenated_delays = concatenated_delays ^ 30; // trfc
#endif
#if QUARTER_RATE
    concatenated_delays = concatenated_delays ^ 15; // trfc
#endif
    concatenated_delays = concatenated_delays << 8;
#if FULL_RATE
    concatenated_delays = concatenated_delays ^ 4; // trcd
#endif
#if HALF_RATE
    concatenated_delays = concatenated_delays ^ 2; // trcd
#endif
#if QUARTER_RATE
    concatenated_delays = concatenated_delays ^ 0; // trcd
#endif
    concatenated_delays = concatenated_delays << 8;
#if FULL_RATE
    concatenated_delays = concatenated_delays ^ 5; // vfifo wait
#endif
#if HALF_RATE
    concatenated_delays = concatenated_delays ^ 3; // vfifo wait
#endif
#if QUARTER_RATE
    concatenated_delays = concatenated_delays ^ 1; // vfifo wait
#endif
    concatenated_delays = concatenated_delays << 8;
#if FULL_RATE
    concatenated_delays = concatenated_delays ^ 4; // mux delay
#endif
#if HALF_RATE
    concatenated_delays = concatenated_delays ^ 2; // mux delay
#endif
#if QUARTER_RATE
    concatenated_delays = concatenated_delays ^ 0; // mux delay
#endif

#if DDR3 || LPDDR2
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_IDLE;
    concatenated_rw_addr = concatenated_rw_addr << 8;
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_ACTIVATE_1;
    concatenated_rw_addr = concatenated_rw_addr << 8;
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_SGLE_READ;
    concatenated_rw_addr = concatenated_rw_addr << 8;
    concatenated_rw_addr = concatenated_rw_addr ^ __RW_MGR_PRECHARGE_ALL;
#endif

#if DDR3 || LPDDR2
    concatenated_refresh = concatenated_refresh ^ __RW_MGR_REFRESH_ALL;
#else
    concatenated_refresh = concatenated_refresh ^ 0;
#endif
    concatenated_refresh = concatenated_refresh << 24;
    concatenated_refresh = concatenated_refresh ^ 546; // trefi
    
	IOWR_32DIRECT (TRK_DTAPS_PER_PTAP, 0, dtaps_per_ptap);
#if BFM_MODE
	IOWR_32DIRECT (TRK_SAMPLE_COUNT, 0, bfm_gbl.trk_sample_count > 0 ? bfm_gbl.trk_sample_count : 7500);
#else
	IOWR_32DIRECT (TRK_SAMPLE_COUNT, 0, 7500);
#endif
	IOWR_32DIRECT (TRK_LONGIDLE, 0, concatenated_longidle);
	IOWR_32DIRECT (TRK_DELAYS, 0, concatenated_delays);
	IOWR_32DIRECT (TRK_RW_MGR_ADDR, 0, concatenated_rw_addr);
	IOWR_32DIRECT (TRK_READ_DQS_WIDTH, 0, RW_MGR_MEM_IF_READ_DQS_WIDTH);
	IOWR_32DIRECT (TRK_RFSH, 0, concatenated_refresh);
}
#endif	/* HHP_HPS */
#endif	/* USE_DQS_TRACKING */

#if HHP_HPS_SIMULATION
void initialize_hps_controller(void)
{
	alt_u32 reg;
	alt_u32 memtype;
	alt_u32 ecc;
	alt_u32 ctrl_width;
	alt_u32 mem_bl;

	if (DDR2) {
		memtype = 1;
	} else if (DDR3) {
		memtype = 2;
	} else if (LPDDR1) {
		memtype = 3;
	} else if (LPDDR2) {
		memtype = 4;
	} else {
		// should never happen
		memtype = 0;
	}

	if (RW_MGR_MEM_DATA_WIDTH == 24 || RW_MGR_MEM_DATA_WIDTH == 40) {
		// we have ecc
		ecc = 1;
	} else {
		ecc = 0;
	}

	reg = 0;
	reg |= SDR_CTRLGRP_CTRLCFG_MEMTYPE_SET(memtype);
	reg |= SDR_CTRLGRP_CTRLCFG_MEMBL_SET(MEM_BURST_LENGTH);
	reg |= SDR_CTRLGRP_CTRLCFG_ADDRORDER_SET(ADDR_ORDER);
	reg |= SDR_CTRLGRP_CTRLCFG_ECCEN_SET(ecc);
	reg |= SDR_CTRLGRP_CTRLCFG_ECCCORREN_SET(0);
	reg |= SDR_CTRLGRP_CTRLCFG_CFG_ENABLE_ECC_CODE_OVERWRITES_SET(0);
	reg |= SDR_CTRLGRP_CTRLCFG_GENSBE_SET(0);
	reg |= SDR_CTRLGRP_CTRLCFG_GENDBE_SET(0);
	reg |= SDR_CTRLGRP_CTRLCFG_REORDEREN_SET(1);
	reg |= SDR_CTRLGRP_CTRLCFG_STARVELIMIT_SET(0x8);
	reg |= SDR_CTRLGRP_CTRLCFG_DQSTRKEN_SET(USE_DQS_TRACKING); // Do we want this?
#if DM_PINS_ENABLED
    reg |= SDR_CTRLGRP_CTRLCFG_NODMPINS_SET(0);
#else
    reg |= SDR_CTRLGRP_CTRLCFG_NODMPINS_SET(1);
#endif
	reg |= SDR_CTRLGRP_CTRLCFG_BURSTINTREN_SET(0);
	reg |= SDR_CTRLGRP_CTRLCFG_BURSTTERMEN_SET(0);
	reg |= SDR_CTRLGRP_CTRLCFG_OUTPUTREG_SET(0);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_CTRLCFG_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_DRAMTIMING1_TCWL_SET(MEM_WTCL_INT);
	reg |= SDR_CTRLGRP_DRAMTIMING1_TAL_SET(MEM_ATCL_INT);
	reg |= SDR_CTRLGRP_DRAMTIMING1_TCL_SET(MEM_TCL);
	reg |= SDR_CTRLGRP_DRAMTIMING1_TRRD_SET(MEM_TRRD);
	reg |= SDR_CTRLGRP_DRAMTIMING1_TFAW_SET(MEM_TFAW);
	reg |= SDR_CTRLGRP_DRAMTIMING1_TRFC_SET(MEM_TRFC);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMTIMING1_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_DRAMTIMING2_TREFI_SET(MEM_TREFI);
	reg |= SDR_CTRLGRP_DRAMTIMING2_TRCD_SET(MEM_TRCD);
	reg |= SDR_CTRLGRP_DRAMTIMING2_TRP_SET(MEM_TRP);
	reg |= SDR_CTRLGRP_DRAMTIMING2_TWTR_SET(MEM_TWTR);
	reg |= SDR_CTRLGRP_DRAMTIMING2_TWR_SET(MEM_TWR);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMTIMING2_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_DRAMTIMING3_TRTP_SET(MEM_TRTP);
	reg |= SDR_CTRLGRP_DRAMTIMING3_TRAS_SET(MEM_TRAS);
	reg |= SDR_CTRLGRP_DRAMTIMING3_TRC_SET(MEM_TRC);
	reg |= SDR_CTRLGRP_DRAMTIMING3_TMRD_SET(MEM_TMRD_CK);
	reg |= SDR_CTRLGRP_DRAMTIMING3_TCCD_SET(CFG_TCCD);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMTIMING3_OFFSET, reg);


	// These values don't really matter for the HPS simulation
	reg = 0;
	reg |= SDR_CTRLGRP_DRAMTIMING4_SELFRFSHEXIT_SET(512);
	reg |= SDR_CTRLGRP_DRAMTIMING4_PWRDOWNEXIT_SET(10);
	reg |= SDR_CTRLGRP_DRAMTIMING4_MINPWRSAVECYCLES_SET(0);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMTIMING4_OFFSET, reg);

	// These values don't really matter for the HPS simulation
	reg = 0;
	reg |= SDR_CTRLGRP_LOWPWRTIMING_AUTOPDCYCLES_SET(0);
	reg |= SDR_CTRLGRP_LOWPWRTIMING_CLKDISABLECYCLES_SET(0);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_LOWPWRTIMING_OFFSET, reg);


	// These values don't really matter for the HPS simulation
	reg = 0;
	reg |= SDR_CTRLGRP_DRAMODT_CFG_WRITE_ODT_CHIP_SET(0);
	reg |= SDR_CTRLGRP_DRAMODT_CFG_READ_ODT_CHIP_SET(0);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMODT_OFFSET, reg);


	reg = 0;
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_ACT_TO_RDWR_SET(INTG_EXTRA_CTL_CLK_ACT_TO_RDWR);
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_ACT_TO_PCH_SET(INTG_EXTRA_CTL_CLK_RD_TO_PCH);
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_ACT_TO_ACT_SET(INTG_EXTRA_CTL_CLK_ACT_TO_ACT);
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_RD_TO_RD_SET(INTG_EXTRA_CTL_CLK_RD_TO_RD);
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_RD_TO_RD_DIFF_CHIP_SET(INTG_EXTRA_CTL_CLK_RD_TO_RD_DIFF_CHIP);
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_RD_TO_WR_SET(INTG_EXTRA_CTL_CLK_RD_TO_WR);
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_RD_TO_WR_BC_SET(INTG_EXTRA_CTL_CLK_RD_TO_WR_BC);
	reg |= SDR_CTRLGRP_EXTRATIME1_CFG_EXTRA_CTL_CLK_RD_TO_WR_DIFF_CHIP_SET(INTG_EXTRA_CTL_CLK_RD_TO_WR_DIFF_CHIP);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_EXTRATIME1_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_EXTRATIME2_CFG_EXTRA_CTL_CLK_RD_TO_PCH_SET(INTG_EXTRA_CTL_CLK_RD_TO_PCH);
	reg |= SDR_CTRLGRP_EXTRATIME2_CFG_EXTRA_CTL_CLK_RD_AP_TO_VALID_SET(INTG_EXTRA_CTL_CLK_RD_AP_TO_VALID);
	reg |= SDR_CTRLGRP_EXTRATIME2_CFG_EXTRA_CTL_CLK_WR_TO_WR_SET(INTG_EXTRA_CTL_CLK_WR_TO_WR);
	reg |= SDR_CTRLGRP_EXTRATIME2_CFG_EXTRA_CTL_CLK_WR_TO_WR_DIFF_CHIP_SET(INTG_EXTRA_CTL_CLK_WR_TO_WR_DIFF_CHIP);
	reg |= SDR_CTRLGRP_EXTRATIME2_CFG_EXTRA_CTL_CLK_WR_TO_RD_SET(INTG_EXTRA_CTL_CLK_WR_TO_RD);
	reg |= SDR_CTRLGRP_EXTRATIME2_CFG_EXTRA_CTL_CLK_WR_TO_RD_BC_SET(INTG_EXTRA_CTL_CLK_WR_TO_RD_BC);
	reg |= SDR_CTRLGRP_EXTRATIME2_CFG_EXTRA_CTL_CLK_WR_TO_RD_DIFF_CHIP_SET(INTG_EXTRA_CTL_CLK_WR_TO_RD_DIFF_CHIP);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_EXTRATIME2_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_EXTRATIME3_CFG_EXTRA_CTL_CLK_WR_TO_PCH_SET(INTG_EXTRA_CTL_CLK_WR_TO_PCH);
	reg |= SDR_CTRLGRP_EXTRATIME3_CFG_EXTRA_CTL_CLK_WR_AP_TO_VALID_SET(INTG_EXTRA_CTL_CLK_WR_AP_TO_VALID);
	reg |= SDR_CTRLGRP_EXTRATIME3_CFG_EXTRA_CTL_CLK_PCH_TO_VALID_SET(INTG_EXTRA_CTL_CLK_PCH_TO_VALID);
	reg |= SDR_CTRLGRP_EXTRATIME3_CFG_EXTRA_CTL_CLK_PCH_ALL_TO_VALID_SET(INTG_EXTRA_CTL_CLK_PCH_ALL_TO_VALID);
	reg |= SDR_CTRLGRP_EXTRATIME3_CFG_EXTRA_CTL_CLK_ACT_TO_ACT_DIFF_BANK_SET(INTG_EXTRA_CTL_CLK_ACT_TO_ACT_DIFF_BANK);
	reg |= SDR_CTRLGRP_EXTRATIME3_CFG_EXTRA_CTL_CLK_FOUR_ACT_TO_ACT_SET(INTG_EXTRA_CTL_CLK_FOUR_ACT_TO_ACT);
	reg |= SDR_CTRLGRP_EXTRATIME3_CFG_EXTRA_CTL_CLK_ARF_TO_VALID_SET(INTG_EXTRA_CTL_CLK_ARF_TO_VALID);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_EXTRATIME3_OFFSET, reg);


	reg = 0;
	reg |= SDR_CTRLGRP_EXTRATIME4_CFG_EXTRA_CTL_CLK_PDN_TO_VALID_SET(INTG_EXTRA_CTL_CLK_PDN_TO_VALID);
	reg |= SDR_CTRLGRP_EXTRATIME4_CFG_EXTRA_CTL_CLK_SRF_TO_VALID_SET(INTG_EXTRA_CTL_CLK_SRF_TO_VALID);
	reg |= SDR_CTRLGRP_EXTRATIME4_CFG_EXTRA_CTL_CLK_SRF_TO_ZQ_CAL_SET(INTG_EXTRA_CTL_CLK_SRF_TO_ZQ_CAL);
	reg |= SDR_CTRLGRP_EXTRATIME4_CFG_EXTRA_CTL_CLK_ARF_PERIOD_SET(INTG_EXTRA_CTL_CLK_ARF_PERIOD);
	reg |= SDR_CTRLGRP_EXTRATIME4_CFG_EXTRA_CTL_CLK_PDN_PERIOD_SET(INTG_EXTRA_CTL_CLK_PDN_PERIOD);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_EXTRATIME4_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_DRAMADDRW_COLBITS_SET(MEM_IF_COL_ADDR_WIDTH);
	reg |= SDR_CTRLGRP_DRAMADDRW_ROWBITS_SET(MEM_IF_ROW_ADDR_WIDTH);
	reg |= SDR_CTRLGRP_DRAMADDRW_BANKBITS_SET(MEM_IF_BANKADDR_WIDTH);
	reg |= SDR_CTRLGRP_DRAMADDRW_CSBITS_SET(MEM_IF_CS_WIDTH > 1 ? MEM_IF_CHIP_BITS : 0);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMADDRW_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_DRAMIFWIDTH_IFWIDTH_SET(RW_MGR_MEM_DATA_WIDTH);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMIFWIDTH_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_DRAMDEVWIDTH_DEVWIDTH_SET(RW_MGR_MEM_DQ_PER_READ_DQS); // should always be 8
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_DRAMDEVWIDTH_OFFSET, reg);

	switch (RW_MGR_MEM_DATA_WIDTH) {
	case 8: ctrl_width = 0; break;
	case 16: // FALLTHROUGH
	case 24: ctrl_width = 1; break;
	case 32: // FALLTHROUGH
	case 40: ctrl_width = 2; break;
	default: ctrl_width = 0; break; /* shouldn't happen */
	}

	reg = 0;
	reg |= SDR_CTRLGRP_CTRLWIDTH_CTRLWIDTH_SET(ctrl_width);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_CTRLWIDTH_OFFSET, reg);

	// hard-coded values taken from test bench
	reg = 0;
	// 30'b111111111111010001000010001000
	reg |= SDR_CTRLGRP_MPPRIORITY_USERPRIORITY_SET(0x3FFD1088);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_MPPRIORITY_OFFSET, reg);

	// hard-coded values taken from test bench
	reg = 0;
	// first 32 bits of 50'b01111011111000010000100001000010000100001000010000
	reg |= SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_0_STATICWEIGHT_31_0_SET(0x21084210);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_0_OFFSET, reg);

	// hard-coded values taken from test bench
	reg = 0;
	// first next 18 bits of 50'b01111011111000010000100001000010000100001000010000
	reg |= SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_STATICWEIGHT_49_32_SET(0x1EF84);
	// first 14 of 64'b0011111000000000000000000000000000000000001000000010000000100000
	reg |= SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_SUMOFWEIGHTS_13_0_SET(0x2002);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_OFFSET, reg);

	// hard-coded values taken from test bench
	reg = 0;
	// next 32 bits of 64'b0011111000000000000000000000000000000000001000000010000000100000
	reg |= SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_2_SUMOFWEIGHTS_45_14_SET(0x80);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_2_OFFSET, reg);

	// hard-coded values taken from test bench
	reg = 0;
	// next 18 bits of 64'b0011111000000000000000000000000000000000001000000010000000100000
	reg |= SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_3_SUMOFWEIGHTS_63_46_SET(0xF800);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_3_OFFSET, reg);

	switch (MEM_BURST_LENGTH) {
	case 2: mem_bl = 0; break;
	case 4: mem_bl = 1; break;
	case 8: mem_bl = 2; break;
	default: mem_bl = 2; break; // should never happen
	}
	reg = 0;
	reg |= SDR_CTRLGRP_STATICCFG_MEMBL_SET(mem_bl);
	reg |= SDR_CTRLGRP_STATICCFG_USEECCASDATA_SET(0); /* allow fpga to access ecc bits; not supported */
	reg |= SDR_CTRLGRP_STATICCFG_APPLYCFG_SET(1);	  /* apply all of the configs here and above */
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_STATICCFG_OFFSET, reg);

	reg = 0;
	reg |= SDR_CTRLGRP_FPGAPORTRST_PORTRSTN_SET(~0);
	IOWR_32DIRECT (BASE_MMR, SDR_CTRLGRP_FPGAPORTRST_OFFSET, reg);
}
#endif
	
void user_init_cal_req(void)
{
	alt_u32 scc_afi_reg;
	    
	scc_afi_reg = IORD_32DIRECT (SCC_MGR_AFI_CAL_INIT, 0);
	
	if (scc_afi_reg == 1 || scc_afi_reg == 16) {// 1 is initialization request
	    initialize();
	    rw_mgr_mem_initialize ();
	    rw_mgr_mem_handoff ();
	    IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 0);
	    IOWR_32DIRECT (PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_SUCCESS);
	} else if (scc_afi_reg == 2 || scc_afi_reg == 32) {
#if ENABLE_NON_DES_CAL		
	    run_mem_calibrate (0);
#else
	    run_mem_calibrate();
#endif
	} 
#if ENABLE_NON_DES_CAL
	else if (scc_afi_reg == 4) {
		//non destructive mem init
		IOWR_32DIRECT (SCC_MGR_AFI_CAL_INIT, 0, scc_afi_reg & ~(1 << 2));

	    	rw_mgr_mem_initialize_no_init();

		
	} else if (scc_afi_reg == 8) {
		//non destructive mem calibrate
		IOWR_32DIRECT (SCC_MGR_AFI_CAL_INIT, 0, scc_afi_reg & ~(1 << 3));

	    	run_mem_calibrate (1);
		IOWR_32DIRECT (RW_MGR_ENABLE_REFRESH, 0, 0);  // Disable refresh engine  
	}
#endif

}

#if TRACKING_WATCH_TEST || TRACKING_ERROR_TEST
void decrement_dqs_en_phase (alt_u32 group) {
	alt_u32 phase = 0;
	alt_u32 v;
	phase = READ_SCC_DQS_EN_PHASE(group);

	if (phase == 0) {
		rw_mgr_decr_vfifo(group, &v);
		scc_mgr_set_dqs_en_phase(group, IO_DQS_EN_PHASE_MAX);
		scc_mgr_set_dqs_en_delay(group, IO_DQS_EN_DELAY_MAX);
		return;
	}

	scc_mgr_set_dqs_en_phase(group, phase - 1);
	scc_mgr_set_dqs_en_delay(group, IO_DQS_EN_DELAY_MAX);
}

void read_samples (void)
{
	alt_u32 group = 0;
	alt_32 sample_count = 0;
	alt_u32 delay = 0;
	alt_u32 phase = 0;

	alt_u32 dtaps_per_ptap = 0;

	dtaps_per_ptap = IORD_32DIRECT(0xD0000, 0);
	TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][group].dtaps_per_ptap, dtaps_per_ptap);

#if TRACKING_WATCH_TEST	
	for (;;) {
		// Stall tracking to ensure accurate reading
		IOWR_32DIRECT (TRK_STALL, 0, TRK_STALL_REQ_VAL);
		// Wait for tracking manager to ack stall request
		while (IORD_32DIRECT (TRK_STALL, 0) != TRK_STALL_ACKED_VAL) {
		}
	
		for (group = 0; group < RW_MGR_MEM_IF_READ_DQS_WIDTH; group++) {
			// Read sample counter
			sample_count = IORD_32DIRECT(0x58F00, group << 2);
			TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][group].sample_count, sample_count);
			delay = READ_SCC_DQS_EN_DELAY(group);
			phase = READ_SCC_DQS_EN_PHASE(group);
			TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][group].dqs_en_phase, (delay | (phase << 16)));
		}

		// Release stall	
		IOWR_32DIRECT(TRK_STALL, 0, 0);
	}
#endif

#if TRACKING_ERROR_TEST
	for (group = 0; group < RW_MGR_MEM_IF_READ_DQS_WIDTH; group++) {
		// Read sample counter
		sample_count = IORD_32DIRECT(0x58F00, group << 2);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][group].sample_count, sample_count);
		delay = READ_SCC_DQS_EN_DELAY(group);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][group].dqs_en_delay, delay);
		phase = READ_SCC_DQS_EN_PHASE(group);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[curr_shadow_reg][group].dqs_en_phase, phase);
	}
#endif
}

void tracking_sample_check (void) 
{
	alt_u32 group = 0;
	alt_u32 t11_d = 0;
	alt_u32 read_val = 0;

	alt_u32 num_samples = 0;
	alt_u32 num_samples_max = 7500;

	alt_u32 bit_chk = 0;
	alt_u32 test_status = 0;

	for (group = 0; group < RW_MGR_MEM_IF_READ_DQS_WIDTH; group++)
	{
		// TODO: Figure out whether the sample counter and sample run
		// values should be defined somewhere, or just leave them
		// hardcoded.
		IOWR_32DIRECT(0x58F00, group << 2, 0x00);
	}

	for (num_samples = 0; num_samples < num_samples_max; num_samples++) {
		//do a read
		//test_status = rw_mgr_mem_calibrate_read_test_all_ranks (group, 1, PASS_ONE_BIT, &bit_chk, 0);
		//do a write
		test_status = rw_mgr_mem_calibrate_write_test_all_ranks (group, 0, PASS_ONE_BIT, &bit_chk);

		// do a sample
		IOWR_32DIRECT(0x58FFC, 0, 0xFF);
	}

	read_samples();
}

void poll_for_sample_check (void) 
{
	alt_u32 check_status = 2;
	alt_u32 delay = 0;
	alt_u32 group = 0;

	alt_u32 READY_FOR_READ = 0xFE;
	alt_u32 READ_FINISHED = 0xFD;
	alt_u32 EXIT_LOOP = 0x00;
	alt_u32 FINISHED_SIGNAL = 0xFF;
	
	for (;;) {
		check_status = IORD_32DIRECT(REG_FILE_TRK_SAMPLE_CHECK, 0);

		if (check_status == READY_FOR_READ) {
			for (group = 0; group < RW_MGR_MEM_IF_READ_DQS_WIDTH; group++) {

				delay = READ_SCC_DQS_EN_DELAY(group);

				if (delay == 0) {
					decrement_dqs_en_phase(group);
				} else {
					delay--;
					scc_mgr_set_dqs_en_delay(group, delay);
				}			
			
				IOWR_32DIRECT (SCC_MGR_DQS_ENA, 0, group);
			}
			
			IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);

			tracking_sample_check();
			check_status = IORD_32DIRECT(REG_FILE_TRK_SAMPLE_CHECK, 0);
			if (check_status != EXIT_LOOP) {
				IOWR_32DIRECT(REG_FILE_TRK_SAMPLE_CHECK, 0, READ_FINISHED);
			}
		} 
		if (check_status == EXIT_LOOP) {
			IOWR_32DIRECT(REG_FILE_TRK_SAMPLE_CHECK, 0, FINISHED_SIGNAL);
			break;
		}
	}
}
#endif // TRACKING_WATCH_TEST || TRACKING_ERROR_TEST

#if BFM_MODE
int seq_main(void)
#elif HPS_HW
int sdram_calibration(void)
#else
int main(void)
#endif
{
	param_t my_param;
	gbl_t my_gbl;
	alt_u32 pass;
	alt_u32 i;

	param = &my_param;
	gbl = &my_gbl;

	// Initialize the debug mode flags
	gbl->phy_debug_mode_flags = 0;
	// Set the calibration enabled by default
	gbl->phy_debug_mode_flags |= PHY_DEBUG_ENABLE_CAL_RPT;
	// Only enable margining by default if requested
#if ENABLE_MARGIN_REPORT_GEN
	gbl->phy_debug_mode_flags |= PHY_DEBUG_ENABLE_MARGIN_RPT;
#endif
	// Only sweep all groups (regardless of fail state) by default if requested 
#if ENABLE_SWEEP_ALL_GROUPS
	gbl->phy_debug_mode_flags |= PHY_DEBUG_SWEEP_ALL_GROUPS;
#endif
	//Set enabled read test by default
#if DISABLE_GUARANTEED_READ
	gbl->phy_debug_mode_flags |= PHY_DEBUG_DISABLE_GUARANTEED_READ;
#endif
#if ENABLE_NON_DESTRUCTIVE_CALIB
	gbl->phy_debug_mode_flags |= PHY_DEBUG_ENABLE_NON_DESTRUCTIVE_CALIBRATION;
#endif

#if BFM_MODE
	init_outfile();
	bfm_set_globals_from_config();
#endif

	// Initialize the register file
	initialize_reg_file();
	
#if HPS_HW
	// Initialize any PHY CSR
	initialize_hps_phy();
#endif

#if HHP_HPS
	scc_mgr_initialize();
#endif

#if USE_DQS_TRACKING
	initialize_tracking();
#endif

	// Initialize the TCL report. This must occur before any printf
	// but after the debug mode flags and register file
#if ENABLE_TCL_DEBUG
	tclrpt_initialize(&my_debug_data);
#endif

   // USER Enable all ranks, groups
   for (i = 0; i < RW_MGR_MEM_NUMBER_OF_RANKS; i++) {
		param->skip_ranks[i] = 0;
	}
	for (i = 0; i < NUM_SHADOW_REGS; ++i) {
		param->skip_shadow_regs[i] = 0;
	}
	param->skip_groups = 0;

	IPRINT("Preparing to start memory calibration");

	TRACE_FUNC();
	DPRINT(1, "%s%s %s ranks=%lu cs/dimm=%lu dq/dqs=%lu,%lu vg/dqs=%lu,%lu dqs=%lu,%lu dq=%lu dm=%lu "
	       "ptap_delay=%lu dtap_delay=%lu dtap_dqsen_delay=%lu, dll=%lu",
	       RDIMM ? "r" : (LRDIMM ? "l" : ""),
	       DDR2 ? "DDR2" : (DDR3 ? "DDR3" : (QDRII ? "QDRII" : (RLDRAMII ? "RLDRAMII" : (RLDRAM3 ? "RLDRAM3" : "??PROTO??")))),
	       FULL_RATE ? "FR" : (HALF_RATE ? "HR" : (QUARTER_RATE ? "QR" : "??RATE??")),
	       (long unsigned int)RW_MGR_MEM_NUMBER_OF_RANKS,
	       (long unsigned int)RW_MGR_MEM_NUMBER_OF_CS_PER_DIMM,
	       (long unsigned int)RW_MGR_MEM_DQ_PER_READ_DQS,
	       (long unsigned int)RW_MGR_MEM_DQ_PER_WRITE_DQS,
	       (long unsigned int)RW_MGR_MEM_VIRTUAL_GROUPS_PER_READ_DQS,
	       (long unsigned int)RW_MGR_MEM_VIRTUAL_GROUPS_PER_WRITE_DQS,
	       (long unsigned int)RW_MGR_MEM_IF_READ_DQS_WIDTH,
	       (long unsigned int)RW_MGR_MEM_IF_WRITE_DQS_WIDTH,
	       (long unsigned int)RW_MGR_MEM_DATA_WIDTH,
	       (long unsigned int)RW_MGR_MEM_DATA_MASK_WIDTH,
	       (long unsigned int)IO_DELAY_PER_OPA_TAP,
	       (long unsigned int)IO_DELAY_PER_DCHAIN_TAP,
	       (long unsigned int)IO_DELAY_PER_DQS_EN_DCHAIN_TAP,
	       (long unsigned int)IO_DLL_CHAIN_LENGTH);
	DPRINT(1, "max values: en_p=%lu dqdqs_p=%lu en_d=%lu dqs_in_d=%lu io_in_d=%lu io_out1_d=%lu io_out2_d=%lu"
	       "dqs_in_reserve=%lu dqs_out_reserve=%lu",
	       (long unsigned int)IO_DQS_EN_PHASE_MAX,
	       (long unsigned int)IO_DQDQS_OUT_PHASE_MAX,
	       (long unsigned int)IO_DQS_EN_DELAY_MAX,
	       (long unsigned int)IO_DQS_IN_DELAY_MAX,
	       (long unsigned int)IO_IO_IN_DELAY_MAX,
	       (long unsigned int)IO_IO_OUT1_DELAY_MAX,
	       (long unsigned int)IO_IO_OUT2_DELAY_MAX,
	       (long unsigned int)IO_DQS_IN_RESERVE,
	       (long unsigned int)IO_DQS_OUT_RESERVE);

#if HCX_COMPAT_MODE || ENABLE_INST_ROM_WRITE
	hc_initialize_rom_data();
#endif	

#if !HARD_PHY
	// Hard PHY does not support soft reset
	IOWR_32DIRECT (RW_MGR_SOFT_RESET, 0, 0);
#endif

	//USER update info for sims
	reg_file_set_stage(CAL_STAGE_NIL);
	reg_file_set_group(0);

	// Load global needed for those actions that require
	// some dynamic calibration support
#if HARD_PHY
	dyn_calib_steps = STATIC_CALIB_STEPS;
#else
	dyn_calib_steps = IORD_32DIRECT(PHY_MGR_CALIB_SKIP_STEPS, 0);
#endif

	// Load global to allow dynamic selection of delay loop settings
	// based on calibration mode
	if (!((DYNAMIC_CALIB_STEPS) & CALIB_SKIP_DELAY_LOOPS)) {
		skip_delay_mask = 0xff;
	} else {
		skip_delay_mask = 0x0;
	}


#ifdef TEST_SIZE
	if (!check_test_mem(1)) {
		IOWR_32DIRECT (PHY_MGR_CAL_DEBUG_INFO, 0, 0x9090);
		IOWR_32DIRECT (PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_FAIL);
	}
	write_test_mem();
	if (!check_test_mem(0)) {
		IOWR_32DIRECT (PHY_MGR_CAL_DEBUG_INFO, 0, 0x9191);
		IOWR_32DIRECT (PHY_MGR_CAL_STATUS, 0, PHY_MGR_CAL_FAIL);
	}
#endif


#if HHP_HPS_SIMULATION
	// configure controller
	initialize_hps_controller();
#endif

#if ENABLE_TCL_DEBUG && USE_USER_RDIMM_VALUE
	tclrpt_loop();
#endif


#if ENABLE_NON_DES_CAL_TEST
	rw_mgr_mem_initialize ();

//	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_IDLE);
//	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_SELF_REFRESH);

	pass = run_mem_calibrate (1);

#else

#if ENABLE_NON_DES_CAL
#if ENABLE_TCL_DEBUG
	tclrpt_loop();
#else
	pass = run_mem_calibrate (1); 
#endif
#else
	pass = run_mem_calibrate ();
#endif

#endif

#if TRACKING_WATCH_TEST
	if (IORD_32DIRECT(REG_FILE_TRK_SAMPLE_CHECK, 0) == 0xEE) {
		read_samples();
	}
#endif

#if ENABLE_PRINTF_LOG
	IPRINT("Calibration complete");
	// Send the end of transmission character
	IPRINT("%c", 0x4);
#endif




#if BFM_MODE
#if ENABLE_TCL_DEBUG
	tclrpt_dump_internal_data();
#endif
	bfm_sequencer_is_done();
#elif HHP_HPS_SIMULATION
	// nothing to do for HPS simulation following calibration
	while (1) {
	}
#elif ENABLE_TCL_DEBUG
  #if HPS_HW
	// EMPTY
  #else
	tclrpt_loop();
  #endif
#else
  #if HPS_HW
	// EMPTY
  #else
	while (1) {
	    user_init_cal_req();
	}

  #endif
#endif


return pass;
}


#if ENABLE_BRINGUP_DEBUGGING

///////////////////////////////////////////////////////////////////////////////////////
// Bring-Up test Support
///////////////////////////////////////////////////////////////////////////////////////


void do_bringup_test_guaranteed_write (void)
{
	alt_u32 r;

	TRACE_FUNC();
	
	for (r = 0; r < RW_MGR_MEM_NUMBER_OF_RANKS; r++) {
		if (param->skip_ranks[r]) {
			//USER request to skip the rank

			continue;
		}

		//USER set rank
		set_rank_and_odt_mask(r, RW_MGR_ODT_MODE_READ_WRITE);

		//USER Load up a constant bursts

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_GUARANTEED_WRITE_0_1_A_5_WAIT0);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_GUARANTEED_WRITE_0_1_A_5_WAIT1);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_2, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_2, 0, __RW_MGR_GUARANTEED_WRITE_0_1_A_5_WAIT2);

		IOWR_32DIRECT (RW_MGR_LOAD_CNTR_3, 0, 0x20);
		IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_3, 0, __RW_MGR_GUARANTEED_WRITE_0_1_A_5_WAIT3);

		IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, 0, __RW_MGR_GUARANTEED_WRITE_0_1_A_5);
	}

	set_rank_and_odt_mask(0, RW_MGR_ODT_MODE_OFF);
}

void do_bringup_test_clear_di_buf (alt_u32 group)
{
	IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);
	IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);

	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 128);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_DO_CLEAR_DI_BUF);
	
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, group << 2, __RW_MGR_DO_CLEAR_DI_BUF);
}

void do_bringup_test_guaranteed_read (alt_u32 group)
{
	IOWR_32DIRECT (PHY_MGR_CMD_FIFO_RESET, 0, 0);
	IOWR_32DIRECT (RW_MGR_RESET_READ_DATAPATH, 0, 0);

	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_0, 0, 16);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_0, 0, __RW_MGR_DO_TEST_READ);
	IOWR_32DIRECT (RW_MGR_LOAD_CNTR_1, 0, 16);
	IOWR_32DIRECT (RW_MGR_LOAD_JUMP_ADD_1, 0, __RW_MGR_DO_TEST_READ_POST_WAIT);
	
	IOWR_32DIRECT (RW_MGR_RUN_SINGLE_GROUP, group << 2, __RW_MGR_DO_TEST_READ);
}

void do_bringup_test ()
{
	int i;
	alt_u32 group;
	alt_u32 v = 0;

	group = 0;

	mem_config ();

	// 15 is the maximum latency (should make dependent on actual design
	IOWR_32DIRECT (PHY_MGR_PHY_RLAT, 0, 15); /* lfifo setting */

#if ARRIAV || CYCLONEV
	for (i = 0; i < RW_MGR_MEM_IF_READ_DQS_WIDTH; i++) {
		IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, i);
		scc_set_bypass_mode(i, 0);
	}
#endif

	// initialize global buffer to something known
	for (i = 0; i < sizeof(di_buf_gbl); i++) {
		di_buf_gbl[i] = 0xee;
	}

	// pre-increment vfifo to ensure not at max value
	rw_mgr_incr_vfifo(group, &v);
	rw_mgr_incr_vfifo(group, &v);
	
	do_bringup_test_clear_di_buf(group);

	while (1) {
		do_bringup_test_guaranteed_write();
		do_bringup_test_guaranteed_read(group);
		load_di_buf_gbl();
		rw_mgr_incr_vfifo(group, &v);
	}
}


#endif // ENABLE_BRINGUP_DEBUGGING

#if ENABLE_ASSERT
void err_report_internal_error
(
    const char* description,
    const char* module,
    const char* file,
    int line
)
{
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    fprintf(stderr, ERR_IE_TEXT, module, file, line, description, "\n");

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	fprintf (stderr, "Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
	{
		fprintf (stderr, "%s\n", strings[i]);
	}

	free (strings);
}
#endif
