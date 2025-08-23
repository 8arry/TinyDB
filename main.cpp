#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include "libcore/database/database.hpp"
#include "libcore/database/condition.hpp"
#include "libcore/sql/lexer.hpp"
#include "libcore/sql/parser.hpp"

#ifdef HAS_PRINTLN
#include <print>
#endif

using namespace tinydb;

// ASCII表格输出格式
class TableFormatter {
public:
    static void printTable(const std::vector<std::string>& columnNames, 
                          const std::vector<tinydb::Row>& rows) {
        if (columnNames.empty()) return;
        
        // 计算每列的最大宽度
        std::vector<size_t> columnWidths(columnNames.size());
        for (size_t i = 0; i < columnNames.size(); ++i) {
            columnWidths[i] = columnNames[i].length();
        }
        
        // 检查数据行的宽度
        for (const auto& row : rows) {
            for (size_t i = 0; i < std::min(row.size(), columnWidths.size()); ++i) {
                std::string cellValue = row[i].toString();
                columnWidths[i] = std::max(columnWidths[i], cellValue.length());
            }
        }
        
        // 打印表格
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

// SQL执行器
class SQLExecutor {
private:
    tinydb::Database db_;

public:
    void execute(const std::string& sql) {
        try {
            // 简单的SQL解析和执行
            if (sql.empty() || sql.find_first_not_of(" \t\n\r") == std::string::npos) {
                return; // 跳过空行
            }
            
            // 词法分析
            sql::Lexer lexer(sql);
            auto tokens = lexer.tokenize();
            
            // 语法分析
            sql::Parser parser(std::move(tokens));
            auto statement = parser.parse();
            
            // 执行语句
            executeStatement(statement.get());
            
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
        // CREATE TABLE 不输出内容，静默成功
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
            // 这里需要按列名顺序重排值，简化版本先不实现
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
        // 简单SELECT（无JOIN）
        std::function<bool(const tinydb::Row&, const tinydb::Table&)> condition = nullptr;
        if (stmt->getWhereCondition()) {
            condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
        }
        
        std::vector<std::string> selectColumns = {"*"}; // 默认选择所有列
        auto rows = condition ? 
            db_.selectFrom(stmt->getTableName(), selectColumns, condition) :
            db_.selectFrom(stmt->getTableName(), selectColumns);
        
        // 获取表结构以确定列名
        auto& table = db_.getTable(stmt->getTableName());
        auto schema = table.getSchema();
        
        std::vector<std::string> columnNames;
        if (stmt->isSelectAll()) {
            // SELECT *
            for (const auto& col : schema) {
                columnNames.push_back(col.name);
            }
        } else {
            // SELECT col1, col2
            columnNames = stmt->getColumns();
        }
        
        // 打印ASCII表格
        TableFormatter::printTable(columnNames, rows);
    }
    
    void executeSelectWithJoin(sql::SelectStatement* stmt) {
        // JOIN查询实现
        auto& mainTable = db_.getTable(stmt->getTableName());
        auto mainRows = db_.selectFrom(stmt->getTableName(), {"*"});
        
        // 构建结果行
        std::vector<tinydb::Row> resultRows;
        
        // 对每个主表行进行JOIN处理
        for (const auto& mainRow : mainRows) {
            executeJoinForRow(stmt, mainTable, mainRow, resultRows);
        }
        
        // 应用WHERE条件
        if (stmt->getWhereCondition()) {
            auto condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
            auto filteredRows = std::vector<tinydb::Row>();
            for (const auto& row : resultRows) {
                // 为JOIN结果创建一个临时表来评估条件
                tinydb::Table tempTable("temp", {});
                if (condition(row, tempTable)) {
                    filteredRows.push_back(row);
                }
            }
            resultRows = std::move(filteredRows);
        }
        
        // 构建列名和选择列
        std::vector<std::string> allColumnNames = buildJoinColumnNames(stmt);
        std::vector<std::string> selectedColumns;
        
        if (stmt->isSelectAll()) {
            selectedColumns = allColumnNames;
        } else {
            selectedColumns = stmt->getColumns();
        }
        
        // 提取选定的列
        std::vector<tinydb::Row> finalRows = extractSelectedColumns(resultRows, allColumnNames, selectedColumns);
        
        // 打印结果
        TableFormatter::printTable(selectedColumns, finalRows);
    }
    
    void executeJoinForRow(sql::SelectStatement* stmt, const tinydb::Table& mainTable, 
                          const tinydb::Row& mainRow, std::vector<tinydb::Row>& resultRows) {
        // 递归处理JOIN链
        if (stmt->getJoins().empty()) {
            resultRows.push_back(mainRow);
            return;
        }
        
        // 处理第一个JOIN（简化：只支持单个JOIN）
        const auto& joinClause = stmt->getJoins()[0];
        auto& joinTable = db_.getTable(joinClause->getTableName());
        auto joinRows = db_.selectFrom(joinClause->getTableName(), {"*"});
        
        // 对每个JOIN表的行进行匹配
        for (const auto& joinRow : joinRows) {
            if (evaluateJoinCondition(*joinClause->getOnCondition(), mainTable, mainRow, joinTable, joinRow)) {
                // 合并行
                auto combinedRow = combineRows(mainTable, mainRow, joinTable, joinRow);
                resultRows.push_back(combinedRow);
            }
        }
    }
    
    bool evaluateJoinCondition(const tinydb::Condition& condition, 
                              const tinydb::Table& leftTable, const tinydb::Row& leftRow,
                              const tinydb::Table& rightTable, const tinydb::Row& rightRow) {
        // 创建一个组合行来评估条件
        auto combinedRow = combineRows(leftTable, leftRow, rightTable, rightRow);
        
        // 使用ConditionAdapter评估条件
        auto lambda = tinydb::ConditionAdapter::toLambda(condition);
        tinydb::Table tempTable("temp", {});
        return lambda(combinedRow, tempTable);
    }
    
    tinydb::Row combineRows(const tinydb::Table& leftTable, const tinydb::Row& leftRow,
                           const tinydb::Table& rightTable, const tinydb::Row& rightRow) {
        std::vector<tinydb::Value> combinedValues;
        
        // 添加左表的所有列
        for (const auto& value : leftRow.getValues()) {
            combinedValues.push_back(value);
        }
        
        // 添加右表的所有列
        for (const auto& value : rightRow.getValues()) {
            combinedValues.push_back(value);
        }
        
        return tinydb::Row(combinedValues);
    }
    
    std::vector<std::string> buildJoinColumnNames(sql::SelectStatement* stmt) {
        std::vector<std::string> columnNames;
        
        // 添加主表列名
        auto& mainTable = db_.getTable(stmt->getTableName());
        auto mainSchema = mainTable.getSchema();
        for (const auto& col : mainSchema) {
            columnNames.push_back(stmt->getTableName() + "." + col.name);
        }
        
        // 添加JOIN表列名
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
        
        // 找到选中列的索引
        std::vector<size_t> selectedIndices;
        for (const auto& selectedCol : selectedColumns) {
            auto it = std::find(allColumnNames.begin(), allColumnNames.end(), selectedCol);
            if (it != allColumnNames.end()) {
                selectedIndices.push_back(std::distance(allColumnNames.begin(), it));
            }
        }
        
        // 提取选中的列
        std::vector<tinydb::Row> result;
        for (const auto& row : rows) {
            std::vector<tinydb::Value> selectedValues;
            const auto& allValues = row.getValues();
            
            for (size_t index : selectedIndices) {
                if (index < allValues.size()) {
                    selectedValues.push_back(allValues[index]);
                }
            }
            
            result.emplace_back(selectedValues);
        }
        
        return result;
    }
    
    void executeUpdate(sql::UpdateStatement* stmt) {
        // 构建更新映射
        std::unordered_map<std::string, tinydb::Value> updates;
        for (const auto& assignment : stmt->getAssignments()) {
            updates[assignment.first] = assignment.second->evaluate();
        }
        
        // 构建WHERE条件
        std::function<bool(const tinydb::Row&, const tinydb::Table&)> condition = nullptr;
        if (stmt->getWhereCondition()) {
            condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
        }
        
        db_.updateTable(stmt->getTableName(), updates, condition);
    }
    
    void executeDelete(sql::DeleteStatement* stmt) {
        // 构建WHERE条件
        std::function<bool(const tinydb::Row&, const tinydb::Table&)> condition = nullptr;
        if (stmt->getWhereCondition()) {
            condition = tinydb::ConditionAdapter::toLambda(*stmt->getWhereCondition());
        }
        
        db_.deleteFrom(stmt->getTableName(), condition);
    }
};

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
        
        // 查找分号结束语句
        size_t semicolonPos = currentStatement.find(';');
        while (semicolonPos != std::string::npos) {
            std::string sql = currentStatement.substr(0, semicolonPos);
            
            // 执行SQL语句
            executor.execute(sql);
            
            // 移除已执行的部分
            currentStatement = currentStatement.substr(semicolonPos + 1);
            semicolonPos = currentStatement.find(';');
        }
    }
    
    return 0;
}
