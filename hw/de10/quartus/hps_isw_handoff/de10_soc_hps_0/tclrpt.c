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

#if ENABLE_TCL_DEBUG

#include "alt_types.h"
#if HPS_HW
#include "sdram_io.h"
#else
#include "io.h"
#include "system.h"
#endif
#include "tclrpt.h"
#include "sequencer.h"
#if BFM_MODE
#include <stdio.h>
#endif

/* TCL io memory */

volatile debug_summary_report_t *debug_summary_report;
volatile debug_cal_report_t *debug_cal_report;
volatile debug_margin_report_t *debug_margin_report;
volatile debug_printf_output_t *debug_printf_output;
volatile debug_data_t *debug_data;

volatile emif_toolkit_debug_data_t *debug_emif_toolkit_debug_data;

alt_u32 tclrpt_get_protocol(void)
{
	// Determine the protocol for the interface. Currently this is
	// done with #defines
	alt_u32 protocol_enum = 0;


	#if DDR3
		protocol_enum = 1;
	#endif
	#if DDR2
		protocol_enum = 2;
	#endif
	#if QDRII
		protocol_enum = 3;
	#endif
	#if RLDRAMII
		protocol_enum = 4;
	#endif
	#if RLDRAM3
		protocol_enum = 5;
	#endif
	#if LPDDR2
		protocol_enum = 6;
	#endif

	return protocol_enum;
}

alt_u32 tclrpt_get_rate(void)
{
	alt_u32 rate_enum;

	rate_enum = 0;

	// Right now the rate is determined by ifdefs
	#if QUARTER_RATE
		rate_enum = 4;
	#else
		#if HALF_RATE
			rate_enum = 2;
		#else
			rate_enum = 1;
		#endif
	#endif

	return rate_enum;
}

void tclrpt_update_rank_group_mask(void)
{
	alt_u32 i;
	alt_u32 active_groups;
	alt_u32 active_ranks;
	alt_u32 rank;
	alt_u32 rank_mask_word;
	alt_u32 shadow_reg;
	alt_u32 skip_shadow_reg;

	// Currently the alt_u32 word param->skip_groups keeps all the groups.
	TCLRPT_SET(debug_summary_report->group_mask[0], param->skip_groups);
	active_groups = RW_MGR_MEM_IF_WRITE_DQS_WIDTH;
	for (i = 0; i < 32; i++)
	{
		if (param->skip_groups & (1 << i))
		{
			active_groups--;
		}
	}
	TCLRPT_SET(debug_summary_report->active_groups, active_groups);

	// Iterate through the ranks array and set the bits
	active_ranks = RW_MGR_MEM_NUMBER_OF_RANKS;
	for (rank_mask_word = 0; rank_mask_word < NUM_RANK_MASK_WORDS; rank_mask_word++)
	{
		for (rank = rank_mask_word*32; rank < RW_MGR_MEM_NUMBER_OF_RANKS; rank++)
		{
			if (param->skip_ranks[rank]) {
				active_ranks--;
				TCLRPT_SET(debug_summary_report->rank_mask[rank_mask_word], debug_summary_report->rank_mask[rank_mask_word] | (1 << (rank-(32*rank_mask_word))));
			}
		}
	}
	
	// Update skip_shadow_regs
	for (shadow_reg = 0, rank = 0; shadow_reg < NUM_SHADOW_REGS; ++shadow_reg, rank += NUM_RANKS_PER_SHADOW_REG)
	{
		skip_shadow_reg = 1;
		for (i = 0; i < NUM_RANKS_PER_SHADOW_REG; ++i)
		{
			if (! param->skip_ranks[rank + i])
			{
				skip_shadow_reg = 0;
				break;
			}
		}
		param->skip_shadow_regs[shadow_reg] = skip_shadow_reg;
	}
	
	TCLRPT_SET(debug_summary_report->active_ranks, active_ranks);
}

void tclrpt_initialize_rank_group_mask(void)
{
	alt_u32 group_mask;
	alt_u32 rank_mask_word;

	// Initialize the globals
	TCLRPT_SET(debug_summary_report->rank_mask_size, NUM_RANK_MASK_WORDS);
	TCLRPT_SET(debug_summary_report->active_ranks, debug_summary_report->mem_num_ranks);

	TCLRPT_SET(debug_summary_report->group_mask_size, NUM_GROUP_MASK_WORDS);
	TCLRPT_SET(debug_summary_report->active_groups, debug_summary_report->mem_write_dqs_width);

	// Initialize all words to be 0
	for (group_mask = 0; group_mask < NUM_GROUP_MASK_WORDS; group_mask++)
	{
		TCLRPT_SET(debug_summary_report->group_mask[group_mask], 0);
	}

	for (rank_mask_word = 0; rank_mask_word < NUM_RANK_MASK_WORDS; rank_mask_word++)
	{
		TCLRPT_SET(debug_summary_report->rank_mask[rank_mask_word], 0);
	}

}

void tclrpt_clear_calibrated_groups_index(void)
{
	alt_u32 group_mask;

	// Initialize all words to be 0
	for (group_mask = 0; group_mask < NUM_GROUP_MASK_WORDS; group_mask++)
	{
		TCLRPT_SET(debug_summary_report->groups_attempted_calibration[group_mask], 0);
	}
}

void tclrpt_set_group_as_calibration_attempted(alt_u32 write_group)
{
#if ENABLE_TCL_DEBUG
	alt_u32 group_mask_word;
	alt_u32 group_mask_bit;
	alt_u32 calibrated_groups;
	
	group_mask_word = write_group / 32;
	group_mask_bit = write_group - group_mask_word*32;
	calibrated_groups = debug_summary_report->groups_attempted_calibration[group_mask_word];
	
	calibrated_groups |= 1 << group_mask_bit;
	
	TCLRPT_SET(debug_summary_report->groups_attempted_calibration[group_mask_word], calibrated_groups);
#endif
}

void tclrpt_initialize_calib_settings(void)
{
  alt_u32 read_group, write_group, dq, dm, sr;

  for (sr = 0; sr < NUM_SHADOW_REGS; sr++) {
	// Initialize the margins observed during calibration
	for (read_group = 0; read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH; read_group++)
	{
		TCLRPT_SET(debug_cal_report->cal_dqs_in_margins[sr][read_group].dq_margin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_margins[sr][read_group].dqs_margin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_margins[sr][read_group].dqsen_margin, 0);
	}

	for (read_group = 0; read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH; read_group++)
	{
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].vfifo_begin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].phase_begin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].delay_begin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].work_begin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].vfifo_end, 0);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].phase_end, 0);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].delay_end, 0);
		TCLRPT_SET(debug_cal_report->cal_dqsen_margins[sr][read_group].work_end, 0);
	}


	for (write_group = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++)
	{
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[sr][write_group].dq_margin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[sr][write_group].dm_margin, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[sr][write_group].dqdqs_start, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[sr][write_group].dqdqs_end, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_margins[sr][write_group].dqs_margin, 0);
	}

	for (dq = 0; dq < RW_MGR_MEM_DATA_WIDTH; dq++)
	{
		TCLRPT_SET(debug_cal_report->cal_dq_in_margins[sr][dq].left_edge, 0);
		TCLRPT_SET(debug_cal_report->cal_dq_in_margins[sr][dq].right_edge, 0);
		TCLRPT_SET(debug_cal_report->cal_dq_out_margins[sr][dq].left_edge, 0);
		TCLRPT_SET(debug_cal_report->cal_dq_out_margins[sr][dq].right_edge, 0);
	}

	for (write_group = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++)
	{
		for (dm = 0; dm < RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP; dm++)
		{
			TCLRPT_SET(debug_cal_report->cal_dm_margins[sr][write_group][dm].left_edge, 0);
			TCLRPT_SET(debug_cal_report->cal_dm_margins[sr][write_group][dm].right_edge, 0);
		}
	}

	// Initialize the DQ and DQS settings
	for (read_group = 0; read_group < RW_MGR_MEM_IF_READ_DQS_WIDTH; read_group++)
	{
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[sr][read_group].dqs_en_delay, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[sr][read_group].dqs_en_phase, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[sr][read_group].dqs_bus_in_delay, 0);
#if TRACKING_ERROR_TEST || TRACKING_WATCH_TEST
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[sr][read_group].sample_count, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_in_settings[sr][read_group].dtaps_per_ptap, 0);
#endif
	}
	for (write_group = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++)
	{
		TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[sr][write_group].dqdqs_out_phase, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[sr][write_group].dqs_out_delay1, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[sr][write_group].dqs_out_delay2, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[sr][write_group].oct_out_delay1, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[sr][write_group].oct_out_delay2, 0);
		TCLRPT_SET(debug_cal_report->cal_dqs_out_settings[sr][write_group].dqs_io_in_delay, 0);
	}
	for (dq = 0; dq < RW_MGR_MEM_DATA_WIDTH; dq++)
	{
		TCLRPT_SET(debug_cal_report->cal_dq_settings[sr][dq].dq_in_delay, 0);
		TCLRPT_SET(debug_cal_report->cal_dq_settings[sr][dq].dq_out_delay1, 0);
		TCLRPT_SET(debug_cal_report->cal_dq_settings[sr][dq].dq_out_delay2, 0);
	}

	for (write_group = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++)
	{
		for (dm = 0; dm < RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP; dm++)
		{
			TCLRPT_SET(debug_cal_report->cal_dm_settings[sr][write_group][dm].dm_in_delay, 0);
			TCLRPT_SET(debug_cal_report->cal_dm_settings[sr][write_group][dm].dm_out_delay1, 0);
			TCLRPT_SET(debug_cal_report->cal_dm_settings[sr][write_group][dm].dm_out_delay2, 0);
		}
		}
	}
}

void tclrpt_initialize_margins(void)
{
	alt_u32 write_group, dq, dm, sr;


	TCLRPT_SET(debug_margin_report->mem_data_width, RW_MGR_MEM_DATA_WIDTH);
	TCLRPT_SET(debug_margin_report->mem_write_dqs_width, RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	TCLRPT_SET(debug_margin_report->num_shadow_regs, NUM_SHADOW_REGS);

	// Initialize the margins
	for (sr = 0; sr < NUM_SHADOW_REGS; sr++)
	{
	for (write_group = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++)
	{
		for (dm = 0; dm < RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP; dm++)
		{
				TCLRPT_SET(debug_margin_report->margin_dm_margins[sr][write_group][dm].max_working_setting, 0);
				TCLRPT_SET(debug_margin_report->margin_dm_margins[sr][write_group][dm].min_working_setting, 0);
		}
	}
	for (dq = 0; dq < RW_MGR_MEM_DATA_WIDTH; dq++)
	{
			TCLRPT_SET(debug_margin_report->margin_dq_in_margins[sr][dq].max_working_setting, 0);
			TCLRPT_SET(debug_margin_report->margin_dq_in_margins[sr][dq].min_working_setting, 0);
	}
	for (dq = 0; dq < RW_MGR_MEM_DATA_WIDTH; dq++)
	{
			TCLRPT_SET(debug_margin_report->margin_dq_out_margins[sr][dq].max_working_setting, 0);
			TCLRPT_SET(debug_margin_report->margin_dq_out_margins[sr][dq].min_working_setting, 0);
		}
	}
}

void tclrpt_initialize_calib_results(void)
{
  alt_u32 write_group, sr;

	// Initialize the FOM and error status
	TCLRPT_SET(debug_summary_report->error_stage, CAL_STAGE_CAL_SKIPPED);
	TCLRPT_SET(debug_summary_report->error_group, 0xff);
	TCLRPT_SET(debug_summary_report->fom_in, 0);
	TCLRPT_SET(debug_summary_report->fom_out, 0);

	for (sr = 0; sr < NUM_SHADOW_REGS; sr++) {
	// Initialize the per group calibration report
	for (write_group = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++)
	{
		  TCLRPT_SET(debug_cal_report->cal_status_per_group[sr][write_group].fom_in, 0);
		  TCLRPT_SET(debug_cal_report->cal_status_per_group[sr][write_group].fom_out, 0);
		  TCLRPT_SET(debug_cal_report->cal_status_per_group[sr][write_group].error_stage, CAL_STAGE_CAL_SKIPPED);
		}
	}
}

void tclrpt_initialize_calib_latency(void)
{
	// KALEN: Need to figure out the best way to return the calibrated
	// read and write latencies. For now leave as blank
	TCLRPT_SET(debug_summary_report->cal_write_latency, 0);
	TCLRPT_SET(debug_summary_report->cal_read_latency, 0);

}

void tclrpt_initialize_printf_output(void)
{
	// Initialize the pointers
	debug_printf_output->head = 0;
	debug_printf_output->count = 0;
	debug_printf_output->slave_lock = 0;
	debug_printf_output->master_lock = 0;
	debug_printf_output->fifo_size = PRINTF_READ_BUFFER_FIFO_WORDS;
	debug_printf_output->word_size = PRINTF_READ_BUFFER_SIZE;
}

void tclrpt_initialize_emif_toolkit_debug_data(void)
{
	// Initialize the points to the calibration data
	debug_emif_toolkit_debug_data->dqs_write_width_ptr = (alt_u32)(&debug_summary_report->mem_write_dqs_width);
	debug_emif_toolkit_debug_data->group_mask_ptr =  (alt_u32)(&debug_summary_report->group_mask);
	debug_emif_toolkit_debug_data->num_ranks_ptr =  (alt_u32)(&debug_summary_report->mem_num_ranks);
	debug_emif_toolkit_debug_data->rank_mask_ptr =  (alt_u32)(&debug_summary_report->rank_mask);
	debug_emif_toolkit_debug_data->active_groups_ptr =  (alt_u32)(&debug_summary_report->active_groups);
	debug_emif_toolkit_debug_data->active_ranks_ptr =  (alt_u32)(&debug_summary_report->active_ranks);
	debug_emif_toolkit_debug_data->group_mask_size_ptr =  (alt_u32)(&debug_summary_report->group_mask_size);
	debug_emif_toolkit_debug_data->rank_mask_size_ptr =  (alt_u32)(&debug_summary_report->rank_mask_size);
}

void tclrpt_initialize_data(void)
{


	// Set the initial value of the report flags
	debug_summary_report->report_flags = 0;
	debug_cal_report->report_flags = 0;
	debug_margin_report->report_flags = 0;

	// Set which reports are enabled based on the global structure.
	// This only applies to the calibration and margining report
	TCLRPT_SET(debug_summary_report->report_flags, debug_summary_report->report_flags |= DEBUG_REPORT_STATUS_REPORT_GEN_ENABLED);

	if (gbl->phy_debug_mode_flags & PHY_DEBUG_ENABLE_CAL_RPT)
	{
		TCLRPT_SET(debug_cal_report->report_flags, debug_cal_report->report_flags |= DEBUG_REPORT_STATUS_REPORT_GEN_ENABLED);
	}
	if (gbl->phy_debug_mode_flags & PHY_DEBUG_ENABLE_MARGIN_RPT)
	{
		TCLRPT_SET(debug_margin_report->report_flags, debug_margin_report->report_flags |= DEBUG_REPORT_STATUS_REPORT_GEN_ENABLED);
	}


	// Initialize the static data into the report

	TCLRPT_SET(debug_summary_report->protocol, tclrpt_get_protocol());
#if !HPS_HW
	TCLRPT_SET(debug_summary_report->sequencer_signature, IORD_32DIRECT (REG_FILE_SIGNATURE, 0));
#endif

    // If available, use ROW/COL addr width as they have the true width,
    // not the static width used by the AC ROM.
#if MEM_IF_ROW_ADDR_WIDTH
#if MEM_IF_COL_ADDR_WIDTH
	TCLRPT_SET(debug_summary_report->mem_address_width, (MEM_IF_COL_ADDR_WIDTH > MEM_IF_ROW_ADDR_WIDTH) ? MEM_IF_COL_ADDR_WIDTH : MEM_IF_ROW_ADDR_WIDTH);
#else
	TCLRPT_SET(debug_summary_report->mem_address_width, RW_MGR_MEM_ADDRESS_WIDTH);
#endif
#else
	TCLRPT_SET(debug_summary_report->mem_address_width, RW_MGR_MEM_ADDRESS_WIDTH);
#endif

	TCLRPT_SET(debug_summary_report->mem_bank_width, RW_MGR_MEM_BANK_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_control_width, RW_MGR_MEM_CONTROL_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_cs_width, RW_MGR_MEM_CHIP_SELECT_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_cke_width, RW_MGR_MEM_CLK_EN_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_odt_width, RW_MGR_MEM_ODT_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_data_width, RW_MGR_MEM_DATA_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_dm_width, RW_MGR_TRUE_MEM_DATA_MASK_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_read_dqs_width, RW_MGR_MEM_IF_READ_DQS_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_write_dqs_width, RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	TCLRPT_SET(debug_summary_report->mem_dq_per_read_dqs, RW_MGR_MEM_DQ_PER_READ_DQS);
	TCLRPT_SET(debug_summary_report->mem_num_ranks, RW_MGR_MEM_NUMBER_OF_RANKS);

#if DDRX
#if LPDDR2
	TCLRPT_SET(debug_summary_report->mem_mmr_burst_len, MEM_BURST_LEN);
#else
	TCLRPT_SET(debug_summary_report->mem_mmr_burst_len, RW_MGR_MR0_BL);
#endif
#else
	TCLRPT_SET(debug_summary_report->mem_mmr_burst_len, MEM_BURST_LEN);
#endif
	TCLRPT_SET(debug_summary_report->mem_mmr_cas, RW_MGR_MR0_CAS_LATENCY);

	TCLRPT_SET(debug_summary_report->mem_num_dm_per_write_group, RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP);

	TCLRPT_SET(debug_summary_report->rate, tclrpt_get_rate());

	TCLRPT_SET(debug_summary_report->dll_length, IO_DLL_CHAIN_LENGTH);

	TCLRPT_SET(debug_summary_report->num_shadow_regs, NUM_SHADOW_REGS);

	// Initialize the timing data
	TCLRPT_SET(debug_summary_report->computed_dtap_per_ptap, 0);
	TCLRPT_SET(debug_summary_report->io_delay_per_opa_tap, IO_DELAY_PER_OPA_TAP);

	// Initialize the margin maxes
	TCLRPT_SET(debug_summary_report->margin_dq_in_left_delay_chain_len, IO_IO_IN_DELAY_MAX + 1);
	TCLRPT_SET(debug_summary_report->margin_dq_in_right_delay_chain_len, IO_DQS_IN_DELAY_MAX + 1);
	TCLRPT_SET(debug_summary_report->margin_dq_out_left_delay_chain_len, IO_IO_OUT1_DELAY_MAX + 1);
	TCLRPT_SET(debug_summary_report->margin_dq_out_right_delay_chain_len, IO_IO_OUT1_DELAY_MAX + 1);


	// Initialize the sizes in the calibration tables
	TCLRPT_SET(debug_cal_report->mem_data_width, RW_MGR_MEM_DATA_WIDTH);
	TCLRPT_SET(debug_cal_report->mem_dm_width, RW_MGR_TRUE_MEM_DATA_MASK_WIDTH);
	TCLRPT_SET(debug_cal_report->mem_num_dm_per_write_group, RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP);
	TCLRPT_SET(debug_cal_report->mem_read_dqs_width, RW_MGR_MEM_IF_READ_DQS_WIDTH);
	TCLRPT_SET(debug_cal_report->mem_write_dqs_width, RW_MGR_MEM_IF_WRITE_DQS_WIDTH);
	TCLRPT_SET(debug_cal_report->num_shadow_regs, NUM_SHADOW_REGS);

	// Initialize the other members of the data structures

	tclrpt_initialize_calib_latency();

	tclrpt_initialize_rank_group_mask();

	tclrpt_initialize_calib_results();

	tclrpt_update_rank_group_mask();

	tclrpt_clear_calibrated_groups_index();

	tclrpt_initialize_calib_settings();

	tclrpt_initialize_margins();

	tclrpt_initialize_emif_toolkit_debug_data();
}

void tclrpt_initialize (debug_data_t *debug_data_ptr)
{
	alt_u32 i;

	/* Set the pointers to the memory used to communicate
	 * with the TCL scripts.
	 */

	if (debug_data_ptr == 0)
	{
		// If the debug data pointer is NULL then initialize to disallow any access

		// Set the register file offset for the data to be 0
#if !HPS_HW
		IOWR_32DIRECT (REG_FILE_DEBUG_DATA_ADDR, 0, 0);
#endif

		// Initialize all pointers to NULL
		debug_summary_report = 0;
		debug_cal_report = 0;
		debug_margin_report = 0;
		debug_printf_output = 0;

		debug_data = 0;
	}
	else
	{
		// Set the register file offset for the data
#if !HPS_HW
		IOWR_32DIRECT (REG_FILE_DEBUG_DATA_ADDR, 0, (alt_u32)debug_data_ptr);
#endif

		// Set the global pointers
		debug_data = debug_data_ptr;
		debug_summary_report = &(debug_data->summary_report);
		debug_cal_report = &(debug_data->cal_report);
		debug_margin_report = &(debug_data->margin_report);
#if ENABLE_PRINTF_LOG
		debug_printf_output = &(debug_data->printf_output);
#else
		debug_printf_output = 0;
#endif
		debug_emif_toolkit_debug_data = &(debug_data->emif_toolkit_debug_data);



		// Initialize the status
		debug_data->status = 0;

		// Update local pointers
		debug_data->summary_report_ptr = (alt_u32)debug_summary_report;
		debug_data->cal_report_ptr = (alt_u32)debug_cal_report;
		debug_data->margin_report_ptr = (alt_u32)debug_margin_report;
#if ENABLE_DQSEN_SWEEP
		debug_data->di_report_ptr = (alt_u32)(&debug_data->di_report);
#endif
		debug_data->emif_toolkit_debug_data_ptr = (alt_u32)(&debug_data->emif_toolkit_debug_data);

		// Set the sizes of the structs
		debug_data->data_size = sizeof(debug_data_t);
		debug_summary_report->data_size = sizeof(debug_summary_report_t);
		debug_cal_report->data_size = sizeof(debug_cal_report_t);
		debug_margin_report->data_size = sizeof(debug_margin_report_t);
#if ENABLE_DQSEN_SWEEP
		debug_data->di_report.data_size = sizeof(rw_manager_di_report_t);
		debug_data->di_report.max_samples = NUM_DI_SAMPLE;
#endif
		debug_emif_toolkit_debug_data->data_size = sizeof(emif_toolkit_debug_data_t);

		// Set the initial value of the report flags
		debug_summary_report->report_flags = 0;
		debug_cal_report->report_flags = 0;
		debug_margin_report->report_flags = 0;
#if ENABLE_DQSEN_SWEEP
		debug_data->di_report.flags = 0;
#endif

		// Initialize the command status
		debug_data->command_status = TCLDBG_TX_STATUS_CMD_EXE;
		debug_data->requested_command = TCLDBG_CMD_NOP;
		for (i = 0; i < COMMAND_PARAM_WORDS; i++)
		{
			debug_data->command_parameters[i] = 0;
		}

#if ENABLE_PRINTF_LOG
		// Initialize the size and pointers
		debug_data->printf_output_ptr = (alt_u32)debug_printf_output;
		debug_printf_output->data_size = sizeof(debug_printf_output_t);

		// Initialize the payload
		tclrpt_initialize_printf_output();
#else
		debug_data->printf_output_ptr = (alt_u32)0;
#endif

	}
}

void tclrpt_initialize_debug_status (void)
{
	// Initialize the status bits of the status word.
	debug_data->status = 0;

	// Initialize the debug status to show that printf log is enabled
#if ENABLE_PRINTF_LOG
	debug_data->status |= 1 << DEBUG_STATUS_PRINTF_ENABLED_BIT;
#endif

}


void tclrpt_mark_interface_as_ready(void)
{
	// Set the debug data command to TCLDBG_CMD_WAIT_CMD and the status to ready.
	// This identifies the interface as being ready to accept commands.
	// We must write the command first as there is the potential that
	// both the JTAG master and NIOS try writing to the same address

	debug_data->requested_command = TCLDBG_CMD_WAIT_CMD;
	debug_data->command_status = TCLDBG_TX_STATUS_CMD_READY;
}

void tclrpt_mark_interface_as_response_ready(void)
{
	// Mark the interface as saying that the response to the requested
	// command is ready.
	debug_data->command_status = TCLDBG_TX_STATUS_RESPONSE_READY;
}

void tclrpt_mark_interface_as_illegal_command(void)
{
	// Mark the interface as saying that the command or parameter requested was
	// illegal
	debug_data->command_status = TCLDBG_TX_STATUS_ILLEGAL_CMD;
}

#if !HPS_HW
void tclrpt_loop(void)
{
	alt_u32 rank, group, pin, pin_in_group, value;
	alt_u32 rdimm_control_word_updated;
	rdimm_control_word_updated = 0;	
	// Set up the interface to be ready for access. The initial state is user mode.
	tclrpt_mark_interface_as_ready();

	// Forever just respond to commands
	for (;;)
	{
		// This is to check if RDIMM Control Word has been updated
		if ( rdimm_control_word_updated == 1 ) 
		{
			break;
		}		
		
	    // KALEN: Do we need this in debug?
	    // AIDIL: Yes since debug is turned on by default, not having this means deep
	    // powerdown would stall controller
		user_init_cal_req();
	    
		if (debug_data->command_status == TCLDBG_TX_STATUS_RESPONSE_READY
				|| debug_data->command_status == TCLDBG_TX_STATUS_ILLEGAL_CMD)
			switch (debug_data->requested_command)
			{
				case TCLDBG_CMD_RESPONSE_ACK :
					// The TCL interface has read the response
					// Since the TCL interface has read the response we can mark the interface
					// As being ready to accept another command
					tclrpt_mark_interface_as_ready();
					break;
			}
		else if (debug_data->command_status == TCLDBG_TX_STATUS_CMD_READY)
		{
			switch (debug_data->requested_command)
			{
				case TCLDBG_SET_UPDATE_PARAMETERS :
					rdimm_control_word_updated = 1;
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_CMD_WAIT_CMD :
					//wait for commands
					break;
				case TCLDBG_CMD_NOP : //NOOP command
					// Perform no operation
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_RUN_MEM_CALIBRATE :
					// Run the full memory calibration
#if ENABLE_NON_DES_CAL					
					run_mem_calibrate(0);
#else
					run_mem_calibrate();
#endif
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_RUN_NON_DES_MEM_CALIBRATE :
					// Run the full memory calibration
#if ENABLE_NON_DES_CAL					
					run_mem_calibrate(1);
#else
					run_mem_calibrate();
#endif
					tclrpt_mark_interface_as_response_ready();
					break;

				case TCLDBG_RUN_EYE_DIAGRAM_PATTERN :
					// Generate the pattern to view eye diagrams
					// This function never returns
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_MARK_ALL_DQS_GROUPS_AS_VALID :
					// Mark all groups as being valid for calibration
					param->skip_groups = 0;
					tclrpt_update_rank_group_mask();
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_MARK_GROUP_AS_SKIP :
					// Mark the specified group as being skipped for calibration

					// Make sure it is a legal group
					if (debug_data->command_parameters[0] < RW_MGR_MEM_IF_WRITE_DQS_WIDTH) {
						param->skip_groups |= 1 << debug_data->command_parameters[0];

						tclrpt_update_rank_group_mask();
						tclrpt_mark_interface_as_response_ready();
					}
					else {
						// Illegal payload detected
						tclrpt_mark_interface_as_illegal_command();
					}

					break;
				case TCLDBG_MARK_ALL_RANKS_AS_VALID :
					// Mark all ranks as being valid for calibration
					for (rank = 0; rank < RW_MGR_MEM_NUMBER_OF_RANKS; rank++)
					{
						param->skip_ranks[rank] = 0;
					}
					tclrpt_update_rank_group_mask();
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_MARK_RANK_AS_SKIP :
					// Make sure it is a legal group
					if (debug_data->command_parameters[0] < RW_MGR_MEM_NUMBER_OF_RANKS) {
						param->skip_ranks[debug_data->command_parameters[0]] = 1;

						tclrpt_update_rank_group_mask();
						tclrpt_mark_interface_as_response_ready();
					}
					else {
						// Illegal payload detected
						tclrpt_mark_interface_as_illegal_command();
					}

					break;
				case TCLDBG_ENABLE_MARGIN_REPORT :
					gbl->phy_debug_mode_flags |= PHY_DEBUG_ENABLE_MARGIN_RPT;
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_ENABLE_SWEEP_ALL_GROUPS :
					gbl->phy_debug_mode_flags |= PHY_DEBUG_SWEEP_ALL_GROUPS;
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_DISABLE_GUARANTEED_READ :
					gbl->phy_debug_mode_flags |= PHY_DEBUG_DISABLE_GUARANTEED_READ;
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_SET_NON_DESTRUCTIVE_CALIBRATION:
				  if (debug_data->command_parameters[0]) {
					gbl->phy_debug_mode_flags |= PHY_DEBUG_ENABLE_NON_DESTRUCTIVE_CALIBRATION;
				  } else {
					gbl->phy_debug_mode_flags &= ~(PHY_DEBUG_ENABLE_NON_DESTRUCTIVE_CALIBRATION);
				  }
				  tclrpt_mark_interface_as_response_ready();
				  break;
				#if ENABLE_DELAY_CHAIN_WRITE
				case TCLDBG_SET_DQ_D1_DELAY :
					// DQ D1 Delay (I/O buffer to input register)
					pin = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];
					group = pin/RW_MGR_MEM_DQ_PER_READ_DQS;
					pin_in_group = pin%RW_MGR_MEM_DQ_PER_READ_DQS;

					// Make sure parameter values are legal
					if (pin < RW_MGR_MEM_DATA_WIDTH && value <= IO_IO_IN_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dq_in_delay(group, pin_in_group, value);
						scc_mgr_load_dq (pin_in_group);
						IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQ_D5_DELAY :
					// DQ D5 Delay (output register to I/O buffer)
					pin = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];
					group = pin/RW_MGR_MEM_DQ_PER_WRITE_DQS;
					pin_in_group = pin%RW_MGR_MEM_DQ_PER_WRITE_DQS;

					// Make sure parameter values are legal
					if (pin < RW_MGR_MEM_DATA_WIDTH && value <= IO_IO_OUT1_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dq_out1_delay(group, pin_in_group, value);
						scc_mgr_load_dq (pin_in_group);
						IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQ_D6_DELAY :
					// DQ D6 Delay (output register to I/O buffer)
					pin = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];
					group = pin/RW_MGR_MEM_DQ_PER_WRITE_DQS;
					pin_in_group = pin%RW_MGR_MEM_DQ_PER_WRITE_DQS;

					// Make sure parameter values are legal
					if (pin < RW_MGR_MEM_DATA_WIDTH && value <= IO_IO_OUT2_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dq_out2_delay(group, pin_in_group, value);
						scc_mgr_load_dq (pin_in_group);
						IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQS_D4_DELAY :
					// DQS D4 Delay (DQS delay chain)
					group = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH && value <= IO_DQS_IN_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dqs_bus_in_delay(group, value);
						scc_mgr_load_dqs (group);
						IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQDQS_OUTPUT_PHASE :
					// DQS DQ Output Phase (deg) = DQS Output Phase (deg) - 90
					group = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH && value <= IO_DQDQS_OUT_PHASE_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dqdqs_output_phase_all_ranks (group, value);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQS_D5_DELAY :
					// DQS D5 Delay (output register to I/O buffer) = D5 OCT Delay (OCT to I/O buffer)
					group = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH && value <= IO_IO_OUT1_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_group_dqs_io_and_oct_out1_gradual (group, value);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQS_D6_DELAY :
					// DQS D6 Delay (output register to I/O buffer) = D6 OCT Delay (OCT to I/O buffer)
					group = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH && value <= IO_IO_OUT2_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_group_dqs_io_and_oct_out2_gradual (group, value);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQS_EN_PHASE :
					// DQS Enable Phase (deg)
					group = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH && value <= IO_DQS_EN_PHASE_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dqs_en_phase_all_ranks (group, value);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DQS_T11_DELAY :
					// DQS T11 Delay (DQS post-amble delay)
					group = debug_data->command_parameters[0];
					value = debug_data->command_parameters[1];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH && value <= IO_DQS_EN_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dqs_en_delay_all_ranks (group, value);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DM_D5_DELAY :
					// DM D5 OCT Delay (OCT to I/O buffer)
					group = debug_data->command_parameters[0];
					pin_in_group = debug_data->command_parameters[1];
					value = debug_data->command_parameters[2];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH && pin_in_group < RW_MGR_NUM_DM_PER_WRITE_GROUP && value <= IO_IO_OUT1_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dm_out1_delay(group, pin_in_group, value);
						scc_mgr_load_dm (pin_in_group);
						IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SET_DM_D6_DELAY :
					// DM D6 OCT Delay (OCT to I/O buffer)
					group = debug_data->command_parameters[0];
					pin_in_group = debug_data->command_parameters[1];
					value = debug_data->command_parameters[2];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH && pin_in_group < RW_MGR_NUM_DM_PER_WRITE_GROUP && value <= IO_IO_OUT2_DELAY_MAX) {
						IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
						scc_mgr_set_dm_out2_delay(group, pin_in_group, value);
						scc_mgr_load_dm (pin_in_group);
						IOWR_32DIRECT (SCC_MGR_UPD, 0, 0);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_REMARGIN_DQ :
					rank = debug_data->command_parameters[0];
					group = debug_data->command_parameters[1];
					// Pre-margining process
					initialize();
					rw_mgr_mem_initialize ();
					mem_config ();
					
					IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
					run_dq_margining(rank, group);
					
					// Post-margining process
					rw_mgr_mem_handoff ();
					IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 0); // Give control back to user logic.
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_REMARGIN_DM :
					rank = debug_data->command_parameters[0];
					group = debug_data->command_parameters[1];
					// Pre-margining process
					initialize();
					rw_mgr_mem_initialize ();
					mem_config ();
					
					IOWR_32DIRECT (SCC_MGR_GROUP_COUNTER, 0, group);
					run_dm_margining(rank, group);
					
					// Post-margining process
					rw_mgr_mem_handoff ();
					IOWR_32DIRECT (PHY_MGR_MUX_SEL, 0, 0); // Give control back to user logic.
					tclrpt_mark_interface_as_response_ready();
					break;
				case TCLDBG_INCR_VFIFO :
					// Increment the VFIFO by 1 step
					group = debug_data->command_parameters[0];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH) {
						rw_mgr_incr_vfifo_auto(group);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_DECR_VFIFO :
					// Decrement the VFIFO by 1 step
					group = debug_data->command_parameters[0];

					// Make sure parameter values are legal
					if (group < RW_MGR_MEM_IF_READ_DQS_WIDTH) {
						rw_mgr_decr_vfifo_auto(group);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				case TCLDBG_SELECT_SHADOW_REG :
					rank = debug_data->command_parameters[0];
					if (rank < RW_MGR_MEM_NUMBER_OF_RANKS) {
						select_shadow_regs_for_update (rank, 0, 1);
						tclrpt_mark_interface_as_response_ready();
					} else {
						tclrpt_mark_interface_as_illegal_command();
					}
					break;
				#endif // ENABLE_DELAY_CHAIN_WRITE
				default :
					// Illegal command
					tclrpt_mark_interface_as_illegal_command();
					break;
			}
		}
	}
}
#endif

#if BFM_MODE
const char* const EMITT_XML_CSTR_CONNECTIONS = "connections";
const char* const EMITT_XML_CSTR_CONNECTION = "connection";
const char* const EMITT_XML_CSTR_CONNECTION_DATA = "connection_data";
const char* const EMITT_XML_CSTR_HARDWARE_NAME = "hardware_name";
const char* const EMITT_XML_CSTR_DEVICE_NAME = "device_name";
const char* const EMITT_XML_CSTR_CONNECTION_PATH = "connection_path";
const char* const EMITT_XML_CSTR_CONNECTION_PATH_BASE = "connection_path_base";
const char* const EMITT_XML_CSTR_HIERARCHY_PATH = "hierarchy_path";
const char* const EMITT_XML_CSTR_CONNECTION_TYPE = "connection_type";
const char* const EMITT_XML_CSTR_SLD_TYPE = "sld_type";
const char* const EMITT_XML_CSTR_EMULATION_DATA = "emulation_data";
const char* const EMITT_XML_CSTR_EMU_DATA = "emu_data";
const char* const EMITT_XML_CSTR_EMU_ADDRESS = "address";

void tclrpt_dump_internal_data(void)
{
	alt_u32* base_data;
	FILE *fh;
	int i;

	// Mark the interface as ready to receive transactions. This is 
	// equivalent to what tclrpt_loop() does
	tclrpt_mark_interface_as_ready();

	fh = fopen("emitt_debug_data.xml", "w");


	fprintf(fh, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fh, "<%s>\n", EMITT_XML_CSTR_CONNECTIONS);
	fprintf(fh, "\t<%s>\n", EMITT_XML_CSTR_CONNECTION);
	fprintf(fh, "\t\t<%s>\n", EMITT_XML_CSTR_CONNECTION_DATA);
	fprintf(fh, "\t\t\t<%s>fake_hardware</%s>\n", EMITT_XML_CSTR_HARDWARE_NAME, EMITT_XML_CSTR_HARDWARE_NAME);
	fprintf(fh, "\t\t\t<%s>fake_device@1</%s>\n", EMITT_XML_CSTR_DEVICE_NAME, EMITT_XML_CSTR_DEVICE_NAME);
	fprintf(fh, "\t\t\t<%s>some_device_connection_path</%s>\n", EMITT_XML_CSTR_CONNECTION_PATH, EMITT_XML_CSTR_CONNECTION_PATH);
	fprintf(fh, "\t\t\t<%s>some_device_connection_path</%s>\n", EMITT_XML_CSTR_CONNECTION_PATH_BASE, EMITT_XML_CSTR_CONNECTION_PATH_BASE);
	fprintf(fh, "\t\t\t<%s></%s>\n", EMITT_XML_CSTR_HIERARCHY_PATH, EMITT_XML_CSTR_HIERARCHY_PATH);
	fprintf(fh, "\t\t\t<%s>EMITT_CONNECTION_TYPE_ENUM::UNKNOWN</%s>\n", EMITT_XML_CSTR_CONNECTION_TYPE, EMITT_XML_CSTR_CONNECTION_TYPE);
	fprintf(fh, "\t\t\t<%s>EMITT_CONNECTION_SLD_TYPE_ENUM::DEVICE</%s>\n", EMITT_XML_CSTR_SLD_TYPE, EMITT_XML_CSTR_SLD_TYPE);
	fprintf(fh, "\t\t</%s>\n", EMITT_XML_CSTR_CONNECTION_DATA);
	fprintf(fh, "\t</%s>\n", EMITT_XML_CSTR_CONNECTION);
	fprintf(fh, "\t<%s>\n", EMITT_XML_CSTR_CONNECTION);
	fprintf(fh, "\t\t<%s>\n", EMITT_XML_CSTR_CONNECTION_DATA);
	fprintf(fh, "\t\t\t<%s>fake_hardware</%s>\n", EMITT_XML_CSTR_HARDWARE_NAME, EMITT_XML_CSTR_HARDWARE_NAME);
	fprintf(fh, "\t\t\t<%s>fake_device@1</%s>\n", EMITT_XML_CSTR_DEVICE_NAME, EMITT_XML_CSTR_DEVICE_NAME);
	fprintf(fh, "\t\t\t<%s>some_connection_path</%s>\n", EMITT_XML_CSTR_CONNECTION_PATH, EMITT_XML_CSTR_CONNECTION_PATH);
	fprintf(fh, "\t\t\t<%s>some_connection_path</%s>\n", EMITT_XML_CSTR_CONNECTION_PATH_BASE, EMITT_XML_CSTR_CONNECTION_PATH_BASE);
	fprintf(fh, "\t\t\t<%s>dut:uniphy_inst|dut_if0:if0|dut_if0_dmaster:dmaster</%s>\n", EMITT_XML_CSTR_HIERARCHY_PATH, EMITT_XML_CSTR_HIERARCHY_PATH);
	fprintf(fh, "\t\t\t<%s>EMITT_CONNECTION_TYPE_ENUM::EMIF_120</%s>\n", EMITT_XML_CSTR_CONNECTION_TYPE, EMITT_XML_CSTR_CONNECTION_TYPE);
	fprintf(fh, "\t\t\t<%s>EMITT_CONNECTION_SLD_TYPE_ENUM::MASTER</%s>\n", EMITT_XML_CSTR_SLD_TYPE, EMITT_XML_CSTR_SLD_TYPE);
	fprintf(fh, "\t\t</%s>\n", EMITT_XML_CSTR_CONNECTION_DATA);

	fprintf(fh, "\t\t<%s>\n", EMITT_XML_CSTR_EMULATION_DATA);

	for(i = 0; i < 10; i++)
	{
		fprintf(fh, "\t\t\t<%s %s = \"0x%lx\">0x%lx</%s>\n", EMITT_XML_CSTR_EMU_DATA, EMITT_XML_CSTR_EMU_ADDRESS, BASE_REG_FILE + i*4, IORD_32DIRECT(BASE_REG_FILE, i*4), EMITT_XML_CSTR_EMU_DATA);
	}


	base_data = (alt_u32*)debug_data;
	for(i = 0; i < debug_data->data_size/4; i++)
	{
		fprintf(fh, "\t\t\t<%s %s = \"0x%lx\">0x%lx</%s>\n", EMITT_XML_CSTR_EMU_DATA, EMITT_XML_CSTR_EMU_ADDRESS, &base_data[i], base_data[i], EMITT_XML_CSTR_EMU_DATA);
	}

	fprintf(fh, "\t\t</%s>\n", EMITT_XML_CSTR_EMULATION_DATA);

	fprintf(fh, "\t</%s>\n", EMITT_XML_CSTR_CONNECTION);
	fprintf(fh, "</%s>\n", EMITT_XML_CSTR_CONNECTIONS);
	fclose(fh);

}

void tclrpt_populate_fake_margin_data(void)
{
  alt_u32 write_group, dq, dm, sr;


  for (sr = 0; sr < NUM_SHADOW_REGS; sr++) {
	for (write_group = 0; write_group < RW_MGR_MEM_IF_WRITE_DQS_WIDTH; write_group++)
	{
		for (dm = 0; dm < RW_MGR_NUM_TRUE_DM_PER_WRITE_GROUP; dm++)
		{
			TCLRPT_SET(debug_margin_report->margin_dm_margins[sr][write_group][dm].max_working_setting, debug_cal_report->cal_dm_margins[sr][write_group][dm].left_edge);
			TCLRPT_SET(debug_margin_report->margin_dm_margins[sr][write_group][dm].min_working_setting, debug_cal_report->cal_dm_margins[sr][write_group][dm].right_edge);
		}
	}
	for (dq = 0; dq < RW_MGR_MEM_DATA_WIDTH; dq++)
	{
		TCLRPT_SET(debug_margin_report->margin_dq_in_margins[sr][dq].max_working_setting, debug_cal_report->cal_dq_in_margins[sr][dq].left_edge);
		TCLRPT_SET(debug_margin_report->margin_dq_in_margins[sr][dq].min_working_setting, debug_cal_report->cal_dq_in_margins[sr][dq].right_edge);
	}
	for (dq = 0; dq < RW_MGR_MEM_DATA_WIDTH; dq++)
	{
		TCLRPT_SET(debug_margin_report->margin_dq_out_margins[sr][dq].max_working_setting, debug_cal_report->cal_dq_out_margins[sr][dq].left_edge);
		TCLRPT_SET(debug_margin_report->margin_dq_out_margins[sr][dq].min_working_setting, debug_cal_report->cal_dq_out_margins[sr][dq].right_edge);
	}
  }
}
#endif


#endif // ENABLE_TCL_DEBUG
