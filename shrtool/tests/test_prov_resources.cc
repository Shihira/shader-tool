#include <vector>

#define EXPOSE_EXCEPTION
#include "gui_test.h"
#include "providers.h"

using namespace shrtool;
using namespace shrtool::render_assets;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

#include "mesh.h"

TEST_CASE(test_provided_mesh) {
    string data = R"EOF(
    # a rectangle form with two triangles
    v 0 0 0
    v 1 0 0
    v 1 1 0
    v 0 1 0
    f 1 2 3
    f 1 3 4
    )EOF";

    stringstream ss(data);
    vector<mesh_indexed> meshes = mesh_io_object::load(ss);
    mesh_indexed& m = meshes[0];

    typedef provider<mesh_indexed, vertex_attr_vector> prov;

    auto p = prov::load(m);
    float* buf = p->share_input(0)->start_map<float>(
            render_assets::buffer::READ);

    for(size_t i = 0; i < m.positions.size(); i++) {
        for(size_t d = 0; d < 4; d++)
            assert_equal(buf[i * 4 + d], m.positions[i][d]);
    }

    p->share_input(0)->stop_map();
}

int main(int argc, char* argv[])
{
    gui_test_context::init("330 core", "");
    return unit_test::test_main(argc, argv);
}
