#define EXPOSE_EXCEPTION

#include <sstream>

#include "unit_test.h"
#include "render_queue.h"

using namespace std;
using namespace shrtool;
using namespace shrtool::unit_test;

vector<pair<int, int>> parse_edges(const string& g)
{
    istringstream is(g);
    vector<pair<int, int>> res;
    string tmp;

    is >> tmp >> tmp >> tmp;
    if(tmp != "{")
        throw parse_error("Failed to parse graph: " + tmp);

    while(true) {
        tmp.clear();
        int n1, n2;

        is >> n1;
        if(is.fail()) {
            is.clear();
            is >> tmp;
            if(tmp != "}")
                throw parse_error("Failed to parse graph: " + tmp);
            else break;
        }
        is >> tmp;
        is >> n2;

        if(tmp != "->")
            throw parse_error("Failed to parse graph: " + tmp);

        res.push_back(make_pair(n1, n2));
    }

    return res;
}

TEST_CASE(test_topology_sort) {
    auto g = parse_edges(R"EOF(
    digraph G {
        1 -> 0
        0 -> 5
        2 -> 3
        4 -> 5
        2 -> 4
        3 -> 5
        5 -> 6
     }
     )EOF");

    void_render_task n[7];
    for(auto& p : g)
        n[p.second].rely_on(n[p.first]);

    queue_render_task q;
    for(auto& e : n) q.tasks.push_back(&e);

    q.sort();

    map<const render_task*, size_t> idx;
    for(auto e : q)
        idx[e] = idx.size();

    for(auto& p : g)
        assert_true(idx[n + p.first] < idx[n + p.second]);
}

TEST_CASE(test_topology_sort_failure) {
    auto g = parse_edges(R"EOF(
    digraph G {
        6 -> 0
        0 -> 1
        1 -> 5
        2 -> 3
        4 -> 5
        2 -> 4
        3 -> 5
        5 -> 6
     }
     )EOF");

    void_render_task n[7];
    for(auto& p : g)
        n[p.second].rely_on(n[p.first]);

    queue_render_task q;
    for(auto& e : n) q.tasks.push_back(&e);

    assert_except(q.sort(), resolve_error);
}

#include <fstream>
#include <libguile.h>

#include "test_utils.h"
#include "image.h"
#include "shader_parser.h"
#include "mesh.h"
#include "properties.h"
#include "matrix.h"

using namespace shrtool::render_assets;
using namespace shrtool::math;

TEST_CASE_FIXTURE(test_shading, singlefunc_fixture) {
    render_target::screen.initial_color(0.2, 0.2, 0.2, 1);
    render_target::screen.enable_depth_test(true);

    ifstream shr_fs(locate_assets("shaders/blinn-phong.scm"));

    shader_info si = shader_parser::load(shr_fs);
    auto s = provider<shader_info, shader>::load(si);
    mesh_uv_sphere ms(2, 16, 8, false);
    auto a = provider<mesh_uv_sphere, vertex_attr_vector>::load(ms);

    universal_property<fcol4, fcol4, fcol4> mat_data(
            fcol4 { 0.1f, 0.1f, 0.13f, 1 },
            fcol4 { 0.7f, 0.7f, 0.7f, 1 },
            fcol4 { 0.2f, 0.2f, 0.5f, 1 }
        );
    universal_property<fcol4, fcol4, fcol4> ill_data(
            fcol4 { -2, 3, 4, 1 },
            fcol4 { 1, 1, 1, 1 },
            fcol4 { 0, 1, 5, 1 }
        );

    fmat4 model_mat = tf::identity<float>();
    fmat4 view_mat = inverse(
            tf::rotate<float>(M_PI / 8, tf::yOz) *
            tf::translate<float>(item_get<2>(ill_data)));
    fmat4 proj_mat = tf::perspective<float>(M_PI / 4, 4.0 / 3, 1, 100);

    universal_property<fmat4, fmat4> mvp_data(
            proj_mat * view_mat * model_mat,
            model_mat
        );

    auto pmvp = provider<decltype(mvp_data), property_buffer>::load(mvp_data);
    auto pill = provider<decltype(ill_data), property_buffer>::load(ill_data);
    auto pmat = provider<decltype(mat_data), property_buffer>::load(mat_data);

    s.property("transfrm", pmvp);
    s.property("illum", pill);
    s.property("material", pmat);

    update = [&]() {
        model_mat *= tf::rotate<float>(M_PI / 120, tf::zOx);

        item_get<0>(mvp_data) = proj_mat * view_mat * model_mat;
        item_get<1>(mvp_data) = model_mat;

        provider<decltype(mvp_data), property_buffer>::update(
                mvp_data, pmvp, true);
    };

    draw = [&]() {
        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        s.draw(a);
    };

    main_loop();
}

int main(int argc, char* argv[])
{
    scm_init_guile();
    gui_test_context::init("330 core", "test_prov_shading");
    test_main(argc, argv);
}

