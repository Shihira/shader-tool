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

DEF_ENUM_MAP(em_format_size_, texture::format, size_t, ({
        // TODO: if larger color format in added, insure texture read not to
        // cause segmentation fault
        { texture::RGBA_U8888, 4 },
        { texture::R_F32, 4 },
        { texture::DEPTH_F32, 4 },
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

id_type texture::create_object() const
{
    GLuint i;
    glGenTextures(1, &i);
    return i;
}

void texture::destroy_object(id_type i) const
{
    glDeleteTextures(1, &i);
}

id_type buffer::create_object() const
{
    GLuint i;
    glGenBuffers(1, &i);
    return i;
}

void buffer::destroy_object(id_type i) const
{
    glDeleteBuffers(1, &i);
}

////////////////////////////////////////////////////////////////////////////////

GLenum get_texture_type(const texture& t)
{
    if(t.get_trait() == texture::CUBEMAP)
        return GL_TEXTURE_CUBE_MAP;

    if(t.get_trait() == texture::LAYERED) {
        if(t.get_depth() == 1)
            return GL_TEXTURE_1D_ARRAY;
        else
            return GL_TEXTURE_2D_ARRAY;
    } else {
        if(t.get_depth() == 1)
            return GL_TEXTURE_2D;
        else
            return GL_TEXTURE_3D;
    }
}

void generic_fill(texture& t, const void* data, texture::format fmt,
        GLenum tex_type, GLenum bind_tex_type, size_t dim, bool opt = true)
{
    bool first_time = t.vacuum();

    if(first_time) {
        if(!t.get_width() * t.get_height() * t.get_depth())
            throw restriction_error("Size of texture cannot be zero");

        if(t.get_internal_format() == texture::DEFAULT_FMT) {
            if(fmt == texture::DEFAULT_FMT)
                fmt = texture::RGBA_U8888;
            t.set_internal_format(fmt);
        }
    }

    glBindTexture(bind_tex_type, t.id());

    if(fmt == texture::DEFAULT_FMT)
        fmt = t.get_internal_format();

    if(dim == 1)
        glTexImage1D(
            tex_type, t.get_level(),
            em_format_(t.get_internal_format()),
            t.get_width(), 0,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );
    else if(dim == 2)
        glTexImage2D(
            tex_type, t.get_level(),
            em_format_(t.get_internal_format()),
            t.get_width(), t.get_height(), 0,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );
    else if(dim == 3)
        glTexImage3D(
            tex_type, t.get_level(),
            em_format_(t.get_internal_format()),
            t.get_width(), t.get_height(), t.get_depth(), 0,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );

    if(first_time && opt) {
        glTexParameteri(bind_tex_type, GL_TEXTURE_MAG_FILTER,
                em_filter_type_(t.get_filter()));
        glTexParameteri(bind_tex_type, GL_TEXTURE_MIN_FILTER,
                em_filter_type_(t.get_filter()));
        if(data) glGenerateMipmap(bind_tex_type);
    }

    glBindTexture(bind_tex_type, GL_NONE);
}

void generic_fill_rect(texture& t,
        size_t offx, size_t offy, size_t offz,
        size_t w, size_t h, size_t d,
        const void* data, texture::format fmt, GLenum tex_type, size_t dim)
{
    if(t.vacuum())
        throw restriction_error("Texture is not yet initailized.");
    if(offx + w > t.get_width() || offy + h > t.get_height() || offz + d > t.get_depth())
        throw restriction_error("Out of bound");

    if(fmt == texture::DEFAULT_FMT)
        fmt = t.get_internal_format();

    if(dim == 1)
        glTexSubImage1D(
            tex_type, t.get_level(),
            offx, w,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );
    else if(dim == 2)
        glTexSubImage2D(
            tex_type, t.get_level(),
            offx, offy, w, h,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );
    else if(dim == 3)
        glTexSubImage3D(
            tex_type, t.get_level(),
            offx, offy, offz, w, h, d,
            em_format_component_(fmt),
            em_format_type_(fmt),
            data
        );
}

void generic_read(texture& t,
        void* data, texture::format fmt, GLenum tex_type)
{
    if(t.vacuum())
        throw restriction_error("Texture is not yet initailized.");

    if(fmt == texture::DEFAULT_FMT)
        fmt = t.get_internal_format();

    glBindTexture(tex_type, t.id());
    glGetTexImage(tex_type, 0,
            em_format_component_(fmt), em_format_type_(fmt), data);
    glBindTexture(tex_type, GL_NONE);
}

void texture::fill(const void* data, format fmt) {
    GLenum tex_type = get_texture_type(*this);

    if(get_depth() == 1)
        generic_fill(*this, data, fmt, tex_type, tex_type, 2);
    else if(get_depth() > 1)
        generic_fill(*this, data, fmt, tex_type, tex_type, 3);
}

void texture::fill_rect(
        size_t offx, size_t offy, size_t offz,
        size_t w, size_t h, size_t d,
        const void* data, format fmt)
{
    GLenum tex_type = get_texture_type(*this);
    glBindTexture(tex_type, id());
    if(get_depth() == 1)
        generic_fill_rect(*this, offx, offy, offz, w, h, d, data, fmt,
            tex_type, 2);
    else if(get_depth() > 1)
        generic_fill_rect(*this, offx, offy, offz, w, h, d, data, fmt,
            tex_type, 3);
    glBindTexture(tex_type, GL_NONE);
}

void texture::read(void* data, texture::format fmt)
{
    generic_read(*this, data, fmt, get_texture_type(*this));
}

void texture::bind_to(size_t tex_bind) const
{
    glActiveTexture(GL_TEXTURE0 + tex_bind);
    glBindTexture(get_texture_type(*this), id());
}

void texture::attach_to(size_t tex_attachment)
{
    if(vacuum())
        reserve();

    GLenum tex_type = get_texture_type(*this);
    glBindTexture(tex_type, id());
    glFramebufferTexture2D(GL_FRAMEBUFFER, tex_attachment,
            tex_type, id(), 0);
    glBindTexture(tex_type, GL_NONE);
}

void texture_cubemap::fill(const void* data, format fmt) {
    if(get_depth() != 6)
        throw restriction_error("Cubemap must has a depth of 6");

    const uint8_t* ptr_data = (const uint8_t*) data;
    size_t size = em_format_size_(fmt) * get_width() * get_height();

    for(size_t i = 0; i < 6; i++) {
        // increase data pointer, except that data is null
        generic_fill(*this, data ? ptr_data + i * size : data,
                fmt, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                GL_TEXTURE_CUBE_MAP, 2, i == 5);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, id());
    if(data) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,  
            GL_CLAMP_TO_EDGE);  
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,  
            GL_CLAMP_TO_EDGE);  
    glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
}

void texture_cubemap::fill_rect(
        size_t offx, size_t offy, size_t offz,
        size_t w, size_t h, size_t d,
        const void* data, format fmt)
{
    if(get_depth() != 6)
        throw restriction_error("Cubemap must has a depth of 6");

    glBindTexture(GL_TEXTURE_CUBE_MAP, id());

    generic_fill_rect(*this, offx, offy, 0, w, h, 1, data, fmt,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + offz, 2);

    glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
}

void texture_cubemap::read(void* data, texture::format fmt)
{
    if(get_depth() != 6)
        throw restriction_error("Cubemap must has a depth of 6");

    uint8_t* ptr_data = (uint8_t*) data;
    size_t size = em_format_size_(fmt) * get_width() * get_height();

    for(size_t i = 0; i < 6; i++) {
        generic_read(*this, ptr_data + size * i, fmt,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
    }

    if(data) glGenerateMipmap(get_texture_type(*this));
}

////////////////////////////////////////////////////////////////////////////////

void buffer::size(size_t sz) {
    if(!vacuum() && size_ != sz)
        throw restriction_error("Buffer size cannot be changed");
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
        throw restriction_error("Size to read is too large");
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


