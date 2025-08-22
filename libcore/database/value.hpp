#pragma once

#include <variant>
#include <string>
#include <stdexcept>
#include <iostream>
#include <concepts>
#include <utility>
#include <functional>
#include <compare>

// 条件性包含C++23特性
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

// 支持的数据类型枚举 (使用C++23 scoped enum)
enum class DataType : std::uint8_t {
    INT,
    STRING
};

// C++23 concept for value types
template<typename T>
concept ValueType = std::same_as<T, int> || 
                   std::same_as<T, std::string> || 
                   std::same_as<T, const char*> ||
                   (std::is_array_v<std::remove_reference_t<T>> && 
                    std::same_as<std::remove_extent_t<std::remove_reference_t<T>>, char>);

// 类型安全的值类，使用C++23特性
class Value {
private:
    std::variant<int, std::string> data;

public:
    // 使用C++23的deducing this特性和concepts
    Value() : data(0) {}
    
    template<ValueType T>
    explicit Value(T&& value) : data(std::forward<T>(value)) {}
    
    // C++23 string literal constructor
    Value(const char* value) : data(std::string(value)) {}

    // 获取类型 (使用C++23的if constexpr和visitor pattern)
    DataType getType() const noexcept {
        return std::visit([](const auto& value) constexpr -> DataType {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<T, int>) {
                return DataType::INT;
            } else if constexpr (std::same_as<T, std::string>) {
                return DataType::STRING;
            }
        }, data);
    }

#if HAS_EXPECTED
    // 使用C++23的std::expected进行错误处理
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

    // 传统的抛出异常版本 (向后兼容)
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

    // 使用C++23的constexpr和pattern matching风格
    static constexpr Value getDefault(DataType type) {
        return [type]() constexpr -> Value {
            switch (type) {
                case DataType::INT:
                    return Value{0};
                case DataType::STRING:
                    return Value{std::string{}};
            }
            __builtin_unreachable(); // GCC builtin, C++23 fallback
        }();
    }

    // 使用C++23的defaulted comparison和spaceship operator
    std::strong_ordering operator<=>(const Value& other) const {
        if (getType() != other.getType()) {
            throw std::runtime_error("Cannot compare values of different types");
        }
        
        return std::visit([&other](const auto& value) -> std::strong_ordering {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<T, int>) {
                return value <=> std::get<int>(other.data);
            } else if constexpr (std::same_as<T, std::string>) {
                return value <=> std::get<std::string>(other.data);
            }
            __builtin_unreachable();
        }, data);
    }

    // 等于操作符需要单独实现，因为variant不支持defaulted comparison
    bool operator==(const Value& other) const {
        return data == other.data;
    }

    // 输出到流 - 声明为friend以访问private成员
    friend std::ostream& operator<<(std::ostream& os, const Value& value);
    
    // 字符串表示 (兼容C++23的std::format)
    std::string toString() const {
        return std::visit([](const auto& value) -> std::string {
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
        }, data);
    }

#if HAS_FORMAT
    // C++23 format support
    template<typename FormatContext>
    auto format(FormatContext& ctx) const {
        return std::visit([&ctx](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<T, int>) {
                return std::format_to(ctx.out(), "{}", value);
            } else if constexpr (std::same_as<T, std::string>) {
                return std::format_to(ctx.out(), "\"{}\"", value);
            }
            __builtin_unreachable();
        }, data);
    }
#endif
};

// C++23 列定义结构，使用designated initializers和defaulted comparison
struct Column {
    std::string name;
    DataType type;
    
    // C++23的designated initializers支持
    Column(const std::string& columnName, DataType columnType) 
        : name(columnName), type(columnType) {}
    
    // 使用C++23的defaulted spaceship operator
    auto operator<=>(const Column& other) const = default;
};

} // namespace tinydb