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
    prop_[name] = &p;

    prop_buf_ref_changed = true;
}

void shader_render_task::set_texture_property(const std::string& name,
        render_assets::texture& p) {
    prop_tex_[name] = &p;

    prop_buf_ref_changed = true;
}

void shader_render_task::render() const {
    IF_FALSE_RET(shr_)
    IF_FALSE_RET(target_)
    IF_FALSE_RET(attr_)
    IF_FALSE_RET(prop_.size())

    if(prop_buf_ref_changed) {
        for(auto& e : prop_tex_)
            shr_->property(e.first, *e.second);
        for(auto& e : prop_)
            shr_->property(e.first, *e.second);

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

