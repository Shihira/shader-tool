#ifndef RENDER_ASSETS_H_INCLUDED
#define RENDER_ASSETS_H_INCLUDED

#include <utility>
#include <string>
#include <vector>
#include <type_traits>
#include <memory>
#include <unordered_map>

#include "singleton.h"

/*
 * NOTE: A render asset must not contain the reference of another.
 */

namespace shrtool {

typedef uint32_t id_type;

template<typename Derived>
class __lazy_id_object {
public:
    id_type id() const {
        if(!_id) _id = reinterpret_cast<const Derived*>(this)->create_object();
        return _id;
    }

    ~__lazy_id_object() {
        if(_id) reinterpret_cast<const Derived*>(this)->destroy_object(_id);
    };

    __lazy_id_object() { }
    __lazy_id_object(const __lazy_id_object& l) = delete;
    __lazy_id_object(__lazy_id_object&& l) { std::swap(_id, l._id); }

    __lazy_id_object& operator=(const __lazy_id_object& rhs) = delete;
    __lazy_id_object& operator=(__lazy_id_object&& rhs) {
        std::swap(_id, rhs._id);
        return *this;
    }

    typedef std::shared_ptr<Derived> ptr;
    typedef std::weak_ptr<Derived> wptr;

private:
    mutable id_type _id = 0;
};

////////////////////////////////////////////////////////////////////////////////
// render assets

namespace render_assets {

class texture : public __lazy_id_object<texture> {
public:
    enum filter_type {
        LINEAR,
        NEAREST
    };

    enum format {
        DEFAULT_FMT,
        RGBA_U8888,
        R_F32,
    };

    id_type create_object() const;
    void destroy_object(id_type i) const;

protected:
    unsigned _level = 0;
    filter_type _filter;
    format _format = DEFAULT_FMT;

public:
    unsigned level() const { return _level; }
    void level(unsigned l) { _level = l; }
    format internal_format() const { return _format; }
    void internal_format(format f) { _format = f; }
};

class texture2d : public texture {
    size_t __w;
    size_t __h;

public:
    texture2d(size_t width, size_t height, format ifmt) :
        __w(width), __h(height) {
        internal_format(ifmt);
    }
    void fill(void* data, format fmt);

    size_t width() const { return __w; }
    void width(size_t w) { __w = w; }
    size_t height() const { return __h; }
    void height(size_t h) { __h = h; }
};

class texture_cubemap2d : public texture {
    size_t __edge_len;

public:
    enum face_index {
        FRONT, BACK,
        TOP, BOTTOM,
        LEFT, RIGHT,
    };

    texture_cubemap2d(size_t edge_len, format ifmt) :
        __edge_len(edge_len) {
        internal_format(ifmt);
    }

    void fill(face_index f, void* data, format fmt);
    size_t edge_length() const { return __edge_len; }
};

class buffer : public __lazy_id_object<buffer> {
protected:
    size_t _size = 0;

public:
    enum buffer_transfer_mode { STATIC = 0, DYNAMIC = 1 };
    enum buffer_access { WRITE = 0, READ = 2 };

    virtual void write(void* data, size_t size) = 0;

    buffer_transfer_mode transfer_mode() const { return __trans_mode; }
    void transfer_mode(buffer_transfer_mode bt) { __trans_mode = bt; }
    buffer_access access() const { return __access; }
    void access(buffer_access ba) { __access = ba; }
    size_t size() const { return _size; }

    id_type create_object() const;
    void destroy_object(id_type i) const;

private:
    buffer_transfer_mode __trans_mode = STATIC;
    buffer_access __access = WRITE;
};

class vertex_attr_buffer : public buffer {
public:
    void write(void* data, size_t size);
};

class property_buffer : public buffer {
public:
    void write(void* data, size_t size);
};

}

#define DEF_ENUM_MAP(fn, from_type, to_type, map_content) \
    static to_type fn(from_type e) { \
        static const std::unordered_map<from_type, to_type> \
            __trans_map map_content; \
        auto __i = __trans_map.find(e); \
        if(__i == __trans_map.end()) \
            throw shrtool::enum_map_error( \
                    std::string("Failed when mapping ") + #fn); \
        return __i->second; \
    }

}



#endif // RENDER_ASSETS_H_INCLUDED
