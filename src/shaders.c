#include <sf/math.h>
#include <sf/fs.h>
#include "sf/gfx/shaders.h"
#include "sf/str.h"

#define EXPECTED_NAME ls_ex
#define EXPECTED_O GLuint
#define EXPECTED_E sf_shader_err
#include <sf/containers/expected.h>

ls_ex sf_load_shader(const GLenum type, const sf_str path) {
    const sf_str spath = sf_str_fmt("%s.%s", path.c_str, type == GL_FRAGMENT_SHADER ? "frag" : "vert");
    uint8_t *sbuffer = NULL;

    GLuint sh = glCreateShader(type);

    const long s = sf_file_size(spath);
    if (s <= 0) {
        sf_str_free(spath);
        return ls_ex_err((sf_shader_err){SF_SHADER_NOT_FOUND, SF_STR_EMPTY});
    }

    sbuffer = malloc((size_t)s + 1);
    sf_fs_ex fres = sf_load_file(sbuffer, spath);
    if (!fres.is_ok) {
        sf_str_free(spath);
        free(sbuffer);
        glDeleteShader(sh);
        return ls_ex_err((sf_shader_err){SF_SHADER_NOT_FOUND, SF_STR_EMPTY});
    }
    sbuffer[s] = '\0';

    glShaderSource(sh, 1, (const GLchar **)&sbuffer, NULL);
    glCompileShader(sh);

    int success;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(sh, 512, NULL, log);

        sf_str_free(spath);
        free(sbuffer);
        glDeleteShader(sh);
        return ls_ex_err((sf_shader_err){
            SF_SHADER_COMPILE_ERROR,
            sf_str_fmt("Failed to compile shader '%s': %s", path.c_str, log)
        });
    }

    sf_str_free(spath);
    free(sbuffer);
    return ls_ex_ok(sh);
}

sf_shader_ex sf_shader_new(const sf_str path) {
    sf_shader out;
    ls_ex res = sf_load_shader(GL_VERTEX_SHADER, path);
    if (!res.is_ok)
        return sf_shader_ex_err((sf_shader_err){res.value.err.type,  res.value.err.compile_err});

    GLuint vertex, fragment;
    vertex = res.value.ok;
    res = sf_load_shader(GL_FRAGMENT_SHADER, path);
    if (!res.is_ok) {
        glDeleteShader(vertex);
        return sf_shader_ex_err((sf_shader_err){res.value.err.type,  res.value.err.compile_err});
    }
    fragment = res.value.ok;

    out.program = glCreateProgram();
    glAttachShader(out.program, vertex);
    glAttachShader(out.program, fragment);
    glLinkProgram(out.program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    int success;
    glGetProgramiv(out.program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(out.program, 512, NULL, log);
        return sf_shader_ex_err((sf_shader_err){
            SF_SHADER_COMPILE_ERROR,
            sf_str_fmt("Failed to compile shader '%s': %s", path, log)
        });
    }

    out.uniforms = sf_uniform_map_new();
    out.path = sf_str_dup(path);

    return sf_shader_ex_ok(out);
}

void sf_shader_free(sf_shader *shader) {
    sf_str_free(shader->path);
    glDeleteProgram(shader->program);
    sf_uniform_map_free(&shader->uniforms);
}

#define EXPECTED_NAME gu_ex
#define EXPECTED_O GLint
#include <sf/containers/expected.h>

gu_ex sf_get_uniform(sf_shader *shader, const sf_str name) {
    sf_shader_bind(shader);
    sf_uniform_map_ex res = sf_uniform_map_get(&shader->uniforms, name);
    if (res.is_ok)
        return gu_ex_ok(res.value.ok);

    const GLint loc = glGetUniformLocation(shader->program, name.c_str);
    if (loc < 0)
        return gu_ex_err();
    sf_uniform_map_set(&shader->uniforms, name, loc);

    return gu_ex_ok(loc);
}

sf_uniform_ex sf_shader_uniform_float(sf_shader *shader, const sf_str name, const float value) {
    const gu_ex res = sf_get_uniform(shader, name);
    if (!res.is_ok)
        return sf_uniform_ex_err((sf_shader_err){SF_SHADER_UNKNOWN_UNIFORM, SF_STR_EMPTY});
    glUniform1f(res.value.ok, value);

    return sf_uniform_ex_ok();
}

sf_uniform_ex sf_shader_uniform_int(sf_shader *shader, const sf_str name, const int value) {
    const gu_ex res = sf_get_uniform(shader, name);
    if (!res.is_ok)
        return sf_uniform_ex_err((sf_shader_err){SF_SHADER_UNKNOWN_UNIFORM, SF_STR_EMPTY});
    glUniform1i(res.value.ok, value);

    return sf_uniform_ex_ok();
}

sf_uniform_ex sf_shader_uniform_vec2(sf_shader *shader, const sf_str name, const sf_vec2 value) {
    const gu_ex res = sf_get_uniform(shader, name);
    if (!res.is_ok)
        return sf_uniform_ex_err((sf_shader_err){SF_SHADER_UNKNOWN_UNIFORM, SF_STR_EMPTY});
    glUniform2f(res.value.ok, value.x, value.y);

    return sf_uniform_ex_ok();
}

sf_uniform_ex sf_shader_uniform_vec3(sf_shader *shader, const sf_str name, const sf_vec3 value) {
    const gu_ex res = sf_get_uniform(shader, name);
    if (!res.is_ok)
        return sf_uniform_ex_err((sf_shader_err){SF_SHADER_UNKNOWN_UNIFORM, SF_STR_EMPTY});
    glUniform3f(res.value.ok, value.x, value.y, value.z);

    return sf_uniform_ex_ok();
}

sf_uniform_ex sf_shader_uniform_mat4(sf_shader *shader, const sf_str name, const mat4 value) {
    const gu_ex res = sf_get_uniform(shader, name);
    if (!res.is_ok)
        return sf_uniform_ex_err((sf_shader_err){SF_SHADER_UNKNOWN_UNIFORM, SF_STR_EMPTY});
    glUniformMatrix4fv(res.value.ok, 1, false, (const GLfloat *)value);

    return sf_uniform_ex_ok();
}

void sf_transform_model(mat4 out, const sf_transform transform) {
    mat4 local;
    glm_mat4_identity(local);
    glm_scale(local, (vec3){transform.scale.x, transform.scale.y, transform.scale.z});

    if (transform.parent) {
        glm_translate(local, (vec3){transform.position.x, transform.position.y, transform.position.z});

        glm_rotate(local, glm_rad(transform.rotation.x), (vec3){1, 0, 0});
        glm_rotate(local, glm_rad(transform.rotation.y), (vec3){0, 1, 0});
        glm_rotate(local, glm_rad(transform.rotation.z), (vec3){0, 0, 1});

        mat4 parent_matrix;
        sf_transform_model(parent_matrix, *transform.parent);
        glm_mat4_mul(parent_matrix, local, out);
    } else {
        glm_rotate(local, glm_rad(transform.rotation.x), (vec3){1, 0, 0});
        glm_rotate(local, glm_rad(transform.rotation.y), (vec3){0, 1, 0});
        glm_rotate(local, glm_rad(transform.rotation.z), (vec3){0, 0, 1});

        glm_translate(local, (vec3){transform.position.x, transform.position.y, transform.position.z});

        glm_mat4_copy(local, out);
    }
}

void sf_transform_view(mat4 out, const sf_transform transform) {
    sf_transform_model(out, transform);
    glm_mat4_inv(out, out);
}
