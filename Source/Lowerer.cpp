/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Lowerer.hpp>

Lowerer::Lowerer(Ref<ASTNode> node)
    : AST(node)
{
}

isize Lowerer::AddWord(Ref<Word> w)
{
    Program.WordTable.PushBack(w);
    return Program.WordTable.Size() - 1;
}
isize Lowerer::Emit(OpCode op, int arg0, isize arg1)
{
    Program.Instructions.PushBack({op, arg0, arg1, -1});
    return Program.Instructions.Size() - 1;
}

struct Program Lowerer::Lower()
{
    auto node = AST;
    LowerNode(node);

    return Program;
}
void Lowerer::LowerNode(Ref<ASTNode> node)
{
    if (node->Type == NodeType::eSequence)
        for (auto& cmd : node.template As<SequenceNode>()->Commands)
            LowerNode(cmd);
    else if (node->Type == NodeType::eCommand)
    {
        auto c = node.template As<CommandNode>();
        auto w = CreateRef<Word>();
        for (auto& arg : c->Arguments)
        {
            if (arg->Type == NodeType::eWord)
                w->Atoms.EmplaceBack(WordAtom::Type::eLiteral,
                                     arg.template As<WordNode>()->Value);
            else if (arg->Type == NodeType::eVariable)
                w->Atoms.EmplaceBack(WordAtom::Type::eVariable,
                                     arg.template As<VariableNode>()->Name);
        }

        isize idx = AddWord(w);
        Emit(OpCode::eExpandWords, idx);
        Emit(OpCode::eExec, idx);
    }
    else if (node->Type == NodeType::eAssignment)
    {
        auto assign   = node.template As<AssignmentNode>();
        auto nameWord = CreateRef<Word>();
        nameWord->Atoms.EmplaceBack(WordAtom::Type::eLiteral, assign->Variable);
        isize nameIndex = AddWord(nameWord);

        if (assign->Value->Type == NodeType::eWord)
        {
            auto valueWord = CreateRef<Word>();
            valueWord->Atoms.EmplaceBack(
                WordAtom::Type::eLiteral,
                assign->Value.template As<WordNode>()->Value);

            isize valueIndex = AddWord(valueWord);
            Emit(OpCode::eSetVar, nameIndex, valueIndex);
        }
    }
    else if (node->Type == NodeType::eCondition)
    {
        auto cond = node.template As<ConditionalNode>();
        LowerNode(cond->Left);
        isize jumpIdx = Emit((cond->CondType == ConditionalNode::Type::eAnd)
                                 ? OpCode::eJumpIfNonZero
                                 : OpCode::eJumpIfZero,
                             0 // placeholder
        );
        LowerNode(cond->Right);
        // patch jump to skip over right-hand side if needed
        Program.Instructions[jumpIdx].Arg0
            = static_cast<isize>(Program.Instructions.Size() - jumpIdx - 1);
    }
}
