# TinyDB

A complete in-memory SQL database implementation using modern C++23 features.

## 🎯 Project Overview

TinyDB is a fully functional in-memory database that implements comprehensive SQL functionality including:
- ✅ **Core SQL Operations**: CREATE TABLE, INSERT, SELECT, UPDATE, DELETE
- ✅ **Advanced WHERE Conditions**: All comparison operators (=, !=, <, >, <=, >=)
- ✅ **Logical Operations**: AND, OR with parentheses and correct precedence
- ✅ **JOIN Operations**: INNER JOIN with qualified column names
- ✅ **Data Persistence**: Export/import database to JSON format
- ✅ **Type Safety**: Robust value system using modern C++23 features
- ✅ **Comprehensive Testing**: 48 unit tests covering all functionality

## 📁 Project Structure

```
TinyDB/
├── libcore/                   # Core library source code
│   ├── database/             # Database core components
│   │   ├── value.hpp/cpp     # Type-safe value system with C++23 features
│   │   ├── table.hpp/cpp     # Table data structure and operations
│   │   ├── database.hpp/cpp  # Database container and management
│   │   ├── condition.hpp/cpp # WHERE condition evaluation system
│   │   └── persistence.hpp/cpp # Database export/import functionality
│   └── sql/                  # SQL parsing and AST components
│       ├── token.hpp/cpp     # Lexical tokens and utilities
│       ├── lexer.hpp/cpp     # SQL lexical analysis
│       ├── parser.hpp/cpp    # Recursive descent SQL parser
│       └── ast.hpp/cpp       # Abstract Syntax Tree definitions
├── tests/                    # Comprehensive test suite
│   ├── catch.hpp            # Catch2-compatible test framework
│   └── catch2_tests.cpp     # Complete test coverage (48 tests)
├── examples/                 # Example SQL queries and demos
│   ├── demo_complete.sql    # Complete feature demonstration
│   └── sample_queries.sql   # Basic SQL examples
├── docs/                    # Project documentation
│   ├── database.md          # Official project requirements
│   └── project_design.md    # Design documentation
├── scripts/                 # Build automation scripts
│   ├── build_clean.ps1      # Clean rebuild script
│   ├── build.ps1           # Standard build script
│   └── test.ps1            # Test runner script
├── main.cpp                 # Interactive SQL console application
├── CMakeLists.txt          # CMake build configuration
└── .gitignore              # Git ignore patterns
```

## 🛠️ Requirements

- **Compiler**: GCC 15.0+ or Clang 15.0+ (C++23 support required)
- **OS**: Windows/Linux/macOS
- **CMake**: 3.26+ (optional)

## 🚀 Quick Start

### Option 1: Clean Build (Recommended)

```powershell
# Complete clean rebuild and test
.\scripts\build_clean.ps1
```

### Option 2: Standard Build

```powershell
# Build the project
.\scripts\build.ps1

# Run all tests
.\scripts\test.ps1

# Run interactive database
.\build\build\tinydb.exe
```

### Option 3: Manual Build

```bash
# Create build directory
mkdir build && cd build

# Configure and build
cmake -G "Ninja" ..
cmake --build .

# Run tests
ctest --output-on-failure

# Run interactive database
.\build\tinydb.exe
```

## 🧪 Testing

The project includes 48 comprehensive unit tests covering:
- ✅ **SQL Parser Tests**: Valid/invalid syntax, error handling
- ✅ **SQL Execution Tests**: All statement types, edge cases  
- ✅ **JOIN Operation Tests**: Complex queries, qualified columns
- ✅ **WHERE Condition Tests**: All operators, logical combinations, parentheses
- ✅ **Persistence Tests**: Export/import, error handling, large datasets
- ✅ **Value System Tests**: Type safety, conversions, comparisons
- ✅ **Edge Case Tests**: Empty values, special characters, error conditions

Run tests with: `.\scripts\build_clean.ps1` or `ctest --output-on-failure`

## 🎓 C++23 Features Used

- **Concepts**: Type constraints and validation
- **std::expected**: Modern error handling (with fallback)
- **Spaceship Operator**: Simplified comparisons
- **std::format**: Modern string formatting (with fallback)
- **Enhanced constexpr**: Compile-time computations
- **Modern variants**: Type-safe unions

## 📖 SQL Syntax Supported

### Core SQL Operations
```sql
-- Table creation
CREATE TABLE users (id int, name str, age int);

-- Data insertion  
INSERT INTO users VALUES (1, "Alice", 25), (2, "Bob", 30);

-- Basic queries
SELECT * FROM users;
SELECT name, age FROM users WHERE age > 25;

-- Advanced WHERE conditions
SELECT * FROM users WHERE age >= 25 AND name != "Alice";
SELECT * FROM users WHERE (age > 30 OR name = "Bob") AND age < 50;

-- JOIN operations
SELECT users.name, orders.amount 
FROM users INNER JOIN orders ON users.id = orders.user_id;

-- Updates and deletions
UPDATE users SET age = 31 WHERE name = "Bob";
DELETE FROM users WHERE id = 1;
```

### Interactive Commands
```sql
-- Database persistence
EXPORT DATABASE TO "my_database.json";
IMPORT DATABASE FROM "my_database.json";

-- Help and utilities
HELP;          -- Show available commands
QUIT;          -- Exit the program
```

### Supported Features
- ✅ **Data Types**: `int`, `str`
- ✅ **Operators**: `=`, `!=`, `<`, `>`, `<=`, `>=`, `AND`, `OR`
- ✅ **Grouping**: Parentheses with correct precedence
- ✅ **Qualified Names**: `table.column` syntax
- ✅ **String Literals**: Double-quoted strings with special characters

## 🏆 Project Status

**Complete Implementation** - All core and extension features implemented:

- ✅ **Value Type System** - Complete with C++23 features
- ✅ **Table & Database Management** - Complete with full CRUD operations
- ✅ **SQL Lexer & Parser** - Complete recursive descent parser
- ✅ **Query Executor** - Complete with JOIN support
- ✅ **WHERE Conditions** - Complete with all operators and precedence
- ✅ **Data Persistence** - Complete JSON export/import
- ✅ **Unit Testing** - Complete with 48 comprehensive tests
- ✅ **Error Handling** - Complete with meaningful error messages
- ✅ **Interactive Console** - Complete with help system

**Extensions Implemented:**
- ✅ **Extension 1**: INNER JOIN with qualified column names
- ✅ **Extension 2**: Comprehensive unit testing with Catch2
- ✅ **Extension 3**: Database persistence with JSON format

## 📝 License

This is an educational project for learning C++23 and database internals.

## 🤝 Contributing

This is a learning project. Feel free to explore the code and suggest improvements!