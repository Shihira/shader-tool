#include <vector>
#include <fstream>

#define EXPOSE_EXCEPTION

#include "test_utils.h"
#include "providers.h"
#include "common/mesh.h"
#include "shader_parser.h"
#include "properties.h"
#include "common/image.h"

using namespace shrtool;
using namespace shrtool::math;
using namespace shrtool::render_assets;
using namespace std;

TEST_CASE_FIXTURE(test_sphere, singlefunc_fixture)
{
    camera cam;
    cam.set_bgcolor(color(51, 51, 51));
    cam.set_depth_test(true);

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
            fcol4 { 0, 2, 4, 1 }
        );
    cam.transformation()
        .rotate(math::PI / 8, tf::yOz)
        .translate(item_get<2>(ill_data));

    transfrm m;

    auto pm = provider<decltype(m), property_buffer>::load(m);
    auto pvp = provider<decltype(cam), property_buffer>::load(cam);
    auto pill = provider<decltype(ill_data), property_buffer>::load(ill_data);
    auto pmat = provider<decltype(mat_data), property_buffer>::load(mat_data);

    s.target(cam);
    s.property("transfrm", pm);
    s.property("camera", pvp);
    s.property("illum", pill);
    s.property("material", pmat);

    update = [&]() {
        m.rotate(math::PI / 120, tf::zOx);

        provider<decltype(m), property_buffer>::update(m, pm, true);
    };

    draw = [&]() {
        cam.clear_buffer(render_target::COLOR_BUFFER);
        cam.clear_buffer(render_target::DEPTH_BUFFER);

        s.draw(a);
    };

    main_loop();
}

TEST_CASE_FIXTURE(test_earth, singlefunc_fixture)
{
    camera cam;
    cam.set_bgcolor(color(51, 51, 51));
    cam.set_depth_test(true);
    cam.set_visible_angle(cam.get_visible_angle() / 3);
    cam.transformation().translate(0, 0, 15);

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

    transfrm m;
    m.rotate(-math::PI / 6, tf::yOz);

    auto pm = provider<decltype(m), property_buffer>::load(m);
    auto pvp = provider<decltype(cam), property_buffer>::load(cam);
    auto pill = provider<decltype(ill_data), property_buffer>::load(ill_data);
    auto pmat = provider<decltype(mat_data), property_buffer>::load(mat_data);

    s.target(cam);
    s.property("transfrm", pm);
    s.property("camera", pvp);
    s.property("illum", pill);
    s.property("material", pmat);
    s.property("texMap", tex);

    update = [&]() {
        m.rotate(math::PI / 6, tf::yOz)
         .rotate(math::PI / 480, tf::zOx)
         .rotate(-math::PI / 6, tf::yOz);

        provider<decltype(m), property_buffer>::update(m, pm, true);
    };

    draw = [&]() {
        cam.clear_buffer(render_target::COLOR_BUFFER);
        cam.clear_buffer(render_target::DEPTH_BUFFER);

        s.draw(a);
    };

    main_loop();
}

TEST_CASE_FIXTURE(test_render_target, singlefunc_fixture) {
    texture2d fb_tex(800, 600, texture::RGBA_U8888);
    texture2d fb_dep(800, 600, texture::DEPTH_F32);
    image out_img(800, 600);

    camera cam;
    cam.attach_texture(render_target::COLOR_BUFFER_0, fb_tex);
    cam.attach_texture(render_target::DEPTH_BUFFER, fb_dep);
    cam.set_depth_test(true);
    cam.set_bgcolor(color(51, 51, 51));
    cam.transformation()
        .translate(0, 0, 5)
        .rotate(math::PI / 8, tf::yOz);

    ifstream shr_fs(locate_assets("shaders/solid-color.scm"));
    ofstream img_fs("output.ppm");

    shader_info si = shader_parser::load(shr_fs);
    auto s = provider<shader_info, shader>::load(si);
    mesh_plane ms(3, 3, 5, 5);
    auto a = provider<mesh_plane, vertex_attr_vector>::load(ms);

    universal_property<fcol4> mat_data(fcol4{ 0.2f, 0.2f, 0.5f, 1 });

    transfrm m;

    auto pm = provider<decltype(m), property_buffer>::load(m);
    auto pvp = provider<decltype(cam), property_buffer>::load(cam);
    auto pmat = provider<decltype(mat_data), property_buffer>::load(mat_data);

    s.target(cam);
    s.property("transfrm", pm);
    s.property("camera", pvp);
    s.property("material", pmat);
    cam.clear_buffer(render_target::COLOR_BUFFER);
    cam.clear_buffer(render_target::DEPTH_BUFFER);
    s.draw(a);

    fb_tex.read(out_img.data(), texture::RGBA_U8888);
    out_img.flip_v(); // images got from GL are upside-down
    image_io_netpbm::save_image(img_fs, out_img);
}

#include <libguile.h>

int main(int argc, char* argv[])
{
    scm_init_guile();
    gui_test_context::init("330 core", "test_prov_shading");
    return unit_test::test_main(argc, argv);
}

