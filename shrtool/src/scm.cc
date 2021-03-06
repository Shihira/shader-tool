#include <unordered_map>
#include <list>
#include <fstream>
#include <sstream>

#include "scm.h"
#include "common/image.h"
#include "common/mesh.h"
#include "properties.h"
#include "render_assets.h"
#include "render_queue.h"
#include "common/logger.h"

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
        { "texcube", layout::TEXCUBEMAP },
    }))

DEF_ENUM_MAP(em_scm_enum_tranform__, std::string, size_t, ({
        { "vertex", shader::VERTEX },
        { "fragment", shader::FRAGMENT },
        { "geometry", shader::GEOMETRY },

        { "xOy", math::tf::xOy },
        { "yOz", math::tf::yOz },
        { "zOx", math::tf::zOx },

        { "color-buffer", render_target::COLOR_BUFFER },
        { "color-buffer-0", render_target::COLOR_BUFFER_0 },
        { "color-buffer-1", render_target::COLOR_BUFFER_2 },
        { "depth-buffer", render_target::DEPTH_BUFFER },

        { "no-face", render_target::NO_FACE },
        { "front-face", render_target::FRONT_FACE },
        { "back-face", render_target::BACK_FACE },
        { "both-face", render_target::BOTH_FACE },

        { "override-blend", render_target::OVERRIDE_BLEND },
        { "alpha-blend", render_target::ALPHA_BLEND },
        { "plus-blend", render_target::PLUS_BLEND },

        { "rgba-u8888", render_assets::texture::RGBA_U8888 },
        { "r-f32", render_assets::texture::R_F32 },
        { "rg-f32", render_assets::texture::RG_F32 },
        { "rgb-f32", render_assets::texture::RGB_F32 },
        { "rgba-f32", render_assets::texture::RGBA_F32 },
        { "depth-f32", render_assets::texture::DEPTH_F32 },
        { "default-fmt", render_assets::texture::DEFAULT_FMT },
    }))

////////////////////////////////////////////////////////////////////////////////

provided_render_task::provider_bindings bindings;

void scm_t::operator()() {
    if(scm_is_true(scm_procedure_p(scm))) {
        scm_call_0(scm);
    }
    else throw restriction_error("not a procedure");
}

bool is_symbol_eq(SCM sym, const std::string& s)
{
    return scm_is_eq(sym, scm_from_latin1_symbol(s.c_str()));
}

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

    static void image_save_ppm(const image& im, const std::string& fn) {
        std::ofstream fout(fn);
        return image_io_netpbm::save_image(fout, im);
    }

    static image texture_to_image(render_assets::texture& tex,
            render_assets::texture::format fmt) {
        if(fmt != render_assets::texture::RGBA_U8888) throw unsupported_error(
                "cannot read texture of this format into an image");
        image img(tex.get_width(), tex.get_height() * tex.get_depth());
        tex.read(img.data(), fmt);
        return std::move(img);
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

    static dynamic_property make_propset() {
        return dynamic_property();
    }

    static provided_render_task make_shading_rtask() {
        return provided_render_task(bindings);
    }

    static scm_t search_function(
            std::string type,
            const std::string& fn) {
        for(char& c : type) c = c == '-' ? '_' : c;
        const meta* m = meta_manager::find_meta(type);

        while(m) {
            std::string name = m->name();
            for(char& c : name) c = c == '_' ? '-' : c;
            name += "-" + fn;

            SCM sym = scm_from_latin1_symbol(name.c_str());
            if(scm_is_true(scm_defined_p(sym, scm_interaction_environment())))
                return sym;

            m = m->get_base();
        }

        warning_log << "canoot find " << fn
            << " for type " << type << std::endl;

        return SCM_ELISP_NIL;
    }

    static scm_t instance_search_function(instance& ins,
            const std::string& fn) {
        SCM ret;
        if(ins.is_pointer()) {
            if(!ins.get_pointer_meta())
                return SCM_ELISP_NIL;
            ret = search_function(ins.get_pointer_meta()->name(), fn).scm;
        } else ret = search_function(ins.get_meta().name(), fn).scm;

        if(!scm_is_symbol(ret)) return SCM_ELISP_NIL;

        ret = scm_symbol_to_string(ret);
        const char* name = scm_to_latin1_string(ret);
        return scm_c_public_ref("shrtool", name);
    }

    static std::string instance_get_type(instance& ins, bool check_ptr) {
        return !ins.is_pointer() ? ins.get_meta().name() :
            ins.get_pointer_meta() && check_ptr ?
            ins.get_pointer_meta()->name() : ins.get_meta().name();
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<builtin>("builtin")
            .enable_auto_register()
            .function("shader_from_config", shader_from_config)
            .function("image_from_ppm", image_from_ppm)
            .function("image_save_ppm", image_save_ppm)
            .function("texture_to_image", texture_to_image)
            .function("make_propset", make_propset)
            .function("make_shading_rtask", make_shading_rtask)
            .function("search_function", &search_function)
            .function("instance_search_function", &instance_search_function)
            .function("instance_get_type", &instance_get_type)
            .function("meshes_from_wavefront", meshes_from_wavefront)
            .function("set_log_level", logger_manager::set_current_level);
    }
};


SCM instance_to_scm(instance&& ins)
{
    if(ins.is_null())
        return SCM_UNSPECIFIED;

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
        return scm_from_double(ins.get<double>());
    if(m.is_same<char>())
        return scm_from_char(ins.get<char>());
    if(m.is_same<std::string>()) {
        std::string& s = ins.get<std::string>();
        return scm_from_latin1_stringn(s.c_str(), s.size());
    }

    if(m.is_same<math::fxmat>()) {
        math::fxmat& mat = ins.get<math::fxmat>();
        SCM smat = scm_make_typed_array(
                scm_from_latin1_symbol("f32"),
                scm_from_double(0),
                SCM_LIST2(scm_from_int(mat.rows()), scm_from_int(mat.cols())));
        scm_t_array_handle handle;
        scm_array_get_handle(smat, &handle);
        float* arr_data = scm_array_handle_f32_writable_elements(&handle);
        std::copy(mat.data(), mat.data() + mat.elem_count(), arr_data);
        scm_array_handle_release(&handle);
        return smat;
    }

    if(m.is_same<math::dxmat>()) {
        math::dxmat& mat = ins.get<math::dxmat>();
        SCM smat = scm_make_typed_array(
                scm_from_latin1_symbol("f64"),
                scm_from_double(0),
                SCM_LIST2(scm_from_int(mat.rows()), scm_from_int(mat.cols())));
        scm_t_array_handle handle;
        scm_array_get_handle(smat, &handle);
        double* arr_data = scm_array_handle_f64_writable_elements(&handle);
        std::copy(mat.data(), mat.data() + mat.elem_count(), arr_data);
        scm_array_handle_release(&handle);
        return smat;
    }

    return make_instance_scm(std::move(ins));
}

instance scm_to_instance(SCM s)
{
    if(scm_is_bool(s))
        return instance::make<bool>(scm_to_bool(s));
    // although it is completely possible to support 64-bit
    // but I would better to just stay on 32-bit, and tag a TODO
    if(scm_is_signed_integer(s,
            std::numeric_limits<int32_t>::min(),
            std::numeric_limits<int32_t>::max())) {
        return instance::make<int>(scm_to_int32(s));
    } else if(scm_is_unsigned_integer(s,
            std::numeric_limits<size_t>::min(),
            std::numeric_limits<size_t>::max())) {
        return instance::make<size_t>(scm_to_size_t(s));
    }

    if(scm_is_real(s))
        return instance::make<double>(scm_to_double(s));
    if(scm_is_string(s))
        return instance::make<std::string>(scm_to_latin1_string(s));
    if(scm_is_symbol(s))
        return instance::make<size_t>(em_scm_enum_tranform__(
            scm_to_latin1_string(scm_symbol_to_string(s))));

    if(scm_is_typed_array(s, scm_from_latin1_symbol("f64"))) {
        math::dxmat mat;
        scm_t_array_handle handle;
        scm_array_get_handle(s, &handle);
        SCM dim = scm_array_dimensions(s);
        const double* arr_data = scm_array_handle_f64_elements(&handle);
        mat.assign(scm_to_size_t(scm_car(dim)),
                scm_to_size_t(scm_cadr(dim)));
        std::copy(arr_data, arr_data + mat.elem_count(), mat.data());
        scm_array_handle_release(&handle);
        return instance::make<math::dxmat>(std::move(mat));
    }

    if(scm_is_typed_array(s, scm_from_latin1_symbol("f32"))) {
        math::fxmat mat;
        scm_t_array_handle handle;
        scm_array_get_handle(s, &handle);
        SCM dim = scm_array_dimensions(s);
        const float* arr_data = scm_array_handle_f32_elements(&handle);
        mat.assign(scm_to_size_t(scm_car(dim)),
                scm_to_size_t(scm_cadr(dim)));
        std::copy(arr_data, arr_data + mat.elem_count(), mat.data());
        scm_array_handle_release(&handle);
        return instance::make<math::fxmat>(std::move(mat));
    }

    return instance::make<scm_t>(scm_t(s));
}

instance* extract_instance(SCM s)
{
    if(!SCM_SMOB_PREDICATE(instance_type, s)) return nullptr;
    return (instance*)SCM_SMOB_DATA(s);
}

SCM general_subroutine(SCM typenm, SCM funcnm, SCM scm_args)
{
    try {
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
    } catch(const error_base& e) {
        return scm_throw(scm_from_latin1_symbol("cpp-error"),
                SCM_LIST2(scm_from_latin1_string(e.error_name()),
                    scm_from_latin1_string(e.what())));
    } catch(const std::exception& e) {
        return scm_throw(scm_from_latin1_symbol("cpp-error"),
                    scm_from_latin1_string(e.what()));
    }
}

void register_all()
{
    scm_c_define_gsubr("general-subroutine",
            3, 0, 0, (void*)general_subroutine);

    for(auto& m : meta_manager::meta_set()) {
        bool reged_cons = false;
        std::string type_name = m.second.name();

        for(auto& f : m.second.function_set()) {
            if(f.first.empty())
                continue;
            if(f.first[0] == '_') {
                if(f.first.length() >= 6 &&
                        f.first.substr(0, 6) == "__init" && !reged_cons) {
                    std::string func_name = "make-" + type_name;
                    for(char& c : func_name) c = c == '_' ? '-' : c;

                    debug_log << "scm registered " << type_name
                        << "::" << f.first << " -> " << func_name << std::endl;

                    scm_call_2(
                        scm_c_public_ref("shrtool", "shrtool-register-constructor"),
                        scm_from_latin1_symbol(func_name.c_str()),
                        scm_from_latin1_string(type_name.c_str()));
                    reged_cons = true;
                }
                continue;
            }

            std::string func_name =
                (type_name != "builtin" ? (type_name + "-") : "") + f.first;
            for(char& c : func_name)
                c = c == '_' ? '-' : c;

            debug_log << "scm registered " << type_name
                << "::" << f.first << " -> " << func_name << std::endl;

            scm_call_3(
                scm_c_public_ref("shrtool", "shrtool-register-function"),
                scm_from_latin1_symbol(func_name.c_str()),
                scm_from_latin1_string(type_name.c_str()),
                scm_from_latin1_string(f.first.c_str()));
        }
    }
}

void init_scm()
{
    instance_type = scm_make_smob_type("instance", 0);
    scm_set_smob_free(instance_type, free_instance_scm);
    scm_set_smob_print(instance_type, print_instance_scm);
    scm_set_smob_equalp(instance_type, equalp_instance_scm);

    typedef auto_register_func_guard_<> ar;
    for(size_t i = 0; i < ar::used; i++)
        ar::functions[i]();

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

    if(!scm_is_pair(shader_list))
        throw unsupported_error("Shader is not a proper list.");

    SCM elem = shader_list;
    while(scm_is_pair(elem)) {
        SCM cur = scm_car(elem);
        if(!scm_is_pair(cur))
            throw unsupported_error("Shader is not a proper list.");

        SCM symbol = scm_car(cur),
            alist = scm_cdr(cur);

        if(is_symbol_eq(symbol, "name")) {
            s.name = scm_to_latin1_string(alist);
        }
        else if(is_symbol_eq(symbol, "attributes")) {
            parse_layout(map_idx(alist, "layout"), s.attributes.value);
        }
        else if(is_symbol_eq(symbol, "property-group")) {
            shader_info::property_group_t pg;
            pg.first = scm_to_latin1_string(map_idx(alist, "name"));
            parse_layout(map_idx(alist, "layout"), pg.second.value);

            s.property_groups.push_back(std::move(pg));
        }
        else if(is_symbol_eq(symbol, "sub-shader")) {
            sub_shader_info ssi;
            SCM ver = map_idx(alist, "version");
            SCM src = map_idx(alist, "source");

            if(!scm_is_false(ver))
                ssi.version = scm_to_latin1_string(ver);
            else ssi.version = "330 core";

            if(!scm_is_false(src))
                ssi.source = scm_to_latin1_string(src);
            else throw unsupported_error("No source provided in sub-shader.");

            const char* type = scm_to_latin1_string(
                    scm_symbol_to_string(map_idx(alist, "type")));
            ssi.type = (shader::shader_type) em_scm_enum_tranform__(type);

            s.sub_shaders.push_back(std::move(ssi));
        }

        elem = scm_cdr(elem);
    }
}

}

}

//////////////////////////////////////// guile helper

extern "C" {

void shrtool_init_scm()
{
    shrtool::refl::meta_manager::init();
    shrtool::scm::init_scm();
}

}

