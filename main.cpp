
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <iomanip>

using namespace std;

enum class TokenType {
    Identifier, Number, Plus, Minus, Multiply, Divide, Modulo, Power,
    LeftParen, RightParen, End
};

struct Token {
    TokenType type;
    string lexeme;
    size_t position;
};

string tokenName(TokenType type) {
    switch (type) {
        case TokenType::Identifier: return "IDENTIFIER";
        case TokenType::Number: return "NUMBER";
        case TokenType::Plus: return "PLUS";
        case TokenType::Minus: return "MINUS";
        case TokenType::Multiply: return "MULTIPLY";
        case TokenType::Divide: return "DIVIDE";
        case TokenType::Modulo: return "MODULO";
        case TokenType::Power: return "POWER";
        case TokenType::LeftParen: return "LEFT_PAREN";
        case TokenType::RightParen: return "RIGHT_PAREN";
        case TokenType::End: return "END";
    }
    return "UNKNOWN";
}

class Lexer {
    string source;
    size_t pos = 0;

    char current() const {
        return pos < source.size() ? source[pos] : '\0';
    }

public:
    explicit Lexer(string input) : source(move(input)) {}

    vector<Token> tokenize() {
        vector<Token> tokens;

        while (pos < source.size()) {
            char c = current();

            if (isspace(static_cast<unsigned char>(c))) {
                ++pos;
                continue;
            }

            size_t start = pos;

            if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
                ++pos;
                while (isalnum(static_cast<unsigned char>(current())) ||
                       current() == '_') ++pos;

                tokens.push_back({TokenType::Identifier,
                                  source.substr(start, pos - start), start});
            } else if (isdigit(static_cast<unsigned char>(c)) ||
                       (c == '.' && pos + 1 < source.size() &&
                        isdigit(static_cast<unsigned char>(source[pos + 1])))) {
                bool dot = false;
                while (isdigit(static_cast<unsigned char>(current())) ||
                       (!dot && current() == '.')) {
                    if (current() == '.') dot = true;
                    ++pos;
                }
                tokens.push_back({TokenType::Number,
                                  source.substr(start, pos - start), start});
            } else {
                TokenType type;
                switch (c) {
                    case '+': type = TokenType::Plus; break;
                    case '-': type = TokenType::Minus; break;
                    case '*': type = TokenType::Multiply; break;
                    case '/': type = TokenType::Divide; break;
                    case '%': type = TokenType::Modulo; break;
                    case '^': type = TokenType::Power; break;
                    case '(': type = TokenType::LeftParen; break;
                    case ')': type = TokenType::RightParen; break;
                    default:
                        throw runtime_error("Lexical error at position " +
                            to_string(pos) + ": invalid character '" + c + "'");
                }
                tokens.push_back({type, string(1, c), pos});
                ++pos;
            }
        }

        tokens.push_back({TokenType::End, "END", source.size()});
        return tokens;
    }
};

struct ASTNode {
    string value;
    unique_ptr<ASTNode> left, right;

    explicit ASTNode(string v) : value(move(v)) {}

    ASTNode(string v, unique_ptr<ASTNode> l, unique_ptr<ASTNode> r)
        : value(move(v)), left(move(l)), right(move(r)) {}
};

class Parser {
    vector<Token> tokens;
    size_t index = 0;

    const Token& current() const { return tokens[index]; }

    bool match(TokenType type) {
        if (current().type == type) {
            ++index;
            return true;
        }
        return false;
    }

    void consume(TokenType type, const string& message) {
        if (!match(type))
            throw runtime_error("Syntax error at position " +
                to_string(current().position) + ": " + message);
    }

    unique_ptr<ASTNode> parseExpression() {
        auto node = parseTerm();

        while (current().type == TokenType::Plus ||
               current().type == TokenType::Minus) {
            string op = current().lexeme;
            ++index;
            node = make_unique<ASTNode>(op, move(node), parseTerm());
        }
        return node;
    }

    unique_ptr<ASTNode> parseTerm() {
        auto node = parseUnary();

        while (current().type == TokenType::Multiply ||
               current().type == TokenType::Divide ||
               current().type == TokenType::Modulo) {
            string op = current().lexeme;
            ++index;
            node = make_unique<ASTNode>(op, move(node), parseUnary());
        }
        return node;
    }

    unique_ptr<ASTNode> parseUnary() {
        if (current().type == TokenType::Plus ||
            current().type == TokenType::Minus) {
            string op = "unary" + current().lexeme;
            ++index;
            return make_unique<ASTNode>(op, parseUnary(), nullptr);
        }
        return parsePower();
    }

    unique_ptr<ASTNode> parsePower() {
        auto node = parsePrimary();

        if (match(TokenType::Power))
            node = make_unique<ASTNode>("^", move(node), parseUnary());

        return node;
    }

    unique_ptr<ASTNode> parsePrimary() {
        if (current().type == TokenType::Identifier ||
            current().type == TokenType::Number) {
            string value = current().lexeme;
            ++index;
            return make_unique<ASTNode>(value);
        }

        if (match(TokenType::LeftParen)) {
            auto node = parseExpression();
            consume(TokenType::RightParen, "expected ')'");
            return node;
        }

        throw runtime_error("Syntax error at position " +
            to_string(current().position) +
            ": expected identifier, number, or '('");
    }

public:
    explicit Parser(vector<Token> input) : tokens(move(input)) {}

    unique_ptr<ASTNode> parse() {
        if (current().type == TokenType::End)
            throw runtime_error("Expression cannot be empty");

        auto root = parseExpression();

        if (current().type != TokenType::End)
            throw runtime_error("Unexpected token: " + current().lexeme);

        return root;
    }
};

void printAST(const ASTNode* node, string prefix = "", bool left = true) {
    if (!node) return;

    cout << prefix;
    if (!prefix.empty()) cout << (left ? "|-- " : "`-- ");

    if (node->value == "unary-") cout << "UNARY_MINUS\n";
    else if (node->value == "unary+") cout << "UNARY_PLUS\n";
    else cout << node->value << '\n';

    string childPrefix = prefix +
        (prefix.empty() ? "" : (left ? "|   " : "    "));

    if (node->left || node->right) {
        printAST(node->left.get(), childPrefix, true);
        printAST(node->right.get(), childPrefix, false);
    }
}

int main() {
    cout << "===============================================\n";
    cout << " Arithmetic Specification Language Analyzer\n";
    cout << " Lexical Analyzer + Syntax Analyzer (C++)\n";
    cout << "===============================================\n";
    cout << "Operators: + - * / % ^ and parentheses\n";
    cout << "Type exit to quit.\n";

    string input;

    while (true) {
        cout << "\nExpression> ";
        if (!getline(cin, input)) break;
        if (input == "exit" || input == "EXIT") break;

        try {
            Lexer lexer(input);
            auto tokens = lexer.tokenize();

            cout << "\nLEXICAL ANALYSIS\n";
            cout << left << setw(16) << "TOKEN TYPE" << "LEXEME\n";
            cout << string(30, '-') << '\n';

            for (const auto& token : tokens) {
                if (token.type != TokenType::End)
                    cout << left << setw(16) << tokenName(token.type)
                         << token.lexeme << '\n';
            }

            Parser parser(tokens);
            auto tree = parser.parse();

            cout << "\nSYNTAX ANALYSIS\n";
            cout << "Result: VALID EXPRESSION\n";
            cout << "\nABSTRACT SYNTAX TREE\n";
            printAST(tree.get());
        } catch (const exception& e) {
            cout << "\nANALYSIS FAILED\n" << e.what() << '\n';
        }
    }

    cout << "\nProgram terminated.\n";
    return 0;
}
