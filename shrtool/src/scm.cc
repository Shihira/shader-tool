#include <unordered_map>
#include <list>
#include <fstream>
#include <sstream>

#include "scm.h"
#include "image.h"
#include "mesh.h"

#define sym_equ(s, str) scm_is_eq((s), scm_from_latin1_symbol(str))
#define map_idx(assc, str) scm_assq_ref((assc), scm_from_latin1_symbol(str))
// bith = built-in type hint
#define scm_bith(t, o, n) SCM_ASSERT_TYPE( \
        scm_##t##_p(o), o, SCM_ARG##n, __FUNCTION__, #t)

namespace shrtool {

using namespace refl;

namespace scm {

static scm_t_bits instance_type = 0;

DEF_ENUM_MAP(em_scm_sym_type_name__, std::string, layout::item_type, ({
        { "color", layout::COLOR },
        { "col2", layout::COL2 },
        { "col3", layout::COL3 },
        { "col4", layout::COL4 },
        { "mat4", layout::MAT4 },
        { "int", layout::INT },
        { "float", layout::FLOAT },
        { "tex2d", layout::TEX2D },
        { "texcubemap", layout::TEXCUBEMAP },
    }))

DEF_ENUM_MAP(em_scm_enum_tranform__, std::string, size_t, ({
        { "vertex", shader::VERTEX },
        { "fragment", shader::FRAGMENT },
        { "geometry", shader::GEOMETRY },
    }))

////////////////////////////////////////////////////////////////////////////////

struct builtin {
    static shader_info shader_from_config(scm_t lst) {
        shader_info s;
        parse_shader_from_scm(lst, s);
        return std::move(s);
    }

    static image image_from_ppm(const std::string& fn) {
        std::ifstream fin(fn);
        return image_io_netpbm::load(fin);
    }

    static scm_t meshes_from_wavefront(const std::string& fn) {
        std::ifstream fin(fn);
        std::vector<mesh_indexed> meshes = mesh_io_object::load(fin);

        SCM vec = scm_make_vector(scm_from_size_t(meshes.size()),
                SCM_UNDEFINED);
        for(size_t i = 0; i < meshes.size(); i++) {
            scm_c_vector_set_x(vec, i, make_instance_scm(
                        instance::make(std::move(meshes[i]))));
        }

        return vec;
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<builtin>("builtin")
            .function("shader_from_config", shader_from_config)
            .function("image_from_ppm", image_from_ppm)
            .function("meshes_from_wavefront", meshes_from_wavefront);
    }
};


SCM instance_to_scm(instance&& ins)
{
    const meta& m = ins.get_meta();
    if(m.is_same<scm_t>())
        return ins.get<scm_t>().scm;
    if(m.is_same<bool>())
        return scm_from_bool(ins.get<bool>());
    if(m.is_same<int>())
        return scm_from_int(ins.get<int>());
    if(m.is_same<size_t>())
        return scm_from_size_t(ins.get<size_t>());
    if(m.is_same<float>())
        return scm_from_double(ins.get<float>());
    if(m.is_same<double>())
        return scm_from_double(ins.get<float>());
    if(m.is_same<char>())
        return scm_from_char(ins.get<char>());
    if(m.is_same<std::string>()) {
        std::string& s = ins.get<std::string>();
        return scm_from_latin1_stringn(s.c_str(), s.size());
    }

    return make_instance_scm(std::move(ins));
}

instance scm_to_instance(SCM s)
{
    if(scm_is_bool(s))
        return instance::make<bool>(scm_to_bool(s));
    if(scm_is_integer(s))
        return instance::make<int>(scm_to_int(s));
    if(scm_is_real(s))
        return instance::make<float>(scm_to_double(s));
    if(scm_is_string(s))
        return instance::make<std::string>(scm_to_latin1_string(s));
    if(scm_is_symbol(s))
        return instance::make<size_t>(em_scm_enum_tranform__(
            scm_to_latin1_string(scm_symbol_to_string(s))));

    return instance::make<scm_t>(scm_t(s));
}

instance* extract_instance(SCM s)
{
    return (instance*)SCM_SMOB_DATA(s);
}

SCM general_subroutine(SCM typenm, SCM funcnm, SCM scm_args)
{
    std::list<instance> args; // must use list
    std::vector<instance*> args_ptr;

    while(scm_is_pair(scm_args)) {
        SCM cur = scm_car(scm_args);
        if(SCM_SMOB_PREDICATE(instance_type, cur)) {
            args_ptr.push_back(extract_instance(cur));
        } else {
            args.emplace_back(scm_to_instance(cur));
            args_ptr.push_back(&args.back());
        }

        scm_args = scm_cdr(scm_args);
    }

    meta& m = meta_manager::get_meta(scm_to_latin1_string(typenm));
    instance ins = m.apply(scm_to_latin1_string(funcnm),
            args_ptr.data(), args_ptr.size());
    return instance_to_scm(std::move(ins));
}

void register_function(
        const std::string& func,
        const std::string& typenm,
        const std::string& funcnm)
{
    /*
     * (define FUNC
     *   (lambda args
     *     (general-subroutine TYPENM FUNCNM args))
     */

    scm_eval(SCM_LIST3(
            scm_from_latin1_symbol("define"),
            scm_from_latin1_symbol(func.c_str()),
            SCM_LIST3(
                scm_from_latin1_symbol("lambda"),
                scm_from_latin1_symbol("args"),
                SCM_LIST4(
                    scm_from_latin1_symbol("general-subroutine"),
                    scm_from_latin1_string(typenm.c_str()),
                    scm_from_latin1_string(funcnm.c_str()),
                    scm_from_latin1_symbol("args")))),
        scm_interaction_environment());
}

void register_all()
{
    scm_c_define_gsubr("general-subroutine",
            3, 0, 0, (void*)general_subroutine);

    for(auto& m : meta_manager::meta_set()) {
        for(auto& f : m.second.function_set()) {
            if(!f.first.empty() && f.first[0] == '_')
                continue;

            std::string type_name = m.second.name();
            std::string func_name =
                (type_name != "builtin" ? (type_name + "-") : "") +
                f.first;
            for(char& c : func_name)
                c = c == '_' ? '-' : c;

            register_function(func_name, type_name, f.first);
        }
    }
}

void init_scm()
{
    instance_type = scm_make_smob_type("instance", 0);
    scm_set_smob_free(instance_type, free_instance_scm);
    scm_set_smob_print(instance_type, print_instance_scm);
    scm_set_smob_equalp(instance_type, equalp_instance_scm);

    scm_t::meta_reg_();
    builtin::meta_reg_();
    shader_info::meta_reg();
    color::meta_reg_();
    image::meta_reg_();
    mesh_indexed::meta_reg_();

    register_all();
}

SCM make_instance_scm(instance&& ins)
{
    instance* pins = new instance(std::move(ins));
    static_assert(sizeof(pins) <= sizeof(scm_t_bits),
            "SMOB cannot contain a pointer");
    return scm_new_smob(instance_type, (scm_t_bits)pins);
}

size_t free_instance_scm(SCM ins)
{
    instance* pins = extract_instance(ins);
    if(pins) delete pins;
    SCM_SET_SMOB_DATA(ins, 0);
    return 0;
}

SCM equalp_instance_scm(SCM a, SCM b)
{
    instance* pa = extract_instance(a);
    instance* pb = extract_instance(b);
    if(pa->get_meta() != pb->get_meta())
        return SCM_BOOL_F;
    if(!pa->get_meta().has_function("__equal"))
        return pa == pb ? SCM_BOOL_T : SCM_BOOL_F;

    return scm_from_bool(pa->call("__equal", *pb).get<bool>());
}

int print_instance_scm(SCM ins, SCM port, scm_print_state* pstate)
{
    instance* pi = (instance*)SCM_SMOB_DATA(ins);

    std::stringstream prn;
    if(pi->is_pointer()) {
        prn << "#<ref";
        if(pi->get_pointer_meta()) {
            prn << ':' << pi->get_pointer_meta()->name() << ' ';

            if(pi->get_pointer_meta()->has_function("__print")) {
                prn << pi->get_pointer_meta()->call("__print", *pi)
                    .get<std::string>() << ">";
            } else
                prn << ' ' << pi << '>';
        } else
            prn << ' ' << pi << '>';
    }
    else if(!pi->get_meta().has_function("__print")) {
         prn << "#<instance " << pi->get_meta().name() << ' ' << pi << ">";
    }
    else {
        prn << "#<instance " << pi->get_meta().name() << ' ' <<
            pi->call("__print").get<std::string>() + ">";
    }

    scm_c_write(port, prn.str().c_str(), prn.str().size());

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

void parse_layout(SCM layout_list, std::vector<layout::type_name_pair>& l)
{
    for(SCM cur = scm_car(layout_list), tail = scm_cdr(layout_list);;
            cur = scm_car(tail), tail = scm_cdr(tail)) {
        const char* t = scm_to_latin1_string(
                scm_symbol_to_string(scm_car(cur)));
        const char* n = scm_to_latin1_string(scm_cdr(cur));

        l.push_back(std::make_pair(
                em_scm_sym_type_name__(t), n));

        if(scm_is_null(tail)) break;
    }
}

void parse_shader_from_scm(scm_t shader_list_, shader_info& s)
{
    SCM shader_list = shader_list_.scm;
    if(scm_is_false(scm_list_p(shader_list)))
        throw unsupported_error("Shader is not a proper list.");

    for(SCM cur = scm_car(shader_list), tail = scm_cdr(shader_list);;
            cur = scm_car(tail), tail = scm_cdr(tail)) {
        SCM symbol = scm_car(cur),
            alist = scm_cdr(cur);

        if(sym_equ(symbol, "name")) {
            s.name = scm_to_latin1_string(alist);
        }
        else if(sym_equ(symbol, "attributes")) {
            parse_layout(map_idx(alist, "layout"), s.attributes.value);
        }
        else if(sym_equ(symbol, "property-group")) {
            shader_info::property_group_t pg;
            pg.first = scm_to_latin1_string(map_idx(alist, "name"));
            parse_layout(map_idx(alist, "layout"), pg.second.value);

            s.property_groups.push_back(std::move(pg));
        }
        else if(sym_equ(symbol, "sub-shader")) {
            sub_shader_info ssi;
            ssi.version = scm_to_latin1_string(map_idx(alist, "version"));
            ssi.source = scm_to_latin1_string(map_idx(alist, "source"));

            if(sym_equ(map_idx(alist, "type"), "fragment"))
                ssi.type = shader::FRAGMENT;
            if(sym_equ(map_idx(alist, "type"), "vertex"))
                ssi.type = shader::VERTEX;

            s.sub_shaders.push_back(std::move(ssi));
        }

        if(scm_is_null(tail)) break;
    }
}

}

}

