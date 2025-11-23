#include "sf/camera.h"
#include "sf/shaders.h"

sf_camera sf_camera_new(const sf_camera_type type, const float fov, const float near, const float far) {
    return (sf_camera){
        .type = type,
        .transform = SF_TRANSFORM_IDENTITY,
        .fov = fov,
        .near = near,
        .far = far,
        .clear_color = (sf_rgba){0, 0, 0, 0},
    };
}

void sf_camera_delete(sf_camera *camera) {
    if (camera->framebuffer != 0) {
        glDeleteFramebuffers(1, &camera->framebuffer);
        sf_texture_delete(&camera->fb_color);
        sf_texture_delete(&camera->fb_stencil);
    }
}

sf_vec3 sf_camera_right(const sf_camera *camera) {
    mat4 mat;
    sf_transform_view(mat, camera->transform);

    vec3 vec = {mat[0][0], mat[0][1], mat[0][2]};
    glm_vec3_normalize(vec);

    return (sf_vec3){vec[0], vec[1], vec[2]};
}

sf_vec3 sf_camera_forward(const sf_camera *camera) {
    mat4 mat;
    sf_transform_view(mat, camera->transform);

    vec3 vec = {mat[2][0], mat[2][1], mat[2][2]};
    glm_vec3_normalize(vec);

    return (sf_vec3){vec[0], vec[1], vec[2]};
}
