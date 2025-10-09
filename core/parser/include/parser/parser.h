#ifndef PARSER_CORE_PARSER_HEADER
#define PARSER_CORE_PARSER_HEADER

#include <cstdint>
#include <memory>

#include <string>
#include <vector>
#include <map>

#include <iostream>

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

        enum class ExpressionError {
            IS_VALID,
            
            TWO_CONSECUTIVE_NUMBERS,
            INVALID_OPERATOR_POSITION,
            INVALID_PARENTHESES
        };

    public:
        class Token {
        public:
            enum Type : uint64_t {
                SYMBOL = 0x1,
                STRING = 0x2,

                NUMBER   = 0x4,
                CONSTANT = 0x8,
                OPERATOR = 0x10,
                FUNCTION = 0x20,

                UNARY    = 0x40,
                BINARY   = 0x80,
                INTEGER  = 0x100,

                RIGHT_TO_LEFT = 0x200
            };

            enum ID : uint64_t {
                // End of Expression
                EOEX = 1,

                PLUS,
                MINUS,
                ASTERISK,
                SLASH,

                OPEN_PAREN,
                CLOSE_PAREN,

                SQRT,

                ID_BITSHIFT = 10,
                ID_BITMASK  = (1ull << 16) - 1,
                
                BINDING_POWER_BITS = 8,
                BINDING_POWER_BITMASK  = (1ull << BINDING_POWER_BITS) - 1,
                BINDING_POWER_BITSHIFT = ID_BITSHIFT + (BINDING_POWER_BITS << 1),
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
            bool HasType(uint64_t type) const noexcept;
            bool Is(ID id) const noexcept;

            // Get Binding Power
            uint8_t GetBP() const noexcept;

            // Binding Power for alternative version of the same token
            uint8_t GetBP2() const noexcept;

            ID GetID() const noexcept;

        public:
            void SetBP(uint8_t bp) noexcept;

        public:
            uint64_t              info = 0;
            std::unique_ptr<Data> data;
        };

    public:
        static uint64_t CreateFunctionTokenInfo(Token::ID id, uint8_t bp = 0, uint8_t bp2 = 0) noexcept;
        static uint64_t CreateOperatorTokenInfo(Token::ID id, uint8_t bp = 0, uint8_t bp2 = 0) noexcept;

    protected:
        struct _FunctionDetails {
        public:
            _FunctionDetails() = default;
            _FunctionDetails(uint64_t info) : info(info) {}

        public:
            uint64_t info = 0;
        };

        static std::map<std::string, _FunctionDetails> s_FunctionMap;
        static std::map<char, _FunctionDetails>        s_OperatorMap;
        static std::map<char, uint64_t>                s_SupportedSymbolMap;
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

                if (isalpha(expression[i])) { // constant, function
                    Token idToken = _ParseID(expression, i);
                    if (idToken.info == 0) {
                        result.clear();
                        return result;
                    }
                    result.emplace_back(std::move(idToken));
                    continue;
                }

                auto opIt = s_OperatorMap.find(expression[i]);
                if (opIt != s_OperatorMap.cend()) { // operator
                    const _FunctionDetails& opDetails = opIt->second;
                    Token opToken(
                        opDetails.info,
                        std::unique_ptr<Token::Data>(
                            std::make_unique<Token::SpecifiedData<_FunctionDetails>>(opDetails).release()
                        )
                    );
                    result.emplace_back(std::move(opToken));
                    ++i;
                    continue;
                }

                auto symbolIt = s_SupportedSymbolMap.find(expression[i]);
                if (symbolIt != s_SupportedSymbolMap.cend()) {
                    result.emplace_back(symbolIt->second);
                    ++i;
                    continue;
                }

                result.clear();
                break;
            }
            
            result.emplace_back(Token::EOEX);
            return result;
        };

        // Specify operators (some operators depend on context) and add implicit tokens
        // Return list of implicit tokens
        std::vector<std::pair<size_t, Token>> Specify(std::vector<Token>& tokens) const {
            Token emptyToken(0);
            Token* prevToken = &emptyToken;
            const _FunctionDetails& multiplicationDetails = s_OperatorMap['*'];

            std::vector<std::pair<size_t, Token>> implicitTokens;
            
            size_t tokenCount = tokens.size();
            for (size_t i = 0; i < tokenCount; ++i) {
                Token& token = tokens[i];

                bool isLeftOperand = prevToken->HasType(Token::NUMBER) || prevToken->Is(Token::CLOSE_PAREN);

                // Binary/Unary operator
                if (token.HasType(Token::BINARY | Token::UNARY)) {
                    // Binary, if has left operand, otherwise unary
                    token.info &= ~(isLeftOperand ? Token::UNARY : Token::BINARY);
                    if (!isLeftOperand) {
                        token.SetBP(token.GetBP2());
                    }
                }
                // Implicit multiplication first case (operand before '(')
                else if (token.Is(Token::OPEN_PAREN) && isLeftOperand) {
                    implicitTokens.emplace_back(i, Token(multiplicationDetails.info,
                        std::unique_ptr<Token::Data>(
                            std::make_unique<Token::SpecifiedData<_FunctionDetails>>(multiplicationDetails).release()
                        )
                    ));
                }
                // Implicit multiplication second case (number before constant, and vice versa)
                else if (token.HasType(Token::NUMBER) && isLeftOperand) {
                    bool condition = (token.HasType(Token::CONSTANT) && !prevToken->HasType(Token::CONSTANT)) ||
                        (!token.HasType(Token::CONSTANT) && (prevToken->HasType(Token::CONSTANT) ||
                        !prevToken->HasType(Token::NUMBER)));

                    // Non-constant before constant or non-number before number
                    if (condition) {
                        implicitTokens.emplace_back(i, Token(multiplicationDetails.info,
                            std::unique_ptr<Token::Data>(
                                std::make_unique<Token::SpecifiedData<_FunctionDetails>>(multiplicationDetails).release()
                            )
                        ));
                    }
                }
                // Implicit multiplication third case (operand before function)
                else if (token.HasType(Token::FUNCTION) && isLeftOperand) {
                    implicitTokens.emplace_back(i, Token(multiplicationDetails.info,
                        std::unique_ptr<Token::Data>(
                            std::make_unique<Token::SpecifiedData<_FunctionDetails>>(multiplicationDetails).release()
                        )
                    ));
                }

                prevToken = &token;
            }
            return implicitTokens;
        }

        // Check, if expression tokens are compatible
        ExpressionError Validate(const std::vector<Token>& tokens) const {
            Token emptyToken(0);
            const Token* prevToken = &emptyToken;

            size_t openParen = 0;
            
            size_t tokenCount = tokens.size();
            for (size_t i = 0; i < tokenCount; ++i) {
                const Token& token = tokens[i];

                if (token.Is(Token::OPEN_PAREN)) {
                    ++openParen;
                }
                else if (token.Is(Token::CLOSE_PAREN)) {
                    if (openParen == 0) return ExpressionError::INVALID_PARENTHESES;
                    --openParen;
                }

                // Number before number, or constant before constant
                if ((token.HasType(Token::CONSTANT) && prevToken->HasType(Token::CONSTANT)) ||
                    (token.HasType(Token::NUMBER) && !token.HasType(Token::CONSTANT) &&
                    prevToken->HasType(Token::NUMBER) && !prevToken->HasType(Token::CONSTANT))) {
                    return ExpressionError::TWO_CONSECUTIVE_NUMBERS;
                }
                
                // There isn't computable left side of binary operator
                if (token.HasType(Token::BINARY) &&
                    ((prevToken->HasType(Token::UNARY) && !prevToken->HasType(Token::RIGHT_TO_LEFT)) ||
                    prevToken->info == 0 || prevToken->HasType(Token::BINARY))) {
                    return ExpressionError::INVALID_OPERATOR_POSITION;
                }
                
                // There isn't computable left side of right-to-left unary operator
                if (token.HasType(Token::UNARY | Token::RIGHT_TO_LEFT) &&
                    !(prevToken->Is(Token::CLOSE_PAREN) || prevToken->HasType(Token::NUMBER))) {
                    return ExpressionError::INVALID_OPERATOR_POSITION;
                }

                prevToken = &token;
            }

            if (openParen > 0) return ExpressionError::INVALID_PARENTHESES;
            
            return ExpressionError::IS_VALID;
        }

        Real Evaluate(const char* expression) {
            std::vector<Token> tokens = Tokenize(expression);
            std::vector<std::pair<size_t, Token>> implicitTokens = Specify(tokens);

            if (Validate(tokens) != ExpressionError::IS_VALID) {
                // error;
                return 0.0;
            }
            m_builder.Reset(&tokens, &implicitTokens);

            std::unique_ptr<_ExprNode> root = m_builder.Build(0);
            return root->Evaluate();
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

            return Token(info, std::unique_ptr<Token::Data>(
                std::make_unique<Token::SpecifiedData<std::string>>(std::move(numberString)).release()
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
                    info | Token::NUMBER | Token::CONSTANT & ~(Token::SYMBOL),
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
        struct _ExprNode {
        public:
            virtual ~_ExprNode() = default;
            virtual Real Evaluate() const = 0;
        };

        template <typename AtomType>
        struct _AtomNode : _ExprNode {
        public:
            _AtomNode() = default;
            _AtomNode(const AtomType& value) : value(value) {}
            _AtomNode(AtomType&& value) : value(std::move(value)) {}

            virtual Real Evaluate() const override {
                return static_cast<Real>(value);
            }

        public:
            AtomType value;
        };

        struct _UnaryNode : _ExprNode {
        public:
            _UnaryNode() = default;
            _UnaryNode(Real(*func)(const Real&)) : func(func) {}
            _UnaryNode(Real(*func)(const Real&), std::unique_ptr<_ExprNode>&& arg) : 
                arg(std::move(arg)), func(func) {}

            virtual Real Evaluate() const override {
                return func(arg->Evaluate());
            };

        public:
            std::unique_ptr<_ExprNode> arg;
            Real(*func)(const Real&) = nullptr;
        };

        struct _BinaryNode : _ExprNode {
        public:
            _BinaryNode() = default;
            _BinaryNode(
                Real(*func)(const Real&, const Real&),
                std::unique_ptr<_ExprNode>&& left,
                std::unique_ptr<_ExprNode>&& right
            ) : left(std::move(left)), right(std::move(right)), func(func) {}
            
            virtual Real Evaluate() const override {
                return func(left->Evaluate(), right->Evaluate());
            };

        public:
            std::unique_ptr<_ExprNode> left;
            std::unique_ptr<_ExprNode> right;
            Real(*func)(const Real&, const Real&) = nullptr;
        };

    private:
        class _Builder {
        public:
            _Builder() = default;
            _Builder(
                const std::vector<Token>* tokens,
                const std::vector<std::pair<size_t, Token>>* implicitTokens
            ) : m_tokens(tokens), m_implicitTokens(implicitTokens) {}

        public:
            std::unique_ptr<_ExprNode> Build(uint8_t rbp) {
                const Token* token = _Advance();
                std::unique_ptr<_ExprNode> left = _Nud(token);
                
                token = _Advance();
                while (token && !token->Is(Token::EOEX) && rbp < token->GetBP()) {
                    left = _Led(token, std::move(left));
                    token = _Advance();
                }

                return std::move(left);

                return std::unique_ptr<_ExprNode>();
            };

            void Reset(
                const std::vector<Token>* tokens,
                const std::vector<std::pair<size_t, Token>>* implicitTokens
            ) {
                m_index = m_implicitIndex = 0;
                m_tokens = tokens;
                m_implicitTokens = implicitTokens;
            }

        private:
            const Token* _Peek() const {
                if (m_implicitIndex < m_implicitTokens->size() &&
                    (*m_implicitTokens)[m_implicitIndex].first == m_index + 1) {
                    return &((*m_implicitTokens)[m_implicitIndex].second);
                }
                if (m_index + 1 < m_tokens->size()) {
                    return &((*m_tokens)[m_index + 1]);
                }
                return nullptr;
            }

            const Token* _Advance() {
                if (m_implicitIndex < m_implicitTokens->size() &&
                    (*m_implicitTokens)[m_implicitIndex].first == m_index) {
                    return &((*m_implicitTokens)[m_implicitIndex++].second);
                }
                if (m_index < m_tokens->size()) {
                    return &((*m_tokens)[m_index++]);
                }
                return nullptr;
            }
            
            // Null denotation (begin of the subexpression)
            std::unique_ptr<_ExprNode> _Nud(const Token* token) {
                if (token->HasType(Token::CONSTANT)) {
                    return std::make_unique<_AtomNode<Real>>(
                        ((Token::SpecifiedData<Real>*)token->data.get())->value
                    );
                }
                if (token->HasType(Token::NUMBER)) {
                    const std::string& numberString = ((Token::SpecifiedData<std::string>*)token->data.get())->value;
                    if (token->HasType(Token::INTEGER)) {
                        return std::make_unique<_AtomNode<Integer>>(Traits::StringToInteger(numberString));
                    }
                    return std::make_unique<_AtomNode<Real>>(Traits::StringToReal(numberString));
                }
                if (token->Is(Token::OPEN_PAREN)) {
                    std::unique_ptr<_ExprNode> expr = Build(0);
                    const Token* next = _Peek();
                    if (next && next->Is(Token::CLOSE_PAREN)) {
                        _Advance();
                    }
                    return std::move(expr);
                }
                if (token->HasType(Token::UNARY)) {
                    std::unique_ptr<_ExprNode> expr = Build(token->GetBP());
                    return std::make_unique<_UnaryNode>(
                        m_unaryFunctionMap[token->GetID()],
                        std::move(expr)
                    );
                }
                return std::unique_ptr<_ExprNode>();
            }

            // Left denotation
            std::unique_ptr<_ExprNode> _Led(const Token* token, std::unique_ptr<_ExprNode>&& left) {
                if (token->HasType(Token::BINARY)) {
                    std::unique_ptr<_ExprNode> expr = Build(token->GetBP());
                    return std::make_unique<_BinaryNode>(
                        m_binaryFunctionMap[token->GetID()],
                        std::move(left),
                        std::move(expr)
                    );
                }
                return std::unique_ptr<_ExprNode>();
            };

        private:
            static Real _UnaryPlus(const Real& x) { return x; }
            static Real _UnaryMinus(const Real& x) { return -x; }

            static Real _Plus    (const Real& x, const Real& y) { return x + y; }
            static Real _Minus   (const Real& x, const Real& y) { return x - y; }
            static Real _Multiply(const Real& x, const Real& y) { return x * y; }
            static Real _Divide  (const Real& x, const Real& y) { return x / y; }

        private:
            size_t                                       m_index          = 0ull;
            size_t                                       m_implicitIndex  = 0ull;
            const std::vector<Token>*                    m_tokens         = nullptr;
            const std::vector<std::pair<size_t, Token>>* m_implicitTokens = nullptr;

            std::map<Token::ID, Real(*)(const Real&)> m_unaryFunctionMap = {
                { Token::PLUS, _UnaryPlus },
                { Token::MINUS, _UnaryMinus }
            };

            std::map<Token::ID, Real(*)(const Real&, const Real&)> m_binaryFunctionMap = {
                { Token::PLUS,     _Plus },
                { Token::MINUS,    _Minus },
                { Token::ASTERISK, _Multiply },
                { Token::SLASH,    _Divide }
            };
        };

    private:
        uint64_t m_flags = 0;

        std::map<std::string, Real> m_constantMap = {
            { "e",  Real(2.718281828459045) },
            { "pi", Real(3.141592653589793) }
        };

        _Builder m_builder;
    };
}

#endif // !PARSER_CORE_PARSER_HEADER