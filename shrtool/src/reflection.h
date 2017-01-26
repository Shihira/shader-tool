#ifndef REFLECTION_H_INCLUDED
#define REFLECTION_H_INCLUDED

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <typeinfo>

#include "singleton.h"
#include "exception.h"
#include "traits.h"
#include "matrix.h"

namespace shrtool {

namespace refl {

////////////////////////////////////////////////////////////////////////////////

struct instance;

struct meta {
    friend struct meta_manager;

private:
    typedef std::function<instance(instance*[], size_t n)> fun_type;

    std::string name_;
    const std::type_info& type_info;
    std::map<std::string, fun_type> functions;

public:
    meta(std::string n, const std::type_info& ti) :
        name_(std::move(n)), type_info(ti) { }
    meta(meta&& m) : name_(std::move(m.name_)), type_info(m.type_info),
        functions(std::move(m.functions)) { }
    meta(const meta& m) = delete; // no copy is permitted

    instance apply(const std::string& name, instance* i[], size_t n) const;
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

template<typename T>
struct typed_meta : meta {
    template<typename To>
    typed_meta& enable_cast();

    /*
     * the argument Enable in parameter means to prevent compiler generates code
     * by default rather than only when the function is called.
     */
    template<bool Enable = true>
    typed_meta& enable_clone() {
        function("__clone", &clone_<Enable>);
        return *this;
    }

    template<bool Enable = true>
    typed_meta& enable_serialize() {
        function("__size", &item_trait_adapter<T, size_t, size_t>::size);
        function("__align", &item_trait_adapter<T, size_t, size_t>::align);
        function("__raw_into", &item_trait_adapter<T, size_t, size_t>::copy);
        function("__glsl_type_name", &item_trait_adapter<T, size_t, size_t>::glsl_type_name);
        return *this;
    }

private:
    template<typename To>
    static To cast_(const T& o) { return To(o); }

    template<bool Enable = true>
    static T clone_(const T& o) {
        return o;
    }
};

struct meta_manager : generic_singleton<meta_manager> {
    template<typename T>
    static typed_meta<T>& reg_class(const std::string& name) {
        auto i = inst().name_to_hash.find(name);
        if(i != inst().name_to_hash.end())
            throw duplication_error(name + " has already been registered");

        size_t h = typeid(T).hash_code();
        inst().name_to_hash[name] = h;

        return static_cast<typed_meta<T>&>(
            inst().metas.insert(std::make_pair(
                h, meta(name, typeid(T)))).first->second);
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

    template<typename T>
    static meta& get_meta() {
        auto m = find_meta<T>();
        if(!m) throw not_found_error("Type has not yet been registered.");
        return *m;
    }

    static meta& get_meta(const std::string& s) {
        auto m = find_meta(s);
        if(!m) throw not_found_error("Type has not yet been registered.");
        return *m;
    }

    static void clear() {
        inst().metas.clear();
        inst().name_to_hash.clear();
    }

    static void init() {
        clear();

        reg_class<char>("byte")
            .enable_clone()
            .enable_serialize();
        reg_class<int>("int")
            .enable_clone()
            .enable_serialize();
        reg_class<size_t>("uint")
            .enable_clone()
            .enable_serialize();
        reg_class<void*>("pointer")
            .enable_clone();
        reg_class<float>("float")
            .enable_clone()
            .enable_serialize();
        reg_class<double>("double")
            .enable_clone()
            .enable_serialize();
        reg_class<std::string>("string")
            .enable_clone();
        reg_class<math::fxmat>("fmatrix")
            .enable_clone()
            .enable_serialize();
        reg_class<math::dxmat>("matrix")
            .enable_clone()
            .enable_serialize();

        enable_cast<int, size_t>();
        enable_cast<int, float>();
        enable_cast<float, double>();
    }

    template<typename T1, typename T2>
    static void enable_cast() {
        typed_meta<T1>& m = static_cast<typed_meta<T1>&>(get_meta<T1>());
        m.template enable_cast <T2>();
    }

private:
    friend class generic_singleton<meta_manager>;

    meta_manager() { }

    std::map<size_t, meta> metas;
    std::map<std::string, size_t> name_to_hash;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T>
template<typename To>
typed_meta<T>& typed_meta<T>::enable_cast()
{
    meta& to_m = meta_manager::get_meta<To>();
    std::string func_name = "__to_" + to_m.name();
    function(func_name, &cast_<To>);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////

struct instance_stor {
    virtual ~instance_stor() { }
};

template<typename T>
struct typed_instance_stor : instance_stor {
    typed_instance_stor(typed_instance_stor&& rhs) : data(std::move(rhs.data)) { }
    typed_instance_stor(const typed_instance_stor& rhs) : data(rhs.data) { }
    typed_instance_stor(T&& rhs) : data(std::move(rhs)) { }
    typed_instance_stor(const T& rhs) : data(rhs) { }

    virtual ~typed_instance_stor() { }

    T data;
};

struct instance {
    template<
        typename T,
        typename Enable = typename std::enable_if<
            !std::is_pointer<T>::value>::type>
    static instance make(T o) {
        instance i;
        i.m = &meta_manager::get_meta<
            typename std::remove_reference<T>::type>();
        i.stor.reset(new typed_instance_stor<
            typename std::remove_reference<T>::type>(std::move(o)));

        return std::move(i);
    }

    /*
     * Note: when you specify type T manually, remember not to bring * along.
     */
    template<typename T>
    static instance make(T* p) {
        instance i = make<void*, void>(reinterpret_cast<T*>(p));
        i.ptrm = meta_manager::find_meta<T>();
        return std::move(i);
    }

    instance() { }
    instance(instance&& i) :
        m(i.m), ptrm(i.ptrm), stor(std::move(i.stor)) { }
    instance(const instance& i) { operator=(i); }

    template<typename ... Args>
    instance call(const std::string& fn, Args&& ... args) const {
        return get_meta().call(fn, *this, std::forward<Args>(args) ...);
    }

    instance& operator=(instance&& i) {
        m = i.m;
        ptrm = i.ptrm;
        stor = std::move(i.stor);
        return *this;
    }

    instance& operator=(const instance& i) {
        return operator=(std::move(i.clone()));
    }

    bool is_null() const { return !stor.get(); }
    bool is_pointer() const { return m->is_same<void*>(); }

    const meta& get_meta() const { return *m; }
    const meta* get_pointer_meta() const { return ptrm; }

    /*
     * You cannot get pointer itself through calling get (except for void*),
     * because get always convert pointers to references. If you exactly need a
     * pointer address, do `&inst.get<int>()`
     */
    template<typename T>
    T& get() {
        if(m->is_same<T>()) {
            auto p = dynamic_cast<typed_instance_stor<T>*>(stor.get());
            if(!p) throw restriction_error("Conversion failed");
            return p->data;
        }

        if(m->is_same<void*>() && (!ptrm || ptrm->is_same<T>())) {
            auto p = dynamic_cast<typed_instance_stor<void*>*>(stor.get());
            if(!p) throw restriction_error("Conversion failed");
            return *reinterpret_cast<T*>(p->data);
        }

        throw type_matching_error("Type not match");
    }

    instance cast_to(const meta& rm) const {
        std::string func_name = "__to_" + rm.name();
        if(!m->has_function(func_name))
            throw not_found_error("No type conversion");
        return m->call(func_name, *this);
    }

    instance clone() const {
        if(!m->has_function("__clone"))
            throw not_found_error("No cloning");
        return m->call("__clone", *this);
    }

private:
    const meta* m = nullptr;
    const meta* ptrm = nullptr;
    std::unique_ptr<instance_stor> stor;
};

////////////////////////////////////////////////////////////////////////////////
// func_caller and function former

/*
 * Policies on Type Determination
 *
 * For functions, inspect their return value types and argument types, and when:
 *
 * - Return value type is a ?, it will be converted to ?
 *     - T&: instance of (void*, T)
 *     - T*: instance of (void*, T)
 *     - T: instance of T
 * - Argument type is a ?, instances are expected to be an ?
 *     - T&: instance of T or ttcbct T or (void*, T)
 *     - T*: instance of (void*, T)
 *     - T: instance of T or ttcbct T
 *
 * ttcbct = type that can be converted to
 *
 * An instance of pointer is actually an instance of type void* which has an
 * additional property "type to which it points", denoted as (void*, T).
 * Pointing to an unknown type is also allowed while its additional property is
 * set to null, which may bring about unsafe type conversion. In the list above,
 * assuming T is not registered, things you can get or pass in are no more than
 * instances of (void*, T)
 *
 * ALL CONST MODIFIER WILL BE REMOVED IMMEDIATELY IN THIS PROCESS.
 */

template<typename T>
struct ret_type_ {
    static instance convert(T t) {
        return instance::make<typename std::decay<T>::type>(std::move(t));
    }
};

template<typename T>
struct ret_type_<T*> {
    static instance convert(T* t) {
        return instance::make<typename std::remove_reference<T>::type>(t);
    }
};

template<typename T>
struct ret_type_<T&> {
    static instance convert(T& t) {
        return instance::make<typename std::remove_cv<T>::type>(&t);
    }
};

template<typename T>
struct arg_type_ {
    instance tmp;
    typedef typename std::remove_cv<T>::type pure_t;
    pure_t& convert(instance& t) {
        if(t.get_meta().is_same<pure_t>())
            return t.get<pure_t>();
        meta& target_m = meta_manager::get_meta<pure_t>();
        tmp = t.cast_to(target_m);
        return tmp.get<pure_t>();
    }
};

template<typename T>
struct arg_type_<T&&> {
    typedef typename std::remove_cv<T>::type pure_t;
    pure_t&& convert(instance& t) {
        if(t.get_meta().is_same<pure_t>()) {
            return std::move(t.get<pure_t>());
        } else
            throw type_matching_error("Type not matched");
    }
};

template<typename T>
struct arg_type_<T*> {
    typedef typename std::remove_cv<T>::type pure_t;
    pure_t*& convert(instance& t) {
        if(!t.is_pointer())
            throw restriction_error("Not a pointer");
        const meta* ptrm = t.get_pointer_meta();
        if(!ptrm && ptrm != meta_manager::find_meta<pure_t>()) // TODO: stricter?
            throw type_matching_error("Type not matched");

        return *reinterpret_cast<pure_t**>(&t.get<void*>());
    }
};

template<typename T>
struct arg_type_<T&> : arg_type_<T>, arg_type_<T*> {
    typedef typename std::remove_cv<T>::type pure_t;
    pure_t& convert(instance& t) {
        if(t.is_pointer())
            return *arg_type_<T*>::convert(t);
        else
            return arg_type_<T>::convert(t);
    }
};

template<typename RetType>
inline RetType func_caller(instance*[], size_t n, std::function<RetType()> f)
{
    if(n != 0)
        throw restriction_error("Number of arguments does not match");
    return f();
}

template<typename RetType, typename Head, typename ... Args>
inline RetType func_caller(instance* args[], size_t n,
        std::function<RetType(Head, Args...)> f)
{
    if(n != sizeof...(Args) + 1)
        throw restriction_error("Number of arguments does not match");

    arg_type_<Head> cur_arg;

    return func_caller(args + 1, n - 1, std::function<RetType(Args...)>(
            [&](Args ... rest) -> RetType {
                return f(std::forward<Head>(cur_arg.convert(*args[0])),
                        std::forward<Args>(rest) ...);
            }));
}

////////////////////////////////////////////////////////////////////////////////

template<typename RetType, typename ... Args>
meta& meta::function(std::string name, RetType (*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[], size_t n) -> instance {
        return ret_type_<RetType>::convert(
                func_caller(args, n, std::function<RetType(Args...)>(f)));
    };

    return *this;
}

template<typename ... Args>
meta& meta::function(std::string name, void (*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[], size_t n) -> instance {
        func_caller(args, n, std::function<void(Args...)>(f));
        return instance();
    };

    return *this;
}

template<typename T, typename RetType, typename ... Args>
meta& meta::function(std::string name, RetType (T::*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[], size_t n) -> instance {
        return ret_type_<RetType>::convert(
            func_caller(args + 1, n - 1, std::function<RetType(Args...)>(
                [&args, f](Args ... a) -> RetType {
                    return (args[0]->get<T>().*f)(std::forward<Args>(a) ...);
                })));
    };

    return *this;
}

template<typename T, typename ... Args>
meta& meta::function(std::string name, void (T::*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[], size_t n) -> instance {
        func_caller(args + 1, n - 1, std::function<void(Args...)>(
            [&args, f](Args ... a) {
                (args[0]->get<T>().*f)(std::forward<Args>(a) ...);
            }));
        return instance();
    };

    return *this;
}

inline instance meta::apply(const std::string& name, instance* i[], size_t n) const
{
    auto fi = functions.find(name);
    if(fi == functions.end())
        throw not_found_error("No such function: " + name);
    fun_type f = fi->second;
    return f(i, n);
}

template<typename ... Args>
instance meta::call(const std::string& name, Args&& ... args) const {
    instance* insts[sizeof...(args)] = { &const_cast<instance&>(args) ... };
    return apply(name, insts, sizeof...(args));
}

}

}

#endif // REFLECTION_H_INCLUDED
