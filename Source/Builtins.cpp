/*
 * Created by v1tr10l7 on 14.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Builtins.hpp>
#include <Environment.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/String/StringUtils.hpp>

using namespace Prism;

namespace Builtins
{
    using BuiltinProc = isize (*)(BuiltinArgs args);
    namespace
    {
        UnorderedMap<String, BuiltinProc> s_Builtins;
        isize                             Exit(BuiltinArgs args)
        {
            isize status
                = args.Empty() ? 0 : StringUtils::ToNumber<i32>(args[0]);
            exit(status);
        }
        isize ChangeDirectory(BuiltinArgs args)
        {
            String target = Environment::GetVariable("PATH");
            if (args.Size() > 1)
            {
                if (args.Size() > 3)
                {
                    PrismWarn("awsh: cd invalid number of arguments");
                    for (usize i = 0; auto arg : args)
                        PrismWarn("arg[{}]='{}'", i++, arg);
                    return 1;
                }
                target = args[1];
            }

            return chdir(target.Raw());
        }
    }; // namespace

    void Initialize()
    {
        // Register builtins
        s_Builtins["cd"]   = ChangeDirectory;
        s_Builtins["exit"] = Exit;
    }
    Optional<isize> TryRun(StringView name, BuiltinArgs args)
    {
        auto builtin = s_Builtins.Find(String(name));
        if (builtin == s_Builtins.end()) return NullOpt;

        auto func = *builtin->Value;
        return func(args);
    }
}; // namespace Builtins
