/*
 * Created by v1tr10l7 on 02.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/String/String.hpp>

using namespace Prism;

enum class TokenType : isize
{
    eUnknown                 = -1,
    eSemicolon               = 1,  // ;
    eAmpersand               = 2,  // &
    ePipe                    = 3,  // |
    eLeftParen               = 4,  // (
    eRightParen              = 5,  // )
    eLeftBrace               = 6,  // {
    eRightBrace              = 7,  // }
    eDoubleSemi              = 8,  // ;;
    eDoublePipe              = 9,  // ||
    eDoubleAmpersand         = 10, // &&
    eLess                    = 11, // <
    eGreater                 = 12, // >
    eGreaterPipe             = 13, // >|
    eShiftLeft               = 14, // <<
    eShiftRight              = 15, // >>
    eLeftRightParen          = 16, // ()
    ePipeAmpersand           = 17, // |&
    eShiftRightPipe          = 18, // >>|
    eLeftGreater             = 19, // <>
    eShiftLeftHyphen         = 20, // <<-
    eLessAmpersand           = 21, // <&
    eGreaterAmpersand        = 22, // >&
    eAmpersandGreater        = 23, // &>
    eAmpersandGreaterPipe    = 24, // &>|
    eShiftRightAmpersand     = 25, // >>&
    eShiftRightAmpersandPipe = 26, // >>&|
    e3Less                   = 27, // <<<
    e2LeftParent             = 28, // ((
    e2RightParent            = 29, // ))
    eAmpersandPipe           = 30, // &|
    eSemiAmpersand           = 31, // ;&
    eSemiPipe                = 32, // ;|
    eNewLine                 = 33, // \n
    eAssign                  = 34, // =
    eIdentifier              = 35,
    eComment                 = 36,
    eEndOfFile               = 37,
    eString                  = 38,
    eVariable                = 39,
    eCommandSubst            = 40,
    eHereDoc                 = 41,
    eArithmetic              = 42,
    eKeyword                 = 43,
    eBraceOpen               = 44, // {
    eBraceClose              = 45, // }
    eComma                   = 46, // ,
    eGlobWord                = 47, // word containing *, ?, or [...]
};

struct Token
{
    TokenType Type;
    String    Text;
    usize     Offset;
};
