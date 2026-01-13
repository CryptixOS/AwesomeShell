/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Lexer.hpp>
#include <Parser.hpp>

Ref<ASTNode> Parser::Parse()
{
    auto ast = ParseSequence();
    if (!End())
        PrismError("Parser: The are unconsumed tokens, which indicated error!");

    for (usize pos = m_Pos; pos < m_Tokens.Size(); pos++)
    {
        auto& token = m_Tokens[pos];
        PrismWarn("{}: {}", StringUtils::ToString(token.Type), token.Text);
    }

    return ast;
}

Ref<ASTNode> Parser::ParseSequence()
{
    auto seq = CreateRef<SequenceNode>();
    while (!End() && !Match(TokenType::eRightParen)
           && !Match(TokenType::eRightBrace))
    {
        auto stmt = ParseConditional();
        if (!stmt) break;

        if (Consume(TokenType::eAmpersand))
        {
            auto bg  = CreateRef<BackgroundNode>();
            bg->Body = stmt;
            stmt     = bg;
        }
        seq->Commands.PushBack(stmt);

        Consume(TokenType::eSemicolon);
        Consume(TokenType::eNewLine);
    }

    Consume(TokenType::eEndOfFile);
    return seq;
}
Ref<ASTNode> Parser::ParseConditional()
{
    auto left = ParsePipeline();
    if (!left) return nullptr;

    while (Match(TokenType::eDoubleAmpersand) || Match(TokenType::eDoublePipe))
    {
        auto token = Current();
        Advance();

        auto right = ParsePipeline();
        if (!right) break;

        auto cond      = CreateRef<ConditionalNode>();
        cond->Left     = left;
        cond->Right    = right;
        cond->CondType = token->Type == TokenType::eDoubleAmpersand
                           ? ConditionalNode::Type::eAnd
                           : ConditionalNode::Type::eOr;

        left           = cond;
    }

    return left;
}
Ref<ASTNode> Parser::ParsePipeline()
{
    auto first = ParseStatement();
    if (!first) return nullptr;

    if (!Match(TokenType::ePipe) && !Match(TokenType::ePipeAmpersand))
        return first;

    auto pipeline = CreateRef<PipelineNode>();
    pipeline->Commands.PushBack(first);

    while (Match(TokenType::ePipe) || Match(TokenType::ePipeAmpersand))
    {
        Advance();

        auto stage = ParseStatement();
        if (!stage) break;

        pipeline->Commands.PushBack(stage);
    }

    return pipeline;
}
Ref<ASTNode> Parser::ParseStatement()
{
    if (Match(TokenType::eLeftParen)) return ParseSubshell();
    if (Match(TokenType::eLeftBrace)) return ParseBlock();

    return IsAssignment() ? ParseAssignment() : ParseCommand();
}
Ref<ASTNode> Parser::ParseWord()
{
    const auto t = Current();
    if (!t.HasValue()) return nullptr;

    if (Match(TokenType::eVariable))
    {
        Advance();

        // auto node    = CreateRef<WordNode>();
        auto node  = CreateRef<VariableNode>();
        node->Name = t->Text;
        // node->StartOffset = t->Offset;
        // node->EndOffset   = t->Offset + t->Text.Size();
        // node->Braced = t->Text.StartsWith("{") && t->Text.EndsWith("}");
        return node;
    }
    if (Match(TokenType::eCommandSubst))
    {
        Advance();

        Lexer  subLexer(t->Text);
        auto&  tokens = subLexer.Analyze();

        Parser subParser(tokens);
        auto   body = subParser.Parse();

        auto   node = CreateRef<CommandSubstitutionNode>();
        node->Body  = body;
        return node;
    }
    if (Match(TokenType::eArithmetic))
    {
        Advance();

        auto node        = CreateRef<ArithmeticNode>();
        node->Expression = t->Text;
        return node;
    }

    if (t->Type != TokenType::eIdentifier && t->Type != TokenType::eString
        && t->Type != TokenType::eGlobWord)
        return nullptr;

    auto word         = CreateRef<WordNode>();
    word->Value       = t->Text;
    word->StartOffset = t->Offset;
    word->EndOffset   = t->Offset + t->Text.Size();

    Advance();
    return word;
}
Ref<ASTNode> Parser::ParseSubshell()
{
    const auto open = Current();
    Advance();

    auto body = ParseSequence();
    if (!Consume(TokenType::eRightParen))
    {
        PrismError("Expected closing ) for subshell", open->Offset);
        return nullptr;
    }

    auto node  = CreateRef<SubshellNode>();
    node->Body = body;
    // node->StartOffset = open->Offset;
    // node->EndOffset   = Current().HasValue() ? Current()->Offset :
    // open->Offset;

    return node;
}
Ref<ASTNode> Parser::ParseBlock()
{
    const auto open = Current();
    Advance();

    auto body = ParseSequence();
    if (!Consume(TokenType::eRightBrace))
    {
        PrismError("Expected closing }} for block", open->Offset);
        return nullptr;
    }

    auto node  = CreateRef<BlockNode>();
    node->Body = body.As<SequenceNode>();
    // node->StartOffset = open->Offset;
    // node->EndOffset   = Current().HasValue() ? Current()->Offset :
    // open->Offset;

    return node;
}
Ref<ASTNode> Parser::ParseAssignment()
{
    auto name = Current();
    Advance();
    Consume(TokenType::eAssign);

    auto value = ParseWord();
    if (!value) return nullptr;

    auto assign         = CreateRef<AssignmentNode>();
    assign->Variable    = name->Text;
    assign->Value       = value;
    assign->StartOffset = name->Offset;
    // assign->EndOffset   = value->EndOffset;

    return assign;
}
Ref<ASTNode> Parser::ParseCommand()
{
    auto cmd      = CreateRef<CommandNode>();
    auto nameWord = ParseWord();

    if (nameWord)
    {
        if (nameWord->Type == NodeType::eWord)
            cmd->Name = nameWord.As<WordNode>()->Value;
        else if (nameWord->Type == NodeType::eVariable)
            cmd->Name = nameWord.As<VariableNode>()->Name;
        else
            PrismError("Unknown node type => {}",
                       StringUtils::ToString(nameWord->Type));
    }

    cmd->Arguments.PushBack(nameWord);
    while (auto word = ParseWord()) cmd->Arguments.PushBack(word);

    ParseRedirections(cmd);
    if (cmd->Arguments.Size() == 0) return nullptr;
    return cmd;
}
Ref<ASTNode> Parser::ParseRedirections(Ref<CommandNode> cmd)
{
    while (Match(TokenType::eLess) || Match(TokenType::eGreater)
           || Match(TokenType::eShiftRight) || Match(TokenType::eShiftLeft)
           || Match(TokenType::eLessAmpersand)
           || Match(TokenType::eGreaterAmpersand))
    {
        auto token = Current();
        Advance();

        auto targetToken = Current();
        Advance();

        auto redir = CreateRef<RedirectionNode>();
        switch (token->Type)
        {
            case TokenType::eLess:
                redir->RedirType = RedirectionNode::Type::Input;
                break;
            case TokenType::eGreater:
                redir->RedirType = RedirectionNode::Type::Output;
                break;
            case TokenType::eShiftLeft:
                redir->RedirType = RedirectionNode::Type::Append;
                break;
            case TokenType::eShiftRight:
                redir->RedirType = RedirectionNode::Type::HereDoc;
                break;
            case TokenType::eLessAmpersand:
                redir->RedirType = RedirectionNode::Type::InputFd;
                break;
            case TokenType::eGreaterAmpersand:
                redir->RedirType = RedirectionNode::Type::OutputFd;
                break;
            default:
                PrismError("Unknown redirection type", token->Offset);
                continue;
        }

        redir->Target = targetToken->Text;
        cmd->Redirections.PushBack(redir);
    }

    return cmd;
}
