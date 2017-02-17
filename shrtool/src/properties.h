#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <type_traits>
#include <algorithm>
#include <sstream>
#include <vector>
#include <initializer_list>

#include "matrix.h"
#include "traits.h"
#include "exception.h"

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

    universal_property& operator!() {
        parent_type::changed_ = true;
        return *this;
    }
};

template<typename T>
struct universal_property<T>
{
protected:
    bool changed_;

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

    universal_property operator!() {
        changed_ = true;
        return *this;
    }

    void mark_applied() {
        changed_ = false;
    }

    bool is_changed() const {
        return changed_;
    }
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
        (StartAt % trait::align() ?
            trait::align() * (StartAt / trait::align() + 1) :
            StartAt);

public:
    static constexpr size_t value = 
        item_offset__<I - 1, trait::size() + base_offset, up_p>::value;
};

// trivial.
template<size_t StartAt, typename UniProp>
struct item_offset__<0, StartAt, UniProp> {
private:
    typedef typename universal_property_item<0, UniProp>::trait trait;

public:
    static constexpr size_t value = 
        (StartAt % trait::align() ?
            trait::align() * (StartAt / trait::align() + 1) :
            StartAt);
};

template<typename UniProp>
constexpr size_t property_size(const UniProp&)
{
    return item_offset__<UniProp::count - 1, 0, UniProp>::value +
        universal_property_item<UniProp::count - 1, UniProp>::trait::size();
}

template<typename UniProp>
std::string item_list_definition__() { return ""; }

template<typename UniProp, typename ...Args>
std::string item_list_definition__(
        std::string head, Args ...names)
{
    return std::string("    ") +
        universal_property_item<0, UniProp>::trait::glsl_type_name() +
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

    static bool is_changed(const input_type& i) {
        return i.is_changed();
    }

    static void mark_applied(input_type& i) {
        i.mark_applied();
    }
};

}

////////////////////////////////////////////////////////////////////////////////
#include "reflection.h"

namespace shrtool {

struct dynamic_property {
    dynamic_property() { }
    dynamic_property(size_t n) {
        resize(n);
    }
    dynamic_property(dynamic_property&& dp) :
            storage(std::move(dp.storage)),
            offsets_(std::move(dp.offsets_)) {
        std::swap(is_changed_, dp.is_changed_);
        std::swap(offset_changed_, dp.offset_changed_);
    }

    std::vector<refl::instance>& underlying() {
        return storage;
    }

    void resize(size_t n) {
        storage.resize(n);

        offset_changed_ = true;
        is_changed_ = true;
    }

    template<typename T>
    void append(T&& obj) {
        storage.push_back(refl::instance::make(std::move(obj)));
        if(!storage.back().get_meta().has_function("__raw_into"))
            throw restriction_error("Not serializable.");

        offset_changed_ = true;
        is_changed_ = true;
    }

    void append_instance(refl::instance&& ins) {
        storage.emplace_back(std::move(ins));

        offset_changed_ = true;
        is_changed_ = true;
    }

    template<typename T>
    T& get(size_t idx) {
        if(idx >= storage.size())
            throw restriction_error("Index out of range");
        return storage[idx].get<T>();
    }

    refl::instance get_instance(size_t idx) {
        return storage.at(idx).clone();
    }

    template<typename T>
    void set(size_t idx, T&& obj) {
        if(idx >= storage.size())
            throw restriction_error("Index out of range");
        storage[idx] = refl::instance::make(std::move(obj));
        if(!storage[idx].get_meta().has_function("__raw_into"))
            throw restriction_error("Not serializable.");

        offset_changed_ = true;
        is_changed_ = true;
    }

    void set_instance(size_t idx, refl::instance&& ins) {
        if(idx >= storage.size())
            throw restriction_error("Index out of range");
        storage.at(idx) = std::move(ins);

        offset_changed_ = true;
        is_changed_ = true;
    }

    std::string definition(const std::string& name,
            const std::vector<std::string>& l) const {
        int i = 0;
        std::string def = "uniform " + name + " {\n";
        for(const std::string& s : l) {
            if(storage[i].is_null()) {
                i += 1; continue;
            }

            def += "    ";
            def += storage[i].call("__glsl_type_name").get<std::string>();
            def += " " + s + ";\n";
            i += 1;
        }
        def += "};";

        return def;
    }

    std::string definition(const std::string& name) const {
        std::vector<std::string> items(size(), "items_");
        for(size_t i = 0; i < size(); i++)
            items[i] += std::to_string(i);
        return definition(name, items);
    }

    size_t size_in_bytes() const {
        update_offsets_();
        return offsets_.back() +
            storage.back().call("__size").get<size_t>();
    }

    size_t size() const {
        return storage.size();
    }

    void copy(uint8_t* buf) const {
        update_offsets_();
        uint8_t* off = buf;
        for(size_t i = 0; i < storage.size(); i++) {
            off = buf + offsets_[i];
            storage[i].call("__raw_into", refl::instance::make((void*)off));
        }
    }

    bool is_changed() const { return is_changed_; }
    void mark_applied() { is_changed_ = false; }

    dynamic_property& operator!() {
        is_changed_ = true;
        return *this;
    }

    static void meta_reg_() {
        refl::meta_manager::reg_class<dynamic_property>("propset")
            .enable_construct<>()
            .enable_construct<size_t>()
            .enable_auto_register()
            .function("get", &dynamic_property::get_instance)
            .function("set", &dynamic_property::set_instance)
            .function("set_float", &dynamic_property::set<float>)
            .function("definition", static_cast<std::string(dynamic_property::*)(const std::string&)const>(&dynamic_property::definition))
            .function("append", &dynamic_property::append_instance)
            .function("append-float", &dynamic_property::append<float>)
            .function("size_in_bytes", &dynamic_property::size_in_bytes)
            .function("resize", &dynamic_property::resize);
    }

protected:
    void update_offsets_() const {
        if(!offset_changed_) return;

        size_t cur_off = 0;
        int i = 0;
        offsets_.resize(size());

        for(const refl::instance& ins : storage) {
            if(ins.is_null()) {
                offsets_[i] = cur_off;
                i += 1; continue;
            }

            size_t align = ins.call("__align").get<size_t>();
            size_t sz = ins.call("__size").get<size_t>();
            cur_off = cur_off % align == 0 ? cur_off :
                (cur_off / align + 1) * align;
            offsets_[i] = cur_off;

            cur_off += sz;
            i += 1;
        }

        offset_changed_ = false;
    }

    std::vector<refl::instance> storage;

    mutable std::vector<size_t> offsets_;
    mutable bool offset_changed_ = true;
    bool is_changed_ = true;
};

template<>
struct prop_trait<dynamic_property> {
    typedef dynamic_property input_type;
    typedef shrtool::indirect_tag transfer_tag;
    typedef uint8_t value_type;

    static size_t size(const input_type& i) { return i.size_in_bytes(); }
    static void copy(const input_type& i, uint8_t* o) { i.copy(o); }

    static bool is_changed(const input_type& i) {
        return i.is_changed();
    }

    static void mark_applied(input_type& i) {
        i.mark_applied();
    }
};

}

#endif // UTILS_H_INCLUDED
