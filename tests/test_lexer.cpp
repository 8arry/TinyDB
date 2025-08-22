#include "libcore/sql/lexer.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <unordered_map>

using namespace tinydb::sql;

void testBasicTokens() {
    std::cout << "Testing basic tokens...\n";
    
    try {
        // 测试关键字
        {
            auto tokens = Lexer::tokenize("CREATE TABLE");
            assert(tokens.size() == 3); // CREATE, TABLE, EOF
            assert(tokens[0].type == TokenType::CREATE);
            assert(tokens[1].type == TokenType::TABLE);
            assert(tokens[2].type == TokenType::END_OF_FILE);
        }
        
        // 测试标识符
        {
            auto tokens = Lexer::tokenize("user_table my_column");
            assert(tokens.size() == 3); // identifier, identifier, EOF
            assert(tokens[0].type == TokenType::IDENTIFIER);
            assert(tokens[0].getStringValue() == "user_table");
            assert(tokens[1].type == TokenType::IDENTIFIER);
            assert(tokens[1].getStringValue() == "my_column");
        }
        
        // 测试整数字面量
        {
            auto tokens = Lexer::tokenize("123 456");
            assert(tokens.size() == 3); // integer, integer, EOF
            assert(tokens[0].type == TokenType::INTEGER);
            assert(tokens[0].getIntValue() == 123);
            assert(tokens[1].type == TokenType::INTEGER);
            assert(tokens[1].getIntValue() == 456);
        }
        
        // 测试字符串字面量
        {
            auto tokens = Lexer::tokenize("'hello' \"world\"");
            assert(tokens.size() == 3); // string, string, EOF
            assert(tokens[0].type == TokenType::STRING_LITERAL);
            assert(tokens[0].getStringValue() == "hello");
            assert(tokens[1].type == TokenType::STRING_LITERAL);
            assert(tokens[1].getStringValue() == "world");
        }
        
        std::cout << "✅ Basic tokens tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Basic tokens failed: " << e.what() << std::endl;
        throw;
    }
}

void testOperators() {
    std::cout << "Testing operators...\n";
    
    try {
        auto tokens = Lexer::tokenize("= != < > <= >=");
        assert(tokens.size() == 7); // 6 operators + EOF
        
        assert(tokens[0].type == TokenType::EQUAL);
        assert(tokens[1].type == TokenType::NOT_EQUAL);
        assert(tokens[2].type == TokenType::LESS_THAN);
        assert(tokens[3].type == TokenType::GREATER_THAN);
        assert(tokens[4].type == TokenType::LESS_EQUAL);
        assert(tokens[5].type == TokenType::GREATER_EQUAL);
        assert(tokens[6].type == TokenType::END_OF_FILE);
        
        std::cout << "✅ Operators tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Operators failed: " << e.what() << std::endl;
        throw;
    }
}

void testDelimiters() {
    std::cout << "Testing delimiters...\n";
    
    try {
        auto tokens = Lexer::tokenize("( ) , ;");
        assert(tokens.size() == 5); // 4 delimiters + EOF
        
        assert(tokens[0].type == TokenType::LEFT_PAREN);
        assert(tokens[1].type == TokenType::RIGHT_PAREN);
        assert(tokens[2].type == TokenType::COMMA);
        assert(tokens[3].type == TokenType::SEMICOLON);
        assert(tokens[4].type == TokenType::END_OF_FILE);
        
        std::cout << "✅ Delimiters tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Delimiters failed: " << e.what() << std::endl;
        throw;
    }
}

void testComplexSQL() {
    std::cout << "Testing complex SQL statements...\n";
    
    try {
        // CREATE TABLE语句
        {
            std::string sql = "CREATE TABLE users (id INT, name STRING);";
            auto tokens = Lexer::tokenize(sql);
            
            // 预期token序列
            std::vector<TokenType> expected = {
                TokenType::CREATE, TokenType::TABLE, TokenType::IDENTIFIER,
                TokenType::LEFT_PAREN, TokenType::IDENTIFIER, TokenType::INT,
                TokenType::COMMA, TokenType::IDENTIFIER, TokenType::STRING,
                TokenType::RIGHT_PAREN, TokenType::SEMICOLON, TokenType::END_OF_FILE
            };
            
            assert(tokens.size() == expected.size());
            for (size_t i = 0; i < expected.size(); ++i) {
                assert(tokens[i].type == expected[i]);
            }
            
            assert(tokens[2].getStringValue() == "users");  // table name
            assert(tokens[4].getStringValue() == "id");     // column name
            assert(tokens[7].getStringValue() == "name");   // column name
        }
        
        // INSERT语句
        {
            std::string sql = "INSERT INTO users VALUES (1, 'Alice');";
            auto tokens = Lexer::tokenize(sql);
            
            std::vector<TokenType> expected = {
                TokenType::INSERT, TokenType::INTO, TokenType::IDENTIFIER,
                TokenType::VALUES, TokenType::LEFT_PAREN, TokenType::INTEGER,
                TokenType::COMMA, TokenType::STRING_LITERAL, TokenType::RIGHT_PAREN,
                TokenType::SEMICOLON, TokenType::END_OF_FILE
            };
            
            assert(tokens.size() == expected.size());
            for (size_t i = 0; i < expected.size(); ++i) {
                assert(tokens[i].type == expected[i]);
            }
            
            assert(tokens[2].getStringValue() == "users");
            assert(tokens[5].getIntValue() == 1);
            assert(tokens[7].getStringValue() == "Alice");
        }
        
        // SELECT with WHERE语句
        {
            std::string sql = "SELECT name FROM users WHERE age >= 18 AND active = 'true';";
            auto tokens = Lexer::tokenize(sql);
            
            // 验证关键部分
            assert(tokens[0].type == TokenType::SELECT);
            assert(tokens[1].type == TokenType::IDENTIFIER);
            assert(tokens[1].getStringValue() == "name");
            assert(tokens[2].type == TokenType::FROM);
            assert(tokens[3].type == TokenType::IDENTIFIER);
            assert(tokens[3].getStringValue() == "users");
            assert(tokens[4].type == TokenType::WHERE);
            
            // 查找WHERE条件中的操作符
            bool foundGreaterEqual = false;
            bool foundAnd = false;
            bool foundEqual = false;
            
            for (const auto& token : tokens) {
                if (token.type == TokenType::GREATER_EQUAL) foundGreaterEqual = true;
                if (token.type == TokenType::AND) foundAnd = true;
                if (token.type == TokenType::EQUAL) foundEqual = true;
            }
            
            assert(foundGreaterEqual);
            assert(foundAnd);
            assert(foundEqual);
        }
        
        std::cout << "✅ Complex SQL tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Complex SQL failed: " << e.what() << std::endl;
        throw;
    }
}

void testWhitespaceHandling() {
    std::cout << "Testing whitespace handling...\n";
    
    try {
        // 测试空白符过滤
        {
            std::string sql = "  SELECT  \t\n  name  \r\n  FROM  users  ";
            auto tokens = Lexer::tokenize(sql);
            
            // 不应包含空白符
            for (const auto& token : tokens) {
                assert(token.type != TokenType::WHITESPACE);
            }
            
            assert(tokens[0].type == TokenType::SELECT);
            assert(tokens[1].type == TokenType::IDENTIFIER);
            assert(tokens[1].getStringValue() == "name");
            assert(tokens[2].type == TokenType::FROM);
            assert(tokens[3].type == TokenType::IDENTIFIER);
            assert(tokens[3].getStringValue() == "users");
            assert(tokens[4].type == TokenType::END_OF_FILE);
        }
        
        // 测试包含空白符的版本
        {
            std::string sql = "SELECT name";
            auto tokensWithWS = Lexer::tokenizeWithWhitespace(sql);
            auto tokensWithoutWS = Lexer::tokenize(sql);
            
            assert(tokensWithWS.size() > tokensWithoutWS.size()); // 应该有更多token
            
            // 过滤空白符后应该相同
            auto filtered = LexerUtils::filterWhitespace(tokensWithWS);
            assert(filtered.size() == tokensWithoutWS.size());
            
            for (size_t i = 0; i < filtered.size(); ++i) {
                assert(filtered[i].type == tokensWithoutWS[i].type);
            }
        }
        
        std::cout << "✅ Whitespace handling tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Whitespace handling failed: " << e.what() << std::endl;
        throw;
    }
}

void testStringEscaping() {
    std::cout << "Testing string escaping...\n";
    
    try {
        // 测试转义字符
        {
            auto tokens = Lexer::tokenize("'Hello\\nWorld' \"Tab\\tSeparated\"");
            assert(tokens.size() == 3); // 2 strings + EOF
            
            assert(tokens[0].type == TokenType::STRING_LITERAL);
            assert(tokens[0].getStringValue() == "Hello\nWorld");
            
            assert(tokens[1].type == TokenType::STRING_LITERAL);
            assert(tokens[1].getStringValue() == "Tab\tSeparated");
        }
        
        // 测试引号转义
        {
            auto tokens = Lexer::tokenize("'It\\'s a test' \"He said \\\"Hello\\\"\"");
            assert(tokens.size() == 3); // 2 strings + EOF
            
            assert(tokens[0].getStringValue() == "It's a test");
            assert(tokens[1].getStringValue() == "He said \"Hello\"");
        }
        
        std::cout << "✅ String escaping tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ String escaping failed: " << e.what() << std::endl;
        throw;
    }
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    try {
        // 测试未终止的字符串
        try {
            Lexer::tokenize("'unterminated string");
            assert(false && "Should have thrown LexerError");
        } catch (const LexerError& e) {
            // 预期的异常
            assert(std::string(e.what()).find("Unterminated") != std::string::npos);
        }
        
        // 测试无效字符
        try {
            Lexer::tokenize("SELECT @ FROM users");
            assert(false && "Should have thrown LexerError");
        } catch (const LexerError& e) {
            // 预期的异常
            assert(std::string(e.what()).find("Unexpected") != std::string::npos);
        }
        
        // 测试不完整的操作符
        try {
            Lexer::tokenize("SELECT name FROM users WHERE age ! 18");
            assert(false && "Should have thrown LexerError");
        } catch (const LexerError& e) {
            // 预期的异常
            assert(std::string(e.what()).find("!") != std::string::npos);
        }
        
        std::cout << "✅ Error handling tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Error handling failed: " << e.what() << std::endl;
        throw;
    }
}

void testLexerUtils() {
    std::cout << "Testing LexerUtils...\n";
    
    try {
        // 测试Token验证
        {
            auto validTokens = Lexer::tokenize("SELECT * FROM users;");
            assert(LexerUtils::validateTokenSequence(validTokens));
            assert(!LexerUtils::hasBasicSyntaxErrors(validTokens));
        }
        
        // 测试括号匹配检查
        {
            auto tokens = Lexer::tokenize("SELECT name FROM users WHERE (age > 18");
            // 移除EOF来模拟不匹配的括号
            tokens.pop_back();
            assert(!LexerUtils::validateTokenSequence(tokens));
        }
        
        // 测试Token查找
        {
            auto tokens = Lexer::tokenize("SELECT name, age FROM users WHERE age > 18;");
            auto commaPositions = LexerUtils::findTokensOfType(tokens, TokenType::COMMA);
            assert(commaPositions.size() == 1);
            
            auto identifierPositions = LexerUtils::findTokensOfType(tokens, TokenType::IDENTIFIER);
            assert(identifierPositions.size() >= 3); // name, age, users (至少)
        }
        
        // 测试格式化输出
        {
            auto tokens = Lexer::tokenize("SELECT name FROM users;");
            std::string formatted = LexerUtils::formatTokens(tokens);
            assert(!formatted.empty());
            assert(formatted.find("SELECT") != std::string::npos);
        }
        
        std::cout << "✅ LexerUtils tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ LexerUtils failed: " << e.what() << std::endl;
        throw;
    }
}

void testPositionTracking() {
    std::cout << "Testing position tracking...\n";
    
    try {
        std::string sql = "SELECT\nname,\n  age\nFROM users;";
        auto tokens = Lexer::tokenize(sql);
        
        // 检查行号和列号是否正确跟踪
        bool foundNewlines = false;
        for (const auto& token : tokens) {
            if (token.line > 1) {
                foundNewlines = true;
                break;
            }
        }
        assert(foundNewlines); // 应该检测到多行
        
        // 检查位置信息
        for (const auto& token : tokens) {
            assert(token.line >= 1);
            assert(token.column >= 1);
            assert(token.position <= sql.length() || token.type == TokenType::END_OF_FILE);
        }
        
        std::cout << "✅ Position tracking tests passed!\n";
    } catch (const std::exception& e) {
        std::cout << "❌ Position tracking failed: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "🧪 Testing SQL Lexer with C++23 features...\n\n";
    
    try {
        testBasicTokens();
        testOperators();
        testDelimiters();
        testComplexSQL();
        testWhitespaceHandling();
        testStringEscaping();
        testErrorHandling();
        testLexerUtils();
        testPositionTracking();
        
        std::cout << "\n🎉 All Lexer tests passed successfully! ✅\n";
        
        // 演示词法分析器功能
        std::cout << "\n📋 Demo: Comprehensive SQL tokenization\n";
        
        std::string demoSQL = R"(
            CREATE TABLE employees (
                id INT,
                name STRING,
                department STRING,
                salary INT
            );
            
            INSERT INTO employees VALUES (1, 'Alice Johnson', 'Engineering', 75000);
            
            SELECT name, salary 
            FROM employees 
            WHERE department = 'Engineering' AND salary >= 70000;
            
            UPDATE employees 
            SET salary = salary * 110 / 100 
            WHERE department = 'Engineering';
            
            DELETE FROM employees WHERE salary < 50000;
        )";
        
        std::cout << "Input SQL:\n" << demoSQL << std::endl;
        
        Lexer lexer(demoSQL);
        auto tokens = lexer.tokenize();
        
        std::cout << "\nTokenized result (" << tokens.size() << " tokens):\n";
        std::cout << LexerUtils::formatTokens(tokens, false) << std::endl;
        
        std::cout << "\nDetailed token analysis:\n";
        
        // 统计各种token类型
        std::unordered_map<TokenType, int> tokenCounts;
        for (const auto& token : tokens) {
            tokenCounts[token.type]++;
        }
        
        std::cout << "Token type statistics:\n";
        for (const auto& [type, count] : tokenCounts) {
            if (count > 0) {
                std::cout << "  " << type << ": " << count << std::endl;
            }
        }
        
        // 验证token序列
        if (LexerUtils::validateTokenSequence(tokens)) {
            std::cout << "✅ Token sequence is syntactically valid\n";
        } else {
            std::cout << "❌ Token sequence has syntax errors\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
