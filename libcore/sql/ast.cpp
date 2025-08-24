#include "ast.hpp"
#include "../database/condition.hpp"
#include "../database/value.hpp"

#ifdef HAS_FORMAT
#include <format>
#endif

namespace tinydb::sql {

// LiteralExpression implementation
std::string LiteralExpression::toString() const {
#ifdef HAS_FORMAT
    return std::format("Literal({})", value_.toString());
#else
    return "Literal(" + value_.toString() + ")";
#endif
}

// ColumnExpression implementation
tinydb::Value ColumnExpression::evaluate() const {
    // Column expression evaluation requires row context at execution time
    // This is a placeholder implementation, actual evaluation is done in the executor
    throw std::runtime_error("Column expression evaluation requires row context");
}

std::string ColumnExpression::toString() const {
#ifdef HAS_FORMAT
    if (isQualified()) {
        return std::format("Column({}.{})", table_name_, column_name_);
    } else {
        return std::format("Column({})", column_name_);
    }
#else
    if (isQualified()) {
        return "Column(" + table_name_ + "." + column_name_ + ")";
    } else {
        return "Column(" + column_name_ + ")";
    }
#endif
}

// CreateTableStatement implementation
CreateTableStatement::CreateTableStatement(std::string table_name,
                                           std::vector<tinydb::Column> columns)
    : table_name_(std::move(table_name)), columns_(std::move(columns)) {
}

std::string CreateTableStatement::toString() const {
    std::string result = "CREATE TABLE " + table_name_ + " (";
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0)
            result += ", ";
        result +=
            columns_[i].name + " " + (columns_[i].type == tinydb::DataType::INT ? "int" : "str");
    }
    result += ")";
    return result;
}

// InsertStatement implementation
std::string InsertStatement::toString() const {
    std::string result = "INSERT INTO " + table_name_;

    if (!columns_.empty()) {
        result += " (";
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (i > 0)
                result += ", ";
            result += columns_[i];
        }
        result += ")";
    }

    result += " VALUES (";
    for (size_t i = 0; i < values_.size(); ++i) {
        if (i > 0)
            result += ", ";
        result += values_[i]->toString();
    }
    result += ")";

    return result;
}

// JoinClause implementation
JoinClause::~JoinClause() {
    delete on_condition_;
}

std::string JoinClause::toString() const {
    std::string result;
    switch (join_type_) {
        case JoinType::INNER:
            result = "INNER JOIN ";
            break;
    }
    result += table_name_;
    if (on_condition_) {
        result += " ON " + on_condition_->toString();
    }
    return result;
}

// SelectStatement implementation
SelectStatement::~SelectStatement() {
    delete where_condition_;
}

std::string SelectStatement::toString() const {
    std::string result = "SELECT ";

    if (columns_.empty()) {
        result += "*";
    } else {
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (i > 0)
                result += ", ";
            result += columns_[i];
        }
    }

    result += " FROM " + table_name_;

    // Add JOIN clause
    for (const auto& join : joins_) {
        result += " " + join->toString();
    }

    if (where_condition_) {
        result += " WHERE " + where_condition_->toString();
    }

    return result;
}

// UpdateStatement implementation
UpdateStatement::~UpdateStatement() {
    delete where_condition_;
}

std::string UpdateStatement::toString() const {
    std::string result = "UPDATE " + table_name_ + " SET ";

    for (size_t i = 0; i < assignments_.size(); ++i) {
        if (i > 0)
            result += ", ";
        result += assignments_[i].first + " = " + assignments_[i].second->toString();
    }

    if (where_condition_) {
        result += " WHERE " + where_condition_->toString();
    }

    return result;
}

// DeleteStatement implementation
DeleteStatement::~DeleteStatement() {
    delete where_condition_;
}

std::string DeleteStatement::toString() const {
    std::string result = "DELETE FROM " + table_name_;

    if (where_condition_) {
        result += " WHERE " + where_condition_->toString();
    }

    return result;
}

} // namespace tinydb::sql
