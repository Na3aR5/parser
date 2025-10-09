#include <parser/parser.h>

core::ParserBase::DefaultTraits::Integer core::ParserBase::DefaultTraits::StringToInteger(
const std::string& s) {
    return std::stol(s);
}

core::ParserBase::DefaultTraits::Integer core::ParserBase::DefaultTraits::StringToInteger(
const std::string& s, size_t begin, size_t end) {
    Integer result = 0;
    Integer m = 1;

    while (begin < end) {
        result *= m;
        result += (Integer)(s[begin] - '0');
        m *= 10;
        ++begin;
    }

    return result;
}

core::ParserBase::DefaultTraits::Real core::ParserBase::DefaultTraits::StringToReal(
const std::string& s) {
    return std::stod(s);
}

core::ParserBase::DefaultTraits::Real core::ParserBase::DefaultTraits::StringToReal(
const std::string& s, size_t begin, size_t end) {
    return std::stod(s.substr(begin, end - begin));
}

bool core::ParserBase::Token::HasType(uint64_t type) const noexcept {
    return (info & type) == type;
}

bool core::ParserBase::Token::Is(ID id) const noexcept {
    return ((info >> ID_BITSHIFT) & ID_BITMASK) == id;
}

uint8_t core::ParserBase::Token::GetBP() const noexcept {
    return (info >> BINDING_POWER_BITSHIFT) & BINDING_POWER_BITMASK;
}

uint8_t core::ParserBase::Token::GetBP2() const noexcept {
    return (info >> (BINDING_POWER_BITSHIFT + BINDING_POWER_BITS)) & BINDING_POWER_BITMASK;
}

core::ParserBase::Token::ID core::ParserBase::Token::GetID() const noexcept {
    return (ID)((info >> ID_BITSHIFT) & ID_BITMASK);
}

void core::ParserBase::Token::SetBP(uint8_t bp) noexcept {
    info &= ~(BINDING_POWER_BITMASK << BINDING_POWER_BITSHIFT);
    info |= (uint64_t)bp << BINDING_POWER_BITSHIFT;
}

uint64_t core::ParserBase::CreateFunctionTokenInfo(Token::ID id, uint8_t bp, uint8_t bp2) noexcept {
    return Token::FUNCTION | Token::SYMBOL | (id << Token::ID_BITSHIFT) |
        ((((uint64_t)bp2 << Token::BINDING_POWER_BITS) | bp) << Token::BINDING_POWER_BITSHIFT);
}

uint64_t core::ParserBase::CreateOperatorTokenInfo(Token::ID id, uint8_t bp, uint8_t bp2) noexcept {
    return Token::OPERATOR | Token::SYMBOL | (id << Token::ID_BITSHIFT) |
        ((((uint64_t)bp2 << Token::BINDING_POWER_BITS) | bp) << Token::BINDING_POWER_BITSHIFT);
}

std::map<std::string, core::ParserBase::_FunctionDetails> core::ParserBase::s_FunctionMap = {
    { "sqrt", core::ParserBase::_FunctionDetails(CreateFunctionTokenInfo(Token::SQRT, 30)) }
};

std::map<char, core::ParserBase::_FunctionDetails> core::ParserBase::s_OperatorMap = {
    { '+', core::ParserBase::_FunctionDetails(
        CreateOperatorTokenInfo(Token::PLUS, 10, 15) | Token::BINARY | Token::UNARY)
    },
    { '-', core::ParserBase::_FunctionDetails(
        CreateOperatorTokenInfo(Token::MINUS, 10, 15) | Token::BINARY | Token::UNARY)
    },
    { '*', core::ParserBase::_FunctionDetails(
        CreateOperatorTokenInfo(Token::ASTERISK, 20) | Token::BINARY)
    },
    { '/', core::ParserBase::_FunctionDetails(
        CreateOperatorTokenInfo(Token::SLASH, 20) | Token::BINARY)
    },
};

std::map<char, uint64_t> core::ParserBase::s_SupportedSymbolMap = {
    { '(', Token::SYMBOL | (Token::OPEN_PAREN  << Token::ID_BITSHIFT) },
    { ')', Token::SYMBOL | (Token::CLOSE_PAREN << Token::ID_BITSHIFT) }
};