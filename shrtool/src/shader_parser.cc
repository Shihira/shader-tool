#include "shader_parser.h"
#include "iostream"
#include "exception.h"

namespace shrtool {

DEF_ENUM_MAP(em_glsl_type_name__, layout::item_type, const char*, ({
        { layout::COLOR, "vec4" },
        { layout::COL2, "vec2" },
        { layout::COL3, "vec3" },
        { layout::COL4, "vec4" },
        { layout::MAT4, "mat4" },
        { layout::INT, "int" },
        { layout::FLOAT, "float" },
        { layout::TEX2D, "sampler2D" },
    }))

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

std::string layout::glsl_type_name(layout::item_type t)
{
    return em_glsl_type_name__(t);
}

}

#include <guile/2.0/libguile.h>

namespace shrtool {

#define sym_equ(s, str) scm_is_eq((s), scm_from_latin1_symbol(str))
#define map_idx(assc, str) scm_assq_ref((assc), scm_from_latin1_symbol(str))

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

void shader_parser::load_shader(std::istream& is, shader_info& s)
{
    std::string scheme_code = std::string(
        std::istreambuf_iterator<char>(is),
        std::istreambuf_iterator<char>());

    SCM shader_list = scm_c_eval_string(scheme_code.c_str());
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
            parse_layout(map_idx(alist, "layout"), s.attributes.lo_);
        }
        else if(sym_equ(symbol, "property-group")) {
            shader_info::property_group_t pg;
            pg.first = scm_to_latin1_string(map_idx(alist, "name"));
            parse_layout(map_idx(alist, "layout"), pg.second.lo_);

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

std::string layout::make_source_as_attr() const
{
    std::string src;
    for(size_t i = 0; i < value.size(); ++i) {
        if(value[i].first >= TEX2D)
            throw unsupported_error("Unable to pass textures as attributes");
        src += "layout (location = " + std::to_string(i) + ") in " +
            layout::glsl_type_name(value[i].first) +
            " " + value[i].second + ";\n";
    }

    return src;
}

std::string layout::make_source_as_prop(const std::string& n) const
{
    std::string src = "uniform " + n + " {\n";
    std::vector<const type_name_pair*> textures;

    for(auto& li : value) {
        if(li.first >= TEX2D) {
            textures.push_back(&li);
            continue;
        }

        src += "    " + layout::glsl_type_name(li.first) +
            " " + li.second + ";\n";
    }
    src += "};\n";

    for(auto* li : textures) {
        src += "uniform " + layout::glsl_type_name(li->first)
            + " " + li->second + ";\n";
    }
    return src;
}

std::string sub_shader_info::make_source(const shader_info& parent) const
{
    std::string src;
    src += "#version " + version + "\n";

    for(auto& p : parent.property_groups)
        src += p.second.make_source_as_prop(p.first);

    if(type == shader::VERTEX)
        src += parent.attributes.make_source_as_attr();

    src += source;

    return src;
}

}

