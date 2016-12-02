#ifndef SCM_H_INCLUDED
#define SCM_H_INCLUDED

#include <guile/2.0/libguile.h>

#include "res_pool.h"
#include "shader_parser.h"

namespace shrtool {

namespace scm {

void init_scm();
res_pool& get_pool();

// object constructor
SCM make_shader(SCM nm, SCM lst);
size_t free_shader(SCM shdr);
SCM equalp_shader(SCM shdr1, SCM shdr2);
int print_shader(SCM shdr, SCM port, scm_print_state* pstate);

// shader-relevant
SCM shader_source(SCM shdr, SCM tag);


// other
SCM find_obj_by_name(SCM name);

// misc.
void parse_shader_from_scm(SCM lst, shader_info& s);

}

}

#endif // SCM_H_INCLUDED

