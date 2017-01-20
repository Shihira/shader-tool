#include <vector>

#include "unit_test.h"
#include "res_pool.h"

using namespace std;
using namespace shrtool;
using namespace shrtool::unit_test;

TEST_CASE(test_store_data) {
    res_pool rp;

    const char* some_str = "some shit things come";

    rp.insert("holy-int", 13);
    rp.insert("holy-float", 13.45f);
    rp.insert<std::string>("shit_string", some_str + 5, some_str + 16);
    rp.insert("damn-vec", std::vector<int>{ 3, 4, 5, 6 });

    auto r1 = rp.ref<int>("holy-int");
    assert_true(!r1.expired());
    assert_equal_print(r1.lock()->data, 13);

    auto r2 = rp.ref<float>("holy-float");
    assert_true(!r2.expired());
    assert_equal_print(r2.lock()->data, 13.45f);

    auto r3 = rp.ref<std::string>("shit_string");
    assert_true(!r3.expired());
    assert_equal_print(r3.lock()->data, "shit things");

    auto r4 = rp.ref<std::vector<int>>("damn-vec");
    assert_true(!r4.expired());
    assert_equal_print(r4.lock()->data[3], 6);
    assert_equal_print(rp.get<std::vector<int>>("damn-vec")[3], 6);

    auto r5 = rp.ref("holy-int");
    assert_equal_print(r5.lock()->name, "holy-int");
    assert_equal_print(r5.lock()->type_name(), typeid(int).name());

    rp.remove("holy-int");
    assert_true(r1.expired());
    rp.remove("shit_string");
    assert_true(r3.expired());
}

TEST_CASE(test_type_hint) {
    ires_pool rp;

    rp.type_hint_enabled(true);
    rp.insert(100, 154UL);
    rp.insert(300, 3.456);
    rp.insert<const char*>(500, "good deed");

    assert_no_except(rp.ref<unsigned long>(100));
    assert_except(rp.ref<int>(100), type_matching_error);
    assert_no_except(rp.ref<double>(300));
    assert_except(rp.ref<float>(300), type_matching_error);
    assert_no_except(rp.ref<const char*>(500));
    assert_except(rp.ref<std::string>(500), type_matching_error);
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv);
}
