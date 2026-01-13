/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <AST.hpp>
#include <Token.hpp>

class Parser
{
  public:
    Parser(Vector<Token>& tokens)
        : m_Tokens(tokens)
    {
    }

    Ref<ASTNode> Parse(); // entry point

  private:
    Vector<Token>&   m_Tokens;
    usize            m_Pos = 0;

    Optional<Token&> Current()
    {
        if (m_Pos >= m_Tokens.Size()) return NullOpt;
        return m_Tokens[m_Pos];
    }
    Optional<Token&> Peek(usize offset = 1)
    {
        usize pos = m_Pos + offset;
        if (pos >= m_Tokens.Size()) return NullOpt;
        return m_Tokens[pos];
    }

    Optional<Token&> Next()
    {
        if (m_Pos < m_Tokens.Size()) m_Pos++;
        return Current();
    }
    PM_ALWAYS_INLINE void Advance(usize i = 1)
    {
        if (m_Pos + i < m_Tokens.Size()) m_Pos += i;
        else m_Pos = m_Tokens.Size() + 1;
    }

    bool Match(TokenType t)
    {
        auto current = Current();
        return current.HasValue() && current->Type == t;
    }
    bool MatchAny(const Token& token, InitializerList<TokenType> types)
    {
        for (auto type : types)
            if (token.Type == type) return true;
        return false;
    }
    inline bool Consume(TokenType type)
    {
        if (m_Pos >= m_Tokens.Size()) return false;
        if (Match(type))
        {
            Advance();
            return true;
        }
        return false;
    }

    inline bool End()
    {
        return m_Pos >= m_Tokens.Size()
            || m_Tokens[m_Pos].Type == TokenType::eEndOfFile;
    }

    inline bool IsAssignment()
    {
        auto id = Current();
        auto eq = Peek();

        return id.HasValue() && eq.HasValue()
            && id->Type == TokenType::eIdentifier
            && eq->Type == TokenType::eAssign;
    }

    Ref<ASTNode> ParseSequence();
    Ref<ASTNode> ParseConditional();
    Ref<ASTNode> ParsePipeline();
    Ref<ASTNode> ParseStatement();
    Ref<ASTNode> ParseWord();
    Ref<ASTNode> ParseSubshell();
    Ref<ASTNode> ParseBlock();
    Ref<ASTNode> ParseAssignment();
    Ref<ASTNode> ParseCommand();
    Ref<ASTNode> ParseRedirections(Ref<CommandNode> cmd);
    Ref<ASTNode> ParseHereDoc();
};
