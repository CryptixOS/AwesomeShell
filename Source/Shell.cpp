/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Executor.hpp>
#include <Lexer.hpp>
#include <Lowerer.hpp>
#include <Parser.hpp>

#include <Prism/Debug/Log.hpp>
#include <Prism/String/Formatter.hpp>
#include <Prism/String/String.hpp>
#include <Prism/String/StringBuilder.hpp>
#include <Prism/Utility/Optional.hpp>

#include <Shell.hpp>

#include <sys/wait.h>
#include <termios.h>

using namespace Prism;

namespace Shell
{
    namespace
    {
        PM_UNUSED bool     s_Restricted = false;
        PM_UNUSED bool     s_PosixMode  = false;
        PM_UNUSED bool     s_Verbose    = false;
        PM_UNUSED TestMode s_TestMode   = TestMode::eNone;

        uid_t              s_UserID     = -1;
        int                s_SessionID  = -1;
        termios            s_Termios;
        String             s_Cwd = "/";

        Vector<StringView> s_Environment;
        isize              s_LastErrorCode = 0;

        void Print(StringView string) { PrismMessage("{}", string); }
        void Prompt()
        {
            constexpr StringView prompt = "\e[31mawsh\e[0m>"_sv;

            Print(prompt);
            fflush(stdout);
        }
        StringBuilder    s_CommandLineBuilder;
        Optional<String> ReadLine()
        {
            s_CommandLineBuilder.Reset();
            for (;;)
            {
                char  keybuf[16];
                usize nread = read(0, keybuf, sizeof(keybuf));
                if (nread == 0) std::exit(0);

                for (usize i = 0; i < nread; ++i)
                {
                    char ch = keybuf[i];
                    if (ch == '\e')
                    {
                        if (i + 2 < nread && keybuf[i + 1] == '[')
                            printf("Arrow key pressed => %c\n", keybuf[i + 2]);
                    }

                    if (ch == 0) continue;
                    if (ch == '\n') return s_CommandLineBuilder.ToString();

                    s_CommandLineBuilder << ch;
                }
            }

            return NullOpt;
        }

#if 0
        void DoTest()
        {
            // String code
            //     = "FOO=bar BAR=42 {\n"
            //       "echo \"start:$FOO:$BAR\" |\n"
            //       "( TMP=$(tr 'a-z' 'A-Z') && echo \"inner:$TMP\" || echo bsb
            //       )
            //       |\n"
            //       "{ read line; RESULT=\"$line:$(wc -c)\"; echo \"$RESULT\"
            //       || ls;
            //       }\n"
            //       "}\n"
            //       "echo $((1+2/3));\n echo $HOME > ./home; cat < $FILE;";
            String code = R"(echo "Hello, World!"; ls -l; pwd)";

            PrismTrace("Lexer: Analyzing input => {}", code);
            Lexer lexer(code);
            auto& tokens = lexer.Analyze();
            PrismInfo("Lexer: Lexing finished, {} tokens produced",
                      tokens.Size());
            for (const auto& token : tokens)
                PrismMessage("Token: Type={}, Value='{}'\n",
                             StringUtils::ToString(token.Type), token.Text);

            PrismTrace("Parser: Parsing tokens...");
            Parser parser(tokens);
            auto   node = parser.Parse();
            PrismInfo("Parser: Parsing finished");

            PrismMessage("Parser: AST Dump:");
            node->Print();
            PrismTrace("Executor: Executing AST...");
            auto errorCode = node->Execute();
            PrismInfo("Executor: Execution finished with code {}", errorCode);
        }
#endif
    }; // namespace

    void Initialize(const Vector<StringView>& envp)
    {
        s_Environment = envp;

        s_UserID      = getuid();
        s_SessionID   = setsid();

        tcsetpgrp(0, getpgrp());
        tcgetattr(0, &s_Termios);

        auto* cwd = getcwd(nullptr, 0);
        s_Cwd     = cwd;
        free(cwd);
    }
    ErrorOr<void> Run()
    {
        for (;;)
        {
            Prompt();
            auto result = ReadLine();
            if (!result.HasValue()) continue;

            auto command = *result;
            Print(command);
            Print("\n");

            auto status     = RunCommand(command);
            s_LastErrorCode = status ? 0 : status.Error();
        }
    }

    void          EnableRestricted() { s_Restricted = true; }
    void          EnablePosixMode() { s_PosixMode = true; }
    void          EnableVerbose() { s_Verbose = true; }
    void          EnableTesting(TestMode mode) { s_TestMode = mode; }

    ErrorOr<void> RunCommand(StringView line)
    {
        Lexer lexer(line.Trim());
        auto& tokens = lexer.Analyze();

        bool  exit   = false;
        IgnoreUnused(exit);
        if (line.StartsWith("exit")
            && (line.Size() == 4 || IsSpace(line[4]) || line[4] == ';'))
            exit = true;

        if (s_TestMode & TestMode::eLexer)
        {
            PrismTrace("Shell: Dumping tokens produced by lexer:");
            for (const auto& token : tokens)
                PrismMessage("Token: Type={}, Value='{}'\n",
                             StringUtils::ToString(token.Type), token.Text);

            if (!(s_TestMode & (TestMode::eParser | TestMode::eExecutor)))
                return {};
        }

        Parser parser(tokens);
        auto   ast = parser.Parse();
        if (s_TestMode & TestMode::eParser)
        {
            ast->Print();
            if (!(s_TestMode & TestMode::eExecutor)) return {};
        }

        Lowerer lowerer(ast);
#define DebugTrace(...)                                                        \
    if (s_TestMode & TestMode::eExecutor) { PrismTrace(__VA_ARGS__); }
#define DebugInfo(...)                                                         \
    if (s_TestMode & TestMode::eExecutor) { PrismInfo(__VA_ARGS__); }

        DebugTrace("Shell: Lowering the ast into IR");
        auto lowered = lowerer.Lower();
        if (s_TestMode & TestMode::eExecutor) DumpProgram(lowered);
        DebugInfo("Shell: Lowering complete");
        Executor e(lowered);
        DebugTrace("Shell: Executin IR");
        e.Execute();
        DebugInfo("Shell: Executing done");

        return {};
    }
    ErrorOr<void> RunFile(PathView path)
    {
        IgnoreUnused(path);
        return Error(ENOSYS);
    }
}; // namespace Shell
