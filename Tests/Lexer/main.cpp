/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Lexer.hpp>
#include <Prism/Algorithm/Find.hpp>
#include <Prism/Debug/Log.hpp>

struct LexerTestCase
{
    StringView Name;
    StringView Input;
    bool       ExpectErrors;
};

static bool RunLexerTest(const LexerTestCase& test, bool printErrors = true)
{
    Lexer lexer(test.Input, printErrors);

    bool  hadErrors = false;
    auto& tokens    = lexer.Analyze();
    bool  hadUnknown
        = FindIf(tokens.begin(), tokens.end(), [](auto& token) -> bool
                 { return token.Type == TokenType::eUnknown; })
       != tokens.end();

    if (hadUnknown && printErrors)
    {
        PrismError("[FAIL] {} — eUnknown token produced\n", test.Name);
        return false;
    }

    if (test.ExpectErrors != hadErrors)
    {
        // std::cout << "[FAIL] " << test.Name
        //           << " — error expectation mismatch (expected "
        // << test.ExpectErrors << ")\n";
        return false;
    }

    PrismInfo("[PASS] {} \n", test.Name);
    return true;
}

static Vector<LexerTestCase> s_LexerTests = {

    // ---------------- BASIC ----------------
    {"Basic commands",
     R"(echo hello world
ls -l /usr/bin
true && false || echo fail
)",
     false},

    // ---------------- KEYWORDS ----------------
    {"Shell keywords",
     R"(if true; then echo ok; fi
for i in a b c; do echo $i; done
while false; do echo never; done
)",
     false},

    {"Keyword abuse (still lexable)",
     R"(echo in do fi then
command in foo
do_something
)",
     false},

    // ---------------- VARIABLES & SUBSTITUTION ----------------
    {"Variables and substitutions",
     R"(echo $USER ${HOME} ${PATH:-/bin}
echo $(ls -l)
echo `uname -a`
echo $((1 + 2 * 3))
)",
     false},

    // ---------------- QUOTES ----------------
    {"Quote handling",
     R"(echo "hello world"
echo 'literal $USER'
echo 'nested $(echo hi)'
echo "escaped \" quote"
)",
     false},

    // ---------------- HEREDOC ----------------
    {"Basic heredoc",
     R"(cat <<EOF
line1
line2
EOF
)",
     false},

    //     {
    //         "Quoted heredoc",
    //         R"(cat <<'EOF'
    // $USER
    // $(date)
    // EOF
    // )",
    //         false
    //     },

    {"Tab-stripped heredoc",
     R"(cat <<-EOF
	line1
		line2
EOF
)",
     false},

    // ---------------- BRACE EXPANSION ----------------
    {"Brace lists",
     R"(echo {a,b,c}
echo {a,{b,c},d}
)",
     false},

    {"Brace ranges",
     R"(echo {1..5}
echo {a..f}
echo file{01..10}.txt
)",
     false},

    // ---------------- GLOBBING ----------------
    {"Simple globs",
     R"(ls *.cpp
ls file?.txt
ls [a-z]*.c
)",
     false},

    {"Glob inside word",
     R"(echo foo*bar
)",
     false},

    // ---------------- EXTENDED GLOBS ----------------
    {"Extended globbing",
     R"(ls !(main).cpp
ls @(foo|bar).txt
ls **/*.cpp
)",
     false},

    // ---------------- RESERVED WORDS ----------------
    {"Reserved words vs builtins",
     R"(time ls
exec echo hi
coproc foo { echo hi; }
command time echo test
)",
     false},

    // ---------------- LOOKAHEAD CRITICAL ----------------
    {"Function definition",
     R"(foo() {
    echo hello
}
)",
     false},

    {"Subshell",
     R"((foo)
)",
     false},

    {"Ambiguous parens",
     R"(foo () (bar)
)",
     false},

    // ---------------- LINE CONTINUATION ----------------
    {"Line continuation",
     R"(echo hello \
world
)",
     false},
};
static Vector<LexerTestCase> s_LexerErrorCases
    = { // ---------------- ERROR CASES ----------------
        {"Unterminated string",
         R"(echo "unterminated
)",
         true},

        {"Unterminated command substitution",
         R"(echo $(ls
)",
         true},

        {"Unterminated heredoc",
         R"(cat <<EOF
missing
)",
         true}};

int main()
{
    usize testCount = s_LexerTests.Size();
    usize passed    = 0;

    for (const auto& test : s_LexerTests)
        if (RunLexerTest(test)) ++passed;

    // for (const auto& test : s_LexerErrorCases)
    //     if (!RunLexerTest(test, false)) ++passed;

    PrismInfo("\nSummary: {}/{} tests passed\n", passed, testCount);
}
