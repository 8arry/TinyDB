#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>

// 包含必要的头文件
#include "../database/value.hpp"

// 前向声明避免循环依赖  
namespace tinydb {
    class Condition;
}

namespace tinydb::sql {

// 前向声明
class ASTNode;
class Expression;
class Condition;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<Expression>;

// AST节点基类
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

// 表达式基类
class Expression : public ASTNode {
public:
    virtual tinydb::Value evaluate() const = 0;
};

// 字面量表达式
class LiteralExpression : public Expression {
private:
    tinydb::Value value_;

public:
    explicit LiteralExpression(tinydb::Value value) : value_(std::move(value)) {}
    
    tinydb::Value evaluate() const override { return value_; }
    std::string toString() const override;
};

// 列引用表达式
class ColumnExpression : public Expression {
private:
    std::string column_name_;

public:
    explicit ColumnExpression(std::string column_name) : column_name_(std::move(column_name)) {}
    
    const std::string& getColumnName() const { return column_name_; }
    tinydb::Value evaluate() const override;
    std::string toString() const override;
};

// SQL语句基类
class Statement : public ASTNode {
public:
    enum class Type {
        CREATE_TABLE,
        INSERT,
        SELECT,
        UPDATE,
        DELETE
    };
    
    virtual Type getType() const = 0;
};

// CREATE TABLE语句
class CreateTableStatement : public Statement {
private:
    std::string table_name_;
    std::vector<tinydb::Column> columns_;

public:
    CreateTableStatement(std::string table_name, std::vector<tinydb::Column> columns);
    // 移到cpp文件实现
    
    Type getType() const override { return Type::CREATE_TABLE; }
    const std::string& getTableName() const { return table_name_; }
    const std::vector<tinydb::Column>& getColumns() const { return columns_; }
    std::string toString() const override;
};

// INSERT语句
class InsertStatement : public Statement {
private:
    std::string table_name_;
    std::vector<std::string> columns_;  // 可选的列名列表
    std::vector<ExpressionPtr> values_;

public:
    InsertStatement(std::string table_name, 
                   std::vector<std::string> columns,
                   std::vector<ExpressionPtr> values)
        : table_name_(std::move(table_name))
        , columns_(std::move(columns))
        , values_(std::move(values)) {}
    
    Type getType() const override { return Type::INSERT; }
    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::string>& getColumns() const { return columns_; }
    const std::vector<ExpressionPtr>& getValues() const { return values_; }
    std::string toString() const override;
};

// SELECT语句 (支持WHERE条件)
class SelectStatement : public Statement {
private:
    std::vector<std::string> columns_;  // 空表示SELECT *
    std::string table_name_;
    tinydb::Condition* where_condition_;  // 可选的WHERE子句（简化内存管理）

public:
    SelectStatement(std::vector<std::string> columns, 
                   std::string table_name,
                   std::unique_ptr<tinydb::Condition> where_condition = nullptr)
        : columns_(std::move(columns))
        , table_name_(std::move(table_name))
        , where_condition_(where_condition.release()) {}
    
    ~SelectStatement(); // 析构函数在.cpp中实现
    
    Type getType() const override { return Type::SELECT; }
    const std::vector<std::string>& getColumns() const { return columns_; }
    const std::string& getTableName() const { return table_name_; }
    bool isSelectAll() const { return columns_.empty(); }
    const tinydb::Condition* getWhereCondition() const { return where_condition_; }
    std::string toString() const override;
};

// UPDATE语句 (支持WHERE条件)
class UpdateStatement : public Statement {
private:
    std::string table_name_;
    std::vector<std::pair<std::string, ExpressionPtr>> assignments_;  // column = value
    tinydb::Condition* where_condition_;

public:
    UpdateStatement(std::string table_name,
                   std::vector<std::pair<std::string, ExpressionPtr>> assignments,
                   std::unique_ptr<tinydb::Condition> where_condition = nullptr)
        : table_name_(std::move(table_name))
        , assignments_(std::move(assignments))
        , where_condition_(where_condition.release()) {}
    
    ~UpdateStatement(); // 析构函数在.cpp中实现
    
    Type getType() const override { return Type::UPDATE; }
    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::pair<std::string, ExpressionPtr>>& getAssignments() const { return assignments_; }
    const tinydb::Condition* getWhereCondition() const { return where_condition_; }
    std::string toString() const override;
};

// DELETE语句 (支持WHERE条件)
class DeleteStatement : public Statement {
private:
    std::string table_name_;
    tinydb::Condition* where_condition_;

public:
    DeleteStatement(std::string table_name,
                   std::unique_ptr<tinydb::Condition> where_condition = nullptr)
        : table_name_(std::move(table_name))
        , where_condition_(where_condition.release()) {}
    
    ~DeleteStatement(); // 析构函数在.cpp中实现
    
    Type getType() const override { return Type::DELETE; }
    const std::string& getTableName() const { return table_name_; }
    const tinydb::Condition* getWhereCondition() const { return where_condition_; }
    std::string toString() const override;
};

} // namespace tinydb::sql
