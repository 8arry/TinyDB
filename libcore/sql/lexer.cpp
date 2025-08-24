#include "lexer.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace tinydb::sql {

// Lexer main method implementations
std::vector<Token> Lexer::tokenize() {
    tokens_.clear();
    current_ = 0;
    line_ = 1;
    column_ = 1;

    scanTokens(false); // Exclude whitespace
    return tokens_;
}

std::vector<Token> Lexer::tokenizeWithWhitespace() {
    tokens_.clear();
    current_ = 0;
    line_ = 1;
    column_ = 1;

    scanTokens(true); // Include whitespace
    return tokens_;
}

void Lexer::scanTokens(bool includeWhitespace) {
    while (!isAtEnd()) {
        scanToken(includeWhitespace);
    }

    // Add EOF token
    addToken(TokenType::END_OF_FILE);
}

void Lexer::scanToken(bool includeWhitespace) {
    char c = advance();

    switch (c) {
        // Single character tokens
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

        // SQL comment handling
        case '-':
            if (match('-')) {
                // SQL comment "--", skip to end of line
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else {
                error("Unexpected character '-' (did you mean '--' for comment?)");
            }
            break;

        // Possible two-character operators
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

        // String literals
        case '\'':
        case '"':
            current_--; // Backtrack, let scanString handle it
            scanString();
            break;

        // Whitespace
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            current_--; // Backtrack, let scanWhitespace handle it
            if (includeWhitespace) {
                scanWhitespace();
            } else {
                // Skip whitespace, but update position
                advance();
            }
            break;

        default:
            if (TokenUtils::isDigit(c)) {
                current_--; // Backtrack, let scanNumber handle it
                scanNumber();
            } else if (TokenUtils::isIdentifierStart(c)) {
                current_--; // Backtrack, let scanIdentifier handle it
                scanIdentifier();
            } else {
                error("Unexpected character: '" + std::string(1, c) + "'");
            }
            break;
    }
}

char Lexer::advance() {
    if (isAtEnd())
        return '\0';

    char c = source_[current_++];
    updatePosition(c);
    return c;
}

char Lexer::peek() const {
    if (isAtEnd())
        return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length())
        return '\0';
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd())
        return false;
    if (source_[current_] != expected)
        return false;

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
    char quote = advance(); // Get quote type
    std::string value;

    while (!isAtEnd() && peek() != quote) {
        char c = advance();
        if (c == '\\' && !isAtEnd()) {
            // Handle escape characters
            char escaped = advance();
            switch (escaped) {
                case 'n':
                    value += '\n';
                    break;
                case 't':
                    value += '\t';
                    break;
                case 'r':
                    value += '\r';
                    break;
                case '\\':
                    value += '\\';
                    break;
                case '\'':
                    value += '\'';
                    break;
                case '"':
                    value += '"';
                    break;
                default:
                    value += escaped; // Add other characters directly
                    break;
            }
        } else {
            value += c;
        }
    }

    if (isAtEnd()) {
        error("Unterminated string literal");
    }

    // Consume closing quote
    advance();

    addToken(TokenType::STRING_LITERAL, value);
}

void Lexer::scanNumber() {
    std::string value;

    // Scan digit characters
    while (!isAtEnd() && TokenUtils::isDigit(peek())) {
        value += advance();
    }

    // Convert to integer
    try {
        int intValue = std::stoi(value);
        addToken(TokenType::INTEGER, intValue);
    } catch (const std::exception&) {
        error("Invalid number: " + value);
    }
}

void Lexer::scanIdentifier() {
    std::string value;

    // First character must be letter or underscore
    if (!TokenUtils::isIdentifierStart(peek())) {
        error("Invalid identifier start");
    }

    // Scan identifier characters
    while (!isAtEnd() && TokenUtils::isIdentifierChar(peek())) {
        value += advance();
    }

    // Check if it's a keyword
    TokenType type = TokenUtils::stringToTokenType(value);

    if (type == TokenType::IDENTIFIER) {
        addToken(TokenType::IDENTIFIER, value);
    } else {
        addToken(type); // Keyword
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
        if (i > 0)
            oss << " ";
        oss << tokens_[i].type;

        if (tokens_[i].hasStringValue()) {
            oss << "(\"" << tokens_[i].getStringValue() << "\")";
        } else if (tokens_[i].hasIntValue()) {
            oss << "(" << tokens_[i].getIntValue() << ")";
        }
    }
    return oss.str();
}

// LexerUtils implementation
bool LexerUtils::validateTokenSequence(const std::vector<Token>& tokens) {
    if (tokens.empty())
        return false;

    // Basic validation: last should be EOF
    if (tokens.back().type != TokenType::END_OF_FILE) {
        return false;
    }

    // Check parentheses matching
    int parenCount = 0;
    for (const auto& token : tokens) {
        if (token.type == TokenType::LEFT_PAREN) {
            parenCount++;
        } else if (token.type == TokenType::RIGHT_PAREN) {
            parenCount--;
            if (parenCount < 0)
                return false; // More right parentheses than left
        }
    }

    return parenCount == 0; // Parentheses must match
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
            if (i > 0)
                oss << " ";
            oss << tokens[i].type;
        }
    }

    return oss.str();
}

bool LexerUtils::hasBasicSyntaxErrors(const std::vector<Token>& tokens) {
    // Basic syntax check
    if (tokens.empty())
        return true;

    // Check for unknown tokens
    for (const auto& token : tokens) {
        if (token.type == TokenType::UNKNOWN) {
            return true;
        }
    }

    // Check parentheses matching
    if (!validateTokenSequence(tokens)) {
        return true;
    }

    return false;
}

} // namespace tinydb::sql
