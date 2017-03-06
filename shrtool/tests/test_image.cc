#include "common/unit_test.h"
#include "common/image.h"

using namespace std;
using namespace shrtool;
using namespace shrtool::unit_test;

TEST_CASE(test_image_interface) {
    image im;

    assert_except(im.data(), restriction_error);

    im = image(2, 2);
    im.pixel(4, 5) = color(0xff567890);

    assert_equal_print(string(im.pixel(4, 5)), "#ff567890");
}

TEST_CASE(test_plain_ppm) {
    const char* s = R"EOF(P3
    # This is an example from official site :)
    4 4
    15
     0  0  0    0  0  0    0  0  0   15  0 15
     0  0  0    0 15  7    0  0  0    0  0  0
     0  0  0    0  0  0    0 15  7    0  0  0
    15  0 15    0  0  0    0  0  0    0  0  0
    )EOF";

    stringstream ss(s);

    image im = image_io_netpbm::load(ss);

    assert_equal_print(im.width(), 4u);
    assert_equal_print(im.height(), 4u);
    assert_equal_print(im.pixel(1, 1), color(0xff77ff00));
    assert_equal_print(im.pixel(2, 2), color(0xff77ff00));
}

TEST_CASE(test_plain_invalid) {
    stringstream ss;

    const char* s1 = R"EOF(P3
    4 4
    15
     0  0  0    0  0  0    0  0  0   15  0 15
     0  0  0    0 15  7    0  0  0    0  0  0
    )EOF";

    const char* s2 = R"EOF(P2
    2 2
    15
     0  0  0    0  0  0
     0  0  0    0 15  7
    )EOF";

    ss = stringstream(s1);
    assert_except(image_io_netpbm::load(ss), parse_error);

    ss = stringstream(s2);
    assert_except(image_io_netpbm::load(ss), unsupported_error);
}


TEST_CASE(test_binary_ppm) {
    char s[] = "P6\n"
    "# This is an example from official site :)\n"
    "4 4\n"
    "15\n"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0f\x00\x0f"
    "\x00\x00\x00\x00\x0f\x07\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x0f\x07\x00\x00\x00"
    "\x0f\x00\x0f\x00\x00\x00\x00\x00\x00\x00\x00\x00";

    stringstream ss(string(s, sizeof(s)));
    image im = image_io_netpbm::load(ss);

    assert_equal_print(im.width(), 4u);
    assert_equal_print(im.height(), 4u);
    assert_equal_print(im.pixel(1, 1), color(0xff77ff00));
    assert_equal_print(im.pixel(2, 2), color(0xff77ff00));
}

TEST_CASE(test_binary_invalid) {
    stringstream ss;

    char s1[] = "P6\n4 4\n15\n"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0f\x00\x0f"
    "\x00\x00\x00\x00\x0f\x07\x00\x00\x00\x00\x00\x00";

    char s2[] = "P6\n2 2\nff\n"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0f\x00\x0f";

    ss = stringstream(string(s1, sizeof(s1)));
    assert_except(image_io_netpbm::load(ss), parse_error);

    ss = stringstream(string(s2, sizeof(s2)));
    assert_except(image_io_netpbm::load(ss), parse_error);
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv);
}

