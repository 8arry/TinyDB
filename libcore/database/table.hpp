#pragma once

#include "value.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <functional>

// 条件性包含C++23 ranges
#if __cplusplus >= 202002L && __has_include(<ranges>)
#include <ranges>
#define HAS_RANGES 1
#else
#define HAS_RANGES 0
#endif

namespace tinydb {

// 前向声明
class Condition;

// C++23 Row类 - 表示表中的一行数据
class Row {
private:
    std::vector<Value> values;

public:
    // 构造函数
    Row() = default;
    explicit Row(std::vector<Value> vals) : values(std::move(vals)) {}
    
    // C++23 范围构造函数
    template<std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, Value>
    explicit Row(R&& range) {
        values.reserve(std::ranges::size(range));
        for (auto&& val : range) {
            values.emplace_back(std::forward<decltype(val)>(val));
        }
    }

    // 访问方法
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

    // 获取值的数量
    size_t size() const noexcept { return values.size(); }
    bool empty() const noexcept { return values.empty(); }

    // 获取所有值的引用（用于遍历）
    const std::vector<Value>& getValues() const noexcept { return values; }
    std::vector<Value>& getValues() noexcept { return values; }

    // 添加值
    void addValue(Value value) {
        values.emplace_back(std::move(value));
    }

    // 设置值
    void setValue(size_t index, Value value) {
        if (index >= values.size()) {
            throw std::out_of_range("Row index out of range");
        }
        values[index] = std::move(value);
    }

    // C++23 比较操作
    auto operator<=>(const Row& other) const = default;

    // 迭代器支持
    auto begin() { return values.begin(); }
    auto end() { return values.end(); }
    auto begin() const { return values.begin(); }
    auto end() const { return values.end(); }
    auto cbegin() const { return values.cbegin(); }
    auto cend() const { return values.cend(); }
};

// Table类 - 管理表的schema和数据
class Table {
private:
    std::vector<Column> schema;
    std::vector<Row> rows;
    std::string tableName;

    // 内部辅助方法
    size_t findColumnIndex(const std::string& columnName) const;
    void validateRow(const Row& row) const;
    void validateColumnExists(const std::string& columnName) const;

public:
    // 构造函数
    explicit Table(std::string name, std::vector<Column> columns);
    
    // 禁用拷贝，启用移动
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&&) = default;
    Table& operator=(Table&&) = default;

    // 基本信息
    const std::string& getName() const noexcept { return tableName; }
    const std::vector<Column>& getSchema() const noexcept { return schema; }
    size_t getColumnCount() const noexcept { return schema.size(); }
    size_t getRowCount() const noexcept { return rows.size(); }
    bool empty() const noexcept { return rows.empty(); }

    // 列信息
    const Column& getColumn(size_t index) const;
    const Column& getColumn(const std::string& name) const;
    bool hasColumn(const std::string& name) const noexcept;
    std::vector<std::string> getColumnNames() const;

    // 数据操作 - 插入
    void insertRow(Row row);
    void insertRow(std::vector<Value> values);
    
    // C++23 范围插入
    template<std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, Value>
    void insertRow(R&& values) {
        insertRow(Row{std::forward<R>(values)});
    }

    // 数据操作 - 查询
    const std::vector<Row>& getAllRows() const noexcept { return rows; }
    std::vector<Row> selectRows(const std::vector<std::string>& columnNames) const;
    std::vector<Row> selectRows(const std::vector<std::string>& columnNames, 
                               const std::function<bool(const Row&, const Table&)>& condition) const;

    // 数据操作 - 更新
    size_t updateRows(const std::function<bool(const Row&, const Table&)>& condition,
                     const std::unordered_map<std::string, Value>& updates);

    // 数据操作 - 删除
    size_t deleteRows(const std::function<bool(const Row&, const Table&)>& condition);
    void clear() noexcept { rows.clear(); }

    // 行访问
    const Row& getRow(size_t index) const;
    Row& getRow(size_t index);

    // 特定值获取（按列名和行索引）
    const Value& getValue(size_t rowIndex, const std::string& columnName) const;
    Value& getValue(size_t rowIndex, const std::string& columnName);

#if HAS_RANGES
    // C++23 范围和视图支持
    auto rowsView() const { return std::views::all(rows); }
    auto rowsView() { return std::views::all(rows); }

    // 过滤行的视图
    template<typename Predicate>
    auto filteredRowsView(Predicate&& pred) const {
        return rows | std::views::filter([&pred, this](const Row& row) {
            return pred(row, *this);
        });
    }
#else
    // 回退实现
    const std::vector<Row>& rowsView() const { return rows; }
    std::vector<Row>& rowsView() { return rows; }

    // 过滤行的回退实现
    template<typename Predicate>
    std::vector<Row> filteredRowsView(Predicate&& pred) const {
        std::vector<Row> result;
        for (const auto& row : rows) {
            if (pred(row, *this)) {
                result.push_back(row);
            }
        }
        return result;
    }
#endif

    // 投影列的视图（选择特定列）
    std::vector<Value> getColumnValues(const std::string& columnName) const;

    // 调试输出
    void printSchema() const;
    void printData() const;
    void printTable() const;
};

} // namespace tinydb
