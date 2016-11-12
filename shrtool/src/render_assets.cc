#include <GL/glew.h>
#include <unordered_map>

#include "render_assets.h"
#include "exception.h"

namespace shrtool {

namespace render_assets {

DEF_ENUM_MAP(em_format_component_, texture::format, GLenum, ({
        { texture::RGBA_U8888, GL_RGBA },
        { texture::R_F32, GL_R },
        { texture::DEPTH_F32, GL_DEPTH_COMPONENT },
    }))

DEF_ENUM_MAP(em_format_type_, texture::format, GLenum, ({
        { texture::RGBA_U8888, GL_UNSIGNED_BYTE },
        { texture::R_F32, GL_FLOAT },
        { texture::DEPTH_F32, GL_FLOAT },
    }))

DEF_ENUM_MAP(em_format_, texture::format, GLenum, ({
        { texture::RGBA_U8888, GL_RGBA8 },
        { texture::R_F32, GL_R32F },
        { texture::DEPTH_F32, GL_DEPTH_COMPONENT32F },
    }))

DEF_ENUM_MAP(em_component_, texture_cubemap2d::face_index, GLenum, ({
        { texture_cubemap2d::FRONT,  GL_TEXTURE_CUBE_MAP_POSITIVE_Z },
        { texture_cubemap2d::BACK,   GL_TEXTURE_CUBE_MAP_NEGATIVE_Z },
        { texture_cubemap2d::TOP,    GL_TEXTURE_CUBE_MAP_POSITIVE_Y },
        { texture_cubemap2d::BOTTOM, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y },
        { texture_cubemap2d::LEFT,   GL_TEXTURE_CUBE_MAP_POSITIVE_X },
        { texture_cubemap2d::RIGHT,  GL_TEXTURE_CUBE_MAP_NEGATIVE_X },
    }))

DEF_ENUM_MAP(em_buffer_usage_, uint32_t, GLenum, ({
        { buffer::STATIC  | buffer::WRITE, GL_STATIC_DRAW },
        { buffer::STATIC  | buffer::READ , GL_STATIC_READ },
        { buffer::DYNAMIC | buffer::WRITE, GL_STATIC_DRAW },
        { buffer::DYNAMIC | buffer::READ,  GL_STATIC_READ },
    }))

DEF_ENUM_MAP(em_buffer_access_, buffer::buffer_access, GLbitfield, ({
        { buffer::WRITE, GL_WRITE_ONLY },
        { buffer::READ, GL_READ_ONLY },
        { buffer::RW, GL_READ_WRITE },
    }))

DEF_ENUM_MAP(em_filter_type_, texture::filter_type, GLenum, ({
        { texture::LINEAR, GL_LINEAR },
        { texture::NEAREST, GL_NEAREST },
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

void texture2d::fill(const void* data, format fmt) {
    glBindTexture(GL_TEXTURE_2D, id());
    format ifmt = internal_format() == DEFAULT_FMT ? fmt : internal_format();
    glTexImage2D(
            GL_TEXTURE_2D, level(),
            em_format_(ifmt),
            width(), height(), 0,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
            em_filter_type_(filter()));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            em_filter_type_(filter()));
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    filled_ = true;
}

void texture2d::read(void* data, format fmt) {
    glBindTexture(GL_TEXTURE_2D, id());
    glGetTexImage(GL_TEXTURE_2D, 0,
            em_format_component_(fmt), em_format_type_(fmt), data);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
}

void texture_cubemap2d::fill(face_index f, void* data, format fmt) {
    GLuint target = em_component_(f);

    glBindTexture(target, id());
    format ifmt = internal_format() == DEFAULT_FMT ? fmt : internal_format();
    glTexImage2D(
            target, level(),
            em_format_(ifmt),
            edge_length(), edge_length(), 0,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );
    glBindTexture(target, GL_NONE);
}

void texture2d::bind_to(size_t tex_bind) const {
    glActiveTexture(GL_TEXTURE0 + tex_bind);
    glBindTexture(GL_TEXTURE_2D, id());
}

void texture_cubemap2d::bind_to(size_t tex_bind) const {
    glActiveTexture(GL_TEXTURE0 + tex_bind);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id());
}

void buffer::size(size_t sz) {
    if(size_ != 0 && size_ != sz)
        throw restriction_error("Buffer size cannot be change a second time");
    size_ = sz;
}

void vertex_attr_buffer::write_raw(const void* data, size_t sz) {
    if(sz) size(sz);
    if(!size())
        throw restriction_error("Buffer has zero size");

    glBindBuffer(GL_ARRAY_BUFFER, id());
    glBufferData(GL_ARRAY_BUFFER, size(), data,
            em_buffer_usage_(transfer_mode() | access()));
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
}

void property_buffer::write_raw(const void* data, size_t sz) {
    if(sz) size(sz);
    if(!size())
        throw restriction_error("Buffer has zero size");

    glBindBuffer(GL_UNIFORM_BUFFER, id());
    glBufferData(GL_UNIFORM_BUFFER, size(), data,
            em_buffer_usage_(transfer_mode() | access()));
    glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);
}

void property_buffer::read_raw(void* data, size_t sz) {
    if(sz > size())
        throw restriction_error("Size to read too large");
    if(!sz) sz = size();

    glBindBuffer(GL_UNIFORM_BUFFER, id());
    glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sz, data);
    glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);
}

void vertex_attr_buffer::read_raw(void* data, size_t sz) {
    if(sz > size())
        throw restriction_error("Size to read too large");
    if(!sz) sz = size();

    glBindBuffer(GL_ARRAY_BUFFER, id());
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, sz, data);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
}

void* vertex_attr_buffer::start_map(buffer_access bt, size_t sz) {
    if(sz) size(sz);
    if(!size())
        throw restriction_error("Buffer has zero size");

    glBindBuffer(GL_ARRAY_BUFFER, id());
    if(first_map) {
        glBufferData(GL_ARRAY_BUFFER, size(), NULL,
            em_buffer_usage_(transfer_mode() | access()));
        first_map = false;
    }
    void* ptr = glMapBuffer(GL_ARRAY_BUFFER, em_buffer_access_(bt));
    mapping_state_ = bt;
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    if(!ptr)
        throw driver_error("Failed to map");
    return ptr;
}

void vertex_attr_buffer::stop_map() {
    glBindBuffer(GL_ARRAY_BUFFER, id());
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    mapping_state_ = NO_ACCESS;
}


void* property_buffer::start_map(buffer_access bt, size_t sz) {
    if(sz) size(sz);
    if(!size())
        throw restriction_error("Buffer has zero size");

    glBindBuffer(GL_UNIFORM_BUFFER, id());
    if(first_map) {
        glBufferData(GL_UNIFORM_BUFFER, size(), NULL,
            em_buffer_usage_(transfer_mode() | access()));
        first_map = false;
    }
    void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, em_buffer_access_(bt));
    mapping_state_ = bt;
    glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);

    if(!ptr)
        throw driver_error("Failed to map");
    return ptr;
}

void property_buffer::stop_map() {
    glBindBuffer(GL_UNIFORM_BUFFER, id());
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);
    mapping_state_ = NO_ACCESS;
}

} // render_assets

} // shrtool


