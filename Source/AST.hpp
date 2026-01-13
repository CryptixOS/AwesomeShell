/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/String/String.hpp>

#include <sys/wait.h>

using namespace Prism;

inline void PrintIndent(usize indent)
{
    for (usize i = 0; i < indent; ++i) printf(" ");
}

enum class NodeType : u32
{

    eSequence,
    eBackground,
    eAnd,
    eOr,
    ePipeline,
    eCodeBlock,
    eSubShell,
    eCommandSubstitution,
    eArithmetic,
    eVariable,
    eAssignment,
    eCommand,
    eRediraction,

    eWord,
    eCastToCommand,
    eCastToList,
    eCloseFdRedirection,
    eCommandLiteral,
    eComment,
    eContinuationControl,
    eDoubleQuotedString,
    eDynamicEvaluate,
    eExecute,
    eFd2FdRedirection,
    eForLoop,
    eFunctionDeclaration,
    eGlob,
    eHereDoc,
    eHistoryEvent,
    eIfCond,
    eImmediateExpression,
    eJoin,
    eJuxtaposition,
    eListConcatenate,
    eMatchExpr,
    eRange,
    eReadRedirection,
    eReadWriteRedirection,
    eSlice,
    eSimpleVariable,
    eSpecialVariable,
    eStringLiteral,
    eStringPartCompose,
    eSyntaxError,
    eSyntheticValue,
    eTilde,
    eVariableDeclarations,
    eWriteAppendRedirection,
    eWriteRedirection,

    eCondition,
    eRedirection,

    eCount,
};

struct Command : public RefCounted
{
};
struct CommandSequence : public RefCounted
{
    Vector<::Ref<Command>> Commands;
    CommandSequence(auto&& cmds)
        : Commands(Move(cmds))
    {
    }
};
struct ASTNode : public RefCounted
{
    virtual ~ASTNode() = default;
    virtual void                            Print(usize indent = 0) const = 0;
    virtual i32                             Execute() const               = 0;
    virtual ErrorOr<::Ref<CommandSequence>> Run() { return Error(ENOSYS); }
    virtual ErrorOr<Vector<::Ref<Command>>> Evaluate() { return Error(ENOSYS); }

    NodeType                                Type;
};
struct SequenceNode final : ASTNode
{
    Vector<::Ref<ASTNode>> Commands;

    inline SequenceNode() { Type = NodeType::eSequence; }
    virtual void Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Sequence:\n");
        for (auto& cmd : Commands) cmd->Print(indent + 4);
    }
    virtual i32 Execute() const override
    {
        int lastStatus = 0;
        for (auto stmt : Commands) lastStatus = stmt->Execute();

        return lastStatus;
    }
    virtual ErrorOr<::Ref<CommandSequence>> Run() override
    {
        Vector<::Ref<Command>> allCommands;

        for (auto cmd : Commands)
        {
            auto commands = cmd->Evaluate();
            for (auto c : *commands) allCommands.PushBack(c);
        }

        return CreateRef<CommandSequence>(Move(allCommands));
    }

    auto begin() { return Commands.begin(); }
    auto end() { return Commands.end(); }
};
struct BackgroundNode final : public ASTNode
{
    inline BackgroundNode() { Type = NodeType::eBackground; }
    ::Ref<ASTNode> Body;

    virtual void   Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Background:\n");
        Body->Print(indent + 4);
    }
    virtual i32 Execute() const override
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            Body->Execute();
            exit(0);
        }
        return 0;
    }
};
struct ConditionalNode final : public ASTNode
{
    inline ConditionalNode() { ASTNode::Type = NodeType::eCondition; }
    enum class Type
    {
        eAnd,
        eOr
    } CondType;
    ::Ref<ASTNode> Left;
    ::Ref<ASTNode> Right;

    virtual void   Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Conditional: %s\n", CondType == Type::eAnd ? "&&" : "||");
        Left->Print(indent + 1);
        Right->Print(indent + 1);
    }
    virtual i32 Execute() const override
    {
        i32 lhsCode = Left->Execute();
        if (CondType == Type::eAnd)
            return !lhsCode ? Right->Execute() : lhsCode;
        if (CondType == Type::eOr) return lhsCode ? Right->Execute() : lhsCode;

        return lhsCode;
    }
};
struct PipelineNode final : ASTNode
{
    inline PipelineNode() { ASTNode::Type = NodeType::ePipeline; }
    Vector<::Ref<ASTNode>> Commands;

    virtual void           Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Pipeline:\n");
        for (auto& cmd : Commands)
        {
            assert(cmd);
            cmd->Print(indent + 4);
        }
    }
    virtual i32 Execute() const override { return 0; }
};
struct SubshellNode final : ASTNode
{
    inline SubshellNode() { ASTNode::Type = NodeType::eSubShell; }
    ::Ref<ASTNode> Body;

    virtual void   Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Subshell:\n");
        Body->Print(indent + 1);
    }
    virtual i32 Execute() const override
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            i32 status = Body->Execute();
            exit(status);
        }
        int wstatus;
        waitpid(pid, &wstatus, 0);
        if (WIFEXITED(wstatus)) return WEXITSTATUS(wstatus);
        else return -1;
    }
};
struct BlockNode final : public ASTNode
{
    inline BlockNode() { ASTNode::Type = NodeType::eCodeBlock; }
    ::Ref<SequenceNode> Body;

    virtual void        Print(usize indent = 0) const override
    {
        for (usize i = 0; i < indent; ++i) printf(" ");
        printf("Block:\n");
        Body->Print(indent + 4);
    }
    virtual i32 Execute() const override { return Body->Execute(); }
};
struct WordNode final : public ASTNode
{
    inline WordNode() { Type = NodeType::eWord; }

    String                 Value;
    usize                  StartOffset = 0;
    usize                  EndOffset   = 0;
    Vector<::Ref<ASTNode>> Commands;

    virtual void           Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        assert(Value.Raw());
        printf("Word: %s\n", Value.Raw());

        // for (auto child : Commands) child->Print(indent + 4);
    }
    virtual i32 Execute() const override
    {
        // For simplicity, we just print the command name and its arguments
        printf("Executing command: %s\n", Value.Raw());
        for (auto& arg : Commands)
        {
            // Here we would normally evaluate the argument
            // For simplicity, we just print its type
            printf("  Arg: ");
            arg->Print(2);
        }
        // In a real shell, here we would fork and exec the command
        return 0; // Return success for simplicity
    }
};
struct AssignmentNode final : public ASTNode
{
    inline AssignmentNode() { Type = NodeType::eAssignment; }
    String         Variable;
    ::Ref<ASTNode> Value;
    usize          StartOffset = 0;
    usize          EndOffset   = 0;

    virtual void   Print(usize indent = 0) const override
    {
        for (usize i = 0; i < indent; ++i) printf(" ");
        printf("Assignment: %s = \n", Variable.Raw());
        Value->Print(indent + 4);
    }
    virtual i32 Execute() const override
    {
        // For simplicity, we assume Value is a WordNode that evaluates to
        // a string
        ::Ref<WordNode> stmt = Value.template As<WordNode>();
        String          result;
        for (auto& arg : stmt->Commands)
            // Here we would normally execute the command and capture its output
            // For simplicity, we just concatenate the argument names
            result += arg.As<WordNode>()->Value + " ";
        // Trim trailing space
        if (!result.Empty()) result = result.Substr(0, result.Size() - 1);
        // Set the environment variable
        setenv(Variable.Raw(), result.Raw(), 1);
        return 0;
    }
};
struct VariableNode final : public ASTNode
{
    inline VariableNode() { Type = NodeType::eVariable; }
    String       Name;
    usize        StartOffset = 0;
    usize        EndOffset   = 0;
    bool         Braced      = false;

    virtual void Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Variable: %s\n", Name.Raw());
    }
    virtual i32 Execute() const override
    {
        const char* value = getenv(Name.Raw());
        if (value)
        {
            printf("%s", value);
            return 0;
        }
        return 1; // Variable not found
    }
};
struct ArithmeticNode final : public ASTNode
{
    inline ArithmeticNode() { Type = NodeType::eArithmetic; }
    String       Expression;

    virtual void Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Arithmetic: %s\n", Expression.Raw());
    }
    virtual i32 Execute() const override
    {
        // For simplicity, we just print the expression
        printf("Evaluating arithmetic expression: %s\n", Expression.Raw());
        return 0;
    }
};
struct CommandSubstitutionNode final : public ASTNode
{
    inline CommandSubstitutionNode() { Type = NodeType::eCommandSubstitution; }
    ::Ref<ASTNode> Body;

    virtual void   Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("CommandSubstitution:\n");
        Body->Print(indent + 4);
    }
    virtual i32 Execute() const override
    {
        // For simplicity, we just print that we're executing the command
        printf("Executing command substitution:\n");
        return Body->Execute();
    }
};
struct CommandNode final : ASTNode
{
    inline CommandNode() { Type = NodeType::eCommand; }
    String                 Name;
    Vector<::Ref<ASTNode>> Arguments;
    Vector<::Ref<ASTNode>> Redirections;
    ::Ref<ASTNode>         HereDoc;

    virtual void           Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("Command: %s\n", Name.Raw());
        for (auto arg : Arguments)
        {
            assert(arg);
            arg->Print(indent + 4);
        }
        if (!Redirections.Empty())
        {
            PrintIndent(indent + 4);
            printf("Redirections:\n");
            for (auto& r : Redirections) r->Print(indent + 4);
        }
        if (HereDoc)
        {
            printf("HereDoc:\n");
            HereDoc->Print(indent + 4);
        }
    }
    virtual i32 Execute() const override
#if 0
    {
        Vector<const char*> argv;
        for (auto& arg : Arguments) argv.PushBack(arg->Name.Raw());

        printf("Executing command: %s\n", Name.Raw());
        for (auto& arg : argv) printf("%s\n", arg);
        pid_t pid = fork();
        if (pid == 0)
        {
            // Here we would set up redirections
            argv.PushBack(nullptr);
            execvp(argv[0], const_cast<char* const*>(argv.Raw()));
            perror("execvp failed");
            exit(1);
        }

        int wstatus = 0;
        while (!WIFEXITED(wstatus)) waitpid(pid, &wstatus, 0);
        return WEXITSTATUS(wstatus);
    }
#else
    {
        return 0;
    }
#endif
};
struct RedirectionNode final : public ASTNode
{
    inline RedirectionNode() { ASTNode::Type = NodeType::eRedirection; }
    enum class Type
    {
        Input,
        Output,
        OutputPipe,
        Append,
        InputFd,
        OutputFd,
        HereDoc
    } RedirType;
    String       Target;

    i32          Fd = -1;
    virtual void Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        const char* typeStr = "";
        switch (RedirType)
        {
            case Type::Input: typeStr = "<"; break;
            case Type::Output: typeStr = ">"; break;
            case Type::OutputPipe: typeStr = "|>"; break;
            case Type::Append: typeStr = ">>"; break;
            case Type::InputFd: typeStr = "<&"; break;
            case Type::OutputFd: typeStr = ">&"; break;
            case Type::HereDoc: typeStr = "<<"; break;
        }
        printf("Redirection: Fd=%d, Type=%s, Target='%s'\n", Fd, typeStr,
               Target.Raw());
    }
    virtual i32 Execute() const override
    {
        // For simplicity, we just print the redirection
        printf("Setting up redirection: Fd=%d, Target='%s'\n", Fd,
               Target.Raw());
        return 0;
    }
};
struct HereDocNode final : public ASTNode
{
    inline HereDocNode() { Type = NodeType::eHereDoc; }
    String       Delimiter;
    String       Content;

    virtual void Print(usize indent = 0) const override
    {
        PrintIndent(indent);
        printf("HereDoc: Delimiter='%s', Content='%s'\n", Delimiter.Raw(),
               Content.Raw());
    }
    virtual i32 Execute() const override
    {
        // For simplicity, we just print the here-document content
        printf("HereDoc Content:\n%s\n", Content.Raw());
        return 0;
    }
};
