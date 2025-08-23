#pragma once

#include "table.hpp"
#include <memory>
#include <functional>
#include <string>
#include <variant>

namespace tinydb {

// Forward declarations
class Row;
class Table;

// Comparison operator enumeration
enum class ComparisonOp {
    EQUAL,          // =
    NOT_EQUAL,      // !=
    LESS_THAN,      // <
    GREATER_THAN,   // >
    LESS_EQUAL,     // <=
    GREATER_EQUAL   // >=
};

// Logical operator enumeration
enum class LogicalOp {
    AND,
    OR,
    NOT
};

// Condition value type - can be literal or column reference
class ConditionValue {
public:
    enum class Type {
            LITERAL,    // Literal value
    COLUMN      // Column reference
    };

private:
    Type type;
    std::variant<Value, std::string> data; // Value for literal, string for column name

public:
    // Constructors
    explicit ConditionValue(Value literal) 
        : type(Type::LITERAL), data(std::move(literal)) {}
    
    explicit ConditionValue(std::string columnName) 
        : type(Type::COLUMN), data(std::move(columnName)) {}
    
    // Create from various types
    static ConditionValue literal(int value) {
        return ConditionValue(Value{value});
    }
    
    static ConditionValue literal(const std::string& value) {
        return ConditionValue(Value{value});
    }
    
    static ConditionValue literal(const char* value) {
        return ConditionValue(Value{std::string(value)});
    }
    
    static ConditionValue column(const std::string& columnName) {
        return ConditionValue(columnName);
    }

    // Get methods
    Type getType() const noexcept { return type; }
    
    bool isLiteral() const noexcept { return type == Type::LITERAL; }
    bool isColumn() const noexcept { return type == Type::COLUMN; }
    
    const Value& getLiteral() const {
        if (type != Type::LITERAL) {
            throw std::runtime_error("ConditionValue is not a literal");
        }
        return std::get<Value>(data);
    }
    
    const std::string& getColumnName() const {
        if (type != Type::COLUMN) {
            throw std::runtime_error("ConditionValue is not a column reference");
        }
        return std::get<std::string>(data);
    }
    
    // Evaluate - get actual value from row and table
    Value evaluate(const Row& row, const Table& table) const;
};

// Condition expression base class
class Condition {
public:
    virtual ~Condition() = default;
    virtual bool evaluate(const Row& row, const Table& table) const = 0;
    virtual std::string toString() const = 0;
    
    // Clone method supports deep copy
    virtual std::unique_ptr<Condition> clone() const = 0;
};

// Comparison condition
class ComparisonCondition : public Condition {
private:
    ConditionValue left;
    ComparisonOp op;
    ConditionValue right;

public:
    ComparisonCondition(ConditionValue leftVal, ComparisonOp operation, ConditionValue rightVal)
        : left(std::move(leftVal)), op(operation), right(std::move(rightVal)) {}

    bool evaluate(const Row& row, const Table& table) const override;
    std::string toString() const override;
    std::unique_ptr<Condition> clone() const override;

    // Get components
    const ConditionValue& getLeft() const { return left; }
    const ConditionValue& getRight() const { return right; }
    ComparisonOp getOperator() const { return op; }
};

// Logical condition
class LogicalCondition : public Condition {
private:
    std::unique_ptr<Condition> left;
    LogicalOp op;
    std::unique_ptr<Condition> right; // nullptr for NOT operations

public:
    // AND/OR constructor
    LogicalCondition(std::unique_ptr<Condition> leftCond, LogicalOp operation, 
                    std::unique_ptr<Condition> rightCond)
        : left(std::move(leftCond)), op(operation), right(std::move(rightCond)) {
        if (op == LogicalOp::NOT) {
            throw std::invalid_argument("Use NOT constructor for NOT operations");
        }
    }

    // NOT constructor
    LogicalCondition(LogicalOp operation, std::unique_ptr<Condition> condition)
        : left(std::move(condition)), op(operation), right(nullptr) {
        if (op != LogicalOp::NOT) {
            throw std::invalid_argument("This constructor is only for NOT operations");
        }
    }

    bool evaluate(const Row& row, const Table& table) const override;
    std::string toString() const override;
    std::unique_ptr<Condition> clone() const override;

    // Get components
    const Condition* getLeft() const { return left.get(); }
    const Condition* getRight() const { return right.get(); }
    LogicalOp getOperator() const { return op; }
};

// Condition builder - provides fluent API
class ConditionBuilder {
public:
    // Comparison operations
    static std::unique_ptr<Condition> compare(ConditionValue left, ComparisonOp op, ConditionValue right) {
        return std::make_unique<ComparisonCondition>(std::move(left), op, std::move(right));
    }
    
    // Convenience methods
    static std::unique_ptr<Condition> equal(ConditionValue left, ConditionValue right) {
        return compare(std::move(left), ComparisonOp::EQUAL, std::move(right));
    }
    
    static std::unique_ptr<Condition> notEqual(ConditionValue left, ConditionValue right) {
        return compare(std::move(left), ComparisonOp::NOT_EQUAL, std::move(right));
    }
    
    static std::unique_ptr<Condition> lessThan(ConditionValue left, ConditionValue right) {
        return compare(std::move(left), ComparisonOp::LESS_THAN, std::move(right));
    }
    
    static std::unique_ptr<Condition> greaterThan(ConditionValue left, ConditionValue right) {
        return compare(std::move(left), ComparisonOp::GREATER_THAN, std::move(right));
    }
    
    static std::unique_ptr<Condition> lessEqual(ConditionValue left, ConditionValue right) {
        return compare(std::move(left), ComparisonOp::LESS_EQUAL, std::move(right));
    }
    
    static std::unique_ptr<Condition> greaterEqual(ConditionValue left, ConditionValue right) {
        return compare(std::move(left), ComparisonOp::GREATER_EQUAL, std::move(right));
    }

    // Logical operations
    static std::unique_ptr<Condition> and_(std::unique_ptr<Condition> left, std::unique_ptr<Condition> right) {
        return std::make_unique<LogicalCondition>(std::move(left), LogicalOp::AND, std::move(right));
    }
    
    static std::unique_ptr<Condition> or_(std::unique_ptr<Condition> left, std::unique_ptr<Condition> right) {
        return std::make_unique<LogicalCondition>(std::move(left), LogicalOp::OR, std::move(right));
    }
    
    static std::unique_ptr<Condition> not_(std::unique_ptr<Condition> condition) {
        return std::make_unique<LogicalCondition>(LogicalOp::NOT, std::move(condition));
    }
};

// Condition factory - simplifies condition creation
namespace Conditions {
    // Column reference
    inline ConditionValue col(const std::string& name) {
        return ConditionValue::column(name);
    }
    
    // Literal
    inline ConditionValue val(int value) {
        return ConditionValue::literal(value);
    }
    
    inline ConditionValue val(const std::string& value) {
        return ConditionValue::literal(value);
    }
    
    inline ConditionValue val(const char* value) {
        return ConditionValue::literal(value);
    }
    
    // 比较操作符重载
    inline std::unique_ptr<Condition> operator==(ConditionValue left, ConditionValue right) {
        return ConditionBuilder::equal(std::move(left), std::move(right));
    }
    
    inline std::unique_ptr<Condition> operator!=(ConditionValue left, ConditionValue right) {
        return ConditionBuilder::notEqual(std::move(left), std::move(right));
    }
    
    inline std::unique_ptr<Condition> operator<(ConditionValue left, ConditionValue right) {
        return ConditionBuilder::lessThan(std::move(left), std::move(right));
    }
    
    inline std::unique_ptr<Condition> operator>(ConditionValue left, ConditionValue right) {
        return ConditionBuilder::greaterThan(std::move(left), std::move(right));
    }
    
    inline std::unique_ptr<Condition> operator<=(ConditionValue left, ConditionValue right) {
        return ConditionBuilder::lessEqual(std::move(left), std::move(right));
    }
    
    inline std::unique_ptr<Condition> operator>=(ConditionValue left, ConditionValue right) {
        return ConditionBuilder::greaterEqual(std::move(left), std::move(right));
    }
    
    // 逻辑操作符
    inline std::unique_ptr<Condition> operator&&(std::unique_ptr<Condition> left, std::unique_ptr<Condition> right) {
        return ConditionBuilder::and_(std::move(left), std::move(right));
    }
    
    inline std::unique_ptr<Condition> operator||(std::unique_ptr<Condition> left, std::unique_ptr<Condition> right) {
        return ConditionBuilder::or_(std::move(left), std::move(right));
    }
    
    inline std::unique_ptr<Condition> operator!(std::unique_ptr<Condition> condition) {
        return ConditionBuilder::not_(std::move(condition));
    }
}

// 条件转换器 - 将Condition转换为lambda函数
class ConditionAdapter {
public:
    static std::function<bool(const Row&, const Table&)> toLambda(const Condition& condition) {
        return [&condition](const Row& row, const Table& table) {
            return condition.evaluate(row, table);
        };
    }
    
    static std::function<bool(const Row&, const Table&)> toLambda(const std::unique_ptr<Condition>& condition) {
        return [&condition](const Row& row, const Table& table) {
            return condition->evaluate(row, table);
        };
    }
};

// 字符串转换工具
std::string comparisonOpToString(ComparisonOp op);
std::string logicalOpToString(LogicalOp op);

} // namespace tinydb
