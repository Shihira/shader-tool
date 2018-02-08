#ifndef RES_POOL_H_INCLUDED
#define RES_POOL_H_INCLUDED

/*
 * res_pool and ires_pool are dynamic typed storage. you can tag your objects
 * with a string or int, or any thing else that can be a map index.
 */

#include <string>
#include <memory>
#include <unordered_map>
#include <typeinfo>

#include "common/exception.h"

namespace shrtool {

struct res_storage_base {
    virtual std::string type_name() const = 0;
    virtual ~res_storage_base() { };

    template<typename T>
    bool type_hint(const T&) const {
        return typeid(T).name() == type_name();
    }

    template<typename T>
    bool type_hint() const {
        return typeid(T).name() == type_name();
    }

    template<typename T>
    bool assert_type(const T&) const {
        if(!type_hint<T>())
            throw type_matching_error("Type mismatch: " +
                    type_name() + " is required but got " +
                    typeid(T).name());
    }

    template<typename T>
    void assert_type() const {
        if(!type_hint<T>())
            throw type_matching_error("Type mismatch: " +
                    type_name() + " is required but got " +
                    typeid(T).name());
    }
};

template<bool AllowChangeName, typename NameT>
struct name_helper {
    typedef NameT name_type;
    name_type name;
    name_helper(name_type n) : name(std::move(n)) { }
};

template<typename NameT>
struct name_helper<false, NameT> {
    typedef NameT name_type;
    const name_type name;
    name_helper(name_type n) : name(std::move(n)) { }
};

template<bool AllowChangeName, typename NameT>
struct base_named_object :
    name_helper<AllowChangeName, NameT>,
    res_storage_base {
    using name_helper<AllowChangeName, NameT>::name_helper;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename BaseT>
struct basic_res_object : BaseT {
private:
    typedef BaseT base_type;
public:
    typedef typename base_type::name_type name_type;
    typedef T value_type;

    T data;

    template<typename ...Args>
    basic_res_object(name_type s, Args ...args) :
        base_type(std::move(s)), data(std::forward<Args>(args)...) { }

    basic_res_object(basic_res_object&& obj) :
        base_type(std::move(obj.name)), data(std::move(obj)) { }

    operator value_type&() { return data; }
    operator const value_type&() const { return data; }
    operator value_type*() { return &data; }
    operator const value_type*() const { return &data; }

    value_type* operator*() { return &data; }
    const value_type* operator*() const { return &data; }
    value_type* operator->() { return &data; }
    const value_type* operator->() const { return &data; }

    ~basic_res_object() { }
    std::string type_name() const override {
        return typeid(T).name();
    }
};

template<typename NameT,
    typename ValueT = base_named_object<false, NameT>,
    typename = typename std::enable_if<std::is_base_of<base_named_object<false, NameT>, ValueT>::value>::type>
class basic_res_pool {
public:
    typedef NameT name_type;
    typedef ValueT stor_base_type;
    typedef std::weak_ptr<stor_base_type> stor_base_ref;

    template<typename T>
    using stor_type = basic_res_object<T, stor_base_type>;
    template<typename T>
    using stor_ref = std::weak_ptr<stor_type<T>>;
    template<typename T>
    using stor_const_ref = std::weak_ptr<const stor_type<T>>;

private:
    std::unordered_map<name_type, std::shared_ptr<stor_base_type>> data_;
    bool type_hint_ = false;
    bool redef_ = false;

public:
    void type_hint_enabled(bool b) { type_hint_ = b; }
    bool type_hint_enabled() const { return type_hint_; }
    void redefinition_allowed(bool b) { redef_ = b; }
    bool redefinition_allowed() const { return redef_; }

    template<typename T, typename ...Args>
    stor_ref<T> insert(name_type name, Args ...args) {
        auto& p = data_[name];
        p.reset(new stor_type<T>(std::move(name),
                    std::forward<Args>(args)...));
        return std::dynamic_pointer_cast<stor_type<T>>(p);
    }

    template<typename T>
    stor_ref<T> insert(name_type name, T&& arg) {
        auto& p = data_[name];
        p.reset(new stor_type<T>(std::move(name), std::move(arg)));
        return std::dynamic_pointer_cast<stor_type<T>>(p);
    }

    template<typename T>
    stor_ref<T> insert(name_type name, const T& arg) {
        auto& p = data_[name];
        p.reset(new stor_type<T>(std::move(name), arg));
        return std::dynamic_pointer_cast<stor_type<T>>(p);
    }

    template<typename T>
    void insert_shared(name_type name,
            const std::shared_ptr<stor_type<T>>& p_) {
        auto& p = data_[name];
        p = std::static_pointer_cast<res_storage_base>(p_);
    }

    void remove(const name_type& name) {
        auto i = data_.find(name);
        if(i != data_.end()) data_.erase(i);
    }

    template<typename T>
    stor_ref<T> ref(const name_type& n) const {
        auto p = data_.find(n);
        if(p == data_.end() || !bool(p->second))
            return std::weak_ptr<stor_type<T>>();
        else {
            if(type_hint_enabled())
                p->second->template assert_type<T>();
            return std::dynamic_pointer_cast<stor_type<T>>(p->second);
        }
    }

    stor_base_ref ref(const name_type& n) const {
        auto p = data_.find(n);
        if(p == data_.end() || !bool(p->second))
            return stor_base_ref();
        else
            return p->second;
    }

    template<typename T>
    stor_const_ref<T> const_ref(const name_type& n) const {
        auto p = data_.find(n);
        if(p == data_.end() || !bool(p->second))
            return std::weak_ptr<stor_type<T>>();
        else {
            if(type_hint_enabled())
                p->second->template assert_type<T>();
            return std::dynamic_pointer_cast<const stor_type<T>>(p->second);
        }
    }

    template<typename T>
    T& get(const name_type& n) const {
        auto r = ref<T>(n);
        if(r.expired())
            throw not_found_error("No item is available");
        return r.lock()->data;
    }

    std::string type_name(const std::string& n) const {
        auto i = data_.find(n);
        if(i != data_.end()) return i->second->type_name();
        else return "";
    }

    basic_res_pool& operator=(basic_res_pool&& p) {
        data_ = std::move(p.data_);
        type_hint_ = p.type_hint_;
        redef_ = p.redef_;
    }

    basic_res_pool() { }
    basic_res_pool(basic_res_pool&& p) { operator=(std::move(p)); }
};

typedef basic_res_pool<std::string> res_pool;
typedef basic_res_pool<size_t> ires_pool;

}

#endif // RES_POOL_H_INCLUDED
