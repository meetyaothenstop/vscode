#ifndef POWER_EVENT_DIAG_H
#define POWER_EVENT_DIAG_H
#include <stdint.h>
#include "power_build_config.h"
#if POWER_EVENT_DIAGNOSTICS
enum {
    POWER_ADC_REASON_JEOS_MISSING=1U,
    POWER_ADC_REASON_JQOVF=2U,
    POWER_ADC_REASON_LATE_INVALID=4U
};
enum {
    POWER_EVENT_UI_ENTRY=1, POWER_EVENT_UI_KEYS, POWER_EVENT_UI_POLL,
    POWER_EVENT_UI_WAIT_DMA, POWER_EVENT_UI_STARTUP_MUTED, POWER_EVENT_UI_WAIT_REFRESH,
    POWER_EVENT_UI_RENDER, POWER_EVENT_UI_FRAME_READY, POWER_EVENT_UI_SUBMIT_PAGE,
    POWER_EVENT_UI_PAGE_QUEUED, POWER_EVENT_UI_PAGE_BUSY, POWER_EVENT_UI_IO_ERROR,
    POWER_EVENT_UI_IDLE, POWER_EVENT_UI_FRAME_DONE
};
typedef struct {
    uint32_t calls, entry_cycles, entry_tick, gap_cycles_max;
    uint32_t stage, stage_cycles, stage_tick, page, io_status;
    uint32_t page_submissions, page_cycles, page_tick, last_page;
    uint32_t entry_samples, stage_samples, page_samples, gap_samples_max;
    uint32_t frames_completed, frame_cycles, frame_tick, frame_samples;
} PowerEventProgress;
typedef struct {
    PowerEventProgress slots[2];
    uint32_t active; /* DMB then atomic index publication; foreground sole writer. */
} PowerEventMailbox;
typedef struct {
    uint32_t valid, code, detail, state, mode, samples, run_id;
    uint32_t capture_cycles, capture_tick, clock_ready, run_age_cycles, run_age_ticks;
    uint32_t start_pending, bootstrap_resume, bootstrap_remaining, arming_remaining;
    uint32_t resume_pending, polarity_samples;
    float target_volts, duty, reference_volts;
    uint32_t left_compare, right_compare;
    PowerEventProgress foreground, ui;
    uint32_t foreground_age_cycles, foreground_age_ticks;
    uint32_t ui_entry_age_cycles, ui_entry_age_ticks;
    uint32_t ui_progress_age_cycles, ui_progress_age_ticks;
    uint32_t ui_page_age_cycles, ui_page_age_ticks;
    uint32_t run_age_samples, foreground_age_samples, ui_entry_age_samples;
    uint32_t ui_progress_age_samples, ui_page_age_samples;
    uint32_t ui_frame_age_cycles, ui_frame_age_ticks, ui_frame_age_samples;
    /* Exact cached IRQ-entry flags, before W1C clearing. Only ADC faults
     * carry a nonzero reason; masks1 missing JEOS,2 JQOVF,4 late ADC2. */
    uint32_t adc1_entry_flags, adc2_entry_flags, adc_reason;
} PowerEventFault;
typedef struct {
    uint32_t version, cpu_hz, clock_ready, run_id, run_start_cycles, run_start_tick;
    uint32_t sample_hz, run_start_samples;
    PowerEventMailbox foreground, ui;
    PowerEventFault fault;
} PowerEventDiagnostics;
extern volatile PowerEventDiagnostics power_event;
/* Foreground-only. No interrupt mask; timestamps are adjacent reads and may
 * span preemption. DWT ages/gaps are modulo32 bits; ticks are a separate clock. */
void PowerEvent_UIMark(uint32_t stage, uint32_t page, uint32_t status);
#endif
#endif
