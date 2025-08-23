#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Include necessary header files
#include "../database/value.hpp"

// Forward declarations to avoid circular dependencies
namespace tinydb {
class Condition;
}

namespace tinydb::sql {

// Forward declarations
class ASTNode;
class Expression;
class Condition;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<Expression>;

// AST node base class
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

// Expression base class
class Expression : public ASTNode {
public:
    virtual tinydb::Value evaluate() const = 0;
};

// Literal expression
class LiteralExpression : public Expression {
private:
    tinydb::Value value_;

public:
    explicit LiteralExpression(tinydb::Value value) : value_(std::move(value)) {
    }

    tinydb::Value evaluate() const override {
        return value_;
    }
    std::string toString() const override;
};

// Column reference expression (supports qualified column names like table.column)
class ColumnExpression : public Expression {
private:
    std::string table_name_; // Optional table name qualifier
    std::string column_name_;

public:
    explicit ColumnExpression(std::string column_name)
        : table_name_(""), column_name_(std::move(column_name)) {
    }

    ColumnExpression(std::string table_name, std::string column_name)
        : table_name_(std::move(table_name)), column_name_(std::move(column_name)) {
    }

    const std::string& getTableName() const {
        return table_name_;
    }
    const std::string& getColumnName() const {
        return column_name_;
    }
    bool isQualified() const {
        return !table_name_.empty();
    }
    std::string getFullName() const {
        return isQualified() ? (table_name_ + "." + column_name_) : column_name_;
    }

    tinydb::Value evaluate() const override;
    std::string toString() const override;
};

// JOIN clause
class JoinClause : public ASTNode {
public:
    enum class JoinType {
        INNER
        // Can be extended to support LEFT, RIGHT, FULL JOIN
    };

private:
    JoinType join_type_;
    std::string table_name_;
    tinydb::Condition* on_condition_; // Use raw pointer to avoid incomplete type issues

public:
    JoinClause(JoinType join_type, std::string table_name,
               std::unique_ptr<tinydb::Condition> on_condition)
        : join_type_(join_type), table_name_(std::move(table_name)),
          on_condition_(on_condition.release()) {
    }

    ~JoinClause(); // Implemented in .cpp

    JoinType getJoinType() const {
        return join_type_;
    }
    const std::string& getTableName() const {
        return table_name_;
    }
    const tinydb::Condition* getOnCondition() const {
        return on_condition_;
    }

    std::string toString() const override;
};

// SQL statement base class
class Statement : public ASTNode {
public:
    enum class Type { CREATE_TABLE, INSERT, SELECT, UPDATE, DELETE };

    virtual Type getType() const = 0;
};

// CREATE TABLE statement
class CreateTableStatement : public Statement {
private:
    std::string table_name_;
    std::vector<tinydb::Column> columns_;

public:
    CreateTableStatement(std::string table_name, std::vector<tinydb::Column> columns);
    // Moved to cpp file implementation

    Type getType() const override {
        return Type::CREATE_TABLE;
    }
    const std::string& getTableName() const {
        return table_name_;
    }
    const std::vector<tinydb::Column>& getColumns() const {
        return columns_;
    }
    std::string toString() const override;
};

// INSERT statement
class InsertStatement : public Statement {
private:
    std::string table_name_;
    std::vector<std::string> columns_; // Optional column name list
    std::vector<ExpressionPtr> values_;

public:
    InsertStatement(std::string table_name, std::vector<std::string> columns,
                    std::vector<ExpressionPtr> values)
        : table_name_(std::move(table_name)), columns_(std::move(columns)),
          values_(std::move(values)) {
    }

    Type getType() const override {
        return Type::INSERT;
    }
    const std::string& getTableName() const {
        return table_name_;
    }
    const std::vector<std::string>& getColumns() const {
        return columns_;
    }
    const std::vector<ExpressionPtr>& getValues() const {
        return values_;
    }
    std::string toString() const override;
};

// SELECT statement (supports JOIN and WHERE conditions)
class SelectStatement : public Statement {
private:
    std::vector<std::string>
        columns_;            // Supports qualified column names like table1.col1, table2.col2
    std::string table_name_; // Main table name
    std::vector<std::unique_ptr<JoinClause>> joins_; // List of JOIN clauses
    tinydb::Condition* where_condition_; // Optional WHERE clause (simplified memory management)

public:
    SelectStatement(std::vector<std::string> columns, std::string table_name,
                    std::vector<std::unique_ptr<JoinClause>> joins = {},
                    std::unique_ptr<tinydb::Condition> where_condition = nullptr)
        : columns_(std::move(columns)), table_name_(std::move(table_name)),
          joins_(std::move(joins)), where_condition_(where_condition.release()) {
    }

    ~SelectStatement(); // Destructor implemented in .cpp

    Type getType() const override {
        return Type::SELECT;
    }
    const std::vector<std::string>& getColumns() const {
        return columns_;
    }
    const std::string& getTableName() const {
        return table_name_;
    }
    const std::vector<std::unique_ptr<JoinClause>>& getJoins() const {
        return joins_;
    }
    bool hasJoins() const {
        return !joins_.empty();
    }
    bool isSelectAll() const {
        return columns_.empty();
    }
    const tinydb::Condition* getWhereCondition() const {
        return where_condition_;
    }
    std::string toString() const override;
};

// UPDATE statement (supports WHERE conditions)
class UpdateStatement : public Statement {
private:
    std::string table_name_;
    std::vector<std::pair<std::string, ExpressionPtr>> assignments_; // column = value
    tinydb::Condition* where_condition_;

public:
    UpdateStatement(std::string table_name,
                    std::vector<std::pair<std::string, ExpressionPtr>> assignments,
                    std::unique_ptr<tinydb::Condition> where_condition = nullptr)
        : table_name_(std::move(table_name)), assignments_(std::move(assignments)),
          where_condition_(where_condition.release()) {
    }

    ~UpdateStatement(); // Destructor implemented in .cpp

    Type getType() const override {
        return Type::UPDATE;
    }
    const std::string& getTableName() const {
        return table_name_;
    }
    const std::vector<std::pair<std::string, ExpressionPtr>>& getAssignments() const {
        return assignments_;
    }
    const tinydb::Condition* getWhereCondition() const {
        return where_condition_;
    }
    std::string toString() const override;
};

// DELETE statement (supports WHERE conditions)
class DeleteStatement : public Statement {
private:
    std::string table_name_;
    tinydb::Condition* where_condition_;

public:
    DeleteStatement(std::string table_name,
                    std::unique_ptr<tinydb::Condition> where_condition = nullptr)
        : table_name_(std::move(table_name)), where_condition_(where_condition.release()) {
    }

    ~DeleteStatement(); // Destructor implemented in .cpp

    Type getType() const override {
        return Type::DELETE;
    }
    const std::string& getTableName() const {
        return table_name_;
    }
    const tinydb::Condition* getWhereCondition() const {
        return where_condition_;
    }
    std::string toString() const override;
};

} // namespace tinydb::sql
