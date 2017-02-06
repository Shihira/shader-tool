#ifndef SHADING_H_INCLUDED
#define SHADING_H_INCLUDED

#include <string>
#include <list>
#include <memory>
#include <unordered_map>

#include "render_assets.h"
#include "exception.h"
#include "reflection.h"

namespace shrtool {

/*
 * shader's behaviour resembles vertex_attr_vector's, they look very different
 * though. I will explain why.
 *
 * 1. Both are consist of some children:
 *     shader - sub_shader
 *     vertex_attr_vector - vertex_attr_buffer.
 * 2. Children of both can be shared, and are in charge of their parents.
 *    Whenever their all parents have gone, they should gone too:
 *     share_sub_shader
 *     share_input
 * 3. After adding children, you have to notice parents to update:
 *     add_sub_shader - link
 *     add_input - updated
 */

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
        COLOR_BUFFER_2,
        COLOR_BUFFER_3,
        COLOR_BUFFER_MAX,
        DEPTH_BUFFER,
    };

    // function render_Texture may alter viewport
    void render_texture(buffer_attachment ba, render_assets::texture& tex);
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

    static render_target& get_screen() {
        return screen;
    }

    float target_ratio() const {
        return float(viewport_[2]) / float(viewport_[3]);
    }

    static void meta_reg() {
        refl::meta_manager::reg_class<render_target>("render_target")
            .function("screen", get_screen)
            .function("set_viewport", &render_target::set_viewport)
            .function("clear_buffer", &render_target::clear_buffer)
            .function("initial_depth", &render_target::initial_depth)
            .function("initial_color", &render_target::initial_color)
            .function("enable_depth_test", &render_target::enable_depth_test)
            .function("is_screen", &render_target::isscr)
            .function("bind_texture", &render_target::render_texture);
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

    typedef std::shared_ptr<sub_shader> sub_shader_ptr;
    typedef std::weak_ptr<sub_shader> weak_sub_shader_ptr;

protected:
    void property_binding(const std::string& name, size_t binding);

    std::unordered_map<shader_type, sub_shader_ptr> sub_shaders_;

    size_t max_binding_index_ = 0;
    std::unordered_map<std::string, size_t> property_binding_;
    std::unordered_map<size_t, const render_assets::texture*> textures_binding_;

    render_target* target_;

public:
    shader() { target(render_target::screen); }

    void add_sub_shader(sub_shader_ptr p);
    sub_shader& add_sub_shader(shader_type t);

    sub_shader_ptr share_sub_shader(shader_type t) {
        auto i = sub_shaders_.find(t);
        if(i != sub_shaders_.end()) return i->second;
        return sub_shader_ptr();
    }

    bool has_sub_shader(shader_type t) { return bool(share_sub_shader(t)); }

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
};

class sub_shader : public lazy_id_object_<sub_shader> {
    shader::shader_type type_;
public:
    sub_shader(shader::shader_type t) : type_(t) { }

    void compile(const std::string& s);

    shader::shader_type type() const { return type_; }
    void type(shader::shader_type t) { type_ = t; }

    id_type create_object() const;
    void destroy_object(id_type i) const;
};

class vertex_attr_vector : public lazy_id_object_<vertex_attr_vector> {
public:
    typedef std::shared_ptr<render_assets::vertex_attr_buffer> buffer_ptr;
    typedef std::weak_ptr<render_assets::vertex_attr_buffer> weak_buffer_ptr;

protected:
    mutable size_t primitives_count_;
    // we use shared_ptr to manage buffers, which enables users to share
    // buffers between different vertex_attr_vectors
    std::unordered_map<size_t, buffer_ptr> bindings_;

public:
    // create a new buffer with old one overridden
    void input(uint32_t loc, buffer_ptr buf) {
        bindings_[loc] = buf;
    }

    render_assets::vertex_attr_buffer& add_input(size_t loc, size_t size = 0) {
        buffer_ptr p(new render_assets::vertex_attr_buffer(size));
        input(loc, p);
        return *p;
    }

    buffer_ptr share_input(size_t loc) {
        auto i = bindings_.find(loc);
        if(i != bindings_.end())
            return i->second;
        return buffer_ptr();
    }

    bool has_input(size_t loc) { return bool(share_input(loc)); }

    void updated();
    void updated(size_t loc);

    size_t primitives_count() const { return primitives_count_; }
    void primitives_count(size_t p) const { primitives_count_ = p; }

    id_type create_object() const;
    void destroy_object(id_type i) const;
};

}


#endif // SHADING_H_INCLUDED

