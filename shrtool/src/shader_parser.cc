#include <iostream>

#include "shader_parser.h"
#include "exception.h"
#include "singleton.h"

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

DEF_ENUM_MAP(em_sexp_sym_type_name__, std::string, layout::item_type, ({
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

#include <chibi/eval.h>

namespace shrtool {

#define sym_equ(s, str) ((s) == sexp_intern(parse_ctx, str, -1))
#define map_idx(assc, str) \
    sexp_cdr(sexp_assq(parse_ctx, sexp_intern(parse_ctx, str, -1), (assc)))
#define parse_ctx (parse_ctx__::inst().ctx)

struct parse_ctx__ : generic_singleton<parse_ctx__> {
    sexp ctx;

    parse_ctx__() {
        ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 0);
        sexp_load_standard_env(ctx, NULL, SEXP_SEVEN);
        sexp_load_standard_ports(ctx, NULL, stdin, stdout, stderr, 0);
        sexp_eval_string(ctx, "(import (shrtool))", -1, NULL);
    }

    ~parse_ctx__() {
        sexp_destroy_context(ctx);
    }
};

void parse_layout(sexp layout_list, std::vector<layout::type_name_pair>& l)
{
    for(sexp cur = sexp_car(layout_list), tail = sexp_cdr(layout_list);;
            cur = sexp_car(tail), tail = sexp_cdr(tail)) {
        const char* t = sexp_string_data(
                sexp_symbol_to_string(parse_ctx, sexp_car(cur)));
        const char* n = sexp_string_data(sexp_cdr(cur));

        l.push_back(std::make_pair(
                em_sexp_sym_type_name__(t), n));

        if(sexp_nullp(tail)) break;
    }
}

void shader_parser::load_shader(std::istream& is, shader_info& s)
{
    std::string scheme_code = std::string(
        std::istreambuf_iterator<char>(is),
        std::istreambuf_iterator<char>());

    sexp shader_list = sexp_eval_string(parse_ctx,
            scheme_code.c_str(), scheme_code.size(), NULL);
    sexp_preserve_object(parse_ctx, shader_list);

    if(sexp_exceptionp(shader_list)) {
        sexp_print_exception(parse_ctx, shader_list,
                sexp_current_error_port(parse_ctx));
        throw unsupported_error("Shader is not a proper list.");
    }
    if(!sexp_listp(parse_ctx, shader_list))
        throw unsupported_error("Shader is not a proper list.");

    for(sexp cur = sexp_car(shader_list), tail = sexp_cdr(shader_list);;
            cur = sexp_car(tail), tail = sexp_cdr(tail)) {
        sexp symbol = sexp_car(cur),
            alist = sexp_cdr(cur);

        if(sym_equ(symbol, "name")) {
            if(!sexp_stringp(alist))
                throw unsupported_error("Bad string.");
            s.name = sexp_string_data(alist);
        }
        else if(sym_equ(symbol, "attributes")) {
            parse_layout(map_idx(alist, "layout"), s.attributes.lo_);
        }
        else if(sym_equ(symbol, "property-group")) {
            shader_info::property_group_t pg;
            pg.first = sexp_string_data(map_idx(alist, "name"));
            parse_layout(map_idx(alist, "layout"), pg.second.lo_);

            s.property_groups.push_back(std::move(pg));
        }
        else if(sym_equ(symbol, "sub-shader")) {
            sub_shader_info ssi;
            sexp str;

            str = map_idx(alist, "version");
            if(!sexp_stringp(str))
                throw unsupported_error("Bad string.");
            ssi.version = sexp_string_data(str);

            str = map_idx(alist, "source");
            if(!sexp_stringp(str))
                throw unsupported_error("Bad string.");
            ssi.source = sexp_string_data(str);

            if(sym_equ(map_idx(alist, "type"), "fragment"))
                ssi.type = shader::FRAGMENT;
            if(sym_equ(map_idx(alist, "type"), "vertex"))
                ssi.type = shader::VERTEX;

            s.sub_shaders.push_back(std::move(ssi));
        }

        if(sexp_nullp(tail)) break;
    }

    sexp_release_object(parse_ctx, shader_list);
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

