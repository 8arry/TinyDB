#pragma once

#include <compare>
#include <concepts>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

// Conditional inclusion of C++23 features
#if __cplusplus >= 202302L && __has_include(<format>)
#include <format>
#define HAS_FORMAT 1
#else
#include <sstream>
#define HAS_FORMAT 0
#endif

#if __cplusplus >= 202302L && __has_include(<expected>)
#include <expected>
#define HAS_EXPECTED 1
#else
#define HAS_EXPECTED 0
#endif

namespace tinydb {

// Supported data types (meets project requirements: int and str)
enum class DataType : std::uint8_t {
    INT, // int - integer type
    STR  // str - string type
};

// C++23 concept for value types
template <typename T>
concept ValueType =
    std::same_as<std::remove_cvref_t<T>, int> ||
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, const char*> || std::same_as<T, const char*> ||
    (std::is_array_v<std::remove_reference_t<T>> &&
     std::same_as<std::remove_extent_t<std::remove_reference_t<T>>, char>);

// Type-safe value class using C++23 features
class Value {
private:
    std::variant<int, std::string> data;

public:
    // Using C++23 deducing this feature and concepts
    Value() : data(0) {
    }

    template <ValueType T> explicit Value(T&& value) : data(std::forward<T>(value)) {
    }

    // C++23 string literal constructor
    Value(const char* value) : data(std::string(value)) {
    }

    // Get type (using C++23 if constexpr and visitor pattern)
    DataType getType() const noexcept {
        return std::visit(
            [](const auto& value) constexpr -> DataType {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::same_as<T, int>) {
                    return DataType::INT;
                } else if constexpr (std::same_as<T, std::string>) {
                    return DataType::STR;
                }
            },
            data);
    }

#if HAS_EXPECTED
    // Using C++23 std::expected for error handling
    std::expected<int, std::string> asInt() const noexcept {
        if (auto* val = std::get_if<int>(&data)) {
            return *val;
        }
        return std::unexpected("Value is not an integer");
    }

    std::expected<std::string, std::string> asString() const noexcept {
        if (auto* val = std::get_if<std::string>(&data)) {
            return *val;
        }
        return std::unexpected("Value is not a string");
    }
#endif

    // Traditional exception throwing version (backward compatibility)
    int asIntUnsafe() const {
#if HAS_EXPECTED
        auto result = asInt();
        if (result) {
            return *result;
        }
        throw std::runtime_error(result.error());
#else
        if (auto* val = std::get_if<int>(&data)) {
            return *val;
        }
        throw std::runtime_error("Value is not an integer");
#endif
    }

    const std::string& asStringUnsafe() const {
        if (auto* val = std::get_if<std::string>(&data)) {
            return *val;
        }
        throw std::runtime_error("Value is not a string");
    }

    // Using C++23 constexpr and pattern matching style
    static constexpr Value getDefault(DataType type) {
        return [type]() constexpr -> Value {
            switch (type) {
                case DataType::INT:
                    return Value{0};
                case DataType::STR:
                    return Value{std::string{}};
            }
            __builtin_unreachable(); // GCC builtin, C++23 fallback
        }();
    }

    // Using C++23 defaulted comparison and spaceship operator
    std::strong_ordering operator<=>(const Value& other) const {
        if (getType() != other.getType()) {
            throw std::runtime_error("Cannot compare values of different types");
        }

        return std::visit(
            [&other](const auto& value) -> std::strong_ordering {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::same_as<T, int>) {
                    return value <=> std::get<int>(other.data);
                } else if constexpr (std::same_as<T, std::string>) {
                    return value <=> std::get<std::string>(other.data);
                }
                __builtin_unreachable();
            },
            data);
    }

    // Equality operator needs separate implementation as variant doesn't support defaulted
    // comparison
    bool operator==(const Value& other) const {
        return data == other.data;
    }

    // Stream output - declared as friend to access private members
    friend std::ostream& operator<<(std::ostream& os, const Value& value);

    // String representation (compatible with C++23 std::format)
    std::string toString() const {
        return std::visit(
            [](const auto& value) -> std::string {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::same_as<T, int>) {
#if HAS_FORMAT
                    return std::format("{}", value);
#else
                    return std::to_string(value);
#endif
                } else if constexpr (std::same_as<T, std::string>) {
                    return value;
                }
                __builtin_unreachable();
            },
            data);
    }

#if HAS_FORMAT
    // C++23 format support
    template <typename FormatContext> auto format(FormatContext& ctx) const {
        return std::visit(
            [&ctx](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::same_as<T, int>) {
                    return std::format_to(ctx.out(), "{}", value);
                } else if constexpr (std::same_as<T, std::string>) {
                    return std::format_to(ctx.out(), "\"{}\"", value);
                }
                __builtin_unreachable();
            },
            data);
    }
#endif
};

// C++23 column definition structure using designated initializers and defaulted comparison
struct Column {
    std::string name;
    DataType type;

    // C++23 designated initializers support
    Column(const std::string& columnName, DataType columnType)
        : name(columnName), type(columnType) {
    }

    // Using C++23 defaulted spaceship operator
    auto operator<=>(const Column& other) const = default;
};

} // namespace tinydb