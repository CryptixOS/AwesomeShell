/*
 * Created by v1tr10l7 on 02.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>
#include <Token.hpp>

class Lexer
{
  public:
    Lexer(Vector<StringView>& args)
        : m_Args(args)
    {
    }
    ErrorOr<Vector<Token>> Analyze();

  private:
    Vector<StringView>& m_Args;
};
