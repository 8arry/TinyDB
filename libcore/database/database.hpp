#pragma once

#include "table.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace tinydb {

// Database类 - 管理多个表的容器
class Database {
private:
    std::unordered_map<std::string, std::unique_ptr<Table>> tables;
    std::string databaseName;

    // 内部辅助方法
    void validateTableName(const std::string& tableName) const;
    std::string normalizeTableName(const std::string& tableName) const;

public:
    // 构造函数
    explicit Database(std::string name = "TinyDB");
    
    // 禁用拷贝，启用移动
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = default;
    Database& operator=(Database&&) = default;

    // 基本信息
    const std::string& getName() const noexcept { return databaseName; }
    size_t getTableCount() const noexcept { return tables.size(); }
    bool empty() const noexcept { return tables.empty(); }

    // 表管理 - 创建
    void createTable(const std::string& tableName, std::vector<Column> schema);
    
    // 创建表的便利方法
    template<typename... ColumnPairs>
    void createTable(const std::string& tableName, ColumnPairs&&... columnPairs) {
        std::vector<Column> schema;
        schema.reserve(sizeof...(columnPairs));
        (schema.emplace_back(std::forward<ColumnPairs>(columnPairs)), ...);
        createTable(tableName, std::move(schema));
    }

    // 表管理 - 删除
    bool dropTable(const std::string& tableName);
    void clear() noexcept { tables.clear(); }

    // 表管理 - 查询
    bool hasTable(const std::string& tableName) const;
    const Table& getTable(const std::string& tableName) const;
    Table& getTable(const std::string& tableName);
    
    // 获取所有表名
    std::vector<std::string> getTableNames() const;

    // 插入行数据
    void insertInto(const std::string& tableName, std::vector<Value> values);
    void insertInto(const std::string& tableName, const Row& row);
    
    // 表操作的便利方法 - 直接插入基本类型值
    template<typename... Values>
    requires (sizeof...(Values) > 0) && (ValueType<Values> && ...)
    void insertInto(const std::string& tableName, Values&&... values) {
        auto& table = getTable(tableName);
        table.insertRow({Value{std::forward<Values>(values)}...});
    }

    // 查询操作
    std::vector<Row> selectFrom(const std::string& tableName, 
                               const std::vector<std::string>& columns = {"*"}) const;
    
    std::vector<Row> selectFrom(const std::string& tableName,
                               const std::vector<std::string>& columns,
                               const std::function<bool(const Row&, const Table&)>& condition) const;

    // 更新操作
    size_t updateTable(const std::string& tableName,
                      const std::unordered_map<std::string, Value>& updates,
                      const std::function<bool(const Row&, const Table&)>& condition);

    // 删除操作
    size_t deleteFrom(const std::string& tableName,
                     const std::function<bool(const Row&, const Table&)>& condition);

    // 数据库统计
    struct DatabaseStats {
        size_t tableCount;
        size_t totalRows;
        size_t totalColumns;
        std::vector<std::pair<std::string, size_t>> tableRowCounts; // 表名和行数
    };
    
    DatabaseStats getStats() const;

    // 数据库操作
    void truncateTable(const std::string& tableName); // 清空表数据但保留结构
    
    // C++23 范围支持
#if HAS_RANGES
    auto tablesView() const {
        return tables | std::views::transform([](const auto& pair) -> const Table& {
            return *pair.second;
        });
    }
    
    auto tableNamesView() const {
        return tables | std::views::keys;
    }
#else
    // 回退实现
    std::vector<std::reference_wrapper<const Table>> tablesView() const {
        std::vector<std::reference_wrapper<const Table>> result;
        result.reserve(tables.size());
        for (const auto& [name, table] : tables) {
            result.emplace_back(*table);
        }
        return result;
    }
    
    std::vector<std::string> tableNamesView() const {
        return getTableNames();
    }
#endif

    // 数据库验证和完整性检查
    struct ValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    ValidationResult validate() const;

    // 调试和输出
    void printDatabase() const;
    void printDatabaseInfo() const;
    void printTableList() const;

    // 导出功能（为将来的序列化准备）
    struct DatabaseSnapshot {
        std::string name;
        std::vector<std::pair<std::string, std::vector<Column>>> schemas;
        std::vector<std::pair<std::string, std::vector<Row>>> data;
    };
    
    DatabaseSnapshot createSnapshot() const;

    // 实用工具
    bool isEmpty(const std::string& tableName) const;
    size_t getRowCount(const std::string& tableName) const;
    size_t getColumnCount(const std::string& tableName) const;

    // 事务支持的基础（为将来扩展准备）
    class Transaction {
    private:
        Database& db;
        std::vector<std::function<void()>> rollbackActions;
        bool committed = false;

    public:
        explicit Transaction(Database& database) : db(database) {}
        ~Transaction() {
            if (!committed) {
                rollback();
            }
        }

        void addRollbackAction(std::function<void()> action) {
            rollbackActions.push_back(std::move(action));
        }

        void commit() {
            committed = true;
            rollbackActions.clear();
        }

        void rollback() {
            for (auto it = rollbackActions.rbegin(); it != rollbackActions.rend(); ++it) {
                (*it)();
            }
            rollbackActions.clear();
        }
    };

    // 创建事务
    std::unique_ptr<Transaction> beginTransaction() {
        return std::make_unique<Transaction>(*this);
    }
};

// 数据库异常类
class DatabaseError : public std::runtime_error {
public:
    explicit DatabaseError(const std::string& message) 
        : std::runtime_error("Database Error: " + message) {}
};

class TableNotFoundError : public DatabaseError {
public:
    explicit TableNotFoundError(const std::string& tableName)
        : DatabaseError("Table '" + tableName + "' not found") {}
};

class TableAlreadyExistsError : public DatabaseError {
public:
    explicit TableAlreadyExistsError(const std::string& tableName)
        : DatabaseError("Table '" + tableName + "' already exists") {}
};

} // namespace tinydb
