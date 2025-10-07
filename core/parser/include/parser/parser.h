#ifndef PARSER_CORE_PARSER_HEADER
#define PARSER_CORE_PARSER_HEADER

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace core {
    class ParserBase {
    public:
        struct DefaultTraits {
        public:
            using Integer = int64_t;
            using Real    = double;

        public:
            static Integer StringToInteger(const std::string& s);
            static Integer StringToInteger(const std::string& s, size_t begin, size_t end);

            static Real StringToReal(const std::string& s);
            static Real StringToReal(const std::string& s, size_t begin, size_t end);
        };

    public:
        class Token {
        public:
            enum Type : uint64_t {
                SYMBOL = 0x1,
                STRING = 0x2,

                NUMBER   = 0x4,
                OPERATOR = 0x8,
                FUNCTION = 0x10,

                UNARY    = 0x20,
                BINARY   = 0x40,
                INTEGER  = 0x80
            };

            enum ID : uint64_t {
                SQRT = 1,

                ID_BITSHIFT = 8
            };

        public:
            // Alternative of "void"
            class Data {
            public:
                virtual ~Data() = default;
            };

            template <typename T>
            class SpecifiedData : public Data {
            public:
                SpecifiedData() = default;
                SpecifiedData(const T& value) : value(value) {}
                SpecifiedData(T&& value) : value(std::move(value)) {}

                virtual ~SpecifiedData() = default;

            public:
                T value;
            };

        public:
            Token() = default;
            Token(Token&& other) : info(other.info), data(std::move(other.data)) {
                other.info = 0;
            }

            Token(uint64_t info) : info(info) {}
            Token(uint64_t info, std::unique_ptr<Data>&& data) :
                info(info), data(std::move(data)) {}

        public:
            uint64_t              info = 0;
            std::unique_ptr<Data> data;
        };

    public:
        static uint64_t CreateFunctionTokenInfo(Token::ID id) noexcept;

    protected:
        struct _FunctionDetails {
        public:
            _FunctionDetails() = default;
            _FunctionDetails(const _FunctionDetails& other) : info(other.info) {}

            _FunctionDetails(_FunctionDetails&& other) : info(other.info) {
                other.info = 0;
            }

            _FunctionDetails(size_t argsCount, uint64_t info) : info(info) {}

        public:
            uint64_t info = 0;
        };

        static std::map<std::string, _FunctionDetails> s_FunctionMap;
    };

    template <typename Traits = ParserBase::DefaultTraits>
    class Parser : public ParserBase {
    public:
        using Integer = typename Traits::Integer;
        using Real    = typename Traits::Real;

    public:
        Parser() = default;

    public:
        std::vector<Token> Tokenize(const char* expression) const {
            size_t i = 0;

            std::vector<Token> result;

            while (expression[i] != '\0') {
                if (isspace(expression[i])) {
                    ++i;
                    continue;
                }

                if (isdigit(expression[i])) { // number
                    Token numToken = _ParseNumber(expression, i);
                    if (numToken.info == 0) {
                        result.clear();
                        return result;
                    }
                    result.emplace_back(std::move(numToken));
                    continue;
                }

                if (isalpha(expression[i])) {
                    Token idToken = _ParseID(expression, i);
                    if (idToken.info == 0) {
                        result.clear();
                        return result;
                    }
                    result.emplace_back(std::move(idToken));
                    continue;
                }

                ++i;
            }

            return result;
        };

    private:
        Token _ParseNumber(const char* e, size_t& i) const {
            size_t   left = i;
            uint64_t info = Token::INTEGER | Token::NUMBER;

            while (e[i] != '\0' && isdigit(e[i])) ++i;
            if (e[i] != '\0' && e[i] == '.') {
                info &= ~Token::INTEGER;
                ++i;
            }

            while (e[i] != '\0' && isdigit(e[i])) ++i;
            if (e[i] != '\0' && e[i] == '.') {
                return Token();
            }

            std::string numberString(e + left, e + i);

            if (info & Token::INTEGER) {
                Integer number = Traits::StringToInteger(numberString);
                return Token(info, std::unique_ptr<Token::Data>(
                    std::make_unique<Token::SpecifiedData<Integer>>(number).release()
                ));
            }

            Real number = Traits::StringToReal(numberString);
            return Token(info, std::unique_ptr<Token::Data>(
                std::make_unique<Token::SpecifiedData<Real>>(number).release()
            ));
        }

        Token _ParseID(const char* e, size_t& i) const {
            size_t   left = i;
            uint64_t info = Token::SYMBOL;

            while (e[i] != '\0' && isalpha(e[i])) ++i;

            std::string idString(e + left, e + i);

            auto constIt = m_constantMap.find(idString);
            if (constIt != m_constantMap.cend()) {
                return Token(
                    info | Token::NUMBER,
                    std::unique_ptr<Token::Data>(
                        std::make_unique<Token::SpecifiedData<Real>>(constIt->second).release()
                    )
                );
            }

            auto funcIt = s_FunctionMap.find(idString);
            if (funcIt != s_FunctionMap.cend()) {
                const _FunctionDetails& funcDetails = funcIt->second;
                return Token(funcDetails.info, std::unique_ptr<Token::Data>(
                    std::make_unique<Token::SpecifiedData<_FunctionDetails>>(funcDetails).release()
                ));
            }

            return Token();
        }

    private:
        uint64_t m_flags = 0;

        std::map<std::string, Real> m_constantMap = {
            { "e",  Real(2.718281828459045) },
            { "pi", Real(3.141592653589793) }
        };
    };
}

#endif // !PARSER_CORE_PARSER_HEADER