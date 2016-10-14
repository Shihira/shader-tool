#include "exception.h"
#include "shading.h"

#include <GL/glew.h>

namespace shrtool {

DEF_ENUM_MAP(__em_shader_type, shader::shader_type, GLenum, ({
        { shader::VERTEX,   GL_VERTEX_SHADER },
        { shader::GEOMETRY, GL_GEOMETRY_SHADER },
        { shader::FRAGMENT, GL_FRAGMENT_SHADER },
    }))

DEF_ENUM_MAP(__em_shader_type_str, shader::shader_type, const char*, ({
        { shader::VERTEX,   "vertex shader" },
        { shader::GEOMETRY, "geometry shader" },
        { shader::FRAGMENT, "fragment shader" },
    }))

DEF_ENUM_MAP(__em_buffer_attachment, render_target::buffer_attachment, GLenum, ({
        { render_target::COLOR_BUFFER_0, GL_COLOR_ATTACHMENT0 },
        { render_target::COLOR_BUFFER_1, GL_COLOR_ATTACHMENT1 },
        { render_target::DEPTH_BUFFER,   GL_DEPTH_ATTACHMENT },
    }))

DEF_ENUM_MAP(__em_clear_mask, render_target::buffer_attachment, GLbitfield, ({
        { render_target::COLOR_BUFFER, GL_COLOR_BUFFER_BIT },
        { render_target::DEPTH_BUFFER, GL_DEPTH_BUFFER_BIT },
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
    if(_screen) glDeleteFramebuffers(1, &i);
}

id_type sub_shader::create_object() const {
    return glCreateShader(__em_shader_type(__type));
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

void render_target::render_texture(
        render_target::buffer_attachment ba,
        render_assets::texture2d &tex) {
    glBindFramebuffer(GL_FRAMEBUFFER, id());
    glFramebufferTexture2D(GL_FRAMEBUFFER, __em_buffer_attachment(ba),
            GL_TEXTURE_2D, tex.id(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

    _viewport[0] = 0;
    _viewport[1] = 0;
    _viewport[2] = tex.width();
    _viewport[3] = tex.height();
}

void render_target::clear_buffer(render_target::buffer_attachment ba) {
    glBindFramebuffer(GL_FRAMEBUFFER, id());
    if(ba == COLOR_BUFFER) {
        glClearColor(
                _init_color[0], _init_color[1],
                _init_color[2], _init_color[3]
            );
    } else if(ba == DEPTH_BUFFER) {
        glClearDepth(_init_depth);
        glDepthFunc(GL_LEQUAL);
    }
    glClear(__em_clear_mask(ba));
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

sub_shader& shader::add_sub_shader(shader::shader_type t) {
    _subshader_list.emplace_back(t);
    sub_shader& s = _subshader_list.back();
    glAttachShader(id(), s.id());
    return s;
}

void shader::property_binding(const std::string& name, size_t binding) {
    if(_property_binding.find(name) != _property_binding.end())
        throw shader_error("Uniform" + name + "has bounded.");

    glUseProgram(id());
    GLuint idx = glGetUniformBlockIndex(id(), name.c_str());
    glUniformBlockBinding(id(), idx, binding);
    glUseProgram(GL_NONE);

    if(binding > _max_binding_index) _max_binding_index = binding;
    _property_binding[name] = binding;
}

size_t shader::property(const std::string& name,
        const render_assets::property_buffer& buf) {
    size_t binding = _max_binding_index + 1;
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

void shader::draw(const vertex_attr_vector& vat) const {
    glUseProgram(id());
    glBindVertexArray(vat.id());
    glBindFramebuffer(GL_FRAMEBUFFER, _target->id());
    if(_target->is_depth_test_enabled()) {
        glEnable(GL_DEPTH_TEST);
    }
    glDrawArrays(GL_TRIANGLES, 0, vat.primitives_count());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(GL_NONE);
    glUseProgram(GL_NONE);
}

void shader::link() {
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
                + __em_shader_type_str(__type) + ":\n" + log_str);
    }
}

void vertex_attr_vector::input(uint32_t loc,const render_assets::buffer& buf) const {
    if(!primitives_count())
        throw shader_error("No primitives count speicfied");

    glBindVertexArray(id());
    glBindBuffer(GL_ARRAY_BUFFER, buf.id());
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc,
            buf.size() / primitives_count() / sizeof(GLfloat),
            GL_FLOAT, GL_TRUE, 0, 0);
    glBindVertexArray(GL_NONE);
}

}
