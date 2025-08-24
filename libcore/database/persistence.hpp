#pragma once

#include "database.hpp"
#include "value.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace tinydb {

/**
 * @brief Database persistence manager
 *
 * Responsible for exporting database to files and restoring database from files
 * Uses JSON format for storage, convenient for debugging and cross-platform compatibility
 */
class PersistenceManager {
public:
    /**
     * @brief Export database to file
     * @param database Database to export
     * @param filename Target filename
     * @throws std::runtime_error If export fails
     */
    static void exportDatabase(const Database& database, const std::string& filename);

    /**
     * @brief Restore database from file
     * @param filename Source filename
     * @return Restored database object
     * @throws std::runtime_error If import fails
     */
    static Database importDatabase(const std::string& filename);

    /**
     * @brief Export single table to string (JSON format)
     * @param table Table to export
     * @param tableName Table name
     * @return JSON string
     */
    static std::string exportTableToJson(const Table& table, const std::string& tableName);

    /**
     * @brief Import table from JSON string
     * @param json JSON string
     * @param database Target database
     * @param tableName Table name
     */
    static void importTableFromJson(const std::string& json, Database& database,
                                    const std::string& tableName);

    /**
     * @brief Convert Value object to JSON string (public method, for testing)
     * @param value Value object
     * @return JSON string
     */
    static std::string valueToJson(const Value& value);

    /**
     * @brief Parse Value object from JSON string (public method, for testing)
     * @param json JSON string
     * @param type Data type
     * @return Value object
     */
    static Value jsonToValue(const std::string& json, DataType type);

private:
    /**
     * @brief Escape JSON string
     * @param str Original string
     * @return Escaped string
     */
    static std::string escapeJsonString(const std::string& str);

    /**
     * @brief Unescape JSON string
     * @param str Escaped string
     * @return Original string
     */
    static std::string unescapeJsonString(const std::string& str);

    /**
     * @brief Parse string value from JSON object
     * @param json JSON string
     * @param key Key name
     * @return String value
     */
    static std::string parseJsonString(const std::string& json, const std::string& key);

    /**
     * @brief Parse integer value from JSON object
     * @param json JSON string
     * @param key Key name
     * @return Integer value
     */
    static int parseJsonInt(const std::string& json, const std::string& key);

    /**
     * @brief Parse array content from JSON object
     * @param json JSON string
     * @param key Key name
     * @return Array content string
     */
    static std::string parseJsonArray(const std::string& json, const std::string& key);
};

/**
 * @brief Persistence exception class
 */
class PersistenceError : public std::runtime_error {
public:
    explicit PersistenceError(const std::string& message)
        : std::runtime_error("Persistence Error: " + message) {
    }
};

} // namespace tinydb
