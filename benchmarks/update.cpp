//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "benchmark/benchmark.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/Property.h"

#include "impl/LogicEngineImpl.h"
#include "fmt/format.h"

namespace rlogic
{
    static void BM_Update_AssignProperty(benchmark::State& state)
    {
        LogicEngine logicEngine;

        const int64_t loopCount = state.range(0);

        const std::string scriptSrc = fmt::format(R"(
            function interface()
                IN.param = INT
                OUT.param = INT
            end
            function run()
                for i = 0,{},1 do
                    OUT.param = IN.param
                end
            end
        )", loopCount);

        logicEngine.createLuaScriptFromSource(scriptSrc);

        for (auto _ : state) // NOLINT(clang-analyzer-deadcode.DeadStores) False positive
        {
            logicEngine.m_impl->update(true);
        }
    }

    // Measures update() speed based on how many properties are assigned in the script's run() method
    // Dirty handling: off
    // ARG: how many assignments are made in run() to the same property
    BENCHMARK(BM_Update_AssignProperty)->Arg(1)->Arg(10)->Arg(100)->Arg(1000);

    static void BM_Update_AssignStruct(benchmark::State& state)
    {
        LogicEngine logicEngine;

        const int64_t loopCount = state.range(0);

        const std::string scriptSrc = fmt::format(R"(
            function interface()
                IN.struct = {{
                    int = INT,
                    float = FLOAT,
                    vec4f = VEC4F,
                    nested = {{
                        int = INT,
                        float = FLOAT,
                        vec4f = VEC4F
                    }}
                }}
                OUT.struct = {{
                    int = INT,
                    float = FLOAT,
                    vec4f = VEC4F,
                    nested = {{
                        int = INT,
                        float = FLOAT,
                        vec4f = VEC4F
                    }}
                }}
            end
            function run()
                for i = 0,{},1 do
                    OUT.struct = IN.struct
                end
            end
        )", loopCount);

        logicEngine.createLuaScriptFromSource(scriptSrc);

        for (auto _ : state) // NOLINT(clang-analyzer-deadcode.DeadStores) False positive
        {
            logicEngine.m_impl->update(true);
        }
    }

    // Same as BM_Update_AssignProperty, but with structs
    BENCHMARK(BM_Update_AssignStruct)->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);

    static void BM_Update_AssignArray(benchmark::State& state)
    {
        LogicEngine logicEngine;

        const int64_t loopCount = state.range(0);

        const std::string scriptSrc = fmt::format(R"(
            function interface()
                IN.array = ARRAY(255, VEC4F)
                OUT.array = ARRAY(255, VEC4F)
            end
            function run()
                for i = 0,{},1 do
                    OUT.array = IN.array
                end
            end
        )", loopCount);

        logicEngine.createLuaScriptFromSource(scriptSrc);

        for (auto _ : state) // NOLINT(clang-analyzer-deadcode.DeadStores) False positive
        {
            logicEngine.m_impl->update(true);
        }
    }

    // Same as BM_Update_AssignProperty, but with arrays
    BENCHMARK(BM_Update_AssignArray)->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
}


