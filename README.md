# TinyDB

A simplified in-memory SQL database implementation using modern C++23 features.

## 🎯 Project Overview

TinyDB is an educational database project that implements core SQL functionality including:
- Table creation and management
- Data insertion, updating, and deletion
- SQL query execution with WHERE conditions
- Type-safe value system using C++23 features

## 📁 Project Structure

```
TinyDB/
├── libcore/                    # Core library source code
│   ├── database/              # Database core components
│   │   ├── value.hpp/cpp      # Type-safe value system
│   │   ├── table.hpp/cpp      # Table data structure
│   │   └── database.hpp/cpp   # Database container
│   ├── parser/                # SQL parsing components
│   ├── executor/              # SQL statement executors
│   ├── output/                # Result formatting
│   └── exception/             # Exception handling
├── tests/                     # Test files
│   ├── test_simple.cpp        # Basic functionality tests
│   └── test_value.cpp         # Value system tests
├── docs/                      # Documentation
│   ├── project_design.md      # Detailed design document
│   └── database.md            # Project requirements
├── examples/                  # Example SQL queries
│   └── sample_queries.sql     # Sample SQL statements
├── scripts/                   # Build and utility scripts
│   ├── build.ps1              # Windows build script
│   └── test.ps1               # Test runner script
├── build/                     # Compiled binaries
└── CMakeLists.txt             # CMake build configuration
```

## 🛠️ Requirements

- **Compiler**: GCC 15.0+ or Clang 15.0+ (C++23 support required)
- **OS**: Windows/Linux/macOS
- **CMake**: 3.26+ (optional)

## 🚀 Quick Start

### Windows (PowerShell)

```powershell
# Build the project
.\scripts\build.ps1

# Run tests
.\scripts\test.ps1

# Run a specific test
.\build\test_simple.exe
```

### Manual Build

```bash
# Set your GCC path (adjust as needed)
export PATH="/path/to/gcc15/bin:$PATH"

# Compile
g++ -std=c++23 -Wall -Wextra -I. libcore/database/value.cpp tests/test_simple.cpp -o test_simple

# Run
./test_simple
```

## 🧪 Testing

The project includes comprehensive tests:
- **Value System Tests**: Type safety, conversions, comparisons
- **Integration Tests**: End-to-end SQL execution (coming soon)

## 🎓 C++23 Features Used

- **Concepts**: Type constraints and validation
- **std::expected**: Modern error handling (with fallback)
- **Spaceship Operator**: Simplified comparisons
- **std::format**: Modern string formatting (with fallback)
- **Enhanced constexpr**: Compile-time computations
- **Modern variants**: Type-safe unions

## 📖 SQL Syntax Supported

```sql
-- Table creation
CREATE TABLE users (id int, name str, age int);

-- Data insertion
INSERT INTO users (id, name, age) VALUES (1, "Alice", 25), (2, "Bob", 30);

-- Queries
SELECT * FROM users;
SELECT name, age FROM users WHERE age > 25;

-- Updates
UPDATE users SET age = 31 WHERE name = "Bob";

-- Deletion
DELETE FROM users WHERE id = 1;
```

## 🔧 Development Status

- ✅ **Value Type System** - Complete
- 🚧 **Table Structure** - In Progress
- ⏳ **Database Management** - Planned
- ⏳ **SQL Parser** - Planned
- ⏳ **Query Executor** - Planned

## 📝 License

This is an educational project for learning C++23 and database internals.

## 🤝 Contributing

This is a learning project. Feel free to explore the code and suggest improvements!