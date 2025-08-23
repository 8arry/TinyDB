#include "parser.hpp"
#include "../database/condition.hpp"

namespace tinydb::sql {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), current_(0) {}

// 辅助方法实现
bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    if (current_ >= tokens_.size()) {
        static Token eof{TokenType::END_OF_FILE, "", 0, 0, 0};
        return eof;
    }
    return tokens_[current_];
}

const Token& Parser::previous() const {
    if (current_ == 0) {
        static Token start{TokenType::END_OF_FILE, "", 0, 0, 0};
        return start;
    }
    return tokens_[current_ - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    throw ParseError(message + " at position " + std::to_string(peek().position));
}

// 主解析方法
std::unique_ptr<Statement> Parser::parse() {
    try {
        return parseStatement();
    } catch (const ParseError& e) {
        synchronize();
        throw;
    }
}

std::vector<std::unique_ptr<Statement>> Parser::parseMultiple() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
            
            // 跳过分号
            match(TokenType::SEMICOLON);
        } catch (const ParseError& e) {
            synchronize();
            throw;
        }
    }
    
    return statements;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::CREATE)) {
        return parseCreateTable();
    }
    if (match(TokenType::INSERT)) {
        return parseInsert();
    }
    if (match(TokenType::SELECT)) {
        return parseSelect();
    }
    if (match(TokenType::UPDATE)) {
        return parseUpdate();
    }
    if (match(TokenType::DELETE)) {
        return parseDelete();
    }
    
    throw ParseError("Expected SQL statement", peek().position);
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    consume(TokenType::TABLE, "Expected 'TABLE' after 'CREATE'");
    
    Token tableName = consume(TokenType::IDENTIFIER, "Expected table name");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after table name");
    
    std::vector<tinydb::Column> columns;
    
    do {
        Token columnName = consume(TokenType::IDENTIFIER, "Expected column name");
        tinydb::DataType dataType = parseDataType();
        
        columns.push_back(tinydb::Column(columnName.getStringValue(), dataType));
        
    } while (match(TokenType::COMMA));
    
    consume(TokenType::RIGHT_PAREN, "Expected ')' after column definitions");
    
    return std::make_unique<CreateTableStatement>(tableName.getStringValue(), std::move(columns));
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    consume(TokenType::INTO, "Expected 'INTO' after 'INSERT'");
    
    Token tableName = consume(TokenType::IDENTIFIER, "Expected table name");
    
    std::vector<std::string> columns;
    
    // 可选的列名列表
    if (match(TokenType::LEFT_PAREN)) {
        do {
            Token columnName = consume(TokenType::IDENTIFIER, "Expected column name");
            columns.push_back(columnName.getStringValue());
        } while (match(TokenType::COMMA));
        
        consume(TokenType::RIGHT_PAREN, "Expected ')' after column list");
    }
    
    consume(TokenType::VALUES, "Expected 'VALUES'");
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'VALUES'");
    
    std::vector<ExpressionPtr> values;
    
    do {
        values.push_back(parseExpression());
    } while (match(TokenType::COMMA));
    
    consume(TokenType::RIGHT_PAREN, "Expected ')' after values");
    
    return std::make_unique<InsertStatement>(tableName.getStringValue(), std::move(columns), std::move(values));
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    std::vector<std::string> columns;
    
    // 解析列列表或 *
    if (match(TokenType::ASTERISK)) {
        // SELECT * - 空列表表示选择所有列
    } else {
        do {
            // 支持限定列名：table.column 或 column
            Token columnName = consume(TokenType::IDENTIFIER, "Expected column name");
            std::string fullColumnName = columnName.getStringValue();
            
            // 检查是否有表名限定符（table.column）
            if (match(TokenType::DOT)) {
                Token actualColumnName = consume(TokenType::IDENTIFIER, "Expected column name after '.'");
                fullColumnName = fullColumnName + "." + actualColumnName.getStringValue();
            }
            
            columns.push_back(fullColumnName);
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::FROM, "Expected 'FROM'");
    Token tableName = consume(TokenType::IDENTIFIER, "Expected table name");
    
    // 解析JOIN子句
    std::vector<std::unique_ptr<JoinClause>> joins = parseJoins();
    
    // WHERE子句解析
    std::unique_ptr<tinydb::Condition> whereCondition = nullptr;
    if (match(TokenType::WHERE)) {
        whereCondition = parseCondition();
    }
    
    return std::make_unique<SelectStatement>(
        std::move(columns), 
        tableName.getStringValue(), 
        std::move(joins),
        std::move(whereCondition)
    );
}

std::unique_ptr<UpdateStatement> Parser::parseUpdate() {
    Token tableName = consume(TokenType::IDENTIFIER, "Expected table name");
    
    consume(TokenType::SET, "Expected 'SET'");
    
    std::vector<std::pair<std::string, ExpressionPtr>> assignments;
    
    do {
        Token columnName = consume(TokenType::IDENTIFIER, "Expected column name");
        consume(TokenType::EQUAL, "Expected '=' after column name");
        ExpressionPtr value = parseExpression();
        
        assignments.emplace_back(columnName.getStringValue(), std::move(value));
        
    } while (match(TokenType::COMMA));
    
    // WHERE子句解析
    std::unique_ptr<tinydb::Condition> whereCondition = nullptr;
    if (match(TokenType::WHERE)) {
        whereCondition = parseCondition();
    }
    
    return std::make_unique<UpdateStatement>(tableName.getStringValue(), std::move(assignments), std::move(whereCondition));
}

std::unique_ptr<DeleteStatement> Parser::parseDelete() {
    consume(TokenType::FROM, "Expected 'FROM' after 'DELETE'");
    Token tableName = consume(TokenType::IDENTIFIER, "Expected table name");
    
    // WHERE子句解析
    std::unique_ptr<tinydb::Condition> whereCondition = nullptr;
    if (match(TokenType::WHERE)) {
        whereCondition = parseCondition();
    }
    
    return std::make_unique<DeleteStatement>(tableName.getStringValue(), std::move(whereCondition));
}

// 表达式解析
ExpressionPtr Parser::parseExpression() {
    if (check(TokenType::INTEGER) || check(TokenType::STRING_LITERAL)) {
        return parseLiteral();
    }
    if (check(TokenType::IDENTIFIER)) {
        return parseColumn();
    }
    
    throw ParseError("Expected expression", peek().position);
}

ExpressionPtr Parser::parseLiteral() {
    if (match(TokenType::INTEGER)) {
        int value = previous().getIntValue();
        return std::make_unique<LiteralExpression>(tinydb::Value{value});
    }
    if (match(TokenType::STRING_LITERAL)) {
        std::string value = previous().getStringValue();
        return std::make_unique<LiteralExpression>(tinydb::Value{value});
    }
    
    throw ParseError("Expected literal value", peek().position);
}

ExpressionPtr Parser::parseColumn() {
    Token firstToken = consume(TokenType::IDENTIFIER, "Expected column name");
    
    // 检查是否有表名限定符（table.column）
    if (match(TokenType::DOT)) {
        Token columnName = consume(TokenType::IDENTIFIER, "Expected column name after '.'");
        return std::make_unique<ColumnExpression>(
            firstToken.getStringValue(),   // table name
            columnName.getStringValue()    // column name
        );
    } else {
        return std::make_unique<ColumnExpression>(firstToken.getStringValue());
    }
}

// 条件解析 - 支持逻辑运算符，从最低优先级开始
std::unique_ptr<tinydb::Condition> Parser::parseCondition() {
    return parseLogicalOr();
}

// 解析OR条件（最低优先级）
std::unique_ptr<tinydb::Condition> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    
    while (match(TokenType::OR)) {
        auto right = parseLogicalAnd();
        left = std::make_unique<tinydb::LogicalCondition>(
            std::move(left), 
            tinydb::LogicalOp::OR, 
            std::move(right)
        );
    }
    
    return left;
}

// 解析AND条件（中等优先级）
std::unique_ptr<tinydb::Condition> Parser::parseLogicalAnd() {
    auto left = parseComparisonCondition();
    
    while (match(TokenType::AND)) {
        auto right = parseComparisonCondition();
        left = std::make_unique<tinydb::LogicalCondition>(
            std::move(left), 
            tinydb::LogicalOp::AND, 
            std::move(right)
        );
    }
    
    return left;
}

std::unique_ptr<tinydb::Condition> Parser::parseComparisonCondition() {
    // 左操作数 - 必须是列名，支持限定列名
    if (!check(TokenType::IDENTIFIER)) {
        throw ParseError("Expected column name in condition", peek().position);
    }
    
    Token leftToken = advance();
    std::string leftColumnName = leftToken.getStringValue();
    
    // 检查是否有表名限定符
    if (match(TokenType::DOT)) {
        Token columnToken = consume(TokenType::IDENTIFIER, "Expected column name after '.'");
        leftColumnName = leftColumnName + "." + columnToken.getStringValue();
    }
    
    auto leftValue = tinydb::ConditionValue::column(leftColumnName);
    
    // 操作符
    tinydb::ComparisonOp op;
    if (match(TokenType::EQUAL)) {
        op = tinydb::ComparisonOp::EQUAL;
    } else if (match(TokenType::NOT_EQUAL)) {
        op = tinydb::ComparisonOp::NOT_EQUAL;
    } else if (match(TokenType::LESS_THAN)) {
        op = tinydb::ComparisonOp::LESS_THAN;
    } else if (match(TokenType::GREATER_THAN)) {
        op = tinydb::ComparisonOp::GREATER_THAN;
    } else if (match(TokenType::LESS_EQUAL)) {
        op = tinydb::ComparisonOp::LESS_EQUAL;
    } else if (match(TokenType::GREATER_EQUAL)) {
        op = tinydb::ComparisonOp::GREATER_EQUAL;
    } else {
        throw ParseError("Expected comparison operator (=, !=, <, >, <=, >=)", peek().position);
    }
    
    // 右操作数 - 可以是字面量或列名
    tinydb::ConditionValue rightValue = tinydb::ConditionValue::literal(0); // 临时初始化
    if (match(TokenType::INTEGER)) {
        int value = previous().getIntValue();
        rightValue = tinydb::ConditionValue::literal(value);
    } else if (match(TokenType::STRING_LITERAL)) {
        std::string value = previous().getStringValue();
        rightValue = tinydb::ConditionValue::literal(value);
    } else if (match(TokenType::IDENTIFIER)) {
        std::string rightColumnName = previous().getStringValue();
        
        // 检查是否有表名限定符
        if (match(TokenType::DOT)) {
            Token columnToken = consume(TokenType::IDENTIFIER, "Expected column name after '.'");
            rightColumnName = rightColumnName + "." + columnToken.getStringValue();
        }
        
        rightValue = tinydb::ConditionValue::column(rightColumnName);
    } else {
        throw ParseError("Expected value or column name", peek().position);
    }
    
    return std::make_unique<tinydb::ComparisonCondition>(leftValue, op, rightValue);
}

// 数据类型解析
tinydb::DataType Parser::parseDataType() {
    if (match(TokenType::INT)) {
        return tinydb::DataType::INT;
    }
    if (match(TokenType::STR)) {
        return tinydb::DataType::STR;
    }
    
    throw ParseError("Expected data type (int or str)", peek().position);
}

// JOIN解析
std::vector<std::unique_ptr<JoinClause>> Parser::parseJoins() {
    std::vector<std::unique_ptr<JoinClause>> joins;
    
    while (check(TokenType::INNER)) {
        joins.push_back(parseJoin());
    }
    
    return joins;
}

std::unique_ptr<JoinClause> Parser::parseJoin() {
    consume(TokenType::INNER, "Expected 'INNER'");
    consume(TokenType::JOIN, "Expected 'JOIN'");
    
    Token joinTable = consume(TokenType::IDENTIFIER, "Expected table name");
    
    consume(TokenType::ON, "Expected 'ON'");
    auto onCondition = parseCondition();
    
    return std::make_unique<JoinClause>(
        JoinClause::JoinType::INNER,
        joinTable.getStringValue(),
        std::move(onCondition)
    );
}

// 错误恢复
void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::CREATE:
            case TokenType::INSERT:
            case TokenType::SELECT:
            case TokenType::UPDATE:
            case TokenType::DELETE:
                return;
            default:
                break;
        }
        
        advance();
    }
}

} // namespace tinydb::sql
