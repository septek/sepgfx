#include "sf/meshes.h"

#include "sf/camera.h"

#define CLEAN_BIND true
const sf_camera *SF_RENDER_DEFAULT = &(sf_camera){
    .type = SF_CAMERA_RENDER_DEFAULT,
    .transform = SF_TRANSFORM_IDENTITY,
    .framebuffer = 0,
};

sf_mesh sf_mesh_new() {
    sf_mesh mesh = {
        .vertices = sf_vec_new(sf_vertex),
        .indices = sf_vec_new(int32_t),
        .cache = sf_map_new(),
        .flags = SF_MESH_ACTIVE | SF_MESH_VISIBLE,
    };

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    // Vertex Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), nullptr);
    // UV Coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    // Vertex Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(5 * sizeof(float)));

    if (CLEAN_BIND) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    sf_opengl_log();

    return mesh;
}

void sf_mesh_delete(sf_mesh *mesh) {
    sf_vec_delete(&mesh->vertices);
    sf_vec_delete(&mesh->indices);
    sf_map_delete(&mesh->cache);

    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);

    mesh->flags &= ~SF_MESH_ACTIVE;
    mesh->flags &= ~SF_MESH_VISIBLE;
}

void sf_mesh_update(const sf_mesh *mesh) {
    glBindVertexArray(mesh->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, (int64_t)(mesh->vertices.count * mesh->vertices.element_size), mesh->vertices.data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (int64_t)(mesh->indices.count * mesh->indices.element_size), mesh->indices.data, GL_STATIC_DRAW);

    if (CLEAN_BIND) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void _sf_mesh_add_vertex(sf_mesh *mesh, const sf_vertex vertex) {
    const sf_map_key key = (sf_map_key){(uint8_t *)&vertex,sizeof(sf_vertex)};
    if (sf_map_exists(&mesh->cache, key)) {
        sf_vec_push(&mesh->indices, sf_map_get(&mesh->cache, key));
        return;
    }

    sf_vec_push(&mesh->vertices, &vertex);
    sf_vec_push(&mesh->indices, &(int32_t){(int32_t)mesh->vertices.count - 1});
    sf_map_insert(&mesh->cache, key, &(int32_t){(int32_t)mesh->vertices.count - 1}, sizeof(int32_t));
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

sf_result sf_mesh_draw(const sf_mesh *mesh, sf_shader *shader, const sf_camera *camera, const sf_transform transform, const sf_texture *texture) {
    sf_shader_bind(shader);

    sf_result res;
    if (camera->type == SF_CAMERA_RENDER_DEFAULT) {
        mat4 identity;
        glm_mat4_identity(identity);
        res = sf_shader_uniform_mat4(shader, sf_lit("m_projection"), identity);
    } else
        res = sf_shader_uniform_mat4(shader, sf_lit("m_projection"), camera->projection);
    if (!res.ok)
        return res;

    mat4 campos;
    sf_transform cp = camera->transform;
    cp.position = (sf_vec3){-cp.position.x, -cp.position.y, -cp.position.z};
    sf_transform_model(campos, cp);
    res = sf_shader_uniform_mat4(shader, sf_lit("m_campos"), campos);
    if (!res.ok)
        return res;

    mat4 model;
    sf_transform_model(model, transform);
    res = sf_shader_uniform_mat4(shader, sf_lit("m_model"), model);
    if (!res.ok)
        return res;

    res = sf_shader_uniform_int(shader, sf_lit("t_sampler"), 0);
    if (!res.ok)
        return res;

    glBindFramebuffer(GL_FRAMEBUFFER, camera->framebuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glDrawElements(GL_TRIANGLES, (int32_t)mesh->indices.count, GL_UNSIGNED_INT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return sf_ok();
}
