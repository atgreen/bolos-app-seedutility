/*
 * License for the BOLOS Seed Utility Application project, originally found
 * here: https://github.com/parkerhoyes/bolos-app-seedutility
 *
 * Copyright (C) 2017 Parker Hoyes <contact@parkerhoyes.com>
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim
 *    that you wrote the original software. If you use this software in a
 *    product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "app_rooms.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bui.h"
#include "bui_font.h"
#include "bui_menu.h"
#include "bui_room.h"

#include "app.h"

#define APP_ROOM_MAIN_ACTIVE (*((app_room_main_active_t*) app_room_ctx.frame_ptr))

//----------------------------------------------------------------------------//
//                                                                            //
//                  Internal Type Declarations & Definitions                  //
//                                                                            //
//----------------------------------------------------------------------------//

typedef struct app_room_main_active_t {
	bui_menu_menu_t menu;
} app_room_main_active_t;

typedef struct app_room_main_inactive_t {
	// The index of the focused menu element
	uint8_t focus;
} app_room_main_inactive_t;

//----------------------------------------------------------------------------//
//                                                                            //
//                Internal Variable Declarations & Definitions                //
//                                                                            //
//----------------------------------------------------------------------------//

static const uint8_t app_room_main_bmp_icon_bb[] = {
	0x00, 0x00, 0x0F, 0xF0, 0x3E, 0x7C, 0x3E, 0x7C,
	0x78, 0x7E, 0x72, 0x7E, 0x64, 0x7E, 0x68, 0x1E,
	0x62, 0x4E, 0x7E, 0x26, 0x7E, 0x16, 0x7E, 0x46,
	0x3E, 0x7C, 0x3E, 0x7C, 0x0F, 0xF0, 0x00, 0x00,
};
static const uint32_t app_room_main_bmp_icon_plt[] = {
	BUI_CLR_BLACK,
	BUI_CLR_WHITE,
};
#define APP_ROOM_MAIN_BMP_ICON \
		((bui_const_bitmap_t) { \
			.w = 16, \
			.h = 16, \
			.bb = app_room_main_bmp_icon_bb, \
			.plt = app_room_main_bmp_icon_plt, \
			.bpp = 1, \
		})

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_main_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event);

static void app_room_main_enter(bool up);
static void app_room_main_exit(bool up);
static void app_room_main_draw();
static void app_room_main_time_elapsed(uint32_t elapsed);
static void app_room_main_button_clicked(bui_button_id_t button);

static uint8_t app_room_main_elem_size(const bui_menu_menu_t *menu, uint8_t i);
static void app_room_main_elem_draw(const bui_menu_menu_t *menu, uint8_t i, bui_ctx_t *bui_ctx, int16_t y);

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

const bui_room_t app_rooms_main = {
	.event_handler = app_room_main_handle_event,
};

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_main_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event) {
	switch (event->id) {
	case BUI_ROOM_EVENT_ENTER: {
		bool up = BUI_ROOM_EVENT_DATA_ENTER(event)->up;
		app_room_main_enter(up);
	} break;
	case BUI_ROOM_EVENT_EXIT: {
		bool up = BUI_ROOM_EVENT_DATA_EXIT(event)->up;
		app_room_main_exit(up);
	} break;
	case BUI_ROOM_EVENT_DRAW: {
		app_room_main_draw();
	} break;
	case BUI_ROOM_EVENT_FORWARD: {
		const bui_event_t *bui_event = BUI_ROOM_EVENT_DATA_FORWARD(event);
		switch (bui_event->id) {
		case BUI_EVENT_TIME_ELAPSED: {
			uint32_t elapsed = BUI_EVENT_DATA_TIME_ELAPSED(bui_event)->elapsed;
			app_room_main_time_elapsed(elapsed);
		} break;
		case BUI_EVENT_BUTTON_CLICKED: {
			bui_button_id_t button = BUI_EVENT_DATA_BUTTON_CLICKED(bui_event)->button;
			app_room_main_button_clicked(button);
		} break;
		// Other events are acknowledged
		default:
			break;
		}
	} break;
	}
}

static void app_room_main_enter(bool up) {
	app_room_main_inactive_t inactive;
	if (up)
		inactive.focus = 0;
	else
		bui_room_pop(&app_room_ctx, &inactive, sizeof(inactive));
	bui_room_alloc(&app_room_ctx, sizeof(app_room_main_active_t));
	APP_ROOM_MAIN_ACTIVE.menu.elem_size_callback = app_room_main_elem_size;
	APP_ROOM_MAIN_ACTIVE.menu.elem_draw_callback = app_room_main_elem_draw;
	bui_menu_init(&APP_ROOM_MAIN_ACTIVE.menu, 4, inactive.focus, true);
	app_disp_invalidate();
}

static void app_room_main_exit(bool up) {
	if (!up)
		os_sched_exit(0); // Go back to the dashboard
	app_room_main_inactive_t inactive;
	inactive.focus = bui_menu_get_focused(&APP_ROOM_MAIN_ACTIVE.menu);
	bui_room_dealloc(&app_room_ctx, sizeof(app_room_main_active_t));
	bui_room_push(&app_room_ctx, &inactive, sizeof(inactive));
}

static void app_room_main_draw() {
	bui_menu_draw(&APP_ROOM_MAIN_ACTIVE.menu, &app_bui_ctx);
}

static void app_room_main_time_elapsed(uint32_t elapsed) {
	if (bui_menu_animate(&APP_ROOM_MAIN_ACTIVE.menu, elapsed))
		app_disp_invalidate();
}

static void app_room_main_button_clicked(bui_button_id_t button) {
	switch (button) {
	case BUI_BUTTON_NANOS_BOTH:
		switch (bui_menu_get_focused(&APP_ROOM_MAIN_ACTIVE.menu)) {
		case 1:
			bui_room_enter(&app_room_ctx, &app_rooms_verifybackup, NULL, 0);
			break;
		case 2:
			bui_room_enter(&app_room_ctx, &app_rooms_about, NULL, 0);
			break;
		case 3:
			bui_room_exit(&app_room_ctx);
			break;
		}
		break;
	case BUI_BUTTON_NANOS_LEFT:
		bui_menu_scroll(&APP_ROOM_MAIN_ACTIVE.menu, true);
		app_disp_invalidate();
		break;
	case BUI_BUTTON_NANOS_RIGHT:
		bui_menu_scroll(&APP_ROOM_MAIN_ACTIVE.menu, false);
		app_disp_invalidate();
		break;
	}
}

static uint8_t app_room_main_elem_size(const bui_menu_menu_t *menu, uint8_t i) {
	switch (i) {
		case 0: return 20;
		case 1: return 15;
		case 2: return 15;
		case 3: return 18;
	}
	// Impossible case
	return 0;
}

static void app_room_main_elem_draw(const bui_menu_menu_t *menu, uint8_t i, bui_ctx_t *bui_ctx, int16_t y) {
	switch (i) {
	case 0:
		bui_ctx_draw_bitmap_full(&app_bui_ctx, APP_ROOM_MAIN_BMP_ICON, 8, y + 2);
		bui_font_draw_string(&app_bui_ctx, "Seed Utility", 32, y + 10, BUI_DIR_LEFT, bui_font_open_sans_extrabold_11);
		break;
	case 1:
		bui_font_draw_string(&app_bui_ctx, "Verify Backup", 64, y + 2, BUI_DIR_TOP, bui_font_open_sans_extrabold_11);
		break;
	case 2:
		bui_font_draw_string(&app_bui_ctx, "About", 64, y + 2, BUI_DIR_TOP, bui_font_open_sans_extrabold_11);
		break;
	case 3:
		bui_ctx_draw_bitmap_full(&app_bui_ctx, BUI_BMP_BADGE_DASHBOARD, 29, y + 2);
		bui_font_draw_string(&app_bui_ctx, "Quit app", 52, y + 9, BUI_DIR_LEFT, bui_font_open_sans_extrabold_11);
		break;
	}
}
