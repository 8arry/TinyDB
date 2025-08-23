#include "../libcore/sql/parser.hpp"
#include "../libcore/sql/lexer.hpp"
#include <iostream>
#include <cassert>

#ifdef HAS_PRINTLN
#include <print>
#endif

using namespace tinydb::sql;
// using namespace tinydb::database;  // 移除，因为会有冲突

void testCreateTableParsing() {
    std::cout << "\n=== Testing CREATE TABLE parsing ===" << std::endl;
    
    std::string sql = "CREATE TABLE users (id INT, name STRING, age INT)";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    
    Parser parser(std::move(tokens));
    auto stmt = parser.parse();
    
    assert(stmt != nullptr);
    assert(stmt->getType() == Statement::Type::CREATE_TABLE);
    
    auto createStmt = static_cast<const CreateTableStatement*>(stmt.get());
    assert(createStmt->getTableName() == "users");
    
    const auto& columns = createStmt->getColumns();
    assert(columns.size() == 3);
    assert(columns[0].name == "id" && columns[0].type == tinydb::database::DataType::INT);
    assert(columns[1].name == "name" && columns[1].type == tinydb::database::DataType::STR);
    assert(columns[2].name == "age" && columns[2].type == tinydb::database::DataType::INT);
    
    std::cout << "CREATE TABLE AST: " << stmt->toString() << std::endl;
    std::cout << "✓ CREATE TABLE parsing passed!" << std::endl;
}

void testInsertParsing() {
    std::cout << "\n=== Testing INSERT parsing ===" << std::endl;
    
    // 测试不指定列名的INSERT
    {
        std::string sql = "INSERT INTO users VALUES (1, 'Alice', 25)";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::INSERT);
        
        auto insertStmt = static_cast<const InsertStatement*>(stmt.get());
        assert(insertStmt->getTableName() == "users");
        assert(insertStmt->getColumns().empty());  // 没有指定列名
        assert(insertStmt->getValues().size() == 3);
        
        std::cout << "INSERT AST: " << stmt->toString() << std::endl;
    }
    
    // 测试指定列名的INSERT
    {
        std::string sql = "INSERT INTO users (name, age) VALUES ('Bob', 30)";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::INSERT);
        
        auto insertStmt = static_cast<const InsertStatement*>(stmt.get());
        assert(insertStmt->getTableName() == "users");
        assert(insertStmt->getColumns().size() == 2);
        assert(insertStmt->getColumns()[0] == "name");
        assert(insertStmt->getColumns()[1] == "age");
        assert(insertStmt->getValues().size() == 2);
        
        std::cout << "INSERT with columns AST: " << stmt->toString() << std::endl;
    }
    
    std::cout << "✓ INSERT parsing passed!" << std::endl;
}

void testSelectParsing() {
    std::cout << "\n=== Testing SELECT parsing ===" << std::endl;
    
    // 测试SELECT *
    {
        std::string sql = "SELECT * FROM users";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::SELECT);
        
        auto selectStmt = static_cast<const SelectStatement*>(stmt.get());
        assert(selectStmt->getTableName() == "users");
        assert(selectStmt->isSelectAll());
        assert(selectStmt->getWhereCondition() == nullptr);
        
        std::cout << "SELECT * AST: " << stmt->toString() << std::endl;
    }
    
    // 测试指定列的SELECT
    {
        std::string sql = "SELECT name, age FROM users";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::SELECT);
        
        auto selectStmt = static_cast<const SelectStatement*>(stmt.get());
        assert(selectStmt->getTableName() == "users");
        assert(!selectStmt->isSelectAll());
        assert(selectStmt->getColumns().size() == 2);
        assert(selectStmt->getColumns()[0] == "name");
        assert(selectStmt->getColumns()[1] == "age");
        
        std::cout << "SELECT columns AST: " << stmt->toString() << std::endl;
    }
    
    // 测试带WHERE的SELECT
    {
        std::string sql = "SELECT name FROM users WHERE age > 18";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::SELECT);
        
        auto selectStmt = static_cast<const SelectStatement*>(stmt.get());
        assert(selectStmt->getTableName() == "users");
        assert(selectStmt->getWhereCondition() != nullptr);
        
        std::cout << "SELECT with WHERE AST: " << stmt->toString() << std::endl;
    }
    
    std::cout << "✓ SELECT parsing passed!" << std::endl;
}

void testUpdateParsing() {
    std::cout << "\n=== Testing UPDATE parsing ===" << std::endl;
    
    // 测试简单UPDATE
    {
        std::string sql = "UPDATE users SET age = 26";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::UPDATE);
        
        auto updateStmt = static_cast<const UpdateStatement*>(stmt.get());
        assert(updateStmt->getTableName() == "users");
        assert(updateStmt->getAssignments().size() == 1);
        assert(updateStmt->getAssignments()[0].first == "age");
        assert(updateStmt->getWhereCondition() == nullptr);
        
        std::cout << "UPDATE AST: " << stmt->toString() << std::endl;
    }
    
    // 测试带WHERE的UPDATE
    {
        std::string sql = "UPDATE users SET name = 'Charlie', age = 35 WHERE id = 1";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::UPDATE);
        
        auto updateStmt = static_cast<const UpdateStatement*>(stmt.get());
        assert(updateStmt->getTableName() == "users");
        assert(updateStmt->getAssignments().size() == 2);
        assert(updateStmt->getAssignments()[0].first == "name");
        assert(updateStmt->getAssignments()[1].first == "age");
        assert(updateStmt->getWhereCondition() != nullptr);
        
        std::cout << "UPDATE with WHERE AST: " << stmt->toString() << std::endl;
    }
    
    std::cout << "✓ UPDATE parsing passed!" << std::endl;
}

void testDeleteParsing() {
    std::cout << "\n=== Testing DELETE parsing ===" << std::endl;
    
    // 测试简单DELETE
    {
        std::string sql = "DELETE FROM users";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::DELETE);
        
        auto deleteStmt = static_cast<const DeleteStatement*>(stmt.get());
        assert(deleteStmt->getTableName() == "users");
        assert(deleteStmt->getWhereCondition() == nullptr);
        
        std::cout << "DELETE AST: " << stmt->toString() << std::endl;
    }
    
    // 测试带WHERE的DELETE
    {
        std::string sql = "DELETE FROM users WHERE age < 18";
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(stmt != nullptr);
        assert(stmt->getType() == Statement::Type::DELETE);
        
        auto deleteStmt = static_cast<const DeleteStatement*>(stmt.get());
        assert(deleteStmt->getTableName() == "users");
        assert(deleteStmt->getWhereCondition() != nullptr);
        
        std::cout << "DELETE with WHERE AST: " << stmt->toString() << std::endl;
    }
    
    std::cout << "✓ DELETE parsing passed!" << std::endl;
}

void testComplexConditions() {
    std::cout << "\n=== Testing complex conditions ===" << std::endl;
    
    std::string sql = "SELECT * FROM users WHERE age > 18 AND name = 'Alice'";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    
    Parser parser(std::move(tokens));
    auto stmt = parser.parse();
    
    assert(stmt != nullptr);
    assert(stmt->getType() == Statement::Type::SELECT);
    
    auto selectStmt = static_cast<const SelectStatement*>(stmt.get());
    assert(selectStmt->getWhereCondition() != nullptr);
    
    std::cout << "Complex condition AST: " << stmt->toString() << std::endl;
    std::cout << "✓ Complex conditions parsing passed!" << std::endl;
}

void testErrorHandling() {
    std::cout << "\n=== Testing error handling ===" << std::endl;
    
    // 测试语法错误
    try {
        std::string sql = "CREATE TABEL users (id INT)";  // 错误的关键字
        
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        Parser parser(std::move(tokens));
        auto stmt = parser.parse();
        
        assert(false);  // 不应该到达这里
    } catch (const ParseError& e) {
        std::cout << "Caught expected parse error: " << e.what() << std::endl;
    }
    
    std::cout << "✓ Error handling passed!" << std::endl;
}

void testMultipleStatements() {
    std::cout << "\n=== Testing multiple statements ===" << std::endl;
    
    std::string sql = R"(
        CREATE TABLE users (id INT, name STRING);
        INSERT INTO users VALUES (1, 'Alice');
        SELECT * FROM users WHERE id = 1;
    )";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    
    Parser parser(std::move(tokens));
    auto statements = parser.parseMultiple();
    
    assert(statements.size() == 3);
    assert(statements[0]->getType() == Statement::Type::CREATE_TABLE);
    assert(statements[1]->getType() == Statement::Type::INSERT);
    assert(statements[2]->getType() == Statement::Type::SELECT);
    
    std::cout << "Multiple statements parsed:" << std::endl;
    for (size_t i = 0; i < statements.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << statements[i]->toString() << std::endl;
    }
    
    std::cout << "✓ Multiple statements parsing passed!" << std::endl;
}

void demonstrateParserCapabilities() {
    std::cout << "\n=== Parser Demonstration ===" << std::endl;
    
    std::vector<std::string> demo_sqls = {
        "CREATE TABLE products (id INT, name STRING, price INT)",
        "INSERT INTO products VALUES (1, 'Laptop', 1299)",
        "INSERT INTO products (name, price) VALUES ('Mouse', 25)",
        "SELECT * FROM products",
        "SELECT name, price FROM products WHERE price > 100",
        "UPDATE products SET price = 1199 WHERE id = 1",
        "DELETE FROM products WHERE price < 50"
    };
    
    std::cout << "Parsing various SQL statements:" << std::endl;
    
    for (size_t i = 0; i < demo_sqls.size(); ++i) {
        try {
            Lexer lexer(demo_sqls[i]);
            auto tokens = lexer.tokenize();
            
            Parser parser(std::move(tokens));
            auto stmt = parser.parse();
            
            std::cout << "\n" << (i + 1) << ". SQL: " << demo_sqls[i] << std::endl;
            std::cout << "   AST: " << stmt->toString() << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "\n" << (i + 1) << ". SQL: " << demo_sqls[i] << std::endl;
            std::cout << "   ERROR: " << e.what() << std::endl;
        }
    }
    
    std::cout << "\n✓ Parser demonstration completed!" << std::endl;
}

int main() {
    try {
#ifdef HAS_PRINTLN
        std::println("🚀 Starting TinyDB SQL Parser Tests...");
#else
        std::cout << "🚀 Starting TinyDB SQL Parser Tests..." << std::endl;
#endif
        
        testCreateTableParsing();
        testInsertParsing();
        testSelectParsing();
        testUpdateParsing();
        testDeleteParsing();
        testComplexConditions();
        testErrorHandling();
        testMultipleStatements();
        demonstrateParserCapabilities();
        
#ifdef HAS_PRINTLN
        std::println("\n🎉 All parser tests passed!");
#else
        std::cout << "\n🎉 All parser tests passed!" << std::endl;
#endif
        
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
