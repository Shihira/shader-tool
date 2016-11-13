#define TEST_SUITE "test_shader_parser"

#define EXPOSE_EXCEPTION

#include <sstream>
#include <fstream>

#include "unit_test.h"
#include "shader_parser.h"

using namespace std;
using namespace shrtool;

TEST_CASE(test_parse_sub_shader) {
    stringstream ss(R"EOF(
    '((name . "fuck-shader")
      (sub-shader
        (type . fragment)
        (version . "330 core")
        (source . "nothing special :-)")))
    )EOF");

    shader_info si;
    shader_parser::load_shader(ss, si);

    assert_equal_print(si.name, "fuck-shader");
    assert_equal_print(si.sub_shaders[0].type, shader::FRAGMENT);
    assert_equal_print(si.sub_shaders[0].version, "330 core");
    assert_equal_print(si.sub_shaders[0].source, "nothing special :-)");
}

TEST_CASE(test_parse_attributes) {
    stringstream ss(R"EOF(
    '((name . "attr-test-shader")
      (attributes
        (layout
          (color . "attr1")
          (col4 . "attr2")
          (col2 . "attr3")
          (int . "attr4")
          (float . "attr5"))))
    )EOF");

    shader_info si;
    shader_parser::load_shader(ss, si);

    assert_equal_print(si.name, "attr-test-shader");

    assert_equal_print(si.attributes.value[0].first, layout::COLOR);
    assert_equal_print(si.attributes.value[1].first, layout::COL4);
    assert_equal_print(si.attributes.value[2].first, layout::COL2);
    assert_equal_print(si.attributes.value[3].first, layout::INT);
    assert_equal_print(si.attributes.value[4].first, layout::FLOAT);

    assert_equal_print(si.attributes.value[0].second, "attr1");
    assert_equal_print(si.attributes.value[1].second, "attr2");
    assert_equal_print(si.attributes.value[2].second, "attr3");
    assert_equal_print(si.attributes.value[3].second, "attr4");
    assert_equal_print(si.attributes.value[4].second, "attr5");
}

TEST_CASE(test_parse_property) {
    stringstream ss(R"EOF(
    '((name . "prop-test-shader(")
      (property-group
        (name . "damn-prop")
        (layout
          (color . "attr1")
          (col4 . "attr2")
          (col2 . "attr3")
          (int . "attr4")
          (float . "attr5"))))
    )EOF");

    shader_info si;
    shader_parser::load_shader(ss, si);

    assert_equal_print(si.name, "prop-test-shader(");
    assert_equal_print(si.property_groups[0].first, "damn-prop");

    auto& lo = si.property_groups[0].second.value;

    assert_equal_print(lo[0].first, layout::COLOR);
    assert_equal_print(lo[1].first, layout::COL4);
    assert_equal_print(lo[2].first, layout::COL2);
    assert_equal_print(lo[3].first, layout::INT);
    assert_equal_print(lo[4].first, layout::FLOAT);

    assert_equal_print(lo[0].second, "attr1");
    assert_equal_print(lo[1].second, "attr2");
    assert_equal_print(lo[2].second, "attr3");
    assert_equal_print(lo[3].second, "attr4");
    assert_equal_print(lo[4].second, "attr5");
}

TEST_CASE(test_parse_blinn_phong_sample) {
    string path = locate_assets("shaders/blinn-phong.scm");

    ifstream fs(path);

    shader_info si;
    shader_parser::load_shader(fs, si);

    assert_equal_print(si.name, "blinn-phong");
    assert_equal_print(si.attributes.value[0].first, layout::COL4);
    assert_equal_print(si.attributes.value[2].second, "uv");
    assert_equal_print(si.property_groups[2].first, "material");
    assert_equal_print(si.property_groups[1].second[0].second, "lightPosition");
    assert_equal_print(si.property_groups[2].second[1].first, layout::COLOR);
    assert_equal_print(si.sub_shaders[0].type, shader::FRAGMENT);
    assert_equal_print(si.sub_shaders[1].type, shader::VERTEX);
}

#include <libguile.h>

int main(int argc, char* argv[])
{
    scm_init_guile();
    return unit_test::test_main(argc, argv);
}
