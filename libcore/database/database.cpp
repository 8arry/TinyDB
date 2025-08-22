#include "database.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace tinydb {

// 构造函数
Database::Database(std::string name) : databaseName(std::move(name)) {
    if (databaseName.empty()) {
        databaseName = "TinyDB";
    }
}

// 内部辅助方法
void Database::validateTableName(const std::string& tableName) const {
    if (tableName.empty()) {
        throw DatabaseError("Table name cannot be empty");
    }
    
    // 检查表名是否包含非法字符
    if (!std::isalpha(tableName[0]) && tableName[0] != '_') {
        throw DatabaseError("Table name must start with a letter or underscore");
    }
    
    for (char c : tableName) {
        if (!std::isalnum(c) && c != '_') {
            throw DatabaseError("Table name can only contain letters, numbers, and underscores");
        }
    }
}

std::string Database::normalizeTableName(const std::string& tableName) const {
    validateTableName(tableName);
    return tableName; // 暂时不做大小写转换，保持原样
}

// 表管理 - 创建
void Database::createTable(const std::string& tableName, std::vector<Column> schema) {
    std::string normalizedName = normalizeTableName(tableName);
    
    if (hasTable(normalizedName)) {
        throw TableAlreadyExistsError(normalizedName);
    }
    
    if (schema.empty()) {
        throw DatabaseError("Table must have at least one column");
    }
    
    // 检查列名重复
    std::unordered_map<std::string, int> columnNames;
    for (const auto& column : schema) {
        if (columnNames.count(column.name) > 0) {
            throw DatabaseError("Duplicate column name: " + column.name);
        }
        columnNames[column.name] = 1;
    }
    
    // 创建表
    auto table = std::make_unique<Table>(normalizedName, std::move(schema));
    tables[normalizedName] = std::move(table);
}

// 表管理 - 删除
bool Database::dropTable(const std::string& tableName) {
    std::string normalizedName = normalizeTableName(tableName);
    
    auto it = tables.find(normalizedName);
    if (it != tables.end()) {
        tables.erase(it);
        return true;
    }
    return false;
}

// 表管理 - 查询
bool Database::hasTable(const std::string& tableName) const {
    try {
        std::string normalizedName = normalizeTableName(tableName);
        return tables.find(normalizedName) != tables.end();
    } catch (const DatabaseError&) {
        return false; // 无效的表名
    }
}

const Table& Database::getTable(const std::string& tableName) const {
    std::string normalizedName = normalizeTableName(tableName);
    
    auto it = tables.find(normalizedName);
    if (it == tables.end()) {
        throw TableNotFoundError(normalizedName);
    }
    
    return *it->second;
}

Table& Database::getTable(const std::string& tableName) {
    std::string normalizedName = normalizeTableName(tableName);
    
    auto it = tables.find(normalizedName);
    if (it == tables.end()) {
        throw TableNotFoundError(normalizedName);
    }
    
    return *it->second;
}

std::vector<std::string> Database::getTableNames() const {
    std::vector<std::string> names;
    names.reserve(tables.size());
    
    for (const auto& [name, table] : tables) {
        names.push_back(name);
    }
    
    // 按字母顺序排序
    std::sort(names.begin(), names.end());
    return names;
}

// 数据操作
void Database::insertInto(const std::string& tableName, std::vector<Value> values) {
    auto& table = getTable(tableName);
    table.insertRow(std::move(values));
}

void Database::insertInto(const std::string& tableName, const Row& row) {
    auto& table = getTable(tableName);
    table.insertRow(row);
}

std::vector<Row> Database::selectFrom(const std::string& tableName, 
                                     const std::vector<std::string>& columns) const {
    const auto& table = getTable(tableName);
    return table.selectRows(columns);
}

std::vector<Row> Database::selectFrom(const std::string& tableName,
                                     const std::vector<std::string>& columns,
                                     const std::function<bool(const Row&, const Table&)>& condition) const {
    const auto& table = getTable(tableName);
    return table.selectRows(columns, condition);
}

size_t Database::updateTable(const std::string& tableName,
                            const std::unordered_map<std::string, Value>& updates,
                            const std::function<bool(const Row&, const Table&)>& condition) {
    auto& table = getTable(tableName);
    return table.updateRows(condition, updates);
}

size_t Database::deleteFrom(const std::string& tableName,
                           const std::function<bool(const Row&, const Table&)>& condition) {
    auto& table = getTable(tableName);
    return table.deleteRows(condition);
}

// 统计信息
Database::DatabaseStats Database::getStats() const {
    DatabaseStats stats;
    stats.tableCount = tables.size();
    stats.totalRows = 0;
    stats.totalColumns = 0;
    
    for (const auto& [name, table] : tables) {
        size_t rowCount = table->getRowCount();
        size_t columnCount = table->getColumnCount();
        
        stats.totalRows += rowCount;
        stats.totalColumns += columnCount;
        stats.tableRowCounts.emplace_back(name, rowCount);
    }
    
    // 按表名排序
    std::sort(stats.tableRowCounts.begin(), stats.tableRowCounts.end());
    
    return stats;
}

// 表操作
void Database::truncateTable(const std::string& tableName) {
    auto& table = getTable(tableName);
    table.clear();
}

// 验证
Database::ValidationResult Database::validate() const {
    ValidationResult result;
    result.isValid = true;
    
    // 检查表名冲突（大小写不敏感）
    std::unordered_map<std::string, std::string> lowerCaseNames;
    for (const auto& [name, table] : tables) {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerCaseNames.count(lowerName) > 0) {
            result.isValid = false;
            result.errors.push_back("Table name conflict: '" + name + "' and '" + lowerCaseNames[lowerName] + "'");
        } else {
            lowerCaseNames[lowerName] = name;
        }
    }
    
    // 检查每个表的完整性
    for (const auto& [name, table] : tables) {
        if (table->getColumnCount() == 0) {
            result.isValid = false;
            result.errors.push_back("Table '" + name + "' has no columns");
        }
        
        // 检查列名冲突
        auto columnNames = table->getColumnNames();
        std::unordered_map<std::string, int> columnNameCount;
        for (const auto& columnName : columnNames) {
            columnNameCount[columnName]++;
            if (columnNameCount[columnName] > 1) {
                result.isValid = false;
                result.errors.push_back("Table '" + name + "' has duplicate column: " + columnName);
            }
        }
        
        // 性能警告
        if (table->getRowCount() > 10000) {
            result.warnings.push_back("Table '" + name + "' has a large number of rows (" + 
                                    std::to_string(table->getRowCount()) + "), consider optimization");
        }
    }
    
    return result;
}

// 调试输出
void Database::printDatabase() const {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "DATABASE: " << databaseName << "\n";
    std::cout << std::string(60, '=') << "\n";
    
    if (tables.empty()) {
        std::cout << "No tables in database.\n\n";
        return;
    }
    
    for (const auto& [name, table] : tables) {
        table->printTable();
    }
}

void Database::printDatabaseInfo() const {
    auto stats = getStats();
    
    std::cout << "\n📊 Database Information: " << databaseName << "\n";
    std::cout << std::string(50, '-') << "\n";
    std::cout << "Tables: " << stats.tableCount << "\n";
    std::cout << "Total Rows: " << stats.totalRows << "\n";
    std::cout << "Total Columns: " << stats.totalColumns << "\n";
    
    if (!stats.tableRowCounts.empty()) {
        std::cout << "\nTable Details:\n";
        for (const auto& [tableName, rowCount] : stats.tableRowCounts) {
            const auto& table = getTable(tableName);
            std::cout << "  " << std::setw(15) << std::left << tableName 
                     << " | Rows: " << std::setw(6) << rowCount
                     << " | Columns: " << table.getColumnCount() << "\n";
        }
    }
    std::cout << "\n";
}

void Database::printTableList() const {
    std::cout << "\n📋 Tables in " << databaseName << ":\n";
    std::cout << std::string(40, '-') << "\n";
    
    if (tables.empty()) {
        std::cout << "No tables found.\n\n";
        return;
    }
    
    auto tableNames = getTableNames();
    for (size_t i = 0; i < tableNames.size(); ++i) {
        const auto& tableName = tableNames[i];
        const auto& table = getTable(tableName);
        
        std::cout << std::setw(2) << (i + 1) << ". " << tableName 
                 << " (" << table.getColumnCount() << " columns, " 
                 << table.getRowCount() << " rows)\n";
    }
    std::cout << "\n";
}

// 快照功能
Database::DatabaseSnapshot Database::createSnapshot() const {
    DatabaseSnapshot snapshot;
    snapshot.name = databaseName;
    
    for (const auto& [tableName, table] : tables) {
        // 保存schema
        snapshot.schemas.emplace_back(tableName, table->getSchema());
        
        // 保存数据
        snapshot.data.emplace_back(tableName, table->getAllRows());
    }
    
    return snapshot;
}

// 实用工具
bool Database::isEmpty(const std::string& tableName) const {
    const auto& table = getTable(tableName);
    return table.empty();
}

size_t Database::getRowCount(const std::string& tableName) const {
    const auto& table = getTable(tableName);
    return table.getRowCount();
}

size_t Database::getColumnCount(const std::string& tableName) const {
    const auto& table = getTable(tableName);
    return table.getColumnCount();
}

} // namespace tinydb
