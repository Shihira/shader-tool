#define TEST_SUITE "test_reflection"

#define EXPOSE_EXCEPTION

#include "unit_test.h"
#include "reflection.h"

using namespace shrtool;
using namespace shrtool::refl;
using namespace std;

struct static_func_class {
public:
    static int add(int a, int b) { return a + b; }
    static void swap(int& a, int& b) {
        int t = a; a = b; b = t;
    }

    static void meta_reg_() {
        meta_manager::reg_class<static_func_class>("static_func_class")
            .function("add", &static_func_class::add)
            .function("swap", &static_func_class::swap);
    }
};

TEST_CASE(test_static_func)
{
    meta_manager::init();
    static_func_class::meta_reg_();

    meta* m = meta_manager::find_meta("static_func_class");
    instance res_1 = m->call("add", instance::make(1), instance::make(5));
    assert_equal_print(res_1.get<int>(), 6);

    instance num_1 = instance::make(12);
    instance num_2 = instance::make(56);
    instance res_2 = m->call("swap", num_1, num_2);
    assert_equal_print(num_1.get<int>(), 56);
    assert_equal_print(num_2.get<int>(), 12);
    assert_true(res_2.is_null());
}

int main(int argc, char* argv[])
{
    return unit_test::test_main(argc, argv);
}

