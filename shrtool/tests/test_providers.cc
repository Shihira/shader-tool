#include <vector>

#define EXPOSE_EXCEPTION
#include "test_utils.h"
#include "providers.h"

using namespace shrtool;
using namespace shrtool::render_assets;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

struct attr_data_1 {
    vector<double> some_data_1;
    vector<int> some_data_2;
};

namespace shrtool {

template<>
struct attr_trait<attr_data_1> {
    typedef attr_data_1 input_type;
    typedef float elem_type;
    typedef indirect_tag transfer_tag;
    
    static int slot(const input_type& i, size_t i_s) {
        static const int slots[] = { 0, 2, -1 };
        return slots[i_s];
    }

    static size_t count(const input_type& i) {
        return i.some_data_1.size() / 3;
    }

    static size_t dim(const input_type& i, int i_s) {
        static const int dims[] = { 3, 2 };
        return dims[i_s];
    }

    // For indirect
    static void copy(const input_type& i, size_t i_s, elem_type* data) {
        if(i_s == 0)
            std::copy(i.some_data_1.begin(), i.some_data_1.end(), data);

        if(i_s == 1)
            std::copy(i.some_data_2.begin(), i.some_data_2.end(), data);
    }
};

}

TEST_CASE(test_attr_indirect_provider) {
    attr_data_1 d;
    d.some_data_1 = vector<double> {
        37, 17, 29, 33, 17, 40,
        19, 48, 38, 48, 49, 44,
    };
    d.some_data_2 = vector<int> {
        47, 12, 25, 26,
        34, 14, 40, 17,
    };

    typedef provider<attr_data_1, vertex_attr_vector> prov;

    auto p = prov::load(d);
    float* buf1 = p->share_input(0)->start_map<float>(buffer::READ);
    float* buf2 = p->share_input(2)->start_map<float>(buffer::READ);

    assert_true(std::equal(d.some_data_1.begin(), d.some_data_1.end(), buf1));
    assert_true(std::equal(d.some_data_2.begin(), d.some_data_2.end(), buf2));

    p->share_input(0)->stop_map();
    p->share_input(2)->stop_map();
}

////////////////////////////////////////////////////////////////////////////////

struct attr_data_2 {
    vector<uint32_t> some_data;
};

namespace shrtool {

template<>
struct attr_trait<attr_data_2> {
    typedef attr_data_2 input_type;
    typedef uint32_t elem_type;
    typedef raw_data_tag transfer_tag;
    
    static int slot(const input_type& i, size_t i_s) {
        static const int slots[] = { 3, -1 };
        return slots[i_s];
    }

    static size_t count(const input_type& i) {
        return i.some_data.size() / 3;
    }

    static size_t dim(const input_type& i, int i_s) {
        static const int dims[] = { 3, 2 };
        return dims[i_s];
    }

    // For indirect
    static elem_type const* data(const input_type& i, size_t i_s) {
        if(i_s == 0)
            return i.some_data.data();
        return nullptr;
    }
};

}

TEST_CASE(test_attr_raw_data_provider) {
    attr_data_2 d;
    d.some_data = vector<uint32_t> {
        37, 17, 29, 33, 17, 40,
        19, 48, 38, 48, 49, 44,
    };

    typedef provider<attr_data_2, vertex_attr_vector> prov;

    auto p = prov::load(d);
    vector<uint32_t> read_data(d.some_data.size());
    p->share_input(3)->read(read_data.data());

    assert_true(std::equal(d.some_data.begin(),
                d.some_data.end(), read_data.begin()));
}

////////////////////////////////////////////////////////////////////////////////

struct prop_data_1 {
    vector<uint32_t> data;
};

namespace shrtool {

template<>
struct prop_trait<prop_data_1> {
    typedef prop_data_1 input_type;
    typedef indirect_tag transfer_tag;
    typedef uint32_t value_type;

    static size_t size(const input_type& i) {
        return i.data.size();
    }

    static void copy(const input_type& i, uint32_t* o) {
        std::copy(i.data.begin(), i.data.end(), o);
    }
};

}


TEST_CASE(test_prop_indirect_provider) {
    prop_data_1 d;
    d.data = vector<uint32_t> { 37, 17, 29, 33, 17, 40, };

    typedef provider<prop_data_1, property_buffer> prov;

    auto p = prov::load(d);

    uint32_t* buf = (uint32_t*) p->start_map(buffer::READ);
    assert_true(std::equal(d.data.begin(), d.data.end(), buf));
    p->stop_map();
}

////////////////////////////////////////////////////////////////////////////////

struct prop_data_2 {
    vector<uint32_t> data;
};

namespace shrtool {

template<>
struct prop_trait<prop_data_2> {
    typedef prop_data_2 input_type;
    typedef raw_data_tag transfer_tag;
    typedef uint32_t value_type;

    static size_t size(const input_type& i) {
        return i.data.size();
    }

    static uint32_t const* data(const input_type& i) {
        return i.data.data();
    }
};

}

TEST_CASE(test_prop_raw_data_provider) {
    prop_data_2 d;
    d.data = vector<uint32_t> { 37, 17, 29, 33, 17, 40, };

    typedef provider<prop_data_2, property_buffer> prov;

    auto p = prov::load(d);

    vector<uint32_t> read_data(d.data.size());
    p->read_raw(read_data.data());
    assert_true(std::equal(d.data.begin(), d.data.end(), read_data.data()));
}

int main(int argc, char* argv[])
{
    gui_test_context::init("330 core", "");
    return unit_test::test_main(argc, argv);
}
