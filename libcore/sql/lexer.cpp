#include "lexer.hpp"
#include <iostream>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace tinydb::sql {

// Lexer 主要方法实现
std::vector<Token> Lexer::tokenize() {
    tokens_.clear();
    current_ = 0;
    line_ = 1;
    column_ = 1;
    
    scanTokens(false); // 不包含空白符
    return tokens_;
}

std::vector<Token> Lexer::tokenizeWithWhitespace() {
    tokens_.clear();
    current_ = 0;
    line_ = 1;
    column_ = 1;
    
    scanTokens(true); // 包含空白符
    return tokens_;
}

void Lexer::scanTokens(bool includeWhitespace) {
    while (!isAtEnd()) {
        scanToken(includeWhitespace);
    }
    
    // 添加EOF标记
    addToken(TokenType::END_OF_FILE);
}

void Lexer::scanToken(bool includeWhitespace) {
    char c = advance();
    
    switch (c) {
        // 单字符Token
        case '(':
            addToken(TokenType::LEFT_PAREN);
            break;
        case ')':
            addToken(TokenType::RIGHT_PAREN);
            break;
        case ',':
            addToken(TokenType::COMMA);
            break;
        case ';':
            addToken(TokenType::SEMICOLON);
            break;
        case '*':
            addToken(TokenType::ASTERISK);
            break;
        case '/':
            addToken(TokenType::SLASH);
            break;
        case '.':
            addToken(TokenType::DOT);
            break;
            
        // SQL注释处理
        case '-':
            if (match('-')) {
                // SQL注释 "--"，跳过到行尾
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else {
                error("Unexpected character '-' (did you mean '--' for comment?)");
            }
            break;
            
        // 可能的双字符操作符
        case '=':
            addToken(TokenType::EQUAL);
            break;
        case '!':
            if (match('=')) {
                addToken(TokenType::NOT_EQUAL);
            } else {
                error("Unexpected character '!' (did you mean '!='?)");
            }
            break;
        case '<':
            if (match('=')) {
                addToken(TokenType::LESS_EQUAL);
            } else {
                addToken(TokenType::LESS_THAN);
            }
            break;
        case '>':
            if (match('=')) {
                addToken(TokenType::GREATER_EQUAL);
            } else {
                addToken(TokenType::GREATER_THAN);
            }
            break;
            
        // 字符串字面量
        case '\'':
        case '"':
            current_--; // 回退，让scanString处理
            scanString();
            break;
            
        // 空白符
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            current_--; // 回退，让scanWhitespace处理
            if (includeWhitespace) {
                scanWhitespace();
            } else {
                // 跳过空白符，但要更新位置
                advance();
            }
            break;
            
        default:
            if (TokenUtils::isDigit(c)) {
                current_--; // 回退，让scanNumber处理
                scanNumber();
            } else if (TokenUtils::isIdentifierStart(c)) {
                current_--; // 回退，让scanIdentifier处理
                scanIdentifier();
            } else {
                error("Unexpected character: '" + std::string(1, c) + "'");
            }
            break;
    }
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    
    char c = source_[current_++];
    updatePosition(c);
    return c;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_[current_] != expected) return false;
    
    advance();
    return true;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

void Lexer::updatePosition(char c) {
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
}

void Lexer::addToken(TokenType type) {
    tokens_.emplace_back(type, current_, line_, column_);
}

void Lexer::addToken(TokenType type, const std::string& value) {
    tokens_.emplace_back(type, value, current_, line_, column_);
}

void Lexer::addToken(TokenType type, int value) {
    tokens_.emplace_back(type, value, current_, line_, column_);
}

void Lexer::scanString() {
    char quote = advance(); // 获取引号类型
    std::string value;
    
    while (!isAtEnd() && peek() != quote) {
        char c = advance();
        if (c == '\\' && !isAtEnd()) {
            // 处理转义字符
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;
                case '"': value += '"'; break;
                default:
                    value += escaped; // 其他字符直接添加
                    break;
            }
        } else {
            value += c;
        }
    }
    
    if (isAtEnd()) {
        error("Unterminated string literal");
    }
    
    // 消费结束引号
    advance();
    
    addToken(TokenType::STRING_LITERAL, value);
}

void Lexer::scanNumber() {
    std::string value;
    
    // 扫描数字字符
    while (!isAtEnd() && TokenUtils::isDigit(peek())) {
        value += advance();
    }
    
    // 转换为整数
    try {
        int intValue = std::stoi(value);
        addToken(TokenType::INTEGER, intValue);
    } catch (const std::exception&) {
        error("Invalid number: " + value);
    }
}

void Lexer::scanIdentifier() {
    std::string value;
    
    // 第一个字符必须是字母或下划线
    if (!TokenUtils::isIdentifierStart(peek())) {
        error("Invalid identifier start");
    }
    
    // 扫描标识符字符
    while (!isAtEnd() && TokenUtils::isIdentifierChar(peek())) {
        value += advance();
    }
    
    // 检查是否为关键字
    TokenType type = TokenUtils::stringToTokenType(value);
    
    if (type == TokenType::IDENTIFIER) {
        addToken(TokenType::IDENTIFIER, value);
    } else {
        addToken(type); // 关键字
    }
}

void Lexer::scanWhitespace() {
    std::string value;
    
    while (!isAtEnd() && TokenUtils::isWhitespace(peek())) {
        value += advance();
    }
    
    addToken(TokenType::WHITESPACE, value);
}

void Lexer::error(const std::string& message) {
    throw LexerError(message, current_, line_, column_);
}

void Lexer::printTokens() const {
    std::cout << "Tokens:\n";
    for (const auto& token : tokens_) {
        std::cout << "  " << token << "\n";
    }
}

std::string Lexer::tokensToString() const {
    std::ostringstream oss;
    for (size_t i = 0; i < tokens_.size(); ++i) {
        if (i > 0) oss << " ";
        oss << tokens_[i].type;
        
        if (tokens_[i].hasStringValue()) {
            oss << "(\"" << tokens_[i].getStringValue() << "\")";
        } else if (tokens_[i].hasIntValue()) {
            oss << "(" << tokens_[i].getIntValue() << ")";
        }
    }
    return oss.str();
}

// LexerUtils 实现
bool LexerUtils::validateTokenSequence(const std::vector<Token>& tokens) {
    if (tokens.empty()) return false;
    
    // 基本验证：最后一个应该是EOF
    if (tokens.back().type != TokenType::END_OF_FILE) {
        return false;
    }
    
    // 检查括号匹配
    int parenCount = 0;
    for (const auto& token : tokens) {
        if (token.type == TokenType::LEFT_PAREN) {
            parenCount++;
        } else if (token.type == TokenType::RIGHT_PAREN) {
            parenCount--;
            if (parenCount < 0) return false; // 右括号多于左括号
        }
    }
    
    return parenCount == 0; // 括号必须匹配
}

std::vector<Token> LexerUtils::filterWhitespace(const std::vector<Token>& tokens) {
    std::vector<Token> filtered;
    filtered.reserve(tokens.size());
    
    for (const auto& token : tokens) {
        if (token.type != TokenType::WHITESPACE) {
            filtered.push_back(token);
        }
    }
    
    return filtered;
}

std::vector<size_t> LexerUtils::findTokensOfType(const std::vector<Token>& tokens, TokenType type) {
    std::vector<size_t> positions;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == type) {
            positions.push_back(i);
        }
    }
    
    return positions;
}

std::string LexerUtils::formatTokens(const std::vector<Token>& tokens, bool verbose) {
    std::ostringstream oss;
    
    if (verbose) {
        oss << "Token Sequence (" << tokens.size() << " tokens):\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            oss << "  [" << i << "] " << tokens[i] << "\n";
        }
    } else {
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) oss << " ";
            oss << tokens[i].type;
        }
    }
    
    return oss.str();
}

bool LexerUtils::hasBasicSyntaxErrors(const std::vector<Token>& tokens) {
    // 基本语法检查
    if (tokens.empty()) return true;
    
    // 检查是否有未知Token
    for (const auto& token : tokens) {
        if (token.type == TokenType::UNKNOWN) {
            return true;
        }
    }
    
    // 检查括号匹配
    if (!validateTokenSequence(tokens)) {
        return true;
    }
    
    return false;
}

} // namespace tinydb::sql
