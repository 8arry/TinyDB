#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

// 包含项目头文件
#include "../libcore/database/database.hpp"
#include "../libcore/sql/lexer.hpp"
#include "../libcore/sql/parser.hpp"
#include "../libcore/database/condition.hpp"

using namespace tinydb;
using namespace tinydb::sql;

// ========== SQL 解析测试 - 正确输入 ==========

TEST_CASE("SQL Parser - Valid CREATE TABLE", "[parser][create]") {
    std::string sql = "CREATE TABLE users (id int, name str);";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();
    
    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::CREATE_TABLE);
    
    auto* createStmt = static_cast<CreateTableStatement*>(statement.get());
    REQUIRE(createStmt->getTableName() == "users");
    
    const auto& columns = createStmt->getColumns();
    REQUIRE(columns.size() == 2);
    REQUIRE(columns[0].name == "id");
    REQUIRE(columns[0].type == DataType::INT);
    REQUIRE(columns[1].name == "name");
    REQUIRE(columns[1].type == DataType::STR);
}

TEST_CASE("SQL Parser - Valid INSERT", "[parser][insert]") {
    std::string sql = "INSERT INTO users VALUES (1, \"Alice\");";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();
    
    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::INSERT);
    
    auto* insertStmt = static_cast<InsertStatement*>(statement.get());
    REQUIRE(insertStmt->getTableName() == "users");
    REQUIRE(insertStmt->getValues().size() == 2);
}

TEST_CASE("SQL Parser - Valid SELECT with WHERE", "[parser][select]") {
    std::string sql = "SELECT id, name FROM users WHERE id = 1;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();
    
    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);
    
    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getTableName() == "users");
    REQUIRE_FALSE(selectStmt->isSelectAll());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
    
    const auto& columns = selectStmt->getColumns();
    REQUIRE(columns.size() == 2);
    REQUIRE(columns[0] == "id");
    REQUIRE(columns[1] == "name");
}

TEST_CASE("SQL Parser - Valid INNER JOIN", "[parser][join]") {
    std::string sql = "SELECT employees.name, departments.name FROM employees INNER JOIN departments ON employees.dept_id = departments.id;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();
    
    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);
    
    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->hasJoins());
    
    const auto& joins = selectStmt->getJoins();
    REQUIRE(joins.size() == 1);
    REQUIRE(joins[0]->getTableName() == "departments");
    REQUIRE(joins[0]->getJoinType() == JoinClause::JoinType::INNER);
    REQUIRE(joins[0]->getOnCondition() != nullptr);
}

TEST_CASE("SQL Parser - Valid UPDATE", "[parser][update]") {
    std::string sql = "UPDATE users SET name = \"Bob\" WHERE id = 1;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();
    
    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::UPDATE);
    
    auto* updateStmt = static_cast<UpdateStatement*>(statement.get());
    REQUIRE(updateStmt->getTableName() == "users");
    REQUIRE(updateStmt->getWhereCondition() != nullptr);
}

TEST_CASE("SQL Parser - Valid DELETE", "[parser][delete]") {
    std::string sql = "DELETE FROM users WHERE id = 1;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();
    
    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::DELETE);
    
    auto* deleteStmt = static_cast<DeleteStatement*>(statement.get());
    REQUIRE(deleteStmt->getTableName() == "users");
    REQUIRE(deleteStmt->getWhereCondition() != nullptr);
}

// ========== SQL 解析测试 - 错误输入 ==========

TEST_CASE("SQL Parser - Invalid Syntax", "[parser][error]") {
    std::string sql = "INVALID SQL STATEMENT";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    
    REQUIRE_THROWS_AS(parser.parse(), std::exception);
}

TEST_CASE("SQL Parser - Missing Table Name", "[parser][error]") {
    std::string sql = "SELECT * FROM;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    
    REQUIRE_THROWS_AS(parser.parse(), std::exception);
}

TEST_CASE("SQL Parser - Missing VALUES", "[parser][error]") {
    std::string sql = "INSERT INTO users VALUES;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    
    REQUIRE_THROWS_AS(parser.parse(), std::exception);
}

TEST_CASE("SQL Parser - Invalid Data Type", "[parser][error]") {
    std::string sql = "CREATE TABLE users (id invalid_type);";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    
    REQUIRE_THROWS_AS(parser.parse(), std::exception);
}

TEST_CASE("SQL Parser - Missing ON Clause in JOIN", "[parser][error]") {
    std::string sql = "SELECT * FROM users INNER JOIN departments;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    
    REQUIRE_THROWS_AS(parser.parse(), std::exception);
}

// ========== SQL 执行测试 - 正确输入 ==========

TEST_CASE("SQL Execution - CREATE and INSERT", "[execution][basic]") {
    Database db;
    
    // 创建表
    std::vector<Column> cols = {
        {"id", DataType::INT},
        {"name", DataType::STR}
    };
    
    REQUIRE_NOTHROW(db.createTable("users", cols));
    
    // 插入数据
    std::vector<Value> values = {Value(1), Value(std::string("Alice"))};
    REQUIRE_NOTHROW(db.insertInto("users", values));
    
    // 验证数据
    auto rows = db.selectFrom("users", {"*"});
    REQUIRE(rows.size() == 1);
}

TEST_CASE("SQL Execution - SELECT with Conditions", "[execution][select]") {
    Database db;
    
    // 准备数据
    std::vector<Column> cols = {
        {"id", DataType::INT},
        {"name", DataType::STR},
        {"age", DataType::INT}
    };
    db.createTable("users", cols);
    
    std::vector<Value> user1 = {Value(1), Value(std::string("Alice")), Value(25)};
    std::vector<Value> user2 = {Value(2), Value(std::string("Bob")), Value(30)};
    db.insertInto("users", user1);
    db.insertInto("users", user2);
    
    // 测试查询
    auto allRows = db.selectFrom("users", {"*"});
    REQUIRE(allRows.size() == 2);
    
    auto specificColumns = db.selectFrom("users", {"id", "name"});
    REQUIRE(specificColumns.size() == 2);
}

TEST_CASE("SQL Execution - JOIN Operations", "[execution][join]") {
    Database db;
    
    // 创建员工表
    std::vector<Column> empCols = {
        {"id", DataType::INT},
        {"name", DataType::STR},
        {"dept_id", DataType::INT}
    };
    db.createTable("employees", empCols);
    
    // 创建部门表
    std::vector<Column> deptCols = {
        {"id", DataType::INT},
        {"name", DataType::STR}
    };
    db.createTable("departments", deptCols);
    
    // 插入测试数据
    std::vector<Value> emp1 = {Value(1), Value(std::string("Alice")), Value(1)};
    std::vector<Value> dept1 = {Value(1), Value(std::string("Engineering"))};
    
    REQUIRE_NOTHROW(db.insertInto("employees", emp1));
    REQUIRE_NOTHROW(db.insertInto("departments", dept1));
    
    // 验证表创建成功
    auto empRows = db.selectFrom("employees", {"*"});
    auto deptRows = db.selectFrom("departments", {"*"});
    REQUIRE(empRows.size() == 1);
    REQUIRE(deptRows.size() == 1);
}

// ========== SQL 执行测试 - 错误输入 ==========

TEST_CASE("SQL Execution - Duplicate Table Creation", "[execution][error]") {
    Database db;
    
    std::vector<Column> cols = {{"id", DataType::INT}};
    
    // 第一次创建应该成功
    REQUIRE_NOTHROW(db.createTable("users", cols));
    
    // 重复创建应该失败
    REQUIRE_THROWS_AS(db.createTable("users", cols), std::exception);
}

TEST_CASE("SQL Execution - Insert into Non-existent Table", "[execution][error]") {
    Database db;
    
    std::vector<Value> values = {Value(1), Value(std::string("Alice"))};
    
    REQUIRE_THROWS_AS(db.insertInto("nonexistent", values), std::exception);
}

TEST_CASE("SQL Execution - Select from Non-existent Table", "[execution][error]") {
    Database db;
    
    REQUIRE_THROWS_AS(db.selectFrom("nonexistent", {"*"}), std::exception);
}

// ========== 数据类型测试 ==========

TEST_CASE("Value Types - Integer Operations", "[value][int]") {
    Value intValue(42);
    
    REQUIRE(intValue.getType() == DataType::INT);
    REQUIRE(intValue.toString() == "42");
}

TEST_CASE("Value Types - String Operations", "[value][string]") {
    Value strValue(std::string("Hello World"));
    
    REQUIRE(strValue.getType() == DataType::STR);
    REQUIRE(strValue.toString() == "Hello World");
}

TEST_CASE("Value Types - Default Values", "[value][default]") {
    Value defaultInt = Value::getDefault(DataType::INT);
    Value defaultStr = Value::getDefault(DataType::STR);
    
    REQUIRE(defaultInt.getType() == DataType::INT);
    REQUIRE(defaultStr.getType() == DataType::STR);
    
    REQUIRE(defaultInt.toString() == "0");
    REQUIRE(defaultStr.toString() == "");
}

// ========== 边界情况测试 ==========

TEST_CASE("Edge Cases - Empty String Values", "[edge][string]") {
    Database db;
    
    std::vector<Column> cols = {
        {"id", DataType::INT},
        {"text", DataType::STR}
    };
    db.createTable("test", cols);
    
    std::vector<Value> values = {Value(1), Value(std::string(""))};
    REQUIRE_NOTHROW(db.insertInto("test", values));
}

TEST_CASE("Edge Cases - Zero and Negative Values", "[edge][numbers]") {
    Database db;
    
    std::vector<Column> cols = {
        {"id", DataType::INT},
        {"value", DataType::INT}
    };
    db.createTable("numbers", cols);
    
    std::vector<Value> zero = {Value(0), Value(0)};
    std::vector<Value> negative = {Value(-1), Value(-999)};
    
    REQUIRE_NOTHROW(db.insertInto("numbers", zero));
    REQUIRE_NOTHROW(db.insertInto("numbers", negative));
}

TEST_CASE("Edge Cases - Case Insensitive Keywords", "[edge][parser]") {
    std::string sql = "select * from USERS where ID = 1;";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();
    
    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);
}
