#define EXPOSE_EXCEPTION

#include "common/unit_test.h"
#include "common/mesh.h"

using namespace std;
using namespace shrtool;
using namespace shrtool::math;
using namespace shrtool::unit_test;

TEST_CASE(test_load_pos_only_rectangle) {
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

    assert_equal_print(meshes[0].positions.size(), 6u);
    assert_equal_print(meshes[0].stor_positions->size(), 4u);
    assert_equal_print(meshes[0].positions[0], col4({ 0, 0, 0, 1 }));
    assert_equal_print(meshes[0].positions[5], col4({ 0, 1, 0, 1 }));
}

TEST_CASE(test_load_pnu_rectangle) {
    string data = R"EOF(
    # a rectangle form with two triangles
    v 0 0 0
    v 1 0 0
    v 1 1 0
    v 0 1 0
    vn 0 0 1
    vn 0 0 -1
    vt 0 0 0
    vt 1 0 0
    vt 1 1 0
    vt 0 1 0
    f 1//1 2//1 3//1
    f 4/-1/2 3/-2/2 1/-4/2
    )EOF";

    stringstream ss(data);
    vector<mesh_indexed> meshes = mesh_io_object::load(ss);
    auto& m = meshes[0];

    assert_equal_print(m.positions.size(), 6u);
    assert_equal_print(m.normals.size(), 6u);
    assert_equal_print(m.uvs.size(), 6u);
    assert_equal_print(m.stor_positions->size(), 4u);
    assert_equal_print(m.stor_normals->size(), 2u);
    assert_equal_print(m.stor_uvs->size(), 4u);

    assert_equal_print(cross(
            col3(m.positions[1] - m.positions[0]),
            col3(m.positions[2] - m.positions[1])), m.normals[0]);
    assert_equal_print(cross(
            col3(m.positions[4] - m.positions[3]),
            col3(m.positions[5] - m.positions[4])), m.normals[3]);
    assert_equal_print(meshes[0].uvs[0], col3({ 0, 0, 0 }));
    assert_equal_print(meshes[0].uvs[3], col3({ 0, 1, 0 }));
}

TEST_CASE(test_load_rectangle) {
    string data = R"EOF(
    # a rectangle that is complete, with four corners
    v 0 0 0
    v 1 0 0
    v 1 1 0
    v 0 1 0
    vn 0 0 1
    f 1//1 2//1 3//1 4//1
    )EOF";

    stringstream ss(data);
    vector<mesh_indexed> meshes = mesh_io_object::load(ss);
    auto& m = meshes[0];

    assert_equal_print(m.positions.size(), 6u);
    assert_equal_print(m.normals.size(), 6u);

    assert_equal_print(cross(
            col3(m.positions[1] - m.positions[0]),
            col3(m.positions[2] - m.positions[1])),
            m.stor_normals->at(0));
    assert_equal_print(cross(
            col3(m.positions[4] - m.positions[3]),
            col3(m.positions[5] - m.positions[4])),
            m.stor_normals->at(0));
}


TEST_CASE(test_load_multiple_shapes) {
    string data = R"EOF(
    # position and normal data may share through a couple of groups
    v 0 0 0
    v 1 0 0
    v 1 1 0
    v 0 1 0
    vn 0 0 1
    g rect
    f 1//1 2//1 3//1 4//1
    g rect-co-tri
    f 1//1 2//1 3//1
    f 1//1 3//1 4//1
    )EOF";

    stringstream ss(data);
    vector<mesh_indexed> meshes = mesh_io_object::load(ss);

    assert_equal(meshes[0].stor_positions, meshes[1].stor_positions);
    assert_equal(meshes[0].stor_normals, meshes[1].stor_normals);

    assert_equal_print(meshes[0].positions.size(), meshes[1].positions.size());
    assert_equal_print(meshes[0].positions.size(), 6u);
}

TEST_CASE(test_uv_sphere) {
    mesh_uv_sphere us(2, 6, 3);

    assert_true(us.has_positions());
    assert_true(us.has_normals());
    assert_true(us.has_uvs());

    for(size_t i = 0; i < us.triangles(); ++i) {
        for(size_t j = 0; j < 3; ++j) {
            assert_float_close(
                norm(col3(us.get_position(i, j))),
                2, 0.00001);
            assert_float_close(
                norm(us.get_normal(i, j)),
                1, 0.00001);
            // need more tests ...
        }

        float area = norm(cross(
            fcol3(us.get_position(i, 1) - us.get_position(i, 0)),
            fcol3(us.get_position(i, 2) - us.get_position(i, 1))));
        assert_true(area > 0.000001);
    }
}

TEST_CASE(test_plane) {
    mesh_plane pl(1, 1, 5, 5);

    for(size_t t = 0; t < pl.triangles(); t++) {
        double res = norm(cross(
            col3(pl.get_position(t, 1) - pl.get_position(t, 0)),
            col3(pl.get_position(t, 2) - pl.get_position(t, 1))));
        assert_true(res > 0.000001);
    }
}

#include "providers.h"

int main(int argc, char* argv[])
{
    test_main(argc, argv);
}

