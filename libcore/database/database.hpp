#pragma once

#include "table.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace tinydb {

// Database class - container for managing multiple tables
class Database {
private:
    std::unordered_map<std::string, std::unique_ptr<Table>> tables;
    std::string databaseName;

    // Internal helper methods
    void validateTableName(const std::string& tableName) const;
    std::string normalizeTableName(const std::string& tableName) const;

public:
    // Constructors
    explicit Database(std::string name = "TinyDB");

    // Disable copy, enable move
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = default;
    Database& operator=(Database&&) = default;

    // Basic information
    const std::string& getName() const noexcept {
        return databaseName;
    }
    size_t getTableCount() const noexcept {
        return tables.size();
    }
    bool empty() const noexcept {
        return tables.empty();
    }

    // Table management - creation
    void createTable(const std::string& tableName, std::vector<Column> schema);

    // Convenience method for creating tables
    template <typename... ColumnPairs>
    void createTable(const std::string& tableName, ColumnPairs&&... columnPairs) {
        std::vector<Column> schema;
        schema.reserve(sizeof...(columnPairs));
        (schema.emplace_back(std::forward<ColumnPairs>(columnPairs)), ...);
        createTable(tableName, std::move(schema));
    }

    // Table management - deletion
    bool dropTable(const std::string& tableName);
    void clear() noexcept {
        tables.clear();
    }

    // Table management - query
    bool hasTable(const std::string& tableName) const;
    const Table& getTable(const std::string& tableName) const;
    Table& getTable(const std::string& tableName);

    // Get all table names
    std::vector<std::string> getTableNames() const;

    // Insert row data
    void insertInto(const std::string& tableName, std::vector<Value> values);
    void insertInto(const std::string& tableName, const Row& row);

    // Convenience method for table operations - direct insertion of basic type values
    template <typename... Values>
        requires(sizeof...(Values) > 0) && (ValueType<Values> && ...)
    void insertInto(const std::string& tableName, Values&&... values) {
        auto& table = getTable(tableName);
        table.insertRow({Value{std::forward<Values>(values)}...});
    }

    // Query operations
    std::vector<Row> selectFrom(const std::string& tableName,
                                const std::vector<std::string>& columns = {"*"}) const;

    std::vector<Row>
    selectFrom(const std::string& tableName, const std::vector<std::string>& columns,
               const std::function<bool(const Row&, const Table&)>& condition) const;

    // Update operations
    size_t updateTable(const std::string& tableName,
                       const std::unordered_map<std::string, Value>& updates,
                       const std::function<bool(const Row&, const Table&)>& condition);

    // Delete operations
    size_t deleteFrom(const std::string& tableName,
                      const std::function<bool(const Row&, const Table&)>& condition);

    // Database statistics
    struct DatabaseStats {
        size_t tableCount;
        size_t totalRows;
        size_t totalColumns;
        std::vector<std::pair<std::string, size_t>> tableRowCounts; // Table name and row count
    };

    DatabaseStats getStats() const;

    // Database operations
    void truncateTable(const std::string& tableName); // Clear table data but preserve structure

    // C++23 range support
#if HAS_RANGES
    auto tablesView() const {
        return tables |
               std::views::transform([](const auto& pair) -> const Table& { return *pair.second; });
    }

    auto tableNamesView() const {
        return tables | std::views::keys;
    }
#else
    // Fallback implementation
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

    // Database validation and integrity check
    struct ValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    ValidationResult validate() const;

    // Debug and output
    void printDatabase() const;
    void printDatabaseInfo() const;
    void printTableList() const;

    // Export functionality (for future serialization)
    struct DatabaseSnapshot {
        std::string name;
        std::vector<std::pair<std::string, std::vector<Column>>> schemas;
        std::vector<std::pair<std::string, std::vector<Row>>> data;
    };

    DatabaseSnapshot createSnapshot() const;

    // Utility functions
    bool isEmpty(const std::string& tableName) const;
    size_t getRowCount(const std::string& tableName) const;
    size_t getColumnCount(const std::string& tableName) const;

    // Transaction support foundation (for future extension)
    class Transaction {
    private:
        Database& db;
        std::vector<std::function<void()>> rollbackActions;
        bool committed = false;

    public:
        explicit Transaction(Database& database) : db(database) {
        }
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

    // Create transaction
    std::unique_ptr<Transaction> beginTransaction() {
        return std::make_unique<Transaction>(*this);
    }
};

// Database exception classes
class DatabaseError : public std::runtime_error {
public:
    explicit DatabaseError(const std::string& message)
        : std::runtime_error("Database Error: " + message) {
    }
};

class TableNotFoundError : public DatabaseError {
public:
    explicit TableNotFoundError(const std::string& tableName)
        : DatabaseError("Table '" + tableName + "' not found") {
    }
};

class TableAlreadyExistsError : public DatabaseError {
public:
    explicit TableAlreadyExistsError(const std::string& tableName)
        : DatabaseError("Table '" + tableName + "' already exists") {
    }
};

} // namespace tinydb
