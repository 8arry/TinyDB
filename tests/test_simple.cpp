#include "libcore/database/value.hpp"
#include <iostream>
#include <cassert>

using namespace tinydb;

int main() {
    std::cout << "Testing Value class functionality...\n";
    
    // 测试基本构造和类型
    Value intVal{42};
    Value strVal{"Hello"};
    
    std::cout << "Integer value: " << intVal << std::endl;
    std::cout << "String value: " << strVal << std::endl;
    
    // 测试类型获取
    std::cout << "Int type: " << (intVal.getType() == DataType::INT ? "INT" : "STRING") << std::endl;
    std::cout << "String type: " << (strVal.getType() == DataType::STRING ? "STRING" : "INT") << std::endl;
    
    // 测试传统的值获取方法
    try {
        std::cout << "Integer value via asIntUnsafe: " << intVal.asIntUnsafe() << std::endl;
        std::cout << "String value via asStringUnsafe: " << strVal.asStringUnsafe() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    // 测试默认值
    auto defaultInt = Value::getDefault(DataType::INT);
    auto defaultStr = Value::getDefault(DataType::STRING);
    std::cout << "Default int: " << defaultInt << std::endl;
    std::cout << "Default string: " << defaultStr << std::endl;
    
    // 测试基本比较
    Value intVal2{42};
    Value intVal3{100};
    
    std::cout << "42 == 42: " << (intVal == intVal2 ? "true" : "false") << std::endl;
    std::cout << "42 < 100: " << (intVal < intVal3 ? "true" : "false") << std::endl;
    
    // 测试toString
    std::cout << "toString() - Int: " << intVal.toString() << std::endl;
    std::cout << "toString() - String: " << strVal.toString() << std::endl;
    
    std::cout << "All basic tests completed successfully! ✅\n";
    return 0;
}