#include "libcore/database/database.hpp"
#include <iostream>
#include <cassert>

using namespace tinydb;

void testDatabaseCreation() {
    std::cout << "Testing Database creation...\n";
    
    try {
        // 测试默认构造
        Database db1;
        assert(db1.getName() == "TinyDB");
        assert(db1.getTableCount() == 0);
        assert(db1.empty());
        
        // 测试自定义名称构造
        Database db2("TestDB");
        assert(db2.getName() == "TestDB");
        assert(db2.empty());
        
        std::cout << "✅ Database creation tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Database creation failed: " << e.what() << std::endl;
        throw;
    }
}

void testTableManagement() {
    std::cout << "Testing table management...\n";
    
    try {
        Database db("TestDB");
        
        // 测试表创建
        std::vector<Column> userSchema = {
            {"id", DataType::INT},
            {"name", DataType::STR},
            {"age", DataType::INT}
        };
        
        db.createTable("users", std::move(userSchema));
        assert(db.getTableCount() == 1);
        assert(db.hasTable("users"));
        assert(!db.hasTable("nonexistent"));
        
        // 测试模板创建方法
        db.createTable("products", 
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"price", DataType::INT}
        );
        assert(db.getTableCount() == 2);
        
        // 测试表名获取
        auto tableNames = db.getTableNames();
        assert(tableNames.size() == 2);
        // 表名应该按字母顺序排序
        assert(tableNames[0] == "products");
        assert(tableNames[1] == "users");
        
        // 测试表访问
        const auto& usersTable = db.getTable("users");
        assert(usersTable.getName() == "users");
        assert(usersTable.getColumnCount() == 3);
        
        // 测试表删除
        bool deleted = db.dropTable("products");
        assert(deleted);
        assert(db.getTableCount() == 1);
        assert(!db.hasTable("products"));
        
        // 测试删除不存在的表
        bool notDeleted = db.dropTable("nonexistent");
        assert(!notDeleted);
        
        std::cout << "✅ Table management tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Table management failed: " << e.what() << std::endl;
        throw;
    }
}

void testDataOperations() {
    std::cout << "Testing data operations...\n";
    
    try {
        Database db("TestDB");
        
        // 创建测试表
        db.createTable("users",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"age", DataType::INT}
        );
        
        // 测试数据插入 - vector方法
        db.insertInto("users", {Value{1}, Value{"Alice"}, Value{25}});
        db.insertInto("users", {Value{2}, Value{"Bob"}, Value{30}});
        
        // 测试数据插入 - vector方法
        db.insertInto("users", {Value{3}, Value{"Charlie"}, Value{35}});
        
        // 测试数据插入 - Row方法
        Row row({Value{4}, Value{"Diana"}, Value{28}});
        db.insertInto("users", row);
        
        assert(db.getRowCount("users") == 4);
        assert(!db.isEmpty("users"));
        
        // 测试数据查询 - 所有列
        auto allUsers = db.selectFrom("users");
        assert(allUsers.size() == 4);
        assert(allUsers[0].size() == 3);
        
        // 测试数据查询 - 特定列
        auto names = db.selectFrom("users", {"name"});
        assert(names.size() == 4);
        assert(names[0].size() == 1);
        assert(names[0][0].asStringUnsafe() == "Alice");
        
        // 测试带条件查询
        auto youngUsers = db.selectFrom("users", {"name", "age"},
            [](const Row& row, const Table& /* table */) {
                return row[2].asIntUnsafe() < 30; // age < 30
            });
        
        assert(youngUsers.size() == 2); // Alice(25) and Diana(28)
        
        // 测试数据更新
        std::unordered_map<std::string, Value> updates = {
            {"age", Value{31}}
        };
        
        size_t updatedCount = db.updateTable("users", updates,
            [](const Row& row, const Table& /* table */) {
                return row[1].asStringUnsafe() == "Bob"; // name == "Bob"
            });
        
        assert(updatedCount == 1);
        
        // 验证更新
        auto bobData = db.selectFrom("users", {"age"},
            [](const Row& row, const Table& /* table */) {
                return row[1].asStringUnsafe() == "Bob";
            });
        assert(bobData.size() == 1);
        assert(bobData[0][0].asIntUnsafe() == 31);
        
        // 测试数据删除
        size_t deletedCount = db.deleteFrom("users",
            [](const Row& row, const Table& /* table */) {
                return row[2].asIntUnsafe() > 32; // age > 32
            });
        
        assert(deletedCount == 1); // Charlie (35)
        assert(db.getRowCount("users") == 3);
        
        std::cout << "✅ Data operations tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Data operations failed: " << e.what() << std::endl;
        throw;
    }
}

void testDatabaseStatistics() {
    std::cout << "Testing database statistics...\n";
    
    try {
        Database db("TestDB");
        
        // 创建多个表
        db.createTable("users",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR}
        );
        
        db.createTable("products",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"price", DataType::INT}
        );
        
        // 添加数据
        db.insertInto("users", {Value{1}, Value{"Alice"}});
        db.insertInto("users", {Value{2}, Value{"Bob"}});
        db.insertInto("products", {Value{1}, Value{"Laptop"}, Value{1000}});
        
        // 测试统计信息
        auto stats = db.getStats();
        assert(stats.tableCount == 2);
        assert(stats.totalRows == 3);
        assert(stats.totalColumns == 5); // 2 + 3
        assert(stats.tableRowCounts.size() == 2);
        
        // 测试实用方法
        assert(db.getColumnCount("users") == 2);
        assert(db.getColumnCount("products") == 3);
        assert(db.getRowCount("users") == 2);
        assert(db.getRowCount("products") == 1);
        
        // 测试清空表
        db.truncateTable("users");
        assert(db.isEmpty("users"));
        assert(db.getRowCount("users") == 0);
        assert(db.hasTable("users")); // 表结构仍然存在
        
        std::cout << "✅ Database statistics tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Database statistics failed: " << e.what() << std::endl;
        throw;
    }
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    try {
        Database db("TestDB");
        
        // 测试访问不存在的表
        try {
            db.getTable("nonexistent");
            assert(false && "Should have thrown TableNotFoundError");
        } catch (const TableNotFoundError& e) {
            // 预期的异常
        }
        
        // 测试重复创建表
        db.createTable("users", Column{"id", DataType::INT});
        
        try {
            db.createTable("users", Column{"id", DataType::INT});
            assert(false && "Should have thrown TableAlreadyExistsError");
        } catch (const TableAlreadyExistsError& e) {
            // 预期的异常
        }
        
        // 测试无效表名
        try {
            db.createTable("", Column{"id", DataType::INT});
            assert(false && "Should have thrown DatabaseError for empty name");
        } catch (const DatabaseError& e) {
            // 预期的异常
        }
        
        try {
            db.createTable("123invalid", Column{"id", DataType::INT});
            assert(false && "Should have thrown DatabaseError for invalid name");
        } catch (const DatabaseError& e) {
            // 预期的异常
        }
        
        // 测试空schema
        try {
            db.createTable("empty", {});
            assert(false && "Should have thrown DatabaseError for empty schema");
        } catch (const DatabaseError& e) {
            // 预期的异常
        }
        
        std::cout << "✅ Error handling tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Error handling failed: " << e.what() << std::endl;
        throw;
    }
}

void testDatabaseValidation() {
    std::cout << "Testing database validation...\n";
    
    try {
        Database db("TestDB");
        
        // 创建正常的表
        db.createTable("users",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR}
        );
        
        db.insertInto("users", {Value{1}, Value{"Alice"}});
        
        // 测试验证
        auto validation = db.validate();
        assert(validation.isValid);
        assert(validation.errors.empty());
        
        std::cout << "✅ Database validation tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Database validation failed: " << e.what() << std::endl;
        throw;
    }
}

void testTransaction() {
    std::cout << "Testing basic transaction...\n";
    
    try {
        Database db("TestDB");
        
        db.createTable("users",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR}
        );
        
        // 测试事务基础功能
        {
            auto transaction = db.beginTransaction();
            db.insertInto("users", {Value{1}, Value{"Alice"}});
            
            // 添加回滚操作
            transaction->addRollbackAction([&db]() {
                db.deleteFrom("users", [](const Row& row, const Table& /* table */) {
                    return row[0].asIntUnsafe() == 1;
                });
            });
            
            assert(db.getRowCount("users") == 1);
            
            // 提交事务
            transaction->commit();
        }
        
        assert(db.getRowCount("users") == 1);
        
        // 测试事务回滚
        {
            auto transaction = db.beginTransaction();
            db.insertInto("users", {Value{2}, Value{"Bob"}});
            
            transaction->addRollbackAction([&db]() {
                db.deleteFrom("users", [](const Row& row, const Table& /* table */) {
                    return row[0].asIntUnsafe() == 2;
                });
            });
            
            assert(db.getRowCount("users") == 2);
            
            // 不提交，让析构函数触发回滚
        }
        
        assert(db.getRowCount("users") == 1); // Bob应该被回滚
        
        std::cout << "✅ Transaction tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Transaction failed: " << e.what() << std::endl;
        throw;
    }
}

void testSnapshot() {
    std::cout << "Testing database snapshot...\n";
    
    try {
        Database db("TestDB");
        
        // 创建数据
        db.createTable("users",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR}
        );
        
        db.insertInto("users", {Value{1}, Value{"Alice"}});
        db.insertInto("users", {Value{2}, Value{"Bob"}});
        
        // 创建快照
        auto snapshot = db.createSnapshot();
        
        assert(snapshot.name == "TestDB");
        assert(snapshot.schemas.size() == 1);
        assert(snapshot.data.size() == 1);
        assert(snapshot.schemas[0].first == "users");
        assert(snapshot.schemas[0].second.size() == 2); // 2列
        assert(snapshot.data[0].first == "users");
        assert(snapshot.data[0].second.size() == 2); // 2行
        
        std::cout << "✅ Snapshot tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Snapshot failed: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "🧪 Testing Database class with C++23 features...\n\n";
    
    try {
        testDatabaseCreation();
        testTableManagement();
        testDataOperations();
        testDatabaseStatistics();
        testErrorHandling();
        testDatabaseValidation();
        testTransaction();
        testSnapshot();
        
        std::cout << "\n🎉 All Database tests passed successfully! ✅\n";
        
        // 演示数据库功能
        std::cout << "\n📋 Demo: Database functionality\n";
        Database demo("DemoDatabase");
        
        // 创建示例数据
        demo.createTable("employees",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"department", DataType::STR},
            Column{"salary", DataType::INT}
        );
        
        demo.insertInto("employees", {Value{1}, Value{"Alice Johnson"}, Value{"Engineering"}, Value{75000}});
        demo.insertInto("employees", {Value{2}, Value{"Bob Smith"}, Value{"Marketing"}, Value{65000}});
        demo.insertInto("employees", {Value{3}, Value{"Charlie Brown"}, Value{"Engineering"}, Value{80000}});
        
        demo.printDatabaseInfo();
        demo.printDatabase();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
