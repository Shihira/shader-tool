#include "unit_test.h"
#include "properties.h"

using namespace std;
using namespace shrtool;
using namespace shrtool::unit_test;

/*
 * What it does:
 *
 *  trim_str(" a b    cde\t f") == "a b cde f"
 *  trim_str("a\tb  cde\n f      ") == "a b cde f"
 */
string trim_str(const string& s)
{
    string res;

    bool first_space = true;
    for(char c : s) {
        if(isspace(c)) {
            if(first_space) {
                res.push_back(' ');
                first_space = false;
                continue;
            } else
                continue;
        }

        first_space = true;
        res.push_back(c);
    }

    auto beg = res.begin() + ((res.front() == ' ') ? 1 : 0),
         end = res.end() - ((res.back() == ' ') ? 1 : 0);

    return string(beg, end);
}

TEST_CASE(test_item_trait)
{
    assert_equal(item_trait<math::fcol3>::size(), 3 * sizeof(float));
    assert_equal(item_trait<math::col4>::size(), 4 * sizeof(double));
    assert_equal(item_trait<math::col4>::glsl_type_name(), string("dvec4"));
}

TEST_CASE(test_universal_property_single_item)
{
    universal_property<math::fcol3> up_1(math::fcol3{1, 2, 3});
    assert_equal(item_get<0>(up_1), (math::fcol3{1, 2, 3}));

    item_get<0>(up_1)[0] = 5;
    item_get<0>(up_1)[1] = 6;

    assert_equal_print(item_get<0>(up_1), (math::fcol3{5, 6, 3}));
    assert_equal_print(item_offset<0>(up_1), 0UL);
    assert_equal_print(property_size(up_1), 3 * sizeof(float));

    assert_equal_print(
        trim_str(property_glsl_definition(
                up_1, "blockName", "diffuseColor")),
        trim_str("uniform blockName { vec3 diffuseColor; };"));
}

TEST_CASE(test_universal_property_multiple)
{
    universal_property<
        math::col4,
        math::col2,
        math::col3,
        char,
        int> up_1;

    constexpr size_t
        szd = 8, szf = 4,
        szi = 4, szb = 1;

    assert_equal_print(item_offset<0>(up_1), 0UL);
    assert_equal_print(item_offset<1>(up_1), 4 * szd);
    assert_equal_print(item_offset<2>(up_1), 8 * szd);
    assert_equal_print(item_offset<3>(up_1), 11 * szd);
    assert_equal_print(item_offset<4>(up_1), 11 * szd + szi);

    universal_property<
        math::matrix<float, 4, 3>,
        math::fcol3, int,
        math::fcol3, char, char, char,
        math::col3,
        double> up_2;

    assert_equal_print(item_offset<1>(up_2), 12 * szf);
    assert_equal_print(item_offset<2>(up_2), 12 * szf + 3 * szf);
    assert_equal_print(item_offset<3>(up_2), 16 * szf);
    assert_equal_print(item_offset<5>(up_2), 19 * szf + szb);
    assert_equal_print(item_offset<7>(up_2), 3 * 4 * szd);
    assert_equal_print(item_offset<8>(up_2), 3 * 4 * szd + 3 * szd);

    assert_equal_print(
        trim_str(property_glsl_definition(
            up_2, "fuckingBlock",
            "mat_abc", "damn", "hell_int",
            "kuso", "shit1", "shit2", "shit3",
            "jim", "jam")
        ),

        trim_str(R"EOF(
            uniform fuckingBlock {
                mat4x3 mat_abc;
                vec3 damn;
                int hell_int;
                vec3 kuso;
                byte shit1;
                byte shit2;
                byte shit3;
                dvec3 jim;
                double jam; };)EOF"));
}

TEST_CASE(test_dynamic_property) {
    dynamic_property dp;

    dp.get<math::matrix<float, 4, 3>>(0);
    dp.get<math::fcol3>(1);
    dp.get<int>(2);
    dp.get<math::fcol3>(3);
    dp.get<char>(4) = 'a';
    dp.get<char>(5) = 'b';
    dp.get<char>(6);
    dp.get<math::col3>(7);
    dp.get<double>(8) = 19.879;

    assert_equal_print(dp.size(), 9U);
    assert_equal_print(dp.size_in_bytes(), 128U);

    assert_equal_print(
        trim_str(dp.definition(
            "fuckingBlock", {
            "mat_abc", "damn", "hell_int",
            "kuso", "shit1", "shit2", "shit3",
            "jim", "jam" })),

        trim_str(R"EOF(
            uniform fuckingBlock {
                mat4x3 mat_abc;
                vec3 damn;
                int hell_int;
                vec3 kuso;
                byte shit1;
                byte shit2;
                byte shit3;
                dvec3 jim;
                double jam;
            };)EOF"));

    dp.type_hint_enabled(true);
    assert_equal_print(dp.get<char>(4), 'a');
    assert_equal_print(dp.get<char>(5), 'b');
    assert_equal_print(dp.get<double>(8), 19.879);
    assert_except(dp.get<char>(8), type_matching_error);
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv);
}
