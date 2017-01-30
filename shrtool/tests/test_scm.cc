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
using namespace refl;

struct scm_obj_fixture {
    SCM strport;

    scm_obj_fixture() {
        scm_init_guile();
        refl::meta_manager::init();
        scm::init_scm();

        strport = scm_open_output_string();
        scm_c_define("strport", strport);
    }

    void def_path(const string& nm, const string& fn) {
        scm_c_define(nm.c_str(), scm_from_latin1_string(
            locate_assets(fn).c_str()));
    }

    std::string port_str() {
        return scm_to_latin1_string(scm_strport_to_string(strport));
    }
};

TEST_CASE_FIXTURE(test_scm_shader, scm_obj_fixture) {
    def_path("shader-list-path", "shaders/blinn-phong.scm");

    scm_c_eval_string("(define blinn-phong-shader"
            "(shader-from-config (load shader-list-path)))");
    refl::instance* si = scm::extract_instance(scm_c_eval_string(
                "blinn-phong-shader"));

    scm_c_eval_string(
        "(display (shader-gen-source blinn-phong-shader 'vertex) strport)");
    scm_c_eval_string(
        "(display blinn-phong-shader strport)");

    stringstream ss;
    ss << si->get<shader_info>().gen_source(shader::VERTEX);
    ss << "#<instance shader " << si << ">";

    assert_equal_print(port_str(), ss.str());
}

TEST_CASE_FIXTURE(test_scm_image, scm_obj_fixture) {
    def_path("image-path", "textures/shihira.ppm");

    scm_c_eval_string("(define shihira-image"
            "(image-from-ppm image-path))");
    scm_c_eval_string("(display (image-pixel shihira-image 1 1) strport)");
    scm_newline(strport);
    scm_c_eval_string("(display (image-width shihira-image) strport)");

    ctest << port_str() << endl;
}

TEST_CASE_FIXTURE(test_scm_meshes, scm_obj_fixture) {
    def_path("teapot-model-path", "models/teapot.obj");

    scm_c_eval_string("(define uv-sphere"
            "(mesh-gen-uv-sphere 5 10 10 #t))");
    scm_c_eval_string("(display (mesh-vertices uv-sphere) strport)");
    scm_newline(strport);
    scm_c_eval_string("(define teapot"
            "(meshes-from-wavefront teapot-model-path))");
    scm_c_eval_string("(display (mesh-triangles"
            "(vector-ref teapot 0)) strport)");

    ctest << port_str() << endl;
}

TEST_CASE_FIXTURE(test_scm_array, scm_obj_fixture) {
    scm_c_eval_string("(import (shrtool))");

    scm_c_eval_string("(define propset (make-propset))");
    scm_c_eval_string("(propset-append propset 2)");
    scm_c_eval_string("(propset-append propset 3.2)");
    scm_c_eval_string("(display (propset-definition propset"
            " \"propset\") strport)");
    scm_newline(strport);
    scm_c_eval_string("(display (propset-get propset 0) strport)");
    scm_newline(strport);
    scm_c_eval_string("(display (propset-get propset 1) strport)");
    scm_newline(strport);
    scm_c_eval_string("(display (propset-get propset 2) strport)");
    scm_newline(strport);
    scm_c_eval_string("(propset-set propset 1 (propset-get propset 0))");
    scm_c_eval_string("(display (propset-get propset 1) strport)");
    scm_newline(strport);
    scm_c_eval_string("(display (propset-definition propset"
            " \"propset\") strport)");

    ctest << port_str() << endl;
}

int main(int argc, char* argv[])
{
    return unit_test::test_main(argc, argv);
}
