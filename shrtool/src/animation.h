#ifndef ANIMATION_H_INCLUDED
#define ANIMATION_H_INCLUDED

#include <vector>
#include <string>
#include <tuple>

#include "common/utilities.h"
#include "common/mesh.h"

namespace shrtool {

struct skeleton;

struct bone {
    skeleton* skel;

    std::string name;
    int index;

    std::vector<int> children;
    int parent = -1;

    trs_transfrm transfrm;

    bone* get_nth_children(int idx) const;
    bone* get_parent() const;

    math::mat4 get_model_mat() const;
};

struct skeleton {
    int root_bone_index;

    std::vector<bone> bone_set;
    std::map<std::string, int> bone_name_index;

    bone& get_root_bone() {
        return bone_set[root_bone_index];
    }

    skeleton() { }

    skeleton(skeleton&& sk) :
            root_bone_index(sk.root_bone_index),
            bone_set(std::move(sk.bone_set)),
            bone_name_index(std::move(sk.bone_name_index)) {
        rebuild_bone_structure();
    }

    skeleton(const skeleton& sk) :
            root_bone_index(sk.root_bone_index),
            bone_set(sk.bone_set),
            bone_name_index(sk.bone_name_index) {
        rebuild_bone_structure();
    }

    void rebuild_bone_structure() {
        for(bone& b : bone_set)
            b.skel = this;
    }
};

struct skeletal_animation {
    typedef std::map<int, trs_transfrm> keyframe;

    std::map<std::string, keyframe> channels;

    PROPERTY_RW(bone*, root_bone);
    PROPERTY_RW(int, start_tick);
    PROPERTY_RW(int, stop_tick);
    PROPERTY_RW(int, tick_per_second);
};

struct animation_io_assimp {
    typedef mesh_indexed mesh_type;
    typedef std::tuple<
        skeleton,
        std::vector<mesh_type>,
        std::vector<skeletal_animation>> anim_tuple;

    anim_tuple* anim;

    static void load_into_meshes(std::istream& is,
            anim_tuple& at,
            const std::string& format = "FBX");

    static anim_tuple load(
            std::istream& is, const std::string fmt = "FBX") {
        anim_tuple at;
        load_into_meshes(is, at, fmt);
        return at;
    }
};

}

#endif // ANIMATION_H_INCLUDED
