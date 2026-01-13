/*
 * Created by v1tr10l7 on 13.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Environment.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/String/String.hpp>

using namespace Prism;

namespace Environment
{
    static UnorderedMap<String, String> s_Environment;

    StringView GetVariable(StringView name) { return s_Environment[name]; }
    void       SetVariable(StringView name, StringView value)
    {
        s_Environment[name] = String(value);
    }
}; // namespace Environment
