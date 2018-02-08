#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <functional>

#include "animation.h"
#include "common/exception.h"
#include "common/logger.h"

using namespace Assimp;
using namespace std;

namespace shrtool {

static math::col4 vec3_to_col4(const aiVector3D& v) {
    math::col4 c;
    c[0] = v.x;
    c[1] = v.y;
    c[2] = v.z;
    c[3] = 1;

    return c;
}

static math::col3 vec3_to_col3(const aiVector3D& v) {
    math::col3 c;
    c[0] = v.x;
    c[1] = v.y;
    c[2] = v.z;

    return c;
}

static math::mat4 mat4_conv(const aiMatrix4x4 m) {
    return math::mat4 {
        m.a1, m.a2, m.a3, m.a4,
        m.b1, m.b2, m.b3, m.b4,
        m.c1, m.c2, m.c3, m.c4,
        m.d1, m.d2, m.d3, m.d4,
    };
}

static void walk_thru_hierarchy(
        aiNode* root_node,
        const function<void(aiNode*)>& cb,
        bool preorder = true /*false to be postorder*/)
{
    if(preorder)
        cb(root_node);

    for(size_t chl_i = 0; chl_i < root_node->mNumChildren; chl_i++) {
        aiNode* chl = root_node->mChildren[chl_i];

        if(chl->mParent != root_node) {
            error_log << "Ill-formed structure: \""
                << chl->mName.C_Str() << "\" - \""
                << root_node->mName.C_Str() << "\"" << endl;
        }

        walk_thru_hierarchy(chl, cb, preorder);
    }

    if(!preorder)
        cb(root_node);
}

template<typename T, size_t M>
static size_t find_available_index(const math::col<T, M>& c) {
    size_t i = 0;
    for(const T* v = c.data(); v[i] < 0 && i < M; i++);
    return i;
}

static void parse_mesh(aiMesh* aim,
        std::vector<animation_io_assimp::mesh_type>& meshes)
{
    animation_io_assimp::mesh_type cur_mesh;

    for(size_t vert_i = 0; vert_i < aim->mNumVertices; vert_i++) {
        if(aim->HasPositions())
            cur_mesh.stor_positions->push_back(
                    vec3_to_col4(aim->mVertices[vert_i]));
        if(aim->HasNormals())
            cur_mesh.stor_normals->push_back(
                    vec3_to_col3(aim->mNormals[vert_i]));
        if(aim->HasTextureCoords(0))
            cur_mesh.stor_uvs->push_back(
                    vec3_to_col3(aim->mTextureCoords[0][vert_i]));

        if(aim->HasBones()) {
            cur_mesh.stor_bone_indices->push_back(
                    math::col4 { -1, -1, -1, -1 });
            cur_mesh.stor_weights->push_back(
                    math::col4 { 0, 0, 0, 0 });
        }
    }

    if(aim->HasBones()) {
        cur_mesh.stor_weights->resize(aim->mNumVertices);
        cur_mesh.stor_bone_indices->resize(aim->mNumVertices);
    }

    for(size_t facet_i = 0; facet_i < aim->mNumFaces; facet_i++) {
        for(size_t vert_i = 0; vert_i < aim->mFaces[facet_i].mNumIndices; vert_i++) {
            if(aim->HasPositions())
                cur_mesh.positions.indices.push_back(
                    aim->mFaces[facet_i].mIndices[vert_i]);
            if(aim->HasNormals())
                cur_mesh.normals.indices.push_back(
                    aim->mFaces[facet_i].mIndices[vert_i]);
            if(aim->HasTextureCoords(0))
                cur_mesh.uvs.indices.push_back(
                    aim->mFaces[facet_i].mIndices[vert_i]);

            if(aim->HasBones()) {
                cur_mesh.weights.indices.push_back(
                    aim->mFaces[facet_i].mIndices[vert_i]);
                cur_mesh.bone_indices.indices.push_back(
                    aim->mFaces[facet_i].mIndices[vert_i]);
            }
        }
    }

    meshes.emplace_back(std::move(cur_mesh));
}

void animation_io_assimp::load_into_meshes(
        std::istream& is, anim_tuple& at,
        const std::string& format)
{
    std::string str((std::istreambuf_iterator<char>(is)),
             std::istreambuf_iterator<char>());

    Importer imp;
    const aiScene* scn = imp.ReadFileFromMemory(str.data(), str.size(),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals, format.c_str());

    if(!scn) {
        throw parse_error(imp.GetErrorString());
    }

    std::vector<mesh_type>& meshes = get<1>(at);

    vector<skeletal_animation> cur_channels;
    vector<bone> bone_set;
    map<string, int> bone_name_index;

    walk_thru_hierarchy(scn->mRootNode, [&](aiNode* n) {
        string name = n->mName.C_Str();

        if(n->mNumMeshes > 0) {
            for(size_t i = 0; i < n->mNumMeshes; i++) {
                parse_mesh(scn->mMeshes[n->mMeshes[i]], meshes);
                meshes.back().name = name;
            }
        }

        auto b_iter = bone_name_index.find(name);

        if(b_iter == bone_name_index.end()) {
            bone b;
            b.name = name;
            b.index = bone_set.size();

            bone_name_index[name] = b.index;

            bone_set.emplace_back(std::move(b)); // size increases here
            b_iter = bone_name_index.find(name);
        }

        bone& b = bone_set[b_iter->second];
        b.transfrm.set_mat(mat4_conv(n->mTransformation));

        if(/*n != root_ai_node &&*/ n->mParent) {
            auto p_iter = bone_name_index.find(n->mParent->mName.C_Str());
            if(p_iter == bone_name_index.end())
                throw parse_error("Ill-formed structure.");

            bone_set[p_iter->second].children.push_back(b_iter->second);
            b.parent = p_iter->second;
        }
    });

    for(size_t mesh_i = 0; mesh_i < scn->mNumMeshes; mesh_i++) {
        aiMesh* aim = scn->mMeshes[mesh_i];
        auto& mesh = meshes[mesh_i];

        for(size_t bone_i = 0; bone_i < aim->mNumBones; bone_i++) {
            aiBone* bn = aim->mBones[bone_i];
            auto b_finder = bone_name_index.find(bn->mName.C_Str());
            if(b_finder == bone_name_index.end())
                throw parse_error("Mesh refers to a bone that doesn't exist");
            int b_idx = b_finder->second;

            for(size_t bv_i = 0; bv_i < bn->mNumWeights; bv_i++) {
                aiVertexWeight& vw = bn->mWeights[bv_i];
                size_t usable_idx = find_available_index(
                        mesh.stor_bone_indices->at(vw.mVertexId));
                mesh.stor_bone_indices->at(vw.mVertexId)[usable_idx] = b_idx;
                mesh.stor_weights->at(vw.mVertexId)[usable_idx] = vw.mWeight;
            }
        }
    }

    skeleton& skel = get<0>(at);
    skel.root_bone_index = bone_name_index[scn->mRootNode->mName.C_Str()];
    skel.bone_set = std::move(bone_set);
    skel.bone_name_index = std::move(bone_name_index);
    skel.rebuild_bone_structure();

    imp.FreeScene();
}

bone* bone::get_nth_children(int idx) const
{
    if(idx >= skel->bone_set.size()) return nullptr;
    return &skel->bone_set[children[idx]];
}

bone* bone::get_parent() const
{
    if(parent < 0) return nullptr;
    return &skel->bone_set[parent];
}

math::mat4 bone::get_model_mat() const
{
    math::mat4 m = math::tf::identity();

    const bone* cur = this;

    while(cur) {
        m = cur->transfrm.get_mat() * m;
        cur = cur->get_parent();
    }

    return m;
}

}
