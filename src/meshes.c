#include "sf/gfx/meshes.h"
#include "sf/gfx/camera.h"
#include "sf/gfx/shaders.h"
#include "sf/str.h"

#define CLEAN_BIND true
const sf_camera *SF_RENDER_DEFAULT = &(sf_camera){
    .type = SF_CAMERA_RENDER_DEFAULT,
    .transform = SF_TRANSFORM_IDENTITY,
    .framebuffer = 0,
    .clear_color = {12, 12, 12, 255},
};

sf_mesh sf_mesh_new(void) {
    sf_mesh mesh = {
        .vertices = sf_vertex_vec_new(),
        .indices = sf_index_vec_new(),
        .cache = sf_index_cache_new(),
        .flags = SF_MESH_ACTIVE | SF_MESH_VISIBLE,
    };

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    // Vertex Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), NULL);
    // UV Coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    // Vertex Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(5 * sizeof(float)));

    if (CLEAN_BIND) {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    sf_opengl_log();

    return mesh;
}

void sf_mesh_delete(sf_mesh *mesh) {
    sf_vertex_vec_free(&mesh->vertices);
    sf_index_vec_free(&mesh->indices);
    sf_index_cache_free(&mesh->cache);

    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);

    mesh->flags &= ~SF_MESH_ACTIVE;
    mesh->flags &= ~SF_MESH_VISIBLE;
}

void sf_mesh_update(const sf_mesh *mesh) {
    glBindVertexArray(mesh->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, (int64_t)(mesh->vertices.count * sizeof(sf_vertex)), mesh->vertices.data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (int64_t)(mesh->indices.count * sizeof(uint32_t)), mesh->indices.data, GL_STATIC_DRAW);

    if (CLEAN_BIND) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void _sf_mesh_add_vertex(sf_mesh *mesh, const sf_vertex vertex) {
    sf_index_cache_ex cache = sf_index_cache_get(&mesh->cache, vertex);
    if (cache.is_ok) {
        sf_index_vec_push(&mesh->indices, cache.value.ok);
        return;
    }

    sf_vertex_vec_push(&mesh->vertices, vertex);
    sf_index_vec_push(&mesh->indices, (uint32_t)mesh->vertices.count - 1);
    sf_index_cache_set(&mesh->cache, vertex, (uint32_t)mesh->vertices.count - 1);
}

void sf_mesh_add_vertex(sf_mesh *mesh, const sf_vertex vertex) {
    _sf_mesh_add_vertex(mesh, vertex);
    sf_mesh_update(mesh);
}

void sf_mesh_add_vertices(sf_mesh *mesh, const sf_vertex *vertices, const size_t count) {
    for (size_t i = 0; i < count; ++i)
        _sf_mesh_add_vertex(mesh, vertices[i]);
    sf_mesh_update(mesh);
}

sf_draw_ex sf_mesh_draw(const sf_mesh *mesh, sf_shader *shader, const sf_camera *camera, const sf_transform transform, const sf_texture *texture) {
    if (shader == NULL)
        return sf_draw_ex_err((sf_draw_err){SF_DRAW_SHADER_MISSING, .value.uniform_name = SF_STR_EMPTY});
    sf_shader_bind(shader);

    if (camera->type == SF_CAMERA_RENDER_DEFAULT) {
        mat4 identity;
        glm_mat4_identity(identity);
        if (!sf_shader_uniform_mat4(shader, sf_lit("m_projection"), identity).is_ok)
            return sf_draw_ex_err((sf_draw_err){SF_DRAW_UNKNOWN_UNIFORM, .value.uniform_name = sf_lit("m_projection")});
    } else if (!sf_shader_uniform_mat4(shader, sf_lit("m_projection"), camera->projection).is_ok)
        return sf_draw_ex_err((sf_draw_err){SF_DRAW_UNKNOWN_UNIFORM, .value.uniform_name = sf_lit("m_projection")});

    mat4 campos;
    sf_transform cp = camera->transform;
    cp.position = (sf_vec3){-cp.position.x, -cp.position.y, -cp.position.z};
    sf_transform_model(campos, cp);
    if (!sf_shader_uniform_mat4(shader, sf_lit("m_campos"), campos).is_ok)
        return sf_draw_ex_err((sf_draw_err){SF_DRAW_UNKNOWN_UNIFORM, .value.uniform_name = sf_lit("m_campos")});

    mat4 model;
    sf_transform_model(model, transform);
    if (!sf_shader_uniform_mat4(shader, sf_lit("m_model"), model).is_ok)
        return sf_draw_ex_err((sf_draw_err){SF_DRAW_UNKNOWN_UNIFORM, .value.uniform_name = sf_lit("m_model")});

    if (!sf_shader_uniform_int(shader, sf_lit("t_sampler"), 0).is_ok)
        return sf_draw_ex_err((sf_draw_err){SF_DRAW_UNKNOWN_UNIFORM, .value.uniform_name = sf_lit("t_sampler")});

    glBindFramebuffer(GL_FRAMEBUFFER, camera->framebuffer);
    glViewport(0, 0, (int)camera->viewport.x, (int)camera->viewport.y);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->indices.count, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return sf_draw_ex_ok();
}
