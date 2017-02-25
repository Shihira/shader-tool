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
        { render_target::COLOR_BUFFER_2, GL_COLOR_ATTACHMENT2 },
        { render_target::COLOR_BUFFER_3, GL_COLOR_ATTACHMENT3 },
        { render_target::DEPTH_BUFFER,   GL_DEPTH_ATTACHMENT },
    }))

/*
DEF_ENUM_MAP(em_clear_mask_, render_target::buffer_attachment, GLenum, ({
        { render_target::COLOR_BUFFER, GL_COLOR },
        { render_target::DEPTH_BUFFER, GL_DEPTH },
    }))
*/

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

render_target render_target::screen;

id_type render_target::create_object() const {
    if(is_screen()) return 0;

    GLuint i;
    glGenFramebuffers(1, &i);
    return i;
}

void render_target::destroy_object(id_type i) const {
    if(!is_screen()) glDeleteFramebuffers(1, &i);
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

void render_target::attach_texture(
        render_target::buffer_attachment ba,
        render_assets::texture &tex) {
    if(this == &screen)
        throw restriction_error("You cannot attach any "
                "texture to the screen.");
    tex_attachments_[ba] = &tex;

    glBindFramebuffer(GL_FRAMEBUFFER, id());
    tex.attach_to(em_buffer_attachment_(ba));
    if(ba != DEPTH_BUFFER)
        glDrawBuffer(em_buffer_attachment_(ba));

    set_viewport(rect::from_size(tex.get_width(), tex.get_height()));

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

void render_target::apply_viewport() const
{
    const auto& vp = get_viewport();
    glViewport(vp.tl[0], vp.tl[1],
            vp.width(), vp.height());
}

void render_target::clear_buffer(render_target::buffer_attachment ba) const {
    glBindFramebuffer(GL_FRAMEBUFFER, id());

    auto tbi = tex_attachments_.find(ba);
    int clear_pass = 1;
    if(tbi != tex_attachments_.end() &&
            tbi->second->get_trait() & texture::CUBEMAP) {
        clear_pass = 6;
    }

    apply_viewport();

    for(int i = 0; i < clear_pass; i++) {

        if(clear_pass == 6)
            glFramebufferTexture2D(GL_FRAMEBUFFER, em_buffer_attachment_(ba),
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tbi->second->id(), 0);

        if(ba == COLOR_BUFFER) {
            fcolor c = bgcolor_;
            glClearColor(c.r(), c.g(), c.b(), c.a());
            glClear(GL_COLOR_BUFFER_BIT);
        } else if(ba == DEPTH_BUFFER) {
            glDepthFunc(GL_LESS);
            glClearDepth(infdepth_);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }

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
        throw shader_error("Uniform " + name + " has bound.");

    glUseProgram(id());
    GLuint idx = glGetUniformBlockIndex(id(), name.c_str());
    if(idx == GL_INVALID_INDEX)
        warning_log << "Unable to find uniform name " << name << std::endl;
    glUniformBlockBinding(id(), idx, binding);
    glUseProgram(GL_NONE);

    if(binding > max_binding_index_) max_binding_index_ = binding;
    property_binding_[name] = binding;
}

size_t shader::property(const std::string& name,
        const render_assets::property_buffer& buf) {
    auto i = property_binding_.find(name);
    size_t binding = max_binding_index_ + 1;

    if(i != property_binding_.end())
        binding = i->second;
    else property_binding(name, binding);

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

    if(target_->get_depth_test())
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    switch(target_->get_draw_face()) {
        case render_target::NO_FACE:
            glEnable(GL_CULL_FACE); glCullFace(GL_FRONT_AND_BACK); break;
        case render_target::FRONT_FACE:
            glEnable(GL_CULL_FACE); glCullFace(GL_BACK); break;
        case render_target::BACK_FACE:
            glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
        case render_target::BOTH_FACE:
            glDisable(GL_CULL_FACE); break;
    }

    switch(target_->get_blend_func()) {
        case render_target::OVERRIDE_BLEND:
            glDisable(GL_BLEND);
            break;
        case render_target::ALPHA_BLEND:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case render_target::PLUS_BLEND:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            break;
    }

    target_->apply_viewport();

    // bind textures
    size_t tex_num = 0;
    for(auto& e : textures_binding_) {
        e.second->bind_to(tex_num);
        glUniform1i(e.first, tex_num);
        tex_num += 1;
    }

    int render_passes = 1;
    auto tbi = target_->tex_attachments_.find(render_target::COLOR_BUFFER_0);
    if(tbi != target_->tex_attachments_.end() &&
            tbi->second->get_trait() & texture::CUBEMAP) {
        render_passes = 6;
    }

    for(int rp = 0; rp < render_passes; rp++) {
        if(render_passes == 6) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, tbi->second->id());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + rp, tbi->second->id(), 0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
            glUniform1i(glGetUniformLocation(id(), "cubemapPass_"), rp);
        }
        glDrawArrays(GL_TRIANGLES, 0, vat.primitives_count());
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(GL_NONE);
    glUseProgram(GL_NONE);

    for(auto& e : textures_binding_) {
        e.second->bind_to(-1);
    }
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
        std::vector<char> log_str(log_len);
        glGetProgramInfoLog(id(), log_len, &actual_log_len, log_str.data());
        throw shader_error(std::string("Error while linking:\n") + log_str.data());
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
        std::vector<char> log_str(log_len);
        glGetShaderInfoLog(id(), log_len + 1, &actual_log_len, log_str.data());
        throw shader_error(std::string("Error while compiling ")
                + em_shader_type_str_(type_) + ":\n" + log_str.data());
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

