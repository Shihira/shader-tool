#include <limits>
#include <iomanip>
#include <sstream>

#include "image.h"
#include "exception.h"

namespace shrtool {

color::operator std::string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const color& c) {
    std::ios::iostate s = os.rdstate();
    os << '#' << std::setw(8)
        << std::setfill('0')
        << std::hex << c.data.rgba;
    os.setstate(s);
    return os;
}

color* image::lazy_data_() const
{
    if(!data_) {
        if(!width_ || !height_)
            throw restriction_error("Zero size image");
        data_ = new color[width_ * height_];
    }
    return data_;
}

static void load_netpbm_body_plain(std::istream& is, image& im, size_t space)
{
    for(auto i = im.begin(); i != im.end(); ++i) {
        uint16_t comp;
        for(size_t c = 0; c < 3; ++c) {
            is >> comp >> std::ws;

            if(is.fail()) {
                if(is.eof()) throw parse_error(
                        "EOF too early while reading image");
                else throw parse_error("Bad Netpbm image: body");
            }

            uint8_t comp_byte;
            if(space > 255)
                comp_byte = comp / ((space + 1) / 256);
            else if(space < 255)
                comp_byte = comp * 256 / (space + 1) + comp;
            i->data.bytes[c] = comp_byte;
        }
    }
}

template<typename CompT>
static void load_netpbm_body_raw(std::istream& is, image& im, size_t space)
{
    for(auto i = im.begin(); i != im.end(); ++i) {
        CompT comp;
        for(size_t c = 0; c < 3; c++) {
            is.read((char*)(&comp), sizeof(comp));

            if(is.fail()) {
                if(is.eof()) throw parse_error(
                        "EOF too early while reading image");
                else throw parse_error("Bad Netpbm image: body");
            }

            uint8_t comp_byte;
            if(space > 255)
                comp_byte = comp / ((space + 1) / 256);
            else if(space < 255)
                comp_byte = comp * 256 / (space + 1) + comp;
            i->data.bytes[c] = comp_byte;
        }
    }
}

static std::istream& load_netpbm_consume_comment(std::istream& is)
{
    while(is.peek() == '#') {
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        is >> std::ws;
    }

    return is;
}

void image_io_netpbm::load_into_image(std::istream& is, image& im)
{
    char P = 0; char num = 0;
    is >> P >> num >> std::ws;
    if(P != 'P' || !std::isdigit(num)) {
        throw unsupported_error("Bad Netpbm image: magic mumber");
    }


    size_t w = 0, h = 0, space = 0; char whsc = 0;
    is  >> load_netpbm_consume_comment >> w
        >> load_netpbm_consume_comment >> h
        >> load_netpbm_consume_comment >> space;

    is.get(whsc); // spec: exact single whitespace
    if(!w || !h || !space || !isspace(whsc))
        throw parse_error("Bad Netpbm image: properties");

    image_geometry_helper__::width(im) = w;
    image_geometry_helper__::height(im) = h;

    if(num == '3')
        load_netpbm_body_plain(is, im, space);
    else if(num == '6') {
        if(space > 255)
            load_netpbm_body_raw<uint16_t>(is, im, space);
        else
            load_netpbm_body_raw<uint8_t>(is, im, space);
    } else
        throw unsupported_error("Format other than PPM is unsupported");
}

}
