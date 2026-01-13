/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Builtins.hpp>
#include <Environment.hpp>
#include <Executor.hpp>
#include <Prism/Debug/Log.hpp>

#include <sys/wait.h>

using namespace Prism;

Executor::Executor(Program& prog, bool debugLog)
    : m_Program(prog)
    , m_DebugLog(debugLog)
{
}
isize Executor::Execute()
{
    for (usize pc = 0; pc < m_Program.Instructions.Size(); pc++)
    {
        auto& instr = m_Program.Instructions[pc];
        switch (instr.Op)
        {
            case OpCode::eExec: HandleExec(instr); break;
            case OpCode::eExpandWords: HandleExpandWords(instr); break;
            case OpCode::eSetVar: HandleSetVar(instr); break;
            case OpCode::eJumpIfNonZero:
            {
                if (m_DebugLog) PrismTrace("NonZero: Word[{}] = ", instr.Arg0);
                isize jumpOffset = instr.Arg0;

                if (m_LastExitCode != 0) pc += jumpOffset;
                break;
            }
            case OpCode::eJumpIfZero:
            {
                if (m_DebugLog) PrismTrace("Zero: Word[{}] = ", instr.Arg0);
                isize jumpOffset = instr.Arg0;

                if (m_LastExitCode == 0) pc += jumpOffset;
                break;
            }
            default: break;
        }
    }

    return 0;
}
isize Executor::Execute(StringView name, const Vector<String>& args)

{
    Vector<const char*> argv;
    for (auto& arg : args) argv.PushBack(arg.Raw());
    argv.PushBack(0);

    i32 pid = fork();
    if (pid == -1)
    {
        perror("awsh: fork failed\n");
        std::exit(1);
    }
    else if (pid == 0)
    {
        execvp(name.Raw(), const_cast<char* const*>(argv.Raw()));
        PrismError("awsh: command not found: {}", name);
        return errno;
    }

    int wstatus = -1;
    while (!WIFEXITED(wstatus)) waitpid(pid, &wstatus, 0);

    int status = WEXITSTATUS(wstatus);
    return status;
};

void Executor::HandleExpandWords(const Instruction& instr)
{
    if (m_DebugLog)
    {
        auto& word = m_Program.WordTable[instr.Arg0];
        fmt::print("Expanded: ");
        for (auto& atom : word->Atoms) fmt::print("{} ", atom.Value);
        fmt::print("\n");
    }
}
void Executor::HandleExec(const Instruction& instr)
{
    auto&         word = m_Program.WordTable[instr.Arg0];

    // Convert Word.Atoms to char*[] for execvp
    Vector<char*> argv;
    for (auto& atom : word->Atoms)
    {
        if (atom.Type == WordAtom::Type::eLiteral)
            argv.PushBack(const_cast<char*>(atom.Value.Raw()));
        else if (atom.Type == WordAtom::Type::eVariable)
        {
            // TODO(v1tr10l7): Exit Code
            // if (atom.Value == "?"_sv)
            //     ;
            auto env = Environment::GetVariable(StringView(atom.Value));
            argv.PushBack(const_cast<char*>(env.Raw()));
        }
    }
    argv.PushBack(nullptr);

    if (m_DebugLog) PrismTrace("Executor: Executing command => {}", argv[0]);
    for (usize i = 0; m_DebugLog && i < argv.Size() - 1; i++)
        PrismMessage("argv[{}]: '{}'\n", i, argv[i]);

    auto name   = argv[0];
    auto status = Builtins::TryRun(name, argv);
    if (status.HasValue())
    {
        m_LastExitCode = *status;
        return;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        execvp(argv[0], argv.Raw());
        perror("execvp failed");
        exit(1);
    }

    int wstatus = 0;
    waitpid(pid, &wstatus, 0);

    m_LastExitCode = WEXITSTATUS(wstatus);
    if (m_DebugLog)
        PrismTrace("Executor: Last Exit Status => {}", m_LastExitCode);
}
void Executor::HandleSetVar(const Instruction& instr)
{
    auto       nameWord  = m_Program.WordTable[instr.Arg0];
    auto       valueWord = m_Program.WordTable[instr.Arg1];
    StringView name      = nameWord->Atoms[0].Value;
    StringView value     = valueWord->Atoms[0].Value;

    Environment::SetVariable(name, value);
}
