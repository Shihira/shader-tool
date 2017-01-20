#include <unordered_map>
#include <fstream>

#include "scm.h"
#include "mesh.h"

#define sym_equ(s, str) scm_is_eq((s), scm_from_latin1_symbol(str))
#define map_idx(assc, str) scm_assq_ref((assc), scm_from_latin1_symbol(str))
// bith = built-in type hint
#define scm_bith(t, o, n) SCM_ASSERT_TYPE( \
        scm_##t##_p(o), o, SCM_ARG##n, __FUNCTION__, #t)

namespace shrtool {

////////////////////////////////////////////////////////////////////////////////
// scm_item_trait

struct scm_item_trait_unknown : scm_item_trait {
    const char* glsl_type_name(SCM obj) const override { return ""; }
    void copy(SCM obj, uint8_t* data) const override { }
    size_t align(SCM obj) const override { return 0; }
    size_t size(SCM obj) const override { return 0; }
};

struct scm_item_trait_int : scm_item_trait {
    const char* glsl_type_name(SCM obj) const override { return "int"; }
    void copy(SCM obj, uint8_t* data) const override {
        *reinterpret_cast<int*>(data) = scm_to_int(obj);
    }
    size_t align(SCM obj) const override {
        return sizeof(int);
    }
    size_t size(SCM obj) const override {
        return sizeof(int);
    }
};

const scm_item_trait* scm_item_trait::query_trait(SCM obj) {
    if(SCM_NUMBERP(obj)) {
        static scm_item_trait_int t;
        return &t;
    }

    static scm_item_trait_unknown ut;
    return &ut;
}

namespace scm {

static scm_t_bits shader_type;
static scm_t_bits model_type;
static res_pool scm_obj_pool;
static std::unordered_map<std::string, SCM> scm_pool;

res_pool& get_pool() { return scm_obj_pool; }

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

void init_scm()
{
    // register shader type
    shader_type = scm_make_smob_type("shader", 0);
    scm_set_smob_free(shader_type, free_shader);
    scm_set_smob_print(shader_type, print_shader);
    scm_set_smob_equalp(shader_type, equalp_shader);

    // register model type
    model_type = scm_make_smob_type("model", 0);
    scm_set_smob_free(model_type, free_model);
    scm_set_smob_print(model_type, print_model);
    scm_set_smob_equalp(model_type, equalp_model);

    scm_c_define_gsubr("shader-source", 2, 0, 0, (void*)shader_source);
    scm_c_define_gsubr("make-shader", 2, 0, 0, (void*)make_shader);
    scm_c_define_gsubr("find-obj-by-name", 1, 0, 0, (void*)find_obj_by_name);
    scm_c_define_gsubr("get-obj-name", 1, 0, 0, (void*)get_obj_name);
}

template<typename T>
SCM make_smob_from_weak_ref(std::weak_ptr<T> w)
{
    scm_t_bits data_stor[3];

    // in most cases it passes
    static_assert(
        sizeof(data_stor) > sizeof(std::weak_ptr<T>),
        "unable to store a weak_ptr in Smob");

    new (data_stor) std::weak_ptr<T>(std::move(w));

    SCM o = scm_new_double_smob(shader_type,
            data_stor[0], data_stor[1], data_stor[2]);

    return o;
}

template<typename T>
void delete_weak_ref_made_smob(SCM o)
{
    typedef std::weak_ptr<T> weak_ptr_T;
    scm_t_bits data_stor[3];

    data_stor[0] = SCM_SMOB_DATA_1(o);
    data_stor[1] = SCM_SMOB_DATA_2(o);
    data_stor[2] = SCM_SMOB_DATA_3(o);

    auto* p = reinterpret_cast<std::weak_ptr<T>*>(data_stor);
    p->~weak_ptr_T();
}

template<typename T>
std::weak_ptr<T> smob_to_weak_ref(SCM o)
{
    scm_t_bits data_stor[3];

    data_stor[0] = SCM_SMOB_DATA_1(o);
    data_stor[1] = SCM_SMOB_DATA_2(o);
    data_stor[2] = SCM_SMOB_DATA_3(o);

    auto& p = *reinterpret_cast<std::weak_ptr<T>*>(data_stor);

    return p;
}

SCM find_obj_by_name(SCM name)
{
    scm_bith(string, name, 1);

    const char* n = scm_to_latin1_string(name);

    auto p = scm_pool.find(n);
    if(p == scm_pool.end())
        return SCM_ELISP_NIL;
    return p->second;
}

SCM get_obj_name(SCM o)
{
    if(SCM_SMOB_PREDICATE(shader_type, o)) {
        auto ref = smob_to_weak_ref<res_pool::stor_type<shader_info>>(o);
        return scm_from_latin1_string(ref.lock()->name.c_str());
    }

    return scm_from_latin1_string("");
}

////////////////////////////////////////////////////////////////////////////////
// shader

SCM make_shader(SCM nm, SCM lst)
{
    scm_bith(string, nm, 1);
    scm_bith(list, nm, 2);

    const char* s_nm = scm_to_latin1_string(nm);
    shader_info si;
    parse_shader_from_scm(lst, si);

    scm_obj_pool.insert<shader_info>(s_nm, std::move(si));
    SCM scm =  make_smob_from_weak_ref(scm_obj_pool.ref<shader_info>(s_nm));
    scm_pool[s_nm] = scm;

    return scm;
}

size_t free_shader(SCM m)
{
    delete_weak_ref_made_smob<res_pool::stor_type<shader_info>>(m);
    return 0;
}

SCM equalp_shader(SCM m1, SCM m2)
{
    auto p1 = smob_to_weak_ref<res_pool::stor_type<shader_info>>(m1);
    auto p2 = smob_to_weak_ref<res_pool::stor_type<shader_info>>(m2);
    if(p1.expired() || p2.expired()) return SCM_BOOL_F;
    if(p1.lock()->name == p2.lock()->name) return SCM_BOOL_T;
    else return SCM_BOOL_F;
}

int print_shader(SCM m, SCM port, scm_print_state* pstate)
{
    auto wp = smob_to_weak_ref<res_pool::stor_type<shader_info>>(m);
    std::string prn;
    if(!wp.expired()) {
        auto p = wp.lock();
        prn = "#<shader " + p->name + ">";
    } else
        prn = "#<shader INVALID!>";

    scm_c_write(port, prn.c_str(), prn.size());

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// model

typedef std::vector<mesh_indexed> meshes_type;

SCM make_model(SCM nm, SCM filename)
{
    scm_bith(string, nm, 1);
    scm_bith(string, nm, 2);

    const char* s_nm = scm_to_latin1_string(nm);
    const char* s_fn = scm_to_latin1_string(nm);

    std::ifstream file(s_fn);
    meshes_type mesh = mesh_io_object::load(file);

    scm_obj_pool.insert<meshes_type>(s_nm, std::move(mesh));
    SCM scm =  make_smob_from_weak_ref(scm_obj_pool.ref<meshes_type>(s_nm));
    scm_pool[s_nm] = scm;

    return scm;
}

size_t free_model(SCM shdr)
{
    delete_weak_ref_made_smob<res_pool::stor_type<meshes_type>>(shdr);
    return 0;
}

SCM equalp_model(SCM shdr1, SCM shdr2)
{
    auto p1 = smob_to_weak_ref<res_pool::stor_type<meshes_type>>(shdr1);
    auto p2 = smob_to_weak_ref<res_pool::stor_type<meshes_type>>(shdr2);
    if(p1.expired() || p2.expired()) return SCM_BOOL_F;
    if(p1.lock()->name == p2.lock()->name) return SCM_BOOL_T;
    else return SCM_BOOL_F;
}

int print_model(SCM shdr, SCM port, scm_print_state* pstate)
{
    auto wp = smob_to_weak_ref<res_pool::stor_type<meshes_type>>(shdr);
    std::string prn;
    if(!wp.expired()) {
        auto p = wp.lock();
        prn = "#<model " + p->name + ">";
    } else
        prn = "#<model INVALID!>";

    scm_c_write(port, prn.c_str(), prn.size());

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// misc.

SCM shader_source(SCM shdr, SCM tag)
{
    scm_assert_smob_type(shader_type, shdr);
    scm_bith(symbol, tag, 2);

    auto wp = smob_to_weak_ref<res_pool::stor_type<shader_info>>(shdr);
    auto p = wp.lock();
    sub_shader_info* ss = nullptr;

    if(sym_equ(tag, "vertex"))
        ss = p->data.get_sub_shader_by_type(shader::VERTEX);
    if(sym_equ(tag, "fragment"))
        ss = p->data.get_sub_shader_by_type(shader::FRAGMENT);
    if(sym_equ(tag, "geometry"))
        ss = p->data.get_sub_shader_by_type(shader::GEOMETRY);

    if(ss) return scm_from_latin1_string(ss->make_source(*p).c_str());
    return scm_from_latin1_string("");
}

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

void parse_shader_from_scm(SCM shader_list, shader_info& s)
{
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

