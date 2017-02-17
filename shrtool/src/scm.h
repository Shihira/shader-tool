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

    void operator()();

    static void meta_reg_() {
        refl::meta_manager::reg_class<scm_t>("scm")
            .enable_auto_register()
            .enable_callable()
            .enable_clone();
    }

    SCM scm;
};

void init_scm();
SCM make_instance_scm(refl::instance&& ins);
SCM instance_to_scm(refl::instance&& ins);
refl::instance scm_to_instance(SCM s);
refl::instance* extract_instance(SCM s);

SCM make_instance_scm(refl::instance&& ins);
size_t free_instance_scm(SCM ins);
SCM equalp_instance_scm(SCM a, SCM b);
int print_instance_scm(SCM ins, SCM port, scm_print_state* pstate);

// misc.
void parse_shader_from_scm(scm_t lst, shader_info& s);
bool is_symbol_eq(SCM sym, const std::string& s);

}

}

#endif // SCM_H_INCLUDED

