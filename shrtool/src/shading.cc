#include <iostream>

#include "exception.h"
#include "shading.h"

#include <GL/glew.h>

namespace shrtool {

using namespace render_assets;

DEF_ENUM_MAP(em_shader_type_, shader::shader_type, GLenum, ({
        { shader::VERTEX,   GL_VERTEX_SHADER },
        { shader::GEOMETRY, GL_GEOMETRY_SHADER },
        { shader::FRAGMENT, GL_FRAGMENT_SHADER },
    }))

DEF_ENUM_MAP(em_shader_type_str_, shader::shader_type, const char*, ({
        { shader::VERTEX,   "vertex shader" },
        { shader::GEOMETRY, "geometry shader" },
        { shader::FRAGMENT, "fragment shader" },
    }))

DEF_ENUM_MAP(em_buffer_attachment_, render_target::buffer_attachment, GLenum, ({
        { render_target::COLOR_BUFFER_0, GL_COLOR_ATTACHMENT0 },
        { render_target::COLOR_BUFFER_1, GL_COLOR_ATTACHMENT1 },
        { render_target::DEPTH_BUFFER,   GL_DEPTH_ATTACHMENT },
    }))

DEF_ENUM_MAP(em_clear_mask_, render_target::buffer_attachment, GLbitfield, ({
        { render_target::COLOR_BUFFER, GL_COLOR_BUFFER_BIT },
        { render_target::DEPTH_BUFFER, GL_DEPTH_BUFFER_BIT },
    }))

DEF_ENUM_MAP(em_element_type_, element_type::element_type_e, GLenum, ({
        { element_type::FLOAT, GL_FLOAT },
        { element_type::BYTE, GL_UNSIGNED_BYTE },
        { element_type::UINT, GL_UNSIGNED_INT },
    }))

DEF_ENUM_MAP(em_element_type_size_, element_type::element_type_e, size_t, ({
        { element_type::FLOAT, sizeof(float) },
        { element_type::BYTE, sizeof(uint8_t) },
        { element_type::UINT, sizeof(uint32_t) },
    }))

////////////////////////////////////////////////////////////////////////////////

render_target render_target::screen(true);

id_type render_target::create_object() const {
    if(isscr()) return 0;

    GLuint i;
    glGenFramebuffers(1, &i);
    return i;
}

void render_target::destroy_object(id_type i) const {
    if(screen_) glDeleteFramebuffers(1, &i);
}

id_type sub_shader::create_object() const {
    return glCreateShader(em_shader_type_(type_));
}

void sub_shader::destroy_object(id_type i) const {
    glDeleteShader(i);
}

id_type shader::create_object() const {
    return glCreateProgram();
}

void shader::destroy_object(id_type i) const {
    glDeleteProgram(i);
}

id_type vertex_attr_vector::create_object() const {
    GLuint i;
    glGenVertexArrays(1, &i);
    return i;
}

void vertex_attr_vector::destroy_object(id_type i) const {
    glDeleteVertexArrays(1, &i);
}

////////////////////////////////////////////////////////////////////////////////

void render_target::set_viewport(size_t x, size_t y, size_t w, size_t h) {
    viewport_[0] = x; viewport_[1] = y;
    viewport_[2] = w; viewport_[3] = h;

    glBindFramebuffer(GL_FRAMEBUFFER, id());
    glViewport(x, y, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

void render_target::render_texture(
        render_target::buffer_attachment ba,
        render_assets::texture2d &tex) {
    glBindFramebuffer(GL_FRAMEBUFFER, id());
    glFramebufferTexture2D(GL_FRAMEBUFFER, em_buffer_attachment_(ba),
            GL_TEXTURE_2D, tex.id(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

    viewport_[0] = 0;
    viewport_[1] = 0;
    viewport_[2] = tex.width();
    viewport_[3] = tex.height();
}

void render_target::clear_buffer(render_target::buffer_attachment ba) {
    glBindFramebuffer(GL_FRAMEBUFFER, id());
    if(ba == COLOR_BUFFER) {
        glClearColor(
                init_color_[0], init_color_[1],
                init_color_[2], init_color_[3]
            );
    } else if(ba == DEPTH_BUFFER) {
        glClearDepth(init_depth_);
        glDepthFunc(GL_LEQUAL);
    }
    glClear(em_clear_mask_(ba));
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

sub_shader& shader::add_sub_shader(shader::shader_type t) {
    sub_shader_ptr p(new sub_shader(t));
    add_sub_shader(p);
    return *p;
}

void shader::add_sub_shader(shader::sub_shader_ptr p) {
    sub_shaders_[p->type()] = p;
}

void shader::property_binding(const std::string& name, size_t binding) {
    if(property_binding_.find(name) != property_binding_.end())
        throw shader_error("Uniform" + name + "has bound.");

    glUseProgram(id());
    GLuint idx = glGetUniformBlockIndex(id(), name.c_str());
    glUniformBlockBinding(id(), idx, binding);
    glUseProgram(GL_NONE);

    if(binding > max_binding_index_) max_binding_index_ = binding;
    property_binding_[name] = binding;
}

size_t shader::property(const std::string& name,
        const render_assets::property_buffer& buf) {
    size_t binding = max_binding_index_ + 1;
    property_binding(name, binding);
    property(binding, buf);

    return binding;
}

void shader::property(size_t binding,
        const render_assets::property_buffer& buf) {
    glUseProgram(id());
    glBindBuffer(GL_UNIFORM_BUFFER, buf.id());
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, buf.id());
    glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);
    glUseProgram(GL_NONE);
}

size_t shader::property(const std::string& name,
        const render_assets::texture& tex) {
    const char* c_name = name.c_str();
    GLuint loc = glGetUniformLocation(id(), c_name);
    property(loc, tex);

    return loc;
}

void shader::property(size_t binding,
        const render_assets::texture& tex) {
    textures_binding_[binding] = &tex;
}

void shader::draw(const vertex_attr_vector& vat) const {
    glUseProgram(id());
    glBindVertexArray(vat.id());
    glBindFramebuffer(GL_FRAMEBUFFER, target_->id());
    if(target_->is_depth_test_enabled()) {
        glEnable(GL_DEPTH_TEST);
    }

    // bind textures
    size_t tex_num = 0;
    for(auto& e : textures_binding_) {
        e.second->bind_to(tex_num);
        glUniform1i(e.first, tex_num);
        tex_num += 1;
    }

    glDrawArrays(GL_TRIANGLES, 0, vat.primitives_count());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(GL_NONE);
    glUseProgram(GL_NONE);
}

void shader::link() {
    for(auto& e : sub_shaders_) {
        glAttachShader(id(), e.second->id());
    }

    glLinkProgram(id());

    GLint status;
    glGetProgramiv(id(), GL_LINK_STATUS, &status);

    if(status == GL_FALSE) {
        GLint log_len, actual_log_len;
        glGetProgramiv(id(), GL_INFO_LOG_LENGTH, &log_len);
        char* log_str = new char[log_len + 1];
        glGetProgramInfoLog(id(), log_len + 1, &actual_log_len, log_str);
        throw shader_error(std::string("Error while linking:\n") + log_str);
    }
}

void sub_shader::compile(const std::string& s) {
    const char* ps = s.c_str();
    GLint len = s.size();

    glShaderSource(id(), 1, &ps, &len);
    glCompileShader(id());

    GLint status;
    glGetShaderiv(id(), GL_COMPILE_STATUS, &status);

    if(status == GL_FALSE) {
        GLint log_len, actual_log_len;
        glGetShaderiv(id(), GL_INFO_LOG_LENGTH, &log_len);
        char* log_str = new char[log_len + 1];
        glGetShaderInfoLog(id(), log_len + 1, &actual_log_len, log_str);
        throw shader_error(std::string("Error while compiling ")
                + em_shader_type_str_(type_) + ":\n" + log_str);
    }
}

void vertex_attr_vector::updated() {
    for(auto& e : bindings_)
        updated(e.first);
}

void vertex_attr_vector::updated(size_t loc) {
    auto& buf = bindings_[loc];

    if(!primitives_count())
        throw shader_error("No primitives count speicfied");

    glBindVertexArray(id());
    glBindBuffer(GL_ARRAY_BUFFER, buf->id());
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, buf->size() /
                primitives_count() / em_element_type_size_(buf->type()),
            em_element_type_(buf->type()), GL_TRUE, 0, 0);
    glBindVertexArray(GL_NONE);
}

}

