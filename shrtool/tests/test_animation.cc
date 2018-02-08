#define EXPOSE_EXCEPTION
#define TEST_SUITE "test_animation"

#include "common/unit_test.h"
#include "test_utils.h"
#include "animation.h"
#include "render_queue.h"

#include <fstream>
#include <libguile.h>

using namespace std;
using namespace shrtool;
using namespace shrtool::math;
using namespace shrtool::unit_test;

void print_skeleton_tree(bone* b, int level = 0) {
    for(int i = 0; i < level; i++)
        ctest << "    ";
    ctest << (b->children.empty() ? "- " : "+ ");
    ctest << b->name << "[ " <<
        "Position: " << b->transfrm.get_position() <<
        "Rotation: " << b->transfrm.get_rotation() <<
        "Scaling: " << b->transfrm.get_scaling() << " ]" << endl;

    for(int i = 0; i < b->children.size(); i++)
        print_skeleton_tree(b->get_nth_children(i), level + 1);
}

TEST_CASE_FIXTURE(test_draw_model, singlefunc_fixture) {
    ifstream shr_fs(locate_assets("shaders/blinn-phong.scm"));
    shader_info si = shader_parser::load(shr_fs);

    ifstream fin(locate_assets("models/soldier.fbx"));
    auto anim = animation_io_assimp::load(fin);

    print_skeleton_tree(&get<0>(anim).get_root_bone());
    mesh_box box(1, 1, 1);

    camera cam;
    cam.set_bgcolor(color(51, 51, 51));
    cam.set_depth_test(true);

    col3 model_pos(find_average(get<1>(anim)));

    universal_property<fcol4, fcol4, fcol4> mat_data(
            fcol4 { 0.1f, 0.1f, 0.1f, 1 },
            fcol4 { 0.7f, 0.7f, 0.7f, 1 },
            fcol4 { 0.5f, 0.5f, 0.5f, 1 }
        );
    universal_property<fcol4, fcol4, fcol4> ill_data(
            fcol4 { -2, 3, 4, 1 },
            fcol4 { 1, 1, 1, 1 },
            fcol4 { 0, 2, 5, 1 }
        );

    cam.transformation()
        .rotate(math::PI / 8, tf::yOz)
        .translate(item_get<2>(ill_data));

    provided_render_task::provider_bindings bindings;
    provided_render_task t(bindings);
    t.set_shader(si);
    t.set_target(cam);
    t.set_property("camera", cam);
    t.set_property("illum", ill_data);
    t.set_property("material", mat_data);

    skeleton& skel = get<0>(anim);
    vector<pair<mat4, double>> bone_mats;
    for(bone& b : skel.bone_set) {
        bone* cur = b.get_parent();

        if(cur)
            bone_mats.push_back(make_pair(cur->get_model_mat(),
                norm(b.transfrm.get_position())));
        else
            bone_mats.push_back(make_pair(math::tf::identity(),
                norm(b.transfrm.get_position())));
    }

    camera_helper ch(this, &cam);
    ch.rotate_axis = col3 { 0, 0, 0 };

    bool show_bones = true;

    keypress = [&](int key, int modifiers) {
        if(key == GLFW_KEY_SPACE) {
            show_bones = !show_bones;
        }
    };

    draw = [&]() {
        cam.clear_buffer(render_target::COLOR_BUFFER);
        cam.clear_buffer(render_target::DEPTH_BUFFER);

        if(show_bones) {
            t.set_attributes(box);
            for(auto& p : bone_mats) {
                if(p.second < 0.0001)
                    continue;

                transfrm tf;
                tf.translate(0, 0.5, 0);
                tf.scale(p.second / 10, p.second, p.second / 10);
                tf *= p.first;
                tf.scale(1 / 20.0);

                t.set_property("transfrm", tf);
                t.render();
            }
        }

        if(!show_bones) {
            for(auto& m : get<1>(anim)) {
                t.set_attributes(m);

                transfrm tf;
                auto bone_i = skel.bone_name_index.find(m.name);
                if(bone_i != skel.bone_name_index.end()) {
                    tf.set_mat(skel.bone_set[bone_i->second].get_model_mat());
                }
                tf.scale(1 / 20.0);

                t.set_property("transfrm", tf);
                t.render();
            }
        }
    };

    main_loop();
}

int main(int argc, char* argv[])
{
    scm_init_guile();
    gui_test_context::init();
    return unit_test::test_main(argc, argv);
}

