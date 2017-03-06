#include <vector>

#define EXPOSE_EXCEPTION
#include "test_utils.h"
#include "providers.h"

using namespace shrtool;
using namespace shrtool::render_assets;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

#include "common/mesh.h"
#include "properties.h"

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
    float* buf = p.share_input(0)->start_map<float>(
            render_assets::buffer::READ);

    for(size_t i = 0; i < m.positions.size(); i++) {
        for(size_t d = 0; d < 4; d++)
            assert_equal(buf[i * 4 + d], m.positions[i][d]);
    }

    p.share_input(0)->stop_map();
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE(test_provided_universal_property) {
    // vec4.float / x16.x4
    typedef universal_property<
        math::matrix<float, 4, 3>, // 0
        math::fcol3, int, // 3
        math::fcol3, char, char, char, // 4
        math::col3, // 6
        double> up_t; // 7.2
    up_t up;

    item_get<0>(up) = math::matrix<float, 4, 3> {
        12, 64, 23,
        45, 31, 63,
        75, 29, 73,
        3,  90, 15,
    };

    item_get<1>(up) = math::fcol3{ 1.1, 2, 3.3 };
    item_get<2>(up) = 4;
    item_get<3>(up) = math::fcol3{ 5.5, 6.6, 7 };
    item_get<4>(up) = 'S';
    item_get<5>(up) = 'H';
    item_get<6>(up) = 'R';
    item_get<7>(up) = math::col3{ 10, 11, 12 };
    item_get<8>(up) = 3.14159;

    typedef provider<up_t, property_buffer> prov;
    auto p = prov::load(up);
    void* buf = p.start_map(buffer::READ);

    assert_equal_print(static_cast<float*>(buf)[0], 12);
    assert_equal_print(static_cast<float*>(buf)[7], 90); // NOTE: col-major
    assert_float_equal(static_cast<float*>(buf)[12], 1.1);
    assert_equal_print(static_cast<int*>(buf)[15], 4);
    assert_equal_print(static_cast<char*>(buf)[77], 'H');
    assert_equal_print(static_cast<char*>(buf)[78], 'R');
    assert_equal_print(static_cast<double*>(buf)[13], 11);
    assert_float_equal(static_cast<double*>(buf)[15], 3.14159);

    p.stop_map();
}

TEST_CASE(test_provided_dynamic_property) {
    refl::meta_manager::init();
    dynamic_property dp;

    dp.append<math::fxmat>(math::fxmat(4, 3, {
        12, 64, 23,
        45, 31, 63,
        75, 29, 73,
        3,  90, 15,
    }));
    dp.append<math::fxmat>(math::fxmat(3, 1, { 1.1, 2, 3.3 }));
    dp.append<int>(4);
    dp.append<math::fxmat>(math::fxmat(3, 1, { 5.5, 6.6, 7 }));
    dp.append<char>('S');
    dp.append<char>('H');
    dp.append<char>('R');
    dp.append<math::dxmat>(math::dxmat(3, 1, {10, 11, 12}));
    dp.append<double>(3.14159);

    typedef provider<dynamic_property, property_buffer> prov;
    auto p = prov::load(dp);
    void* buf = p.start_map(buffer::READ);

    assert_equal_print(static_cast<float*>(buf)[0], 12);
    assert_equal_print(static_cast<float*>(buf)[7], 90); // NOTE: col-major
    assert_float_equal(static_cast<float*>(buf)[12], 1.1);
    assert_equal_print(static_cast<int*>(buf)[15], 4);
    assert_equal_print(static_cast<char*>(buf)[77], 'H');
    assert_equal_print(static_cast<char*>(buf)[78], 'R');
    assert_equal_print(static_cast<double*>(buf)[13], 11);
    assert_float_equal(static_cast<double*>(buf)[15], 3.14159);

    p.stop_map();
}

int main(int argc, char* argv[])
{
    gui_test_context::init("330 core", "");
    return unit_test::test_main(argc, argv);
}
