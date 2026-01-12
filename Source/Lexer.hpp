/*
 * Created by v1tr10l7 on 02.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/String/StringView.hpp>
#include <Token.hpp>

using namespace Prism;
using namespace Literals;

enum class LexerState
{
    eNormal,      // Default
    eSingleQuote, // Inside '
    eDoubleQuote, // Inside "
    eDollarParen, // $(...)
    eBacktick,    // `...`
    eHereDoc,     // <<EOF ... EOF
    eArithmetic,  // $(( ... ))
};

class Lexer
{
  public:
    Lexer(StringView input, bool printErrors = true)
        : m_Input(input)
        , m_LogErrors(printErrors)
    {
    }
    Lexer(Vector<StringView>& args, bool printErrors = true)
        : m_LogErrors(printErrors)
    {
        for (auto arg : args)
        {
            m_Input += arg;
            m_Input += ' ';
        }
    }
    Vector<Token>& Analyze();

  private:
    Vector<Token> m_Tokens;
    String        m_Input      = ""_s;
    bool          m_LogErrors  = false;
    usize         m_CurrentPos = 0;
    LexerState    m_State      = LexerState::eNormal;

    struct PendingHereDoc
    {
        String Delimiter;
        bool   AllowExpansion;
    };

    Vector<PendingHereDoc> m_PendingHereDocs;

    Token                  NextToken();
    u8                     Peek() const
    {
        return m_CurrentPos < m_Input.Size() ? m_Input[m_CurrentPos] : '\0';
    }
    u8 PeekNext() const
    {
        return m_CurrentPos + 1 < m_Input.Size() ? m_Input[m_CurrentPos + 1]
                                                 : '\0';
    }
    void                  Advance(usize n = 1) { m_CurrentPos += n; }
    void                  SkipWhitespace();

    inline constexpr bool IsWordStart(u8 c) const
    {
        return StringUtils::IsAlphanumeric(c) || c == '_' || c == '/'
            || c == '-' || c == '.' || c == '*' || c == '[' || c == ']'
            || c == '!' || c == '@' || c == '+' || c == '?'; // Added @, +, ?
    }
    Token LexWord();
    bool  TryMatchOperatorPeek();
    Token LexIdentifier();
    Token LexComment();
    Token LexSingleQuoteString();
    Token LexDoubleQuoteString();
    Token LexVariable();
    Token LexCommandSubstitution();
    Token LexBacktick();
    Token LexArithmetic();
    Token LexHereDoc(String delimiter, bool allowExpansion = true);
    bool  TryMatchOperator(Token& out);

    void  RegisterHereDoc(StringView delimiter, bool allowExpansion);
    Token ConsumeHereDoc(const PendingHereDoc& hd);

    void  ReportError(usize line, StringView message);
};
