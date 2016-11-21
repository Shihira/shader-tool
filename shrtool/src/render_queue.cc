#include "render_queue.h"
#include "exception.h"

#include <vector>
#include <iostream>

#define IF_FALSE_RET(x) if(!x) return;

namespace shrtool {

void queue_render_task::sort() {
    struct sort_node {
        const render_task* t;
        size_t indegree;
        std::vector<sort_node*> suc;
    };

    std::vector<sort_node> adj_table;
    std::list<sort_node*> resolved_nodes;
    std::list<const render_task*> sorted_tasks;

    for(auto* t : tasks) {
        t->sort_info__ = adj_table.size();
        adj_table.emplace_back();
        adj_table.back().t = t;
        adj_table.back().indegree = t->deps_.size();
    }

    for(auto& n : adj_table) {
        for(auto* dep : n.t->deps_) {
            adj_table[dep->sort_info__].suc.push_back(&n);
        }
        if(n.indegree == 0)
            resolved_nodes.push_back(&n);
    }

    while(!resolved_nodes.empty()) {
        sort_node* res = resolved_nodes.front();
        sorted_tasks.push_back(res->t);
        for(auto* n : res->suc) {
            n->indegree -= 1;
            if(n->indegree == 0) resolved_nodes.push_back(n);
        }
        resolved_nodes.pop_front();
    }

    if(sorted_tasks.size() != tasks.size())
        throw resolve_error("Cannot resolve the rendering order");

    tasks = std::move(sorted_tasks);
}

void shader_render_task::set_property(const std::string& name,
        render_assets::property_buffer& p) {
    prop_maybe pm;
    pm.what = prop_maybe::PROP_BUF;
    pm.value.prop_buf = &p;
    prop_[name] = pm;

    prop_buf_ref_changed = true;
}

void shader_render_task::set_property(const std::string& name,
        render_assets::texture& p) {
    prop_maybe pm;
    pm.what = prop_maybe::TEX;
    pm.value.tex = &p;
    prop_[name] = pm;

    prop_buf_ref_changed = true;
}

void shader_render_task::render() const {
    IF_FALSE_RET(shr_)
    IF_FALSE_RET(target_)
    IF_FALSE_RET(attr_)
    IF_FALSE_RET(prop_.size())

    if(prop_buf_ref_changed) {
        for(auto& e : prop_) {
            if(e.second.what == prop_maybe::PROP_BUF)
                shr_->property(e.first, *e.second.value.prop_buf);
            if(e.second.what == prop_maybe::TEX)
                shr_->property(e.first, *e.second.value.tex);
        }
        prop_buf_ref_changed = false;
    }

    shr_->target(*target_);

    shr_->draw(*attr_);
}

void queue_render_task::render() const {
    for(const render_task* t : tasks)
        t->render();
}

}

