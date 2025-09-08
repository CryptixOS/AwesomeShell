/*
 * Created by v1tr10l7 on 08.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <cstdio>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <Lexer.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/String.hpp>
#include <Prism/String/StringView.hpp>

uid_t       s_UserID    = -1;
int         s_SessionID = -1;
termios     s_Termios;
const char* s_Cwd = "/";

using namespace Prism;

extern "C"
{
    void _Unwind_RaiseException() {}
    void _Unwind_Resume_or_Rethrow() {}
    void _Unwind_Resume() {}
    void _Unwind_GetTextRelBase() {}
    void _Unwind_SetGR() {}
    void _Unwind_SetIP() {}
    void _Unwind_GetRegionStart() {}
    void _Unwind_GetLanguageSpecificData() {}
    void _Unwind_GetDataRelBase() {}
    void _Unwind_GetIPInfo() {}
    void _Unwind_DeleteException() {}
}

void prompt()
{
    constexpr StringView p = "awsh>";

    printf(p.Raw());
    fflush(stdout);
}

static Vector<char> s_KeyBuffer;
bool                readLine()
{
    s_KeyBuffer.Clear();
    for (;;)
    {
        char    keybuf[16];
        ssize_t nread = read(0, keybuf, sizeof(keybuf));
        if (nread == 0) exit(0);

        for (ssize_t i = 0; i < nread; ++i)
        {
            char ch = keybuf[i];
            if (ch == 0) continue;
            if (ch == '\n')
            {
                s_KeyBuffer.PushBack(0);
                return true;
            }

            s_KeyBuffer.EmplaceBack(ch);
        }
    }
    return 0;
}

Vector<char*> SplitArguments(StringView line)
{
    Vector<char*> args;
    usize         start     = 0;
    usize         end       = StringView::NPos;

    auto          findSpace = [line](usize pos) -> usize
    {
        for (usize i = pos; i < line.Size(); i++)
            if (line[i] == ' ') return i;
        return StringView::NPos;
    };

    while ((end = findSpace(start)) != StringView::NPos)
    {
        usize length = end - start;
        char* arg    = new char[length + 1];
        std::memcpy(arg, line.Raw() + start, length);
        arg[length] = 0;

        start       = end + 1;
        args.PushBack(arg);
    }

    // handle last segment
    if (start < line.Size())
    {
        usize length = line.Size() - start;
        char* arg    = new char[length + 1];
        std::memcpy(arg, line.Raw() + start, length);
        arg[length] = 0;

        args.PushBack(arg);
    }

    args.PushBack(0);
    return args;
}

void executeCommand(StringView line)
{
    printf("line: %s\n", line.Raw());
    auto args = SplitArguments(line);
    for (usize i = 0; const auto segment : args)
        printf("%zu: '%s'\n", i++, segment ? segment : "(null)");

    int pid = fork();
    if (pid == -1)
    {
        perror("awsh: fork failed\n");
        exit(-1);
    }
    else if (pid == 0) { execvp(args[0], (char**)args.Raw()); }

    for (const auto& arg : args) delete[] arg;
    int status;
    waitpid(pid, &status, 0);
}

ErrorOr<void> NeonMain(const Vector<StringView>& args,
                       const Vector<StringView>& envs)
{
    IgnoreUnused(envs);
    Lexer lexer(const_cast<Vector<StringView>&>(args));
    auto  tokens = PM_TryOrRet(lexer.Analyze());

    s_UserID     = getuid();
    s_SessionID  = setsid();
    tcsetpgrp(0, getpgrp());
    tcgetattr(0, &s_Termios);

    auto* cwd = getcwd(nullptr, 0);
    s_Cwd     = cwd;
    free(cwd);

    for (;;)
    {
        prompt();
        if (!readLine()) continue;
        StringView line(s_KeyBuffer.begin(), s_KeyBuffer.Size());

        printf("%s\n", line.Raw());
        executeCommand(line);
    }

    return {};
}
