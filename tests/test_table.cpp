#include "libcore/database/table.hpp"
#include <iostream>
#include <cassert>
#include <vector>

using namespace tinydb;

void testRowBasics() {
    std::cout << "Testing Row class basics...\n";
    
    // 测试基本构造
    Row row1({Value{1}, Value{"Alice"}, Value{25}});
    assert(row1.size() == 3);
    assert(row1[0].asIntUnsafe() == 1);
    assert(row1[1].asStringUnsafe() == "Alice");
    assert(row1[2].asIntUnsafe() == 25);
    
    // 测试范围构造
    std::vector<Value> values = {Value{2}, Value{"Bob"}, Value{30}};
    Row row2(values);
    assert(row2.size() == 3);
    assert(row2[0].asIntUnsafe() == 2);
    
    // 测试修改
    row2[1] = Value{"Bobby"};
    assert(row2[1].asStringUnsafe() == "Bobby");
    
    // 测试迭代器
    int count = 0;
    for (const auto& value : row1) {
        count++;
    }
    assert(count == 3);
    
    std::cout << "✅ Row class tests passed!\n";
}

void testTableCreation() {
    std::cout << "Testing Table creation...\n";
    
    try {
        // 创建schema
        std::cout << "  Creating schema...\n";
        std::vector<Column> schema = {
            {"id", DataType::INT},
            {"name", DataType::STRING},
            {"age", DataType::INT}
        };
        
        std::cout << "  Creating table...\n";
        Table table("users", std::move(schema));
    
    // 测试基本信息
    assert(table.getName() == "users");
    assert(table.getColumnCount() == 3);
    assert(table.getRowCount() == 0);
    assert(table.empty());
    
    // 测试列信息
    assert(table.hasColumn("id"));
    assert(table.hasColumn("name"));
    assert(table.hasColumn("age"));
    assert(!table.hasColumn("email"));
    
    const auto& idColumn = table.getColumn("id");
    assert(idColumn.name == "id");
    assert(idColumn.type == DataType::INT);
    
    auto columnNames = table.getColumnNames();
    assert(columnNames.size() == 3);
    assert(columnNames[0] == "id");
    assert(columnNames[1] == "name");
    assert(columnNames[2] == "age");
    
        std::cout << "✅ Table creation tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Table creation failed: " << e.what() << std::endl;
        throw;
    }
}

void testDataInsertion() {
    std::cout << "Testing data insertion...\n";
    
    std::vector<Column> schema = {
        {"id", DataType::INT},
        {"name", DataType::STRING},
        {"age", DataType::INT}
    };
    
    Table table("users", std::move(schema));
    
    // 插入数据 - 使用Row
    table.insertRow(Row{{Value{1}, Value{"Alice"}, Value{25}}});
    assert(table.getRowCount() == 1);
    
    // 插入数据 - 使用vector
    table.insertRow({Value{2}, Value{"Bob"}, Value{30}});
    assert(table.getRowCount() == 2);
    
    // 插入数据 - 使用范围
    std::vector<Value> values = {Value{3}, Value{"Charlie"}, Value{35}};
    table.insertRow(values);
    assert(table.getRowCount() == 3);
    
    // 测试数据访问
    const auto& row1 = table.getRow(0);
    assert(row1[0].asIntUnsafe() == 1);
    assert(row1[1].asStringUnsafe() == "Alice");
    assert(row1[2].asIntUnsafe() == 25);
    
    // 测试值访问
    assert(table.getValue(1, "name").asStringUnsafe() == "Bob");
    assert(table.getValue(2, "age").asIntUnsafe() == 35);
    
    std::cout << "✅ Data insertion tests passed!\n";
}

void testDataSelection() {
    std::cout << "Testing data selection...\n";
    
    std::vector<Column> schema = {
        {"id", DataType::INT},
        {"name", DataType::STRING},
        {"age", DataType::INT}
    };
    
    Table table("users", std::move(schema));
    
    // 插入测试数据
    table.insertRow({Value{1}, Value{"Alice"}, Value{25}});
    table.insertRow({Value{2}, Value{"Bob"}, Value{30}});
    table.insertRow({Value{3}, Value{"Charlie"}, Value{35}});
    table.insertRow({Value{4}, Value{"Diana"}, Value{28}});
    
    // 测试选择所有列
    auto allRows = table.selectRows({"*"});
    assert(allRows.size() == 4);
    assert(allRows[0].size() == 3);
    
    // 测试选择特定列
    auto nameAgeRows = table.selectRows({"name", "age"});
    assert(nameAgeRows.size() == 4);
    assert(nameAgeRows[0].size() == 2);
    assert(nameAgeRows[0][0].asStringUnsafe() == "Alice");
    assert(nameAgeRows[0][1].asIntUnsafe() == 25);
    
    // 测试带条件的选择
    auto youngUsers = table.selectRows({"name"}, 
        [](const Row& row, const Table& table) {
            return row[2].asIntUnsafe() < 30; // age < 30
        });
    
    assert(youngUsers.size() == 2); // Alice(25) and Diana(28)
    assert(youngUsers[0][0].asStringUnsafe() == "Alice");
    assert(youngUsers[1][0].asStringUnsafe() == "Diana");
    
    // 测试列值获取
    auto ages = table.getColumnValues("age");
    assert(ages.size() == 4);
    assert(ages[0].asIntUnsafe() == 25);
    assert(ages[1].asIntUnsafe() == 30);
    
    std::cout << "✅ Data selection tests passed!\n";
}

void testDataUpdate() {
    std::cout << "Testing data update...\n";
    
    std::vector<Column> schema = {
        {"id", DataType::INT},
        {"name", DataType::STRING},
        {"age", DataType::INT}
    };
    
    Table table("users", std::move(schema));
    
    // 插入测试数据
    table.insertRow({Value{1}, Value{"Alice"}, Value{25}});
    table.insertRow({Value{2}, Value{"Bob"}, Value{30}});
    table.insertRow({Value{3}, Value{"Charlie"}, Value{35}});
    
    // 更新Bob的年龄
    std::unordered_map<std::string, Value> updates = {
        {"age", Value{31}}
    };
    
    size_t updatedCount = table.updateRows(
        [](const Row& row, const Table& table) {
            return row[1].asStringUnsafe() == "Bob"; // name == "Bob"
        },
        updates
    );
    
    assert(updatedCount == 1);
    assert(table.getValue(1, "age").asIntUnsafe() == 31);
    
    // 更新所有人的年龄（+1）
    size_t allUpdated = table.updateRows(
        [](const Row& row, const Table& table) {
            return true; // 所有行
        },
        {{"age", Value{40}}} // 设置为固定值40（简化测试）
    );
    
    assert(allUpdated == 3);
    assert(table.getValue(0, "age").asIntUnsafe() == 40);
    assert(table.getValue(1, "age").asIntUnsafe() == 40);
    assert(table.getValue(2, "age").asIntUnsafe() == 40);
    
    std::cout << "✅ Data update tests passed!\n";
}

void testDataDeletion() {
    std::cout << "Testing data deletion...\n";
    
    std::vector<Column> schema = {
        {"id", DataType::INT},
        {"name", DataType::STRING},
        {"age", DataType::INT}
    };
    
    Table table("users", std::move(schema));
    
    // 插入测试数据
    table.insertRow({Value{1}, Value{"Alice"}, Value{25}});
    table.insertRow({Value{2}, Value{"Bob"}, Value{30}});
    table.insertRow({Value{3}, Value{"Charlie"}, Value{35}});
    table.insertRow({Value{4}, Value{"Diana"}, Value{28}});
    
    // 删除年龄大于30的用户
    size_t deletedCount = table.deleteRows(
        [](const Row& row, const Table& table) {
            return row[2].asIntUnsafe() > 30; // age > 30
        }
    );
    
    assert(deletedCount == 1); // Charlie (35)
    assert(table.getRowCount() == 3);
    
    // 验证剩余的数据
    auto names = table.getColumnValues("name");
    assert(names.size() == 3);
    assert(names[0].asStringUnsafe() == "Alice");
    assert(names[1].asStringUnsafe() == "Bob");
    assert(names[2].asStringUnsafe() == "Diana");
    
    // 清空表
    table.clear();
    assert(table.empty());
    assert(table.getRowCount() == 0);
    
    std::cout << "✅ Data deletion tests passed!\n";
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    std::vector<Column> schema = {
        {"id", DataType::INT},
        {"name", DataType::STRING}
    };
    
    Table table("users", std::move(schema));
    
    // 测试类型不匹配错误
    try {
        table.insertRow({Value{"not_int"}, Value{"Alice"}}); // 第一列应该是int
        assert(false && "Should have thrown type mismatch error");
    } catch (const std::invalid_argument& e) {
        // 预期的异常
    }
    
    // 测试列数不匹配错误
    try {
        table.insertRow({Value{1}}); // 缺少一列
        assert(false && "Should have thrown column count error");
    } catch (const std::invalid_argument& e) {
        // 预期的异常
    }
    
    // 测试不存在的列
    try {
        table.getColumn("nonexistent");
        assert(false && "Should have thrown column not found error");
    } catch (const std::invalid_argument& e) {
        // 预期的异常
    }
    
    // 插入正确的数据
    table.insertRow({Value{1}, Value{"Alice"}});
    
    // 测试行索引越界
    try {
        table.getRow(999);
        assert(false && "Should have thrown out of range error");
    } catch (const std::out_of_range& e) {
        // 预期的异常
    }
    
    std::cout << "✅ Error handling tests passed!\n";
}

void testC23Features() {
    std::cout << "Testing C++23 features...\n";
    
    std::vector<Column> schema = {
        {"id", DataType::INT},
        {"name", DataType::STRING},
        {"score", DataType::INT}
    };
    
    Table table("students", std::move(schema));
    
    // 插入数据
    table.insertRow({Value{1}, Value{"Alice"}, Value{95}});
    table.insertRow({Value{2}, Value{"Bob"}, Value{87}});
    table.insertRow({Value{3}, Value{"Charlie"}, Value{92}});
    
    // 测试范围和视图
    auto rowsView = table.rowsView();
    int count = 0;
    for (const auto& row : rowsView) {
        count++;
        assert(row.size() == 3);
    }
    assert(count == 3);
    
    // 测试过滤视图
    auto highScorers = table.filteredRowsView([](const Row& row, const Table& table) {
        return row[2].asIntUnsafe() > 90; // score > 90
    });
    
    int highScorerCount = 0;
    for (const auto& row : highScorers) {
        highScorerCount++;
        assert(row[2].asIntUnsafe() > 90);
    }
    assert(highScorerCount == 2); // Alice(95) and Charlie(92)
    
    std::cout << "✅ C++23 features tests passed!\n";
}

int main() {
    std::cout << "🧪 Testing Table class with C++23 features...\n\n";
    
    try {
        testRowBasics();
        testTableCreation();
        testDataInsertion();
        testDataSelection();
        testDataUpdate();
        testDataDeletion();
        testErrorHandling();
        testC23Features();
        
        std::cout << "\n🎉 All Table tests passed successfully! ✅\n";
        
        // 演示表格输出
        std::cout << "\n📋 Demo: Table output formatting\n";
        std::vector<Column> demoSchema = {
            {"id", DataType::INT},
            {"name", DataType::STRING},
            {"age", DataType::INT}
        };
        
        Table demoTable("demo_users", std::move(demoSchema));
        demoTable.insertRow({Value{1}, Value{"Alice"}, Value{25}});
        demoTable.insertRow({Value{2}, Value{"Bob"}, Value{30}});
        demoTable.insertRow({Value{3}, Value{"Charlie"}, Value{35}});
        
        demoTable.printTable();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
