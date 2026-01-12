/*
 * Created by v1tr10l7 on 08.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Debug/Log.hpp>
#include <Prism/String/StringBuilder.hpp>
#include <Shell.hpp>

#include <getopt.h>

using namespace Prism;

static i32    s_SavedArgc = 0;
static char** s_SavedArgv = nullptr;

void          help()
{
    printf("Usage: awsh [options] ...\n");
    printf("       awsh [options] script-file ...\n");
    printf("Options:\n");
    printf("  -c, --command           Read commands from the command string\n");
    printf("  -i, --interactive       Force interactive behavior\n");
    printf("  -l, --login             Act as a login shell\n");
    printf("  -r, --restricted        Run in restricted mode\n");
    printf("  -p, --posix             Enable POSIX-compliant behavior\n");
    printf(
        "  -t, --test <argument>   Test command execution up to a given "
        "stage\n");
    printf("                     (lexer, parser, executor)\n");
    printf("  -V, --verbose           Enable verbose output\n");
    printf("  -v, --version           Display version information and exit\n");
    printf("  -h, --help              Display this help message and exit\n");
}
void printVersion()
{
    printf("awsh version 0.1.0\n");
    printf("Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>\n");
}

ErrorOr<void> NeonMain(const Vector<StringView>& args,
                       const Vector<StringView>& envs)
{
    auto options = Prism::ToArray<option>({
        {"command", no_argument, nullptr, 'c'},
        {"interactive", no_argument, nullptr, 'i'},
        {"login", no_argument, nullptr, 'l'},
        {"restricted", no_argument, nullptr, 'r'},
        {"posix", no_argument, nullptr, 'p'},
        {"test", required_argument, nullptr, 't'},
        {"verbose", no_argument, nullptr, 'V'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
    });

    enum class RunMode
    {
        eSingleCommand = 1,
        eScriptFile    = 2,
        eInteractive   = 3,
    } runMode
        = RunMode::eScriptFile;

    usize modeSetCount = 0;
    i32   optionIndex  = 0;
    for (;;)
    {
        i32 c = getopt_long(s_SavedArgc, s_SavedArgv, "p:t:h", options.Raw(),
                            &optionIndex);
        if (c == -1) break;
        switch (c)
        {
            case 'c':
                runMode = RunMode::eSingleCommand;
                ++modeSetCount;
                break;
            case 'i':
                runMode = RunMode::eInteractive;
                ++modeSetCount;
                break;
            case 'l': PrismToDoWarn(); break;
            case 'r': Shell::EnableRestricted(); break;
            case 'p': Shell::EnablePosixMode(); break;
            case 't':
            {
                auto testModeString = optarg;
                if (!optarg)
                {
                    PrismError("Test mode requires an argument");
                    return Error(EINVAL);
                }
                auto testMode = Shell::TestMode::eNone;
                if (testModeString == "lexer"_sv)
                    testMode = Shell::TestMode::eLexer;
                else if (testModeString == "parser"_sv)
                    testMode = Shell::TestMode::eParser;
                else if (testModeString == "executor"_sv)
                    testMode = Shell::TestMode::eExecutor;

                Shell::EnableTesting(testMode);
                break;
            }
            case 'V': Shell::EnableVerbose(); break;
            case 'v': printVersion(); return {};
            case 'h': help(); return {};
        }
    }

    if (modeSetCount > 1)
    {
        PrismError("Only one of -c, -i, or script-file can be specified");
        return Error(EINVAL);
    }

    if (optind < s_SavedArgc)
    {
        if (runMode == RunMode::eInteractive)
        {
            PrismError("Cannot specify both -i and a script file to execute");
            return Error(EINVAL);
        }
        else if (runMode == RunMode::eSingleCommand)
        {
            StringBuilder builder;
            for (isize i = optind; i < s_SavedArgc; i++)
            {
                if (i > optind) builder.Append(' ');
                builder.Append(s_SavedArgv[i]);
            }

            return Shell::RunCommand(builder.ToString());
        }

        PathView path = args[optind];
        if (optind + 1 < s_SavedArgc)
            PrismWarn("Additional arguments to script file are ignored");
        return Shell::RunFile(path);
    }
    else if ((runMode == RunMode::eSingleCommand
              || runMode == RunMode::eScriptFile)
             && modeSetCount)
    {
        PrismError("Mode was set to {} and no arguments were specified",
                   StringUtils::ToString(runMode));
        return Error(EINVAL);
    }

    Shell::Initialize(envs);
    return Shell::Run();
}
i32 main(int argc, char** argv, char** envp)
{
    Vector<StringView> argArr;
    Vector<StringView> envArr;

    for (isize i = 0; i < argc && argv[i]; i++) argArr.EmplaceBack(argv[i]);
    for (usize i = 0; envp[i]; i++) envArr.EmplaceBack(envp[i]);

    s_SavedArgc = argc;
    s_SavedArgv = argv;
    auto status = NeonMain(argArr, envArr);
    if (!status) return status.Error();

    return 0;
}
