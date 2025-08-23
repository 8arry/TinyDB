#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include "database.hpp"
#include "value.hpp"

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
     * @param table 要导出的表
     * @param tableName 表名
     * @return JSON格式的字符串
     */
    static std::string exportTableToJson(const Table& table, const std::string& tableName);
    
    /**
     * @brief 从JSON字符串导入表
     * @param json JSON格式的字符串
     * @param database 目标数据库
     * @param tableName 表名
     */
    static void importTableFromJson(const std::string& json, Database& database, const std::string& tableName);

    /**
     * @brief 将Value对象转换为JSON字符串 (公共方法，用于测试)
     * @param value Value对象
     * @return JSON格式的字符串
     */
    static std::string valueToJson(const Value& value);
    
    /**
     * @brief 从JSON字符串解析Value对象 (公共方法，用于测试)
     * @param json JSON字符串
     * @param type 数据类型
     * @return Value对象
     */
    static Value jsonToValue(const std::string& json, DataType type);

private:
    
    /**
     * @brief 转义JSON字符串
     * @param str 原始字符串
     * @return 转义后的字符串
     */
    static std::string escapeJsonString(const std::string& str);
    
    /**
     * @brief 反转义JSON字符串
     * @param str 转义后的字符串
     * @return 原始字符串
     */
    static std::string unescapeJsonString(const std::string& str);
    
    /**
     * @brief 解析JSON对象中的字符串值
     * @param json JSON字符串
     * @param key 键名
     * @return 字符串值
     */
    static std::string parseJsonString(const std::string& json, const std::string& key);
    
    /**
     * @brief 解析JSON对象中的整数值
     * @param json JSON字符串
     * @param key 键名
     * @return 整数值
     */
    static int parseJsonInt(const std::string& json, const std::string& key);
    
    /**
     * @brief 解析JSON数组
     * @param json JSON字符串
     * @param key 键名
     * @return 数组内容字符串
     */
    static std::string parseJsonArray(const std::string& json, const std::string& key);
};

/**
 * @brief 持久化异常类
 */
class PersistenceError : public std::runtime_error {
public:
    explicit PersistenceError(const std::string& message) 
        : std::runtime_error("Persistence Error: " + message) {}
};

} // namespace tinydb
