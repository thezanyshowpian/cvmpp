#include "cvm/disassembler.hpp"
#include "cvm/lexer.hpp"
#include "cvm/parser.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static int failures = 0;

static void check(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << '\n';
        ++failures;
    }
}

// Lex + parse a source string; return (stmts, hadError).
static std::pair<std::vector<StmtPtr>, bool> parseStr(const std::string& src) {
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto stmts = parser.parse();
    bool err = parser.hadError();
    return {std::move(stmts), err};
}

// Capture AstPrinter stdout output as a string.
static std::string captureAst(const std::vector<StmtPtr>& stmts) {
    std::ostringstream oss;
    auto* old = std::cout.rdbuf(oss.rdbuf());
    AstPrinter{}.print(stmts);
    std::cout.rdbuf(old);
    return oss.str();
}

int main() {
    // ── Section 1: Literals and atoms ────────────────────────────────────────
    {
        auto [stmts, err] = parseStr("42;");
        check(!err && stmts.size() == std::size_t{1}, "literal 42 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* lit = es ? dynamic_cast<Literal*>(es->expression.get()) : nullptr;
        check(lit && lit->value.type == ValueType::INT && lit->value.i == 42,
              "42 is INT Literal(42)");
    }
    {
        auto [stmts, err] = parseStr("true;");
        check(!err, "literal true parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* lit = es ? dynamic_cast<Literal*>(es->expression.get()) : nullptr;
        check(lit && lit->value.type == ValueType::BOOL && lit->value.b == true,
              "true is BOOL Literal(true)");
    }
    {
        auto [stmts, err] = parseStr("false;");
        check(!err, "literal false parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* lit = es ? dynamic_cast<Literal*>(es->expression.get()) : nullptr;
        check(lit && lit->value.type == ValueType::BOOL && lit->value.b == false,
              "false is BOOL Literal(false)");
    }
    {
        auto [stmts, err] = parseStr("input;");
        check(!err, "input parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        check(es && dynamic_cast<Input*>(es->expression.get()) != nullptr,
              "input is Input node");
    }
    {
        auto [stmts, err] = parseStr("x;");
        check(!err, "identifier parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* var = es ? dynamic_cast<Variable*>(es->expression.get()) : nullptr;
        check(var && var->name.lexeme == "x", "x is Variable(x)");
    }

    // ── Section 2: Arithmetic precedence ─────────────────────────────────────
    {
        // 1 + 2 * 3  →  root is +, right child is *
        auto [stmts, err] = parseStr("1 + 2 * 3;");
        check(!err, "1+2*3 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* add = es ? dynamic_cast<Binary*>(es->expression.get()) : nullptr;
        check(add && add->op.lexeme == "+", "1+2*3 root is +");
        auto* mul = add ? dynamic_cast<Binary*>(add->right.get()) : nullptr;
        check(mul && mul->op.lexeme == "*", "1+2*3 right child is *");
    }
    {
        // (1 + 2) * 3  →  root is *, left child is +
        auto [stmts, err] = parseStr("(1 + 2) * 3;");
        check(!err, "(1+2)*3 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* mul = es ? dynamic_cast<Binary*>(es->expression.get()) : nullptr;
        check(mul && mul->op.lexeme == "*", "(1+2)*3 root is *");
        auto* add = mul ? dynamic_cast<Binary*>(mul->left.get()) : nullptr;
        check(add && add->op.lexeme == "+", "(1+2)*3 left child is +");
    }

    // ── Section 3: Unary ──────────────────────────────────────────────────────
    {
        auto [stmts, err] = parseStr("-5;");
        check(!err, "-5 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* neg = es ? dynamic_cast<Unary*>(es->expression.get()) : nullptr;
        check(neg && neg->op.lexeme == "-", "-5 is Unary(-)");
        auto* lit = neg ? dynamic_cast<Literal*>(neg->right.get()) : nullptr;
        check(lit && lit->value.i == 5, "-5 right child is Literal(5)");
    }

    // ── Section 4: Comparison and equality ───────────────────────────────────
    {
        auto [stmts, err] = parseStr("3 < 5;");
        check(!err, "3<5 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* cmp = es ? dynamic_cast<Binary*>(es->expression.get()) : nullptr;
        check(cmp && cmp->op.lexeme == "<", "3<5 is Binary(<)");
    }
    {
        auto [stmts, err] = parseStr("4 == 4;");
        check(!err, "4==4 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* eq = es ? dynamic_cast<Binary*>(es->expression.get()) : nullptr;
        check(eq && eq->op.lexeme == "==", "4==4 is Binary(==)");
    }

    // ── Section 5: Assignment (right-associative) ─────────────────────────────
    {
        auto [stmts, err] = parseStr("x = 10;");
        check(!err, "x=10 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* asgn = es ? dynamic_cast<Assign*>(es->expression.get()) : nullptr;
        check(asgn && asgn->name.lexeme == "x", "x=10 is Assign(x, ...)");
        auto* lit = asgn ? dynamic_cast<Literal*>(asgn->value.get()) : nullptr;
        check(lit && lit->value.i == 10, "x=10 value is Literal(10)");
    }
    {
        // a = b = 5  →  Assign(a, Assign(b, Literal(5)))
        auto [stmts, err] = parseStr("a = b = 5;");
        check(!err, "a=b=5 parses");
        auto* es = dynamic_cast<ExprStmt*>(stmts[0].get());
        auto* outer = es ? dynamic_cast<Assign*>(es->expression.get()) : nullptr;
        check(outer && outer->name.lexeme == "a", "a=b=5 outer target is a");
        auto* inner = outer ? dynamic_cast<Assign*>(outer->value.get()) : nullptr;
        check(inner && inner->name.lexeme == "b", "a=b=5 inner target is b (right-assoc)");
    }

    // ── Section 6: Statements ─────────────────────────────────────────────────
    {
        auto [stmts, err] = parseStr("let x = 42;");
        check(!err && stmts.size() == std::size_t{1}, "let x=42 parses");
        auto* ls = dynamic_cast<LetStmt*>(stmts[0].get());
        check(ls && ls->name.lexeme == "x", "let x=42 name is x");
        auto* lit = ls ? dynamic_cast<Literal*>(ls->initializer.get()) : nullptr;
        check(lit && lit->value.i == 42, "let x=42 initializer is Literal(42)");
    }
    {
        auto [stmts, err] = parseStr("print x + 1;");
        check(!err && stmts.size() == std::size_t{1}, "print x+1 parses");
        auto* ps = dynamic_cast<PrintStmt*>(stmts[0].get());
        check(ps != nullptr, "print is PrintStmt");
        auto* add = ps ? dynamic_cast<Binary*>(ps->expression.get()) : nullptr;
        check(add && add->op.lexeme == "+", "print x+1 expression is Binary(+)");
    }
    {
        auto [stmts, err] = parseStr("if (n < 10) { print true; }");
        check(!err && stmts.size() == std::size_t{1}, "if without else parses");
        auto* is = dynamic_cast<IfStmt*>(stmts[0].get());
        check(is != nullptr, "if is IfStmt");
        check(is && is->elseBranch == nullptr, "if without else has null elseBranch");
    }
    {
        auto [stmts, err] = parseStr("if (n < 10) { print true; } else { print false; }");
        check(!err && stmts.size() == std::size_t{1}, "if-else parses");
        auto* is = dynamic_cast<IfStmt*>(stmts[0].get());
        check(is && is->elseBranch != nullptr, "if-else has non-null elseBranch");
    }
    {
        auto [stmts, err] = parseStr("while (i < 5) { i = i + 1; }");
        check(!err && stmts.size() == std::size_t{1}, "while parses");
        check(dynamic_cast<WhileStmt*>(stmts[0].get()) != nullptr, "while is WhileStmt");
    }

    // ── Section 7: AstPrinter output ─────────────────────────────────────────
    {
        auto [stmts, err] = parseStr("let x = 10;\nlet y = 20;\nx = x + y;\nprint x;");
        check(!err && stmts.size() == std::size_t{4}, "4-statement program parses");
        std::string out = captureAst(stmts);
        std::string expected =
            "(let x 10)\n"
            "(let y 20)\n"
            "(expr-stmt (assign x (+ x y)))\n"
            "(print x)\n";
        check(out == expected, "AstPrinter output matches expected");
        if (out != expected) {
            std::cerr << "  expected: " << expected;
            std::cerr << "  got:      " << out;
        }
    }

    // ── Section 8: Error recovery ─────────────────────────────────────────────
    {
        // "let x = ;" errors; "print 1;" should still parse
        std::ostringstream devnull;
        auto* savedErr = std::cerr.rdbuf(devnull.rdbuf());
        auto [stmts, err] = parseStr("let x = ;\nprint 1;");
        std::cerr.rdbuf(savedErr);
        check(err, "let x = ; sets hadError");
        check(stmts.size() == std::size_t{1}, "error recovery: print 1 parsed");
        auto* ps = stmts.empty() ? nullptr : dynamic_cast<PrintStmt*>(stmts[0].get());
        check(ps != nullptr, "recovered statement is PrintStmt");
    }
    {
        // "let = 5;" errors (missing identifier)
        std::ostringstream devnull;
        auto* savedErr = std::cerr.rdbuf(devnull.rdbuf());
        auto [stmts, err] = parseStr("let = 5;");
        std::cerr.rdbuf(savedErr);
        check(err, "let = 5; sets hadError");
    }

    // ── Section 9: Line numbers ───────────────────────────────────────────────
    {
        auto [stmts, err] = parseStr("let x = 10;\nlet y = 20;");
        check(!err && stmts.size() == std::size_t{2}, "2-line program parses");
        check(stmts[0]->line == 1, "first statement is on line 1");
        check(stmts[1]->line == 2, "second statement is on line 2");
    }

    // ── Result ────────────────────────────────────────────────────────────────
    if (failures == 0) {
        std::cout << "All parser tests passed.\n";
        return 0;
    }
    std::cerr << failures << " test(s) FAILED.\n";
    return 1;
}
