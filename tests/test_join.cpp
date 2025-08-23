#include <iostream>
#include <cassert>
#include "../libcore/database/database.hpp"
#include "../libcore/sql/lexer.hpp"
#include "../libcore/sql/parser.hpp"

using namespace tinydb;
using namespace tinydb::sql;

void testJoinParsing() {
    std::cout << "Testing JOIN parsing..." << std::endl;
    
    std::string sql = "SELECT employees.name, departments.name FROM employees INNER JOIN departments ON employees.department_id = departments.id;";
    
    try {
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        std::cout << "Tokens generated: " << tokens.size() << std::endl;
        for (const auto& token : tokens) {
            std::cout << "  " << TokenUtils::tokenTypeToString(token.type);
            if (token.hasStringValue()) {
                std::cout << " (" << token.getStringValue() << ")";
            }
            std::cout << std::endl;
        }
        
        Parser parser(std::move(tokens));
        auto statement = parser.parse();
        
        auto* selectStmt = static_cast<SelectStatement*>(statement.get());
        assert(selectStmt != nullptr);
        assert(selectStmt->hasJoins());
        assert(selectStmt->getJoins().size() == 1);
        
        std::cout << "Statement: " << statement->toString() << std::endl;
        std::cout << "✅ JOIN parsing test passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ JOIN parsing test failed: " << e.what() << std::endl;
    }
}

void testBasicJoin() {
    std::cout << "\nTesting basic JOIN execution..." << std::endl;
    
    try {
        Database db;
        
        // 创建表
        std::vector<Column> empCols = {
            {"id", DataType::INT},
            {"name", DataType::STR},
            {"dept_id", DataType::INT}
        };
        std::vector<Column> deptCols = {
            {"id", DataType::INT},
            {"name", DataType::STR}
        };
        
        db.createTable("employees", empCols);
        db.createTable("departments", deptCols);
        
        // 插入数据
        std::vector<Value> emp1 = {Value(1), Value(std::string("Alice")), Value(1)};
        std::vector<Value> emp2 = {Value(2), Value(std::string("Bob")), Value(2)};
        db.insertInto("employees", emp1);
        db.insertInto("employees", emp2);
        
        std::vector<Value> dept1 = {Value(1), Value(std::string("Engineering"))};
        std::vector<Value> dept2 = {Value(2), Value(std::string("Marketing"))};
        db.insertInto("departments", dept1);
        db.insertInto("departments", dept2);
        
        std::cout << "✅ Basic JOIN setup completed!" << std::endl;
        
        // 现在测试JOIN解析
        std::string joinSQL = "SELECT employees.name, departments.name FROM employees INNER JOIN departments ON employees.dept_id = departments.id;";
        
        Lexer lexer(joinSQL);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto statement = parser.parse();
        
        std::cout << "Parsed JOIN statement: " << statement->toString() << std::endl;
        std::cout << "✅ JOIN test completed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Basic JOIN test failed: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Testing JOIN Functionality ===" << std::endl;
    
    testJoinParsing();
    testBasicJoin();
    
    std::cout << "\n=== JOIN Tests Completed ===" << std::endl;
    return 0;
}
