#ifndef CAMERA_H
#define CAMERA_H

#include <sf/math.h>
#include <cglm/cglm.h>
#include "export.h"
#include "shaders.h"
#include "textures.h"
#include <glad/glad.h>

typedef enum {
    SF_CAMERA_RENDER_DEFAULT,
    SF_CAMERA_PERSPECTIVE,
    SF_CAMERA_ORTHOGRAPHIC,
} sf_camera_type;

/// A movable camera.
typedef struct {
    sf_camera_type type;
    sf_transform transform;
    float fov, near, far;
    mat4 projection;

    GLuint framebuffer;
    sf_texture fb_color;
    GLuint fb_stencil;
    sf_rgba clear_color;
} sf_camera;

/// Create a new camera with its own framebuffer.
EXPORT sf_camera sf_camera_new(sf_camera_type type, float fov, float near, float far);
/// Delete a camera and its framebuffer.
EXPORT void sf_camera_delete(sf_camera *camera);

/// Get the right direction vector of a camera.
EXPORT sf_vec3 sf_camera_right(const sf_camera *camera);
/// Get the forward direction vector of a camera.
EXPORT sf_vec3 sf_camera_forward(const sf_camera *camera);

#endif // CAMERA_H
