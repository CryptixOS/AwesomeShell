/*
 * Created by v1tr10l7 on 14.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Optional.hpp>

namespace Builtins
{
    using BuiltinArgs = const Vector<char*>&;

    void            Initialize();
    Optional<isize> TryRun(StringView name, BuiltinArgs args);
}; // namespace Builtins
