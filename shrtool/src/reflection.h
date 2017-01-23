#ifndef REFLECTION_H_INCLUDED
#define REFLECTION_H_INCLUDED

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <typeinfo>

#include "singleton.h"
#include "exception.h"

namespace shrtool {

namespace refl {

////////////////////////////////////////////////////////////////////////////////

struct instance;

struct meta {
    friend struct meta_manager;

private:
    typedef std::function<instance(instance*[])> fun_type;

    std::string name_;
    const std::type_info& type_info;
    std::unordered_map<std::string, fun_type> functions;

public:
    meta(std::string n, const std::type_info& ti) :
        name_(std::move(n)), type_info(ti) { }
    meta(meta&& m) : name_(std::move(m.name_)), type_info(m.type_info),
        functions(std::move(m.functions)) { }
    meta(const meta& m) = delete; // no copy is permitted

    instance apply(const std::string& name, instance* i[]) const;
    template<typename ... Args>
    instance call(const std::string& name, Args&& ... args) const;

    bool has_function(const std::string& n) const {
        return functions.find(n) != functions.end();
    }

    template<typename RetType, typename ... Args>
    meta& function(std::string name, RetType (*f) (Args...));
    template<typename ... Args>
    meta& function(std::string name, void (*f) (Args...));
    template<typename T, typename RetType, typename ... Args>
    meta& function(std::string name, RetType (T::*f) (Args...));
    template<typename T, typename ... Args>
    meta& function(std::string name, void (T::*f) (Args...));

    template<typename T>
    bool is_same() const {
        return typeid(T).hash_code() == info().hash_code();
    }

    const std::type_info& info() const {
        return type_info;
    }

    const std::string name() const {
        return name_;
    }

    size_t hash_code() const { return info().hash_code(); }

    bool operator==(const meta& m) const {
        return hash_code() == m.hash_code();
    }
    bool operator!=(const meta& m) const {
        return !operator==(m);
    }
};

struct meta_manager : generic_singleton<meta_manager> {
    template<typename T>
    static meta& reg_class(const std::string& name) {
        auto i = inst().name_to_hash.find(name);
        if(i != inst().name_to_hash.end())
            throw duplication_error(name + " has already been registered");

        size_t h = typeid(T).hash_code();
        inst().name_to_hash[name] = h;

        return inst().metas.insert(std::make_pair(
                h, meta(name, typeid(T)))).first->second;
    }

    template<typename T>
    static meta* find_meta() {
        auto i = inst().metas.find(typeid(T).hash_code());
        if(i == inst().metas.end()) return nullptr;
        else return &(i->second);
    }

    static meta* find_meta(const std::string& s) {
        auto i = inst().name_to_hash.find(s);
        if(i == inst().name_to_hash.end()) return nullptr;
        else return &(inst().metas.find(i->second)->second);
    }

    static void clear() {
        inst().metas.clear();
        inst().name_to_hash.clear();
    }

    static void init() {
        clear();

        reg_class<char>("byte");
        reg_class<int>("int");
        reg_class<size_t>("uint");
        reg_class<void*>("pointer");
        reg_class<float>("float");
        reg_class<double>("double");

        enable_cast<int, size_t>();
        enable_cast<int, float>();
        enable_cast<float, double>();
    }

    template<typename T1, typename T2>
    static void enable_cast() {
        meta& meta1 = *find_meta<T1>();
        meta& meta2 = *find_meta<T2>();
        std::string func_name = "__to_" + meta2.name();

        meta1.function(func_name, &cast_<T1, T2>);
    }

private:
    template<typename T1, typename T2>
    static T2 cast_(const T1& rhs) { return T2(rhs); }

    friend class generic_singleton<meta_manager>;

    meta_manager() { }

    std::unordered_map<size_t, meta> metas;
    std::unordered_map<std::string, size_t> name_to_hash;
};

////////////////////////////////////////////////////////////////////////////////

struct instance_stor {
    virtual instance_stor* clone() const = 0;
};

template<typename T>
struct typed_instance_stor : instance_stor {
    typed_instance_stor(typed_instance_stor&& rhs) : data(std::move(rhs.data)) { }
    typed_instance_stor(const typed_instance_stor& rhs) : data(rhs.data) { }
    typed_instance_stor(T&& rhs) : data(std::move(rhs)) { }
    typed_instance_stor(const T& rhs) : data(rhs) { }

    virtual instance_stor* clone() const override {
        return new typed_instance_stor(data);
    }

    T data;
};

struct instance {
    template<typename T>
    static instance make(T o) {
        instance i;
        i.m = meta_manager::find_meta<T>();
        if(!i.m) throw not_found_error("Type has not been registered");
        i.stor.reset(new typed_instance_stor<T>(std::forward<T>(o)));

        return std::move(i);
    }

    template<typename T>
    static instance makeptr(T* p) {
        return make<void*>(reinterpret_cast<T*>(p));
    }

    instance() { }
    instance(instance&& i) : m(i.m), stor(std::move(i.stor)) { }
    instance(const instance& i) : m(i.m), stor(i.stor->clone()) { }

    instance& operator=(instance&& i) {
        m = i.m;
        stor = std::move(i.stor);
        return *this;
    }

    instance& operator=(const instance& i) {
        m = i.m;
        stor.reset(i.stor->clone());
        return *this;
    }

    bool is_null() const { return !stor.get(); }

    const meta& get_meta() { return *m; }

    template<typename T>
    T& get() {
        if(!m->is_same<T>())
            throw restriction_error("Type not match");
        return dynamic_cast<typed_instance_stor<T>*>(stor.get())->data;
    }

    template<typename T>
    T* getptr() {
        if(!m->is_same<void*>())
            throw restriction_error("Type not match");
        return
            reinterpret_cast<T*>(dynamic_cast<
                    typed_instance_stor<void*>*>(stor.get())->data);
    }

    instance cast_to(const meta& rm) const {
        std::string func_name = "__to_" + rm.name();
        if(!m->has_function(func_name))
            throw not_found_error("Not type conversion");
        return m->call(func_name, *this);
    }

private:
    const meta* m = nullptr;
    std::unique_ptr<instance_stor> stor;
};

////////////////////////////////////////////////////////////////////////////////
// func_caller and function former
//
// Since its impossible to register pointer to all types, pointers are all cast
// to void*, whatever they point to. All other types are cast to references
// when invoking a function. Not to distinguish const reference or not.

template<typename T>
struct raw_type_ {
    typedef typename std::decay<T>::type type;
};

template<typename T>
struct raw_type_<T*> {
    typedef void* type;
};

template<typename RetType>
inline RetType func_caller(instance*[], std::function<RetType()> f)
{
    return f();
}

template<typename RetType, typename Head, typename ... Args>
inline RetType func_caller(instance* args[],
        std::function<RetType(Head, Args...)> f)
{
    typedef typename raw_type_<Head>::type raw_type;
    typedef typename std::remove_reference<Head>::type* ptr_type;
    const meta* target_meta = meta_manager::find_meta<raw_type>();
    if(!target_meta)
        throw not_found_error("Type has not been registered");

    ptr_type head_arg;

    if(args[0]->get_meta() != *target_meta) {
        instance tmp = std::move(args[0]->cast_to(*target_meta));
        head_arg = reinterpret_cast<ptr_type>(&(tmp.get<raw_type>()));

        return func_caller(args + 1, std::function<RetType(Args...)>(
                [&](Args ... rest) -> RetType {
                    return f(*head_arg, rest ...);
                }));
    } else {
        head_arg = reinterpret_cast<ptr_type>(&(args[0]->get<raw_type>()));

        return func_caller(args + 1, std::function<RetType(Args...)>(
                [&](Args ... rest) -> RetType {
                    return f(*head_arg, rest ...);
                }));
    }
}

template<typename RetType, typename ... Args>
meta& meta::function(std::string name, RetType (*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[]) -> instance {
        return instance::make<typename raw_type_<RetType>::type>(
                func_caller(args, std::function<RetType(Args...)>(f)));
    };

    return *this;
}

template<typename ... Args>
meta& meta::function(std::string name, void (*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[]) -> instance {
        func_caller(args, std::function<void(Args...)>(f));
        return instance();
    };

    return *this;
}

template<typename T, typename RetType, typename ... Args>
meta& meta::function(std::string name, RetType (T::*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[]) -> instance {
        return instance::make<typename raw_type_<RetType>::type>(
            func_caller(args + 1, std::function<RetType(Args...)>(
                [&args, f](Args ... a) -> RetType {
                    return (args[0]->get<T>().*f)(a ...);
                })));
    };

    return *this;
}

template<typename T, typename ... Args>
meta& meta::function(std::string name, void (T::*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[]) -> instance {
        func_caller(args + 1, std::function<void(Args...)>(
            [&args, f](Args ... a) {
                (args[0]->get<T>().*f)(a ...);
            }));
        return instance();
    };

    return *this;
}

inline instance meta::apply(const std::string& name, instance* i[]) const
{
    auto fi = functions.find(name);
    if(fi == functions.end())
        throw not_found_error("No such function: " + name);
    fun_type f = fi->second;
    return f(i);
}

template<typename ... Args>
instance meta::call(const std::string& name, Args&& ... args) const {
    instance* insts[sizeof...(args)] = { const_cast<instance*>(&args) ... };
    return apply(name, insts);
}

}

}

#endif // REFLECTION_H_INCLUDED
