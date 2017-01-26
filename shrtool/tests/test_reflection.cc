#define TEST_SUITE "test_reflection"

#define EXPOSE_EXCEPTION

#include <iostream>

#include "unit_test.h"
#include "reflection.h"

using namespace shrtool;
using namespace shrtool::refl;
using namespace std;

struct static_func_class {
public:
    static int add(int a, int b) { return a + b; }
    static float addf(float a, float b) { return a + b; }
    static void swap(int& a, int& b) {
        int t = a; a = b; b = t;
    }
    static int* swap_and_max(int* a, int* b) {
        int t = *a; *a = *b; *b = t;
        return *a > *b ? a : b;
    }
    static int& max_ref(int& a, int& b) {
        return a > b ? a : b;
    }

    static void meta_reg_() {
        meta_manager::reg_class<static_func_class>("static_func_class")
            .function("add", &static_func_class::add)
            .function("addf", &static_func_class::addf)
            .function("swap", &static_func_class::swap)
            .function("max_ref", &static_func_class::max_ref)
            .function("swap_and_max", &static_func_class::swap_and_max);
    }
};

TEST_CASE(test_static_function)
{
    meta_manager::init();
    static_func_class::meta_reg_();

    meta& m = meta_manager::get_meta("static_func_class");
    instance res_1 = m.call("add", instance::make(1), instance::make(5));
    assert_equal_print(res_1.get<int>(), 6);

    instance num_1 = instance::make(12);
    instance num_2 = instance::make(56);
    instance res_2 = m.call("swap", num_1, num_2);
    assert_equal_print(num_1.get<int>(), 56);
    assert_equal_print(num_2.get<int>(), 12);
    assert_true(res_2.is_null());
}

TEST_CASE(test_conversion)
{
    meta_manager::init();
    static_func_class::meta_reg_();

    instance res_1 = instance::make(58).cast_to(
            meta_manager::get_meta<float>());
    assert_true(res_1.get_meta() == meta_manager::get_meta<float>());
    assert_float_equal(res_1.get<float>(), 58);

    meta& m = meta_manager::get_meta("static_func_class");
    instance res_2 = m.call("addf", instance::make(7), instance::make(8));
    assert_true(res_1.get_meta() == meta_manager::get_meta("float"));
    assert_float_equal(res_2.get<float>(), 15);
}

TEST_CASE(test_pointer)
{
    meta_manager::init();
    static_func_class::meta_reg_();

    meta& m = meta_manager::get_meta("static_func_class");
    int a = 12;
    int b = 56;
    instance res_1 = m.call("swap_and_max",
            instance::make(&a), instance::make(&b));
    assert_equal_print(a, 56);
    assert_equal_print(b, 12);
    assert_equal_print(&res_1.get<int>(), &a);

    int c = 100;
    int d = 200;
    instance res_2 = m.call("max_ref",
            instance::make(&c), instance::make(&d));
    assert_equal_print(res_2.get<int>(), 200);
    assert_equal_print(&res_2.get<int>(), &d);
}

TEST_CASE(test_serialization)
{
    meta_manager::init();
    static_func_class::meta_reg_();

    instance obj = instance::make(24);
    assert_except(meta_manager::get_meta<int>().call("__size"),
            restriction_error);
    instance res_1 = meta_manager::get_meta<int>().call("__size", obj);
    assert_equal_print(res_1.get<size_t>(), sizeof(int));

    int i;
    instance ptr = instance::make(&i);
    meta_manager::get_meta<int>().call("__raw_into", obj, ptr);
    assert_equal_print(i, 24);
}

struct member_func_class {
    int a = 0;
    float b = 0;
    std::string s;
public:
    member_func_class() = default;
    member_func_class(member_func_class&& o) = default;
    member_func_class(const member_func_class& o) = default;

    float multiply() { return a * b; }
    void set_a(int a_) { a = a_; }
    void set_b(float b_) { b = b_; }
    void set_s(const string& s_) { s = s_; }
    std::string get_s() { return s; }
    void set_move_s(string&& s_) { s = std::move(s_); }

    static void meta_reg_() {
        meta_manager::reg_class<member_func_class>("member_func_class")
            .enable_clone()
            .function("multiply", &member_func_class::multiply)
            .function("set_a", &member_func_class::set_a)
            .function("set_b", &member_func_class::set_b)
            .function("set_s", &member_func_class::set_s)
            .function("set_move_s", &member_func_class::set_move_s);
    }
};

TEST_CASE(test_member_function)
{
    meta_manager::init();
    member_func_class::meta_reg_();

    instance ins = instance::make(member_func_class());
    ins.get_meta().call("set_a", ins, instance::make(7));
    ins.call("set_b", instance::make(8.2f));
    instance res_1 = ins.get_meta().call("multiply", ins);
    assert_float_equal(res_1.get<float>(), 57.4f);
}

TEST_CASE(test_clone)
{
    meta_manager::init();
    member_func_class::meta_reg_();

    instance ins = instance::make(member_func_class());
    ins.call("set_a", instance::make(7));
    ins.call("set_b", instance::make(8));
    instance ins2 = ins;
    assert_equal_print(ins2.get<member_func_class>().multiply(), 56);
    assert_true(&ins.get<member_func_class>()
            != &ins2.get<member_func_class>());
}

TEST_CASE(test_move)
{
    meta_manager::init();
    member_func_class::meta_reg_();

    instance ins = instance::make(member_func_class());
    instance s1 = instance::make(string("god bless my program"));
    instance s2 = instance::make(string("hello world"));
    ins.call("set_s", s1);
    assert_equal_print(ins.get<member_func_class>().get_s(),
            "god bless my program");

    ins.call("set_move_s", s2);
    assert_true(s2.get<string>() != "hello world");
}

int main(int argc, char* argv[])
{
    return unit_test::test_main(argc, argv);
}

