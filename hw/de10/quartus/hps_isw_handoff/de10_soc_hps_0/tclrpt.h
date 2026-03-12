#ifndef TCLRPT_H_
#define TCLRPT_H_
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


#include "sequencer.h"

#if ENABLE_TCL_DEBUG
#define TCLRPT_SET(item, value) item = value
#else
#define TCLRPT_SET(item, value)
#endif

// None of the rest of the file should be referenced if ENABLE_TCL_DEBUG is not
// set (although it's not a problem if it is, but this helps catch errors)
#if ENABLE_TCL_DEBUG

#define PRINTF_READ_BUFFER_SIZE 128/4
#define PRINTF_READ_BUFFER_FIFO_WORDS 32

#define NUM_DI_SAMPLE 100

#define DI_REPORT_FLAGS_READY 0x00000001
#define DI_REPORT_FLAGS_DONE 0x00000002

//*****************************************************************************
// TCL Commands
//*****************************************************************************
// The wait command
#define TCLDBG_CMD_WAIT_CMD 1000

// No operation command
#define TCLDBG_CMD_NOP 0

// Command response acknowledged
#define TCLDBG_CMD_RESPONSE_ACK 1

// Run the full test
#define TCLDBG_RUN_FULLTEST 2

// Query the parameterization info
#define TCLDBG_PARAM_INFO 3

// Query the status of calibration
#define TCLDBG_CAL_STATUS 4

// Run memory calibration
#define TCLDBG_RUN_MEM_CALIBRATE 5

// Run pattern to generate eye diagrams
#define TCLDBG_RUN_EYE_DIAGRAM_PATTERN 6

// Run full test and see how far we can push DQ delay on input and output sides
#define TCLDBG_FIND_FULL_TEST_DQ 7

// Run full test and see how far we can push DQS delay on input and output sides
#define TCLDBG_FIND_FULL_TEST_DQS 8

// Run full test and see how far we can push DM delay on output sides
#define TCLDBG_FIND_FULL_TEST_DM 9

// Query the margins found during read and write calibration
#define TCLDBG_QUERY_CALIB_MARGINS 10

// Query the settings applied during calibration on DQ pins
#define TCLDBG_QUERY_DQ_SETTINGS 11

// Query the settings applied during calibration on DQS groups
#define TCLDBG_QUERY_DQS_SETTINGS 12

// Query the state of the PHY. User mode or debug mode
#define TCLDBG_QUERY_PHY_USER_DEBUG_MODE 13

// Mark all groups as being valid for calibration
#define TCLDBG_MARK_ALL_DQS_GROUPS_AS_VALID 14

// Mark a specific group to be skipped for calibration
#define TCLDBG_MARK_GROUP_AS_SKIP 15

// Query the DQS skip group mask
#define TCLDBG_QUERY_GROUP_AS_SKIP 16

// Mark all ranks as being valid for calibration
#define TCLDBG_MARK_ALL_RANKS_AS_VALID 17

// Mark a specific rank to be skipped for calibration
#define TCLDBG_MARK_RANK_AS_SKIP 18

// Query the rank skip mask
#define TCLDBG_QUERY_RANK_AS_SKIP 19

// Query the settings applied during calibration on DM pins
#define TCLDBG_QUERY_DM_SETTINGS 20

// Enable the margining report as part of calibration
#define TCLDBG_ENABLE_MARGIN_REPORT 21

// Enable sweeping all groups of calibration
#define TCLDBG_ENABLE_SWEEP_ALL_GROUPS 22

// Enable the guaranteed read test as part of calibration
#define TCLDBG_DISABLE_GUARANTEED_READ 23

// Enable/disable non-destructive calibration
#define TCLDBG_SET_NON_DESTRUCTIVE_CALIBRATION 24

#if ENABLE_DELAY_CHAIN_WRITE
// Set DQ D1 Delay (I/O buffer to input register)
#define TCLDBG_SET_DQ_D1_DELAY 25

// Set DQ D5 Delay (output register to I/O buffer)
#define TCLDBG_SET_DQ_D5_DELAY 26

// Set DQ D6 Delay (output register to I/O buffer)
#define TCLDBG_SET_DQ_D6_DELAY 27

// Set DQS D4 Delay (DQS delay chain)
#define TCLDBG_SET_DQS_D4_DELAY 28

// Set DQS DQ Output Phase (deg)
#define TCLDBG_SET_DQDQS_OUTPUT_PHASE 29

// Set DQS D5 Delay (output register to I/O buffer)
#define TCLDBG_SET_DQS_D5_DELAY 30

// Set DQS D6 Delay (output register to I/O buffer)
#define TCLDBG_SET_DQS_D6_DELAY 31

// Set DQS DQS Enable Phase (deg)
#define TCLDBG_SET_DQS_EN_PHASE 32

// Set DQS T11 Delay (DQS post-amble delay)
#define TCLDBG_SET_DQS_T11_DELAY 33

// Set DM D5 Delay (output register to I/O buffer)
#define TCLDBG_SET_DM_D5_DELAY 34

// Set DM D6 Delay (output register to I/O buffer)
#define TCLDBG_SET_DM_D6_DELAY 35

// Rerun DQ margining without calibrating
#define TCLDBG_REMARGIN_DQ 36

// Rerun DM margining without calibrating
#define TCLDBG_REMARGIN_DM 37

// Increment VFIFO
#define TCLDBG_INCR_VFIFO 38

// Decrement VFIFO
#define TCLDBG_DECR_VFIFO 39

// Select shadow register
#define TCLDBG_SELECT_SHADOW_REG 40

#endif // ENABLE_DELAY_CHAIN_WRITE

// Update RDIMM Control Word
#define TCLDBG_SET_UPDATE_PARAMETERS 41

// Run memory calibration
#define TCLDBG_RUN_NON_DES_MEM_CALIBRATE 42

//*****************************************************************************
// TCL RX Status Codes
//*****************************************************************************
// RX interface waiting for command
#define TCLDBG_RX_STATUS_WAIT_CMD 0

// RX interface command ready for operation.
#define TCLDBG_RX_STATUS_CMD_READY 1

// RX interface command executing
#define TCLDBG_RX_STATUS_CMD_EXE 2

//*****************************************************************************
// TCL TX Status Codes
//*****************************************************************************
// RX interface ready to accept commands in debug mode
#define TCLDBG_TX_STATUS_CMD_READY 0

// TX interface response not ready as command is running
#define TCLDBG_TX_STATUS_CMD_EXE 1

// RX interface illegal command
#define TCLDBG_TX_STATUS_ILLEGAL_CMD 2

// TX interface response ready
#define TCLDBG_TX_STATUS_RESPONSE_READY 3



//*****************************************************************************
// Main report status bits
//*****************************************************************************
#define DEBUG_STATUS_PRINTF_ENABLED_BIT 0
#define DEBUG_STATUS_CALIBRATION_STARTED 1
#define DEBUG_STATUS_CALIBRATION_ENDED 2

//*****************************************************************************
// Individual reports status bits
//*****************************************************************************
#define DEBUG_REPORT_STATUS_REPORT_READY 0x00000001
#define DEBUG_REPORT_STATUS_REPORT_GEN_ENABLED 0x00000002
#define DEBUG_REPORT_DTAP_PER_PTAP_DYNAMIC 0x00000004

//*****************************************************************************
// Debug report sizes
//*****************************************************************************
#define NUM_RANK_MASK_WORDS ((RW_MGR_MEM_NUMBER_OF_RANKS % 32) == 0 ? (RW_MGR_MEM_NUMBER_OF_RANKS/32) : (RW_MGR_MEM_NUMBER_OF_RANKS/32)+1)
#define NUM_GROUP_MASK_WORDS ((RW_MGR_MEM_IF_READ_DQS_WIDTH % 32) == 0 ? (RW_MGR_MEM_IF_READ_DQS_WIDTH/32) : (RW_MGR_MEM_IF_READ_DQS_WIDTH/32)+1)

#define COMMAND_PARAM_WORDS 4

//*****************************************************************************
// Debug report structs
// Margins are reported in terms of delay chain taps.
//*****************************************************************************
typedef struct debug_cal_observed_dq_margins_struct {
	alt_32 left_edge;
	alt_32 right_edge;
} debug_cal_observed_dq_margins_t;

typedef struct debug_cal_observed_dqs_in_margins_struct {
	alt_32 dq_margin;
	alt_32 dqs_margin;
	alt_u32 dqsen_margin;
} debug_cal_observed_dqs_in_margins_t;

typedef struct debug_cal_observed_dqsen_margins_struct {
	alt_u32 vfifo_begin;
	alt_u32 phase_begin;
	alt_u32 delay_begin;
	alt_u32 work_begin;
	alt_u32 vfifo_end;
	alt_u32 phase_end;
	alt_u32 delay_end;
	alt_u32 work_end;
} debug_cal_observed_dqsen_margins_t;

typedef struct debug_cal_observed_dqs_out_margins_struct {
	alt_32 dq_margin;
	alt_32 dqs_margin;
	alt_32 dm_margin;
	alt_u32 dqdqs_start;
	alt_u32 dqdqs_end;
} debug_cal_observed_dqs_out_margins_t;

typedef struct debug_cal_dq_settings_struct {
	alt_u32 dq_in_delay;
	alt_u32 dq_out_delay1;
	alt_u32 dq_out_delay2;
} debug_cal_dq_settings_t;

typedef struct debug_cal_dqs_in_settings_struct {
	alt_u32 dqs_bus_in_delay;
	alt_u32 dqs_en_phase;
	alt_u32 dqs_en_delay;
#if TRACKING_ERROR_TEST || TRACKING_WATCH_TEST
	alt_u32 sample_count;
	alt_u32 dtaps_per_ptap;
#endif
} debug_cal_dqs_in_settings_t;

typedef struct debug_cal_dqs_out_settings_struct {
	alt_u32 dqdqs_out_phase;
	alt_u32 dqs_out_delay1;
	alt_u32 dqs_out_delay2;
	alt_u32 oct_out_delay1;
	alt_u32 oct_out_delay2;
	alt_u32 dqs_io_in_delay;
} debug_cal_dqs_out_settings_t;

typedef struct debug_cal_dm_settings_struct {
	alt_u32 dm_in_delay;
	alt_u32 dm_out_delay1;
	alt_u32 dm_out_delay2;
} debug_cal_dm_settings_t;

/* Error stages are defined in sequencer.h */
typedef struct debug_cal_status_per_group_struct {
	alt_u32 error_stage;
	alt_u32 error_sub_stage;
	alt_u32 fom_in;
	alt_u32 fom_out;
} debug_cal_status_per_group_t;

/* Summary report */
typedef struct debug_summary_report_struct {
	// Size in 32-bit words of the report
	alt_u32 data_size;

	alt_u32 report_flags;
	
	alt_u32 sequencer_signature;
	alt_u32 protocol;

	alt_u32 error_stage;
	alt_u32 error_sub_stage;
	alt_u32 error_group;
	alt_u32 fom_in;
	alt_u32 fom_out;

	alt_u32 mem_address_width;
	alt_u32 mem_bank_width;
	alt_u32 mem_control_width;
	alt_u32 mem_cs_width;
	alt_u32 mem_cke_width;
	alt_u32 mem_odt_width;
	alt_u32 mem_data_width;
	alt_u32 mem_dm_width;
	alt_u32 mem_read_dqs_width;
	alt_u32 mem_write_dqs_width;
	alt_u32 mem_dq_per_read_dqs;
	alt_u32 mem_num_ranks;

	alt_u32 num_shadow_regs;

	alt_u32 mem_mmr_burst_len;
	alt_u32 mem_mmr_cas;

	alt_u32 mem_num_dm_per_write_group;

	alt_u32 rate;

	alt_u32 cal_write_latency;
	alt_u32 cal_read_latency;

	alt_u32 dll_length;

	alt_u32 rank_mask_size;
	alt_u32 active_ranks;
	alt_u32 rank_mask[NUM_RANK_MASK_WORDS];

	alt_u32 group_mask_size;
	alt_u32 active_groups;
	alt_u32 group_mask[NUM_GROUP_MASK_WORDS];

	alt_u32 groups_attempted_calibration[NUM_GROUP_MASK_WORDS];

	alt_u32 computed_dtap_per_ptap;
	// The delay per phase tap is the period/dll_length
	alt_u32 io_delay_per_opa_tap;
	// The delay per delay tap is the delay per phase tap
	// divided by the number of delay taps per phase tap
	// (i.e. io_delay_per_opa_tap / computed_dtap_per_ptap)
	// The value of computed_dtap_per_ptap is computed during calibration.

	alt_u32 margin_dq_in_left_delay_chain_len;
	alt_u32 margin_dq_in_right_delay_chain_len;
	alt_u32 margin_dq_out_left_delay_chain_len;
	alt_u32 margin_dq_out_right_delay_chain_len;

} debug_summary_report_t;

/* Calibration report:  The calibration status per group is here (cal_status_per_group)*/
typedef struct debug_cal_report_struct {
	// Size in 32-bit words of the report
	alt_u32 data_size;

	alt_u32 report_flags;

	alt_u32 mem_data_width;
	alt_u32 mem_dm_width;
	alt_u32 mem_num_dm_per_write_group;
	alt_u32 mem_read_dqs_width;
	alt_u32 mem_write_dqs_width;

	alt_u32 num_shadow_regs;
	
	/* Pass/fail status per group */
	debug_cal_status_per_group_t cal_status_per_group[NUM_SHADOW_REGS][RW_MGR_MEM_IF_WRITE_DQS_WIDTH];

	/* Margins observed before calibration.   */
	debug_cal_observed_dq_margins_t cal_dq_in_margins[NUM_SHADOW_REGS][RW_MGR_MEM_DATA_WIDTH];
	debug_cal_observed_dq_margins_t cal_dq_out_margins[NUM_SHADOW_REGS][RW_MGR_MEM_DATA_WIDTH];
	debug_cal_observed_dq_margins_t cal_dm_margins[NUM_SHADOW_REGS][RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP];
	debug_cal_observed_dqsen_margins_t cal_dqsen_margins[NUM_SHADOW_REGS][RW_MGR_MEM_IF_READ_DQS_WIDTH];

	debug_cal_observed_dqs_in_margins_t cal_dqs_in_margins[NUM_SHADOW_REGS][RW_MGR_MEM_IF_READ_DQS_WIDTH];
	debug_cal_observed_dqs_out_margins_t cal_dqs_out_margins[NUM_SHADOW_REGS][RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	
	/* Phase, delay chain settings */
	debug_cal_dq_settings_t cal_dq_settings[NUM_SHADOW_REGS][RW_MGR_MEM_DATA_WIDTH];
	debug_cal_dqs_in_settings_t cal_dqs_in_settings[NUM_SHADOW_REGS][RW_MGR_MEM_IF_READ_DQS_WIDTH];
	debug_cal_dqs_out_settings_t cal_dqs_out_settings[NUM_SHADOW_REGS][RW_MGR_MEM_IF_WRITE_DQS_WIDTH];
	debug_cal_dm_settings_t cal_dm_settings[NUM_SHADOW_REGS][RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP];

} debug_cal_report_t;

/* Left and right edges of the data valid window, in units of delay taps. */
typedef struct debug_margin_min_max_margins_struct {
	alt_u32 min_working_setting;
	alt_u32 max_working_setting;
} debug_margin_min_max_margins_t;

/* Post-calibration margin report (must be enabled using the TCLDBG_ENABLE_MARGIN_REPORT command first)*/
typedef struct debug_margin_report_struct {
	// Size in 32-bit words of the report
	alt_u32 data_size;

	alt_u32 report_flags;
	
	alt_u32 mem_data_width;
	alt_u32 mem_write_dqs_width;

	alt_u32 num_shadow_regs;

	debug_margin_min_max_margins_t margin_dm_margins[NUM_SHADOW_REGS][RW_MGR_MEM_IF_WRITE_DQS_WIDTH][RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP];

	debug_margin_min_max_margins_t margin_dq_in_margins[NUM_SHADOW_REGS][RW_MGR_MEM_DATA_WIDTH];
	debug_margin_min_max_margins_t margin_dq_out_margins[NUM_SHADOW_REGS][RW_MGR_MEM_DATA_WIDTH];
	
	
} debug_margin_report_t;

typedef alt_u32 printf_read_buffer_t[PRINTF_READ_BUFFER_SIZE];

typedef struct debug_printf_output {
	alt_u32 data_size;

	alt_u32 fifo_size;
	alt_u32 word_size;

	alt_u32 head;
	alt_u32 count;
	alt_u32 slave_lock;
	alt_u32 master_lock;


	printf_read_buffer_t read_buffer[PRINTF_READ_BUFFER_FIFO_WORDS];
	printf_read_buffer_t active_word;

} debug_printf_output_t;

typedef struct rw_manager_di_buffer {
	alt_u32 bit_chk;
	alt_u32 delay;
	alt_u32 v;
	alt_u32 p;
	alt_u32 d;
	alt_u32 di_buffer_0a;
	alt_u32 di_buffer_0b;
	alt_u32 di_buffer_1a;
	alt_u32 di_buffer_1b;
	alt_u32 di_buffer_2a;
	alt_u32 di_buffer_2b;
	alt_u32 di_buffer_3a;
	alt_u32 di_buffer_3b;
	alt_u32 di_buffer_4a;
	alt_u32 di_buffer_4b;
} rw_manager_di_buffer_t;

typedef struct rw_manager_di_report {
	alt_u32 data_size;
	alt_u32 flags;
	alt_u32 cur_samples;
	alt_u32 max_samples;
	rw_manager_di_buffer_t di_buffer[NUM_DI_SAMPLE];
} rw_manager_di_report_t;

typedef struct emif_toolkit_debug_data {
	// Size in 32-bit words of all the emif toolkit debug data
	alt_u32 data_size;

	alt_u32 dqs_write_width_ptr;
	alt_u32 group_mask_ptr;
	alt_u32 num_ranks_ptr;
	alt_u32 rank_mask_ptr;
	alt_u32 active_groups_ptr;
	alt_u32 active_ranks_ptr;
	alt_u32 group_mask_size_ptr;
	alt_u32 rank_mask_size_ptr;
} emif_toolkit_debug_data_t;

/* This the main debug data structure.  This is where you write
commands, poll command status, pass command parameters, etc.  Contained
within this data structure are the reports. The memory address of this 
data structure is in core_debug_defines.h (it is dynamic). For example:
#define SEQ_CORE_DEBUG_BASE 0x000140c4
The sizes of all the data structures are dynamic, as they depend on 
interface size and other parameters.  Accessing them outside a software
context is trickier but it can be done by looking at the "data_size"
field of the reports.
*/
typedef struct debug_data_struct {
	// Size in 32-bit words of all the debug data
	alt_u32 data_size;

	// Status bits
	alt_u32 status;

	// Command interaction
	alt_u32 requested_command;
	alt_u32 command_status;
	alt_u32 command_parameters[COMMAND_PARAM_WORDS];

	// Pointers to the reports
	alt_u32 summary_report_ptr;
	alt_u32 cal_report_ptr;
	alt_u32 margin_report_ptr;

	// Printf output report
	alt_u32 printf_output_ptr;

	// Debug toolkit debugging data
	alt_u32 emif_toolkit_debug_data_ptr;

#if ENABLE_DQSEN_SWEEP
	// di report
	alt_u32 di_report_ptr;
#endif

	// Report data structures
	debug_summary_report_t summary_report;
	debug_cal_report_t cal_report;
	debug_margin_report_t margin_report;

#if ENABLE_PRINTF_LOG
	debug_printf_output_t printf_output;
#endif

#if ENABLE_DQSEN_SWEEP
	rw_manager_di_report_t di_report;
#endif

	emif_toolkit_debug_data_t emif_toolkit_debug_data;

} debug_data_t;

/* TCL io memory */

volatile extern debug_summary_report_t *debug_summary_report;
volatile extern debug_cal_report_t *debug_cal_report;
volatile extern debug_margin_report_t *debug_margin_report;
volatile extern debug_printf_output_t *debug_printf_output;
volatile extern debug_data_t *debug_data;

volatile extern emif_toolkit_debug_data_t *debug_emif_toolkit_debug_data;


extern void tclrpt_initialize_debug_status (void);
extern void tclrpt_initialize (debug_data_t *);
extern void tclrpt_loop(void);
extern void tclrpt_initialize_data(void);
extern void tclrpt_set_group_as_calibration_attempted(alt_u32 write_group);

#if BFM_MODE
extern void tclrpt_dump_internal_data(void);
extern void tclrpt_populate_fake_margin_data(void);
#endif

#endif // ENABLE_TCL_DEBUG

#endif
