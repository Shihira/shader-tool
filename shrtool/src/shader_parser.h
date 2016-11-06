#ifndef SHADER_PARSER_H_INCLUDED
#define SHADER_PARSER_H_INCLUDED

#include <vector>
#include <string>
#include <utility>

#include "shading.h" 

namespace shrtool {

class layout {
    friend struct shader_parser;

public:
    enum item_type {
        COLOR,
        COL2,
        COL3,
        COL4,
        MAT4,
        INT,
        FLOAT,
    };

    typedef std::pair<item_type, std::string> type_name_pair;

private:
    std::vector<type_name_pair> lo_;

public:
    const std::vector<type_name_pair>& value = lo_;

    layout() { }
    layout(const layout& l) : lo_(l.lo_) { }
    layout(layout&& l) : lo_(std::move(l.lo_)) { }

    static std::string glsl_type_name(item_type t);

    const type_name_pair& operator[](size_t i) const {
        return value[i];
    }
};

struct sub_shader_info {
    shader::shader_type type;
    std::string version;
    std::string source;
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

}

#endif // SHADER_H_INCLUDED

