#ifndef RENDER_ASSETS_H_INCLUDED
#define RENDER_ASSETS_H_INCLUDED

#include <utility>
#include <string>
#include <vector>
#include <type_traits>
#include <memory>
#include <unordered_map>

/*
 * NOTE: A render asset must not contain the reference of another.
 */

namespace shrtool {

typedef uint32_t id_type;

template<typename Derived>
class lazy_id_object_ {
public:
    id_type id() const {
        if(!id_) id_ = reinterpret_cast<const Derived*>(this)->create_object();
        return id_;
    }

    ~lazy_id_object_() {
        if(id_) reinterpret_cast<const Derived*>(this)->destroy_object(id_);
    };

    lazy_id_object_() { }
    lazy_id_object_(const lazy_id_object_& l) = delete;
    lazy_id_object_(lazy_id_object_&& l) { std::swap(id_, l.id_); }

    lazy_id_object_& operator=(const lazy_id_object_& rhs) = delete;
    lazy_id_object_& operator=(lazy_id_object_&& rhs) {
        std::swap(id_, rhs.id_);
        return *this;
    }

    typedef std::shared_ptr<Derived> ptr;
    typedef std::weak_ptr<Derived> wptr;

private:
    mutable id_type id_ = 0;
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
    unsigned level_ = 0;
    filter_type _filter = NEAREST;
    format format_ = DEFAULT_FMT;

public:
    unsigned level() const { return level_; }
    void level(unsigned l) { level_ = l; }
    format internal_format() const { return format_; }
    void internal_format(format f) { format_ = f; }
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

namespace element_type {
    enum element_type_e {
        UNKNOWN = 0, FLOAT, BYTE, UINT
    };

    template<typename T>
    struct element_type_helper {
        static constexpr element_type_e type = UNKNOWN;
    };

    template<>
    struct element_type_helper<float> {
        static constexpr element_type_e type = FLOAT;
    };

    template<>
    struct element_type_helper<uint8_t> {
        static constexpr element_type_e type = BYTE;
    };

    template<>
    struct element_type_helper<uint32_t> {
        static constexpr element_type_e type = UINT;
    };
}

class buffer : public lazy_id_object_<buffer> {
public:
    enum buffer_transfer_mode {
        STATIC = 0, DYNAMIC = 1
    };

    enum buffer_access {
        NO_ACCESS = 0, WRITE = 4, READ = 2 , RW = WRITE | READ
    };

protected:
    size_t size_ = 0;
    buffer_access mapping_state_ = NO_ACCESS;
    buffer_transfer_mode trans_mode_ = STATIC;
    buffer_access access_ = WRITE;
    element_type::element_type_e etype_ = element_type::UNKNOWN;

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

    virtual void write_raw(const void* data, size_t sz = 0) = 0;
    virtual void read_raw(void* data, size_t sz = 0) = 0;

    virtual void* start_map(buffer_access bt, size_t sz = 0) = 0;
    virtual void stop_map() = 0;

    template<typename T>
    void write(const T* data, size_t sz = 0) {
        etype_ = element_type::element_type_helper<T>::type;
        write_raw(data, sz * sizeof(T));
    }

    template<typename T>
    void read(T* data, size_t sz = 0) {
        read_raw(data, sz * sizeof(T));
    }

    template<typename T>
    T* start_map(buffer_access bt, size_t sz = 0) {
        etype_ = element_type::element_type_helper<T>::type;
        return static_cast<T*>(start_map(bt, sz * sizeof(T)));
    }

    buffer_transfer_mode transfer_mode() const { return trans_mode_; }
    void transfer_mode(buffer_transfer_mode bt) { trans_mode_ = bt; }
    buffer_access access() const { return access_; }
    void access(buffer_access ba) { access_ = ba; }
    size_t size() const { return size_; }
    void size(size_t sz);
    void type(element_type::element_type_e t) { etype_ = t; }
    element_type::element_type_e type() const { return etype_; }

    id_type create_object() const;
    void destroy_object(id_type i) const;
};

class vertex_attr_buffer : public buffer {
    bool first_map = true;

public:
    void write_raw(const void* data, size_t sz = 0) override;
    void read_raw(void* data, size_t sz = 0) override;
    using buffer::buffer;
    using buffer::start_map;
    void* start_map(buffer_access bt, size_t sz = 0) override;
    void stop_map() override;
};

class property_buffer : public buffer {
    bool first_map = true;

public:
    void write_raw(const void* data, size_t sz = 0) override;
    void read_raw(void* data, size_t sz = 0) override;
    using buffer::buffer;
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
