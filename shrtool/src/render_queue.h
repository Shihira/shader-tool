#ifndef RENDER_QUEUE_H_INCLUDED
#define RENDER_QUEUE_H_INCLUDED

#include <map>
#include <set>
#include <list>
#include <algorithm>

#include "shading.h"
#include "providers.h"

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

    virtual void render() const = 0;
};

class void_render_task : public render_task {
    virtual void render() const { }
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
    struct prop_maybe {
        enum { PROP_BUF, TEX } what;
        union {
            render_assets::property_buffer* prop_buf;
            render_assets::texture* tex;
        } value;
    };

    shader* shr_ = nullptr;
    render_target* target_ = nullptr;
    vertex_attr_vector* attr_ = nullptr;
    std::map<std::string, prop_maybe> prop_;

    mutable bool prop_buf_ref_changed = true;

public:
    void set_shader(shader& s) {
        shr_ = &s;
        prop_buf_ref_changed = true;
    }

    void set_target(render_target& t) { target_ = &t; }
    void set_attributes(vertex_attr_vector& v) { attr_ = &v; }
    void reset() {
        shr_ = nullptr;
        target_ = nullptr;
        attr_ = nullptr;
        prop_.clear();
        prop_buf_ref_changed = true;
    }

    void set_property(const std::string& name,
            render_assets::property_buffer& p);
    void set_property(const std::string& name,
            render_assets::texture& p);

    void render() const override;
};

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
        std::map<size_t, render_assets::property_buffer> property_bindings;

        template<typename Prov, typename T, typename Bindings>
        static typename Prov::output_type& set_binding(
                const T& obj, Bindings& b) {
            size_t hashcode = reinterpret_cast<size_t>(&obj);
            auto res = b.find(hashcode);
            if(res == b.end()) {
                Prov::update(obj, b[hashcode], true);
                res = b.find(hashcode);
            }

            return res->second;
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

    template<typename T, typename Enabled = typename std::enable_if<
        !std::is_same<T, shader>::value>::type>
    void set_shader(T& obj) {
        shader& r = provider_bindings::set_binding<provider<T, shader>>(
                obj, pb_.shader_bindings);
        shader_updater = [&obj, &r]() {
            provider<T, shader>::update(obj, r, false);
        };
    }

    using shader_render_task::set_target;
    template<typename T>
    void set_target(T& obj) {
        render_target& r = provider_bindings::set_binding
            <provider<T, render_target>>(obj, pb_.target_bindings);
        set_target(r);
        target_updater = [&obj, &r]() {
            provider<T, render_target>::update(obj, r, false);
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

    template<typename T>
    void set_tex2d_property(const std::string& name, T& obj) {
        render_assets::texture2d& r = provider_bindings::set_binding
            <provider<T, render_assets::texture2d>>(obj, pb_.texture2d_bindings);
        set_property(name, r);
        prop_updater[name] = [&obj, &r]() {
            provider<T, render_assets::texture2d>::update(obj, r, false);
        };
    }

    void set_tex2d_property(const std::string& name, render_assets::texture2d& tex) {
        shader_render_task::set_property(name, tex);
    }

    void update() const {
        shader_updater();
        target_updater();
        attr_updater();

        for(auto& pu : prop_updater)
            pu.second();
    }

    void render() const override {
        update();
        shader_render_task::render();
    }
};

}

#endif // RENDER_QUEUE_H_INCLUDED
