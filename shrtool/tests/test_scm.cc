#define TEST_SUITE "test_scm"

#define EXPOSE_EXCEPTION
#define NO_GUI_TEST

#include <sstream>
#include <fstream>

#include "unit_test.h"
#include "test_utils.h"
#include "scm.h"

using namespace std;
using namespace shrtool;

TEST_CASE(test_scm_shader) {
    scm::init_scm();

    SCM strport = scm_open_output_string();
    scm_c_define("shader-list-path", scm_from_latin1_string(
        locate_assets("shaders/blinn-phong.scm").c_str()));
    scm_c_define("strport", strport);

    scm_c_eval_string(R"EOF(
        (define blinn-phong-shader
            (make-shader "blinn-phong" (load shader-list-path)))
        (display (shader-source blinn-phong-shader 'vertex) strport)
        (display blinn-phong-shader strport)
        )EOF");

    assert_true(scm_is_true(scm_c_eval_string(R"EOF(
        (equal? blinn-phong-shader 
            (find-obj-by-name "blinn-phong"))
        )EOF")));

    assert_true(scm_is_true(scm_c_eval_string(R"EOF(
        (equal? "blinn-phong"
            (get-obj-name blinn-phong-shader))
        )EOF")));

    string src;

    {
        auto p = scm::get_pool().ref<shader_info>("blinn-phong");
        assert_true(!p.expired());
        auto& shdr = p.lock()->data;
        auto* sshdr = shdr.get_sub_shader_by_type(shader::VERTEX);
        assert_true(sshdr);
        src = sshdr->make_source(shdr);
    }

    scm::get_pool().remove("blinn-phong");
    scm_c_eval_string("(display blinn-phong-shader strport)");

    assert_equal_print(
            src +
            string("#<shader blinn-phong>") +
            string("#<shader INVALID!>"),
            scm_to_latin1_string(scm_get_output_string(strport)));
}

int main(int argc, char* argv[])
{
    scm_init_guile();
    return unit_test::test_main(argc, argv);
}
