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

uint64_t core::ParserBase::CreateFunctionTokenInfo(Token::ID id) noexcept {
    return Token::FUNCTION | Token::SYMBOL | (id << Token::ID_BITSHIFT);
}

std::map<std::string, core::ParserBase::_FunctionDetails> core::ParserBase::s_FunctionMap = {
    { "sqrt", core::ParserBase::_FunctionDetails{ 1,
        core::ParserBase::CreateFunctionTokenInfo(core::ParserBase::Token::SQRT) } }
};