#pragma once

#include "token.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <stdexcept>

namespace tinydb::sql {

// 词法分析异常
class LexerError : public std::runtime_error {
private:
    size_t position_;
    size_t line_;
    size_t column_;

public:
    LexerError(const std::string& message, size_t pos, size_t line, size_t col)
        : std::runtime_error(message), position_(pos), line_(line), column_(col) {}
    
    size_t getPosition() const { return position_; }
    size_t getLine() const { return line_; }
    size_t getColumn() const { return column_; }
    
    std::string getDetailedMessage() const {
        return what() + std::string(" at line ") + std::to_string(line_) 
               + ", column " + std::to_string(column_);
    }
};

// 词法分析器类
class Lexer {
private:
    std::string source_;        // 源SQL文本
    size_t current_;           // 当前字符位置
    size_t line_;              // 当前行号
    size_t column_;            // 当前列号
    std::vector<Token> tokens_; // 已解析的Token列表

public:
    // 构造函数
    explicit Lexer(std::string source) 
        : source_(std::move(source)), current_(0), line_(1), column_(1) {}
    
    // 禁用拷贝，启用移动
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;
    
    // 主要接口
    std::vector<Token> tokenize();
    std::vector<Token> tokenizeWithWhitespace(); // 包含空白符的版本
    
    // 获取结果
    const std::vector<Token>& getTokens() const { return tokens_; }
    
    // 调试输出
    void printTokens() const;
    std::string tokensToString() const;
    
    // 静态便利方法
    static std::vector<Token> tokenize(const std::string& source) {
        Lexer lexer(source);
        return lexer.tokenize();
    }
    
    static std::vector<Token> tokenizeWithWhitespace(const std::string& source) {
        Lexer lexer(source);
        return lexer.tokenizeWithWhitespace();
    }

private:
    // 内部解析方法
    void scanTokens(bool includeWhitespace = false);
    void scanToken(bool includeWhitespace = false);
    
    // 字符处理
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    bool isAtEnd() const;
    
    // Token创建
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& value);
    void addToken(TokenType type, int value);
    
    // 专用扫描方法
    void scanString();
    void scanNumber();
    void scanIdentifier();
    void scanWhitespace();
    
    // 操作符处理
    void scanOperator();
    
    // 错误处理
    void error(const std::string& message);
    
    // 位置管理
    void updatePosition(char c);
    size_t getCurrentPosition() const { return current_; }
    size_t getCurrentLine() const { return line_; }
    size_t getCurrentColumn() const { return column_; }
};

// 词法分析工具类
class LexerUtils {
public:
    // 验证Token序列
    static bool validateTokenSequence(const std::vector<Token>& tokens);
    
    // 过滤空白符
    static std::vector<Token> filterWhitespace(const std::vector<Token>& tokens);
    
    // 查找特定类型的Token
    static std::vector<size_t> findTokensOfType(const std::vector<Token>& tokens, TokenType type);
    
    // Token序列格式化输出
    static std::string formatTokens(const std::vector<Token>& tokens, bool verbose = false);
    
    // 检查Token序列是否包含语法错误
    static bool hasBasicSyntaxErrors(const std::vector<Token>& tokens);
};

} // namespace tinydb::sql
