#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include <iostream>

#include "traits.h"
#include "reflection.h"
#include "render_assets.h"

namespace shrtool {

struct color {
    union {
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        } comp;
        uint32_t rgba;
        uint8_t bytes[4];
    } data;

    uint8_t& operator[](size_t i) {
        return data.bytes[i];
    }

    color() { data.comp.a = 0xff; }

    color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff) {
        data.comp.r = r; data.comp.g = g;
        data.comp.b = b; data.comp.a = a;
    }

    color(const std::string& s) {
        size_t c;
        std::stoi(s.c_str() + 1, &c, 16);
        data.rgba = c;
    }

    explicit color(uint32_t rgba) {
        data.rgba = rgba;
    }

    color& operator=(uint32_t rgba) {
        data.rgba = rgba;
        return *this;
    }

    color& operator=(const color& c) {
        return operator=(c.data.rgba);
    }

    bool operator==(const color& c) const {
        return data.rgba == c.data.rgba;
    }

    operator std::string() const;

    size_t rgba() const { return data.rgba; }

    int r() const { return data.bytes[0]; }
    int g() const { return data.bytes[1]; }
    int b() const { return data.bytes[2]; }
    int a() const { return data.bytes[3]; }

    typedef uint8_t* iterator;
    typedef uint8_t const* const_iterator;

    iterator begin() { return data.bytes; }
    iterator end() { return data.bytes + 4; }
    const_iterator begin() const { return data.bytes; }
    const_iterator end() const { return data.bytes + 4; }
    const_iterator cbegin() { return data.bytes; }
    const_iterator cend() { return data.bytes + 4; }

    static color from_string(const std::string s) {
        return color(std::strtoul(s.c_str() + 1, nullptr, 16));
    }

    static color from_value(uint32_t rgba) {
        return color(rgba);
    }

    static color from_rgba(int r, int g, int b, int a) {
        return color(clamp_uchar(r), clamp_uchar(g),
                clamp_uchar(b), clamp_uchar(a));
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<color>("color")
            .enable_clone()
            .enable_equal()
            .enable_print()
            .function("from_string", from_string)
            .function("from_value", from_value)
            .function("from_rgba", from_rgba)
            .function("r", &color::r)
            .function("g", &color::g)
            .function("b", &color::b)
            .function("a", &color::a)
            .function("rgba", &color::rgba);

        refl::meta_manager::enable_cast<size_t, color>();
    }

private:
    static uint8_t clamp_uchar(int v) {
        return v < 0 ? 0 : v > 255 ? 255 : v;
    }
};

static_assert(sizeof(color) == 4, "Bad: sizeof(color) != 4");

std::ostream& operator<<(std::ostream& os, const color& c);

class image {
    friend struct image_geometry_helper__;

    size_t width_ = 0;
    size_t height_ = 0;

    mutable color* data_ = nullptr;
    // mark that if data_ should be deleted on destruction
    bool data_internal_ = false;

    color* lazy_data_() const;

public:
    typedef color* iterator;
    typedef color const* const_iterator;

    // create an new image
    image(size_t w = 0, size_t h = 0, color* data_ = nullptr) :
        width_(w), height_(h), data_(data_), data_internal_(data_) { }
    image(const image& rhs) : width_(rhs.width_), height_(rhs.height_) {
        std::copy(rhs.begin(), rhs.end(), begin());
    }
    image(image&& rhs) : width_(rhs.width_), height_(rhs.height_) {
        // we cannot call data() here. just keep the current state.
        std::swap(data_, rhs.data_);
    }

    image& operator=(const image& rhs) {
        width_ = rhs.width_;
        height_ = rhs.height_;
        std::copy(rhs.begin(), rhs.end(), begin());
        return *this;
    }

    image& operator=(image&& rhs) {
        width_ = rhs.width_;
        height_ = rhs.height_;
        std::swap(data_, rhs.data_);
        return *this;
    }

    void flip_h();
    void flip_v();

    size_t width() const { return width_; }
    size_t height() const { return height_; }

    iterator begin() { return data(); }
    iterator end() { return data() + width_ * height_; }
    const_iterator begin() const { return data(); }
    const_iterator end() const { return data() + width_ * height_; }
    const_iterator cbegin() const { return data(); }
    const_iterator cend() const { return data() + width_ * height_; }

    color& pixel(size_t l, size_t t) {
        return data()[t * width_ + l];
    }

    const color& pixel(size_t l, size_t t) const {
        return data()[t * width_ + l];
    }

    color* data() { return lazy_data_(); }
    color const* data() const { return lazy_data_(); }

    ~image() {
        if(data_ && data_internal_) delete[] data_;
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<image>("image")
            .enable_clone()
            .function("flip_h", &image::flip_h)
            .function("flip_v", &image::flip_v)
            .function("width", &image::width)
            .function("height", &image::height)
            .function("pixel", static_cast<color&(image::*)(size_t, size_t)>(&image::pixel));
    }
};

struct image_geometry_helper__ {
    static size_t& height(image& img) { return img.height_; }
    static size_t& width(image& img) { return img.width_; }
};

struct image_io_netpbm {
    image* img;

    image_io_netpbm(image& im) : img(&im) { }

    static image load(std::istream& is) {
        image im;
        load_into_image(is, im);
        return std::move(im);
    }

    std::istream& operator()(std::istream& is) {
        load_into_image(is, *img);
        return is;
    }

    std::ostream& operator()(std::ostream& os) {
        save_image(os, *img);
        return os;
    }

    static void load_into_image(std::istream& is, image& im);
    static void save_image(std::ostream& os, const image& im);
};

template<>
struct texture2d_trait<image> {
    typedef shrtool::raw_data_tag transfer_tag;
    typedef image input_type;

    static size_t width(const input_type& i) {
        return i.width();
    }

    static size_t height(const input_type& i) {
        return i.height();
    }

    static shrtool::render_assets::texture2d::format
    format(const input_type& i) {
        return shrtool::render_assets::texture2d::RGBA_U8888;
    }

    static const void* data(const input_type& i) {
        return i.data();
    }
};

}

#endif // IMAGE_H_INCLUDED
