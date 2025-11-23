#include <sf/dynamic.h>
#include "sf/window.h"

#include "sf/shaders.h"

void sf_cb_err(const int error_code, const char *error_string) {
    fprintf(stderr, "OpenGL Error %d: '%s.'", error_code, error_string);
}

void sf_cb_key(GLFWwindow* window, const int key, [[maybe_unused]] int scancode, const int action, [[maybe_unused]] int mods) {
    sf_window *win = glfwGetWindowUserPointer(window);
    switch (action) {
        case GLFW_RELEASE: win->keyboard[key] = SF_KEY_RELEASED; break;
        case GLFW_PRESS: win->keyboard[key] = SF_KEY_PRESSED; break;
        default: break;
    }
}

void sf_cb_char(GLFWwindow* window, const unsigned int codepoint) {
    sf_window *win = glfwGetWindowUserPointer(window);
    if (codepoint <= 127) {
        if (win->kb_p == UINT8_MAX - 1)
            memcpy(win->keyboard_string, win->keyboard_string + 1, UINT8_MAX - 2);
        win->keyboard_string[win->kb_p] = (char)codepoint;
        if (win->kb_p < UINT8_MAX - 1) win->kb_p++;
    }
}

void sf_cb_resize(GLFWwindow* window, const int width, const int height) {
    sf_window *win = glfwGetWindowUserPointer(window);
    win->size = (sf_vec2){(float)width, (float)height};
    sf_window_set_camera(win, win->camera);

    if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
        win->hints |= SF_WINDOW_MAXIMIZED;
    else
        win->hints &= ~SF_WINDOW_MAXIMIZED;
}

void APIENTRY sf_gl_dbglog(GLuint source, GLuint type, GLuint id, GLuint severity,
    [[maybe_unused]] GLsizei length, const GLchar *message, [[maybe_unused]] const void *userParam) {
    printf("[OpenGL] (Source %u) (Type %u) (ID %u), (Severity %u) \"%s\"\n", source, type, id, severity, message);
}

sf_result sf_window_new(sf_window **out, const sf_str title, const sf_vec2 size, sf_camera *camera, const uint8_t hints) {
    *out = sf_calloc(1, sizeof(sf_window));
    memcpy(*out, &(sf_window) {
        .title = sf_str_dup(title),
        .size = size,
        .mouse_position = { 0, 0 },
        .hints = hints,
    }, sizeof(sf_window));
    sf_window *win = *out;

    //TODO: GLFW Init
    if (!glfwInit()) {
        glfwTerminate();
        return sf_err(sf_lit("GLFW Failed to initialize."));
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, (hints & SF_WINDOW_RESIZABLE) == SF_WINDOW_RESIZABLE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    if (!((win->handle = glfwCreateWindow((int)size.x, (int)size.y, title.c_str, nullptr, nullptr))))
        return sf_err(sf_lit("GLFW Failed to open the window."));

    glfwSetWindowUserPointer(win->handle, win); // Point to myself
    glfwSetErrorCallback(sf_cb_err);
    glfwSetKeyCallback(win->handle, sf_cb_key);
    glfwSetCharCallback(win->handle, sf_cb_char);
    glfwSetFramebufferSizeCallback(win->handle, sf_cb_resize);

    glfwMakeContextCurrent(win->handle);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return sf_err(sf_lit("GLAD Failed to initialize!"));
    sf_window_set_camera(win, camera);

    win->fb_mesh = sf_mesh_new();
    sf_mesh_add_vertices(&win->fb_mesh, (sf_vertex[]){
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, sf_rgbagl(SF_WHITE)},
        {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, sf_rgbagl(SF_WHITE)},
        {{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, sf_rgbagl(SF_WHITE)},

        {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, sf_rgbagl(SF_WHITE)},
        {{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, sf_rgbagl(SF_WHITE)},
        {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, sf_rgbagl(SF_WHITE)},
    }, 6);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(sf_gl_dbglog, nullptr);
    glEnable(GL_DEPTH_TEST);

    if ((hints & SF_WINDOW_VISIBLE) == SF_WINDOW_VISIBLE)
        glfwShowWindow(win->handle);

    if ((hints & SF_WINDOW_FULLSCREEN) == SF_WINDOW_FULLSCREEN) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        glfwSetWindowMonitor(win->handle, monitor,
            0, 0,
            size.x, size.y,
            0);
    }

    return sf_ok();
}

void sf_window_close(sf_window *window) {
    sf_str_free(window->title);
    sf_mesh_delete(&window->fb_mesh);
    glfwDestroyWindow(window->handle);
}

sf_str sf_key_string(sf_window *window) {
    const sf_str str = sf_str_cdup(window->keyboard_string);
    memset(window->keyboard_string, 0, 64);
    window->kb_p = 0;
    return str;
}

void sf_window_set_camera(sf_window *window, sf_camera *camera) {
    if (camera->type == SF_CAMERA_ORTHOGRAPHIC)
        glm_ortho(0, window->size.x, window->size.y, 0, camera->near, camera->far, camera->projection);
    else glm_perspective(camera->fov, window->size.x/window->size.y, camera->near, camera->far, camera->projection);

    if (camera->framebuffer == 0) {
        glGenFramebuffers(1, &camera->framebuffer);

        camera->fb_color = sf_texture_new(SF_TEXTURE_RGBA, window->size);
        camera->fb_stencil = sf_texture_new(SF_TEXTURE_DEPTH_STENCIL, window->size);

        glBindFramebuffer(GL_FRAMEBUFFER, camera->framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, camera->fb_color.handle, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, camera->fb_stencil.handle, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        sf_texture_resize(&camera->fb_color, window->size);
        sf_texture_resize(&camera->fb_stencil, window->size);
    }

    window->camera = camera;
}

bool sf_window_loop(const sf_window *window) {
    //TODO: Prepare for frame.
    glfwMakeContextCurrent(window->handle);
    sf_opengl_log();
    glfwPollEvents();

    glBindFramebuffer(GL_FRAMEBUFFER, window->camera->framebuffer);
    const sf_glcolor gl = sf_rgbagl(window->camera->clear_color);
    glClearColor(gl.r, gl.g, gl.b, gl.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return !glfwWindowShouldClose(window->handle);
}

sf_result sf_window_draw(sf_window *window, sf_shader *post_shader) {
    glfwMakeContextCurrent(window->handle);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, (int)window->size.x, (int)window->size.y);
    const sf_result res = sf_mesh_draw(&window->fb_mesh, post_shader, SF_RENDER_DEFAULT, SF_TRANSFORM_IDENTITY, &window->camera->fb_color);
    glfwSwapBuffers(window->handle);
    if (!res.ok)
        return res;

    for (int i = 0; i < SF_KEY_COUNT; ++i) {
        if (window->keyboard[i] == SF_KEY_PRESSED)
            window->keyboard[i] = SF_KEY_DOWN;
        if (window->keyboard[i] == SF_KEY_RELEASED)
            window->keyboard[i] = 0;
    }

    return sf_ok();
}

void sf_window_set_title(sf_window *window, const sf_str title) {
    sf_str_free(window->title);
    window->title = sf_str_dup(title);
    glfwSetWindowTitle(window->handle, title.c_str);
}

void sf_window_set_size(sf_window *window, const sf_vec2 size) {
    window->size = size;
    glfwSetWindowSize(window->handle, (int)size.x, (int)size.y);
}

void sf_window_update_hints(sf_window *window, uint8_t hints) {
    (hints & SF_WINDOW_VISIBLE) == SF_WINDOW_VISIBLE ? glfwShowWindow(window->handle) : glfwHideWindow(window->handle);

    if ((hints & SF_WINDOW_MAXIMIZED) == SF_WINDOW_MAXIMIZED)
        glfwMaximizeWindow(window->handle);
    else if (glfwGetWindowAttrib(window->handle, GLFW_MAXIMIZED))
        glfwRestoreWindow(window->handle);

    if ((hints & SF_WINDOW_FULLSCREEN) == SF_WINDOW_FULLSCREEN) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        glfwSetWindowMonitor(window->handle, monitor,
            0, 0,
            window->size.x, window->size.y,
            0);
    } else if (window->hints & SF_WINDOW_FULLSCREEN) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window->handle, nullptr,
            (mode->width - window->size.x) / 2, (mode->height - window->size.y) / 2,
            window->size.x, window->size.y,
            0);
        window->hints &= ~SF_WINDOW_FULLSCREEN;
    }
}
