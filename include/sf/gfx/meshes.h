#ifndef MESHES_H
#define MESHES_H

#include <sf/math.h>
#include <string.h>
#include "sf/gfx/camera.h"
#include "sf/gfx/shaders.h"
#include "sf/gfx/textures.h"

/// Camera that renders to the default framebuffer instead of its own framebuffer.
extern const sf_camera *SF_RENDER_DEFAULT;

/// Contains vertex data for composing a mesh.
#pragma pack(push, 1)
typedef struct {
    sf_vec3 position;
    sf_vec2 uv;
    sf_glcolor color;
} sf_vertex;
#pragma pack(pop)

#define VEC_NAME sf_vertex_vec
#define VEC_T sf_vertex
#include <sf/containers/vec.h>
#define VEC_NAME sf_index_vec
#define VEC_T int32_t
#include <sf/containers/vec.h>
#define MAP_NAME sf_index_cache
#define MAP_K sf_vertex
#define MAP_V int32_t
#define EQUAL_FN(v1, v2) (memcmp(&v1, &v2, sizeof(sf_vertex)))
#include <sf/containers/map.h>

/// A bitfield containing information about an active mesh.
typedef uint8_t sf_mesh_flags;
#define SF_MESH_ACTIVE (sf_mesh_flags)(1 << 0)
#define SF_MESH_VISIBLE (sf_mesh_flags)(1 << 1)

/// A mesh containing data for drawing a 3d model of any variety.
typedef struct {
    GLuint vao, vbo, ebo;
    sf_vertex_vec vertices;
    sf_index_vec indices;
    sf_index_cache cache;
    sf_mesh_flags flags;
} sf_mesh;

typedef struct {
    enum {
        SF_DRAW_SHADER_MISSING,
        SF_DRAW_UNKNOWN_UNIFORM,
    } type;
    union {
        sf_str uniform_name;
    } value;
} sf_draw_err;

#define EXPECTED_NAME sf_draw_ex
#define EXPECTED_E sf_draw_err
#include <sf/containers/expected.h>

/// Create a new, empty mesh.
EXPORT sf_mesh sf_mesh_new(void);
/// Free a mesh and delete all of its vertices.
EXPORT void sf_mesh_delete(sf_mesh *mesh);

/// Copy a mesh to vram (Vertex Buffer)
EXPORT void sf_mesh_update(const sf_mesh *mesh);
/// Add a single vertex to a mesh's model.
EXPORT void sf_mesh_add_vertex(sf_mesh *mesh, sf_vertex vertex);
/// Add an array of vertices to a mesh's model.
EXPORT void sf_mesh_add_vertices(sf_mesh *mesh, const sf_vertex *vertices, size_t count);


/// Draw a mesh to the framebuffer of the specified camera.
/// To draw to the default framebuffer, pass SF_RENDER_DEFAULT.
EXPORT sf_draw_ex sf_mesh_draw(const sf_mesh *mesh, sf_shader *shader, const sf_camera *camera, sf_transform transform, const sf_texture *texture);

#endif // MESHES_H
