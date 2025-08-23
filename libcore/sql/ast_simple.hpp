#pragma once

#include <memory>
#include <vector>
#include <string>
#include "../database/value.hpp"

namespace tinydb::sql {

// 前向声明
class ASTNode;
class Expression;

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
    virtual tinydb::database::Value evaluate() const = 0;
};

// 字面量表达式
class LiteralExpression : public Expression {
private:
    tinydb::database::Value value_;

public:
    explicit LiteralExpression(tinydb::database::Value value) : value_(std::move(value)) {}
    
    tinydb::database::Value evaluate() const override { return value_; }
    std::string toString() const override;
};

// 列引用表达式
class ColumnExpression : public Expression {
private:
    std::string column_name_;

public:
    explicit ColumnExpression(std::string column_name) : column_name_(std::move(column_name)) {}
    
    const std::string& getColumnName() const { return column_name_; }
    tinydb::database::Value evaluate() const override;
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
    std::vector<tinydb::database::Column> columns_;

public:
    CreateTableStatement(std::string table_name, std::vector<tinydb::database::Column> columns)
        : table_name_(std::move(table_name)), columns_(std::move(columns)) {}
    
    Type getType() const override { return Type::CREATE_TABLE; }
    const std::string& getTableName() const { return table_name_; }
    const std::vector<tinydb::database::Column>& getColumns() const { return columns_; }
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

// 简化的SELECT语句（暂时不支持WHERE）
class SelectStatement : public Statement {
private:
    std::vector<std::string> columns_;  // 空表示SELECT *
    std::string table_name_;

public:
    SelectStatement(std::vector<std::string> columns, std::string table_name)
        : columns_(std::move(columns)), table_name_(std::move(table_name)) {}
    
    Type getType() const override { return Type::SELECT; }
    const std::vector<std::string>& getColumns() const { return columns_; }
    const std::string& getTableName() const { return table_name_; }
    bool isSelectAll() const { return columns_.empty(); }
    std::string toString() const override;
};

// 简化的UPDATE语句（暂时不支持WHERE）
class UpdateStatement : public Statement {
private:
    std::string table_name_;
    std::vector<std::pair<std::string, ExpressionPtr>> assignments_;  // column = value

public:
    UpdateStatement(std::string table_name,
                   std::vector<std::pair<std::string, ExpressionPtr>> assignments)
        : table_name_(std::move(table_name)), assignments_(std::move(assignments)) {}
    
    Type getType() const override { return Type::UPDATE; }
    const std::string& getTableName() const { return table_name_; }
    const std::vector<std::pair<std::string, ExpressionPtr>>& getAssignments() const { return assignments_; }
    std::string toString() const override;
};

// 简化的DELETE语句（暂时不支持WHERE）
class DeleteStatement : public Statement {
private:
    std::string table_name_;

public:
    explicit DeleteStatement(std::string table_name) : table_name_(std::move(table_name)) {}
    
    Type getType() const override { return Type::DELETE; }
    const std::string& getTableName() const { return table_name_; }
    std::string toString() const override;
};

} // namespace tinydb::sql
