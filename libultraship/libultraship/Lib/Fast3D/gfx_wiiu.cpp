#ifdef __WIIU__

#include <stdio.h>
#include <time.h>

#include <coreinit/time.h>
#include <coreinit/thread.h>
#include <coreinit/foreground.h>

#include <gx2/state.h>
#include <gx2/context.h>
#include <gx2/display.h>
#include <gx2/event.h>
#include <gx2/swap.h>
#include <gx2/mem.h>
#include <gx2r/mem.h>

#include <whb/proc.h>
#include <proc_ui/procui.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include "PR/ultra64/gbi.h"

#include "gfx_window_manager_api.h"
#include "gfx_pc.h"
#include "gfx_gx2.h"
#include "gfx_heap.h"

bool has_foreground = false;
static void *command_buffer_pool = nullptr;
GX2ContextState *context_state = nullptr;

static GX2TVRenderMode tv_render_mode;
static void *tv_scan_buffer = nullptr;
static uint32_t tv_scan_buffer_size = 0;
uint32_t tv_width;
uint32_t tv_height;

static GX2DrcRenderMode drc_render_mode;
static void *drc_scan_buffer = nullptr;
static uint32_t drc_scan_buffer_size = 0;

static int frame_divisor = 1;

static void *gfx_wiiu_gx2r_alloc(GX2RResourceFlags flags, uint32_t size, uint32_t alignment) {
    // Color, depth, scan buffers all belong in MEM1
    if ((flags & (GX2R_RESOURCE_BIND_COLOR_BUFFER
                | GX2R_RESOURCE_BIND_DEPTH_BUFFER
                | GX2R_RESOURCE_BIND_SCAN_BUFFER
                | GX2R_RESOURCE_USAGE_FORCE_MEM1))
        && !(flags & GX2R_RESOURCE_USAGE_FORCE_MEM2)) {
        return GfxHeapAllocMEM1(size, alignment);
    } else {
        return GfxHeapAllocMEM2(size, alignment);
    }
}

static void gfx_wiiu_gx2r_free(GX2RResourceFlags flags, void *block) {
    if ((flags & (GX2R_RESOURCE_BIND_COLOR_BUFFER
                | GX2R_RESOURCE_BIND_DEPTH_BUFFER
                | GX2R_RESOURCE_BIND_SCAN_BUFFER
                | GX2R_RESOURCE_USAGE_FORCE_MEM1))
        && !(flags & GX2R_RESOURCE_USAGE_FORCE_MEM2)) {
        return GfxHeapFreeMEM1(block);
    } else {
        return GfxHeapFreeMEM2(block);
    }
}

static uint32_t gfx_wiiu_proc_callback_acquired(void *context) {
    has_foreground = true;

    assert(GfxHeapInitMEM1());
    assert(GfxHeapInitForeground());

    tv_scan_buffer = GfxHeapAllocForeground(tv_scan_buffer_size, GX2_SCAN_BUFFER_ALIGNMENT);
    assert(tv_scan_buffer);

    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, tv_scan_buffer, tv_scan_buffer_size);
    GX2SetTVBuffer(tv_scan_buffer, tv_scan_buffer_size, tv_render_mode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);

    drc_scan_buffer = GfxHeapAllocForeground(drc_scan_buffer_size, GX2_SCAN_BUFFER_ALIGNMENT);
    assert(drc_scan_buffer);

    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, drc_scan_buffer, drc_scan_buffer_size);
    GX2SetDRCBuffer(drc_scan_buffer, drc_scan_buffer_size, drc_render_mode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);

    return 0;
}

static uint32_t gfx_wiiu_proc_callback_released(void* context) {
    if (tv_scan_buffer) {
        GfxHeapFreeForeground(tv_scan_buffer);
        tv_scan_buffer = nullptr;
    }

    if (drc_scan_buffer) {
        GfxHeapFreeForeground(drc_scan_buffer);
        drc_scan_buffer = nullptr;
    }

    GfxHeapDestroyMEM1();
    GfxHeapDestroyForeground();

    has_foreground = false;

    return 0;
}

static void gfx_wiiu_init(const char *game_name, bool start_in_fullscreen) {
    WHBProcInit();

    command_buffer_pool = GfxHeapAllocMEM2(0x400000, GX2_COMMAND_BUFFER_ALIGNMENT);
    assert(command_buffer_pool);

    uint32_t initAttribs[] = {
        GX2_INIT_CMD_BUF_BASE, (uintptr_t) command_buffer_pool,
        GX2_INIT_CMD_BUF_POOL_SIZE, 0x400000,
        GX2_INIT_ARGC, 0,
        GX2_INIT_ARGV, 0,
        GX2_INIT_END
    };
    GX2Init(initAttribs);

    switch(GX2GetSystemTVScanMode()) {
    case GX2_TV_SCAN_MODE_480I:
    case GX2_TV_SCAN_MODE_480P:
        tv_render_mode = GX2_TV_RENDER_MODE_WIDE_480P;
        tv_width = 854;
        tv_height = 480;
        break;
    case GX2_TV_SCAN_MODE_1080I:
    case GX2_TV_SCAN_MODE_1080P:
        tv_render_mode = GX2_TV_RENDER_MODE_WIDE_1080P;
        tv_width = 1920;
        tv_height = 1080;
        break;
    case GX2_TV_SCAN_MODE_720P:
    default:
        tv_render_mode = GX2_TV_RENDER_MODE_WIDE_720P;
        tv_width = 1280;
        tv_height = 720;
        break;
    }

    drc_render_mode = GX2GetSystemDRCScanMode();

    uint32_t unk;
    GX2CalcTVSize(tv_render_mode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE, &tv_scan_buffer_size, &unk);
    GX2CalcDRCSize(drc_render_mode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE, &drc_scan_buffer_size, &unk);

    GX2RSetAllocator(&gfx_wiiu_gx2r_alloc, &gfx_wiiu_gx2r_free);

    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, gfx_wiiu_proc_callback_acquired, nullptr, 100);
    ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, gfx_wiiu_proc_callback_released, nullptr, 100);

    gfx_wiiu_proc_callback_acquired(nullptr);

    context_state = (GX2ContextState *) GfxHeapAllocMEM2(sizeof(GX2ContextState), GX2_CONTEXT_STATE_ALIGNMENT);
    assert(context_state);

    GX2SetupContextStateEx(context_state, TRUE);
    GX2SetContextState(context_state);

    GX2SetTVScale(tv_width, tv_height);
    GX2SetDRCScale(tv_width, tv_height);

    GX2SetSwapInterval(frame_divisor);

    gfx_current_dimensions.width = tv_width;
    gfx_current_dimensions.height = tv_height;
}

static void gfx_wiiu_shutdown(void) {
    if (has_foreground) {
        gfx_wiiu_proc_callback_released(nullptr);
    }

    GX2RSetAllocator(nullptr, nullptr);
    GX2Shutdown();

    if (context_state) {
        GfxHeapFreeMEM2(context_state);
        context_state = nullptr;
    }

    if (command_buffer_pool) {
        GfxHeapFreeMEM2(command_buffer_pool);
        command_buffer_pool = nullptr;
    }
}

void gfx_wiiu_set_context_state(void) {
    GX2SetContextState(context_state);
}

static void gfx_wiiu_set_fullscreen_changed_callback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
}

static void gfx_wiiu_set_fullscreen(bool enable) {
}

static void gfx_wiiu_show_cursor(bool hide) {
}

static void gfx_wiiu_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode), void (*on_all_keys_up)(void)) {
}

static void gfx_wiiu_main_loop(void (*run_one_game_iter)(void)) {
    while (WHBProcIsRunning()) {
        run_one_game_iter();
    }

    gfx_gx2_shutdown();
    gfx_wiiu_shutdown();
    WHBProcShutdown();
}

static void gfx_wiiu_get_dimensions(uint32_t *width, uint32_t *height) {
    *width = tv_width;
    *height = tv_height;
}

static void gfx_wiiu_handle_events(void) {
}

static bool gfx_wiiu_start_frame(void) {
    uint32_t swapCount, flipCount;
    OSTime lastFlip, lastVsync;
    uint32_t waitCount = 0;

    while (true) {
        GX2GetSwapStatus(&swapCount, &flipCount, &lastFlip, &lastVsync);

        if (flipCount >= swapCount) {
            break;
        }

        if (waitCount >= 10) {
            // GPU timed out, drop frame
            return false;
        }

        waitCount++;
        GX2WaitForVsync();
    }

    return true;
}

static void gfx_wiiu_swap_buffers_begin(void) {
    GX2SwapScanBuffers();
    GX2Flush();

    gfx_wiiu_set_context_state();

    GX2SetTVEnable(TRUE);
    GX2SetDRCEnable(TRUE);
}

static void gfx_wiiu_swap_buffers_end(void) {
}

static double gfx_wiiu_get_time(void) {
    return 0.0;
}

static void gfx_wiiu_set_framedivisor(int divisor)
{
    if (frame_divisor != divisor) {
        GX2SetSwapInterval(divisor);
        frame_divisor = divisor;
    }
}

struct GfxWindowManagerAPI gfx_wiiu = {
    gfx_wiiu_init,
    gfx_wiiu_set_keyboard_callbacks,
    gfx_wiiu_set_fullscreen_changed_callback,
    gfx_wiiu_set_fullscreen,
    gfx_wiiu_show_cursor,
    gfx_wiiu_main_loop,
    gfx_wiiu_get_dimensions,
    gfx_wiiu_handle_events,
    gfx_wiiu_start_frame,
    gfx_wiiu_swap_buffers_begin,
    gfx_wiiu_swap_buffers_end,
    gfx_wiiu_get_time,
    gfx_wiiu_set_framedivisor
};

#endif
