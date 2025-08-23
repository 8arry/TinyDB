#pragma once

#include "token.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <stdexcept>

namespace tinydb::sql {

// Lexical analysis exception
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

// Lexical analyzer class
class Lexer {
private:
    std::string source_;        // Source SQL text
    size_t current_;           // Current character position
    size_t line_;              // Current line number
    size_t column_;            // Current column number
    std::vector<Token> tokens_; // Parsed token list

public:
    // Constructors
    explicit Lexer(std::string source) 
        : source_(std::move(source)), current_(0), line_(1), column_(1) {}
    
    // Disable copy, enable move
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;
    
    // Main interface
    std::vector<Token> tokenize();
    std::vector<Token> tokenizeWithWhitespace(); // Version including whitespace
    
    // Get results
    const std::vector<Token>& getTokens() const { return tokens_; }
    
    // Debug output
    void printTokens() const;
    std::string tokensToString() const;
    
    // Static convenience methods
    static std::vector<Token> tokenize(const std::string& source) {
        Lexer lexer(source);
        return lexer.tokenize();
    }
    
    static std::vector<Token> tokenizeWithWhitespace(const std::string& source) {
        Lexer lexer(source);
        return lexer.tokenizeWithWhitespace();
    }

private:
    // Internal parsing methods
    void scanTokens(bool includeWhitespace = false);
    void scanToken(bool includeWhitespace = false);
    
    // Character processing
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    bool isAtEnd() const;
    
    // Token creation
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& value);
    void addToken(TokenType type, int value);
    
    // Specialized scanning methods
    void scanString();
    void scanNumber();
    void scanIdentifier();
    void scanWhitespace();
    
    // Operator processing
    void scanOperator();
    
    // Error handling
    void error(const std::string& message);
    
    // Position management
    void updatePosition(char c);
    size_t getCurrentPosition() const { return current_; }
    size_t getCurrentLine() const { return line_; }
    size_t getCurrentColumn() const { return column_; }
};

// Lexical analysis utility class
class LexerUtils {
public:
    // Validate token sequence
    static bool validateTokenSequence(const std::vector<Token>& tokens);
    
    // Filter whitespace
    static std::vector<Token> filterWhitespace(const std::vector<Token>& tokens);
    
    // Find specific type tokens
    static std::vector<size_t> findTokensOfType(const std::vector<Token>& tokens, TokenType type);
    
    // Token sequence formatted output
    static std::string formatTokens(const std::vector<Token>& tokens, bool verbose = false);
    
    // Check if token sequence contains syntax errors
    static bool hasBasicSyntaxErrors(const std::vector<Token>& tokens);
};

} // namespace tinydb::sql
