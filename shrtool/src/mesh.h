#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include <vector>
#include <array>
#include <memory>

#include "matrix.h"
#include "image.h"

namespace shrtool {

/*
 * mesh_base defines some semantic functions. If you are wondering why has_***
 * is not virtual functions, the answer is: mesh_base is a CRTP base class. you
 * have known the derived class when you know the base class, so you literally
 * have no chance to use a mesh_base reference. A CRTP base class has very
 * pure purpose: adding extra functionalities.
 */
template<typename Derived>
struct mesh_base {
private:
    Derived& self() { return *reinterpret_cast<Derived*>(this); }
    const Derived& self() const {
        return *reinterpret_cast<const Derived*>(this);
    }

public:
    /*
     * // provide at least these member variables as follow
     * // where Container supports size() and operator[](size_t)
     *
     * Container<math::col4> positions;
     * Container<math::col3> normals;
     * Container<math::col3> uvs;
     */

    // you can define your has_** if the member variable's size
    // doesn't means any HAS or HASN'T
    bool has_positions() const { return self().positions.size() > 0; }
    bool has_normals() const { return self().normals.size() > 0; }
    bool has_uvs() const { return self().uvs.size() > 0; }

    bool empty() const {
        return self().positions.size() == 0;
    }

    math::col4& get_position(size_t tri, size_t vert)
        { return self().positions[tri * 3 + vert]; }
    const math::col4& get_position(size_t tri, size_t vert) const
        { return self().positions[tri * 3 + vert]; }
    math::col4& get_normal(size_t tri, size_t vert)
        { return self().normals[tri * 3 + vert]; }
    const math::col4& get_normal(size_t tri, size_t vert) const
        { return self().normals[tri * 3 + vert]; }
    math::col4& get_uv(size_t tri, size_t vert)
        { return self().uvs[tri * 3 + vert]; }
    const math::col4& get_uv(size_t tri, size_t vert) const
        { return self().uvs[tri * 3 + vert]; }
};

struct mesh_basic : mesh_base<mesh_basic> {
public:
    std::vector<math::col4> positions;
    std::vector<math::col3> normals;
    std::vector<math::col3> uvs;

    mesh_basic() { }

    mesh_basic(mesh_basic&& mp) :
        positions(std::move(mp.positions)),
        normals(std::move(mp.normals)),
        uvs(std::move(mp.uvs)) { }

    mesh_basic(const mesh_basic& mp) :
        positions(mp.positions),
        normals(mp.normals),
        uvs(mp.uvs) { }
};

/*
 * indexed_attr give great convenience to people who want to operate on data
 * through indices access, and indexed_attr will do you the conversion.
 */
template<typename T, typename ContainerRef = std::vector<T>*>
struct indexed_attr {
    /*
     * built-in iterator for special purpose
     */
    template<typename ValType>
    struct indexed_attr_iterator :
            std::iterator<std::bidirectional_iterator_tag, ValType> {
        indexed_attr_iterator(indexed_attr& ia, size_t ioi) :
            ia_(ia), index_of_idx_(ioi) { }
        indexed_attr_iterator(const indexed_attr_iterator& other) :
            ia_(other.ia_), index_of_idx_(other.index_of_idx_) { }

        typedef indexed_attr_iterator self_type;

        self_type& operator++() { index_of_idx_++; return *this; }
        self_type operator++(int) {
            self_type other = *this; index_of_idx_++; return other; }
        self_type& operator--() { index_of_idx_--; return *this; }
        self_type operator--(int) {
            self_type other = *this; index_of_idx_--; return other; }

        bool operator!=(const self_type& rhs) const {
            return index_of_idx_ != rhs.index_of_idx_;
        }
        bool operator==(const self_type& rhs) const {
            return index_of_idx_ == rhs.index_of_idx_;
        }

        ValType& operator*() { return (*ia_.refer)[ia_.indices[index_of_idx_]]; }

        self_type& operator=(const self_type& other) {
            index_of_idx_ = other.index_of_idx_;
        }
    private:
        indexed_attr& ia_;
        size_t index_of_idx_;
    };

    ContainerRef& refer;

    typedef ContainerRef container_type_refernce;
    std::vector<size_t> indices;

    indexed_attr(ContainerRef& r) : refer(r) { }
    indexed_attr(indexed_attr&& idx):
        refer(idx.refer),
        indices(std::move(idx.indices)) { }
    indexed_attr(const indexed_attr& idx):
        refer(idx.refer), indices(idx.indices) { }

    indexed_attr_iterator<T> begin() {
        return indexed_attr_iterator<T>(*this, 0);
    }

    indexed_attr_iterator<T> end() {
        return indexed_attr_iterator<T>(*this, indices.size());
    }

    indexed_attr_iterator<const T> begin() const {
        return indexed_attr_iterator<const T>(*this, 0);
    }

    indexed_attr_iterator<const T> end() const {
        return indexed_attr_iterator<const T>(*this, indices.size());
    }

    size_t size() const {
        return indices.size();
    }

    T& operator[](size_t i) { return (*refer)[indices[i]]; }
    const T& operator[](size_t i) const { return (*refer)[indices[i]]; }
};


struct mesh_indexed : mesh_base<mesh_indexed> {
    template<typename T>
    using stor_ptr = std::shared_ptr<std::vector<T>>;

    stor_ptr<math::col4> stor_positions;
    stor_ptr<math::col3> stor_normals;
    stor_ptr<math::col3> stor_uvs;

    indexed_attr<math::col4, stor_ptr<math::col4>> positions;
    indexed_attr<math::col3, stor_ptr<math::col3>> normals;
    indexed_attr<math::col3, stor_ptr<math::col3>> uvs;

    bool has_positions() const {
        return
            stor_positions &&
            stor_positions->size() &&
            positions.size() > 0;
    }

    bool has_normals() const {
        return
            stor_normals &&
            stor_normals->size() &&
            normals.size() > 0;
    }

    bool has_uvs() const {
        return
            stor_uvs &&
            stor_uvs->size() &&
            uvs.size() > 0;
    }

    mesh_indexed(bool init_stor = true) :
        positions(stor_positions),
        normals(stor_normals),
        uvs(stor_uvs) {
        if(init_stor) {
            stor_positions.reset(new std::vector<math::col4>);
            stor_normals.reset(new std::vector<math::col3>);
            stor_uvs.reset(new std::vector<math::col3>);
        }
    }

    mesh_indexed(const mesh_indexed& im) :
        stor_positions(im.stor_positions),
        stor_normals(im.stor_normals),
        stor_uvs(im.stor_uvs),
        positions(im.positions),
        normals(im.normals),
        uvs(im.uvs) { }

    mesh_indexed(mesh_indexed&& im) :
        stor_positions(std::move(im.stor_positions)),
        stor_normals(std::move(im.stor_normals)),
        stor_uvs(std::move(im.stor_uvs)),
        positions(std::move(im.positions)),
        normals(std::move(im.normals)),
        uvs(std::move(im.uvs)) { }
};

struct mesh_io_object {
    typedef mesh_indexed mesh_type;
    typedef std::vector<mesh_type> meshes_type;

    meshes_type* meshes;

    mesh_io_object(meshes_type& ms) : meshes(&ms) { }

    static meshes_type load(std::istream& is) {
        meshes_type ms;
        load_into_meshes(is, ms);
        return std::move(ms);
    }

    std::istream& operator()(std::istream& is) {
        load_into_meshes(is, *meshes);
        return is;
    }

    static void load_into_meshes(std::istream& is, meshes_type& ms);
};

}

#endif // MESH_H_INCLUDED
