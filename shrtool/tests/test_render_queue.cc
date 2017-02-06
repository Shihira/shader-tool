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

struct rotate_model_fixture : singlefunc_fixture {
    bool actions[4] = { false, false, false, false, };

    bool update_helper(math::fmat4& model_mat) {
        bool updated = false;

        if(actions[0] && (updated |= actions[0]))
            model_mat = tf::rotate<float>(M_PI / 120, tf::zOx) * model_mat;
        if(actions[1] && (updated |= actions[1]))
            model_mat = tf::rotate<float>(-M_PI / 120, tf::zOx) * model_mat;
        if(actions[2] && (updated |= actions[2]))
            model_mat = tf::rotate<float>(M_PI / 120, tf::yOz) * model_mat;
        if(actions[3] && (updated |= actions[3]))
            model_mat = tf::rotate<float>(-M_PI / 120, tf::yOz) * model_mat;

        return updated;
    }

    void keyrelease_helper(int key, int mod) {
        if(key == GLFW_KEY_LEFT) actions[0] = false;
        else if(key == GLFW_KEY_RIGHT) actions[1] = false;
        else if(key == GLFW_KEY_UP) actions[2] = false;
        else if(key == GLFW_KEY_DOWN) actions[3] = false;
    };

    void keypress_helper(int key, int mod) {
        if(key == GLFW_KEY_LEFT) actions[0] = true;
        else if(key == GLFW_KEY_RIGHT) actions[1] = true;
        else if(key == GLFW_KEY_UP) actions[2] = true;
        else if(key == GLFW_KEY_DOWN) actions[3] = true;
    };
};

TEST_CASE_FIXTURE(test_shading, rotate_model_fixture) {
    render_target::screen.initial_color(0.2, 0.2, 0.2, 1);
    render_target::screen.enable_depth_test(true);

    ifstream shr_fs(locate_assets("shaders/blinn-phong.scm"));
    ifstream tp_fs(locate_assets("models/teapot.obj"));
    ifstream chs_fs(locate_assets("models/chess.obj"));

    shader_info si = shader_parser::load(shr_fs);
    mesh_uv_sphere ms(2, 16, 8, false);
    vector<mesh_indexed> tp = mesh_io_object::load(tp_fs);
    vector<mesh_indexed> chs = mesh_io_object::load(chs_fs);

    size_t mesh_i = 0;
    vector<mesh_indexed*> meshes = { &ms, &tp[0], &chs[2] };
    vector<fmat4> mesh_mats = {
        tf::identity<float>(),
        tf::scale<float>(1./30, 1./30, 1./30),
        tf::scale<float>(1./60, 1./60, 1./60) *
            tf::translate<float>(- find_average(chs[2]).cutdown<fcol3>()),
    };

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

    fmat4 view_mat = inverse(
            tf::rotate<float>(M_PI / 8, tf::yOz) *
            tf::translate<float>(item_get<2>(ill_data)));
    fmat4 proj_mat = tf::perspective<float>(M_PI / 4, 4.0 / 3, 1, 100);

    universal_property<fmat4, fmat4> mvp_data(
            proj_mat * view_mat * mesh_mats[mesh_i],
            mesh_mats[mesh_i]
        );

    provided_render_task::provider_bindings bindings;
    provided_render_task t(bindings);
    t.set_shader(si);
    t.set_target(render_target::screen);
    t.set_attributes(ms);
    t.set_property("transfrm", mvp_data);
    t.set_property("illum", ill_data);
    t.set_property("material", mat_data);

    update = [&]() {
        fmat4& model_mat = mesh_mats[mesh_i];
        if(!update_helper(model_mat)) return;

        item_get<0>(!mvp_data) = proj_mat * view_mat * model_mat;
        item_get<1>(!mvp_data) = model_mat;
    };

    keyrelease = [&](int key, int mod) {
        keyrelease_helper(key, mod);
    };

    keypress = [&](int key, int mod) {
        keypress_helper(key, mod);

        if(key == GLFW_KEY_SPACE) {
            mesh_i = (mesh_i + 1) % meshes.size();
            t.set_attributes(*meshes[mesh_i]);

            item_get<0>(!mvp_data) = proj_mat * view_mat * mesh_mats[mesh_i];
            item_get<1>(!mvp_data) = mesh_mats[mesh_i];
        }
    };

    draw = [&]() {
        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        t.render();
    };

    main_loop();
}

TEST_CASE_FIXTURE(test_cubemap, rotate_model_fixture)
{
    render_target::screen.initial_color(0.2, 0.2, 0.2, 1);
    render_target::screen.enable_depth_test(true);

    ifstream fshader(locate_assets("shaders/lambertian-cubemap.scm"));
    ifstream fcubemap(locate_assets("textures/skybox-texture.ppm"));
    //ifstream fcubemap(locate_assets("textures/cubemap.ppm"));

    shader_info shdr = shader_parser::load(fshader);

    image cubemap_org = image_io_netpbm::load(fcubemap);
    image cubemap = image::load_cubemap_from(cubemap_org);
    //image cubemap = image_io_netpbm::load(fcubemap);

    mesh_box box(3, 3, 3);
    mesh_uv_sphere sphere(3, 50, 25);

    universal_property<float, float, fcol2> mat_data(0.1, 0.9, fcol2 {0, 0});
    universal_property<fcol4, fcol4> ill_data(fcol4 { -5, 3, 5, 1 },
            fcol4 { 1, 1, 1, 1 }); // far~~

    fmat4 model_mat = tf::identity<float>();
    fmat4 view_mat = inverse(
            //tf::rotate<float>(M_PI / 6, tf::zOx) *
            //tf::rotate<float>(M_PI / 6, tf::yOz) *
            tf::translate<float>(col4{0, 0, 5, 1}));
    fmat4 proj_mat = tf::perspective<float>(M_PI / 4, 4.0 / 3, 1, 100);

    size_t vmi = 0;
    universal_property<fmat4, fmat4> mvp_data(
            proj_mat * view_mat * model_mat,
            model_mat
        );

    provided_render_task::provider_bindings bindings;
    provided_render_task rtask(bindings);
    rtask.set_shader(shdr);
    rtask.set_target(render_target::screen);
    rtask.set_attributes(box);
    rtask.set_property("illum", ill_data);
    rtask.set_property("material", mat_data);
    rtask.set_property("transfrm", mvp_data);
    rtask.set_texture_property<texture_cubemap>("texMap", cubemap);

    fmat4 view_mat_diff = tf::identity<float>();
    update = [&]() {
        if(!update_helper(view_mat_diff)) return;

        item_get<0>(!mvp_data) = proj_mat * view_mat *
            view_mat_diff * model_mat;
    };

    keyrelease = [&](int key, int mod) {
        keyrelease_helper(key, mod);
    };

    keypress = [&](int key, int mod) {
        keypress_helper(key, mod);

        if(key == GLFW_KEY_SPACE) {
            vmi = (vmi + 1) % 2;
            if(vmi == 0) rtask.set_attributes(box);
            if(vmi == 1) rtask.set_attributes(sphere);
        }
    };

    draw = [&]() {
        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        rtask.render();
    };

    main_loop();
}

int main(int argc, char* argv[])
{
    scm_init_guile();
    gui_test_context::init("330 core", "test_render_queue");
    test_main(argc, argv);
}

