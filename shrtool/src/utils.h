#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <type_traits>
#include <sstream>

#include "matrix.h"

namespace shrtool {

////////////////////////////////////////////////////////////////////////////////
// universal_property

template<typename T>
struct plain_item_trait
{
    typedef T value_type;
    static constexpr size_t size = sizeof(value_type);
    static constexpr size_t align = sizeof(value_type);

    static void copy(const T& v, value_type* buf) { *buf = v; }
    static const char* type_name() { return "unknown"; }
};

template<typename T>
struct item_trait : plain_item_trait<T> { };

template<>
struct item_trait<char> : plain_item_trait<char>
{
    static const char* type_name() { return "byte"; }
};

template<>
struct item_trait<int> : plain_item_trait<int>
{
    static const char* type_name() { return "int"; }
};

template<>
struct item_trait<double> : plain_item_trait<double>
{
    static const char* type_name() { return "double"; }
};

template<>
struct item_trait<float> : plain_item_trait<float>
{
    static const char* type_name() { return "float"; }
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
            auto& c = m.col(n);
            std::copy(c.begin(), c.end(), buf);
        }
    }

    static const char* type_name() {
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

    static const char* type_name() {
        static const char name_[] = {
            std::is_same<value_type, uint8_t>::value ? 'b' :
            std::is_same<value_type, int>::value ? 'i' :
            std::is_same<value_type, double>::value ? 'd' : '\0',
            'v', 'e', 'c', M + '0', '\0' };
        static const char* name_p = name_[0] ? name_ : name_ + 1;

        return name_p;
    }
};

template<typename T, typename ...Args>
struct universal_property : universal_property<Args...>
{
    typedef universal_property<Args...> parent_type;

    universal_property() { }

    universal_property(const T& t, Args ...args)
        : parent_type(args...), data(t) { }

    universal_property(const universal_property& other)
        : parent_type(other), data(other.data) { }

    universal_property(universal_property&& other)
        : parent_type(std::move(other)), data(std::move(other.data)) { }

    typedef T value_type;
    value_type data;
    static constexpr size_t count = parent_type::count + 1;
};

template<typename T>
struct universal_property<T>
{
public:
    typedef void parent_type;

    universal_property() { }

    universal_property(const T& t) : data(t) { }

    universal_property(const universal_property& other)
        : data(other.data) { }

    universal_property(universal_property&& other)
        : data(std::move(other.data)) { }

    typedef T value_type;
    value_type data;
    static constexpr size_t count = 1;
};

template<size_t I, typename UniProp>
struct universal_property_item
{
    typedef typename universal_property_item<
            I - 1, typename UniProp::parent_type>::prop_type prop_type;
    typedef typename prop_type::value_type value_type;
    typedef item_trait<value_type> trait;
};

template<typename UniProp>
struct universal_property_item<0, UniProp>
{
    typedef UniProp prop_type;
    typedef typename prop_type::value_type value_type;
    typedef item_trait<value_type> trait;
};

template<size_t I, typename ...Args>
typename universal_property_item<
    I, universal_property<Args...>>::value_type &
item_get(universal_property<Args...>& up)
{
    typedef typename universal_property_item<
        I, universal_property<Args...>>::prop_type tail;
    return static_cast<tail*>(&up)->data;
}

template<size_t I, typename ...Args>
const typename universal_property_item<
    I, universal_property<Args...>>::value_type &
item_get(const universal_property<Args...>& up)
{
    typedef typename universal_property_item<
        I, universal_property<Args...>>::prop_type tail;
    return static_cast<const tail*>(&up)->data;
}

template<typename ...Args>
universal_property<Args...> make_porperty(Args ...args)
{
    return universal_property<Args...>(std::forward<Args...>(args...));
}

template<size_t I, size_t StartAt, typename UniProp>
struct item_offset__ {
private:
    typedef typename UniProp::parent_type up_p;
    typedef typename universal_property_item<0, UniProp>::trait trait;

    static constexpr size_t base_offset = 
        (StartAt % trait::align ?
            trait::align * (StartAt / trait::align + 1) :
            StartAt);

public:
    static constexpr size_t value = 
        item_offset__<I - 1, trait::size + base_offset, up_p>::value;
};

// trivial.
template<size_t StartAt, typename UniProp>
struct item_offset__<0, StartAt, UniProp> {
private:
    typedef typename universal_property_item<0, UniProp>::trait trait;

public:
    static constexpr size_t value = 
        (StartAt % trait::align ?
            trait::align * (StartAt / trait::align + 1) :
            StartAt);
};

template<typename UniProp>
constexpr size_t property_size(const UniProp&)
{
    return item_offset__<UniProp::count - 1, 0, UniProp>::value +
        universal_property_item<UniProp::count - 1, UniProp>::trait::size;
}

std::string item_list_definition__() { return ""; }

template<typename UniProp, typename ...Args>
std::string item_list_definition__(
        std::string head, Args ...names)
{
    return std::string("    ") + universal_property_item<
            UniProp::count - 1, UniProp>::trait::type_name() +
        " " + head + ";\n" + item_list_definition__(names...);
}

template<typename UniProp, typename ...Args>
std::string property_glsl_definition(
        const UniProp& up,
        std::string block_name,
        Args ...names)
{
    return "uniform " + block_name + " {\n" +
        item_list_definition__<UniProp>(names...) + "};";
}

template<size_t I, typename UniProp, size_t StartAt = 0>
constexpr size_t item_offset(const UniProp&) {
    return item_offset__<I, StartAt, UniProp>::value;
}

}

#endif // UTILS_H_INCLUDED
