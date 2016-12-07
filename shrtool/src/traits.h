#ifndef RES_TRAIT_INCLUDED
#define RES_TRAIT_INCLUDED

#include "matrix.h"

namespace shrtool {

template<typename T>
struct plain_item_trait
{
    typedef T value_type;
    static constexpr size_t size = sizeof(value_type);
    static constexpr size_t align = sizeof(value_type);

    static void copy(const T& v, value_type* buf) { *buf = v; }
    static const char* glsl_type_name() { return "unknown"; }
};

template<typename T>
struct item_trait : plain_item_trait<T> { };

template<>
struct item_trait<char> : plain_item_trait<char>
{
    static const char* glsl_type_name() { return "byte"; }
};

template<>
struct item_trait<int> : plain_item_trait<int>
{
    static const char* glsl_type_name() { return "int"; }
};

template<>
struct item_trait<double> : plain_item_trait<double>
{
    static const char* glsl_type_name() { return "double"; }
};

template<>
struct item_trait<float> : plain_item_trait<float>
{
    static const char* glsl_type_name() { return "float"; }
};

// col-major matrix
template<typename T, size_t M, size_t N>
struct item_trait<math::matrix<T, M, N>>
{
    typedef float value_type;
    static constexpr size_t size = M * N * sizeof(value_type);
    static constexpr size_t align = item_trait<math::col<value_type, M>>::align;

    static void copy(const math::matrix<T, M, N>& m, value_type* buf) {
        for(size_t n = 0; n < N; ++n, buf += M) {
            const auto& c = m.col(n);
            std::copy(c.begin(), c.end(), buf);
        }
    }

    static const char* glsl_type_name() {
        static const char name_[] = {
            'm', 'a', 't', M + '0',
            M == N ? '\0' : 'x', N + '0', '\0' };
        return name_;
    }
};

template<typename T, size_t M>
struct item_trait<math::matrix<T, M, 1>>
{
    typedef T value_type;
    static constexpr size_t size = M * sizeof(value_type);
    static constexpr size_t align = (M < 3 ? M : 4) * sizeof(value_type);

    static void copy(const math::col<T, M>& c, value_type* buf) {
        std::copy(c.begin(), c.end(), buf);
    }

    static const char* glsl_type_name() {
        static const char name_[] = {
            std::is_same<value_type, uint8_t>::value ? 'b' :
            std::is_same<value_type, int>::value ? 'i' :
            std::is_same<value_type, double>::value ? 'd' : '\0',
            'v', 'e', 'c', M + '0', '\0' };
        static const char* name_p = name_[0] ? name_ : name_ + 1;

        return name_p;
    }
};

struct raw_data_tag { };
struct indirect_tag { };

template<typename InputType, typename Enable = void>
struct attr_trait {
    /*
     * Please provide:
     */

    // typedef InputType input_type;
    // typedef shrtool::indirect_tag transfer_tag;
    // typedef float elem_type;
    //
    // static int slot(const input_type& i, size_t i_s);
    // static int count(const input_type& i);
    // static int dim(const input_type& i, size_t i_s);

    /*
     * For raw_data
     */
    // static elem_type const* data(const input_type& i, size_t i_s);
    /*
     * For indirect
     */
    // static void copy(const input_type& i, size_t i_s, elem_type* data);
};

template<typename InputType, typename Enable = void>
struct prop_trait {
    /*
     * Please provide:
     */

    //typedef InputType input_type;
    //typedef shrtool::indirect_tag transfer_tag;
    /* value_type determines the type of copy arguments o */
    //typedef float value_type;

    // static size_t size(const input_type& i);
    /*
     * For raw_data
     */
    // static elem_type const* data(const input_type& i);
    /*
     * For indirect
     */
    // static void copy(const input_type& i, value_type* o);
};

template<typename InputType, typename Enable = void>
struct texture2d_trait {
    //typedef shrtool::raw_data_tag transfer_tag;
    //typedef InputType input_type;

    //static size_t width(const input_type& i) {
    //    return i.width();
    //}

    //static size_t height(const input_type& i) {
    //    return i.height();
    //}

    //static shrtool::render_assets::texture2d::format
    //format(const input_type& i) {
    //    return shrtool::render_assets::texture2d::RGBA_U8888;
    //}

    //static const void* data(const input_type& i) {
    //    return i.data();
    //}
};

template<typename InputType, typename Enable = void>
struct shader_trait {
    //typedef InputType input_type;

    //static std::string source(const input_type& i, size_t e,
    //      shader::shader_type& t);
};

}

#endif // RES_TRAIT_INCLUDED

