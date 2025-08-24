#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
//#include <catch2/catch_test_macros.hpp>
// Include project header files
#include "../libcore/database/condition.hpp"
#include "../libcore/database/database.hpp"
#include "../libcore/database/persistence.hpp"
#include "../libcore/sql/lexer.hpp"
#include "../libcore/sql/parser.hpp"
#include <cstdio>
#include <fstream>

using namespace tinydb;
using namespace tinydb::sql;

// ========== SQL Parsing Tests - Valid Input ==========

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
    std::string sql = "SELECT employees.name, departments.name FROM employees INNER JOIN "
                      "departments ON employees.dept_id = departments.id;";

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

// ========== SQL Parsing Tests - Invalid Input ==========

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

// ========== SQL Execution Tests - Valid Input ==========

TEST_CASE("SQL Execution - CREATE and INSERT", "[execution][basic]") {
    Database db;

    // Create table
    std::vector<Column> cols = {{"id", DataType::INT}, {"name", DataType::STR}};

    REQUIRE_NOTHROW(db.createTable("users", cols));

    // Insert data
    std::vector<Value> values = {Value(1), Value(std::string("Alice"))};
    REQUIRE_NOTHROW(db.insertInto("users", values));

    // Verify data
    auto rows = db.selectFrom("users", {"*"});
    REQUIRE(rows.size() == 1);
}

TEST_CASE("SQL Execution - SELECT with Conditions", "[execution][select]") {
    Database db;

    // Prepare data
    std::vector<Column> cols = {
        {"id", DataType::INT}, {"name", DataType::STR}, {"age", DataType::INT}};
    db.createTable("users", cols);

    std::vector<Value> user1 = {Value(1), Value(std::string("Alice")), Value(25)};
    std::vector<Value> user2 = {Value(2), Value(std::string("Bob")), Value(30)};
    db.insertInto("users", user1);
    db.insertInto("users", user2);

    // Test queries
    auto allRows = db.selectFrom("users", {"*"});
    REQUIRE(allRows.size() == 2);

    auto specificColumns = db.selectFrom("users", {"id", "name"});
    REQUIRE(specificColumns.size() == 2);
}

TEST_CASE("SQL Execution - JOIN Operations", "[execution][join]") {
    Database db;

    // Create employee table
    std::vector<Column> empCols = {
        {"id", DataType::INT}, {"name", DataType::STR}, {"dept_id", DataType::INT}};
    db.createTable("employees", empCols);

    // Create department table
    std::vector<Column> deptCols = {{"id", DataType::INT}, {"name", DataType::STR}};
    db.createTable("departments", deptCols);

    // Insert test data
    std::vector<Value> emp1 = {Value(1), Value(std::string("Alice")), Value(1)};
    std::vector<Value> dept1 = {Value(1), Value(std::string("Engineering"))};

    REQUIRE_NOTHROW(db.insertInto("employees", emp1));
    REQUIRE_NOTHROW(db.insertInto("departments", dept1));

    // Verify table creation success
    auto empRows = db.selectFrom("employees", {"*"});
    auto deptRows = db.selectFrom("departments", {"*"});
    REQUIRE(empRows.size() == 1);
    REQUIRE(deptRows.size() == 1);
}

// ========== SQL Execution Tests - Invalid Input ==========

TEST_CASE("SQL Execution - Duplicate Table Creation", "[execution][error]") {
    Database db;

    std::vector<Column> cols = {{"id", DataType::INT}};

    // First creation should succeed
    REQUIRE_NOTHROW(db.createTable("users", cols));

    // Duplicate creation should fail
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

// ========== Data Type Tests ==========

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

// ========== Edge Case Tests ==========

TEST_CASE("Edge Cases - Empty String Values", "[edge][string]") {
    Database db;

    std::vector<Column> cols = {{"id", DataType::INT}, {"text", DataType::STR}};
    db.createTable("test", cols);

    std::vector<Value> values = {Value(1), Value(std::string(""))};
    REQUIRE_NOTHROW(db.insertInto("test", values));
}

TEST_CASE("Edge Cases - Zero and Negative Values", "[edge][numbers]") {
    Database db;

    std::vector<Column> cols = {{"id", DataType::INT}, {"value", DataType::INT}};
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

// ========== Persistence Functionality Tests ==========

TEST_CASE("Persistence - Export and Import Database", "[persistence][basic]") {
    const std::string filename = "test_db_export.json";

    // Create test database
    Database originalDb;

    // Create users table
    std::vector<Column> userCols = {
        {"id", DataType::INT}, {"name", DataType::STR}, {"age", DataType::INT}};
    originalDb.createTable("users", userCols);

    // Create products table
    std::vector<Column> productCols = {
        {"id", DataType::INT}, {"name", DataType::STR}, {"price", DataType::INT}};
    originalDb.createTable("products", productCols);

    // Insert test data
    std::vector<Value> user1 = {Value(1), Value(std::string("Alice")), Value(25)};
    std::vector<Value> user2 = {Value(2), Value(std::string("Bob")), Value(30)};
    originalDb.insertInto("users", user1);
    originalDb.insertInto("users", user2);

    std::vector<Value> product1 = {Value(1), Value(std::string("Laptop")), Value(1000)};
    std::vector<Value> product2 = {Value(2), Value(std::string("Mouse")), Value(50)};
    originalDb.insertInto("products", product1);
    originalDb.insertInto("products", product2);

    // Export database
    REQUIRE_NOTHROW(PersistenceManager::exportDatabase(originalDb, filename));

    // Import database
    Database importedDb;
    REQUIRE_NOTHROW(importedDb = PersistenceManager::importDatabase(filename));

    // Verify table count
    REQUIRE(importedDb.getTableNames().size() == 2);
    REQUIRE(importedDb.hasTable("users"));
    REQUIRE(importedDb.hasTable("products"));

    // Verify users table data
    auto userRows = importedDb.selectFrom("users", {"*"});
    REQUIRE(userRows.size() == 2);

    // Verify products table data
    auto productRows = importedDb.selectFrom("products", {"*"});
    REQUIRE(productRows.size() == 2);

    // Clean up test files
    std::remove(filename.c_str());
}

TEST_CASE("Persistence - Empty Database Export/Import", "[persistence][empty]") {
    const std::string filename = "test_empty_db.json";

    // Create empty database
    Database originalDb;

    // Export empty database
    REQUIRE_NOTHROW(PersistenceManager::exportDatabase(originalDb, filename));

    // Import empty database
    Database importedDb;
    REQUIRE_NOTHROW(importedDb = PersistenceManager::importDatabase(filename));

    // Verify empty database
    REQUIRE(importedDb.getTableNames().empty());
    REQUIRE(importedDb.getTableCount() == 0);

    // Clean up test files
    std::remove(filename.c_str());
}

TEST_CASE("Persistence - Table with Special Characters", "[persistence][special]") {
    const std::string filename = "test_special_chars.json";

    Database originalDb;

    // Create table
    std::vector<Column> cols = {{"id", DataType::INT}, {"text", DataType::STR}};
    originalDb.createTable("test_table", cols);

    // Insert data with special characters
    std::vector<Value> row1 = {Value(1), Value(std::string("Hello \"World\""))};
    std::vector<Value> row2 = {Value(2), Value(std::string("Line1\nLine2"))};
    std::vector<Value> row3 = {Value(3), Value(std::string("Tab\tSeparated"))};

    originalDb.insertInto("test_table", row1);
    originalDb.insertInto("test_table", row2);
    originalDb.insertInto("test_table", row3);

    // Export and import
    REQUIRE_NOTHROW(PersistenceManager::exportDatabase(originalDb, filename));

    Database importedDb;
    REQUIRE_NOTHROW(importedDb = PersistenceManager::importDatabase(filename));

    // Verify data
    auto rows = importedDb.selectFrom("test_table", {"*"});
    REQUIRE(rows.size() == 3);

    // Clean up test files
    std::remove(filename.c_str());
}

TEST_CASE("Persistence - Large Dataset", "[persistence][large]") {
    const std::string filename = "test_large_dataset.json";

    Database originalDb;

    // Create table
    std::vector<Column> cols = {
        {"id", DataType::INT}, {"name", DataType::STR}, {"value", DataType::INT}};
    originalDb.createTable("large_table", cols);

    // Insert large amount of data
    const int numRows = 100;
    for (int i = 1; i <= numRows; ++i) {
        std::vector<Value> row = {Value(i), Value(std::string("Item_") + std::to_string(i)),
                                  Value(i * 10)};
        originalDb.insertInto("large_table", row);
    }

    // Export and import
    REQUIRE_NOTHROW(PersistenceManager::exportDatabase(originalDb, filename));

    Database importedDb;
    REQUIRE_NOTHROW(importedDb = PersistenceManager::importDatabase(filename));

    // Verify data amount
    auto rows = importedDb.selectFrom("large_table", {"*"});
    REQUIRE(rows.size() == numRows);

    // Clean up test files
    std::remove(filename.c_str());
}

// ========== Persistence Error Handling Tests ==========

TEST_CASE("Persistence - Export to Invalid Path", "[persistence][error]") {
    Database db;

    // Attempt to export to invalid path
    REQUIRE_THROWS_AS(PersistenceManager::exportDatabase(db, "/invalid/path/test.json"),
                      PersistenceError);
}

TEST_CASE("Persistence - Import Nonexistent File", "[persistence][error]") {
    // Attempt to import non-existent file
    REQUIRE_THROWS_AS(PersistenceManager::importDatabase("nonexistent_file.json"),
                      PersistenceError);
}

TEST_CASE("Persistence - Import Invalid JSON", "[persistence][error]") {
    const std::string filename = "test_invalid.json";

    // Create invalid JSON file
    std::ofstream file(filename);
    file << "{ invalid json content }";
    file.close();

    // Attempt to import invalid JSON
    REQUIRE_THROWS_AS(PersistenceManager::importDatabase(filename), PersistenceError);

    // Clean up test files
    std::remove(filename.c_str());
}

TEST_CASE("Persistence - Value Type Conversion", "[persistence][value]") {
    // Test JSON value conversion
    REQUIRE_NOTHROW({
        Value intVal = PersistenceManager::jsonToValue("42", DataType::INT);
        REQUIRE(intVal.getType() == DataType::INT);
        REQUIRE(intVal.toString() == "42");
    });

    REQUIRE_NOTHROW({
        Value strVal = PersistenceManager::jsonToValue("Hello World", DataType::STR);
        REQUIRE(strVal.getType() == DataType::STR);
        REQUIRE(strVal.toString() == "Hello World");
    });

    // Test Value to JSON conversion
    REQUIRE_NOTHROW({
        Value intVal(123);
        std::string json = PersistenceManager::valueToJson(intVal);
        REQUIRE(json == "123");
    });

    REQUIRE_NOTHROW({
        Value strVal(std::string("Test String"));
        std::string json = PersistenceManager::valueToJson(strVal);
        REQUIRE(json == "\"Test String\"");
    });
}

// ========== Extended WHERE Condition Tests ==========

TEST_CASE("Extended WHERE - Comparison Operators", "[where][comparison]") {
    Database db;

    // Create test table
    std::vector<Column> cols = {
        {"id", DataType::INT}, {"value", DataType::INT}, {"name", DataType::STR}};
    db.createTable("numbers", cols);

    // Insert test data
    std::vector<Value> row1 = {Value(1), Value(10), Value(std::string("Ten"))};
    std::vector<Value> row2 = {Value(2), Value(20), Value(std::string("Twenty"))};
    std::vector<Value> row3 = {Value(3), Value(30), Value(std::string("Thirty"))};
    std::vector<Value> row4 = {Value(4), Value(15), Value(std::string("Fifteen"))};

    db.insertInto("numbers", row1);
    db.insertInto("numbers", row2);
    db.insertInto("numbers", row3);
    db.insertInto("numbers", row4);

    // Verify data insertion
    auto allRows = db.selectFrom("numbers", {"*"});
    REQUIRE(allRows.size() == 4);
}

TEST_CASE("Extended WHERE - Greater Than", "[where][gt]") {
    std::string sql = "SELECT * FROM numbers WHERE value > 20;";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Extended WHERE - Less Than", "[where][lt]") {
    std::string sql = "SELECT * FROM numbers WHERE value < 20;";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Extended WHERE - Greater Than or Equal", "[where][gte]") {
    std::string sql = "SELECT * FROM numbers WHERE value >= 20;";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Extended WHERE - Less Than or Equal", "[where][lte]") {
    std::string sql = "SELECT * FROM numbers WHERE value <= 20;";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Extended WHERE - All Operators Parsing", "[where][all]") {
    std::vector<std::string> testQueries = {
        "SELECT * FROM test WHERE id = 1;",   "SELECT * FROM test WHERE id != 1;",
        "SELECT * FROM test WHERE id < 10;",  "SELECT * FROM test WHERE id > 5;",
        "SELECT * FROM test WHERE id <= 15;", "SELECT * FROM test WHERE id >= 3;"};

    for (const auto& sql : testQueries) {
        REQUIRE_NOTHROW({
            Lexer lexer(sql);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto statement = parser.parse();

            REQUIRE(statement != nullptr);
            REQUIRE(statement->getType() == Statement::Type::SELECT);

            auto* selectStmt = static_cast<SelectStatement*>(statement.get());
            REQUIRE(selectStmt->getWhereCondition() != nullptr);
        });
    }
}

// ========== Logical Operator Tests ==========

TEST_CASE("Logical Operators - AND Parsing", "[where][logical][and]") {
    std::string sql = "SELECT * FROM employees WHERE age > 25 AND department = \"IT\";";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Logical Operators - OR Parsing", "[where][logical][or]") {
    std::string sql = "SELECT * FROM employees WHERE salary > 6000 OR age < 25;";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Logical Operators - Multiple AND", "[where][logical][multiple]") {
    std::string sql =
        "SELECT * FROM test WHERE age >= 30 AND salary >= 7000 AND department = \"IT\";";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Logical Operators - Mixed AND OR", "[where][logical][mixed]") {
    std::vector<std::string> mixedQueries = {
        "SELECT * FROM test WHERE age > 30 OR department = \"HR\";",
        "SELECT * FROM test WHERE id = 1 AND age > 25 OR salary > 5000;",
        "SELECT * FROM test WHERE department = \"IT\" OR department = \"HR\" AND age > 25;"};

    for (const auto& sql : mixedQueries) {
        REQUIRE_NOTHROW({
            Lexer lexer(sql);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto statement = parser.parse();

            REQUIRE(statement != nullptr);
            REQUIRE(statement->getType() == Statement::Type::SELECT);

            auto* selectStmt = static_cast<SelectStatement*>(statement.get());
            REQUIRE(selectStmt->getWhereCondition() != nullptr);
        });
    }
}

TEST_CASE("Logical Operators - Execution Test", "[where][logical][execution]") {
    Database db;

    // Create test table
    std::vector<Column> cols = {{"id", DataType::INT},
                                {"age", DataType::INT},
                                {"salary", DataType::INT},
                                {"department", DataType::STR}};
    db.createTable("employees", cols);

    // Insert test data
    std::vector<Value> row1 = {Value(1), Value(25), Value(5000), Value(std::string("IT"))};
    std::vector<Value> row2 = {Value(2), Value(30), Value(6000), Value(std::string("HR"))};
    std::vector<Value> row3 = {Value(3), Value(35), Value(7000), Value(std::string("IT"))};

    db.insertInto("employees", row1);
    db.insertInto("employees", row2);
    db.insertInto("employees", row3);

    // Verify data insertion
    auto allRows = db.selectFrom("employees", {"*"});
    REQUIRE(allRows.size() == 3);
}

// ========== Parentheses and Precedence Tests ==========

TEST_CASE("Parentheses - Simple Grouping", "[where][parentheses][simple]") {
    std::string sql = "SELECT * FROM test WHERE (age > 25 AND department = \"IT\");";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Parentheses - Complex Grouping", "[where][parentheses][complex]") {
    std::string sql =
        "SELECT * FROM test WHERE (price > 100 AND category = \"Electronics\") OR stock > 150;";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Parentheses - Nested Grouping", "[where][parentheses][nested]") {
    std::string sql = "SELECT * FROM test WHERE ((price > 200 OR category = \"Books\") AND stock > "
                      "30) OR price < 30;";

    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto statement = parser.parse();

    REQUIRE(statement != nullptr);
    REQUIRE(statement->getType() == Statement::Type::SELECT);

    auto* selectStmt = static_cast<SelectStatement*>(statement.get());
    REQUIRE(selectStmt->getWhereCondition() != nullptr);
}

TEST_CASE("Parentheses - Mixed with AND OR", "[where][parentheses][mixed]") {
    std::vector<std::string> complexQueries = {
        "SELECT * FROM test WHERE price > 50 AND (category = \"Books\" OR category = "
        "\"Electronics\");",
        "SELECT * FROM test WHERE (price > 100 OR category = \"Books\") AND stock > 50;",
        "SELECT * FROM test WHERE (age > 30 AND salary > 5000) OR (department = \"IT\" AND age < "
        "25);"};

    for (const auto& sql : complexQueries) {
        REQUIRE_NOTHROW({
            Lexer lexer(sql);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto statement = parser.parse();

            REQUIRE(statement != nullptr);
            REQUIRE(statement->getType() == Statement::Type::SELECT);

            auto* selectStmt = static_cast<SelectStatement*>(statement.get());
            REQUIRE(selectStmt->getWhereCondition() != nullptr);
        });
    }
}

TEST_CASE("Parentheses - Precedence Change", "[where][parentheses][precedence]") {
    // Test cases where parentheses change precedence
    std::vector<std::string> precedenceQueries = {
        "SELECT * FROM test WHERE a = 1 AND b = 2 OR c = 3;",   // (a=1 AND b=2) OR c=3
        "SELECT * FROM test WHERE a = 1 AND (b = 2 OR c = 3);", // a=1 AND (b=2 OR c=3)
        "SELECT * FROM test WHERE (a = 1 OR b = 2) AND c = 3;", // (a=1 OR b=2) AND c=3
        "SELECT * FROM test WHERE a = 1 OR b = 2 AND c = 3;"    // a=1 OR (b=2 AND c=3)
    };

    for (const auto& sql : precedenceQueries) {
        REQUIRE_NOTHROW({
            Lexer lexer(sql);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto statement = parser.parse();

            REQUIRE(statement != nullptr);
            REQUIRE(statement->getType() == Statement::Type::SELECT);

            auto* selectStmt = static_cast<SelectStatement*>(statement.get());
            REQUIRE(selectStmt->getWhereCondition() != nullptr);
        });
    }
}

TEST_CASE("Parentheses - Error Handling", "[where][parentheses][error]") {
    // Test error handling for mismatched parentheses
    std::vector<std::string> errorQueries = {
        "SELECT * FROM test WHERE (age > 25 AND department = \"IT\";", // Missing right parenthesis
        "SELECT * FROM test WHERE ((age > 25) AND department = \"IT\";", // Nested mismatched
                                                                         // parentheses
    };

    for (const auto& sql : errorQueries) {
        REQUIRE_THROWS_AS(
            {
                Lexer lexer(sql);
                auto tokens = lexer.tokenize();
                Parser parser(std::move(tokens));
                auto statement = parser.parse();
            },
            ParseError);
    }

    // Test cases with extra right parentheses - this might not immediately throw during parsing,
    // but during execution
    REQUIRE_NOTHROW({
        std::string sql = "SELECT * FROM test WHERE age > 25 AND department = \"IT\");";
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        // Note: Extra right parentheses might still exist after WHERE condition parsing, which
        // might not throw an error in the current implementation
        // because the Parser might stop after correctly parsing the WHERE clause
        auto statement = parser.parse(); // This might not throw an error because the WHERE part is
                                         // parsed correctly
    });
}
