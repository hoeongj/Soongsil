#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

struct SyntaxErrorInProgram : runtime_error {
    SyntaxErrorInProgram() : runtime_error("Syntax Error") {}
};

static const unordered_set<string> KEYWORDS = {
    "variable", "constant", "print",  "repeat", "begin",
    "end",      "until",    "select", "else",
};

static const size_t MAX_TOKEN_LEN = 10;

bool is_number(const string& token) {
    if (token.empty() || token.size() > MAX_TOKEN_LEN) {
        return false;
    }
    for (char ch : token) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }
    return true;
}

bool is_var_name(const string& token) {
    if (token.empty() || token.size() > MAX_TOKEN_LEN || KEYWORDS.count(token) != 0) {
        return false;
    }
    if (token[0] < 'a' || token[0] > 'z') {
        return false;
    }
    for (char ch : token) {
        if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))) {
            return false;
        }
    }
    return true;
}

using Env = unordered_map<string, long long>;

struct Expr {
    virtual ~Expr() = default;
    virtual long long eval(const Env& env) const = 0;
};

struct NumberExpr : Expr {
    long long value;

    explicit NumberExpr(long long value) : value(value) {}

    long long eval(const Env& env) const override {
        (void)env;
        return value;
    }
};

struct VarExpr : Expr {
    string name;

    explicit VarExpr(string name) : name(std::move(name)) {}

    long long eval(const Env& env) const override {
        return env.at(name);
    }
};

struct NegExpr : Expr {
    unique_ptr<Expr> expr;

    explicit NegExpr(unique_ptr<Expr> expr) : expr(std::move(expr)) {}

    long long eval(const Env& env) const override {
        return -expr->eval(env);
    }
};

struct BinExpr : Expr {
    string op;
    unique_ptr<Expr> left;
    unique_ptr<Expr> right;

    BinExpr(string op, unique_ptr<Expr> left, unique_ptr<Expr> right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

    long long eval(const Env& env) const override {
        long long lhs = left->eval(env);
        long long rhs = right->eval(env);
        if (op == "+") {
            return lhs + rhs;
        }
        if (op == "-") {
            return lhs - rhs;
        }
        return lhs * rhs;
    }
};

struct BoolExpr {
    string op;
    unique_ptr<Expr> left;
    unique_ptr<Expr> right;

    BoolExpr(string op, unique_ptr<Expr> left, unique_ptr<Expr> right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

    bool eval(const Env& env) const {
        long long lhs = left->eval(env);
        long long rhs = right->eval(env);
        if (op == "==") {
            return lhs == rhs;
        }
        if (op == "!=") {
            return lhs != rhs;
        }
        if (op == "<") {
            return lhs < rhs;
        }
        if (op == ">") {
            return lhs > rhs;
        }
        if (op == "<=") {
            return lhs <= rhs;
        }
        return lhs >= rhs;
    }
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual void execute(Env& env, vector<long long>& output) const = 0;
};

struct AssignStmt : Stmt {
    string name;
    unique_ptr<Expr> expr;

    AssignStmt(string name, unique_ptr<Expr> expr)
        : name(std::move(name)), expr(std::move(expr)) {}

    void execute(Env& env, vector<long long>& output) const override {
        (void)output;
        env[name] = expr->eval(env);
    }
};

struct PrintStmt : Stmt {
    unique_ptr<Expr> expr;

    explicit PrintStmt(unique_ptr<Expr> expr) : expr(std::move(expr)) {}

    void execute(Env& env, vector<long long>& output) const override {
        output.push_back(expr->eval(env));
    }
};

struct RepeatStmt : Stmt {
    vector<unique_ptr<Stmt>> body;
    unique_ptr<BoolExpr> cond;

    RepeatStmt(vector<unique_ptr<Stmt>> body, unique_ptr<BoolExpr> cond)
        : body(std::move(body)), cond(std::move(cond)) {}

    void execute(Env& env, vector<long long>& output) const override {
        while (true) {
            for (const auto& stmt : body) {
                stmt->execute(env, output);
            }
            if (!cond->eval(env)) {
                break;
            }
        }
    }
};

struct SelectStmt : Stmt {
    unique_ptr<BoolExpr> cond;
    vector<unique_ptr<Stmt>> true_body;
    vector<unique_ptr<Stmt>> false_body;

    SelectStmt(unique_ptr<BoolExpr> cond,
               vector<unique_ptr<Stmt>> true_body,
               vector<unique_ptr<Stmt>> false_body)
        : cond(std::move(cond)),
          true_body(std::move(true_body)),
          false_body(std::move(false_body)) {}

    void execute(Env& env, vector<long long>& output) const override {
        const auto& body = cond->eval(env) ? true_body : false_body;
        for (const auto& stmt : body) {
            stmt->execute(env, output);
        }
    }
};

struct Symbol {
    bool is_const;
    long long value;
};

struct Program {
    Env env;
    vector<unique_ptr<Stmt>> statements;
};

class Parser {
   public:
    explicit Parser(vector<string> tokens) : tokens(std::move(tokens)) {}

    Program parse_program() {
        while (peek() == "variable" || peek() == "constant") {
            parse_declaration();
        }

        vector<unique_ptr<Stmt>> statements;
        while (!at_end()) {
            statements.push_back(parse_statement());
        }

        Env env;
        for (const auto& item : symbols) {
            env[item.first] = item.second.value;
        }
        return Program{std::move(env), std::move(statements)};
    }

   private:
    vector<string> tokens;
    size_t pos = 0;
    unordered_map<string, Symbol> symbols;

    bool at_end() const {
        return pos >= tokens.size();
    }

    string peek() const {
        if (at_end()) {
            return "";
        }
        return tokens[pos];
    }

    string pop() {
        if (at_end()) {
            throw SyntaxErrorInProgram();
        }
        return tokens[pos++];
    }

    void expect(const string& expected) {
        if (pop() != expected) {
            throw SyntaxErrorInProgram();
        }
    }

    void parse_declaration() {
        string kind = pop();
        string name = pop();
        if (!is_var_name(name) || symbols.count(name) != 0) {
            throw SyntaxErrorInProgram();
        }

        if (kind == "variable") {
            expect(";");
            symbols[name] = Symbol{false, 0};
            return;
        }

        expect("=");
        string number = pop();
        if (!is_number(number)) {
            throw SyntaxErrorInProgram();
        }
        expect(";");
        symbols[name] = Symbol{true, stoll(number)};
    }

    unique_ptr<Stmt> parse_statement() {
        string token = peek();

        if (token == "print") {
            pop();
            auto expr = parse_aexpr();
            expect(";");
            return make_unique<PrintStmt>(std::move(expr));
        }

        if (token == "repeat") {
            pop();
            auto body = parse_block();
            expect("until");
            expect("(");
            auto cond = parse_bexpr();
            expect(")");
            expect(";");
            return make_unique<RepeatStmt>(std::move(body), std::move(cond));
        }

        if (token == "select") {
            pop();
            expect("(");
            auto cond = parse_bexpr();
            expect(")");
            auto true_body = parse_block();
            expect("else");
            auto false_body = parse_block();
            expect(";");
            return make_unique<SelectStmt>(
                std::move(cond), std::move(true_body), std::move(false_body));
        }

        if (is_var_name(token)) {
            string name = pop();
            auto found = symbols.find(name);
            if (found == symbols.end() || found->second.is_const) {
                throw SyntaxErrorInProgram();
            }
            expect("=");
            auto expr = parse_aexpr();
            expect(";");
            return make_unique<AssignStmt>(name, std::move(expr));
        }

        throw SyntaxErrorInProgram();
    }

    vector<unique_ptr<Stmt>> parse_block() {
        expect("begin");
        vector<unique_ptr<Stmt>> body;
        while (peek() != "end") {
            if (at_end()) {
                throw SyntaxErrorInProgram();
            }
            body.push_back(parse_statement());
        }
        expect("end");
        return body;
    }

    unique_ptr<BoolExpr> parse_bexpr() {
        auto left = parse_aexpr();
        string op = pop();
        if (!(op == "==" || op == "!=" || op == "<" || op == ">" ||
              op == "<=" || op == ">=")) {
            throw SyntaxErrorInProgram();
        }
        auto right = parse_aexpr();
        return make_unique<BoolExpr>(op, std::move(left), std::move(right));
    }

    unique_ptr<Expr> parse_aexpr() {
        auto expr = parse_term();
        while (peek() == "+" || peek() == "-" || peek() == "*") {
            string op = pop();
            auto right = parse_term();
            expr = make_unique<BinExpr>(op, std::move(expr), std::move(right));
        }
        return expr;
    }

    unique_ptr<Expr> parse_term() {
        bool negated = false;
        if (peek() == "-") {
            pop();
            negated = true;
        }

        string token = pop();
        unique_ptr<Expr> expr;
        if (is_number(token)) {
            expr = make_unique<NumberExpr>(stoll(token));
        } else if (is_var_name(token)) {
            if (symbols.count(token) == 0) {
                throw SyntaxErrorInProgram();
            }
            expr = make_unique<VarExpr>(token);
        } else if (token == "(") {
            expr = parse_aexpr();
            expect(")");
        } else {
            throw SyntaxErrorInProgram();
        }

        if (negated) {
            return make_unique<NegExpr>(std::move(expr));
        }
        return expr;
    }
};

vector<string> tokenize(const string& line) {
    vector<string> tokens;
    string current;

    auto flush = [&]() {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (size_t i = 0; i < line.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(line[i]);

        if (isspace(c)) {
            flush();
            continue;
        }

        if (i + 1 < line.size() && line[i + 1] == '=' &&
            (c == '=' || c == '!' || c == '<' || c == '>')) {
            flush();
            string op;
            op += static_cast<char>(c);
            op += '=';
            tokens.push_back(op);
            ++i;
            continue;
        }

        if (c == ';' || c == '(' || c == ')' || c == '+' || c == '-' ||
            c == '*' || c == '=' || c == '<' || c == '>') {
            flush();
            tokens.push_back(string(1, static_cast<char>(c)));
            continue;
        }

        current += static_cast<char>(c);
    }

    flush();
    return tokens;
}

string normalize_input(string line) {
    const string en_dash = string("\xE2") + string("\x80") + string("\x93");
    size_t pos = 0;
    while ((pos = line.find(en_dash, pos)) != string::npos) {
        line.replace(pos, en_dash.size(), "-");
        ++pos;
    }
    return line;
}

vector<long long> run_line(const string& line) {
    Parser parser(tokenize(normalize_input(line)));
    Program program = parser.parse_program();
    vector<long long> output;
    for (const auto& stmt : program.statements) {
        stmt->execute(program.env, output);
    }
    return output;
}

int main() {
    string line;
    while (getline(cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        bool blank = true;
        for (char c : line) {
            if (!isspace(static_cast<unsigned char>(c))) {
                blank = false;
                break;
            }
        }
        if (blank) {
            break;
        }

        try {
            vector<long long> output = run_line(line);
            if (!output.empty()) {
                for (size_t i = 0; i < output.size(); ++i) {
                    if (i > 0) {
                        cout << ' ';
                    }
                    cout << output[i];
                }
                cout << endl;
            }
        } catch (...) {
            cout << "Syntax Error!" << endl;
        }
    }

    return 0;
}
