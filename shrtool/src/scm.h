#ifndef SCM_H_INCLUDED
#define SCM_H_INCLUDED

#include <guile/2.0/libguile.h>

#include "shader_parser.h"
#include "reflection.h"

namespace shrtool {

namespace scm {

struct scm_t {
    scm_t(SCM s) : scm(s) { scm_gc_protect_object(scm); }
    scm_t(const scm_t& s) : scm_t(s.scm) { }
    ~scm_t() { scm_gc_unprotect_object(scm); }

    static void meta_reg_() {
        refl::meta_manager::reg_class<scm_t>("scm")
            .enable_clone();
    }

    SCM scm;
};

void init_scm();

SCM make_instance_scm(refl::instance&& ins);
size_t free_instance_scm(SCM ins);
SCM equalp_instance_scm(SCM a, SCM b);
int print_instance_scm(SCM ins, SCM port, scm_print_state* pstate);

struct builtin {
    static void parse_shader_from_scm(scm_t lst, shader_info& s);

    static shader_info make_shader(scm_t lst) {
        shader_info s;
        parse_shader_from_scm(lst, s);
        return std::move(s);
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<builtin>("builtin")
            .function("make_shader", make_shader);
    }
};

}

}

#endif // SCM_H_INCLUDED

