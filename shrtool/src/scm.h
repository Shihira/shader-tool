#ifndef SCM_H_INCLUDED
#define SCM_H_INCLUDED

#include <guile/2.0/libguile.h>

#include "res_pool.h"
#include "shader_parser.h"
#include "properties.h"
#include "render_queue.h"

namespace shrtool {

struct scm_item_trait {
    virtual const char* glsl_type_name(SCM obj) const = 0;
    virtual void copy(SCM obj, uint8_t* data) const = 0;
    virtual size_t align(SCM obj) const = 0;
    virtual size_t size(SCM obj) const = 0;

    static const scm_item_trait* query_trait(SCM obj);
};

template<>
struct dynamic_property_storage<SCM> : dynamic_property_storage_base {
    dynamic_property_storage(SCM v) :
        t_(scm_item_trait::query_trait(v)) { }

    const SCM& value() const { return value_; }
    void value(const SCM& o) {
        value_ = o;
        t_ = scm_item_trait::query_trait(o);
    }

    const char* glsl_type_name() const override {
        return t_->glsl_type_name(value());
    }

    void copy(uint8_t* data) const override {
        t_->copy(value(), data);
    }

    size_t align(size_t start_at) const override {
        return start_at % t_->align(value()) ?
            (start_at / t_->align(value()) + 1) *
            t_->align(value()) : start_at;
    }

    size_t size() const override {
        return t_->size(value());
    }

    virtual ~dynamic_property_storage() { }

private:
    const scm_item_trait* t_;
    SCM value_ = nullptr;
};

namespace scm {

void init_scm();
res_pool& get_pool();

//// object constructor
// shader
SCM make_shader(SCM nm, SCM lst);
size_t free_shader(SCM shdr);
SCM equalp_shader(SCM shdr1, SCM shdr2);
int print_shader(SCM shdr, SCM port, scm_print_state* pstate);
// model
SCM make_model(SCM nm, SCM filename);
size_t free_model(SCM m);
SCM equalp_model(SCM m1, SCM m2);
int print_model(SCM m, SCM port, scm_print_state* pstate);
// render_task
SCM make_render_task(SCM nm, SCM filename);
size_t free_render_task(SCM rt);
SCM equalp_render_task(SCM rt1, SCM rt2);
int print_render_task(SCM rt, SCM port, scm_print_state* pstate);

// shader-relevant
SCM shader_source(SCM shdr, SCM tag);


// other
SCM find_obj_by_name(SCM name);
SCM get_obj_name(SCM obj);

// misc.
void parse_shader_from_scm(SCM lst, shader_info& s);

}

}

#endif // SCM_H_INCLUDED

