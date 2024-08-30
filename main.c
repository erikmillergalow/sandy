#include <stdio.h>
#define SOKOL_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_GLCORE

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_log.h"

sg_pass_action pass_action;

size_t canvas_width = 800;
size_t canvas_height = 600;

size_t *canvas;
size_t *nextCanvas;
size_t *updated;

enum {
    EMPTY,
    WALL,
    SAND,
    WATER,
};

bool drawPrimary = false;
bool drawSecondary = false;

void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f }
        }
    };
}

void frame(void) {
    float g = pass_action.colors[0].clear_value.g + 0.01f;
    pass_action.colors[0].clear_value.g = (g > 1.0f) ? 0.0f : g;

    // update canvas


    // passes render into swapchain, presenting rendering result onto the screen
    sg_begin_pass(&(sg_pass){ .action = pass_action, .swapchain = sglue_swapchain() });
    
    /* __dbgui_draw(); */
    
    // finish rendering pass
    sg_end_pass();

    // done with current frame
    sg_commit();
}

void cleanup(void) {
    /* __dbgui_shutdown(); */
    sg_shutdown();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }
    }

    if (e->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
        if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
            drawPrimary = true;
            printf("x: %f\n", e->mouse_x);
            printf("y: %f\n", e->mouse_y);
        }
        if (e->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
            drawSecondary = true;
        }
    }

    if (e->type == SAPP_EVENTTYPE_MOUSE_UP) {
        if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
            drawPrimary = false;
        }
        if (e->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
            drawSecondary = false;
        }
    }
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        /* event_cb = __dbgui_event, */
        .width = canvas_width,
        .height = canvas_height,
        .window_title = "sandy",
        .logger.func = slog_func,
    };
}
