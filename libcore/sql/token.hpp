#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>

namespace tinydb::sql {

// Token type enumeration
enum class TokenType {
    // Keywords
    CREATE,
    TABLE,
    INSERT,
    INTO,
    VALUES,
    SELECT,
    FROM,
    WHERE,
    UPDATE,
    SET,
    DELETE,
    AND,
    OR,
    NOT,
    INNER, // INNER (for INNER JOIN)
    JOIN,  // JOIN
    ON,    // ON (for JOIN condition)

    // Data type keywords (meets project requirements)
    INT, // int
    STR, // str

    // Identifiers and literals
    IDENTIFIER,     // Table names, column names, etc.
    INTEGER,        // Integer literals
    STRING_LITERAL, // String literals

    // Operators
    EQUAL,         // =
    NOT_EQUAL,     // !=
    LESS_THAN,     // <
    GREATER_THAN,  // >
    LESS_EQUAL,    // <=
    GREATER_EQUAL, // >=
    ASTERISK,      // * (for SELECT * and multiplication)
    SLASH,         // / (division)

    // Separators
    LEFT_PAREN,  // (
    RIGHT_PAREN, // )
    COMMA,       // ,
    SEMICOLON,   // ;
    DOT,         // . (for table.column)

    // Special tokens
    WHITESPACE,  // Whitespace
    END_OF_FILE, // End of file
    UNKNOWN      // Unknown character
};

// Token value type - can be string, integer or empty
using TokenValue = std::variant<std::monostate, std::string, int>;

// Token structure
struct Token {
    TokenType type;
    TokenValue value;
    size_t position; // Position in source text
    size_t line;     // Line number
    size_t column;   // Column number

    // Constructors
    Token() : type(TokenType::UNKNOWN), value(std::monostate{}), position(0), line(1), column(1) {
    }

    Token(TokenType t, size_t pos = 0, size_t ln = 1, size_t col = 1)
        : type(t), value(std::monostate{}), position(pos), line(ln), column(col) {
    }

    Token(TokenType t, std::string val, size_t pos = 0, size_t ln = 1, size_t col = 1)
        : type(t), value(std::move(val)), position(pos), line(ln), column(col) {
    }

    Token(TokenType t, int val, size_t pos = 0, size_t ln = 1, size_t col = 1)
        : type(t), value(val), position(pos), line(ln), column(col) {
    }

    // Helper methods
    bool hasStringValue() const {
        return std::holds_alternative<std::string>(value);
    }

    bool hasIntValue() const {
        return std::holds_alternative<int>(value);
    }

    bool hasNoValue() const {
        return std::holds_alternative<std::monostate>(value);
    }

    const std::string& getStringValue() const {
        return std::get<std::string>(value);
    }

    int getIntValue() const {
        return std::get<int>(value);
    }

    // Comparison operations
    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }

    bool operator!=(const Token& other) const {
        return !(*this == other);
    }
};

// Token type utility functions
class TokenUtils {
public:
    // Keywords mapping
    static const std::unordered_set<std::string>& getKeywords() {
        static const std::unordered_set<std::string> keywords = {
            "CREATE", "TABLE",  "INSERT", "INTO",   "VALUES", "SELECT", "FROM",
            "WHERE",  "UPDATE", "SET",    "DELETE", "AND",    "OR",     "NOT",
            "INNER",  "JOIN",   "ON",     "INT",    "STR"};
        return keywords;
    }

    // Check if string is a keyword
    static bool isKeyword(const std::string& word) {
        return getKeywords().contains(word);
    }

    // Convert string to TokenType
    static TokenType stringToTokenType(const std::string& word) {
        std::string upperWord = toUpperCase(word);

        if (upperWord == "CREATE")
            return TokenType::CREATE;
        if (upperWord == "TABLE")
            return TokenType::TABLE;
        if (upperWord == "INSERT")
            return TokenType::INSERT;
        if (upperWord == "INTO")
            return TokenType::INTO;
        if (upperWord == "VALUES")
            return TokenType::VALUES;
        if (upperWord == "SELECT")
            return TokenType::SELECT;
        if (upperWord == "FROM")
            return TokenType::FROM;
        if (upperWord == "WHERE")
            return TokenType::WHERE;
        if (upperWord == "UPDATE")
            return TokenType::UPDATE;
        if (upperWord == "SET")
            return TokenType::SET;
        if (upperWord == "DELETE")
            return TokenType::DELETE;
        if (upperWord == "AND")
            return TokenType::AND;
        if (upperWord == "OR")
            return TokenType::OR;
        if (upperWord == "NOT")
            return TokenType::NOT;
        if (upperWord == "INNER")
            return TokenType::INNER;
        if (upperWord == "JOIN")
            return TokenType::JOIN;
        if (upperWord == "ON")
            return TokenType::ON;
        if (upperWord == "INT")
            return TokenType::INT;
        if (upperWord == "STR")
            return TokenType::STR;

        return TokenType::IDENTIFIER; // If not a keyword, then it's an identifier
    }

    // Convert TokenType to string
    static std::string tokenTypeToString(TokenType type) {
        switch (type) {
            case TokenType::CREATE:
                return "CREATE";
            case TokenType::TABLE:
                return "TABLE";
            case TokenType::INSERT:
                return "INSERT";
            case TokenType::INTO:
                return "INTO";
            case TokenType::VALUES:
                return "VALUES";
            case TokenType::SELECT:
                return "SELECT";
            case TokenType::FROM:
                return "FROM";
            case TokenType::WHERE:
                return "WHERE";
            case TokenType::UPDATE:
                return "UPDATE";
            case TokenType::SET:
                return "SET";
            case TokenType::DELETE:
                return "DELETE";
            case TokenType::AND:
                return "AND";
            case TokenType::OR:
                return "OR";
            case TokenType::NOT:
                return "NOT";
            case TokenType::INNER:
                return "INNER";
            case TokenType::JOIN:
                return "JOIN";
            case TokenType::ON:
                return "ON";
            case TokenType::INT:
                return "INT";
            case TokenType::STR:
                return "STR";
            case TokenType::IDENTIFIER:
                return "IDENTIFIER";
            case TokenType::INTEGER:
                return "INTEGER";
            case TokenType::STRING_LITERAL:
                return "STRING_LITERAL";
            case TokenType::EQUAL:
                return "=";
            case TokenType::NOT_EQUAL:
                return "!=";
            case TokenType::LESS_THAN:
                return "<";
            case TokenType::GREATER_THAN:
                return ">";
            case TokenType::LESS_EQUAL:
                return "<=";
            case TokenType::GREATER_EQUAL:
                return ">=";
            case TokenType::ASTERISK:
                return "*";
            case TokenType::SLASH:
                return "/";
            case TokenType::LEFT_PAREN:
                return "(";
            case TokenType::RIGHT_PAREN:
                return ")";
            case TokenType::COMMA:
                return ",";
            case TokenType::SEMICOLON:
                return ";";
            case TokenType::DOT:
                return ".";
            case TokenType::WHITESPACE:
                return "WHITESPACE";
            case TokenType::END_OF_FILE:
                return "EOF";
            case TokenType::UNKNOWN:
                return "UNKNOWN";
            default:
                return "INVALID";
        }
    }

    // Convert string to uppercase
    static std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return result;
    }

    // Check character type
    static bool isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    static bool isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }

    static bool isWhitespace(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static bool isIdentifierStart(char c) {
        return isAlpha(c) || c == '_';
    }

    static bool isIdentifierChar(char c) {
        return isAlphaNumeric(c) || c == '_';
    }
};

// Token output stream operator
std::ostream& operator<<(std::ostream& os, const Token& token);
std::ostream& operator<<(std::ostream& os, TokenType type);

} // namespace tinydb::sql
