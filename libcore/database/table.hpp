#pragma once

#include "value.hpp"
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// Conditional inclusion of C++23 ranges
#if __cplusplus >= 202002L && __has_include(<ranges>)
#include <ranges>
#define HAS_RANGES 1
#else
#define HAS_RANGES 0
#endif

namespace tinydb {

// Forward declarations
class Condition;

// C++23 Row class - represents a row of data in a table
class Row {
private:
    std::vector<Value> values;

public:
    // Constructors
    Row() = default;
    explicit Row(std::vector<Value> vals) : values(std::move(vals)) {
    }

    // C++23 range constructor
    template <std::ranges::range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, Value>
    explicit Row(R&& range) {
        values.reserve(std::ranges::size(range));
        for (auto&& val : range) {
            values.emplace_back(std::forward<decltype(val)>(val));
        }
    }

    // Access methods
    const Value& operator[](size_t index) const {
        if (index >= values.size()) {
            throw std::out_of_range("Row index out of range");
        }
        return values[index];
    }

    Value& operator[](size_t index) {
        if (index >= values.size()) {
            throw std::out_of_range("Row index out of range");
        }
        return values[index];
    }

    // Get value count
    size_t size() const noexcept {
        return values.size();
    }
    bool empty() const noexcept {
        return values.empty();
    }

    // Get reference to all values (for iteration)
    const std::vector<Value>& getValues() const noexcept {
        return values;
    }
    std::vector<Value>& getValues() noexcept {
        return values;
    }

    // Add value
    void addValue(Value value) {
        values.emplace_back(std::move(value));
    }

    // Set value
    void setValue(size_t index, Value value) {
        if (index >= values.size()) {
            throw std::out_of_range("Row index out of range");
        }
        values[index] = std::move(value);
    }

    // C++23 comparison operations
    auto operator<=>(const Row& other) const = default;

    // Iterator support
    auto begin() {
        return values.begin();
    }
    auto end() {
        return values.end();
    }
    auto begin() const {
        return values.begin();
    }
    auto end() const {
        return values.end();
    }
    auto cbegin() const {
        return values.cbegin();
    }
    auto cend() const {
        return values.cend();
    }
};

// Table class - manages table schema and data
class Table {
private:
    std::vector<Column> schema;
    std::vector<Row> rows;
    std::string tableName;

    // Internal helper methods
    size_t findColumnIndex(const std::string& columnName) const;
    void validateRow(const Row& row) const;
    void validateColumnExists(const std::string& columnName) const;

public:
    // Constructors
    explicit Table(std::string name, std::vector<Column> columns);

    // Disable copy, enable move
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&&) = default;
    Table& operator=(Table&&) = default;

    // Basic information
    const std::string& getName() const noexcept {
        return tableName;
    }
    const std::vector<Column>& getSchema() const noexcept {
        return schema;
    }
    size_t getColumnCount() const noexcept {
        return schema.size();
    }
    size_t getRowCount() const noexcept {
        return rows.size();
    }
    bool empty() const noexcept {
        return rows.empty();
    }

    // Column information
    const Column& getColumn(size_t index) const;
    const Column& getColumn(const std::string& name) const;
    bool hasColumn(const std::string& name) const noexcept;
    std::vector<std::string> getColumnNames() const;

    // Data operations - insertion
    void insertRow(Row row);
    void insertRow(std::vector<Value> values);

    // C++23 range insertion
    template <std::ranges::range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, Value>
    void insertRow(R&& values) {
        insertRow(Row{std::forward<R>(values)});
    }

    // Data operations - query
    const std::vector<Row>& getAllRows() const noexcept {
        return rows;
    }
    std::vector<Row> selectRows(const std::vector<std::string>& columnNames) const;
    std::vector<Row>
    selectRows(const std::vector<std::string>& columnNames,
               const std::function<bool(const Row&, const Table&)>& condition) const;

    // Data operations - update
    size_t updateRows(const std::function<bool(const Row&, const Table&)>& condition,
                      const std::unordered_map<std::string, Value>& updates);

    // Data operations - deletion
    size_t deleteRows(const std::function<bool(const Row&, const Table&)>& condition);
    void clear() noexcept {
        rows.clear();
    }

    // Row access
    const Row& getRow(size_t index) const;
    Row& getRow(size_t index);

    // Get specific value (by column name and row index)
    const Value& getValue(size_t rowIndex, const std::string& columnName) const;
    Value& getValue(size_t rowIndex, const std::string& columnName);

#if HAS_RANGES
    // C++23 range and view support
    auto rowsView() const {
        return std::views::all(rows);
    }
    auto rowsView() {
        return std::views::all(rows);
    }

    // Filtered rows view
    template <typename Predicate> auto filteredRowsView(Predicate&& pred) const {
        return rows |
               std::views::filter([&pred, this](const Row& row) { return pred(row, *this); });
    }
#else
    // Fallback implementation
    const std::vector<Row>& rowsView() const {
        return rows;
    }
    std::vector<Row>& rowsView() {
        return rows;
    }

    // Fallback implementation for filtered rows
    template <typename Predicate> std::vector<Row> filteredRowsView(Predicate&& pred) const {
        std::vector<Row> result;
        for (const auto& row : rows) {
            if (pred(row, *this)) {
                result.push_back(row);
            }
        }
        return result;
    }
#endif

    // Column projection view (select specific columns)
    std::vector<Value> getColumnValues(const std::string& columnName) const;

    // Debug output
    void printSchema() const;
    void printData() const;
    void printTable() const;
};

} // namespace tinydb
