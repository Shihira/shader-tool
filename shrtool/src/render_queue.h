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

    void set_render_target(render_target& t) { target_ = &t; }
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

    template<typename ImportType>
    void import(const ImportType& imp) {
    }
};

}

#endif // RENDER_QUEUE_H_INCLUDED
