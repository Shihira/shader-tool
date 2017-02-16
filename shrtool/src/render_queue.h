#ifndef RENDER_QUEUE_H_INCLUDED
#define RENDER_QUEUE_H_INCLUDED

#include <map>
#include <set>
#include <list>
#include <algorithm>

#include "shading.h"
#include "providers.h"
#include "reflection.h"

namespace shrtool {

/*
 * render_task is an object that has a member function `void render()`.
 *
 * What is interesting is its dependency support. See below.
 */

class render_task {
    friend class queue_render_task;

    std::set<const render_task*> deps_;
    mutable uint32_t sort_info__; // used while sorting

public:
    void rely_on(const render_task& r) { deps_.insert(&r); }
    void reclaim_reliance(const render_task& r) {
        auto i = deps_.find(&r);
        if(i != deps_.end()) deps_.erase(i);
    }
    bool is_successor(const render_task& r) {
        return deps_.find(&r) != deps_.end();
    }

    virtual void render() const { }

    static void meta_reg_() {
        refl::meta_manager::reg_class<render_task>("rtask")
            .enable_construct<>()
            .function("rely_on", &render_task::rely_on)
            .function("reclaim", &render_task::reclaim_reliance)
            .function("is_successor", &render_task::is_successor);
    }
};

typedef render_task void_render_task;

class proc_render_task : public render_task {
    std::function<void()> func_;

public:
    proc_render_task() { }
    proc_render_task(std::function<void()> f) : func_(f) { }

    void set_proc(std::function<void()> f) { func_ = f; }
    void render() const override {
        if(func_) func_();
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<proc_render_task>("proc_rtask")
            .enable_construct<>()
            .enable_construct<std::function<void()>>()
            .enable_base<render_task>()
            .function("set_proc", &proc_render_task::set_proc);
    }
};

/*
 * It is not that sorting is forced before rendering, because it is quite costly.
 * When you chose not to sort, you are supposed to have the ability to
 * responsible for the consequence. Seize the right moment to sort, and to alter
 * it no longer. Note that (topology) sort has a complexity of O(V+E).
 *
 * I will recommend you to group tasks whose dependency relationships are mostly
 * unchanged into one queue, have it sorted, and add as a task into another
 * queue, where deps are rather simpler. See, queues can be splited into levels.
 *
 * Have fun with this functionality :-)
 *
 * For actual rendering, switching property set and attribute set may be a lot
 * more costly. TODO: let sorting detect and reduce state switching.
 */

class queue_render_task : public render_task {
public:
    std::list<const render_task*> tasks;

    void sort();
    void render() const override;

    typedef std::list<const render_task*>::iterator iterator;
    typedef std::list<const render_task*>::const_iterator const_iterator;

    iterator begin() { return tasks.begin(); }
    iterator end() { return tasks.end(); }
    const_iterator begin() const { return tasks.begin(); }
    const_iterator end() const { return tasks.end(); }
    const_iterator cbegin() const { return tasks.cbegin(); }
    const_iterator cend() const { return tasks.cend(); }

    void append(const render_task& t) { tasks.push_back(&t); }
    void remove(const render_task& t) { tasks.remove(&t); }
    size_t size() const { return tasks.size(); }
    void clear() { tasks.clear(); }

    static void meta_reg_() {
        refl::meta_manager::reg_class<queue_render_task>("queue_rtask")
            .enable_construct<>()
            .enable_base<render_task>()
            .function("sort", &queue_render_task::sort)
            .function("append", &queue_render_task::append)
            .function("size", &queue_render_task::size)
            .function("clear", &queue_render_task::clear)
            .function("remove", &queue_render_task::remove);
    }
};

/*
 * A shader_render_tasks takes four tuple <S, A, P, T>, and is usually repre-
 * sented as a function-invoke-like `T = S(A, P)`, where:
 *
 * - T for render target
 * - S for shader
 * - A for attributes
 * - P for a set of properties
 *
 * `void render()` does the invoke.
 */

class shader_render_task : public render_task {
    shader* shr_ = nullptr;
    render_target* target_;
    vertex_attr_vector* attr_ = nullptr;
    std::map<std::string, render_assets::property_buffer*> prop_;
    std::map<std::string, render_assets::texture*> prop_tex_;

    mutable bool prop_buf_ref_changed = true;

public:
    shader_render_task() { reset(); }

    render_target* get_target() { return target_; }
    shader* get_shader() { return shr_; }

    void set_shader(shader& s) {
        shr_ = &s;
        prop_buf_ref_changed = true;
    }

    void set_target(render_target& t) { target_ = &t; }
    void set_attributes(vertex_attr_vector& v) { attr_ = &v; }
    void reset() {
        shr_ = nullptr;
        target_ = &render_target::screen;
        attr_ = nullptr;
        prop_.clear();
        prop_buf_ref_changed = true;
    }

    void set_property(const std::string& name,
            render_assets::property_buffer& p);
    void set_texture_property(const std::string& name,
            render_assets::texture& p);

    void render() const override;
};

}

#include "shader_parser.h"
#include "properties.h"
#include "mesh.h"

namespace shrtool {

////////////////////////////////////////////////////////////////////////////////
// predesigned solution

/*
 * Lazy update: Generally a GPU object is update whenever the source object is
 * modified. But when the source object is modified serveral times, it would act
 * in a very low performance. For example, a property object has serveral items
 * to be set separately, but when you set an item, I cannot tell whether you are
 * going to set other items. Based on this consideration, GPU updates are done
 * right before rendering and no where else.
 */

class provided_render_task : public shader_render_task {
public:
    struct provider_bindings {
        std::map<size_t, shader> shader_bindings;
        std::map<size_t, render_target> target_bindings;
        std::map<size_t, vertex_attr_vector> attr_bindings;
        std::map<size_t, render_assets::texture2d> texture2d_bindings;
        std::map<size_t, render_assets::texture_cubemap> texture_cubemap_bindings;
        std::map<size_t, render_assets::property_buffer> property_bindings;

        template<typename Prov, typename T, typename Bindings>
        static typename Prov::output_type& set_binding(
                T& obj, Bindings& b) {
            size_t hashcode = reinterpret_cast<size_t>(&obj);
            auto res = b.find(hashcode);
            if(res == b.end()) {
                Prov::update(obj, b[hashcode], true);
                res = b.find(hashcode);
            }

            return res->second;
        }

        std::map<size_t, render_assets::texture2d>& get_binding(
                render_assets::texture2d*) {
            return texture2d_bindings;
        }

        std::map<size_t, render_assets::texture_cubemap>& get_binding(
                render_assets::texture_cubemap*) {
            return texture_cubemap_bindings;
        }
    };

private:
    provider_bindings& pb_;

    std::function<void()> shader_updater;
    std::function<void()> target_updater;
    std::function<void()> attr_updater;
    std::map<std::string, std::function<void()>> prop_updater;

public:
    provided_render_task(provider_bindings& pb) : pb_(pb) { }

    using shader_render_task::set_shader;
    template<typename T, typename Enabled = typename std::enable_if<
        !std::is_same<T, shader>::value>::type>
    void set_shader(T& obj) {
        shader& r = provider_bindings::set_binding<provider<T, shader>>(
                obj, pb_.shader_bindings);
        set_shader(r);
        shader_updater = [&obj, &r]() {
            provider<T, shader>::update(obj, r, false);
        };
    }

    using shader_render_task::set_attributes;
    template<typename T>
    void set_attributes(T& obj) {
        vertex_attr_vector& r = provider_bindings::set_binding
            <provider<T, vertex_attr_vector>>(obj, pb_.attr_bindings);
        set_attributes(r);
        attr_updater = [&obj, &r]() {
            provider<T, vertex_attr_vector>::update(obj, r, false);
        };
    }

    using shader_render_task::set_property;
    template<typename T>
    void set_property(const std::string& name, T& obj) {
        render_assets::property_buffer& r = provider_bindings::set_binding
            <provider<T, render_assets::property_buffer>>(obj, pb_.property_bindings);
        set_property(name, r);
        prop_updater[name] = [&obj, &r]() {
            provider<T, render_assets::property_buffer>::update(obj, r, false);
        };
    }

    template<typename Tex, typename T,
        typename Enable = typename std::enable_if<
            std::is_base_of<render_assets::texture, Tex>::value>::type>
    void set_texture_property(const std::string& name, T& obj) {
        Tex& r = provider_bindings::set_binding<provider<T, Tex>>(
                obj, pb_.get_binding((Tex*)nullptr));
        set_texture_property(name, r);
        prop_updater[name] = [&obj, &r]() {
            provider<T, Tex>::update(obj, r, false);
        };
    }

    void set_texture_property(const std::string& name, render_assets::texture& tex) {
        shader_render_task::set_texture_property(name, tex);
    }

    void update() const {
        if(shader_updater) shader_updater();
        if(target_updater) target_updater();
        if(attr_updater) attr_updater();

        for(auto& pu : prop_updater)
            if(pu.second) pu.second();
    }

    void render() const override {
        update();
        shader_render_task::render();
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<provided_render_task>("shading_rtask")
            .enable_base<render_task>()
            .function("set_shader", &provided_render_task::set_shader<shader_info>)
            .function("set_property", &provided_render_task::set_property<dynamic_property>)
            .function("set_property_camera", &provided_render_task::set_property<camera>)
            .function("set_property_transfrm", &provided_render_task::set_property<transfrm>)
            .function("set_attributes", &provided_render_task::set_attributes<mesh_indexed>)
            .function("set_texture2d_image", &provided_render_task::set_texture_property<render_assets::texture2d, image>)
            .function("set_texture_cubemap_image", &provided_render_task::set_texture_property<render_assets::texture_cubemap, image>)
            .function("set_texture", static_cast<void(provided_render_task::*)(const std::string&, render_assets::texture&)>(&provided_render_task::set_texture_property))
            .function("set_target", static_cast<void(provided_render_task::*)(render_target&)>(&provided_render_task::set_target));
    }
};

}

#endif // RENDER_QUEUE_H_INCLUDED
