#include <vector>
#include <fstream>

#define EXPOSE_EXCEPTION
#include "test_utils.h"
#include "providers.h"
#include "mesh.h"
#include "shader_parser.h"
#include "properties.h"
#include "image.h"

using namespace shrtool;
using namespace shrtool::math;
using namespace shrtool::render_assets;
using namespace std;

TEST_CASE_FIXTURE(test_sphere, singlefunc_fixture)
{
    render_target::screen.initial_color(0.2, 0.2, 0.2, 1);
    render_target::screen.enable_depth_test(true);

    ifstream shr_fs(locate_assets("shaders/blinn-phong.scm"));

    shader_info si = shader_parser::load(shr_fs);
    auto s = provider<shader_info, shader>::load(si);
    mesh_uv_sphere ms(2, 16, 8, false);
    auto a = provider<mesh_uv_sphere, vertex_attr_vector>::load(ms);

    universal_property<fcol4, fcol4, fcol4> mat_data(
            col4 { 0.1f, 0.1f, 0.13f, 1 },
            col4 { 0.7f, 0.7f, 0.7f, 1 },
            col4 { 0.2f, 0.2f, 0.5f, 1 }
        );
    universal_property<fcol4, fcol4, fcol4> ill_data(
            col4 { -2, 3, 4, 1 },
            col4 { 1, 1, 1, 1 },
            col4 { 0, 1, 5, 1 }
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

    s->property("mvp", *pmvp);
    s->property("illum", *pill);
    s->property("material", *pmat);

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

        s->draw(*a);
    };

    main_loop();
}

TEST_CASE_FIXTURE(test_earth, singlefunc_fixture)
{
    render_target::screen.initial_color(0.2, 0.2, 0.2, 1);
    render_target::screen.enable_depth_test(true);

    ifstream shr_fs(locate_assets("shaders/lambertian-texture.scm"));
    ifstream map_fs(locate_assets("textures/earth_m.ppm"));

    shader_info si = shader_parser::load(shr_fs);
    auto s = provider<shader_info, shader>::load(si);
    mesh_uv_sphere ms(3, 100, 50);
    auto a = provider<mesh_uv_sphere, vertex_attr_vector>::load(ms);
    image img = image_io_netpbm::load(map_fs);
    auto tex = provider<image, render_assets::texture2d>::load(img);

    universal_property<float, float, fcol2> mat_data(0.1, 0.9, fcol2 {0, 0});
    universal_property<fcol4> ill_data( col4 { -300, 0, 50, 1 }); // far~~

    fmat4 model_mat = tf::identity<float>();
    fmat4 view_mat = inverse(
            tf::rotate<float>(M_PI / 6, tf::yOz) *
            tf::translate<float>(col4{0, 0, 5, 1}));
    fmat4 proj_mat = tf::perspective<float>(M_PI / 4, 4.0 / 3, 1, 100);

    universal_property<fmat4, fmat4> mvp_data(
            proj_mat * view_mat * model_mat,
            model_mat
        );

    auto pmvp = provider<decltype(mvp_data), property_buffer>::load(mvp_data);
    auto pill = provider<decltype(ill_data), property_buffer>::load(ill_data);
    auto pmat = provider<decltype(mat_data), property_buffer>::load(mat_data);

    s->property("mvp", *pmvp);
    s->property("illum", *pill);
    s->property("material", *pmat);
    s->property("texMap", *tex);

    update = [&]() {
        model_mat *= tf::rotate<float>(M_PI / 480, tf::zOx);

        item_get<0>(mvp_data) = proj_mat * view_mat * model_mat;
        item_get<1>(mvp_data) = model_mat;

        provider<decltype(mvp_data), property_buffer>::update(
                mvp_data, pmvp, true);
    };

    draw = [&]() {
        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        s->draw(*a);
    };

    main_loop();
}

int main(int argc, char* argv[])
{
    gui_test_context::init("330 core", "test_prov_shading");
    return unit_test::test_main(argc, argv);
}

