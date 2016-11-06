#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <type_traits>
#include <sstream>

#include "matrix.h"
#include "traits.h"

namespace shrtool {

////////////////////////////////////////////////////////////////////////////////
// universal_property

/*
 * Universal Property is a tuple-like data structure that can store many types
 * of data (those acceptable for OpenGL driver). It is totally static, computes
 * size, transforms data, computes align in a totally static way, hence it has
 * high efficiency.
 */

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
    return static_cast<tail&>(up).data;
}

template<size_t I, typename ...Args>
const typename universal_property_item<
    I, universal_property<Args...>>::value_type &
item_get(const universal_property<Args...>& up)
{
    typedef typename universal_property_item<
        I, universal_property<Args...>>::prop_type tail;
    return static_cast<const tail&>(up).data;
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

template<typename UniProp>
std::string item_list_definition__() { return ""; }

template<typename UniProp, typename ...Args>
std::string item_list_definition__(
        std::string head, Args ...names)
{
    return std::string("    ") +
        universal_property_item<0, UniProp>::trait::type_name() +
        " " + head + ";\n" +
        item_list_definition__<typename UniProp::parent_type>(names...);
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

template<size_t I, typename UniProp, size_t StartAt = 0>
constexpr size_t item_offset() {
    return item_offset__<I, StartAt, UniProp>::value;
}

}

namespace shrtool {

template<typename ...Args>
struct prop_trait<universal_property<Args...>> {
    typedef universal_property<Args...> input_type;
    typedef shrtool::indirect_tag transfer_tag;
    typedef uint8_t value_type;

private:
    template<size_t OrgI>
    static void copy__(const void* up, uint8_t* o) { /* do nothing */ }

    template<size_t OrgI, typename UniProp>
    static void copy__(const UniProp* up, uint8_t* o) {
        typedef item_trait<typename UniProp::value_type> trait;
        uint8_t* off_o = o + item_offset<OrgI, input_type>();

        trait::copy(up->data,
            reinterpret_cast<typename trait::value_type*>(off_o));

        copy__<OrgI + 1>((const typename UniProp::parent_type*)up, o);
    }

public:
    static size_t size(const input_type& i) {
        return property_size(i);
    }

    static void copy(const input_type& i, uint8_t* o) {
        copy__<0, input_type>(&i, o);
    }
};

}

#endif // UTILS_H_INCLUDED
