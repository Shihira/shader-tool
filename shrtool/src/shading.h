#ifndef SHADING_H_INCLUDED
#define SHADING_H_INCLUDED

#include <string>
#include <list>
#include <memory>
#include <unordered_map>

#include "render_assets.h"
#include "exception.h"
#include "reflection.h"
#include "utilities.h"
#include "traits.h"

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

/*
 * render_target is not only a frame buffer, it saves all states that is need
 * to be set before a pass of render, like viewport and clear color, which are
 * mostly global states in OpenGL. And yes, those operations will mostly be done
 * in shader::draw.
 */

class render_target : public lazy_id_object_<render_target> {
public:
    enum buffer_attachment {
        COLOR_BUFFER,
        COLOR_BUFFER_0,
        COLOR_BUFFER_1,
        COLOR_BUFFER_2,
        COLOR_BUFFER_3,
        COLOR_BUFFER_MAX,
        DEPTH_BUFFER,
    };

protected:
    std::map<buffer_attachment, render_assets::texture*> tex_attachments_;

public:
    PROPERTY_RW(color, bgcolor)
    PROPERTY_RW(float, infdepth)
    PROPERTY_R_DW(rect, viewport, PROP_ASSERT(this != &screen,
                "Viewport is unchangable for default screen"), {})
    PROPERTY_RW(bool, depth_test)

    render_target() : infdepth_(1), depth_test_(false) { }

    void attach_texture(buffer_attachment ba, render_assets::texture& tex);
    void force_set_viewport_(const rect& r) { viewport_ = r; }
    virtual void apply_viewport() const;

    render_assets::texture* get_attachment(buffer_attachment ba) {
        auto i = tex_attachments_.find(ba);
        if(i != tex_attachments_.end())
            return i->second;
        return nullptr;
    }

    void clear_buffer(buffer_attachment ba) const;

    id_type create_object() const;
    void destroy_object(id_type i) const;

    static render_target& get_default_screen() {
        return screen;
    }

    double target_ratio() const {
        return viewport_.width() / viewport_.height();
    }

    bool is_screen() const {
        return tex_attachments_.empty() && vacuum();
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<render_target>("render_target")
            .function("screen", get_default_screen)
            .function("set_viewport", &render_target::set_viewport)
            .function("get_viewport", &render_target::get_viewport)
            .function("clear_buffer", &render_target::clear_buffer)
            .function("set_infdepth", &render_target::set_infdepth)
            .function("get_infdepth", &render_target::get_infdepth)
            .function("set_bgcolor", &render_target::set_bgcolor)
            .function("get_bgcolor", &render_target::get_bgcolor)
            .function("set_depth_test", &render_target::set_depth_test)
            .function("get_depth_test", &render_target::get_depth_test)
            .function("is_screen", &render_target::is_screen)
            .function("attach_texture", &render_target::attach_texture);
    }

    // This is a global screen. Its viewport will always be maintained at the
    // window size.
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

struct camera : render_target {
    transfrm& transformation() { return tf_; }
    const transfrm& transformation() const { return tf_; }

    PROPERTY_RW(bool, auto_viewport)
    PROPERTY_R_DW(float, visible_angle, {}, PROP_MARKCHG(changed_))
    PROPERTY_R_DW(float, near_clip_plane, {}, PROP_MARKCHG(changed_))
    PROPERTY_R_DW(float, far_clip_plane, {}, PROP_MARKCHG(changed_))
    PROPERTY_GENERIC_RW(float, aspect, {
        if(!aspect_) return auto_viewport_ ?
            render_target::screen.get_viewport().ratio() :
            get_viewport().ratio(); },
        {}, {}, PROP_MARKCHG(changed_))

    camera() :
        auto_viewport_(true),
        visible_angle_(M_PI / 2),
        near_clip_plane_(1),
        far_clip_plane_(100),
        aspect_(0)
    { }

    camera(camera&& c) :
        render_target(std::move(c)),
        auto_viewport_(c.auto_viewport_),
        visible_angle_(c.visible_angle_),
        near_clip_plane_(c.near_clip_plane_),
        far_clip_plane_(c.far_clip_plane_),
        aspect_(c.aspect_)
    { }

    camera(const camera& c) = delete;

    bool is_changed() const { return changed_ || tf_.is_changed(); }
    void mark_applied() { changed_ = false; tf_.mark_applied(); }

    const math::mat4& get_view_mat() const {
        return transformation().get_inverse_mat();
    }

    math::mat4 calc_projection_mat() const {
        return math::tf::perspective(
            get_visible_angle() / 2,
            get_aspect(),
            get_near_clip_plane(),
            get_far_clip_plane());
    }

    math::mat4 calc_vp_mat() const {
        return calc_projection_mat() *
            transformation().get_inverse_mat();
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<camera>("camera")
            .enable_construct<>()
            .enable_base<render_target>()
            .function("transformation", static_cast<transfrm&(camera::*)()>(&camera::transformation))
            .function("get_visible_angle", &camera::get_visible_angle)
            .function("get_near_clip_plane", &camera::get_near_clip_plane)
            .function("get_far_clip_plane", &camera::get_far_clip_plane)
            .function("get_aspect", &camera::get_aspect)
            .function("set_visible_angle", &camera::set_visible_angle)
            .function("set_near_clip_plane", &camera::set_near_clip_plane)
            .function("set_far_clip_plane", &camera::set_far_clip_plane)
            .function("set_aspect", &camera::set_aspect)
            .function("get_view_mat", &camera::get_view_mat)
            .function("calc_projection_mat", &camera::calc_projection_mat)
            .function("calc_vp_mat", &camera::calc_vp_mat);
    }

    void apply_viewport() const override;

protected:
    transfrm tf_;
    bool changed_ = true;
};

template<>
struct prop_trait<camera> {
    typedef camera input_type;
    typedef float value_type;
    typedef shrtool::indirect_tag transfer_tag;

    static size_t size(const input_type& i) {
        return sizeof(float) * 32;
    }

    static bool is_changed(const input_type& i) {
        return i.is_changed();
    }

    static void mark_applied(input_type& i) {
        i.mark_applied();
    }

    static void copy(const input_type& i, value_type* o) {
        value_type* o_1 = o + 16;
        const math::mat4& v = i.get_view_mat(),
              vp = i.calc_vp_mat();

        for(size_t c = 0; c < 4; c++)
            for(size_t r = 0; r < 4; r++) {
                *(o++) = v.at(r, c);
                *(o_1++) = vp.at(r, c);
            }
    }
};

}


#endif // SHADING_H_INCLUDED

