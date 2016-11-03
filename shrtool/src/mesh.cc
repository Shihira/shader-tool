#include <sstream>

#include "mesh.h"
#include "exception.h"

namespace shrtool {

using math::col3;
using math::col4;

inline void read_v(mesh_indexed& m, const std::string& str_v)
{
    col4 v;
    std::istringstream is(str_v);

    for(int i = 0; i < 4; i++) {
        is >> v[i];
        if(is.fail()) {
            if(i == 3) {
                v[3] = 1;
                is.clear();
            } else throw parse_error("Positions is not 3D.");
        }
    }

    m.stor_positions->push_back(v);
}

inline void read_vt(mesh_indexed& m, const std::string& str_vt)
{
    col3 vt;
    std::istringstream is(str_vt);

    for(int i = 0; i < 3; i++) {
        is >> vt[i];
        if(is.fail()) {
            if(i == 2) {
                vt[2] = 1;
                is.clear();
            } else throw parse_error("UV coordinates is not 2D.");
        }
    }

    m.stor_uvs->push_back(vt);
}

inline void read_vn(mesh_indexed& m, const std::string& str_vn)
{
    col3 vn;
    std::istringstream is(str_vn);

    for(int i = 0; i < 3; i++) {
        is >> vn[i];
        if(is.fail()) 
            throw parse_error("Normal vector is not 3D");
    }

    m.stor_normals->push_back(vn);
}

inline void read_f(mesh_indexed& m, const std::string& str_f) {
    // 3 choices for face element:
    //   i. v1 v2 v3 ...
    //  ii. v1//vt1 ...
    // iii. v1/vn1/vt1 ...

    int v, vn, vt;
    std::istringstream is(str_f);

    std::vector<std::tuple<int, int, int>> face;

    while(true) {
        std::string str_fe;
        is >> str_fe;

        if(str_fe.empty()) break;

        if(str_fe.find("//") != str_fe.npos) { // ii.
            int assigned = std::sscanf(str_fe.c_str(), "%d//%d", &v, &vn);
            if(assigned != 2)
                throw parse_error("Face format ill-formed.");
            vt = v;
        } else if(str_fe.find('/') != str_fe.npos) { // iii.
            int assigned = std::sscanf(str_fe.c_str(), "%d/%d/%d", &v, &vt, &vn);
            if(assigned != 3)
                throw parse_error("Face format ill-formed.");
        } else { // i.
            int assigned = std::sscanf(str_fe.c_str(), "%d", &v);
            if(assigned != 1)
                throw parse_error("Face format ill-formed.");
            vn = v;
            vt = v;
        }

        face.push_back(std::make_tuple(v, vn, vt));

        is >> std::ws;
    }

    if(face.size() > 3) {
        // fan rule
        std::vector<std::tuple<int, int, int>> new_face;
        for(size_t i = 1; i < face.size() - 1; i++) {
            new_face.push_back(face[0]);
            new_face.push_back(face[i]);
            new_face.push_back(face[i+1]);
        }
        face = std::move(new_face);
    }

    for(auto& f : face) {
        std::tie(v, vn, vt) = f;

        // handle negative indices
        if(v < 0) v = m.stor_positions->size() + v; else v--;
        if(vn < 0) vn = m.stor_normals->size() + vn; else vn--;
        if(vt < 0) vt = m.stor_uvs->size() + vt; else vt--;

        m.positions.indices.push_back(v);
        m.normals.indices.push_back(vn);
        m.uvs.indices.push_back(vt);
    }
}

void mesh_io_object::load_into_meshes(
        std::istream& is, meshes_type& ms) {
    std::string name;

    mesh_type::stor_ptr<col4> stor_positions(new std::vector<col4>);
    mesh_type::stor_ptr<col3> stor_normals(new std::vector<col3>);
    mesh_type::stor_ptr<col3> stor_uvs(new std::vector<col3>);

    std::string line;

    auto create_mesh = [&]() {
        ms.emplace_back(false); // false to disable stor init
        mesh_type& current_mesh_ = ms.back();

        current_mesh_.stor_positions = stor_positions;
        current_mesh_.stor_normals = stor_normals;
        current_mesh_.stor_uvs = stor_uvs;
    };

    auto current_mesh = [&]() -> mesh_type& {
        if(ms.empty()) create_mesh();
        return ms.back();
    };

    while(!is.eof() && !is.fail()) {
        std::string cmd;
        is >> std::ws >> cmd >> std::ws;

        std::getline(is, line);

        if(cmd == "g" || cmd == "o") {
            if(!current_mesh().empty())
                create_mesh();
        } else if(cmd == "v") {
            read_v(current_mesh(), line);
        } else if(cmd == "vn") {
            read_vn(current_mesh(), line);
        } else if(cmd == "vt") {
            read_vt(current_mesh(), line);
        } else if(cmd == "f") {
            read_f(current_mesh(), line);
        } else {
            // ignore
        }
    }
}

mesh_uv_sphere::mesh_uv_sphere(double radius, size_t tesel_u, size_t tesel_v)
{
    if(tesel_u < 3 || tesel_v < 2) return;

    // generate vertices, normals, and uvs
    for(size_t v = 0; v <= tesel_v; ++v) {
        double angle_v = double(v) / tesel_v * math::PI;
        double y = radius * std::cos(angle_v);
        // this is the radius of the circle where the current plane (determined
        // by y) intersects with the sphere.
        double r_ = radius * std::sin(angle_v);

        for(size_t u = 0; u < tesel_u; ++u) {
            double angle_u = double(u) / tesel_u * math::PI * 2;
            double x = r_ * std::cos(angle_u);
            double z = r_ * std::sin(angle_u);

            stor_positions->push_back(col4{x, y, z, 1});
            stor_normals->push_back(col3{x/radius, y/radius, z/radius});
            stor_uvs->push_back(col3{double(u) / tesel_u,
                    double(v) / tesel_v, 1});

            // take the center point of each grid in textures on polars
            if(v == 0 || v == tesel_v)
                stor_uvs->back()[0] = (u + 0.5) / tesel_u;
        }
    }

    // generate triangles
    for(size_t v = 0; v < tesel_v; ++v) {
        for(size_t u = 0; u < tesel_u; ++u) {
            size_t i = v * tesel_v + u;
            size_t i_r = v * tesel_v + (u + 1) % tesel_u;
            size_t i_b = i + tesel_u;
            size_t i_rb = i_r + tesel_u;

            if(v != 0) { // not north polar
                positions.indices.push_back(i_r);
                positions.indices.push_back(i);
                positions.indices.push_back(i_b);

                normals.indices.push_back(i_r);
                normals.indices.push_back(i);
                normals.indices.push_back(i_b);

                uvs.indices.push_back(i_r);
                uvs.indices.push_back(i);
                uvs.indices.push_back(i_b);
            }

            if(v != tesel_v - 1) { // not south polar
                positions.indices.push_back(i_b);
                positions.indices.push_back(i_rb);
                positions.indices.push_back(i_r);

                normals.indices.push_back(i_b);
                normals.indices.push_back(i_rb);
                normals.indices.push_back(i_r);

                uvs.indices.push_back(i_b);
                uvs.indices.push_back(i_rb);
                uvs.indices.push_back(i_r);
            }
        }
    }
}

mesh_plane::mesh_plane(size_t tesel_x, size_t tesel_y)
{
}

}

