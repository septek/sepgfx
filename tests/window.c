#include "sf/fs.h"
#include "sf/gfx/camera.h"
#include "sf/gfx/meshes.h"
#include "sf/gfx/textures.h"
#include "sf/math.h"
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sf/gfx/window.h>
#include <stdio.h>

int main(int argc, char **argv) {
    sf_vec2 window_size = {1280, 720};
    if (argc == 3) {
        char *end;
        long value = strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || errno == ERANGE || value < 0 || value > 9999) {
            fprintf(stderr, "Width '%s' is invalid.\n", argv[1]);
            return -1;
        }
        window_size.x = value;

        value = strtol(argv[2], &end, 10);
        if (end == argv[1] || *end != '\0' || errno == ERANGE || value < 0 || value > 9999) {
            fprintf(stderr, "Height '%s' is invalid.\n", argv[1]);
            return -1;
        }
        window_size.y = value;
    } else if (argc != 1) {
        fprintf(stderr, "Args: width (0-9999), height (0-9999)\n");
        return -1;
    }

    // You should allocate cameras on the heap, or at the very least try to keep them in the same
    // place on the stack during the lifetime of a window. You can update it, though. (see sf_window_set_camera)
    sf_camera *main_cam = calloc(1, sizeof(sf_camera));
    *main_cam = sf_camera_new(SF_CAMERA_PERSPECTIVE, 90, 0.1f, 100.0f);
    assert(main_cam);
    sf_window_ex wx = sf_window_new(sf_lit("Cool Window"), window_size,
        main_cam, SF_WINDOW_VISIBLE | SF_WINDOW_RESIZABLE);
    if (!wx.is_ok) {
        // You can process each error case if you want.
        switch (wx.value.err) {
            case SF_GLFW_INIT_FAILED: fprintf(stderr, "GLFW failed to initialize\n"); break;
            case SF_GLFW_CREATE_FAILED: fprintf(stderr, "GLFW failed to create the window\n"); break;
            case SF_GLAD_INIT_FAILED: fprintf(stderr, "GLAD failed to initialize\n"); break;
            default: fprintf(stderr, "Unexpected window failure\n"); break;
        }
        return -1;
    }
    sf_window *win = wx.value.ok;

    sf_shader_ex sx = sf_shader_new(sf_lit("sample_shaders/default"));
    if (!sx.is_ok) {
        switch (sx.value.err.type) {
            case SF_SHADER_COMPILE_ERROR: fprintf(stderr, "Default shader failed to compile: %s\n", sx.value.err.compile_err.c_str); break;
            case SF_SHADER_NOT_FOUND: fprintf(stderr, "Default shader missing?\n"); break;
            default: fprintf(stderr, "Unexpected shader failure.\n"); break;
        }
        return -1;
    }
    sf_shader def = sx.value.ok;

    sf_mesh box = sf_mesh_new();
    sf_mesh_add_vertices(&box, (sf_vertex[]){
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, sf_rgbagl(SF_WHITE)},
        {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, sf_rgbagl(SF_WHITE)},
        {{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, sf_rgbagl(SF_WHITE)},

        {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, sf_rgbagl(SF_WHITE)},
        {{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, sf_rgbagl(SF_WHITE)},
        {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, sf_rgbagl(SF_WHITE)},
    }, 6);

    sf_texture_ex dx = sf_texture_load(sf_lit("doom.png"));
    if (!dx.is_ok) {
        switch (dx.value.err) {
            case SF_FILE_NOT_FOUND: fprintf(stderr, "No DOOM? :(\n"); break;
            case SF_READ_FAILURE: fprintf(stderr, "Your DOOM is corrupted.\n"); break;
            default: fprintf(stderr, "Critical DOOM failure.\n"); break;
        }
        return -1;
    }
    sf_texture doom = dx.value.ok;

    main_cam->transform.position = (sf_vec3){0, 0, 10};

    sf_transform identity = SF_TRANSFORM_IDENTITY;
    identity.scale = (sf_vec3){5, 5, 5};
    while (sf_window_loop(win)) {
        sf_vec3 input = {
            sf_key_check(win, SF_KEY_RIGHT_ARROW) - sf_key_check(win, SF_KEY_LEFT_ARROW),
            sf_key_check(win, SF_KEY_SPACE) - sf_key_check(win, SF_KEY_LEFT_CONTROL),
            sf_key_check(win, SF_KEY_DOWN_ARROW) - sf_key_check(win, SF_KEY_UP_ARROW)
        };
        main_cam->transform.position = sf_vec3_add(main_cam->transform.position, sf_vec3_multf(input, 0.1f));

        sf_draw_ex d = sf_mesh_draw(&box, &def, main_cam, identity, &doom);
        if (!d.is_ok) {
            switch (d.value.err.type) {
                case SF_DRAW_UNKNOWN_UNIFORM: fprintf(stderr, "[Draw] Unknown uniform: '%s'\n", d.value.err.value.uniform_name.c_str); break;
                case SF_DRAW_SHADER_MISSING: fprintf(stderr, "[Draw] How\n"); break;
            }
        }
        d = sf_window_draw(win, &def);
        if (!d.is_ok) {
            switch (d.value.err.type) {
                case SF_DRAW_UNKNOWN_UNIFORM: fprintf(stderr, "[Draw] Unknown uniform: '%s'\n", d.value.err.value.uniform_name.c_str); break;
                case SF_DRAW_SHADER_MISSING: fprintf(stderr, "[Draw] How\n"); break;
            }
        }
    }

    sf_shader_free(&def);
    sf_texture_delete(&doom);
    sf_mesh_delete(&box);
    sf_window_close(win);
    sf_camera_delete(main_cam);
    free(main_cam);
}
