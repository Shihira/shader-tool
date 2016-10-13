#include <GL/glew.h>
#include <unordered_map>

#include "render_assets.h"
#include "exception.h"

using namespace std;

namespace shrtool {

namespace render_assets {

DEF_ENUM_MAP(__em_format_component, texture::format, GLenum, ({
        { texture::RGBA_U8888, GL_RGBA },
        { texture::R_F32, GL_R },
    }))

DEF_ENUM_MAP(__em_format_type, texture::format, GLenum, ({
        { texture::RGBA_U8888, GL_UNSIGNED_INT_8_8_8_8 },
        { texture::R_F32, GL_FLOAT },
    }))

DEF_ENUM_MAP(__em_format, texture::format, GLenum, ({
        { texture::RGBA_U8888, GL_RGBA8 },
        { texture::R_F32, GL_R32F },
    }))

DEF_ENUM_MAP(__em_component, texture_cubemap2d::face_index, GLenum, ({
        { texture_cubemap2d::FRONT,  GL_TEXTURE_CUBE_MAP_POSITIVE_Z },
        { texture_cubemap2d::BACK,   GL_TEXTURE_CUBE_MAP_NEGATIVE_Z },
        { texture_cubemap2d::TOP,    GL_TEXTURE_CUBE_MAP_POSITIVE_Y },
        { texture_cubemap2d::BOTTOM, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y },
        { texture_cubemap2d::LEFT,   GL_TEXTURE_CUBE_MAP_POSITIVE_X },
        { texture_cubemap2d::RIGHT,  GL_TEXTURE_CUBE_MAP_NEGATIVE_X },
    }))

DEF_ENUM_MAP(__em_buffer_usage, uint32_t, GLenum, ({
        { buffer::STATIC  | buffer::WRITE, GL_STATIC_COPY },
        { buffer::STATIC  | buffer::READ , GL_STATIC_READ },
        { buffer::DYNAMIC | buffer::WRITE, GL_STATIC_COPY },
        { buffer::DYNAMIC | buffer::READ,  GL_STATIC_READ },
    }))

////////////////////////////////////////////////////////////////////////////////

#define __DEF_GEN_DEL_CONSTRUCT_OBJECT(cls, glgenfunc, gldelfunc) \
    id_type cls::create_object() const \
        { GLuint i; glgenfunc(1, &i); return i; } \
    void cls::destroy_object(id_type i) const \
        { gldelfunc(1, &i); }

__DEF_GEN_DEL_CONSTRUCT_OBJECT(texture, glGenTextures, glDeleteTextures)
__DEF_GEN_DEL_CONSTRUCT_OBJECT(buffer, glGenBuffers, glDeleteBuffers)

////////////////////////////////////////////////////////////////////////////////

void texture2d::fill(void* data, format fmt) {
    glBindTexture(GL_TEXTURE_2D, id());
    format ifmt = internal_format() == DEFAULT_FMT ? fmt : internal_format();
    glTexImage2D(
            GL_TEXTURE_2D, level(),
            __em_format(ifmt),
            width(), height(), 0,
            __em_format_component(fmt),
            __em_format_type(fmt),
            data
        );
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
}

void texture_cubemap2d::fill(face_index f, void* data, format fmt) {
    GLuint target = __em_component(f);

    glBindTexture(target, id());
    format ifmt = internal_format() == DEFAULT_FMT ? fmt : internal_format();
    glTexImage2D(
            target, level(),
            __em_format(ifmt),
            edge_length(), edge_length(), 0,
            __em_format_component(fmt),
            __em_format_type(fmt),
            data
        );
    glBindTexture(target, GL_NONE);
}

void vertex_attr_buffer::write(void* data, size_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, id());
    glBufferData(GL_ARRAY_BUFFER, size, data,
            __em_buffer_usage(transfer_mode() | access()));
    _size = size;
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
}

void property_buffer::write(void* data, size_t size) {
    glBindBuffer(GL_UNIFORM_BUFFER, id());
    glBufferData(GL_UNIFORM_BUFFER, size, data,
            __em_buffer_usage(transfer_mode() | access()));
    _size = size;
    glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);
}

}

}


