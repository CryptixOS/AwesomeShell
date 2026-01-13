/*
 * Created by v1tr10l7 on 12.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>

#include <Prism/String/StringView.hpp>
#include <Prism/Utility/PathView.hpp>

namespace Shell
{
    enum class TestMode
    {
        eNone        = 0,
        eLexer       = 1,
        eParser      = 2,
        eLexerParser = 3,
        eExecutor    = 4,
        eAll         = 7,
    };
    inline constexpr TestMode operator|(const TestMode& lhs, const TestMode rhs)
    {
        return static_cast<TestMode>(ToUnderlying(lhs) | ToUnderlying(rhs));
    }
    inline constexpr bool operator&(const TestMode& lhs, const TestMode rhs)
    {
        return ToUnderlying(lhs) & ToUnderlying(rhs);
    }

    void          Initialize(const Vector<StringView>& envp);
    ErrorOr<void> Run();

    void          EnableRestricted();
    void          EnablePosixMode();
    void          EnableVerbose();
    void          EnableTesting(TestMode mode);

    ErrorOr<void> RunCommand(StringView command);
    ErrorOr<void> RunFile(PathView path);
}; // namespace Shell
