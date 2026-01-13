/*
 * Created by v1tr10l7 on 13.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/String/StringView.hpp>

namespace Environment
{
    StringView GetVariable(StringView name);
    void       SetVariable(StringView name, StringView value);
}; // namespace Environment
