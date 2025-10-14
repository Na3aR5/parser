#include <parser/parser.h>

#include <cmath>

core::ParserBase::DefaultTraits::DefaultTraits() :
sqrtFunction(std::sqrt),
powFunction(std::pow),
sinFunction(std::sin),
cosFunction(std::cos),
tanFunction(std::tan),
lnFunction(std::log)
{}

core::ParserBase::DefaultTraits::Integer core::ParserBase::DefaultTraits::StringToInteger(
const std::string& s) {
    return std::stol(s);
}

core::ParserBase::DefaultTraits::Real core::ParserBase::DefaultTraits::StringToReal(
const std::string& s) {
    return std::stod(s);
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

size_t core::ParserBase::Token::GetFunctionArgCount() const noexcept {
    return (info >> FUNCTION_ARGS_BITSHIFT) & FUNCTION_ARGS_BITMASK;
}

void core::ParserBase::Token::SetBP(uint8_t bp) noexcept {
    info &= ~(BINDING_POWER_BITMASK << BINDING_POWER_BITSHIFT);
    info |= (uint64_t)bp << BINDING_POWER_BITSHIFT;
}

void core::ParserBase::Token::SetFunctionArgCount(size_t count) noexcept {
    info &= ~(FUNCTION_ARGS_BITMASK << FUNCTION_ARGS_BITSHIFT);
    info |= count << FUNCTION_ARGS_BITSHIFT;
}

uint64_t core::ParserBase::CreateFunctionTokenInfo(Token::ID id, size_t argCount) noexcept {
    return Token::FUNCTION | Token::SYMBOL | (id << Token::ID_BITSHIFT) |
        ((30) << Token::BINDING_POWER_BITSHIFT) | (argCount << Token::FUNCTION_ARGS_BITSHIFT);
}

uint64_t core::ParserBase::CreateOperatorTokenInfo(Token::ID id, uint8_t bp, uint8_t bp2) noexcept {
    return Token::OPERATOR | Token::SYMBOL | (id << Token::ID_BITSHIFT) |
        ((((uint64_t)bp2 << Token::BINDING_POWER_BITS) | bp) << Token::BINDING_POWER_BITSHIFT);
}

std::map<std::string, uint64_t> core::ParserBase::s_FunctionMap = {
    { "sqrt", CreateFunctionTokenInfo(Token::SQRT, 1) },
    { "sin",  CreateFunctionTokenInfo(Token::SIN, 1) },
    { "cos",  CreateFunctionTokenInfo(Token::COS, 1) },
    { "tan",  CreateFunctionTokenInfo(Token::TAN, 1) },
    { "cot",  CreateFunctionTokenInfo(Token::COT, 1) },
    { "ln",   CreateFunctionTokenInfo(Token::LN,  1) },

    // 2 args
    { "pow", CreateFunctionTokenInfo(Token::POW, 2) },

    // any arg count
    { "avg", CreateFunctionTokenInfo(Token::AVG, 0) | Token::ANY_ARG_COUNT }
};

std::map<char, uint64_t> core::ParserBase::s_OperatorMap = {
    { '+', CreateOperatorTokenInfo(Token::PLUS,  10, 15) | Token::BINARY | Token::UNARY },
    { '-', CreateOperatorTokenInfo(Token::MINUS, 10, 15) | Token::BINARY | Token::UNARY },
    { '*', CreateOperatorTokenInfo(Token::ASTERISK, 20)  | Token::BINARY },
    { '/', CreateOperatorTokenInfo(Token::SLASH,    20)  | Token::BINARY },

    { '^', CreateOperatorTokenInfo(Token::CARET, 25)     | Token::BINARY }
};

std::map<char, uint64_t> core::ParserBase::s_SupportedSymbolMap = {
    { '(', Token::SYMBOL | (Token::OPEN_PAREN  << Token::ID_BITSHIFT) },
    { ')', Token::SYMBOL | (Token::CLOSE_PAREN << Token::ID_BITSHIFT) | Token::EOEX_LIKE },
    { ',', Token::SYMBOL | (Token::COMMA       << Token::ID_BITSHIFT) | Token::EOEX_LIKE }
};