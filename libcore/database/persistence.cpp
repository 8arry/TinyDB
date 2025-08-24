#include "persistence.hpp"
#include <iomanip>
#include <iostream>

namespace tinydb {

void PersistenceManager::exportDatabase(const Database& database, const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw PersistenceError("Cannot open file for writing: " + filename);
        }

        file << "{\n";
        file << "  \"format_version\": \"1.0\",\n";
        file << "  \"database_name\": \"TinyDB\",\n";
        file << "  \"tables\": {\n";

        // Get all table names
        auto tableNames = database.getTableNames();

        for (size_t i = 0; i < tableNames.size(); ++i) {
            const auto& tableName = tableNames[i];
            const auto& table = database.getTable(tableName);

            file << "    \"" << tableName << "\": ";
            file << exportTableToJson(table, tableName);

            if (i < tableNames.size() - 1) {
                file << ",";
            }
            file << "\n";
        }

        file << "  }\n";
        file << "}\n";

        file.close();

        std::cout << "Database exported successfully to: " << filename << std::endl;
        std::cout << "Exported " << tableNames.size() << " table(s)" << std::endl;

    } catch (const std::exception& e) {
        throw PersistenceError("Export failed: " + std::string(e.what()));
    }
}

Database PersistenceManager::importDatabase(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw PersistenceError("Cannot open file for reading: " + filename);
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        if (content.empty()) {
            throw PersistenceError("File is empty: " + filename);
        }

        Database database;

        // Simple JSON parsing - find tables object
        auto tablesStart = content.find("\"tables\": {");
        if (tablesStart == std::string::npos) {
            throw PersistenceError("Invalid file format: missing 'tables' section");
        }

        tablesStart = content.find("{", tablesStart);
        auto tablesEnd = content.rfind("}");

        if (tablesStart == std::string::npos || tablesEnd == std::string::npos) {
            throw PersistenceError("Invalid JSON format");
        }

        std::string tablesContent = content.substr(tablesStart + 1, tablesEnd - tablesStart - 1);

        // Parse each table
        size_t pos = 0;
        while (pos < tablesContent.length()) {
            // Find table name
            auto nameStart = tablesContent.find("\"", pos);
            if (nameStart == std::string::npos)
                break;

            auto nameEnd = tablesContent.find("\"", nameStart + 1);
            if (nameEnd == std::string::npos)
                break;

            std::string tableName = tablesContent.substr(nameStart + 1, nameEnd - nameStart - 1);

            // Find table's JSON content
            auto colonPos = tablesContent.find(":", nameEnd);
            if (colonPos == std::string::npos)
                break;

            auto tableStart = tablesContent.find("{", colonPos);
            if (tableStart == std::string::npos)
                break;

            // Find matching right brace
            int braceCount = 1;
            size_t tableEnd = tableStart + 1;
            while (tableEnd < tablesContent.length() && braceCount > 0) {
                if (tablesContent[tableEnd] == '{')
                    braceCount++;
                else if (tablesContent[tableEnd] == '}')
                    braceCount--;
                tableEnd++;
            }

            std::string tableJson = tablesContent.substr(tableStart, tableEnd - tableStart);

            // Import table
            importTableFromJson(tableJson, database, tableName);

            pos = tableEnd;
        }

        std::cout << "Database imported successfully from: " << filename << std::endl;
        std::cout << "Imported " << database.getTableNames().size() << " table(s)" << std::endl;

        return database;

    } catch (const std::exception& e) {
        throw PersistenceError("Import failed: " + std::string(e.what()));
    }
}

std::string PersistenceManager::exportTableToJson(const Table& table,
                                                  const std::string& tableName) {
    std::ostringstream json;

    json << "{\n";
    json << "      \"name\": \"" << tableName << "\",\n";

    // Export table structure
    const auto& schema = table.getSchema();
    json << "      \"schema\": [\n";

    for (size_t i = 0; i < schema.size(); ++i) {
        const auto& column = schema[i];
        json << "        {\n";
        json << "          \"name\": \"" << column.name << "\",\n";
        json << "          \"type\": \"" << (column.type == DataType::INT ? "int" : "str")
             << "\"\n";
        json << "        }";

        if (i < schema.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "      ],\n";

    // Export data
    const auto& rows = table.getAllRows();
    json << "      \"data\": [\n";

    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        const auto& values = row.getValues();

        json << "        [";

        for (size_t j = 0; j < values.size(); ++j) {
            json << valueToJson(values[j]);

            if (j < values.size() - 1) {
                json << ", ";
            }
        }

        json << "]";

        if (i < rows.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "      ]\n";
    json << "    }";

    return json.str();
}

void PersistenceManager::importTableFromJson(const std::string& json, Database& database,
                                             const std::string& tableName) {
    try {
        // Parse table structure
        std::string schemaStr = parseJsonArray(json, "schema");
        std::vector<Column> columns;

        // Simple parse schema array
        size_t pos = 0;
        while (pos < schemaStr.length()) {
            auto objStart = schemaStr.find("{", pos);
            if (objStart == std::string::npos)
                break;

            auto objEnd = schemaStr.find("}", objStart);
            if (objEnd == std::string::npos)
                break;

            std::string objStr = schemaStr.substr(objStart, objEnd - objStart + 1);

            std::string columnName = parseJsonString(objStr, "name");
            std::string columnType = parseJsonString(objStr, "type");

            DataType type = (columnType == "int") ? DataType::INT : DataType::STR;
            columns.push_back({columnName, type});

            pos = objEnd + 1;
        }

        // Create table
        database.createTable(tableName, columns);

        // Parse data
        std::string dataStr = parseJsonArray(json, "data");

        // Parse data rows
        pos = 0;
        while (pos < dataStr.length()) {
            auto arrayStart = dataStr.find("[", pos);
            if (arrayStart == std::string::npos)
                break;

            auto arrayEnd = dataStr.find("]", arrayStart);
            if (arrayEnd == std::string::npos)
                break;

            std::string rowStr = dataStr.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

            // Parse values in the row
            std::vector<Value> values;
            size_t valuePos = 0;
            size_t columnIndex = 0;

            while (valuePos < rowStr.length() && columnIndex < columns.size()) {
                // Skip whitespace and commas
                while (valuePos < rowStr.length() &&
                       (rowStr[valuePos] == ' ' || rowStr[valuePos] == ',')) {
                    valuePos++;
                }

                if (valuePos >= rowStr.length())
                    break;

                // Parse value
                std::string valueStr;
                if (rowStr[valuePos] == '"') {
                    // String value
                    auto stringEnd = rowStr.find("\"", valuePos + 1);
                    if (stringEnd != std::string::npos) {
                        valueStr = rowStr.substr(valuePos + 1, stringEnd - valuePos - 1);
                        valuePos = stringEnd + 1;
                    }
                } else {
                    // Numeric value
                    auto nextComma = rowStr.find(",", valuePos);
                    if (nextComma == std::string::npos) {
                        valueStr = rowStr.substr(valuePos);
                        valuePos = rowStr.length();
                    } else {
                        valueStr = rowStr.substr(valuePos, nextComma - valuePos);
                        valuePos = nextComma;
                    }

                    // Remove whitespace
                    while (!valueStr.empty() && valueStr.back() == ' ') {
                        valueStr.pop_back();
                    }
                }

                // Create Value object
                Value value = jsonToValue(valueStr, columns[columnIndex].type);
                values.push_back(value);
                columnIndex++;
            }

            // Insert row
            if (!values.empty()) {
                database.insertInto(tableName, values);
            }

            pos = arrayEnd + 1;
        }

    } catch (const std::exception& e) {
        throw PersistenceError("Failed to import table '" + tableName +
                               "': " + std::string(e.what()));
    }
}

std::string PersistenceManager::valueToJson(const Value& value) {
    if (value.getType() == DataType::INT) {
        return value.toString();
    } else {
        return "\"" + escapeJsonString(value.toString()) + "\"";
    }
}

Value PersistenceManager::jsonToValue(const std::string& json, DataType type) {
    if (type == DataType::INT) {
        try {
            int intValue = std::stoi(json);
            return Value(intValue);
        } catch (const std::exception&) {
            throw PersistenceError("Invalid integer value: " + json);
        }
    } else {
        return Value(unescapeJsonString(json));
    }
}

std::string PersistenceManager::escapeJsonString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 2);

    for (char c : str) {
        switch (c) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
        }
    }

    return escaped;
}

std::string PersistenceManager::unescapeJsonString(const std::string& str) {
    std::string unescaped;
    unescaped.reserve(str.length());

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"':
                    unescaped += '"';
                    i++;
                    break;
                case '\\':
                    unescaped += '\\';
                    i++;
                    break;
                case 'n':
                    unescaped += '\n';
                    i++;
                    break;
                case 'r':
                    unescaped += '\r';
                    i++;
                    break;
                case 't':
                    unescaped += '\t';
                    i++;
                    break;
                default:
                    unescaped += str[i];
                    break;
            }
        } else {
            unescaped += str[i];
        }
    }

    return unescaped;
}

std::string PersistenceManager::parseJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    auto keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        throw PersistenceError("Key not found: " + key);
    }

    auto valueStart = json.find("\"", keyPos + searchKey.length());
    if (valueStart == std::string::npos) {
        throw PersistenceError("Invalid string value for key: " + key);
    }

    auto valueEnd = json.find("\"", valueStart + 1);
    if (valueEnd == std::string::npos) {
        throw PersistenceError("Unterminated string value for key: " + key);
    }

    return json.substr(valueStart + 1, valueEnd - valueStart - 1);
}

int PersistenceManager::parseJsonInt(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    auto keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        throw PersistenceError("Key not found: " + key);
    }

    auto valueStart = keyPos + searchKey.length();
    while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) {
        valueStart++;
    }

    auto valueEnd = valueStart;
    while (valueEnd < json.length() && json[valueEnd] != ',' && json[valueEnd] != '}' &&
           json[valueEnd] != '\n') {
        valueEnd++;
    }

    std::string valueStr = json.substr(valueStart, valueEnd - valueStart);

    try {
        return std::stoi(valueStr);
    } catch (const std::exception&) {
        throw PersistenceError("Invalid integer value for key: " + key);
    }
}

std::string PersistenceManager::parseJsonArray(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    auto keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        throw PersistenceError("Key not found: " + key);
    }

    auto arrayStart = json.find("[", keyPos);
    if (arrayStart == std::string::npos) {
        throw PersistenceError("Array not found for key: " + key);
    }

    // Find matching right bracket
    int bracketCount = 1;
    size_t arrayEnd = arrayStart + 1;
    while (arrayEnd < json.length() && bracketCount > 0) {
        if (json[arrayEnd] == '[')
            bracketCount++;
        else if (json[arrayEnd] == ']')
            bracketCount--;
        arrayEnd++;
    }

    return json.substr(arrayStart + 1, arrayEnd - arrayStart - 2);
}

} // namespace tinydb
