#ifndef EXCEPTION_H_INCLUDED
#define EXCEPTION_H_INCLUDED

#include <stdexcept>

namespace shrtool {

class error_base : public std::exception {
public:
    std::string reason;
    error_base(const std::string& r) : reason(r) { }
    const char* what() const noexcept override { return reason.c_str(); }
};


#define DEFINE_TRIVIAL_ERROR(name) \
    class name : public error_base { \
        using error_base::error_base; \
    };

DEFINE_TRIVIAL_ERROR(assert_error)
DEFINE_TRIVIAL_ERROR(shader_error)
DEFINE_TRIVIAL_ERROR(driver_error)
DEFINE_TRIVIAL_ERROR(enum_map_error)
DEFINE_TRIVIAL_ERROR(unsupported_error)
DEFINE_TRIVIAL_ERROR(parse_error)
DEFINE_TRIVIAL_ERROR(restriction_error)
DEFINE_TRIVIAL_ERROR(resolve_error)
DEFINE_TRIVIAL_ERROR(type_matching_error)
DEFINE_TRIVIAL_ERROR(not_found_error)

}

#endif // EXCEPTION_H_INCLUDED

