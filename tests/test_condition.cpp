#include "libcore/database/condition.hpp"
#include "libcore/database/database.hpp"
#include <iostream>
#include <cassert>

using namespace tinydb;
using namespace tinydb::Conditions;

void testConditionValue() {
    std::cout << "Testing ConditionValue...\n";
    
    try {
        // 测试字面量
        auto intLiteral = ConditionValue::literal(42);
        auto strLiteral = ConditionValue::literal("Hello");
        auto cstrLiteral = ConditionValue::literal("World");
        
        assert(intLiteral.isLiteral());
        assert(!intLiteral.isColumn());
        assert(intLiteral.getLiteral().asIntUnsafe() == 42);
        
        assert(strLiteral.isLiteral());
        assert(strLiteral.getLiteral().asStringUnsafe() == "Hello");
        
        // 测试列引用
        auto columnRef = ConditionValue::column("age");
        assert(columnRef.isColumn());
        assert(!columnRef.isLiteral());
        assert(columnRef.getColumnName() == "age");
        
        // 测试工厂方法
        auto col1 = col("name");
        auto val1 = val(25);
        auto val2 = val("Alice");
        
        assert(col1.isColumn());
        assert(val1.isLiteral());
        assert(val2.isLiteral());
        
        std::cout << "✅ ConditionValue tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ ConditionValue failed: " << e.what() << std::endl;
        throw;
    }
}

void testComparisonCondition() {
    std::cout << "Testing ComparisonCondition...\n";
    
    try {
        // 创建测试表
        Database db;
        db.createTable("users",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"age", DataType::INT}
        );
        
        // 插入测试数据
        db.insertInto("users", {Value{1}, Value{"Alice"}, Value{25}});
        db.insertInto("users", {Value{2}, Value{"Bob"}, Value{30}});
        db.insertInto("users", {Value{3}, Value{"Charlie"}, Value{35}});
        
        const auto& table = db.getTable("users");
        const auto& rows = table.getAllRows();
        
        // 测试相等比较: age = 30
        auto condition1 = ConditionBuilder::equal(col("age"), val(30));
        assert(condition1->evaluate(rows[1], table) == true);  // Bob
        assert(condition1->evaluate(rows[0], table) == false); // Alice
        
        // 测试大于比较: age > 25
        auto condition2 = ConditionBuilder::greaterThan(col("age"), val(25));
        assert(condition2->evaluate(rows[0], table) == false); // Alice (25)
        assert(condition2->evaluate(rows[1], table) == true);  // Bob (30)
        assert(condition2->evaluate(rows[2], table) == true);  // Charlie (35)
        
        // 测试字符串比较: name = "Alice"
        auto condition3 = ConditionBuilder::equal(col("name"), val("Alice"));
        assert(condition3->evaluate(rows[0], table) == true);  // Alice
        assert(condition3->evaluate(rows[1], table) == false); // Bob
        
        // 测试小于等于: age <= 30
        auto condition4 = ConditionBuilder::lessEqual(col("age"), val(30));
        assert(condition4->evaluate(rows[0], table) == true);  // Alice (25)
        assert(condition4->evaluate(rows[1], table) == true);  // Bob (30)
        assert(condition4->evaluate(rows[2], table) == false); // Charlie (35)
        
        // 测试toString
        std::string condStr = condition1->toString();
        std::cout << "Condition string: " << condStr << std::endl;
        
        std::cout << "✅ ComparisonCondition tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ ComparisonCondition failed: " << e.what() << std::endl;
        throw;
    }
}

void testLogicalCondition() {
    std::cout << "Testing LogicalCondition...\n";
    
    try {
        // 创建测试表
        Database db;
        db.createTable("employees",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"age", DataType::INT},
            Column{"salary", DataType::INT}
        );
        
        // 插入测试数据
        db.insertInto("employees", {Value{1}, Value{"Alice"}, Value{25}, Value{50000}});
        db.insertInto("employees", {Value{2}, Value{"Bob"}, Value{30}, Value{60000}});
        db.insertInto("employees", {Value{3}, Value{"Charlie"}, Value{35}, Value{70000}});
        db.insertInto("employees", {Value{4}, Value{"Diana"}, Value{28}, Value{55000}});
        
        const auto& table = db.getTable("employees");
        const auto& rows = table.getAllRows();
        
        // 测试 AND: age > 25 AND salary >= 60000
        auto condition1 = ConditionBuilder::and_(
            ConditionBuilder::greaterThan(col("age"), val(25)),
            ConditionBuilder::greaterEqual(col("salary"), val(60000))
        );
        
        assert(condition1->evaluate(rows[0], table) == false); // Alice: age=25, salary=50000
        assert(condition1->evaluate(rows[1], table) == true);  // Bob: age=30, salary=60000
        assert(condition1->evaluate(rows[2], table) == true);  // Charlie: age=35, salary=70000
        assert(condition1->evaluate(rows[3], table) == false); // Diana: age=28, salary=55000
        
        // 测试 OR: age < 27 OR salary > 65000
        auto condition2 = ConditionBuilder::or_(
            ConditionBuilder::lessThan(col("age"), val(27)),
            ConditionBuilder::greaterThan(col("salary"), val(65000))
        );
        
        assert(condition2->evaluate(rows[0], table) == true);  // Alice: age=25
        assert(condition2->evaluate(rows[1], table) == false); // Bob: age=30, salary=60000
        assert(condition2->evaluate(rows[2], table) == true);  // Charlie: salary=70000
        assert(condition2->evaluate(rows[3], table) == false); // Diana: age=28, salary=55000
        
        // 测试 NOT: NOT (age = 30)
        auto condition3 = ConditionBuilder::not_(
            ConditionBuilder::equal(col("age"), val(30))
        );
        
        assert(condition3->evaluate(rows[0], table) == true);  // Alice: age != 30
        assert(condition3->evaluate(rows[1], table) == false); // Bob: age == 30
        assert(condition3->evaluate(rows[2], table) == true);  // Charlie: age != 30
        assert(condition3->evaluate(rows[3], table) == true);  // Diana: age != 30
        
        // 测试复杂条件: (age >= 30 AND salary >= 60000) OR name = "Alice"
        auto condition4 = ConditionBuilder::or_(
            ConditionBuilder::and_(
                ConditionBuilder::greaterEqual(col("age"), val(30)),
                ConditionBuilder::greaterEqual(col("salary"), val(60000))
            ),
            ConditionBuilder::equal(col("name"), val("Alice"))
        );
        
        assert(condition4->evaluate(rows[0], table) == true);  // Alice: name = "Alice"
        assert(condition4->evaluate(rows[1], table) == true);  // Bob: age >= 30 AND salary >= 60000
        assert(condition4->evaluate(rows[2], table) == true);  // Charlie: age >= 30 AND salary >= 60000
        assert(condition4->evaluate(rows[3], table) == false); // Diana: age < 30 AND name != "Alice"
        
        // 测试toString
        std::string condStr = condition4->toString();
        std::cout << "Complex condition: " << condStr << std::endl;
        
        std::cout << "✅ LogicalCondition tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ LogicalCondition failed: " << e.what() << std::endl;
        throw;
    }
}

void testOperatorOverloads() {
    std::cout << "Testing operator overloads...\n";
    
    try {
        // 创建测试表
        Database db;
        db.createTable("products",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"price", DataType::INT}
        );
        
        db.insertInto("products", {Value{1}, Value{"Laptop"}, Value{1000}});
        db.insertInto("products", {Value{2}, Value{"Mouse"}, Value{25}});
        db.insertInto("products", {Value{3}, Value{"Keyboard"}, Value{75}});
        
        const auto& table = db.getTable("products");
        const auto& rows = table.getAllRows();
        
        // 测试操作符重载: price > 50
        auto condition1 = col("price") > val(50);
        assert(condition1->evaluate(rows[0], table) == true);  // Laptop: 1000
        assert(condition1->evaluate(rows[1], table) == false); // Mouse: 25
        assert(condition1->evaluate(rows[2], table) == true);  // Keyboard: 75
        
        // 测试组合操作符: price >= 75 && name != "Laptop"
        auto condition2 = (col("price") >= val(75)) && (col("name") != val("Laptop"));
        assert(condition2->evaluate(rows[0], table) == false); // Laptop: price >= 75 but name == "Laptop"
        assert(condition2->evaluate(rows[1], table) == false); // Mouse: price < 75
        assert(condition2->evaluate(rows[2], table) == true);  // Keyboard: price >= 75 and name != "Laptop"
        
        // 测试OR操作符: price < 30 || price > 500
        auto condition3 = (col("price") < val(30)) || (col("price") > val(500));
        assert(condition3->evaluate(rows[0], table) == true);  // Laptop: price > 500
        assert(condition3->evaluate(rows[1], table) == true);  // Mouse: price < 30
        assert(condition3->evaluate(rows[2], table) == false); // Keyboard: 30 <= price <= 500
        
        // 测试NOT操作符: !(price = 25)
        auto condition4 = !(col("price") == val(25));
        assert(condition4->evaluate(rows[0], table) == true);  // Laptop: price != 25
        assert(condition4->evaluate(rows[1], table) == false); // Mouse: price == 25
        assert(condition4->evaluate(rows[2], table) == true);  // Keyboard: price != 25
        
        std::cout << "✅ Operator overload tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Operator overload failed: " << e.what() << std::endl;
        throw;
    }
}

void testConditionAdapter() {
    std::cout << "Testing ConditionAdapter...\n";
    
    try {
        // 创建测试表
        Database db;
        db.createTable("students",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"grade", DataType::INT}
        );
        
        db.insertInto("students", {Value{1}, Value{"Alice"}, Value{95}});
        db.insertInto("students", {Value{2}, Value{"Bob"}, Value{87}});
        db.insertInto("students", {Value{3}, Value{"Charlie"}, Value{92}});
        
        // 创建条件: grade >= 90
        auto condition = col("grade") >= val(90);
        
        // 转换为lambda并用于数据库查询
        auto lambda = ConditionAdapter::toLambda(condition);
        auto highGraders = db.selectFrom("students", {"name", "grade"}, lambda);
        
        assert(highGraders.size() == 2); // Alice (95) and Charlie (92)
        assert(highGraders[0][0].asStringUnsafe() == "Alice");
        assert(highGraders[1][0].asStringUnsafe() == "Charlie");
        
        std::cout << "✅ ConditionAdapter tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ ConditionAdapter failed: " << e.what() << std::endl;
        throw;
    }
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    try {
        Database db;
        db.createTable("test",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR}
        );
        
        db.insertInto("test", {Value{1}, Value{"Alice"}});
        
        const auto& table = db.getTable("test");
        const auto& rows = table.getAllRows();
        
        // 测试类型不匹配错误
        auto badCondition1 = col("id") == val("string"); // int与string比较
        try {
            badCondition1->evaluate(rows[0], table);
            assert(false && "Should have thrown type mismatch error");
        } catch (const std::runtime_error& e) {
            // 预期的异常
        }
        
        // 测试不存在的列
        auto badCondition2 = col("nonexistent") == val(1);
        try {
            badCondition2->evaluate(rows[0], table);
            assert(false && "Should have thrown column not found error");
        } catch (const std::runtime_error& e) {
            // 预期的异常
        }
        
        std::cout << "✅ Error handling tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Error handling failed: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "🧪 Testing WHERE Condition System with C++23 features...\n\n";
    
    try {
        testConditionValue();
        testComparisonCondition();
        testLogicalCondition();
        testOperatorOverloads();
        testConditionAdapter();
        testErrorHandling();
        
        std::cout << "\n🎉 All Condition tests passed successfully! ✅\n";
        
        // 演示复杂条件使用
        std::cout << "\n📋 Demo: Complex condition usage\n";
        Database demo("DemoDatabase");
        
        demo.createTable("employees",
            Column{"id", DataType::INT},
            Column{"name", DataType::STR},
            Column{"department", DataType::STR},
            Column{"salary", DataType::INT},
            Column{"experience", DataType::INT}
        );
        
        demo.insertInto("employees", {Value{1}, Value{"Alice"}, Value{"Engineering"}, Value{75000}, Value{5}});
        demo.insertInto("employees", {Value{2}, Value{"Bob"}, Value{"Marketing"}, Value{65000}, Value{3}});
        demo.insertInto("employees", {Value{3}, Value{"Charlie"}, Value{"Engineering"}, Value{80000}, Value{7}});
        demo.insertInto("employees", {Value{4}, Value{"Diana"}, Value{"Sales"}, Value{55000}, Value{2}});
        demo.insertInto("employees", {Value{5}, Value{"Eve"}, Value{"Engineering"}, Value{90000}, Value{10}});
        
        // 复杂查询: 高薪工程师或有经验的销售
        // (department = "Engineering" AND salary >= 75000) OR (department = "Sales" AND experience >= 3)
        auto complexCondition = 
            ((col("department") == val("Engineering")) && (col("salary") >= val(75000))) ||
            ((col("department") == val("Sales")) && (col("experience") >= val(3)));
        
        std::cout << "Complex condition: " << complexCondition->toString() << std::endl;
        
        auto results = demo.selectFrom("employees", {"name", "department", "salary"}, 
                                     ConditionAdapter::toLambda(complexCondition));
        
        std::cout << "Results:\n";
        for (const auto& row : results) {
            std::cout << "  " << row[0].toString() << " (" << row[1].toString() 
                     << ") - $" << row[2].toString() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

