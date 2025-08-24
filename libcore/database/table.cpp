#include "table.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace tinydb {

// Table constructors
Table::Table(std::string name, std::vector<Column> columns)
    : schema(std::move(columns)), tableName(std::move(name)) {
    if (schema.empty()) {
        throw std::invalid_argument("Table must have at least one column");
    }
    if (tableName.empty()) {
        throw std::invalid_argument("Table name cannot be empty");
    }
}

// Internal helper methods
size_t Table::findColumnIndex(const std::string& columnName) const {
    for (size_t i = 0; i < schema.size(); ++i) {
        if (schema[i].name == columnName) {
            return i;
        }
    }
    throw std::invalid_argument("Column '" + columnName + "' not found");
}

void Table::validateRow(const Row& row) const {
    if (row.size() != schema.size()) {
        throw std::invalid_argument("Row has " + std::to_string(row.size()) +
                                    " values, but table has " + std::to_string(schema.size()) +
                                    " columns");
    }

    // Check type matching
    for (size_t i = 0; i < schema.size(); ++i) {
        if (row[i].getType() != schema[i].type) {
            throw std::invalid_argument(
                "Type mismatch in column '" + schema[i].name + "': expected " +
                (schema[i].type == DataType::INT ? "int" : "string") + ", got " +
                (row[i].getType() == DataType::INT ? "int" : "string"));
        }
    }
}

void Table::validateColumnExists(const std::string& columnName) const {
    if (!hasColumn(columnName)) {
        throw std::invalid_argument("Column '" + columnName + "' does not exist");
    }
}

// Column information methods
const Column& Table::getColumn(size_t index) const {
    if (index >= schema.size()) {
        throw std::out_of_range("Column index out of range");
    }
    return schema[index];
}

const Column& Table::getColumn(const std::string& name) const {
    size_t index = findColumnIndex(name);
    return schema[index];
}

bool Table::hasColumn(const std::string& name) const noexcept {
    for (const auto& column : schema) {
        if (column.name == name) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> Table::getColumnNames() const {
    std::vector<std::string> names;
    names.reserve(schema.size());

    for (const auto& column : schema) {
        names.push_back(column.name);
    }

    return names;
}

// Data insertion
void Table::insertRow(Row row) {
    validateRow(row);
    rows.emplace_back(std::move(row));
}

void Table::insertRow(std::vector<Value> values) {
    insertRow(Row{std::move(values)});
}

// Query operations
std::vector<Row> Table::selectRows(const std::vector<std::string>& columnNames) const {
    // Handle "*" wildcard
    if (columnNames.size() == 1 && columnNames[0] == "*") {
        return rows; // Return copy of all rows
    }

    // Validate columns exist
    std::vector<size_t> columnIndices;
    columnIndices.reserve(columnNames.size());

    for (const auto& columnName : columnNames) {
        validateColumnExists(columnName);
        columnIndices.push_back(findColumnIndex(columnName));
    }

    // Build result
    std::vector<Row> result;
    result.reserve(rows.size());

    for (const auto& row : rows) {
        std::vector<Value> selectedValues;
        selectedValues.reserve(columnIndices.size());

        for (size_t index : columnIndices) {
            selectedValues.push_back(row[index]);
        }

        result.emplace_back(std::move(selectedValues));
    }

    return result;
}

std::vector<Row>
Table::selectRows(const std::vector<std::string>& columnNames,
                  const std::function<bool(const Row&, const Table&)>& condition) const {
    // Filter rows first, then project columns
    std::vector<Row> filteredRows;

    for (const auto& row : rows) {
        if (condition(row, *this)) {
            filteredRows.push_back(row);
        }
    }

    // If no column restriction, return all columns
    if (columnNames.empty() || (columnNames.size() == 1 && columnNames[0] == "*")) {
        return filteredRows;
    }

    // Project specified columns
    std::vector<size_t> columnIndices;
    columnIndices.reserve(columnNames.size());

    for (const auto& columnName : columnNames) {
        validateColumnExists(columnName);
        columnIndices.push_back(findColumnIndex(columnName));
    }

    std::vector<Row> result;
    result.reserve(filteredRows.size());

    for (const auto& row : filteredRows) {
        std::vector<Value> selectedValues;
        selectedValues.reserve(columnIndices.size());

        for (size_t index : columnIndices) {
            selectedValues.push_back(row[index]);
        }

        result.emplace_back(std::move(selectedValues));
    }

    return result;
}

// Update operations
size_t Table::updateRows(const std::function<bool(const Row&, const Table&)>& condition,
                         const std::unordered_map<std::string, Value>& updates) {
    size_t updatedCount = 0;

    // Validate that columns to update exist
    for (const auto& [columnName, value] : updates) {
        validateColumnExists(columnName);

        // Validate type matching
        const auto& column = getColumn(columnName);
        if (value.getType() != column.type) {
            throw std::invalid_argument("Type mismatch for column '" + columnName + "': expected " +
                                        (column.type == DataType::INT ? "int" : "string") +
                                        ", got " +
                                        (value.getType() == DataType::INT ? "int" : "string"));
        }
    }

    // Execute update
    for (auto& row : rows) {
        if (condition(row, *this)) {
            for (const auto& [columnName, value] : updates) {
                size_t columnIndex = findColumnIndex(columnName);
                row.setValue(columnIndex, value);
            }
            ++updatedCount;
        }
    }

    return updatedCount;
}

// Delete operations
size_t Table::deleteRows(const std::function<bool(const Row&, const Table&)>& condition) {
    size_t originalSize = rows.size();

    auto newEnd = std::remove_if(rows.begin(), rows.end(), [&condition, this](const Row& row) {
        return condition(row, *this);
    });

    rows.erase(newEnd, rows.end());

    return originalSize - rows.size();
}

// Row access
const Row& Table::getRow(size_t index) const {
    if (index >= rows.size()) {
        throw std::out_of_range("Row index out of range");
    }
    return rows[index];
}

Row& Table::getRow(size_t index) {
    if (index >= rows.size()) {
        throw std::out_of_range("Row index out of range");
    }
    return rows[index];
}

// Value access
const Value& Table::getValue(size_t rowIndex, const std::string& columnName) const {
    if (rowIndex >= rows.size()) {
        throw std::out_of_range("Row index out of range");
    }

    size_t columnIndex = findColumnIndex(columnName);
    return rows[rowIndex][columnIndex];
}

Value& Table::getValue(size_t rowIndex, const std::string& columnName) {
    if (rowIndex >= rows.size()) {
        throw std::out_of_range("Row index out of range");
    }

    size_t columnIndex = findColumnIndex(columnName);
    return rows[rowIndex][columnIndex];
}

// Column value retrieval
std::vector<Value> Table::getColumnValues(const std::string& columnName) const {
    size_t columnIndex = findColumnIndex(columnName);

    std::vector<Value> values;
    values.reserve(rows.size());

    for (const auto& row : rows) {
        values.push_back(row[columnIndex]);
    }

    return values;
}

// Debug output methods
void Table::printSchema() const {
    std::cout << "Table: " << tableName << "\n";
    std::cout << "Columns:\n";

    for (size_t i = 0; i < schema.size(); ++i) {
        const auto& column = schema[i];
        std::cout << "  " << i << ": " << column.name << " ("
                  << (column.type == DataType::INT ? "int" : "string") << ")\n";
    }
}

void Table::printData() const {
    if (rows.empty()) {
        std::cout << "No data in table " << tableName << "\n";
        return;
    }

    for (size_t i = 0; i < rows.size(); ++i) {
        std::cout << "Row " << i << ": ";
        const auto& row = rows[i];

        for (size_t j = 0; j < row.size(); ++j) {
            if (j > 0)
                std::cout << ", ";
            std::cout << row[j];
        }
        std::cout << "\n";
    }
}

void Table::printTable() const {
    std::cout << "\n=== Table: " << tableName << " ===\n";

    // Print column headers
    for (size_t i = 0; i < schema.size(); ++i) {
        if (i > 0)
            std::cout << " | ";
        std::cout << std::setw(12) << std::left << schema[i].name;
    }
    std::cout << "\n";

    // Print separator line
    for (size_t i = 0; i < schema.size(); ++i) {
        if (i > 0)
            std::cout << "-+-";
        std::cout << std::string(12, '-');
    }
    std::cout << "\n";

    // Print data
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0)
                std::cout << " | ";
            std::cout << std::setw(12) << std::left << row[i].toString();
        }
        std::cout << "\n";
    }

    std::cout << "\nRows: " << rows.size() << "\n\n";
}

} // namespace tinydb
