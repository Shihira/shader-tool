#ifndef SHADER_PARSER_H_INCLUDED
#define SHADER_PARSER_H_INCLUDED

#include <vector>
#include <string>
#include <utility>

#include "shading.h" 
#include "providers.h"

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
struct provider<shader_info, shader> {
    typedef shader_info input_type;
    typedef shader output_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    static output_type_ptr load(input_type& i) {
        output_type_ptr p(new output_type);
        for(auto& ssi : i.sub_shaders) {
            auto& ss = p->add_sub_shader(ssi.type);
            ss.compile(ssi.make_source(i));
        }
        p->link();

        return std::move(p);
    }

    static void update(input_type& i, output_type_ptr& o, bool anew) {
        if(anew) {
            for(auto& ssi : i.sub_shaders) {
                auto ss = o->share_sub_shader(ssi.type);
                if(!ss) {
                    o->add_sub_shader(ssi.type);
                    ss = o->share_sub_shader(ssi.type);
                }
                ss->compile(ssi.make_source(i));
            }
            o->link();
        }
    }
};

}

#endif // SHADER_H_INCLUDED

