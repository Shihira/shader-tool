#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <string>
#include <list>
#include <unordered_map>

#include "render_assets.h"
#include "exception.h"

namespace shrtool {

class render_target : public __lazy_id_object<render_target> {
protected:
    bool _screen = false;
    float _init_color[4] = { 0, 0, 0, 1 };
    size_t _viewport[4] = { 0, 0, 0, 0 };
    float _init_depth = 1;
    bool _enabled_depth_test = false;

    render_target(bool scr) : _screen(scr) { }

public:
    render_target() : _screen(false) { }

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
    void initial_depth(float d) { _init_depth = d; }
    void initial_color(float r, float g, float b, float a) {
        _init_color[0] = r; _init_color[1] = g;
        _init_color[2] = b; _init_color[3] = a;
    }
    void enable_depth_test(bool e) { _enabled_depth_test = e; }
    bool is_depth_test_enabled() const { return _enabled_depth_test; }

    id_type create_object() const;
    void destroy_object(id_type i) const;

    bool isscr() const { return _screen; }

    float target_ratio() const {
        return float(_viewport[2]) / float(_viewport[3]);
    }

    static render_target screen;
};

class sub_shader;
class vertex_attr_vector;

class shader : public __lazy_id_object<shader> {
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
    void target(render_target& tgt) { _target = &tgt; }
    void draw(const vertex_attr_vector& vat) const;

    void link();

    id_type create_object() const;
    void destroy_object(id_type i) const;

protected:
    std::list<sub_shader> _subshader_list;
    std::unordered_map<std::string, size_t> _property_binding;
    size_t _max_binding_index = 0;

    render_target* _target;
};

class sub_shader : public __lazy_id_object<sub_shader> {
    shader::shader_type __type;
public:
    sub_shader(shader::shader_type t) : __type(t) { }

    void compile(const std::string& s);

    id_type create_object() const;
    void destroy_object(id_type i) const;
};

class vertex_attr_vector : public __lazy_id_object<vertex_attr_vector> {
protected:
    mutable size_t _primitives_count;

public:
    void input(uint32_t loc,const render_assets::buffer& buf) const;

    size_t primitives_count() const { return _primitives_count; }
    void primitives_count(const size_t& p) const { _primitives_count = p; }

    id_type create_object() const;
    void destroy_object(id_type i) const;
};

}


#endif // SHADER_H_INCLUDED

