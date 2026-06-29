import re
import sys


sys.setrecursionlimit(10000)


class SyntaxErrorInProgram(Exception):
    pass


KEYWORDS = {
    "variable",
    "constant",
    "print",
    "repeat",
    "begin",
    "end",
    "until",
    "select",
    "else",
}


MAX_TOKEN_LEN = 10


def is_number(token):
    if not token or len(token) > MAX_TOKEN_LEN:
        return False
    return all("0" <= ch <= "9" for ch in token)


def is_var_name(token):
    if not token or len(token) > MAX_TOKEN_LEN or token in KEYWORDS:
        return False
    if not ("a" <= token[0] <= "z"):
        return False
    return all(("a" <= ch <= "z") or ("0" <= ch <= "9") for ch in token)


class NumberExpr:
    def __init__(self, value):
        self.value = value

    def eval(self, env):
        return self.value


class VarExpr:
    def __init__(self, name):
        self.name = name

    def eval(self, env):
        return env[self.name]


class NegExpr:
    def __init__(self, expr):
        self.expr = expr

    def eval(self, env):
        return -self.expr.eval(env)


class BinExpr:
    def __init__(self, op, left, right):
        self.op = op
        self.left = left
        self.right = right

    def eval(self, env):
        left = self.left.eval(env)
        right = self.right.eval(env)
        if self.op == "+":
            return left + right
        if self.op == "-":
            return left - right
        return left * right


class BoolExpr:
    def __init__(self, op, left, right):
        self.op = op
        self.left = left
        self.right = right

    def eval(self, env):
        left = self.left.eval(env)
        right = self.right.eval(env)
        if self.op == "==":
            return left == right
        if self.op == "!=":
            return left != right
        if self.op == "<":
            return left < right
        if self.op == ">":
            return left > right
        if self.op == "<=":
            return left <= right
        return left >= right


class AssignStmt:
    def __init__(self, name, expr):
        self.name = name
        self.expr = expr

    def execute(self, env, output):
        env[self.name] = self.expr.eval(env)


class PrintStmt:
    def __init__(self, expr):
        self.expr = expr

    def execute(self, env, output):
        output.append(self.expr.eval(env))


class RepeatStmt:
    def __init__(self, body, cond):
        self.body = body
        self.cond = cond

    def execute(self, env, output):
        while True:
            for stmt in self.body:
                stmt.execute(env, output)
            if not self.cond.eval(env):
                break


class SelectStmt:
    def __init__(self, cond, true_body, false_body):
        self.cond = cond
        self.true_body = true_body
        self.false_body = false_body

    def execute(self, env, output):
        body = self.true_body if self.cond.eval(env) else self.false_body
        for stmt in body:
            stmt.execute(env, output)


class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0
        self.symbols = {}

    def peek(self):
        if self.pos >= len(self.tokens):
            return None
        return self.tokens[self.pos]

    def pop(self):
        token = self.peek()
        if token is None:
            raise SyntaxErrorInProgram()
        self.pos += 1
        return token

    def expect(self, expected):
        if self.pop() != expected:
            raise SyntaxErrorInProgram()

    def parse_program(self):
        while self.peek() in ("variable", "constant"):
            self.parse_declaration()

        statements = []
        while self.peek() is not None:
            statements.append(self.parse_statement())

        env = {name: value for name, (is_const, value) in self.symbols.items()}
        return env, statements

    def parse_declaration(self):
        kind = self.pop()
        name = self.pop()
        if not is_var_name(name) or name in self.symbols:
            raise SyntaxErrorInProgram()

        if kind == "variable":
            self.expect(";")
            self.symbols[name] = (False, 0)
            return

        self.expect("=")
        number = self.pop()
        if not is_number(number):
            raise SyntaxErrorInProgram()
        self.expect(";")
        self.symbols[name] = (True, int(number))

    def parse_statement(self):
        token = self.peek()
        if token == "print":
            self.pop()
            expr = self.parse_aexpr()
            self.expect(";")
            return PrintStmt(expr)

        if token == "repeat":
            self.pop()
            body = self.parse_block()
            self.expect("until")
            self.expect("(")
            cond = self.parse_bexpr()
            self.expect(")")
            self.expect(";")
            return RepeatStmt(body, cond)

        if token == "select":
            self.pop()
            self.expect("(")
            cond = self.parse_bexpr()
            self.expect(")")
            true_body = self.parse_block()
            self.expect("else")
            false_body = self.parse_block()
            self.expect(";")
            return SelectStmt(cond, true_body, false_body)

        if is_var_name(token):
            name = self.pop()
            if name not in self.symbols or self.symbols[name][0]:
                raise SyntaxErrorInProgram()
            self.expect("=")
            expr = self.parse_aexpr()
            self.expect(";")
            return AssignStmt(name, expr)

        raise SyntaxErrorInProgram()

    def parse_block(self):
        self.expect("begin")
        body = []
        while self.peek() != "end":
            if self.peek() is None:
                raise SyntaxErrorInProgram()
            body.append(self.parse_statement())
        self.expect("end")
        return body

    def parse_bexpr(self):
        left = self.parse_aexpr()
        op = self.pop()
        if op not in ("==", "!=", "<", ">", "<=", ">="):
            raise SyntaxErrorInProgram()
        right = self.parse_aexpr()
        return BoolExpr(op, left, right)

    def parse_aexpr(self):
        expr = self.parse_term()
        while self.peek() in ("+", "-", "*"):
            op = self.pop()
            right = self.parse_term()
            expr = BinExpr(op, expr, right)
        return expr

    def parse_term(self):
        negated = False
        if self.peek() == "-":
            self.pop()
            negated = True

        token = self.pop()
        if is_number(token):
            expr = NumberExpr(int(token))
        elif is_var_name(token):
            if token not in self.symbols:
                raise SyntaxErrorInProgram()
            expr = VarExpr(token)
        elif token == "(":
            expr = self.parse_aexpr()
            self.expect(")")
        else:
            raise SyntaxErrorInProgram()

        return NegExpr(expr) if negated else expr


_TOKEN_PATTERN = re.compile(r"(==|!=|<=|>=|[;()+\-*=<>])")


def tokenize(line):
    line = line.replace("\u2013", "-")
    line = _TOKEN_PATTERN.sub(r" \1 ", line)
    return line.split()


def run_line(line):
    tokens = tokenize(line)
    parser = Parser(tokens)
    env, statements = parser.parse_program()
    output = []
    for stmt in statements:
        stmt.execute(env, output)
    return output


def main():
    for raw_line in sys.stdin:
        line = raw_line.rstrip("\n")
        if line.endswith("\r"):
            line = line[:-1]
        if line.strip() == "":
            break

        try:
            output = run_line(line)
            if output:
                print(" ".join(str(value) for value in output), flush=True)
        except Exception:
            print("Syntax Error!", flush=True)


if __name__ == "__main__":
    main()
