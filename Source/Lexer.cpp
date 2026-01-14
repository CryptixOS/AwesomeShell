/*
 * Created by v1tr10l7 on 02.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Lexer.hpp>
#include <Prism/Debug/Log.hpp>

struct OperatorDef
{
    const char* Lexeme;
    TokenType   Type;
};

static constexpr OperatorDef s_Operators[] = {
    {">>&|", TokenType::eShiftRightAmpersandPipe},
    {">>&", TokenType::eShiftRightAmpersand},
    {">>|", TokenType::eShiftRightPipe},
    {">>", TokenType::eShiftRight},
    {"<<<", TokenType::e3Less},
    {"<<-", TokenType::eShiftLeftHyphen},
    {"<<", TokenType::eShiftLeft},
    {"<>", TokenType::eLeftGreater},
    {"<|", TokenType::eLess},
    {"<&", TokenType::eLessAmpersand},
    {"&>|", TokenType::eAmpersandGreaterPipe},
    {"&>", TokenType::eAmpersandGreater},
    {"&|", TokenType::eAmpersandPipe},
    {"|&", TokenType::ePipeAmpersand},
    {"||", TokenType::eDoublePipe},
    {"&&", TokenType::eDoubleAmpersand},
    {";;", TokenType::eDoubleSemi},
    {";|", TokenType::eSemiPipe},
    {";&", TokenType::eSemiAmpersand},
    {"(", TokenType::eLeftParen},
    {")", TokenType::eRightParen},
    {"{", TokenType::eLeftBrace},
    {"}", TokenType::eRightBrace},
    {";", TokenType::eSemicolon},
    {"|", TokenType::ePipe},
    {"&", TokenType::eAmpersand},
    {"<", TokenType::eLess},
    {">", TokenType::eGreater},
    {"=", TokenType::eAssign},
};

static constexpr StringView s_ShellKeywords[]
    = {"if", "then", "else", "elif", "fi", "for",      "while", "until",
       "do", "done", "case", "esac", "in", "function", "select"};
static bool IsKeyword(StringView s)
{
    for (auto kw : s_ShellKeywords)
        if (s == kw) return true;
    return false;
}

Vector<Token>& Lexer::Analyze()
{

    Token tok;
    do
    {
        tok = NextToken();
#if 0
        PrismMessage("Token: {}, Type: {}\n", tok.Text,
                     StringUtils::ToString(tok.Type));
#endif

        m_Tokens.PushBack(tok);
    } while (tok.Type != TokenType::eEndOfFile
             && tok.Type != TokenType::eUnknown);

    return m_Tokens;
}

void Lexer::SkipWhitespace()
{
    while (Peek() == ' ' || Peek() == '\t') Advance();
}

Token Lexer::LexIdentifier()
{
    usize start   = m_CurrentPos;
    bool  hasGlob = false;

    for (;;)
    {
        char c = Peek();

        // Standard word characters
        if (StringUtils::IsAlphanumeric(c) || c == '_' || c == '-' || c == '.'
            || c == '/')
            Advance();
        // Glob characters
        else if (c == '*' || c == '?' || c == '[' || c == ']' || c == '!'
                 || c == '@' || c == '+' || c == '|')
        {
            hasGlob = true;
            Advance();
        }
        // Handle nested parens in globs (e.g., @(a|b))
        else if (hasGlob && (c == '(' || c == ')')) Advance();
        else break;
    }

    String text = m_Input.Substr(start, m_CurrentPos - start);
    return {IsKeyword(text)
                ? TokenType::eKeyword
                : (hasGlob ? TokenType::eGlobWord : TokenType::eIdentifier),
            Move(text), start};
}

Token Lexer::LexWord() { return LexIdentifier(); }

Token Lexer::LexComment()
{
    usize start = m_CurrentPos;
    while (Peek() != '\0' && Peek() != '\n') Advance();
    return {TokenType::eComment, m_Input.Substr(start, m_CurrentPos - start),
            start};
}

bool Lexer::TryMatchOperator(Token& out)
{
    const OperatorDef* best    = nullptr;
    usize              bestLen = 0;

    for (const auto& op : s_Operators)
    {
        usize len = StringUtils::Length(op.Lexeme);
        if (len <= bestLen) continue;

        if (m_Input.Substr(m_CurrentPos, len) == op.Lexeme)
        {
            best    = &op;
            bestLen = len;
        }
    }

    if (!best) return false;

    out = {best->Type, best->Lexeme, m_CurrentPos};
    m_CurrentPos += bestLen;
    return true;
}

Token Lexer::LexSingleQuoteString()
{
    usize start = m_CurrentPos;
    while (Peek() != '\'' && Peek() != '\0') Advance();
    if (Peek() != '\'')
    {
        ReportError(start, "Unterminated single-quoted string");
        m_State = LexerState::eNormal;
        return {TokenType::eUnknown,
                m_Input.Substr(start, m_CurrentPos - start), start};
    }
    String str = m_Input.Substr(start, m_CurrentPos - start);
    Advance();
    m_State = LexerState::eNormal;
    return {TokenType::eString, str, start - 1};
}

Token Lexer::LexDoubleQuoteString()
{
    usize start = m_CurrentPos;
    while (Peek() != '"' && Peek() != '\0')
    {
        if (Peek() == '\\') Advance(2);
        else Advance();
    }
    if (Peek() != '"')
    {
        ReportError(start, "Unterminated double-quoted string");
        m_State = LexerState::eNormal;
        return {TokenType::eUnknown,
                m_Input.Substr(start, m_CurrentPos - start), start};
    }
    String str = m_Input.Substr(start, m_CurrentPos - start);
    Advance();
    m_State = LexerState::eNormal;
    return {TokenType::eString, str, start - 1};
}

Token Lexer::LexVariable()
{
    // Skip the $ part
    Advance();
    usize start = m_CurrentPos;

    if (Peek() == '{')
    {
        Advance();
        while (StringUtils::IsAlphanumeric(Peek()) || Peek() == '_'
               || Peek() == ':' || Peek() == '-' || Peek() == '?'
               || Peek() == '/')
            Advance();
        if (Peek() != '}')
            ReportError(start, "Unterminated variable expansion");
        else Advance();
    }
    else if (Peek() == '?') Advance();
    else
        while (StringUtils::IsAlphanumeric(Peek()) || Peek() == '_') Advance();

    return {TokenType::eVariable, m_Input.Substr(start, m_CurrentPos - start),
            start};
}

Token Lexer::LexCommandSubstitution()
{
    usize tokenStart   = m_CurrentPos - 2; // $(
    usize contentStart = m_CurrentPos;
    i32   depth        = 1;
    while (depth > 0 && Peek() != '\0')
    {
        if (Peek() == '(' && m_CurrentPos > 0
            && m_Input[m_CurrentPos - 1] == '$')
            ++depth;
        else if (Peek() == ')') --depth;
        Advance();
    }
    if (depth != 0)
        ReportError(contentStart, "Unterminated command substitution");
    String text = m_Input.Substr(contentStart, m_CurrentPos - contentStart
                                                   - (depth != 0 ? 0 : 1));
    m_State     = LexerState::eNormal;
    return {TokenType::eCommandSubst, Move(text), tokenStart};
}

Token Lexer::LexBacktick()
{
    usize start = m_CurrentPos;
    while (Peek() != '`' && Peek() != '\0') Advance();
    if (Peek() != '`')
        ReportError(start, "Unterminated backtick command substitution");
    else Advance();
    m_State = LexerState::eNormal;
    return {TokenType::eCommandSubst,
            m_Input.Substr(start, m_CurrentPos - start), start - 1};
}

Token Lexer::LexArithmetic()
{
    usize tokenStart   = m_CurrentPos - 3; // $((
    usize contentStart = m_CurrentPos;
    i32   depth        = 1;

    while (depth > 0 && Peek() != '\0')
    {
        if (Peek() == '(' && m_CurrentPos > 1
            && m_Input[m_CurrentPos - 2] == '$')
            ++depth;
        else if (Peek() == ')') --depth;
        Advance();
    }

    if (depth != 0)
        ReportError(tokenStart, "Unterminated arithmetic expression");
    String text = m_Input.Substr(contentStart,
                                 m_CurrentPos - contentStart - (depth == 0));
    Advance(); // Consume the final ')'

    m_State = LexerState::eNormal;
    return {TokenType::eArithmetic, Move(text), tokenStart};
}

Token Lexer::LexHereDoc(String delimiter, bool allowExpansion)
{
    IgnoreUnused(allowExpansion);
    usize  start = m_CurrentPos;
    String content;
    while (m_CurrentPos < m_Input.Size())
    {
        String line;
        while (Peek() != '\n' && Peek() != '\0')
        {
            line += Peek();
            Advance();
        }
        if (Peek() == '\n')
        {
            Advance();
            line += '\n';
        }

        if (line.Trim() == delimiter)
            return {TokenType::eHereDoc, content, start};
        content += line;
    }
    ReportError(start, "Unterminated here-document");
    return {TokenType::eUnknown, content, start};
}

// -------------------- NextToken --------------------
Token Lexer::NextToken()
{
    SkipWhitespace();
    if (m_CurrentPos >= m_Input.Size())
        return {TokenType::eEndOfFile, "", m_CurrentPos};

    Token tok;
    switch (m_State)
    {
        case LexerState::eNormal:
        {
            if (Peek() == '\\' && PeekNext() == '\n')
            {
                Advance(2);
                return NextToken();
            }
            if (Peek() == '\n')
            {
                Advance();
                return {TokenType::eNewLine, "\n", m_CurrentPos - 1};
            }
            if (Peek() == '#') return LexComment();
            if (Peek() == '\'')
            {
                Advance();
                m_State = LexerState::eSingleQuote;
                return LexSingleQuoteString();
            }
            if (Peek() == '"')
            {
                Advance();
                m_State = LexerState::eDoubleQuote;
                return LexDoubleQuoteString();
            }
            if (Peek() == '$' && PeekNext() == '(')
            {
                if (m_Input[m_CurrentPos + 2] == '(')
                {
                    Advance(3);
                    m_State = LexerState::eArithmetic;
                    return LexArithmetic();
                }
                Advance(2);
                m_State = LexerState::eDollarParen;
                return LexCommandSubstitution();
            }
            if (Peek() == '`')
            {
                Advance();
                m_State = LexerState::eBacktick;
                return LexBacktick();
            }
            if (Peek() == '$') return LexVariable();

            Token op;
            if (TryMatchOperator(op))
            {
                if (op.Type == TokenType::eShiftLeft
                    || op.Type == TokenType::eShiftLeftHyphen)
                {
                    SkipWhitespace();
                    usize start = m_CurrentPos;

                    while (!StringUtils::IsSpace(Peek()) && Peek() != '\0')
                        Advance();

#if 0
                    bool quoted = false;
                    if ((m_Input[start] == '\''
                         && m_Input[m_CurrentPos - 1] == '\'')
                        || (m_Input[start] == '"'
                            && m_Input[m_CurrentPos - 1] == '"'))
                        quoted = true;

                    String delimiter = m_Input.Substr(
                        start + quoted, m_CurrentPos - start - quoted);

                    return LexHereDoc(delimiter);
#else
                    String delimiter
                        = m_Input.Substr(start, m_CurrentPos - start);
                    RegisterHereDoc(Move(delimiter),
                                    op.Type == TokenType::eShiftLeft);
#endif
                }
                return op;
            }
            if (IsWordStart(Peek())) return LexWord();
            // Look ahead: if it's !( or @(, it's definitely a glob word
            if ((Peek() == '@' || Peek() == '!') && PeekNext() == '(')
                return LexWord();
            if (Peek() == '(' || Peek() == ')') return LexWord();
            if (Peek() == '{')
            {
                Advance();
                return {TokenType::eBraceOpen, "{", m_CurrentPos - 1};
            }
            if (Peek() == '}')
            {
                Advance();
                return {TokenType::eBraceClose, "}", m_CurrentPos - 1};
            }
            if (Peek() == ',')
            {
                Advance();
                return {TokenType::eComma, ",", m_CurrentPos - 1};
            }
            Advance();
            return {TokenType::eUnknown, String(1, m_Input[m_CurrentPos - 1]),
                    m_CurrentPos - 1};
        }

        case LexerState::eSingleQuote: return LexSingleQuoteString();
        case LexerState::eDoubleQuote: return LexDoubleQuoteString();
        case LexerState::eDollarParen: return LexCommandSubstitution();
        case LexerState::eBacktick: return LexBacktick();
        case LexerState::eArithmetic: return LexArithmetic();
        case LexerState::eHereDoc: return LexHereDoc("EOF");
        default: return {TokenType::eUnknown, "", m_CurrentPos};
    }

    return tok;
}

void Lexer::RegisterHereDoc(StringView delimiter, bool allowExpansion)
{
    m_PendingHereDocs.PushBack({
        .Delimiter      = Move(delimiter),
        .AllowExpansion = allowExpansion,
    });
}
Token Lexer::ConsumeHereDoc(const PendingHereDoc& hd)
{
    usize  start = m_CurrentPos;
    String content;

    while (m_CurrentPos < m_Input.Size())
    {
        String line;
        while (Peek() != '\n' && Peek() != '\0')
        {
            line += Peek();
            Advance();
        }

        if (Peek() == '\n')
        {
            Advance();
            line += '\n';
        }

        if (line.Trim() == hd.Delimiter)
            return {TokenType::eHereDoc, Move(content), start};

        content += line;
    }

    ReportError(start, "Unterminated here-document");
    return {TokenType::eUnknown, Move(content), start};
}

void Lexer::ReportError(usize line, StringView message)
{
    if (m_LogErrors) PrismError("{}: {}\n", line, message);
}
