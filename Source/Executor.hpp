/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Lowerer.hpp>
#include <Prism/String/String.hpp>

class Executor
{
  public:
    Executor(Program& program, isize lastExitCode = 0, bool debugLog = false);

    isize Execute();
    isize Execute(StringView name, const Vector<String>& args);

  private:
    Program& m_Program;
    isize    m_LastExitCode = 0;
    bool     m_DebugLog     = false;

    void     HandleExec(const Instruction& instr);
    void     HandleExpandWords(const Instruction& instr);
    void     HandleSetVar(const Instruction& instr);
};
