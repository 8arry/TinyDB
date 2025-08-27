# TinyDB Project Report

## Software Design

TinyDB implements a modular in-memory database with layered architecture:
1. **Lexical/Parsing Layer**: Tokenizes and parses SQL into AST
2. **Database Layer**: Manages tables with CRUD operations
3. **Value System**: Type-safe storage using `std::variant`

The core library follows RAII principles with clear separation between parsing, storage, and execution components.

## Key Design Choices

- **Smart Pointers**: `std::unique_ptr` for automatic memory management of AST nodes
- **Exception Handling**: Graceful error recovery without crashes
- **Type Safety**: `std::variant` for runtime type flexibility with compile-time safety
- **Efficient Storage**: `std::vector` for rows, `std::unordered_map` for table lookup

## C++ Features Used

**Modern C++23 Features:**
- `std::variant` and `std::visit` for type-safe value storage
- Move semantics for efficient data transfer
- RAII and smart pointers for resource management
- Template metaprogramming for generic condition evaluation

**Advanced Features:**
- Lambda expressions for WHERE clause evaluation
- `std::function` for type-erased condition objects
- Exception safety with strong guarantees
- Operator overloading for clean syntax

The design leverages modern C++ for both performance and safety while maintaining clean, extensible architecture.
