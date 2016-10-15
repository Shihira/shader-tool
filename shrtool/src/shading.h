#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <string>
#include <list>
#include <unordered_map>

#include "render_assets.h"
#include "exception.h"

namespace shrtool {

class render_target : public lazy_id_object_<render_target> {
protected:
    bool screen_ = false;
    float init_color_[4] = { 0, 0, 0, 1 };
    size_t viewport_[4] = { 0, 0, 0, 0 };
    float init_depth_ = 1;
    bool enabled_depth_test_ = false;

    render_target(bool scr) : screen_(scr) { }

public:
    render_target() : screen_(false) { }

    enum buffer_attachment {
        COLOR_BUFFER,
        COLOR_BUFFER_0,
        COLOR_BUFFER_1,
        DEPTH_BUFFER,
    };

    // function render_Texture may alter viewport
    void render_texture(buffer_attachment ba, render_assets::texture2d& tex);
    void set_viewport(size_t x, size_t y, size_t w, size_t h);
    void clear_buffer(buffer_attachment ba);
    void initial_depth(float d) { init_depth_ = d; }
    void initial_color(float r, float g, float b, float a) {
        init_color_[0] = r; init_color_[1] = g;
        init_color_[2] = b; init_color_[3] = a;
    }
    void enable_depth_test(bool e) { enabled_depth_test_ = e; }
    bool is_depth_test_enabled() const { return enabled_depth_test_; }

    id_type create_object() const;
    void destroy_object(id_type i) const;

    bool isscr() const { return screen_; }

    float target_ratio() const {
        return float(viewport_[2]) / float(viewport_[3]);
    }

    static render_target screen;
};

class sub_shader;
class vertex_attr_vector;

class shader : public lazy_id_object_<shader> {
public:
    enum shader_type {
        VERTEX,
        GEOMETRY,
        FRAGMENT,
    };

    shader() { target(render_target::screen); }

    sub_shader& add_sub_shader(shader_type t);
    void property_binding(const std::string& name, size_t binding);
    // auto bind property with its name
    // and return the allocated binding index
    size_t property(const std::string& name,
            const render_assets::property_buffer& buf);
    void property(size_t binding,
            const render_assets::property_buffer& buf);
    size_t property(const std::string& name,
            const render_assets::texture& tex);
    void property(size_t binding,
            const render_assets::texture& tex);
    void target(render_target& tgt) { target_ = &tgt; }
    void draw(const vertex_attr_vector& vat) const;

    void link();

    id_type create_object() const;
    void destroy_object(id_type i) const;

protected:
    std::list<sub_shader> subshader_list_;
    std::unordered_map<std::string, size_t> property_binding_;
    std::unordered_map<size_t, const render_assets::texture*> textures_binding_;
    size_t max_binding_index_ = 0;

    render_target* target_;
};

class sub_shader : public lazy_id_object_<sub_shader> {
    shader::shader_type type_;
public:
    sub_shader(shader::shader_type t) : type_(t) { }

    void compile(const std::string& s);

    id_type create_object() const;
    void destroy_object(id_type i) const;
};

class vertex_attr_vector : public lazy_id_object_<vertex_attr_vector> {
protected:
    mutable size_t primitives_count_;

public:
    void input(uint32_t loc,const render_assets::buffer& buf) const;

    size_t primitives_count() const { return primitives_count_; }
    void primitives_count(const size_t& p) const { primitives_count_ = p; }

    id_type create_object() const;
    void destroy_object(id_type i) const;
};

}


#endif // SHADER_H_INCLUDED

