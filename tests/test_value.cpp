#include "libcore/database/value.hpp"
#include <iostream>
#include <cassert>
#include <format>
#include <print>  // C++23 print functionality

using namespace tinydb;

int main() {
    std::println("Testing Value class with C++23 features...");
    
    // 测试基本构造和类型（使用C++23的模板参数推导）
    Value intVal{42};
    Value strVal{"Hello"};
    
    // 使用constexpr函数获取默认值
    constexpr auto defaultIntVal = Value::getDefault(DataType::INT);
    constexpr auto defaultStrVal = Value::getDefault(DataType::STR);
    
    assert(intVal.getType() == DataType::INT);
    assert(strVal.getType() == DataType::STR);
    
    // 测试C++23的std::expected版本
    {
        auto intResult = intVal.asInt();
        assert(intResult.has_value());
        assert(intResult.value() == 42);
        
        auto strFromInt = intVal.asString();
        assert(!strFromInt.has_value());
        std::println("Expected error: {}", strFromInt.error());
    }
    
    {
        auto strResult = strVal.asString();
        assert(strResult.has_value());
        assert(strResult.value() == "Hello");
        
        auto intFromStr = strVal.asInt();
        assert(!intFromStr.has_value());
        std::println("Expected error: {}", intFromStr.error());
    }
    
    // 测试传统版本（向后兼容）
    assert(intVal.asIntUnsafe() == 42);
    assert(strVal.asStringUnsafe() == "Hello");
    assert(defaultIntVal.asIntUnsafe() == 0);
    assert(defaultStrVal.asStringUnsafe() == "");
    
    // 测试C++23的spaceship operator
    Value intVal2{42};
    Value intVal3{100};
    
    assert(intVal == intVal2);
    assert(intVal != intVal3);
    assert(intVal < intVal3);
    assert(intVal3 > intVal);
    
    // 测试字符串比较
    Value strVal2{"Hello"};
    Value strVal3{"World"};
    
    assert(strVal == strVal2);
    assert(strVal != strVal3);
    assert(strVal < strVal3);  // "Hello" < "World"
    
    // 测试C++23的std::format和std::print
    std::println("Integer value: {}", intVal);
    std::println("String value: {}", strVal);
    std::println("Formatted: {}", intVal.toString());
    
    // 测试Column结构（使用C++23的designated initializers概念）
    Column idCol{"id", DataType::INT};
    Column nameCol{"name", DataType::STR};
    
    assert(idCol.name == "id");
    assert(idCol.type == DataType::INT);
    assert(nameCol.name == "name");
    assert(nameCol.type == DataType::STR);
    
    // 测试Column的spaceship operator
    Column idCol2{"id", DataType::INT};
    assert(idCol == idCol2);
    
    std::println("All C++23 tests passed! ✅");
    return 0;
}