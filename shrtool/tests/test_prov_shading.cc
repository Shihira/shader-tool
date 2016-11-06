#include <vector>

#define EXPOSE_EXCEPTION
#include "test_utils.h"
#include "providers.h"
#include "mesh.h"
#include "properties.h"
#include "image.h"

using namespace shrtool;
using namespace shrtool::render_assets;
using namespace std;

TEST_CASE_FIXTURE(test_earth, singlefunc_fixture)
{
}

int main(int argc, char* argv[])
{
    gui_test_context::init("330 core", "test_prov_shading");
    return unit_test::test_main(argc, argv);
}
