#ifndef RENDER_ASSETS_H_INCLUDED
#define RENDER_ASSETS_H_INCLUDED

#include <utility>
#include <string>
#include <vector>
#include <type_traits>
#include <memory>
#include <unordered_map>

#include "reflection.h"
#include "exception.h"

/*
 * NOTE: A render asset must not contain the reference of another.
 */

namespace shrtool {

typedef uint32_t id_type;

template<typename Derived>
class lazy_id_object_ {
public:
    virtual bool vacuum() const { return id_ == 0; }

    void destroy() { 
        if(id_) reinterpret_cast<const Derived*>(this)->destroy_object(id_);
    }

    id_type id() const {
        if(!id_) id_ = reinterpret_cast<const Derived*>(this)->create_object();
        return id_;
    }

    ~lazy_id_object_() { destroy(); };

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

#define LAZEOBJ_SCALAR_PROP(type, name, def) \
    protected: type name##_ = def; \
    public: type name() const { return name##_; } \
    public: \
    void name(type t) { \
        if(!vacuum() && name##_ != t) \
            throw restriction_error(#name " can no longer be changed"); \
        name##_ = t; \
    }

////////////////////////////////////////////////////////////////////////////////
// render assets

namespace render_assets {

struct texture_policy_i;

class texture : public lazy_id_object_<texture> {
public:
    enum filter_type {
        LINEAR,
        NEAREST
    };

    enum format {
        // DEFAULT_FMT means that an unspecified format field will reference to
        // another specified field. If none is specified, it is equivalent to
        // RGBA_U8888.
        DEFAULT_FMT,
        RGBA_U8888,
        R_F32,
        DEPTH_F32,
    };

    enum traits {
        NONE = 0,
        LAYERED = 1,
        CUBEMAP = 2,
    };

    /*
     * texture base is not allow to be copied, for this may drop virtual
     * functions
     */
    id_type create_object() const;
    void destroy_object(id_type i) const;

    LAZEOBJ_SCALAR_PROP(size_t, level, 0)
    LAZEOBJ_SCALAR_PROP(size_t, width, 0)
    LAZEOBJ_SCALAR_PROP(size_t, height, 0)
    LAZEOBJ_SCALAR_PROP(size_t, depth, 0)
    LAZEOBJ_SCALAR_PROP(format, internal_format, DEFAULT_FMT)
    LAZEOBJ_SCALAR_PROP(filter_type, filter, LINEAR)
    LAZEOBJ_SCALAR_PROP(traits, trait, NONE)

public:
    texture(texture&& tex) = delete;
    texture(const texture& tex) = delete;

    texture(size_t width = 0, size_t height = 0, format ifmt = DEFAULT_FMT)
        : texture(width, height, 1, ifmt) { }
    texture(size_t width, size_t height, size_t depth, format ifmt = DEFAULT_FMT)
        : width_(width), height_(height), depth_(depth), internal_format_(ifmt) { }

    virtual void fill(const void* data, format fmt = DEFAULT_FMT);
    void reserve(format fmt = DEFAULT_FMT) { fill(nullptr, fmt); }

    virtual void fill_rect(
            size_t offx, size_t offy, size_t offz,
            size_t w, size_t h, size_t d,
            const void* data, format fmt = DEFAULT_FMT);
    void fill_rect(
            size_t w, size_t h, size_t d,
            const void* data, format fmt = DEFAULT_FMT) {
        fill_rect(0, 0, 0, w, h, d, data, fmt);
    }
    void fill_rect(
            size_t offx, size_t offy, size_t w, size_t h,
            const void* data, format fmt = DEFAULT_FMT) {
        fill_rect(offx, offy, 0, w, h, 1, data, fmt);
    }
    void fill_rect(size_t w, size_t h,
            const void* data, format fmt = DEFAULT_FMT) {
        fill_rect(0, 0, w, h, data, fmt);
    }

    virtual void read(void* data, format fmt = DEFAULT_FMT);

    virtual void bind_to(size_t tex_bind) const;
    virtual void attach_to(size_t tex_attachment);
};

class texture2d : public texture {
public:
    texture2d(texture2d&& tex) :
            texture2d(tex.width(), tex.height(), tex.internal_format()) {
        lazy_id_object_<texture>::operator=(std::move(tex));
    }

    texture2d(size_t width = 0, size_t height = 0, format ifmt = DEFAULT_FMT)
        : texture(width, height, ifmt) { }

    static void meta_reg_() {
        refl::meta_manager::reg_class<texture2d>("texture2d")
            .enable_construct<size_t, size_t>()
            .function("reserve", &texture2d::reserve)
            .function("width", static_cast<size_t(texture2d::*)()const>(&texture2d::width))
            .function("height", static_cast<size_t(texture2d::*)()const>(&texture2d::height));
    }
};

class texture_cubemap : public texture {
public:
    texture_cubemap(texture_cubemap&& tex) :
            texture_cubemap(tex.width(), tex.internal_format()) {
        lazy_id_object_<texture>::operator=(std::move(tex));
    }

    enum face_index : size_t {
        POS_X = 0, NEG_X = 1,
        POS_Y = 2, NEG_Y = 3,
        POS_Z = 4, NEG_Z = 5,
    };

    texture_cubemap(size_t edge_len = 0, format ifmt = DEFAULT_FMT) :
            texture(edge_len, edge_len, 6, ifmt) {
        trait(CUBEMAP);
    }

    virtual void fill(const void* data, format fmt = DEFAULT_FMT) override;

    virtual void fill_rect(
            size_t offx, size_t offy, size_t offz,
            size_t w, size_t h, size_t d,
            const void* data, format fmt = DEFAULT_FMT) override;

    virtual void read(void* data, format fmt = DEFAULT_FMT) override;
};

////////////////////////////////////////////////////////////////////////////////

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

    bool vacuum() const override { return size_ == 0; }

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
    template<typename T>
    void write(const T* data, size_t sz = 0) {
        // in property buffer we do not care about its actual type
        // so just uniformly set it to BYTE: don't be surprised :)
        etype_ = element_type::BYTE;
        write_raw(data, sz * sizeof(T));
    }

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
