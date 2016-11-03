#ifndef RES_TRAIT_INCLUDED
#define RES_TRAIT_INCLUDED

namespace shrtool {

struct raw_data_tag { };
struct indirect_tag { };

template<typename InputType, typename Enable = void>
struct attr_trait {
    /*
     * Please provide:
     */

    // typedef InputType input_type;
    // typedef shrtool::indirect_tag transfer_tag;
    // typedef float elem_type;
    //
    // static int slot(const input_type& i, size_t i_s);
    // static int count(const input_type& i);
    // static int dim(const input_type& i, size_t i_s);

    /*
     * For raw_data
     */
    // static elem_type const* data(const input_type& i, size_t i_s);
    /*
     * For indirect
     */
    // static void copy(const input_type& i, size_t i_s, elem_type* data);
};

template<typename InputType, typename Enable = void>
struct prop_trait {
    /*
     * Please provide:
     */

    //typedef InputType input_type;
    //typedef shrtool::indirect_tag transfer_tag;
    //typedef float value_type;

    // static size_t size(const input_type& i);
    /*
     * For raw_data
     */
    // static elem_type const* data(const input_type& i);
    /*
     * For indirect
     */
    // static void copy(const input_type& i, float* o);
};

template<typename InputType, typename Enable = void>
struct texture2d_trait {
    // typedef InputType input_type;

    // static size_t width(const input_type& i) {
    //     return i.width();
    // }

    // static size_t height(const input_type& i) {
    //     return i.height();
    // }

    // static shrtool::render_assets::texture2d::format
    // format(const input_type& i) {
    //     return shrtool::render_assets::texture2d::RGBA_U8888;
    // }

    // static void* data(const input_type& i) {
    //     return i.data();
    // }
};

}

#endif // RES_TRAIT_INCLUDED

