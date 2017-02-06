#include "shader_parser.h"
#include "iostream"
#include "exception.h"
#include "scm.h"

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
        { layout::TEXCUBEMAP, "samplerCube" },
    }))


std::string layout::glsl_type_name(layout::item_type t)
{
    return em_glsl_type_name__(t);
}

const sub_shader_info* shader_info::get_sub_shader_by_type(
        shader::shader_type t) const
{
    for(const sub_shader_info& i : sub_shaders)
        if(i.type == t) return &i;
    return nullptr;
}

sub_shader_info* shader_info::get_sub_shader_by_type(shader::shader_type t)
{
    for(sub_shader_info& i : sub_shaders)
        if(i.type == t) return &i;
    return nullptr;
}

void shader_parser::load_shader(std::istream& is, shader_info& s)
{
    std::string scheme_code = std::string(
        std::istreambuf_iterator<char>(is),
        std::istreambuf_iterator<char>());

    SCM shader_list = scm_c_eval_string(scheme_code.c_str());
    scm::parse_shader_from_scm(shader_list, s);
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

