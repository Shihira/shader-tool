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
class lazy_id_object_ {
public:
    id_type id() const {
        if(!_id) _id = reinterpret_cast<const Derived*>(this)->create_object();
        return _id;
    }

    ~lazy_id_object_() {
        if(_id) reinterpret_cast<const Derived*>(this)->destroy_object(_id);
    };

    lazy_id_object_() { }
    lazy_id_object_(const lazy_id_object_& l) = delete;
    lazy_id_object_(lazy_id_object_&& l) { std::swap(_id, l._id); }

    lazy_id_object_& operator=(const lazy_id_object_& rhs) = delete;
    lazy_id_object_& operator=(lazy_id_object_&& rhs) {
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

class texture : public lazy_id_object_<texture> {
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
    filter_type filter() const { return _filter; }
    void filter(filter_type f) { _filter = f; }

    virtual void bind_to(size_t tex_bind) const = 0;
};

class texture2d : public texture {
    size_t w_;
    size_t h_;

public:
    texture2d(size_t width, size_t height, format ifmt = DEFAULT_FMT) :
        w_(width), h_(height) {
        internal_format(ifmt);
    }
    void fill(void* data, format fmt);

    size_t width() const { return w_; }
    void width(size_t w) { w_ = w; }
    size_t height() const { return h_; }
    void height(size_t h) { h_ = h; }

    // there is a texture limitation for each render pass in graphical dirvers
    // this is for binding textures to a number for the current render pass
    // usually called by shader::draw
    virtual void bind_to(size_t tex_bind) const;
};

class texture_cubemap2d : public texture {
    size_t edge_len_;

public:
    enum face_index {
        FRONT, BACK,
        TOP, BOTTOM,
        LEFT, RIGHT,
    };

    texture_cubemap2d(size_t edge_len, format ifmt = DEFAULT_FMT) :
        edge_len_(edge_len) {
        internal_format(ifmt);
    }

    void fill(face_index f, void* data, format fmt);
    size_t edge_length() const { return edge_len_; }

    virtual void bind_to(size_t tex_bind) const;
};

class buffer : public lazy_id_object_<buffer> {
protected:
    size_t size_ = 0;

public:
    /*
     * You can choose to specify the size on construction or not. If not, you
     * must specify a size during write or map. They are mutual exclusive:
     * you cannot specify the size a second time, except that sizes are equal.
     *
     * Each time you calling a write/start_map without leaving sz == 0 means
     * you want to reset the size, when please keep the size equal.
     */
    buffer(size_t sz = 0) : size_(sz) { }
    enum buffer_transfer_mode { STATIC = 0, DYNAMIC = 1 };
    enum buffer_access { NO_ACCESS = 0, WRITE = 4, READ = 2 , RW = 6};

    virtual void write(void* data, size_t sz = 0) = 0;
    virtual void* start_map(buffer_access bt, size_t sz = 0) = 0;
    virtual void stop_map() = 0;

    template<typename T>
    T* start_map(buffer_access bt, size_t sz = 0) {
        return static_cast<T*>(start_map(bt, sz * sizeof(T)));
    }

    buffer_transfer_mode transfer_mode() const { return trans_mode_; }
    void transfer_mode(buffer_transfer_mode bt) { trans_mode_ = bt; }
    buffer_access access() const { return access_; }
    void access(buffer_access ba) { access_ = ba; }
    size_t size() const { return size_; }
    void size(size_t sz);

    id_type create_object() const;
    void destroy_object(id_type i) const;

protected:
    buffer_access mapping_state_ = NO_ACCESS;

private:
    buffer_transfer_mode trans_mode_ = STATIC;
    buffer_access access_ = WRITE;
};

class vertex_attr_buffer : public buffer {
public:
    void write(void* data, size_t sz = 0) override;
    using buffer::start_map;
    void* start_map(buffer_access bt, size_t sz = 0) override;
    void stop_map() override;
};

class property_buffer : public buffer {
public:
    void write(void* data, size_t sz = 0) override;
    using buffer::start_map;
    void* start_map(buffer_access bt, size_t sz = 0) override;
    void stop_map() override;
};

}

#define DEF_ENUM_MAP(fn, from_type, to_type, map_content) \
    static to_type fn(from_type e) { \
        static const std::unordered_map<from_type, to_type> \
            trans_map_ map_content; \
        auto i_ = trans_map_.find(e); \
        if(i_ == trans_map_.end()) \
            throw shrtool::enum_map_error( \
                    std::string("Failed when mapping ") + #fn); \
        return i_->second; \
    }

}



#endif // RENDER_ASSETS_H_INCLUDED
