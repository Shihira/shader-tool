#ifndef UNIT_TEST_H_INCLUDED
#define UNIT_TEST_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <sstream>

#include "singleton.h"
#include "exception.h"

#ifndef TEST_SUITE
#define TEST_SUITE "Default"
#endif

namespace shrtool {

namespace unit_test {

class test_case {
public:
    typedef std::function<void()> runner_type;

    runner_type runner;
    std::string name;

    bool run() {
        if(!runner)
            return false;

        runner();
        return true;
    }

    test_case(
            const std::string& nm,
            runner_type rnr) :
        runner(rnr), name(nm) { }

    test_case(test_case&& tc) :
        runner(std::move(tc.runner)), name(std::move(tc.name)) { }
};

class test_suite {
    std::vector<test_case> test_cases_;

public:
    std::string name;

    test_suite(const std::string& nm = "unnamed") :
        name(nm) { }

    void add_test_case(test_case&& tc) {
        test_cases_.emplace_back(std::move(tc));
    }

    inline bool test_all();
};

class test_context : public generic_singleton<test_context> {
    std::map<std::string, test_suite> suites_;
    bool stop_on_failure_ = false;

public:
    std::stringstream ctest;
    std::stringstream full_log;

    static void stop_on_failure(bool b) { inst().stop_on_failure_ = b; }
    static bool stop_on_failure() { return inst().stop_on_failure_; }

    static void add_test_case(
            const std::string& test_suite_name,
            test_case&& tc) {
        auto& suites = inst().suites_;
        auto i = suites.find(test_suite_name);
        if(i == suites.end()) {
            suites.emplace(std::make_pair(
                    test_suite_name, test_suite(test_suite_name)));
            i = suites.find(test_suite_name);
        }

        i->second.add_test_case(std::move(tc));
    }

    static bool test_all() {
        bool state = true;
        for(auto& e : inst().suites_) {
            state &= e.second.test_all();

            if(stop_on_failure() && !state)
                break;
        }

        return state;
    }

    static void commit_log(const std::string name) {
        inst().full_log
            << "\033[1m----- " << name << " -----\033[0m\n"
            << inst().ctest.str();

        inst().ctest.str("");
    }
};

#define ctest (shrtool::unit_test::test_context::inst().ctest)

struct test_case_adder__ {
    test_case_adder__(
            const std::string& test_suite_name,
            const std::string& test_case_name,
            test_case::runner_type fn) {
        test_context::add_test_case(
                test_suite_name,
                test_case(test_case_name, fn));
    }
};

inline bool test_suite::test_all() {
    bool state = true;
    for(auto& tc : test_cases_) {
        std::cout << "Running " << tc.name << "..." << std::flush;

        try {
            state &= tc.run();
            std::cout << "\033[1;32mPassed\033[0m" << std::endl;
        } catch(assert_error e) {
            std::cout << "\033[1;33mFailure\033[0m: "
                << e.what() << std::endl;
            state &= false;
        } catch(std::exception e) {
            std::cout << "\033[1;31mError\033[0m: "
                << e.what() << std::endl;
            state &= false;
        }

        if(!ctest.str().empty())
            test_context::commit_log(name + "/" + tc.name);

        if(test_context::stop_on_failure() && !state)
            break;
    }
    return state;
}

}

}

#define TEST_CASE(func_name) \
    void func_name(); \
    shrtool::unit_test::test_case_adder__ test_case_adder_##func_name##__( \
        TEST_SUITE, #func_name, func_name); \
    void func_name()

#define TEST_CASE_FIXTURE(func_name, fix_name) \
    struct test_case_##func_name##_fixture__ : fix_name { \
        inline void operator() (); }; \
    shrtool::unit_test::test_case_adder__ test_case_adder_##func_name##__( \
        TEST_SUITE, #func_name, [](){ test_case_##func_name##_fixture__ d; d(); }); \
    void test_case_##func_name##_fixture__::operator() ()

#define assert_true(expr) { \
    if(!bool(expr)) { \
        std::stringstream ss; \
        ss << #expr << " != true"; \
        throw assert_error(ss.str()); \
    } \
}

#define assert_false(expr) { \
    if(bool(expr)) { \
        std::stringstream ss; \
        ss << #expr << " != false"; \
        throw assert_error(ss.str()); \
    } \
}

#define assert_equal(expr1, expr2) { \
    if((expr1) == (expr2)) { } \
    else { \
        std::stringstream ss; \
        ss << #expr1 << " != " << #expr2; \
        throw assert_error(ss.str()); \
    } \
}

#define assert_equal_print(expr1, expr2) { \
    auto val1 = (expr1); \
    auto val2 = (expr2); \
    if(val1 == val2) { } \
    else { \
        std::stringstream ss; \
        ss << #expr1 << "(" << val1 << ")" << " != " \
            << #expr2 << "(" << val2 << ")"; \
        throw assert_error(ss.str()); \
    } \
}

#define assert_float_close(expr1, expr2, bias) { \
    auto val1 = (expr1); \
    auto val2 = (expr2); \
    double diff = val1 - val2; \
    if(diff < 0) diff = -diff; \
    if(diff > bias) { \
        std::stringstream ss; \
        ss << expr1 << "(" << val1 << ")" << " !~ " \
            << expr2 << "(" << val2 << ")"; \
        throw assert_error(ss.str()); \
    } \
}

#define assert_float_equal(expr1, expr2) \
    assert_float_close(expr1, expr2, 0.00001)

#endif // UNIT_TEST_H_INCLUDED

