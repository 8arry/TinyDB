#include "value.hpp"

namespace tinydb {

std::ostream& operator<<(std::ostream& os, const Value& value) {
    // Using modern C++ std::visit and constexpr lambda
    std::visit(
        [&os](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::same_as<T, int>) {
#if HAS_FORMAT
                os << std::format("{}", val);
#else
                os << val;
#endif
            } else if constexpr (std::same_as<T, std::string>) {
#if HAS_FORMAT
                os << std::format("\"{}\"", val);
#else
                os << "\"" << val << "\"";
#endif
            }
        },
        value.data);

    return os;
}

} // namespace tinydb