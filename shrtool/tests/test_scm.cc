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
    refl::meta_manager::init();
    scm::init_scm();

    SCM strport = scm_open_output_string();
    scm_c_define("shader-list-path", scm_from_latin1_string(
        locate_assets("shaders/blinn-phong.scm").c_str()));
    scm_c_define("strport", strport);

    scm_c_eval_string(
        "(define blinn-phong-shader (make-shader (load shader-list-path)))");
    scm_c_eval_string(
        "(display (shader-source blinn-phong-shader 'vertex) strport)");
    scm_c_eval_string(
        "(display blinn-phong-shader strport)");

    ctest << scm_to_latin1_string(scm_strport_to_string(strport)) << endl;
}

int main(int argc, char* argv[])
{
    scm_init_guile();
    return unit_test::test_main(argc, argv);
}
