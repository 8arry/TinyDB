#pragma once

#include "table.hpp"
#include <memory>
#include <functional>
#include <string>
#include <variant>

namespace tinydb {

// 前向声明
class Row;
class Table;

// 比较操作符枚举
enum class ComparisonOp {
    EQUAL,          // =
    NOT_EQUAL,      // !=
    LESS_THAN,      // <
    GREATER_THAN,   // >
    LESS_EQUAL,     // <=
    GREATER_EQUAL   // >=
};

// 逻辑操作符枚举
enum class LogicalOp {
    AND,
    OR,
    NOT
};

// 条件值类型 - 可以是字面量或列引用
class ConditionValue {
public:
    enum class Type {
        LITERAL,    // 字面量值
        COLUMN      // 列引用
    };

private:
    Type type;
    std::variant<Value, std::string> data; // Value为字面量，string为列名

public:
    // 构造函数
    explicit ConditionValue(Value literal) 
        : type(Type::LITERAL), data(std::move(literal)) {}
    
    explicit ConditionValue(std::string columnName) 
        : type(Type::COLUMN), data(std::move(columnName)) {}
    
    // 从各种类型创建
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

    // 获取方法
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
    
    // 求值 - 从行和表中获取实际值
    Value evaluate(const Row& row, const Table& table) const;
};

// 条件表达式基类
class Condition {
public:
    virtual ~Condition() = default;
    virtual bool evaluate(const Row& row, const Table& table) const = 0;
    virtual std::string toString() const = 0;
    
    // 克隆方法支持深拷贝
    virtual std::unique_ptr<Condition> clone() const = 0;
};

// 比较条件
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

    // 获取组件
    const ConditionValue& getLeft() const { return left; }
    const ConditionValue& getRight() const { return right; }
    ComparisonOp getOperator() const { return op; }
};

// 逻辑条件
class LogicalCondition : public Condition {
private:
    std::unique_ptr<Condition> left;
    LogicalOp op;
    std::unique_ptr<Condition> right; // NOT操作时为nullptr

public:
    // AND/OR构造函数
    LogicalCondition(std::unique_ptr<Condition> leftCond, LogicalOp operation, 
                    std::unique_ptr<Condition> rightCond)
        : left(std::move(leftCond)), op(operation), right(std::move(rightCond)) {
        if (op == LogicalOp::NOT) {
            throw std::invalid_argument("Use NOT constructor for NOT operations");
        }
    }

    // NOT构造函数
    LogicalCondition(LogicalOp operation, std::unique_ptr<Condition> condition)
        : left(std::move(condition)), op(operation), right(nullptr) {
        if (op != LogicalOp::NOT) {
            throw std::invalid_argument("This constructor is only for NOT operations");
        }
    }

    bool evaluate(const Row& row, const Table& table) const override;
    std::string toString() const override;
    std::unique_ptr<Condition> clone() const override;

    // 获取组件
    const Condition* getLeft() const { return left.get(); }
    const Condition* getRight() const { return right.get(); }
    LogicalOp getOperator() const { return op; }
};

// 条件构建器 - 提供流式API
class ConditionBuilder {
public:
    // 比较操作
    static std::unique_ptr<Condition> compare(ConditionValue left, ComparisonOp op, ConditionValue right) {
        return std::make_unique<ComparisonCondition>(std::move(left), op, std::move(right));
    }
    
    // 便利方法
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

    // 逻辑操作
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

// 条件工厂 - 简化条件创建
namespace Conditions {
    // 列引用
    inline ConditionValue col(const std::string& name) {
        return ConditionValue::column(name);
    }
    
    // 字面量
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
