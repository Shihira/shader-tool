#ifndef PROVIDERS_H_INCLUDED
#define PROVIDERS_H_INCLUDED

/*
 * Providers act a role of bridge connecting resources (meshes, images or
 * suchlike) and render assets (textures, buffers, or even shaders). Usually
 * it is has the same lifetime as the resource, or even a member of resource.
 *
 * For data that has been fetched from data structures and stored in GPU
 * resources, we call it PROVIDED by providers.
 */

#include <functional>

#include "shading.h"
#include "traits.h"

namespace shrtool {

template<typename InputType, typename OutputType>
struct provider { };

template<typename tag>
struct attr_provider_updater { };

template<>
struct attr_provider_updater<raw_data_tag> {
    typedef vertex_attr_vector output_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    template<typename input_type,
        typename Trait = attr_trait<input_type>>
    static void update(input_type& i, output_type_ptr& o, bool anew) {
        if(anew) {
            o->primitives_count(Trait::count(i));
            for(size_t s_i = 0; ; ++s_i) {
                int s = Trait::slot(i, s_i);
                if(s < 0) break;
                auto b = o->share_input(s);
                b->write(Trait::data(i, s_i));
                o->updated(s);
            }
        }
    }
};

template<>
struct attr_provider_updater<indirect_tag> {
    typedef vertex_attr_vector output_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    template<typename input_type,
        typename Trait = attr_trait<input_type>>
    static void update(input_type& i, output_type_ptr& o, bool anew) {
        if(anew) {
            o->primitives_count(Trait::count(i));
            for(size_t s_i = 0; ; ++s_i) {
                int s = Trait::slot(i, s_i);
                if(s < 0) break;
                auto b = o->share_input(s);
                typename Trait::elem_type* p =
                    b->start_map<typename Trait::elem_type>(
                            render_assets::buffer::WRITE);
                Trait::copy(i, s_i, p);
                b->stop_map();
                o->updated(s);
            }
        }
    }
};

template<typename InputType>
struct provider<InputType, vertex_attr_vector> {
    typedef vertex_attr_vector output_type;
    typedef InputType input_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    template<typename Trait = attr_trait<input_type>>
    static output_type_ptr load(input_type& i) {
        output_type_ptr p(new output_type);
        for(size_t s_i = 0; ; ++s_i) {
            int s = Trait::slot(i, s_i);
            if(s < 0) break;
            p->add_input(s, Trait::dim(i, s_i) *
                    Trait::count(i) * sizeof(typename Trait::elem_type));
        }
        update(i, p, true);
        return p;
    }

    static void update(input_type& i, output_type_ptr& o, bool anew) {
        attr_provider_updater<
            typename attr_trait<input_type>::transfer_tag>
            ::update(i, o, anew);
    }

};

template<typename tag>
struct prop_provider_updater { };

template<>
struct prop_provider_updater<raw_data_tag> {
    typedef render_assets::property_buffer output_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    template<typename input_type,
        typename Trait = prop_trait<input_type>>
    static void update(input_type& i, output_type_ptr& o, bool anew) {
        if(anew) {
            if(o->size() && o->size() < Trait::size(i))
                o.reset(new output_type);
            o->write(Trait::data(i), Trait::size(i));
        }
    }
};

// prop_data_type provides a correct pointer type
template<typename T, typename T2 = int>
constexpr void* prop_data_type_(T2) { return nullptr; }
template<typename T>
constexpr typename T::value_type* prop_data_type_(int) { return nullptr; }

template<>
struct prop_provider_updater<indirect_tag> {
    typedef render_assets::property_buffer output_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    template<typename input_type,
        typename Trait = prop_trait<input_type>>
    static void update(input_type& i, output_type_ptr& o, bool anew) {
        if(anew) {
            if(o->size() && o->size() < Trait::size(i))
                o.reset(new output_type);
            void* p = o->start_map(render_assets::buffer::WRITE,
                    Trait::size(i));
            Trait::copy(i, reinterpret_cast<
                    decltype(prop_data_type_<Trait>(0))>(p));
            o->stop_map();
        }
    }
};

template<typename InputType>
struct provider<InputType, render_assets::property_buffer> {
    typedef render_assets::property_buffer output_type;
    typedef InputType input_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    static output_type_ptr load(input_type& i) {
        output_type_ptr p(new output_type);
        update(i, p, true);
        return p;
    }

    template<typename Trait = prop_trait<input_type>>
    static void update(input_type& i, output_type_ptr& o, bool anew) {
        prop_provider_updater<typename Trait::transfer_tag>
            ::update(i, o, anew);
    }
};

template<typename InputType>
struct provider<InputType, render_assets::texture2d> {
    typedef render_assets::texture2d output_type;
    typedef InputType input_type;
    typedef std::unique_ptr<output_type> output_type_ptr;

    template<typename Trait = texture2d_trait<input_type>>
    static output_type_ptr load(input_type& i) {
        output_type_ptr p(new output_type(Trait::width(i), Trait::height(i)));
        update(i, p, true);
        return p;
    }

    template<typename Trait = texture2d_trait<input_type>>
    static void update(input_type& i, output_type_ptr& p, bool renew) {
        p->fill(Trait::data(i), Trait::format(i));
    }
};

}

#endif // PROVIDERS_H_INCLUDED

