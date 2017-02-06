#ifndef PROVIDERS_H_INCLUDED
#define PROVIDERS_H_INCLUDED

/*
 * Providers act a role of bridge connecting resources (meshes, images or
 * suchlike) and render assets (textures, buffers, or even shaders). It has
 * two functions: load and update.
 *
 * - load: this function create a GPU resource anew, without any data. GPU
 *   resource classes defined in shading.h allow you to create empty objects
 *   that is not committed to the GPU. After that, it applies it with update.
 *   When you don't want to fill any data in a new object, you can use like
 *   `provider<void, texture2d>::load`.
 *
 * - update: this function has two mode, anew and not anew. Updating anew is
 *   only allowed to be applied on empty resources or old resources that have
 *   exact the same configuration (like size, data format/layout), which
 *   overrides the resource completely, costing quite much time. Non-anew mode
 *   requires a non-empty object, or display driver may reject to execute the
 *   commands and throw exceptions.
 *
 * Providers are mainly used by shader_render_task.
 *
 * NOTE: NON-ANEW MODE IS CURRENTLY NOT IMPLEMENTED IN MOST PROVIDERS.
 */

#include <functional>
#include <iostream>

#include "shading.h"
#include "traits.h"

namespace shrtool {

#define DEF_LOAD_FUNC \
    static output_type load(input_type& i) { \
        output_type new_; \
        update(i, new_, true); \
        return new_; \
    }

template<typename InputType, typename OutputType>
struct provider {
    /*
    typedef OutputType output_type;
    typedef InputType input_type;

    DEF_LOAD_FUNC

    static void update(const InputType& i, OutputType& o, bool anew) { }
    */
};

////////////////////////////////////////////////////////////////////////////////

template<typename tag>
struct attr_provider_updater { };

template<>
struct attr_provider_updater<raw_data_tag> {
    typedef vertex_attr_vector output_type;

    template<typename input_type,
        typename Trait = attr_trait<input_type>>
    static void update(const input_type& i, output_type& o, bool anew) {
        if(anew) {
            o.primitives_count(Trait::count(i));
            for(size_t s_i = 0; ; ++s_i) {
                int s = Trait::slot(i, s_i);
                if(s < 0) break;
                auto b = o.share_input(s);
                if(!b) {
                    o.add_input(s, Trait::dim(i, s_i) *
                        Trait::count(i) * sizeof(typename Trait::elem_type));
                    b = o.share_input(s);
                }
                b->write(Trait::data(i, s_i));
                o.updated(s);
            }
        }
    }
};

template<>
struct attr_provider_updater<indirect_tag> {
    typedef vertex_attr_vector output_type;

    template<typename input_type,
        typename Trait = attr_trait<input_type>>
    static void update(const input_type& i, output_type& o, bool anew) {
        if(anew) {
            o.primitives_count(Trait::count(i));
            for(size_t s_i = 0; ; ++s_i) {
                int s = Trait::slot(i, s_i);
                if(s < 0) break;
                auto b = o.share_input(s);
                if(!b) {
                    o.add_input(s, Trait::dim(i, s_i) *
                        Trait::count(i) * sizeof(typename Trait::elem_type));
                    b = o.share_input(s);
                }
                typename Trait::elem_type* p =
                    b->start_map<typename Trait::elem_type>(
                            render_assets::buffer::WRITE);
                Trait::copy(i, s_i, p);
                b->stop_map();
                o.updated(s);
            }
        }
    }
};

template<typename InputType>
struct provider<InputType, vertex_attr_vector>{
    typedef vertex_attr_vector output_type;
    typedef InputType input_type;

    DEF_LOAD_FUNC

    static void update(const input_type& i, output_type& o, bool anew) {
        attr_provider_updater<
            typename attr_trait<input_type>::transfer_tag>
            ::update(i, o, anew);
    }

};

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename Trait = prop_trait<T>,
    typename Func = decltype(&Trait::is_changed)>
bool optional_is_changed(const T& i, int) { return Trait::is_changed(i); }
template<typename T, typename Trait = prop_trait<T>,
    typename Func = void, typename Int = int>
bool optional_is_changed(const T& i, Int) { return false; }

template<typename T, typename Trait = prop_trait<T>,
    typename Func = decltype(&Trait::is_changed)>
void optional_mark_applied(T& i, int) { Trait::mark_applied(i); }
template<typename T, typename Trait = prop_trait<T>,
    typename Func = void, typename Int = int>
void optional_mark_applied(T& i, Int) { }

template<typename T, typename T2 = int>
constexpr void* optional_value_type(T2) { return nullptr; }
template<typename T>
constexpr typename T::value_type* optional_value_type(int) { return nullptr; }

template<typename tag>
struct prop_provider_updater { };

template<>
struct prop_provider_updater<raw_data_tag> {
    typedef render_assets::property_buffer output_type;

    template<typename input_type,
        typename Trait = prop_trait<input_type>>
    static void update(input_type& i, output_type& o, bool anew) {
        if(!anew && !optional_is_changed(i, 0)) return;

        o.write(Trait::data(i), Trait::size(i));
        optional_mark_applied(i, 0);
    }
};

template<>
struct prop_provider_updater<indirect_tag> {
    typedef render_assets::property_buffer output_type;

    template<typename input_type,
        typename Trait = prop_trait<input_type>>
    static void update(input_type& i, output_type& o, bool anew) {
        if(!anew && !optional_is_changed(i, 0)) return;

        void* p = o.start_map(render_assets::buffer::WRITE,
                Trait::size(i));
        Trait::copy(i, reinterpret_cast<
                decltype(optional_value_type<Trait>(0))>(p));
        o.stop_map();
        optional_mark_applied(i, 0);
    }
};

template<typename InputType>
struct provider<InputType, render_assets::property_buffer> {
    typedef render_assets::property_buffer output_type;
    typedef InputType input_type;

    DEF_LOAD_FUNC

    template<typename Trait = prop_trait<input_type>>
    static void update(input_type& i, output_type& o, bool anew) {
        prop_provider_updater<typename Trait::transfer_tag>
            ::update(i, o, anew);
    }
};

////////////////////////////////////////////////////////////////////////////////

template<typename InputType>
struct provider<InputType, render_assets::texture2d> {
    typedef render_assets::texture2d output_type;
    typedef InputType input_type;

    DEF_LOAD_FUNC

    template<typename Trait = texture2d_trait<input_type>>
    static void update(input_type& i, output_type& p, bool anew) {
        if(anew) {
            p.width(Trait::width(i));
            p.height(Trait::height(i));
            p.fill(Trait::data(i), Trait::format(i));
        }
    }
};

template<typename InputType>
struct provider<InputType, render_assets::texture_cubemap> {
    typedef render_assets::texture_cubemap output_type;
    typedef InputType input_type;

    DEF_LOAD_FUNC

    template<typename Trait = texture2d_trait<input_type>>
    static void update(input_type& i, output_type& p, bool anew) {
        if(anew) {
            p.width(Trait::width(i));
            p.height(Trait::height(i) / 6);
            p.fill(Trait::data(i), Trait::format(i));
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template<typename InputType>
struct provider<InputType, shader> {
    typedef shader output_type;
    typedef InputType input_type;

    DEF_LOAD_FUNC

    template<typename Trait = shader_trait<input_type>>
    static void update(const input_type& i, output_type& p, bool anew) {
        if(anew) {
            for(size_t e = 0; ; e++) {
                shader::shader_type t;
                std::string src = Trait::source(i, e, t);
                if(src.empty()) break;

                auto s = p.share_sub_shader(t);
                if(!s) {
                    p.add_sub_shader(t);
                    s = p.share_sub_shader(t);
                }
                s->compile(src);
            }
            p.link();
        }
    }
};

#undef DEF_LOAD_FUNC

}

#endif // PROVIDERS_H_INCLUDED

