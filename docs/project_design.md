# TinyDB 内存数据库项目设计方案

## 项目分析总结

### 项目目标
实现一个支持简化SQL语法的内存数据库，能够解析和执行包含多个SQL语句的文件，支持表创建、数据插入/修改和查询操作。

### 核心功能需求
1. **表管理**: CREATE TABLE 语句支持
2. **数据操作**: INSERT INTO, DELETE FROM, UPDATE 语句
3. **数据查询**: SELECT 语句（支持 WHERE 条件）
4. **数据类型**: 支持 `int` 和 `str` 两种基本类型
5. **输出格式**: ASCII 表格格式的查询结果显示
6. **错误处理**: 语法和语义错误的异常处理

### 语法规则要点
- 大小写敏感的关键字
- 字符串用双引号包围
- 语句以分号结尾
- 支持 WHERE 条件（等式、不等式、逻辑运算）
- 忽略引号外的空白字符

### 评分标准
1. 代码编译无警告（`-Wall -Wextra`）
2. 正确处理有效SQL查询
3. 良好的C++设计和特性使用
4. 无效输入的错误处理（无崩溃）
5. 内存安全（通过sanitizers测试）
6. 性能优化（避免不必要的拷贝）

---

## 1. 项目总体设计思路

### 1.1 数据库表示
```cpp
// 核心数据结构设计
class Database {
private:
    std::unordered_map<std::string, Table> tables;
public:
    // 表操作接口
    void createTable(const std::string& name, const std::vector<Column>& columns);
    Table& getTable(const std::string& name);
    bool tableExists(const std::string& name) const;
};

class Table {
private:
    std::vector<Column> schema;
    std::vector<Row> rows;
public:
    // 数据操作接口
    void insertRow(const Row& row);
    void deleteRows(const Condition& condition);
    void updateRows(const Condition& condition, const std::map<std::string, Value>& updates);
    std::vector<Row> selectRows(const std::vector<std::string>& columns, const Condition& condition);
};

enum class DataType { INT, STRING };

struct Column {
    std::string name;
    DataType type;
};

class Value {
private:
    DataType type;
    std::variant<int, std::string> data;
public:
    // 类型安全的值操作
};
```

### 1.2 语法解析流程
```
SQL文件 → 词法分析 → 语法分析 → AST生成 → 语句执行
```

1. **词法分析**: 将输入分解为tokens（关键字、标识符、字面量、操作符）
2. **语法分析**: 构建抽象语法树（AST）
3. **语义分析**: 验证表存在性、类型匹配等
4. **执行**: 在数据库实例上执行操作

### 1.3 语句执行流程
```cpp
// 执行器设计模式
class StatementExecutor {
public:
    virtual ~StatementExecutor() = default;
    virtual void execute(Database& db) = 0;
};

class CreateTableExecutor : public StatementExecutor { /* ... */ };
class InsertExecutor : public StatementExecutor { /* ... */ };
class SelectExecutor : public StatementExecutor { /* ... */ };
// ...
```

---

## 2. 模块划分

### 2.1 核心库架构
```
libcore/
├── parser/
│   ├── lexer.hpp/cpp           # 词法分析器
│   ├── parser.hpp/cpp          # 语法分析器
│   └── ast.hpp/cpp             # 抽象语法树定义
├── database/
│   ├── database.hpp/cpp        # 数据库主类
│   ├── table.hpp/cpp           # 表结构
│   ├── value.hpp/cpp           # 值类型系统
│   └── condition.hpp/cpp       # WHERE条件处理
├── executor/
│   ├── executor.hpp/cpp        # 执行器基类
│   ├── create_executor.hpp/cpp # CREATE语句执行
│   ├── insert_executor.hpp/cpp # INSERT语句执行
│   ├── select_executor.hpp/cpp # SELECT语句执行
│   ├── delete_executor.hpp/cpp # DELETE语句执行
│   └── update_executor.hpp/cpp # UPDATE语句执行
├── output/
│   ├── formatter.hpp/cpp       # 输出格式化
│   └── ascii_table.hpp/cpp     # ASCII表格输出
└── exception/
    └── database_exception.hpp/cpp # 异常类定义
```

### 2.2 主程序
```
main.cpp                       # 主程序入口
├── 读取stdin输入
├── 调用解析器解析SQL
├── 调用执行器执行语句
└── 输出结果或错误信息
```

### 2.3 模块职责划分

#### 2.3.1 解析模块 (Parser Module)
- **Lexer**: 词法分析，将输入转换为token流
- **Parser**: 语法分析，构建AST
- **AST**: 语句的内存表示

#### 2.3.2 存储模块 (Storage Module)
- **Database**: 管理多个表的容器
- **Table**: 单个表的数据和结构
- **Value**: 类型安全的值系统
- **Condition**: WHERE条件的表示和评估

#### 2.3.3 执行模块 (Executor Module)
- **StatementExecutor**: 执行器基类（访问者模式）
- **具体执行器**: 每种SQL语句的执行逻辑

#### 2.3.4 输出模块 (Output Module)
- **Formatter**: 结果格式化接口
- **ASCIITableFormatter**: ASCII表格输出实现

---

## 3. 关键实现细节

### 3.1 SQL语法支持

#### 3.1.1 CREATE TABLE语法
```cpp
// 解析示例: CREATE TABLE users (id int, name str);
struct CreateTableStatement {
    std::string tableName;
    std::vector<Column> columns;
};

class CreateTableParser {
public:
    CreateTableStatement parse(TokenStream& tokens);
private:
    Column parseColumnDefinition(TokenStream& tokens);
    DataType parseDataType(const std::string& typeStr);
};
```

#### 3.1.2 INSERT语法
```cpp
// 解析示例: INSERT INTO users (id, name) VALUES (1, "Alice"), (2, "Bob");
struct InsertStatement {
    std::string tableName;
    std::vector<std::string> columns;
    std::vector<std::vector<Value>> valuesList;
};
```

#### 3.1.3 SELECT语法
```cpp
// 解析示例: SELECT id, name FROM users WHERE age > 18;
struct SelectStatement {
    std::vector<std::string> columns; // "*" 表示所有列
    std::string tableName;
    std::optional<Condition> whereCondition;
};
```

### 3.2 WHERE条件处理
```cpp
// 条件表达式的递归结构
class Condition {
public:
    virtual ~Condition() = default;
    virtual bool evaluate(const Row& row, const Table& table) const = 0;
};

class ComparisonCondition : public Condition {
private:
    std::string columnName;
    ComparisonOperator op; // =, !=, <, >, <=, >=
    Value value;
public:
    bool evaluate(const Row& row, const Table& table) const override;
};

class LogicalCondition : public Condition {
private:
    std::unique_ptr<Condition> left;
    std::unique_ptr<Condition> right;
    LogicalOperator op; // AND, OR
public:
    bool evaluate(const Row& row, const Table& table) const override;
};
```

### 3.3 错误处理策略
```cpp
// 异常类层次
class DatabaseException : public std::exception {
protected:
    std::string message;
public:
    explicit DatabaseException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class SyntaxError : public DatabaseException {
public:
    SyntaxError(const std::string& msg, size_t line, size_t column);
};

class SemanticError : public DatabaseException {
public:
    SemanticError(const std::string& msg);
};

class RuntimeError : public DatabaseException {
public:
    RuntimeError(const std::string& msg);
};
```

### 3.4 内存管理和性能优化
```cpp
// 使用现代C++特性
class Table {
private:
    std::vector<Column> schema;
    std::vector<Row> rows;
    
public:
    // 移动语义支持
    void insertRow(Row&& row) { rows.emplace_back(std::move(row)); }
    
    // 避免不必要的拷贝
    const std::vector<Row>& getRows() const { return rows; }
    
    // 使用智能指针管理动态内存
    std::unique_ptr<Condition> parseCondition(const std::string& whereClause);
};
```

---

## 4. 可选扩展功能实现建议

### 4.1 Extension 1: JOIN语句
```cpp
// JOIN实现设计
struct JoinStatement {
    std::string leftTable;
    std::string rightTable;
    JoinType type; // INNER, LEFT, RIGHT, FULL
    Condition joinCondition;
    std::vector<std::string> selectColumns;
};

class JoinExecutor : public StatementExecutor {
public:
    void execute(Database& db) override {
        // 1. 获取两个表
        // 2. 执行笛卡尔积
        // 3. 应用JOIN条件过滤
        // 4. 投影选择的列
    }
private:
    std::vector<Row> performInnerJoin(const Table& left, const Table& right, const Condition& condition);
};

// 列名解析: table1.col1
struct QualifiedColumnName {
    std::string tableName;
    std::string columnName;
};
```

### 4.2 Extension 2: 单元测试
```cpp
// 使用Catch2框架
#include <catch2/catch.hpp>

TEST_CASE("CREATE TABLE parsing", "[parser]") {
    SECTION("Valid table creation") {
        std::string sql = "CREATE TABLE users (id int, name str);";
        auto stmt = parseCreateTable(sql);
        REQUIRE(stmt.tableName == "users");
        REQUIRE(stmt.columns.size() == 2);
        REQUIRE(stmt.columns[0].name == "id");
        REQUIRE(stmt.columns[0].type == DataType::INT);
    }
    
    SECTION("Invalid syntax") {
        std::string sql = "CREATE TABLE (id int);"; // 缺少表名
        REQUIRE_THROWS_AS(parseCreateTable(sql), SyntaxError);
    }
}

TEST_CASE("Database operations", "[database]") {
    Database db;
    
    SECTION("Table creation and insertion") {
        db.createTable("users", {{"id", DataType::INT}, {"name", DataType::STRING}});
        REQUIRE(db.tableExists("users"));
        
        Table& table = db.getTable("users");
        table.insertRow({Value(1), Value("Alice")});
        REQUIRE(table.getRowCount() == 1);
    }
}
```

### 4.3 Extension 3: 持久化
```cpp
// 序列化接口
class Serializer {
public:
    virtual ~Serializer() = default;
    virtual void serialize(const Database& db, std::ostream& out) = 0;
    virtual Database deserialize(std::istream& in) = 0;
};

// 二进制格式
class BinarySerializer : public Serializer {
public:
    void serialize(const Database& db, std::ostream& out) override {
        // 1. 写入表数量
        // 2. 对每个表：写入schema和数据
        // 3. 使用紧凑的二进制格式
    }
};

// JSON格式（便于调试）
class JSONSerializer : public Serializer {
public:
    void serialize(const Database& db, std::ostream& out) override {
        // 使用nlohmann/json库
        json dbJson;
        for (const auto& [name, table] : db.getTables()) {
            dbJson["tables"][name] = tableToJson(table);
        }
        out << dbJson.dump(2);
    }
};
```

---

## 5. 测试与验证计划

### 5.1 单元测试计划
1. **解析器测试**
   - 有效SQL语句解析
   - 语法错误检测
   - 边界情况处理

2. **数据库操作测试**
   - 表创建和删除
   - 数据插入、更新、删除
   - 查询操作
   - WHERE条件评估

3. **类型系统测试**
   - 值类型转换
   - 类型匹配验证
   - 默认值处理

### 5.2 集成测试计划
```cpp
// 端到端测试示例
TEST_CASE("Complete workflow", "[integration]") {
    std::string sqlCommands = R"(
        CREATE TABLE users (id int, name str, age int);
        INSERT INTO users (id, name, age) VALUES (1, "Alice", 25), (2, "Bob", 30);
        SELECT name, age FROM users WHERE age > 25;
        UPDATE users SET age = 31 WHERE name = "Bob";
        DELETE FROM users WHERE id = 1;
    )";
    
    Database db;
    SQLExecutor executor(db);
    
    auto results = executor.executeScript(sqlCommands);
    
    // 验证最终状态
    REQUIRE(db.getTable("users").getRowCount() == 1);
    // ... 更多验证
}
```

### 5.3 性能测试
```cpp
TEST_CASE("Performance tests", "[performance]") {
    // 大数据量插入测试
    BENCHMARK("Insert 10k rows") {
        Database db;
        db.createTable("test", {{"id", DataType::INT}, {"data", DataType::STRING}});
        
        for (int i = 0; i < 10000; ++i) {
            db.getTable("test").insertRow({Value(i), Value("data" + std::to_string(i))});
        }
    };
    
    // 复杂查询性能测试
    BENCHMARK("Complex WHERE query") {
        // 测试复杂条件查询的性能
    };
}
```

### 5.4 内存安全测试
```bash
# AddressSanitizer
g++ -fsanitize=address -g -O1 *.cpp -o tinydb_asan

# MemorySanitizer  
g++ -fsanitize=memory -g -O1 *.cpp -o tinydb_msan

# ThreadSanitizer (如果实现多线程)
g++ -fsanitize=thread -g -O1 *.cpp -o tinydb_tsan

# Valgrind
valgrind --tool=memcheck --leak-check=full ./tinydb
```

---

## 6. 可能的难点与解决思路

### 6.1 解析难点

#### 6.1.1 字符串解析
**难点**: 正确处理引号内的内容，忽略引号内的关键字和分号
```cpp
// 解决方案：状态机解析
class StringTokenizer {
private:
    enum State { NORMAL, IN_STRING, ESCAPE };
    State currentState = NORMAL;
    
public:
    std::vector<Token> tokenize(const std::string& input) {
        std::vector<Token> tokens;
        std::string current;
        
        for (char c : input) {
            switch (currentState) {
                case NORMAL:
                    if (c == '"') {
                        currentState = IN_STRING;
                        // 开始字符串token
                    } else if (isspace(c)) {
                        // 结束当前token
                    } else {
                        current += c;
                    }
                    break;
                    
                case IN_STRING:
                    if (c == '"') {
                        currentState = NORMAL;
                        // 结束字符串token
                    } else {
                        current += c; // 保留所有字符，包括分号
                    }
                    break;
            }
        }
        return tokens;
    }
};
```

#### 6.1.2 WHERE条件解析
**难点**: 支持复杂的逻辑表达式和操作符优先级
```cpp
// 解决方案：递归下降解析
class ConditionParser {
public:
    std::unique_ptr<Condition> parseCondition(TokenStream& tokens) {
        return parseOrExpression(tokens);
    }
    
private:
    std::unique_ptr<Condition> parseOrExpression(TokenStream& tokens) {
        auto left = parseAndExpression(tokens);
        
        while (tokens.peek().value == "OR") {
            tokens.consume(); // consume OR
            auto right = parseAndExpression(tokens);
            left = std::make_unique<LogicalCondition>(std::move(left), LogicalOperator::OR, std::move(right));
        }
        
        return left;
    }
    
    std::unique_ptr<Condition> parseAndExpression(TokenStream& tokens) {
        // 类似实现，优先级更高
    }
    
    std::unique_ptr<Condition> parseComparison(TokenStream& tokens) {
        // 解析比较表达式
    }
};
```

### 6.2 内存管理难点

#### 6.2.1 大数据量处理
**难点**: 避免不必要的数据拷贝，特别是在查询结果返回时
```cpp
// 解决方案：使用视图和移动语义
class QueryResult {
private:
    std::vector<std::vector<Value>> data; // 拥有数据
    std::vector<std::reference_wrapper<const std::vector<Value>>> view; // 引用现有数据
    bool ownsData;
    
public:
    // 构造函数选择拥有或引用数据
    QueryResult(std::vector<std::vector<Value>>&& ownedData) 
        : data(std::move(ownedData)), ownsData(true) {}
        
    QueryResult(const std::vector<Row>& existingRows)
        : ownsData(false) {
        for (const auto& row : existingRows) {
            view.emplace_back(row.getValues());
        }
    }
};
```

#### 6.2.2 异常安全
**难点**: 保证数据库状态的一致性，即使在异常情况下
```cpp
// 解决方案：RAII和事务模式
class Transaction {
private:
    Database& db;
    std::vector<std::function<void()>> rollbackActions;
    bool committed = false;
    
public:
    explicit Transaction(Database& database) : db(database) {}
    
    ~Transaction() {
        if (!committed) {
            rollback();
        }
    }
    
    void addRollbackAction(std::function<void()> action) {
        rollbackActions.push_back(action);
    }
    
    void commit() {
        committed = true;
        rollbackActions.clear();
    }
    
    void rollback() {
        for (auto it = rollbackActions.rbegin(); it != rollbackActions.rend(); ++it) {
            (*it)();
        }
        rollbackActions.clear();
    }
};
```

### 6.3 性能优化难点

#### 6.3.1 查询优化
**难点**: WHERE条件的高效评估
```cpp
// 解决方案：简单索引和查询计划
class SimpleIndex {
private:
    std::unordered_map<Value, std::vector<size_t>> index; // 值到行号的映射
    std::string columnName;
    
public:
    void addRow(size_t rowIndex, const Value& value) {
        index[value].push_back(rowIndex);
    }
    
    std::vector<size_t> findRows(const Value& value) const {
        auto it = index.find(value);
        return it != index.end() ? it->second : std::vector<size_t>{};
    }
};

// 查询计划优化
class QueryPlanner {
public:
    QueryPlan optimizeQuery(const SelectStatement& stmt, const Table& table) {
        QueryPlan plan;
        
        // 1. 分析WHERE条件，选择最优的索引
        // 2. 决定扫描顺序
        // 3. 选择最优的执行策略
        
        return plan;
    }
};
```

### 6.4 扩展性难点

#### 6.4.1 新语句类型添加
**难点**: 在不修改现有代码的情况下添加新的SQL语句
```cpp
// 解决方案：访问者模式 + 工厂模式
class StatementVisitor {
public:
    virtual ~StatementVisitor() = default;
    virtual void visit(const CreateTableStatement& stmt) = 0;
    virtual void visit(const InsertStatement& stmt) = 0;
    virtual void visit(const SelectStatement& stmt) = 0;
    // 添加新语句时只需添加新的visit方法
};

class StatementFactory {
private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Statement>(TokenStream&)>> parsers;
    
public:
    void registerParser(const std::string& keyword, 
                       std::function<std::unique_ptr<Statement>(TokenStream&)> parser) {
        parsers[keyword] = parser;
    }
    
    std::unique_ptr<Statement> createStatement(TokenStream& tokens) {
        std::string keyword = tokens.peek().value;
        auto it = parsers.find(keyword);
        if (it != parsers.end()) {
            return it->second(tokens);
        }
        throw SyntaxError("Unknown statement: " + keyword);
    }
};
```

---

## 总结

这个TinyDB项目是一个很好的C++实践项目，涵盖了：

1. **现代C++特性**: 智能指针、移动语义、RAII、异常处理
2. **设计模式**: 访问者模式、工厂模式、策略模式
3. **算法与数据结构**: 解析算法、哈希表、树形结构
4. **软件工程**: 模块化设计、单元测试、错误处理

通过合理的模块划分和接口设计，项目具有良好的可扩展性和维护性。核心是要平衡功能完整性、代码质量和实现复杂度，确保在有限的开发时间内交付一个高质量的产品。