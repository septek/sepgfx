#ifndef SF_WINDOW_H
#define SF_WINDOW_H

#include <sf/math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "sf/gfx/camera.h"
#include "sf/gfx/key.h"
#include "export.h"
#include "meshes.h"

#define SF_WINDOW_RESIZABLE     0b10000000
#define SF_WINDOW_VISIBLE       0b01000000
#define SF_WINDOW_MAXIMIZED     0b00100000
#define SF_WINDOW_FULLSCREEN    0b00010000

/// A window with an active OpenGL context and keyboard controls.
typedef struct {
    GLFWwindow *handle;
    uint8_t hints;
    sf_str title;
    sf_vec2 size;
    sf_vec2 mouse_position;

    sf_camera *camera;
    sf_mesh fb_mesh;

    int8_t keyboard[GLFW_KEY_LAST + 1];
    uint8_t kb_p;
    char keyboard_string[UINT8_MAX];
} sf_window;

/// Construct and open a new OpenGL window.
EXPORT sf_result sf_window_new(sf_window **out, sf_str title, sf_vec2 size, sf_camera *camera, uint8_t hints);
/// Free a window and its resources.
EXPORT void sf_window_close(sf_window *window);

/// Check is a key is currently pressed down.
static inline bool sf_key_check(const sf_window *window, const sf_key key)    { return window->keyboard[key] > 0;                }
/// Check if a key was pressed on this frame.
static inline bool sf_key_pressed(const sf_window *window, const sf_key key)  { return window->keyboard[key] == SF_KEY_PRESSED; }
/// Check if a key was released on this frame.
static inline bool sf_key_released(const sf_window *window, const sf_key key) { return window->keyboard[key] == SF_KEY_RELEASED; }
/// Get the string of keys pressed since the last time this function was called.
sf_str sf_key_string(sf_window *window);

/// Update the camera the window is rendering from.
EXPORT void sf_window_set_camera(sf_window *window, sf_camera *camera);
/// Prepare for a frame, and/or return whether a window should close.
/// Use this in a while loop.
EXPORT bool sf_window_loop(const sf_window *window);
/// Swap a window's buffers and finish the frame.
EXPORT sf_result sf_window_draw(sf_window *window, sf_shader *post_shader);

/// Set the displayed title of a window.
EXPORT void sf_window_set_title(sf_window *window, const sf_str title);
/// Set the displayed size of a window.
EXPORT void sf_window_set_size(sf_window *window, sf_vec2 size);
/// Update the properties of a window. Only some hints can be changed after the window is created.
EXPORT void sf_window_update_hints(sf_window *window, uint8_t hints);

#endif // SF_WINDOW_H
