#pragma once

#include "token.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>
#include <stdexcept>

namespace tinydb::sql {

// 解析器异常
class ParseError : public std::runtime_error {
private:
    size_t position_;
    std::string expected_;
    std::string actual_;

public:
    ParseError(const std::string& message, size_t position = 0)
        : std::runtime_error(message), position_(position) {}
    
    ParseError(const std::string& expected, const std::string& actual, size_t position)
        : std::runtime_error("Expected '" + expected + "', but got '" + actual + "'")
        , position_(position), expected_(expected), actual_(actual) {}
    
    size_t getPosition() const { return position_; }
    const std::string& getExpected() const { return expected_; }
    const std::string& getActual() const { return actual_; }
};

// SQL解析器
class Parser {
private:
    std::vector<Token> tokens_;
    size_t current_;
    
    // 辅助方法
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    
    // 解析方法
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    std::unique_ptr<UpdateStatement> parseUpdate();
    std::unique_ptr<DeleteStatement> parseDelete();
    
    // 表达式解析
    ExpressionPtr parseExpression();
    ExpressionPtr parseLiteral();
    ExpressionPtr parseColumn();
    
    // JOIN解析
    std::vector<std::unique_ptr<JoinClause>> parseJoins();
    std::unique_ptr<JoinClause> parseJoin();
    
    // 条件解析
    std::unique_ptr<tinydb::Condition> parseCondition();
    std::unique_ptr<tinydb::Condition> parseComparisonCondition();
    
    // 数据类型解析
    tinydb::DataType parseDataType();
    
    // 错误处理
    void synchronize();

public:
    explicit Parser(std::vector<Token> tokens);
    
    // 解析单个SQL语句
    std::unique_ptr<Statement> parse();
    
    // 解析多个SQL语句
    std::vector<std::unique_ptr<Statement>> parseMultiple();
};

} // namespace tinydb::sql
