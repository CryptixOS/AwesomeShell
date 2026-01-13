/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <AST.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/String/StringUtils.hpp>

enum class OpCode
{
    eExpandWords, // expand a Word
    eExec,        // execute a command
    eGetVar,
    eSetVar,
    eJumpIfNonZero,
    eJumpIfZero,
};

struct Instruction
{
    OpCode Op;
    isize  Arg0 = -1; // index into WordTable
    isize  Arg1 = -1;
    i32    Payload;
};

struct WordAtom
{
    enum class Type
    {
        eLiteral,
        eVariable,
    } Type;

    String Value;
};
struct Word : public RefCounted
{
    Vector<WordAtom> Atoms;
};

struct Program
{
    Vector<Instruction> Instructions;
    Vector<Ref<Word>>   WordTable;
};

struct Lowerer
{
    Lowerer(Ref<ASTNode> node);

    Ref<ASTNode>   AST;
    struct Program Program;

    isize          AddWord(Ref<Word> w);
    isize          Emit(OpCode op, int arg0 = -1, isize arg1 = -1);

    struct Program Lower();
    void           LowerNode(Ref<ASTNode> node);
};
constexpr void DumpProgram(const Program& prog)
{
    fmt::print("=== Words ===\n");
    for (usize i = 0; i < prog.WordTable.Size(); i++)
    {
        fmt::print("[{}]: ", i);
        for (auto& atom : prog.WordTable[i]->Atoms)
            fmt::print("{} ", atom.Value);
        fmt::print("\n");
    }

    fmt::print("=== Redirections ===\n");
    // for (usize i = 0; i < prog.Redirections.size(); i++)
    // {
    //     auto& r = prog.Redirections[i];
    //     fmt::print("[{}] FD: {} Mode: {} Target: ", i, r.FD,
    //                (r.Mode == Redirection::Type::Read    ? "Read"
    //                 : r.Mode == Redirection::Type::Write ? "Write"
    //                                                      : "Append"));
    //     for (auto& atom : r.Target.Atoms) fmt::print("{} ", atom);
    //     fmt::print("\n");
    // }
    //
    fmt::print("=== Instructions ===\n");
    for (usize i = 0; i < prog.Instructions.Size(); i++)
    {
        auto& instr = prog.Instructions[i];
        fmt::print("[{}] Op: {} Arg0: {} Arg1: {} Payload: {}\n", i,
                   StringUtils::ToString(instr.Op).Raw(), instr.Arg0,
                   instr.Arg1, instr.Payload);
    }
}
