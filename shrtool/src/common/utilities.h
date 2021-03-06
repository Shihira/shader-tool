#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#include <iostream>
#include <sstream>
#include <iomanip>

#include "matrix.h"
#include "reflection.h"
#include "traits.h"

namespace shrtool {

// useful macros

template<typename T, typename Enable = void>
struct prop_get_ret_type__ {
    typedef const T& ret_type;
};

template<typename T>
struct prop_get_ret_type__<T, typename std::enable_if<
        std::is_scalar<T>::value>::type> {
    typedef T ret_type;
};

#define PROPERTY_GENERIC_RW(type, name, befr, aftr, befw, aftw) \
    protected: type name##_; \
    public: typename prop_get_ret_type__<type>::ret_type get_##name() const { befr; return name##_; aftr; } \
    public: void set_##name(type const & v) { befw; name##_ = v; aftw; }

#define PROPERTY_GENERIC_RO(type, name, befr, aftr) \
    protected: type name##_; \
    public: typename prop_get_ret_type__<type>::ret_type get_##name() const { befr; return name##_; aftr; }

#define PROPERTY_GENERIC_WO(type, name, chan_tag, befw, aftw) \
    protected: type name##_; \
    public: void set_##name(type const & v) { befw; name##_ = v; aftw; }

#define PROPERTY_RW(type, name) \
    PROPERTY_GENERIC_RW(type, name, {}, {}, {}, {})
// DR/DW indicated reading/writing is being decorated
#define PROPERTY_DR_W(type, name, befr, aftr) \
    PROPERTY_GENERIC_RW(type, name, befw, aftw, {}, {})
#define PROPERTY_R_DW(type, name, befw, aftw) \
    PROPERTY_GENERIC_RW(type, name, {}, {}, befw, aftw)

#define PROPERTY_RO(type, name) \
    PROPERTY_GENERIC_RO(type, name, {}, {})
#define PROPERTY_WO(type, name) \
    PROPERTY_GENERIC_WO(type, name, {}, {})

#define PROP_ASSERT(cond, reason) { if(!(cond)) throw shrtool::restriction_error(reason); }
#define PROP_MARKCHG(change_tag) { change_tag = true; }
#define PROP_MARKCHG2(change_tag1, change_tag2) { change_tag1 = true; change_tag2 = true; }

#define DEF_ENUM_MAP(fn, from_type, to_type, map_content) \
    static to_type fn(from_type e) { \
        static const std::unordered_map<from_type, to_type> \
            trans_map_ map_content; \
        auto i_ = trans_map_.find(e); \
        if(i_ == trans_map_.end())  {\
            std::stringstream ss; \
            ss << "Failed when mapping " << #fn << ": " << e; \
            throw shrtool::enum_map_error(ss.str()); \
        } \
        return i_->second; \
    }


////////////////////////////////////////////////////////////////////////////////
// color

struct color;

std::ostream& operator<<(std::ostream& os, const color& c);

enum color_format : size_t {
    RGBA_U8888 = 128,
    R_F32,
    RG_F32,
    RGB_F32,
    RGBA_F32,
};

struct color {
    union {
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        } channels;
        uint32_t rgba;
        uint8_t bytes[4];
    } data;

    uint8_t& operator[](size_t i) {
        return data.bytes[i];
    }

    color() { data.channels.a = 0xff; }

    color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff) {
        data.channels.r = r; data.channels.g = g;
        data.channels.b = b; data.channels.a = a;
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

    static color from_value(size_t rgba) {
        return color((uint32_t)rgba);
    }

    static color from_rgba(int r, int g, int b, int a) {
        return color(clamp_uchar(r), clamp_uchar(g),
                clamp_uchar(b), clamp_uchar(a));
    }

    operator std::string() const {
        std::stringstream ss;
        ss << '#' << std::setw(8) << std::setfill('0')
            << std::hex << data.rgba;
        return ss.str();
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<color>("color")
            .enable_construct<size_t>()
            .enable_serialize()
            .enable_clone()
            .enable_equal()
            .enable_print()
            .enable_auto_register()
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

    static constexpr color_format format() {
        return RGBA_U8888;
    }

private:
    static uint8_t clamp_uchar(int v) {
        return v < 0 ? 0 : v > 255 ? 255 : v;
    }
};

struct fcolor {
    union {
        struct {
            float r;
            float g;
            float b;
            float a;
        } channels;
        float floats[4];
    } data;

    float& operator[](size_t i) {
        return data.floats[i];
    }

    fcolor() { data.channels.a = 1; }

    fcolor(const color& c) { (*this) = c; }

    fcolor(float r, float g, float b, float a = 1) {
        data.channels.r = r; data.channels.g = g;
        data.channels.b = b; data.channels.a = a;
    }

    fcolor& operator=(const fcolor& c) {
        for(size_t i = 0; i < 4; i++)
            data.floats[i] = c.data.floats[i];
        return *this;
    }

    fcolor& operator=(const color& c) {
        for(size_t i = 0; i < 4; i++)
            data.floats[i] = c.data.bytes[i] / 255.f;
        return *this;
    }

    operator color() const {
        return color::from_rgba(
            r() * 255, g() * 255, b() * 255, a() * 255);
    }

    bool operator==(const fcolor& c) const {
        bool res = true;
        for(size_t i = 0; res && i < 4; i++)
            res &= data.floats[i] == c.data.floats[i];
        return res;
    }

    float r() const { return data.floats[0]; }
    float g() const { return data.floats[1]; }
    float b() const { return data.floats[2]; }
    float a() const { return data.floats[3]; }

    typedef float* iterator;
    typedef float const* const_iterator;

    iterator begin() { return data.floats; }
    iterator end() { return data.floats + 4; }
    const_iterator begin() const { return data.floats; }
    const_iterator end() const { return data.floats + 4; }
    const_iterator cbegin() { return data.floats; }
    const_iterator cend() { return data.floats + 4; }

    static void meta_reg_() {
        refl::meta_manager::reg_class<fcolor>("fcolor")
            .enable_construct<float, float, float, float>()
            .enable_clone()
            .enable_equal()
            .enable_auto_register()
            .function("r", &color::r)
            .function("g", &color::g)
            .function("b", &color::b)
            .function("a", &color::a)
            .function("rgba", &color::rgba);

        refl::meta_manager::enable_cast<color, fcolor>();
    }

    static constexpr color_format format() {
        return RGBA_F32;
    }
};


template<>
struct item_trait<color> {
    typedef float value_type;
    static constexpr size_t size() {
        return sizeof(float) * 4;
    }
    static constexpr size_t align() {
        return sizeof(float) * 4;
    }

    static void copy(const color& v, value_type* buf) {
        fcolor c = v;
        buf[0] = c.r();
        buf[1] = c.g();
        buf[2] = c.b();
        buf[3] = c.a();
    }

    static const char* glsl_type_name() {
        return "vec4";
    }
};

inline std::ostream& operator<<(std::ostream& os, const color& c)
{
    return os << std::string(c);
}

////////////////////////////////////////////////////////////////////////////////
// rect

struct rect {
    math::col2 tl;
    math::col2 br;

    rect(rect&& r) :
        tl(std::move(r.tl)), br(std::move(r.br)) { }
    rect(const rect& r) : tl(r.tl), br(r.br) { }

    rect() : rect(0, 0, 0, 0) { }
    rect(math::col2 a, math::col2 b) :
        tl(std::move(a)), br(std::move(b)) { regulate(); }
    rect(double x1, double y1, double x2, double y2) :
        tl{x1, y1}, br{x2, y2} { regulate(); }

    double area() const {
        return (br[0] - tl[0]) * (br[1] - tl[1]);
    }

    double width() const { return br[0] - tl[0]; }
    double height() const { return br[1] - tl[1]; }

    // when tl == [0, 0], rect represents a size
    rect get_size() const { return rect(0, 0, br[0], br[1]); }
    static rect from_size(const math::col2& br) {
        return rect(0, 0, br[0], br[1]);
    }
    static rect from_size(double w, double h) {
        return rect(0, 0, w, h);
    }

    rect operator-(const rect& r) const {
        return rect(tl - r.tl, br - r.br);
    }

    rect operator+(const rect& r) const {
        return rect(tl + r.tl, br + r.br);
    }

    rect& operator=(const rect& r) {
        tl = r.tl;
        br = r.br;
        return *this;
    }

    rect& operator=(rect&& r) {
        tl = std::move(r.tl);
        br = std::move(r.br);
        return *this;
    }

    void regulate() {
        if(tl[0] > br[0])
            std::swap(tl[0], br[0]);
        if(tl[1] > br[1])
            std::swap(tl[1], br[1]);
    }

    double ratio() const { return width() / height(); }

    static void meta_reg_() {
        refl::meta_manager::reg_class<rect>("rect")
            .enable_construct<math::dxmat, math::dxmat>()
            .enable_construct<double, double, double, double>()
            .enable_print()
            .enable_auto_register()
            .function("area", &rect::area)
            .function("width", &rect::width)
            .function("height", &rect::height)
            .function("get_size", &rect::get_size)
            .function("from_size", static_cast<rect(*)(double, double)>(&rect::from_size))
            .function("regulate", &rect::regulate);
    }
};

inline std::ostream& operator<<(std::ostream& os, const rect& c)
{ 
    os << '(' << c.tl[0] << ',' << c.tl[1] << ')' << " -> " <<
        '(' << c.br[0] << ',' << c.br[1] << ')';
    return os;
}

////////////////////////////////////////////////////////////////////////////////
// transfrm

struct base_transfrm {
    virtual const math::mat4& get_mat() const = 0;
    virtual const math::mat4& get_inverse_mat() const = 0;

    bool is_changed() const { return changed_; }
    void mark_applied() { changed_ = false; }

protected:
    bool changed_ = false;
};

struct transfrm : base_transfrm {
    transfrm() :
        mat_(math::tf::identity()),
        inv_mat_(math::tf::identity()) { }

    transfrm(const transfrm& t) :
        mat_(t.mat_),
        inv_mat_(t.inv_mat_) { }

    transfrm(transfrm&& t) :
        mat_(std::move(t.mat_)),
        inv_mat_(std::move(t.inv_mat_)) { }

    transfrm& operator=(const transfrm& t) {
        mat_ = t.mat_;
        inv_mat_ = t.inv_mat_;
        changed_ = true;
        return *this;
    }

    transfrm& translate(double x, double y, double z) {
        translate(math::col4 { x, y, z, 1 });
        return *this;
    }

    transfrm& translate(math::col4 pos) {
        mat_ = math::tf::translate(pos) * mat_;
        pos[0] = -pos[0];
        pos[1] = -pos[1];
        pos[2] = -pos[2];
        inv_mat_ = inv_mat_ * math::tf::translate(pos);
        changed_ = true;
        return *this;
    }

    transfrm& translate(const math::col3& pos) {
        mat_ = math::tf::translate(pos) * mat_;
        inv_mat_ = inv_mat_ * math::tf::translate(-pos);
        changed_ = true;
        return *this;
    }

    transfrm& rotate(double a, math::tf::plane p) {
        mat_ = math::tf::rotate(a, p) * mat_;
        inv_mat_ = inv_mat_ * math::tf::rotate(-a, p);
        changed_ = true;
        return *this;
    }

    transfrm& rotate(const math::col3& rot) {
        rotate(rot[0], math::tf::yOz);
        rotate(rot[2], math::tf::xOy);
        rotate(rot[1], math::tf::zOx);
        return *this;
    }

    transfrm& rotate_axis(double angle, math::col3 vec) {
        vec = vec / math::norm(vec);

        math::mat4 w = {
            0, -vec[2], vec[1], 0,
            vec[2], 0, -vec[0], 0,
            -vec[1], vec[0], 0, 0,
            0, 0, 0, 0,
        };
        math::mat4 w2 = w * w;

        double c = cos(angle), s = sin(angle);

        math::mat4 m = math::tf::identity() + w2 * (1 - c) + w * s;
        m.at(3, 3) = 1;

        mat_ = m * mat_;
        inv_mat_ *= math::inverse(m);
        changed_ = true;

        return *this;
    }

    transfrm& scale(double x, double y, double z) {
        mat_ = math::tf::scale(x, y, z) * mat_;
        inv_mat_ = inv_mat_ * math::tf::scale(1/x, 1/y, 1/z);
        changed_ = true;
        return *this;
    }

    transfrm& scale(double s) {
        return scale(s, s, s);
    }

    math::col3 get_rotation_euler() const {
        math::col3 euler;

        if (mat_.at(1, 0) > 0.998) { // singularity at north pole
            euler[0] = 0;
            euler[1] = -atan2(mat_.at(0, 2),mat_.at(2, 2));
            euler[2] = M_PI/2;
        }
        else if (mat_.at(1, 0) < -0.998) { // singularity at south pole
            euler[0] = 0;
            euler[1] = -atan2(mat_.at(0, 2),mat_.at(2, 2));
            euler[2] = -M_PI/2;
        }
        else {
            euler[0] = -atan2(-mat_.at(1, 2),mat_.at(1, 1));
            euler[1] = -atan2(-mat_.at(2, 0),mat_.at(0, 0));
            euler[2] = asin(mat_.at(1, 0));
        }

        /*
        // this piece of code is used to test the applying order
        int orders[6][3] = {
            {0, 1, 2}, {0, 2, 1}, {1, 0, 2},
            {1, 2, 0}, {2, 0, 1}, {2, 1, 0},
        };

        for(int i = 0; i < 6; i++) {
            transfrm tf;

            for(int j = 0; j < 3; j++) {
                int o = orders[i][j];
                tf.rotate(euler[o], (shrtool::math::tf::plane)((o + 1) % 3));
            }

            double err = 0;
            for(int r = 0; r < 3; r++)
            for(int c = 0; c < 3; c++)
                err += tf.get_mat().at(r, c) - get_mat().at(r, c);

            std::cout << i << ": " << err << std::endl;
        }
        */

        return euler;
    }

    math::col4 get_translation() const {
        math::col4 c = mat_.col(3);
        c /= c[3];
        return c;
    }

    void set_mat(const math::mat4& m) {
        mat_ = m;
        inv_mat_ = math::inverse(mat_);
        changed_ = true;
    }

    bool operator==(const transfrm& a) const {
        return a.mat_ == mat_;
    }

    transfrm& operator*=(const base_transfrm& tf) {
        mat_ = tf.get_mat() * mat_;
        inv_mat_ *= tf.get_inverse_mat();
        changed_ = true;
        return *this;
    }

    transfrm& operator*=(const math::mat4& m) {
        mat_ = m * mat_;
        inv_mat_ *= math::inverse(m);
        changed_ = true;
        return *this;
    }

    const math::mat4& get_mat() const { return mat_; }
    const math::mat4& get_inverse_mat() const { return inv_mat_; }

    static void meta_reg_() {
        refl::meta_manager::reg_class<transfrm>("transfrm")
            .enable_construct<>()
            .enable_clone()
            .enable_equal()
            .enable_auto_register()
            .function("rotate", static_cast<transfrm&(transfrm::*)(double, math::tf::plane)>(&transfrm::rotate))
            .function("scale", static_cast<transfrm&(transfrm::*)(double, double, double)>(&transfrm::scale))
            .function("translate", static_cast<transfrm&(transfrm::*)(double, double, double)>(&transfrm::translate))
            .function("get_mat", &transfrm::get_mat)
            .function("get_inverse_mat", &transfrm::get_inverse_mat);
    }

protected:
    math::mat4 mat_;
    math::mat4 inv_mat_;
};

struct trs_transfrm : base_transfrm {
    PROPERTY_R_DW(math::quat, rotation, {}, PROP_MARKCHG2(changed_, mat_update_))
    PROPERTY_R_DW(math::col3, position, {}, PROP_MARKCHG2(changed_, mat_update_))
    PROPERTY_R_DW(math::col3, scaling, {}, PROP_MARKCHG2(changed_, mat_update_))

    const math::mat4& get_mat() const {
        update_mat();
        return mat_;
    }

    const math::mat4& get_inverse_mat() const {
        update_mat();
        return inv_mat_;
    }

    void set_mat(const math::mat4& m) {
        position_ = math::col3(m.col(3) / m.at(3, 3));
        scaling_[0] = math::norm(math::col4(m.col(0)));
        scaling_[1] = math::norm(math::col4(m.col(1)));
        scaling_[2] = math::norm(math::col4(m.col(2)));

        math::mat4 a = m;
        a.col(0) /= scaling_[0];
        a.col(1) /= scaling_[1];
        a.col(2) /= scaling_[2];
        a.col(3) = math::col4 { 0, 0, 0, 1 };

        rotation_[3] = std::sqrt(1 + a.at(0, 0) + a.at(1, 1) + a.at(2, 2)) / 2;
        double w4 = 4 * rotation_[3];
        rotation_[0] = (a.at(2, 1) - a.at(1, 2)) / w4;
        rotation_[1] = (a.at(0, 2) - a.at(2, 0)) / w4;
        rotation_[2] = (a.at(1, 0) - a.at(0, 1)) / w4;

        mat_update_ = true;
    }

private:
    mutable math::mat4 mat_;
    mutable math::mat4 inv_mat_;

protected:
    mutable bool mat_update_ = true;

    void update_mat() const {
        using namespace math;

        if(!mat_update_) return;

        mat_ = tf::identity() *
            tf::translate(position_) *
            tf::rotate(rotation_) *
            tf::scale(scaling_);

        inv_mat_ = inverse(mat_);

        mat_update_ = false;
    }
};

template<>
struct prop_trait<transfrm> {
    typedef transfrm input_type;
    typedef float value_type;
    typedef shrtool::indirect_tag transfer_tag;

    static size_t size(const input_type& i) {
        return sizeof(float) * 32;
    }

    static bool is_changed(const input_type& i) {
        return i.is_changed();
    }

    static void mark_applied(input_type& i) {
        i.mark_applied();
    }

    static void copy(const input_type& i, value_type* o) {
        value_type* o_1 = o + 16;
        for(size_t c = 0; c < 4; c++)
            for(size_t r = 0; r < 4; r++) {
                *(o++) = i.get_mat().at(r, c);
                *(o_1++) = i.get_inverse_mat().at(r, c);
            }
    }
};

}

#endif // UTILITIES_H_INCLUDED
