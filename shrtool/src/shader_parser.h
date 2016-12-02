#ifndef SHADER_PARSER_H_INCLUDED
#define SHADER_PARSER_H_INCLUDED

#include <vector>
#include <string>
#include <utility>

#include "shading.h" 
#include "providers.h"
#include "traits.h"

namespace shrtool {

struct sub_shader_info;

class layout {
    friend struct shader_parser;

public:
    enum item_type {
        COLOR = 32,
        COL2,
        COL3,
        COL4,
        MAT4,
        INT,
        FLOAT,
        TEX2D = 1024,
        TEXCUBEMAP
    };

    typedef std::pair<item_type, std::string> type_name_pair;

public:
    std::vector<type_name_pair> value;

    layout() { }
    layout(const layout& l) : value(l.value) { }
    layout(layout&& l) : value(std::move(l.value)) { }

    static std::string glsl_type_name(item_type t);

    const type_name_pair& operator[](size_t i) const {
        return value[i];
    }

    std::string make_source_as_attr() const;
    std::string make_source_as_prop(const std::string& n) const;
};

struct shader_info;

struct sub_shader_info {
    shader::shader_type type;
    std::string version;
    std::string source;

    std::string make_source(const shader_info& parent) const;

    sub_shader_info() = default;
    sub_shader_info(const sub_shader_info&) = default;
    sub_shader_info(sub_shader_info&&) = default;
};

struct shader_info {
    typedef std::pair<std::string, layout> property_group_t;

    std::string name;

    layout attributes;
    std::vector<property_group_t> property_groups;
    std::vector<sub_shader_info> sub_shaders;

    shader_info() = default;
    shader_info(const shader_info&) = default;
    shader_info(shader_info&&) = default;

    const sub_shader_info* get_sub_shader_by_type(shader::shader_type t) const;
    sub_shader_info* get_sub_shader_by_type(shader::shader_type t);
};

struct shader_parser {
    shader_info* si;

    shader_parser(shader_info& s) : si(&s) { }

    static shader_info load(std::istream& is) {
        shader_info s;
        load_shader(is, s);
        return std::move(s);
    }

    static void load_shader(std::istream& is, shader_info& s);
};

template<>
struct shader_trait<shader_info> {
    typedef shader_info input_type;

    static std::string source(const input_type& i, size_t e,
            shader::shader_type& t) {
        if(e >= i.sub_shaders.size()) return "";
        t = i.sub_shaders[e].type;
        return i.sub_shaders[e].make_source(i);
    }
};

}

#endif // SHADER_H_INCLUDED

