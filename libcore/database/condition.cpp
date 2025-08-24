#include "condition.hpp"
#include <sstream>
#include <stdexcept>

namespace tinydb {

// ConditionValue implementation
Value ConditionValue::evaluate(const Row& row, const Table& table) const {
    switch (type) {
        case Type::LITERAL:
            return std::get<Value>(data);

        case Type::COLUMN: {
            const auto& columnName = std::get<std::string>(data);

            // Find column index
            const auto& schema = table.getSchema();
            size_t columnIndex = 0;
            bool found = false;

            for (size_t i = 0; i < schema.size(); ++i) {
                // Handle qualified column names: if columnName contains ".", only compare the
                // column name part
                std::string actualColumnName = columnName;
                size_t dotPos = columnName.find('.');
                if (dotPos != std::string::npos) {
                    actualColumnName = columnName.substr(dotPos + 1);
                }

                if (schema[i].name == actualColumnName) {
                    columnIndex = i;
                    found = true;
                    break;
                }
            }

            if (!found) {
                throw std::runtime_error("Column '" + columnName + "' not found in table");
            }

            if (columnIndex >= row.size()) {
                throw std::runtime_error("Row does not have enough columns");
            }

            return row[columnIndex];
        }

        default:
            throw std::runtime_error("Unknown ConditionValue type");
    }
}

// ComparisonCondition implementation
bool ComparisonCondition::evaluate(const Row& row, const Table& table) const {
    Value leftVal = left.evaluate(row, table);
    Value rightVal = right.evaluate(row, table);

    // Check type compatibility
    if (leftVal.getType() != rightVal.getType()) {
        throw std::runtime_error("Cannot compare values of different types");
    }

    switch (op) {
        case ComparisonOp::EQUAL:
            return leftVal == rightVal;

        case ComparisonOp::NOT_EQUAL:
            return leftVal != rightVal;

        case ComparisonOp::LESS_THAN:
            return leftVal < rightVal;

        case ComparisonOp::GREATER_THAN:
            return leftVal > rightVal;

        case ComparisonOp::LESS_EQUAL:
            return leftVal <= rightVal;

        case ComparisonOp::GREATER_EQUAL:
            return leftVal >= rightVal;

        default:
            throw std::runtime_error("Unknown comparison operator");
    }
}

std::string ComparisonCondition::toString() const {
    std::ostringstream oss;

    // Left operand
    if (left.isLiteral()) {
        oss << left.getLiteral().toString();
    } else {
        oss << left.getColumnName();
    }

    // Operator
    oss << " " << comparisonOpToString(op) << " ";

    // Right operand
    if (right.isLiteral()) {
        oss << right.getLiteral().toString();
    } else {
        oss << right.getColumnName();
    }

    return oss.str();
}

std::unique_ptr<Condition> ComparisonCondition::clone() const {
    return std::make_unique<ComparisonCondition>(left, op, right);
}

// LogicalCondition implementation
bool LogicalCondition::evaluate(const Row& row, const Table& table) const {
    switch (op) {
        case LogicalOp::AND:
            if (!left || !right) {
                throw std::runtime_error("AND operation requires two operands");
            }
            return left->evaluate(row, table) && right->evaluate(row, table);

        case LogicalOp::OR:
            if (!left || !right) {
                throw std::runtime_error("OR operation requires two operands");
            }
            return left->evaluate(row, table) || right->evaluate(row, table);

        case LogicalOp::NOT:
            if (!left || right) {
                throw std::runtime_error("NOT operation requires exactly one operand");
            }
            return !left->evaluate(row, table);

        default:
            throw std::runtime_error("Unknown logical operator");
    }
}

std::string LogicalCondition::toString() const {
    std::ostringstream oss;

    switch (op) {
        case LogicalOp::AND:
            oss << "(" << left->toString() << " AND " << right->toString() << ")";
            break;

        case LogicalOp::OR:
            oss << "(" << left->toString() << " OR " << right->toString() << ")";
            break;

        case LogicalOp::NOT:
            oss << "NOT (" << left->toString() << ")";
            break;

        default:
            oss << "UNKNOWN_LOGICAL_OP";
            break;
    }

    return oss.str();
}

std::unique_ptr<Condition> LogicalCondition::clone() const {
    switch (op) {
        case LogicalOp::AND:
        case LogicalOp::OR:
            return std::make_unique<LogicalCondition>(left->clone(), op, right->clone());

        case LogicalOp::NOT:
            return std::make_unique<LogicalCondition>(op, left->clone());

        default:
            throw std::runtime_error("Unknown logical operator in clone()");
    }
}

// Utility functions
std::string comparisonOpToString(ComparisonOp op) {
    switch (op) {
        case ComparisonOp::EQUAL:
            return "=";
        case ComparisonOp::NOT_EQUAL:
            return "!=";
        case ComparisonOp::LESS_THAN:
            return "<";
        case ComparisonOp::GREATER_THAN:
            return ">";
        case ComparisonOp::LESS_EQUAL:
            return "<=";
        case ComparisonOp::GREATER_EQUAL:
            return ">=";
        default:
            return "UNKNOWN";
    }
}

std::string logicalOpToString(LogicalOp op) {
    switch (op) {
        case LogicalOp::AND:
            return "AND";
        case LogicalOp::OR:
            return "OR";
        case LogicalOp::NOT:
            return "NOT";
        default:
            return "UNKNOWN";
    }
}

} // namespace tinydb
