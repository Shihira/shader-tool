#include <vector>
#include <iostream>
#include <chrono>

#include "render_queue.h"

#define IF_FALSE_RET(x, msg) if(!x) { warning_log << msg << std::endl; return; }

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
}

void shader_render_task::set_texture_property(const std::string& name,
        render_assets::texture& p) {
    prop_tex_[name] = &p;
}

void shader_render_task::render() const {
    IF_FALSE_RET(shr_, "no shader is set")
    IF_FALSE_RET(target_, "no target is set")
    IF_FALSE_RET(attr_, "no attribute is set")

    for(auto& e : prop_tex_)
        shr_->property(e.first, *e.second);
    for(auto& e : prop_)
        shr_->property(e.first, *e.second);

    shr_->target(*target_);

    shr_->draw(*attr_);
}

void queue_render_task::render() const {

    for(const render_task* t : tasks) {
        auto beg = std::chrono::system_clock::now();
        t->render();
        auto dur = std::chrono::system_clock::now() - beg;

        if(prof_enabled_) {
            auto i = prof_.find(t);
            if(i == prof_.end()) {
                prof_[t] = std::chrono::duration_cast<
                    std::chrono::microseconds>(dur).count();
            } else {
               i->second = i->second * 20 + std::chrono::duration_cast<
                   std::chrono::microseconds>(dur).count();
               i->second /= 21;
            }
        }
    }
}

void queue_render_task::print_profile_log() const
{
    int idx = 0;

    for(const render_task* t : tasks) {
        auto& log = info_log << "Profiling info: " << t <<
            " (index " << idx << ") ";
        auto i = prof_.find(t);
        if(i == prof_.end())
            log << "has no record." << std::endl;
        else
            log << float(i->second / 1000) << " ms." << std::endl;

        idx += 1;
    }
}

void provided_render_task::update() const
{
    if(shader_updater) shader_updater();
    if(target_updater) target_updater();
    if(attr_updater) attr_updater();

    for(auto& pu : prop_updater) {
        if(pu.second) pu.second();
    }
}

void provided_render_task::render() const
{
    update();
    shader_render_task::render();
}

}

