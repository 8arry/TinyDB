cmake_minimum_required(VERSION 3.26)
project(TinyDB VERSION 1.0.0 LANGUAGES CXX)

# 设置C++23标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 启用CTest
enable_testing()

# 编译器特定设置
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC 12+ 支持C++23
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0")
        message(FATAL_ERROR "GCC 12 or later is required for C++23 support")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang 15+ 支持C++23
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
        message(FATAL_ERROR "Clang 15 or later is required for C++23 support")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC 2022 17.4+ 支持C++23
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.34")
        message(FATAL_ERROR "MSVC 2022 17.4 or later is required for C++23 support")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# 创建核心库
file(GLOB_RECURSE CORE_SOURCES "libcore/**/*.cpp")
file(GLOB_RECURSE CORE_HEADERS "libcore/**/*.hpp")

add_library(tinydb_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# 设置包含目录
target_include_directories(tinydb_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 单元测试 - 使用Catch2框架

# Catch2测试
add_executable(catch2_tests tests/catch2_tests.cpp)
target_link_libraries(catch2_tests tinydb_core)
set_target_properties(catch2_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
)

# 添加CTest支持
add_test(NAME TinyDB_Tests COMMAND catch2_tests)



# 主程序
add_executable(tinydb main.cpp)
target_link_libraries(tinydb tinydb_core)
set_target_properties(tinydb PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
)

# 安装规则
install(TARGETS tinydb_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY libcore/ DESTINATION include/tinydb
    FILES_MATCHING PATTERN "*.hpp"
)