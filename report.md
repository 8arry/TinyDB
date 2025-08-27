# TinyDB Project Report

## Software Design

TinyDB implements a modular in-memory database with a well-structured layered architecture that promotes maintainability and extensibility:

1. **Lexical/Parsing Layer**: The tokenizer (`lexer.cpp`) converts SQL text into meaningful tokens, while the parser (`parser.cpp`) builds Abstract Syntax Trees (AST) for each statement type
2. **Database Layer**: Core classes including `Database`, `Table`, and `Value` handle data storage and manipulation with full CRUD support
3. **Value System**: Type-safe storage using `std::variant` enables flexible data types while maintaining compile-time safety

The core library strictly follows RAII principles, ensuring automatic resource management and preventing memory leaks. Clear separation between parsing, storage, and execution components allows for independent testing and future extensions.

## Key Design Choices

- **Smart Pointers**: Extensive use of `std::unique_ptr` provides automatic memory management for AST nodes and complex data structures, eliminating manual memory management
- **Exception Handling**: Comprehensive exception-based error handling ensures graceful error recovery without system crashes, with detailed error messages for debugging
- **Type Safety**: `std::variant` provides runtime type flexibility while maintaining compile-time safety, supporting both integer and string data types seamlessly
- **Efficient Storage**: `std::vector` for table rows enables fast iteration and dynamic sizing, while `std::unordered_map` provides O(1) table lookup by name
- **Condition System**: WHERE clauses are converted to lambda functions, enabling efficient row filtering with support for complex logical expressions

## C++ Features Used

**Modern C++23 Features:**
- `std::variant` and `std::visit` for type-safe value storage and pattern matching
- Move semantics for efficient data transfer, minimizing unnecessary copies
- Template metaprogramming for generic condition evaluation and type-safe operations

**Advanced Features:**
- Lambda expressions for WHERE clause evaluation, providing flexible and efficient filtering
- `std::function` for type-erased condition objects, enabling polymorphic behavior
- Exception safety with strong guarantees, ensuring data consistency even during errors
- Operator overloading for clean, intuitive syntax in condition expressions
- Perfect forwarding for efficient parameter passing in template functions

The design leverages modern C++ features to achieve both high performance and memory safety while maintaining clean, readable, and extensible architecture suitable for future enhancements.
