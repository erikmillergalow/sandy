#include <stdio.h>
#define SOKOL_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_GLCORE
#define SOKOL_DEBUG

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "basic.glsl.h"

// nice that we can define shaders inline - uncertain if this works
// across all platforms

// vertex Shader
static const char* vs_source = 
    "#version 330\n"
    "in vec3 position;\n"
    "in vec3 aColor;\n"
    "in vec2 aTexCoord;\n"
    "\n"
    "out vec3 ourColor;\n"
    "out vec2 TexCoord;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = vec4(position.x, -position.y, position.z, 1.0);\n"
    "    ourColor = aColor;\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

// fragment Shader
static const char* fs_source = 
    "#version 330\n"
    "out vec4 FragColor;\n"
    "\n"
    "in vec3 ourColor;\n"
    "in vec2 TexCoord;\n"
    "\n"
    "uniform sampler2D ourTexture;\n"
    "\n"
    "void main() {\n"
    "    FragColor = texture(ourTexture, TexCoord);\n"
    "}\n";

sg_pass_action pass_action;
/*
    sg_bindings

    The sg_bindings structure defines the resource binding slots
    of the sokol_gfx render pipeline, used as argument to the
    sg_apply_bindings() function.

    A resource binding struct contains:

    - 1..N vertex buffers
    - 0..N vertex buffer offsets
    - 0..1 index buffers
    - 0..1 index buffer offsets
    - 0..N vertex shader stage images
    - 0..N vertex shader stage samplers
    - 0..N vertex shader storage buffers
    - 0..N fragment shader stage images
    - 0..N fragment shader stage samplers
    - 0..N fragment shader storage buffers

    For the max number of bindings, see the constant definitions:

    - SG_MAX_VERTEX_BUFFERS
    - SG_MAX_SHADERSTAGE_IMAGES
    - SG_MAX_SHADERSTAGE_SAMPLERS
    - SG_MAX_SHADERSTAGE_STORAGEBUFFERS

    The optional buffer offsets can be used to put different unrelated
    chunks of vertex- and/or index-data into the same buffer objects.
*/
sg_bindings bind;
sg_pipeline pip;

size_t canvas_width = 800;
size_t canvas_height = 600;

int *canvas; // current world state
int *next_canvas; // next world state
int *updated; // track updated cells
uint8_t *world_pixels; // colors to send to texture for rendering

size_t step = 0;

enum {
    EMPTY,
    WALL,
    SAND,
    WATER,
};

float draw_position[2];
bool drawPrimary = false;
bool drawSecondary = false;
int primaryMaterial = SAND;
int secondaryMaterial = WATER;
/* int placement_radius = 10; */
int placement_radius = 10;

void place_material(int material, int radius) {
    for (int i = -placement_radius / 2; i < placement_radius / 2; i++) {
        size_t x = draw_position[0] + i;
        for (int j = -placement_radius / 2; j < placement_radius / 2; j++) {
            size_t y = draw_position[1] + j;
            if (x < canvas_width && y < canvas_height) {
                int index = x + canvas_width * y;
                canvas[index] = material;
            }
        }
    }
}

void draw_material(int index, int material) {
    if (material == EMPTY) {
        world_pixels[index] = 0;
        world_pixels[index + 1] = 0;
        world_pixels[index + 2] = 0;
        world_pixels[index + 3] = 0;
    } else if (material == WALL) {
        world_pixels[index] = 170;
        world_pixels[index + 1] = 170;
        world_pixels[index + 2] = 170;
        world_pixels[index + 3] = 255;
    } else if (material == SAND) {
        world_pixels[index] = 240;
        world_pixels[index + 1] = 200;
        world_pixels[index + 2] = 60;
        world_pixels[index + 3] = 255;
    } else if (material == WATER) {
        world_pixels[index] = 30;
        world_pixels[index + 1] = 90;
        world_pixels[index + 2] = 220;
        world_pixels[index + 3] = 255;
    }
}


sg_image_desc canvas_desc;
sg_image canvas_texture;

void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    // bind texture vertices 
    float vertices[] = {
        // positions         // colors           // texture coords
        1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
        -1.0f,  1.0f, 0.0f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
    };
    bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
        .label = "quad-vertices"
    });

    // bind texture indices
    uint16_t indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(indices),
        .data = SG_RANGE(indices),
        .label = "quad-indices"
    });

    /* sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend())); */
    sg_shader_desc shader_desc = {
        .vs.source = vs_source,
        .fs.source = fs_source,
        .fs.images[0] = { .image_type = SG_IMAGETYPE_2D }
    };
    sg_shader shader = sg_make_shader(&shader_desc);
    pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shader,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT3,
                [1].format = SG_VERTEXFORMAT_FLOAT3,
                [2].format = SG_VERTEXFORMAT_FLOAT2,
            }
        },
        .colors[0] = {
            .blend = {
                .enabled = false
            }
        },
        .label = "triangle-pipeline"
    });

    // not completely sure why this isn't needed anymore, must be something
    // to do with simple_shader_desc not setting .fs.images[0].image_type?
    //
    // fs stands for fragment shader, NOT file system
    // bind texture to fragment shader, set sampling options
    /* bind.fs.images[0] = canvas_texture; */
    /* bind.fs.samplers[0] = sg_make_sampler(&(sg_sampler_desc){ */
    /*     .min_filter = SG_FILTER_LINEAR, */
    /*     .mag_filter = SG_FILTER_LINEAR, */
    /*     .wrap_u = SG_WRAP_CLAMP_TO_EDGE, */
    /*     .wrap_v = SG_WRAP_CLAMP_TO_EDGE */
    /* }); */

    // clear framebuffer
    pass_action = (sg_pass_action) {
        .colors[0] = { 
            .load_action=SG_LOADACTION_CLEAR,
            .clear_value={0.0f, 0.0f, 0.0f, 1.0f}
        }
    };

    // initialize world
    canvas = malloc(canvas_width * canvas_height * sizeof(int));
    next_canvas = malloc(canvas_width * canvas_height * sizeof(int));
    updated = malloc(canvas_width * canvas_height * sizeof(int));
    world_pixels = malloc(canvas_width * canvas_height * 4 * sizeof(uint8_t));
    for (size_t i = 0; i < canvas_width * canvas_height; i++) {
        canvas[i] = 0;
        next_canvas[i] = 0;
        updated[i] = 0;
    }
    
    // initialize canvas texture
    canvas_desc = (sg_image_desc){
        .width = canvas_width,
        .height = canvas_height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM
    };
    canvas_texture = sg_make_image(&canvas_desc);
    if (!canvas_texture.id) {
        fprintf(stderr, "failed to create canvas texture\n");
    }
}

void processSand(int index) {
    if ((index + canvas_width + 1) < canvas_width * canvas_height) {
        int down_neighbor = canvas[index + canvas_width];
        int down_left_neighbor = canvas[index + (canvas_width - 1)];
        int down_right_neighbor = canvas[index + (canvas_width + 1)];

        if (down_left_neighbor == EMPTY && down_right_neighbor == EMPTY) {
            int random_fall = step % 2;
            if (!random_fall) {
                down_right_neighbor = WALL;
            } else {
                down_left_neighbor = WALL;
            }
        }

        if (down_neighbor == WALL) {
            next_canvas[index] = SAND;
            updated[index] = SAND;
        } else if (down_left_neighbor == EMPTY) {
            next_canvas[index] = EMPTY;
            next_canvas[index + canvas_width - 1] = SAND;
            updated[index + canvas_width - 1] = true;
        } else if (down_left_neighbor == EMPTY) {
            next_canvas[index] = EMPTY;
            next_canvas[index + canvas_width - 1] = SAND;
            updated[index + canvas_width - 1] = true;
        } else if (down_neighbor == EMPTY) {
            next_canvas[index] = EMPTY;
            next_canvas[index + canvas_width] = SAND;
            updated[index + canvas_width] = true;
        }
    }
}

void processWater(int index) {

}

void process_world() {
    // this should shuffle indices then loop randomly
    for (int i = 0; i < canvas_width * canvas_height; i++) {
        int material = canvas[i];

        if (!updated[i]) {
            if (material == EMPTY) {
                next_canvas[i] = EMPTY;
            } else if (material == WALL) {
                next_canvas[i] = WALL;
                updated[i] = true;
            } else if (material == SAND) {
                processSand(i);
            } else if (material == WATER) {
                processWater(i);
            }
        }
    }

    int *temp_canvas = canvas;
    canvas = next_canvas;
    next_canvas = canvas;

    for (int i = 0; i < canvas_width * canvas_height; i++) {
        updated[i] = false;
    }
}

void frame(void) {

    // handle user input
    if (drawPrimary) {
        place_material(primaryMaterial, placement_radius);
    } else if (drawSecondary) {
        place_material(secondaryMaterial, placement_radius);
    }

    // run simulation frame
    process_world();
    step++;

    size_t index = 0;
    for (size_t i = 0; i < canvas_width * canvas_height; i++) {
        /* draw_material(index, next_canvas[i]); */
        draw_material(index, canvas[i]);
        index += 4;
    }

    /* int *swap_canvas; */
    /* swap_canvas = canvas; */
    /* canvas = next_canvas; */
    /* next_canvas = canvas; */

    // send pixel color buffer to texture to render
    sg_image_data canvas_update = {
        .subimage[0][0] = {
            .ptr = world_pixels,
            .size = canvas_width * canvas_height * 4
        }
    };
    sg_update_image(canvas_texture, &canvas_update);

    // passes render into swapchain, presenting rendering result onto the screen
    sg_begin_pass(&(sg_pass){ .action = pass_action, .swapchain = sglue_swapchain() });

    sg_apply_pipeline(pip);
    sg_apply_bindings(&bind);
    sg_draw(0, 6, 1);
    
    /* __dbgui_draw(); */
    
    // finish rendering pass
    sg_end_pass();

    // done with current frame
    sg_commit();
}

void cleanup(void) {
    /* __dbgui_shutdown(); */
    free(canvas);
    free(next_canvas);
    free(updated);
    sg_shutdown();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }
    }

    if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        draw_position[0] = e->mouse_x;
        draw_position[1] = e->mouse_y;
        /* printf("x: %f\n", e->mouse_x); */
        /* printf("y: %f\n", e->mouse_y); */
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
