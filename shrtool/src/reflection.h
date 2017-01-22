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

struct instance;

struct meta {
    friend struct meta_manager;

private:
    typedef std::function<instance(instance*[])> memfun_type;

    std::string name;
    const std::type_info& type_info;
    std::unordered_map<std::string, memfun_type> functions;

public:
    meta(std::string n, const std::type_info& ti) :
        name(std::move(n)), type_info(ti) { }
    meta(meta&& m) : name(std::move(m.name)), type_info(m.type_info),
        functions(std::move(m.functions)) { }
    meta(const meta& m) = delete;

    instance apply(const std::string& name, instance* i[]);
    template<typename ... Args>
    instance call(const std::string& name, Args& ... args);
    template<typename ... Args>
    instance call(const std::string& name, Args&& ... args);

    template<typename RetType, typename ... Args>
    meta& function(std::string name, RetType (*f) (Args...));
    template<typename ... Args>
    meta& function(std::string name, void (*f) (Args...));

    template<typename T>
    bool is_same() const {
        return typeid(T).hash_code() == info().hash_code();
    }

    const std::type_info& info() const {
        return type_info;
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

        reg_class<char>("char");
        reg_class<int>("int");
        reg_class<float>("float");
        reg_class<double>("double");
    }

private:
    friend class generic_singleton<meta_manager>;

    meta_manager() { }

    std::unordered_map<size_t, meta> metas;
    std::unordered_map<std::string, size_t> name_to_hash;
};

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

    instance() { }
    instance(instance&& i) : m(i.m), stor(std::move(i.stor)) { }
    instance(const instance& i) : m(i.m), stor(i.stor->clone()) { }

    bool is_null() const { return !stor.get(); }

    const meta& get_meta() { return *m; }
    template<typename T>
    T& get() {
        if(!m->is_same<T>())
            throw restriction_error("Type not match");
        return dynamic_cast<typed_instance_stor<T>*>(stor.get())->data;
    }

private:
    const meta* m = nullptr;
    std::unique_ptr<instance_stor> stor;
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
    Head& head_arg = (*args)->get<typename std::decay<Head>::type>();
    return func_caller(args + 1, std::function<RetType(Args...)>(
            [&](Args ... rest) -> RetType {
                return f(head_arg, rest ...);
            }));
}

template<typename RetType, typename ... Args>
meta& meta::function(std::string name, RetType (*f) (Args...))
{
    functions[std::move(name)] = [f](instance* args[]) -> instance {
        return instance::make<typename std::decay<RetType>::type>(
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

inline instance meta::apply(const std::string& name, instance* i[])
{
    auto fi = functions.find(name);
    if(fi == functions.end())
        throw not_found_error("No such function: " + name);
    memfun_type f = fi->second;
    return f(i);
}

template<typename ... Args>
instance meta::call(const std::string& name, Args& ... args) {
    instance* insts[sizeof...(args)] = { &args ... };
    return apply(name, insts);
}

template<typename ... Args>
instance meta::call(const std::string& name, Args&& ... args) {
    instance* insts[sizeof...(args)] = { &args ... };
    return apply(name, insts);
}

}

}

#endif // REFLECTION_H_INCLUDED
