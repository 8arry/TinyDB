#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <cstdlib>
#include "libcore/database/database.hpp"
#include "libcore/database/condition.hpp"
#include "libcore/database/persistence.hpp"
#include "libcore/sql/lexer.hpp"
#include "libcore/sql/parser.hpp"

#ifdef HAS_PRINTLN
#include <print>
#endif

using namespace tinydb;

// ASCII table output formatter
class TableFormatter {
public:
    static void printTable(const std::vector<std::string>& columnNames, 
                          const std::vector<tinydb::Row>& rows) {
        if (columnNames.empty()) return;
        
        // Calculate maximum width for each column
        std::vector<size_t> columnWidths(columnNames.size());
        for (size_t i = 0; i < columnNames.size(); ++i) {
            columnWidths[i] = columnNames[i].length();
        }
        
        // Check width of data rows
        for (const auto& row : rows) {
            for (size_t i = 0; i < std::min(row.size(), columnWidths.size()); ++i) {
                std::string cellValue = row[i].toString();
                columnWidths[i] = std::max(columnWidths[i], cellValue.length());
            }
        }
        
        // Print table
        printSeparator(columnWidths);
        printRow(columnNames, columnWidths);
        printSeparator(columnWidths);
        
        for (const auto& row : rows) {
            std::vector<std::string> stringRow;
            for (const auto& value : row) {
                stringRow.push_back(value.toString());
            }
            printRow(stringRow, columnWidths);
        }
        
        printSeparator(columnWidths);
    }

private:
    static void printSeparator(const std::vector<size_t>& widths) {
        std::cout << "+";
        for (size_t width : widths) {
            std::cout << std::string(width + 2, '-') << "+";
        }
        std::cout << std::endl;
    }
    
    static void printRow(const std::vector<std::string>& cells, 
                        const std::vector<size_t>& widths) {
        std::cout << "|";
        for (size_t i = 0; i < cells.size() && i < widths.size(); ++i) {
            std::cout << " " << std::left << std::setw(widths[i]) << cells[i] << " |";
        }
        std::cout << std::endl;
    }
};

// SQL Executor
class SQLExecutor {
private:
    tinydb::Database db_;

public:
    void execute(const std::string& sql) {
        try {
            // Simple SQL parsing and execution
            if (sql.empty() || sql.find_first_not_of(" \t\n\r") == std::string::npos) {
                return; // Skip empty lines
            }
            

            
            // Lexical analysis
            sql::Lexer lexer(sql);
            auto tokens = lexer.tokenize();
            
            // Syntax analysis
            sql::Parser parser(std::move(tokens));
            auto statement = parser.parse();
            
            // Execute statement (if exists)
            if (statement) {
                executeStatement(statement.get());
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

private:
    void executeStatement(sql::Statement* stmt) {
        switch (stmt->getType()) {
            case sql::Statement::Type::CREATE_TABLE:
                executeCreateTable(static_cast<sql::CreateTableStatement*>(stmt));
                break;
                
            case sql::Statement::Type::INSERT:
                executeInsert(static_cast<sql::InsertStatement*>(stmt));
                break;
                
            case sql::Statement::Type::SELECT:
                executeSelect(static_cast<sql::SelectStatement*>(stmt));
                break;
                
            case sql::Statement::Type::UPDATE:
                executeUpdate(static_cast<sql::UpdateStatement*>(stmt));
                break;
                
            case sql::Statement::Type::DELETE:
                executeDelete(static_cast<sql::DeleteStatement*>(stmt));
                break;
        }
    }
    
    void executeCreateTable(sql::CreateTableStatement* stmt) {
        db_.createTable(stmt->getTableName(), stmt->getColumns());
        // CREATE TABLE has no output, silent success
    }
    
    void executeInsert(sql::InsertStatement* stmt) {
        std::vector<tinydb::Value> values;
        for (const auto& expr : stmt->getValues()) {
            values.push_back(expr->evaluate());
        }
        
        if (stmt->getColumns().empty()) {
            // INSERT INTO table VALUES (...)
            db_.insertInto(stmt->getTableName(), values);
        } else {
            // INSERT INTO table (col1, col2) VALUES (...)
            // Need to reorder values by column names, simplified version not implemented
            db_.insertInto(stmt->getTableName(), values);
        }
    }
    
    void executeSelect(sql::SelectStatement* stmt) {
        if (stmt->hasJoins()) {
            executeSelectWithJoin(stmt);
        } else {
            executeSelectSimple(stmt);
        }
    }
    
    void executeSelectSimple(sql::SelectStatement* stmt) {
        // Simple SELECT (no JOIN)
        std::function<bool(const tinydb::Row&, const tinydb::Table&)> condition = nullptr;
        if (stmt->getWhereCondition()) {
            condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
        }
        
        // Determine columns to select
        std::vector<std::string> selectColumns;
        std::vector<std::string> displayColumnNames;
        
        if (stmt->isSelectAll()) {
            // SELECT *
            selectColumns = {"*"};
            auto& table = db_.getTable(stmt->getTableName());
            auto schema = table.getSchema();
            for (const auto& col : schema) {
                displayColumnNames.push_back(col.name);
            }
        } else {
            // SELECT col1, col2 - need to handle qualified column names
            selectColumns = stmt->getColumns();
            displayColumnNames = stmt->getColumns();
            
            // Handle qualified column names: convert "table.column" to "column"
            for (auto& colName : selectColumns) {
                size_t dotPos = colName.find('.');
                if (dotPos != std::string::npos) {
                    colName = colName.substr(dotPos + 1); // Keep only column name part
                }
            }
        }
        
        auto rows = condition ? 
            db_.selectFrom(stmt->getTableName(), selectColumns, condition) :
            db_.selectFrom(stmt->getTableName(), selectColumns);
        
        // Print ASCII table
        TableFormatter::printTable(displayColumnNames, rows);
    }
    
    void executeSelectWithJoin(sql::SelectStatement* stmt) {
        // JOIN query implementation
        auto& mainTable = db_.getTable(stmt->getTableName());
        auto mainRows = db_.selectFrom(stmt->getTableName(), {"*"});
        
        // Build result rows
        std::vector<tinydb::Row> resultRows;
        
        // Process JOIN for each main table row
        for (const auto& mainRow : mainRows) {
            executeJoinForRow(stmt, mainTable, mainRow, resultRows);
        }
        
        // Apply WHERE conditions
        if (stmt->getWhereCondition()) {
            auto condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
            auto filteredRows = std::vector<tinydb::Row>();
            for (const auto& row : resultRows) {
                // Create temporary table for JOIN result to evaluate conditions
                // Merge schemas of all related tables
                auto allColumnNames = buildJoinColumnNames(stmt);
                std::vector<tinydb::Column> combinedSchema;
                
                // Build combined schema (simplified: set all as int type, only for column name lookup)
                for (const auto& colName : allColumnNames) {
                    size_t dotPos = colName.find('.');
                    std::string actualColName = (dotPos != std::string::npos) ? 
                        colName.substr(dotPos + 1) : colName;
                    combinedSchema.push_back({actualColName, tinydb::DataType::INT});
                }
                
                tinydb::Table tempTable("temp", combinedSchema);
                if (condition(row, tempTable)) {
                    filteredRows.push_back(row);
                }
            }
            resultRows = std::move(filteredRows);
        }
        
        // Build column names and selected columns
        std::vector<std::string> allColumnNames = buildJoinColumnNames(stmt);
        std::vector<std::string> selectedColumns;
        

        
        if (stmt->isSelectAll()) {
            selectedColumns = allColumnNames;
        } else {
            selectedColumns = stmt->getColumns();
        }
        
        // Extract selected columns
        std::vector<tinydb::Row> finalRows = extractSelectedColumns(resultRows, allColumnNames, selectedColumns);
        
        // Print results
        TableFormatter::printTable(selectedColumns, finalRows);
    }
    
    void executeJoinForRow(sql::SelectStatement* stmt, const tinydb::Table& mainTable, 
                          const tinydb::Row& mainRow, std::vector<tinydb::Row>& resultRows) {
        // Recursively process JOIN chain
        if (stmt->getJoins().empty()) {
            resultRows.push_back(mainRow);
            return;
        }
        
        // Process first JOIN (simplified: only support single JOIN)
        const auto& joinClause = stmt->getJoins()[0];
        auto& joinTable = db_.getTable(joinClause->getTableName());
        auto joinRows = db_.selectFrom(joinClause->getTableName(), {"*"});
        
        // Match rows for each JOIN table
        for (const auto& joinRow : joinRows) {
            if (evaluateJoinCondition(*joinClause->getOnCondition(), mainTable, mainRow, joinTable, joinRow)) {
                // Merge rows
                auto combinedRow = combineRows(mainTable, mainRow, joinTable, joinRow);
                resultRows.push_back(combinedRow);
            }
        }
    }
    
    bool evaluateJoinCondition(const tinydb::Condition& condition, 
                              const tinydb::Table& leftTable, const tinydb::Row& leftRow,
                              const tinydb::Table& rightTable, const tinydb::Row& rightRow) {
        // Create combined row to evaluate condition
        auto combinedRow = combineRows(leftTable, leftRow, rightTable, rightRow);
        
        // Use ConditionAdapter to evaluate condition
        auto lambda = tinydb::ConditionAdapter::toLambda(condition);
        // Merge schemas of two tables to create temporary table schema
        auto leftSchema = leftTable.getSchema();
        auto rightSchema = rightTable.getSchema();
        std::vector<tinydb::Column> combinedSchema;
        
        // Add left table columns
        for (const auto& col : leftSchema) {
            combinedSchema.push_back(col);
        }
        // Add right table columns
        for (const auto& col : rightSchema) {
            combinedSchema.push_back(col);
        }
        
        tinydb::Table tempTable("temp", combinedSchema);
        return lambda(combinedRow, tempTable);
    }
    
    tinydb::Row combineRows(const tinydb::Table& /* leftTable */, const tinydb::Row& leftRow,
                           const tinydb::Table& /* rightTable */, const tinydb::Row& rightRow) {
        std::vector<tinydb::Value> combinedValues;
        
        // Add all columns from left table
        for (const auto& value : leftRow.getValues()) {
            combinedValues.push_back(value);
        }
        
        // Add all columns from right table
        for (const auto& value : rightRow.getValues()) {
            combinedValues.push_back(value);
        }
        
        return tinydb::Row(combinedValues);
    }
    
    std::vector<std::string> buildJoinColumnNames(sql::SelectStatement* stmt) {
        std::vector<std::string> columnNames;
        
        // Add main table column names
        auto& mainTable = db_.getTable(stmt->getTableName());
        auto mainSchema = mainTable.getSchema();
        for (const auto& col : mainSchema) {
            columnNames.push_back(stmt->getTableName() + "." + col.name);
        }
        
        // Add JOIN table column names
        for (const auto& joinClause : stmt->getJoins()) {
            auto& joinTable = db_.getTable(joinClause->getTableName());
            auto joinSchema = joinTable.getSchema();
            for (const auto& col : joinSchema) {
                columnNames.push_back(joinClause->getTableName() + "." + col.name);
            }
        }
        
        return columnNames;
    }
    
    std::vector<tinydb::Row> extractSelectedColumns(const std::vector<tinydb::Row>& rows,
                                                   const std::vector<std::string>& allColumnNames,
                                                   const std::vector<std::string>& selectedColumns) {
        if (selectedColumns.empty() || rows.empty()) {
            return rows;
        }
        
        // If no matching columns found, return empty result
        if (allColumnNames.empty()) {
            return {};
        }
        
        // Find indices of selected columns
        std::vector<size_t> selectedIndices;
        for (const auto& selectedCol : selectedColumns) {
            auto it = std::find(allColumnNames.begin(), allColumnNames.end(), selectedCol);
            if (it != allColumnNames.end()) {
                selectedIndices.push_back(std::distance(allColumnNames.begin(), it));
            }
        }
        
        // If no matching columns found, return empty row instead of empty result
        if (selectedIndices.empty()) {
            return {};
        }
        
        // Extract selected columns
        std::vector<tinydb::Row> result;
        for (const auto& row : rows) {
            std::vector<tinydb::Value> selectedValues;
            const auto& allValues = row.getValues();
            
            for (size_t index : selectedIndices) {
                if (index < allValues.size()) {
                    selectedValues.push_back(allValues[index]);
                }
            }
            
            // Only add row when there are valid values
            if (!selectedValues.empty()) {
                result.emplace_back(selectedValues);
            }
        }
        
        return result;
    }
    
    void executeUpdate(sql::UpdateStatement* stmt) {
        // Build update mapping
        std::unordered_map<std::string, tinydb::Value> updates;
        for (const auto& assignment : stmt->getAssignments()) {
            updates[assignment.first] = assignment.second->evaluate();
        }
        
        // Build WHERE condition
        std::function<bool(const tinydb::Row&, const tinydb::Table&)> condition = nullptr;
        if (stmt->getWhereCondition()) {
            condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
        }
        
        db_.updateTable(stmt->getTableName(), updates, condition);
    }
    
    void executeDelete(sql::DeleteStatement* stmt) {
        // Build WHERE condition
        std::function<bool(const tinydb::Row&, const tinydb::Table&)> condition = nullptr;
        if (stmt->getWhereCondition()) {
            condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
        }
        
        db_.deleteFrom(stmt->getTableName(), condition);
    }
    
public:
    // Persistence support methods
    const Database& getDatabase() const { return db_; }
    
    void replaceDatabase(Database&& newDb) {
        db_ = std::move(newDb);
        std::cout << "Database replaced successfully." << std::endl;
    }
};

// Handle special commands (import/export, etc.)
bool handleSpecialCommand(const std::string& command, SQLExecutor& executor) {
    // Trim leading/trailing whitespace
    std::string trimmed = command;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    // Convert to lowercase for comparison
    std::string lowercaseCmd = trimmed;
    std::transform(lowercaseCmd.begin(), lowercaseCmd.end(), lowercaseCmd.begin(), ::tolower);
    
    try {
        // EXPORT DATABASE command
        if (lowercaseCmd.substr(0, 15) == "export database") {
            size_t toPos = lowercaseCmd.find(" to ");
            if (toPos != std::string::npos) {
                std::string filename = trimmed.substr(toPos + 4);
                // Remove quotes
                if (filename.front() == '"' && filename.back() == '"') {
                    filename = filename.substr(1, filename.length() - 2);
                }
                
                PersistenceManager::exportDatabase(executor.getDatabase(), filename);
                return true;
            }
        }
        
        // IMPORT DATABASE command
        else if (lowercaseCmd.substr(0, 15) == "import database") {
            size_t fromPos = lowercaseCmd.find(" from ");
            if (fromPos != std::string::npos) {
                std::string filename = trimmed.substr(fromPos + 6);
                // Remove quotes
                if (filename.front() == '"' && filename.back() == '"') {
                    filename = filename.substr(1, filename.length() - 2);
                }
                
                try {
                    Database newDb = PersistenceManager::importDatabase(filename);
                    executor.replaceDatabase(std::move(newDb));
                    return true;
                } catch (const std::exception& e) {
                    std::cout << "Import failed: " << e.what() << std::endl;
                    return true; // Command handled, but failed
                }
            }
        }
        
        // HELP command
        else if (lowercaseCmd == "help" || lowercaseCmd == "\\h") {
            std::cout << "\n=== TinyDB Help ===" << std::endl;
            std::cout << "SQL Commands:" << std::endl;
            std::cout << "  CREATE TABLE name (col1 type1, col2 type2, ...);" << std::endl;
            std::cout << "  INSERT INTO table VALUES (val1, val2, ...);" << std::endl;
            std::cout << "  SELECT col1, col2 FROM table [WHERE condition];" << std::endl;
            std::cout << "  SELECT * FROM table1 INNER JOIN table2 ON condition;" << std::endl;
            std::cout << "  UPDATE table SET col=val WHERE condition;" << std::endl;
            std::cout << "  DELETE FROM table WHERE condition;" << std::endl;
            std::cout << "\nWHERE Conditions:" << std::endl;
            std::cout << "  Comparison: =, !=, <, >, <=, >=" << std::endl;
            std::cout << "  Logical: AND, OR" << std::endl;
            std::cout << "  Grouping: ( ) parentheses for precedence" << std::endl;
            std::cout << "  Examples:" << std::endl;
            std::cout << "    WHERE age > 18 AND department = \"IT\"" << std::endl;
            std::cout << "    WHERE (price > 100 AND category = \"Electronics\") OR stock > 150" << std::endl;
            std::cout << "    WHERE price > 50 AND (category = \"Books\" OR category = \"IT\")" << std::endl;
            std::cout << "\nPersistence Commands:" << std::endl;
            std::cout << "  EXPORT DATABASE TO \"filename.json\";" << std::endl;
            std::cout << "  IMPORT DATABASE FROM \"filename.json\";" << std::endl;
            std::cout << "\nOther Commands:" << std::endl;
            std::cout << "  HELP or \\h - Show this help" << std::endl;
            std::cout << "  QUIT or \\q - Exit the program" << std::endl;
            std::cout << "\nData Types: int, str" << std::endl;
            std::cout << "===================" << std::endl;
            return true;
        }
        
        // QUIT command
        else if (lowercaseCmd == "quit" || lowercaseCmd == "\\q" || lowercaseCmd == "exit") {
            std::cout << "Goodbye!" << std::endl;
            exit(0);
        }
        
    } catch (const std::exception& e) {
        std::cout << "Command failed: " << e.what() << std::endl;
        return true;
    }
    
    return false; // Not a special command
}

int main() {
    SQLExecutor executor;
    std::string line;
    std::string currentStatement;
    
#ifdef HAS_PRINTLN
    std::println("TinyDB - In-Memory Database");
    std::println("Enter SQL statements (end with semicolon):");
#else
    std::cout << "TinyDB - In-Memory Database" << std::endl;
    std::cout << "Enter SQL statements (end with semicolon):" << std::endl;
#endif
    
    while (std::getline(std::cin, line)) {
        currentStatement += line + " ";
        
        // Find semicolon to end statement
        size_t semicolonPos = currentStatement.find(';');
        while (semicolonPos != std::string::npos) {
            std::string sql = currentStatement.substr(0, semicolonPos);
            
            // Handle special command or execute SQL statement
            if (!handleSpecialCommand(sql, executor)) {
                executor.execute(sql);
            }
            
            // Remove executed part
            currentStatement = currentStatement.substr(semicolonPos + 1);
            semicolonPos = currentStatement.find(';');
        }
    }
    
    return 0;
}
