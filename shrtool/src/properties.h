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
};

////////////////////////////////////////////////////////////////////////////////

struct dynamic_property_storage_base {
    virtual const char* glsl_type_name() const = 0;
    virtual void copy(uint8_t* data) const = 0;
    virtual size_t align(size_t start_at) const = 0;
    virtual size_t size() const = 0;
    virtual ~dynamic_property_storage_base() { }
};

template<typename T>
struct dynamic_property_storage : dynamic_property_storage_base {
    typedef item_trait<T> trait;
    typedef T value_type;

    value_type v;

    virtual const char* glsl_type_name() const override {
        return trait::glsl_type_name();
    }

    virtual void copy(uint8_t* data) const override {
        trait::copy(v, reinterpret_cast<typename trait::value_type*>(data));
    }

    virtual size_t align(size_t start_at) const override {
        return start_at % trait::align ?
            (start_at / trait::align + 1) * trait::align : start_at;
    }

    virtual size_t size() const override {
        return trait::size;
    }

    ~dynamic_property_storage() override { }

    template<typename ...Args>
    dynamic_property_storage(Args ...args) : v(std::forward(args)...) { }
};

struct dynamic_property {
private:
    std::vector<std::unique_ptr<dynamic_property_storage_base>> storage_;
    bool type_hint_;

    // offset cache
    mutable std::vector<size_t> offsets_;
    mutable size_t size_in_bytes_ = 0;
    mutable bool item_type_updated_ = false;

    void update_offsets_() const {
        if(!item_type_updated_) return;

        offsets_.resize(storage_.size());
        size_t align = 0, i = 0;
        for(const auto& p : storage_) {
            if(!p) { offsets_[i] = align; continue; }
            align = p->align(align);
            offsets_[i] = align;
            align += p->size();

            i += 1;
        }

        size_in_bytes_ = align;
        item_type_updated_ = false;
    }

public:
    void type_hint_enabled(bool b) { type_hint_ = b; }
    bool type_hint_enabled() const { return type_hint_; }

    size_t offset(size_t i) {
        update_offsets_();
        return offsets_[i];
    }

    size_t size() const {
        return storage_.size();
    }

    size_t size_in_bytes() const {
        update_offsets_();
        return size_in_bytes_;
    }

    void copy(uint8_t* data) const {
        update_offsets_();
        size_t i = 0;
        for(const auto& p : storage_) {
            if(p) p->copy(data + offsets_[i]);
            i += 1;
        }
    }

    std::string definition(const std::string& name,
            std::initializer_list<std::string> l) {
        int i = 0;
        std::string def = "uniform " + name + " {\n";
        for(const std::string& s : l) {
            if(!storage_[i]) { i += 1; continue; }
            def += "    ";
            def += storage_[i]->glsl_type_name();
            def += " " + s + ";\n";
            i += 1;
        }
        def += "};";

        return def;
    }

    template<typename T>
    T& get(size_t i) {
        if(storage_.size() < i + 1) {
            storage_.resize(i + 1);
        }

        dynamic_property_storage<T>* p = nullptr;
        if(storage_[i]) {
            if(type_hint_) {
                if(item_trait<T>::glsl_type_name() !=
                        std::string(storage_[i]->glsl_type_name()))
                    throw type_matching_error("Type not matched");
                p = dynamic_cast<dynamic_property_storage<T>*>(
                        storage_[i].get());
            }

            p = reinterpret_cast<dynamic_property_storage<T>*>(
                    storage_[i].get());
        } else {
            p = new dynamic_property_storage<T>();
            storage_[i].reset(p);
            item_type_updated_ = true;
        }

        return p->v;
    }

    template<typename T>
    const T& get(size_t i) const {
        if(storage_.size() < i + 1)
            throw restriction_error("Index exceeds");
        if(!storage_[i])
            throw restriction_error("Not initialized");

        dynamic_property_storage<T>* p = nullptr;
        if(type_hint_) {
            if(item_trait<T>::glsl_type_name !=
                    std::string(storage_[i]->glsl_type_name()))
                throw type_matching_error("Type not matched");
            p = dynamic_cast<dynamic_property_storage<T>*>(
                    storage_[i].get());
        }

        p = reinterpret_cast<dynamic_property_storage<T>*>(
                storage_[i].get());

        return p->v;
    }
};

template<>
struct prop_trait<dynamic_property> {
    typedef dynamic_property input_type;
    typedef shrtool::indirect_tag transfer_tag;
    typedef uint8_t value_type;

    static size_t size(const input_type& i) { return i.size_in_bytes(); }
    static void copy(const input_type& i, uint8_t* o) { i.copy(o); }
};

}

#endif // UTILS_H_INCLUDED
