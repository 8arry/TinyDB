#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <unordered_set>

namespace tinydb::sql {

// Token类型枚举
enum class TokenType {
    // 关键字
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
    
    // 数据类型关键字 (符合项目要求)
    INT,        // int
    STR,        // str
    
    // 标识符和字面量
    IDENTIFIER,     // 表名、列名等
    INTEGER,        // 整数字面量
    STRING_LITERAL, // 字符串字面量
    
    // 操作符
    EQUAL,          // =
    NOT_EQUAL,      // !=
    LESS_THAN,      // <
    GREATER_THAN,   // >
    LESS_EQUAL,     // <=
    GREATER_EQUAL,  // >=
    ASTERISK,       // * (用于SELECT *和乘法)
    SLASH,          // / (除法)
    
    // 分隔符
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    COMMA,          // ,
    SEMICOLON,      // ;
    
    // 特殊标记
    WHITESPACE,     // 空白符
    END_OF_FILE,    // 文件结束
    UNKNOWN         // 未知字符
};

// Token值类型 - 可以是字符串、整数或空
using TokenValue = std::variant<std::monostate, std::string, int>;

// Token结构
struct Token {
    TokenType type;
    TokenValue value;
    size_t position;    // 在源文本中的位置
    size_t line;        // 行号
    size_t column;      // 列号
    
    // 构造函数
    Token() : type(TokenType::UNKNOWN), value(std::monostate{}), position(0), line(1), column(1) {}
    
    Token(TokenType t, size_t pos = 0, size_t ln = 1, size_t col = 1)
        : type(t), value(std::monostate{}), position(pos), line(ln), column(col) {}
    
    Token(TokenType t, std::string val, size_t pos = 0, size_t ln = 1, size_t col = 1)
        : type(t), value(std::move(val)), position(pos), line(ln), column(col) {}
    
    Token(TokenType t, int val, size_t pos = 0, size_t ln = 1, size_t col = 1)
        : type(t), value(val), position(pos), line(ln), column(col) {}
    
    // 辅助方法
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
    
    // 比较操作
    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }
    
    bool operator!=(const Token& other) const {
        return !(*this == other);
    }
};

// Token类型工具函数
class TokenUtils {
public:
    // 关键字映射
    static const std::unordered_set<std::string>& getKeywords() {
        static const std::unordered_set<std::string> keywords = {
            "CREATE", "TABLE", "INSERT", "INTO", "VALUES",
            "SELECT", "FROM", "WHERE", "UPDATE", "SET", "DELETE",
            "AND", "OR", "NOT", "INT", "STRING"
        };
        return keywords;
    }
    
    // 检查是否为关键字
    static bool isKeyword(const std::string& word) {
        return getKeywords().contains(word);
    }
    
    // 字符串转TokenType
    static TokenType stringToTokenType(const std::string& word) {
        std::string upperWord = toUpperCase(word);
        
        if (upperWord == "CREATE") return TokenType::CREATE;
        if (upperWord == "TABLE") return TokenType::TABLE;
        if (upperWord == "INSERT") return TokenType::INSERT;
        if (upperWord == "INTO") return TokenType::INTO;
        if (upperWord == "VALUES") return TokenType::VALUES;
        if (upperWord == "SELECT") return TokenType::SELECT;
        if (upperWord == "FROM") return TokenType::FROM;
        if (upperWord == "WHERE") return TokenType::WHERE;
        if (upperWord == "UPDATE") return TokenType::UPDATE;
        if (upperWord == "SET") return TokenType::SET;
        if (upperWord == "DELETE") return TokenType::DELETE;
        if (upperWord == "AND") return TokenType::AND;
        if (upperWord == "OR") return TokenType::OR;
        if (upperWord == "NOT") return TokenType::NOT;
        if (upperWord == "INT") return TokenType::INT;
        if (upperWord == "STR") return TokenType::STR;
        
        return TokenType::IDENTIFIER; // 不是关键字则为标识符
    }
    
    // TokenType转字符串
    static std::string tokenTypeToString(TokenType type) {
        switch (type) {
            case TokenType::CREATE: return "CREATE";
            case TokenType::TABLE: return "TABLE";
            case TokenType::INSERT: return "INSERT";
            case TokenType::INTO: return "INTO";
            case TokenType::VALUES: return "VALUES";
            case TokenType::SELECT: return "SELECT";
            case TokenType::FROM: return "FROM";
            case TokenType::WHERE: return "WHERE";
            case TokenType::UPDATE: return "UPDATE";
            case TokenType::SET: return "SET";
            case TokenType::DELETE: return "DELETE";
            case TokenType::AND: return "AND";
            case TokenType::OR: return "OR";
            case TokenType::NOT: return "NOT";
            case TokenType::INT: return "INT";
            case TokenType::STR: return "STR";
            case TokenType::IDENTIFIER: return "IDENTIFIER";
            case TokenType::INTEGER: return "INTEGER";
            case TokenType::STRING_LITERAL: return "STRING_LITERAL";
            case TokenType::EQUAL: return "=";
            case TokenType::NOT_EQUAL: return "!=";
            case TokenType::LESS_THAN: return "<";
            case TokenType::GREATER_THAN: return ">";
            case TokenType::LESS_EQUAL: return "<=";
            case TokenType::GREATER_EQUAL: return ">=";
            case TokenType::ASTERISK: return "*";
            case TokenType::SLASH: return "/";
            case TokenType::LEFT_PAREN: return "(";
            case TokenType::RIGHT_PAREN: return ")";
            case TokenType::COMMA: return ",";
            case TokenType::SEMICOLON: return ";";
            case TokenType::WHITESPACE: return "WHITESPACE";
            case TokenType::END_OF_FILE: return "EOF";
            case TokenType::UNKNOWN: return "UNKNOWN";
            default: return "INVALID";
        }
    }
    
    // 字符串转大写
    static std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return result;
    }
    
    // 检查字符类型
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

// Token输出流操作符
std::ostream& operator<<(std::ostream& os, const Token& token);
std::ostream& operator<<(std::ostream& os, TokenType type);

} // namespace tinydb::sql
