#include <iostream>
#include <chrono>

#include "common/exception.h"
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

    auto att = em_buffer_attachment_(ba);

    if((tex.get_trait() & texture::CUBEMAP) == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, id());
        tex.attach_to(att);
        if(ba != DEPTH_BUFFER)
            glDrawBuffer(att);
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, id());
        tex.attach_to((att << 3) + 0);
        if(ba != DEPTH_BUFFER)
            glDrawBuffer(att);
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

        if(sub_targets_.size() > 5) {
            glDeleteFramebuffers(sub_targets_.size() - 5,
                    &sub_targets_[5]);
            sub_targets_.resize(5);
        } else if(sub_targets_.size() < 5) {
            size_t org_size = sub_targets_.size();
            sub_targets_.resize(5);
            glGenFramebuffers(5 - org_size, &sub_targets_[org_size]);
        }

        for(size_t i = 1; i < 6; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, sub_targets_[i - 1]);
            tex.attach_to((att << 3) + i);
            glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
        }
    }

    set_viewport(rect::from_size(tex.get_width(), tex.get_height()));
}

void render_target::apply_properties() const
{
    const auto& vp = get_viewport();
    glViewport(vp.tl[0], vp.tl[1],
            vp.width(), vp.height());

    if(get_depth_test())
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if(get_wire_frame())
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    switch(get_draw_face()) {
        case NO_FACE:
            glEnable(GL_CULL_FACE); glCullFace(GL_FRONT_AND_BACK); break;
        case FRONT_FACE:
            glEnable(GL_CULL_FACE); glCullFace(GL_BACK); break;
        case BACK_FACE:
            glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
        case BOTH_FACE:
            glDisable(GL_CULL_FACE); break;
    }

    switch(get_blend_func()) {
        case OVERRIDE_BLEND:
            glDisable(GL_BLEND);
            break;
        case ALPHA_BLEND:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case PLUS_BLEND:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            break;
    }

}

void render_target::clear_buffer(render_target::buffer_attachment ba) const {
    glBindFramebuffer(GL_FRAMEBUFFER, id());

    const auto& vp = get_viewport();
    glViewport(vp.tl[0], vp.tl[1],
            vp.width(), vp.height());

    
    for(int i = 0; i < sub_targets_.size() + 1; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, i == 0 ? id() : sub_targets_[i-1]);
        if(ba == COLOR_BUFFER) {
            fcolor c = bgcolor_;
            glClearColor(c.r(), c.g(), c.b(), c.a());
            glClear(GL_COLOR_BUFFER_BIT);
        } else if(ba == DEPTH_BUFFER) {
            glDepthFunc(GL_LESS);
            glClearDepth(infdepth_);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    }
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

    target_->apply_properties();

    // bind textures
    size_t tex_num = 0;
    for(auto& e : textures_binding_) {
        e.second->bind_to(tex_num);
        glUniform1i(e.first, tex_num);
        tex_num += 1;
    }

    if(!target_->has_multiple_pass()) {
        glBindFramebuffer(GL_FRAMEBUFFER, target_->id());
        glDrawArrays(GL_TRIANGLES, 0, vat.primitives_count());
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    } else {
        for(int i = 0; i < target_->sub_targets_.size() + 1; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER,
                i == 0 ? target_->id() : target_->sub_targets_[i-1]);
            glUniform1i(glGetUniformLocation(id(), "renderPass_"), i);
            glDrawArrays(GL_TRIANGLES, 0, vat.primitives_count());
        }
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    }

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

std::vector<math::mat4> camera::get_cubemap_view_mat() const {
    std::vector<math::mat4> ms(6);
    constexpr int
        pos_x = 0, pos_y = 2, pos_z = 4,
        neg_x = 1, neg_y = 3, neg_z = 5;
    ms[pos_z] = math::tf::rotate(math::PI, math::tf::yOz);
    ms[pos_x] = math::tf::rotate(-math::PI/2, math::tf::zOx) * ms[pos_z];
    ms[neg_z] = math::tf::rotate(-math::PI/2, math::tf::zOx) * ms[pos_x];
    ms[neg_x] = math::tf::rotate(-math::PI/2, math::tf::zOx) * ms[neg_z];
    ms[pos_y] = math::tf::rotate(-math::PI/2, math::tf::yOz) * ms[pos_z];
    ms[neg_y] = math::tf::rotate( math::PI/2, math::tf::yOz) * ms[pos_z];
    return std::move(ms);
}

}

