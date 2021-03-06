#define TEST_SUITE "test_matrix"

#include "common/unit_test.h"
#include "common/matrix.h"

/*
 * TIPS:
 *
 * You can always run this piece of code to generate a random matrix:
 *
 *     print(
 *         ";\n".join([
 *             ", ".join([
 *                 str(random.randint(0, 100))
 *                 for j in range(N)])
 *             for i in range(N)])
 *         )
 */

using namespace shrtool;
using namespace shrtool::math;
using namespace std;

template<int size>
array<int, size> int_range(int first, int last, int step = 1) {
    array<int, size> arr;

    for(int i = 0, n = first; n <= last && i < size; ++i, n += step)
        arr[i] = n;

    return arr;
}

template<typename T1, typename T2, typename Func>
void for_each_2(T1& t1, T2& t2, Func f) {
    auto t1_i = t1.begin();
    auto t2_i = t2.begin();

    for(; t1_i != t1.end() && t2_i != t2.end(); t1_i++, t2_i++)
        f(*t1_i, *t2_i);
}

template<typename IterType>
struct iterable_wrapper {
    IterType beg_;
    IterType end_;

    IterType begin() { return beg_; }
    IterType end() { return end_; }
};

////////////////////////////////////////////////////////////////////////////////
// step iterator tests

struct si_fixture {
    template<typename IterType, size_t Step>
    using step_iterator = detail::step_iterator<IterType, Step>;

    static constexpr int step = 3;
    static constexpr int size = 10;
    static constexpr int max_val = step * (size - 1);

    typedef array<int, size> vector_type;

    typedef step_iterator<vector_type::iterator, step> iterator;
    typedef step_iterator<vector_type::const_iterator, step> const_iterator;

    array<int, size * step> int_list;
    vector_type expected;

    si_fixture() :
        int_list(int_range<size * step>(0, max_val)),
        expected(int_range<size>(0, max_val, step))
    { }

    iterator begin()
        { return iterator(int_list.begin()); }
    iterator end()
        { return iterator(int_list.begin() + step * size); }
};

TEST_CASE_FIXTURE(si_iterable, si_fixture)
{
    for_each_2(*this, expected, equal_to<int>());
}

TEST_CASE_FIXTURE(si_const_assignment, si_fixture)
{
    auto const_this = iterable_wrapper<const_iterator>
        { begin(), end() };

    for_each_2(const_this, expected, equal_to<int>());
}

////////////////////////////////////////////////////////////////////////////////
// vector reference tests

struct vr_fixture {
    template<typename IterType, size_t Step>
    using step_iterator = detail::step_iterator<IterType, Step>;

    static constexpr int step = 3;
    static constexpr int size = 10;
    static constexpr int max_val = step * (size - 1);

    typedef array<int, size> vector_type;
    typedef vector_ref<vector_type::const_iterator, vector_type> const_vr;
    typedef vector_ref<vector_type::iterator, vector_type> mutable_vr;

    typedef step_iterator<array<int, size * step>::iterator, step> org_iter;
    typedef step_iterator<array<int, size * step>::const_iterator, step> const_org_iter;

    array<int, size * step> org;
    const vector_type expected;
    vector_ref<org_iter, vector_type> org_vr;

    template<typename T>
    const_vr make_const_vr(const T& t) {
        return const_vr(t.begin(), t.end());
    }

    template<typename T>
    mutable_vr make_mutable_vr(T& t) {
        return mutable_vr(t.begin(), t.end());
    }

    vr_fixture() :
        org(int_range<size * step>(0, max_val)),
        expected(int_range<10>(0, max_val, step)),
        org_vr(org_iter(org.begin()), org_iter(org.begin()) + size) { }
};

TEST_CASE_FIXTURE(vr_equality, vr_fixture)
{
    const_vr cmp_vr = make_const_vr(expected);

    assert_equal(cmp_vr, org_vr);
    assert_equal(org_vr, expected);
    assert_false(org_vr != cmp_vr);
}

TEST_CASE_FIXTURE(vr_const_assignment, vr_fixture)
{
    const_vr cmp_vr = make_const_vr(expected);
    vector_ref<const_org_iter, vector_type> const_org_vr = org_vr;

    assert_equal(const_org_vr, cmp_vr);
    assert_equal(const_org_vr, org_vr);
}

TEST_CASE_FIXTURE(vr_evaluate, vr_fixture)
{
    const vector_type
        dif_list {  1,  2,  3,  4,  5,  5,  4,  3,  2,  1 },
        sub_list { -1,  1,  3,  5,  7, 10, 14, 18, 22, 26 },
        sum_list {  1,  5,  9, 13, 17, 20, 22, 24, 26, 28 },
        mul_list {  0,  6, 12, 18, 24, 30, 36, 42, 48, 54 },
        div_list {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9 };

    const_vr
        dif_vr = make_const_vr(dif_list),
        exp_vr = make_const_vr(expected),
        sub_vr = make_const_vr(sub_list);

    assert_equal(org_vr, exp_vr);
    assert_equal(org_vr + dif_vr, sum_list);
    assert_equal(org_vr - dif_vr, sub_list);
    assert_equal(org_vr * 2     , mul_list);
    assert_equal(org_vr / 3     , div_list);

    assert_equal(sub_vr, org_vr - dif_vr);
    assert_equal(org_vr * dif_vr, 405);
}

TEST_CASE_FIXTURE(vr_modify_origin, vr_fixture)
{
    const vector_type
        dif_list {  1,  2,  3,  4,  5,  5,  4,  3,  2,  1 },
        sum_list {  1,  5,  9, 13, 17, 20, 22, 24, 26, 28 },
        mul_list {  0,  6, 12, 18, 24, 30, 36, 42, 48, 54 },
        div_list {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9 };

    const_vr
        dif_vr = make_const_vr(dif_list),
        exp_vr = make_const_vr(expected);

    org_vr += dif_vr;
    assert_equal(org_vr, sum_list);

    org_vr -= dif_vr;
    assert_equal(org_vr, exp_vr);

    org_vr *= 2;
    assert_equal(org_vr, mul_list);

    org_vr = exp_vr;
    assert_equal(org_vr, exp_vr);

    org_vr /= 3;
    assert_equal(org_vr, div_list);

    org_vr = expected;
    assert_equal(org_vr, exp_vr);
}

////////////////////////////////////////////////////////////////////////////////
// matrix tests

struct mat_fixture {
    mat34 nsqr;
    const mat4 sqr;
    mat4 sqr2;
    row4 row;
    const col3 col;

    mat_fixture() :
        nsqr {
            6,  22, 14, 15,
            24, 15, 22, 8,
            29, 9,  26, 30,
        },
        sqr {
            17, 15, 5,  18,
            27, 10, 17, 27,
            22, 12, 13, 10,
            21, 8,  7,  19,
        },
        sqr2 {
            6,  22, 14, 15,
            24, 15, 22, 8,
            29, 9,  26, 30,
            21, 8,  7,  19,
        },
        row {
            13, 25,  8, 10
        },
        col {
            24, 2,  19
        } { }
};

TEST_CASE_FIXTURE(mat_subscript, mat_fixture) {
    assert_equal(nsqr[2][1], 9);
    assert_equal(sqr[0][3], 18);
    assert_equal(row[2], 8);
    assert_equal(col[1], 2);

    assert_equal(sqr.at(0, 3), 18);
    assert_equal(sqr.row(0)[3], 18);
    assert_equal(sqr.col(3)[0], 18);

    assert_equal(sqr.row(3) * sqr.col(0), 1126);
}

TEST_CASE_FIXTURE(mat_multiply, mat_fixture) {
    assert_equal(nsqr * sqr, mat34({
        1319,        598,        691,       1127,
        1465,        838,        717,       1209,
        1938,       1077,        846,       1595,
    }));

    assert_equal(sqr2 * sqr, mat4({
        1319,        598,        691,       1127,
        1465,        838,        717,       1209,
        1938,       1077,        846,       1595,
        1126,        631,        465,       1025,
    }));

    sqr2 *= sqr;
    assert_equal(sqr2, mat4({
        1319,        598,        691,       1127,
        1465,        838,        717,       1209,
        1938,       1077,        846,       1595,
        1126,        631,        465,       1025,
    }));
}

TEST_CASE_FIXTURE(mat_vector_modify_origin, mat_fixture) {
    sqr2[1] -= sqr2[0] * 4;
    sqr2[2] += -sqr2[0] * (29. / 6);
    sqr2[3] -= sqr2[0] * (21. / 6);

    for(size_t i = 1; i < 4; i++)
        assert_float_equal(sqr2[i][0], 0);
}

TEST_CASE_FIXTURE(mat_vector_operations, mat_fixture) {
    assert_float_equal(det(sqr), 18564);
    assert_float_equal(det(sqr2), -141055);
    assert_equal_print(det(matrix<long long, 4, 4>(sqr)), 18564);

    assert_float_equal(norm(col), sqrt(941));
    assert_float_equal(norm(sqr.col(0)), sqrt(1943));
}

TEST_CASE_FIXTURE(mat_print, mat_fixture) {
    ctest <<
        nsqr << endl <<
        sqr << endl <<
        sqr2 << endl <<
        row << endl <<
        col << endl <<
        nsqr[2] << endl <<
        sqr2[0] << endl <<
        sqr.col(3) << endl;
}

TEST_CASE(mat_determinant) {
    mat4 dat1 = {
        82, 71, 21, 32,
        47, 88, 95, 44,
        91, 55, 77, 31,
        14, 36, 17, 71,
    };

    matrix<double, 6, 6> dat2 = {
        7, 23, 0, 97, 57, 92,
        35, 47, 95, 81, 77, 26,
        73, 90, 65, 39, 65, 9,
        64, 90, 95, 51, 20, 35,
        5, 82, 75, 33, 7, 39,
        23, 68, 64, 49, 83, 85
    };

    assert_equal_print(det(dat1), 20390716);
    assert_float_equal(det(dat2) / 1e+11, -1.27450478636);
}

TEST_CASE(mat_inverse) {
    mat3
        dat1 = {
            1, 2, 3,
            3, 2, 1, 
            0, 6, 0,
        },
        res1 = inverse(dat1),
        ans1 = {
           -0.12500,  0.37500, -0.08333,
           -0.00000,  0.00000,  0.16667,
            0.37500, -0.12500, -0.08333,
        };

    mat4
        dat2 = {
            1, 2, 3, 4,
            3, 2, 1, 9,
            0, 6, 0, 7,
            11, 10, 20, 30,
        },
        res2 = inverse(dat2),
        ans2 = {
            -5.307692308, -1.0,  0.6923076923,  8.461538462e-01,
            -2.638461538, -0.7,  0.5615384615,  4.307692308e-01,
             0.846153846, -0.0, -0.1538461538, -7.692307692e-02,
             2.261538462,  0.6, -0.3384615385, -3.692307692e-01,
        };

    assert_true(ans1.close(res1, 0.0001));
    assert_true(ans2.close(res2, 0.000001));
}

TEST_CASE(vec_multiply) {
    col4 col_vec_1 = { 1, 2, 3, 4 };
    row4 row_vec_1 = { 4, 5, 6, 7 };
    col4 col_vec_2 = { 4, 5, 6, 7 };

    assert_equal_print(dot(col_vec_1, row_vec_1), dot(col_vec_1, col_vec_2));
    assert_equal_print(dot(col_vec_1, row_vec_1), (4 + 10 + 18 + 28));

    col3 col_vec_3 = { 1, 2, 3 };
    row3 row_vec_2 = { 4, 5, 6 };
    col3 col_vec_4 = { 4, 5, 6 };

    assert_equal_print(
        cross(col_vec_3, row_vec_2),
        cross(col_vec_3, col_vec_4));
    assert_equal_print(
        cross(col_vec_3, row_vec_2),
        col3({12 - 15, 12 - 6, 5 - 8}));
    assert_equal_print(
        cross(col_vec_4, row_vec_2),
        col3({0, 0, 0}));
}

TEST_CASE(test_dynmatrix) {
    dynmatrix<double> dm(4, 4, {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 8, 7, 6,
        5, 4, 3, 2,
    });

    assert_equal_print(dm.at(1, 2), 7);
    dm.at(3, 3) = -5;
    assert_equal_print(dm.at(3, 3), -5);
}

int main(int argc, char* argv[])
{
    return unit_test::test_main(argc, argv);
}


