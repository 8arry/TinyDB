#include "token.hpp"
#include <iostream>
#include <cctype>

namespace tinydb::sql {

// Token stream output operator implementation
std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "Token{" << token.type;
    
    if (token.hasStringValue()) {
        os << ", \"" << token.getStringValue() << "\"";
    } else if (token.hasIntValue()) {
        os << ", " << token.getIntValue();
    }
    
    os << ", pos=" << token.position 
       << ", line=" << token.line 
       << ", col=" << token.column << "}";
    
    return os;
}

std::ostream& operator<<(std::ostream& os, TokenType type) {
    os << TokenUtils::tokenTypeToString(type);
    return os;
}

} // namespace tinydb::sql
