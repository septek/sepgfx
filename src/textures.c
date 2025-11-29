#include <sf/fs.h>
#include "sf/gfx/textures.h"
#include "stb/stb_image.h"

sf_texture sf_texture_new(sf_texture_type type, const sf_vec2 dimensions) {
    sf_texture tex = {
        .type = type,
    };

    glGenTextures(1, &tex.handle);
    glBindTexture(GL_TEXTURE_2D, tex.handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sf_texture_resize(&tex, dimensions);

    return tex;
}

sf_texture_ex sf_texture_load(const sf_str path) {
    sf_texture out = {0};
    if (!sf_file_exists(path))
        return sf_texture_ex_err(SF_FILE_NOT_FOUND);

    stbi_set_flip_vertically_on_load(1);
    int width, height, channels;
    uint8_t *buffer = stbi_load(path.c_str, &width, &height, &channels, 4 /* RGBA */);
    if (!buffer)
        return sf_texture_ex_err(SF_READ_FAILURE);
    out.dimensions = (sf_vec2){(float)width, (float)height};

    glGenTextures(1, &out.handle);
    glBindTexture(GL_TEXTURE_2D, out.handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)out.dimensions.x,
    (int)out.dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return sf_texture_ex_ok(out);
}

EXPORT void sf_texture_resize(sf_texture *texture, const sf_vec2 dimensions) {
    if (dimensions.x == texture->dimensions.x && dimensions.y == texture->dimensions.y)
        return;

    glBindTexture(GL_TEXTURE_2D, texture->handle);
    GLint internal_format = GL_RGBA8;
    GLuint format = GL_RGBA;
    GLuint g_type = GL_UNSIGNED_BYTE;
    switch (texture->type) {
        case SF_TEXTURE_RGB:
            internal_format = GL_RGB;
            format = GL_RGB;
            g_type = GL_UNSIGNED_BYTE;
            break;
        case SF_TEXTURE_DEPTH_STENCIL:
            internal_format = GL_DEPTH24_STENCIL8;
            format = GL_DEPTH_STENCIL;
            g_type = GL_UNSIGNED_INT_24_8;
            break;
        default: break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (int)dimensions.x,
        (int)dimensions.y, 0, format, g_type, NULL);
    if (texture->type != SF_TEXTURE_DEPTH_STENCIL) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    texture->dimensions = dimensions;
}

void sf_texture_delete(sf_texture *texture) {
    glDeleteTextures(1, &texture->handle);
    texture->dimensions = (sf_vec2){0, 0};
}
