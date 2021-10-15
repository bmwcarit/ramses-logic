//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "WithTempDirectory.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "impl/LuaScriptImpl.h"
#include <fstream>

using namespace testing;

namespace rlogic::internal
{
    class ALuaScriptWithModule : public ::testing::Test
    {
    protected:
        const std::string_view m_moduleSourceCode = R"(
            local mymath = {}
            function mymath.add(a,b)
                return a+b
            end
            mymath.PI=3.1415
            return mymath
        )";
        const std::string_view m_moduleSourceCode2 = R"(
            local myothermath = {}
            function myothermath.sub(a,b)
                return a-b
            end
            function myothermath.colorType()
                return {
                    red = INT,
                    blue = INT,
                    green = INT
                }
            end
            myothermath.color = {
                red = 255,
                green = 128,
                blue = 72
            }
            return myothermath
        )";

        LuaConfig createDeps(const std::vector<std::pair<std::string_view, std::string_view>>& dependencies)
        {
            LuaConfig config;
            for (const auto& [alias, moduleSrc] : dependencies)
            {
                LuaModule* mod = m_logicEngine.createLuaModule(moduleSrc);
                config.addDependency(alias, *mod);
            }

            return config;
        }

        static LuaConfig WithStdMath()
        {
            LuaConfig config;
            config.addStandardModuleDependency(EStandardModule::Math);
            return config;
        }

        LogicEngine m_logicEngine;
    };

    TEST_F(ALuaScriptWithModule, CanBeCreated)
    {
        LuaConfig config;
        LuaModule* mod = m_logicEngine.createLuaModule(m_moduleSourceCode);
        config.addDependency("mymath", *mod);

        const auto script = m_logicEngine.createLuaScript(R"(
            modules("mymath")

            function interface()
                OUT.v = INT
                OUT.pi = FLOAT
            end

            function run()
                OUT.v = mymath.add(1,2)
                OUT.pi = mymath.PI
            end
        )", config);
        ASSERT_NE(nullptr, script);
        EXPECT_THAT(script->m_script.getModules(), ElementsAre(Pair("mymath", mod)));

        m_logicEngine.update();
        EXPECT_EQ(3, *script->getOutputs()->getChild("v")->get<int32_t>());
        EXPECT_FLOAT_EQ(3.1415f, *script->getOutputs()->getChild("pi")->get<float>());
    }

    TEST_F(ALuaScriptWithModule, UsesModuleUnderDifferentName)
    {
        const auto script = m_logicEngine.createLuaScript(R"(
            modules("mymodule")

            function interface()
                OUT.v = INT
                OUT.pi = FLOAT
            end

            function run()
                OUT.v = mymodule.add(1,2)
                OUT.pi = mymodule.PI
            end
        )", createDeps({ { "mymodule", m_moduleSourceCode } }));
        ASSERT_NE(nullptr, script);

        m_logicEngine.update();
        EXPECT_EQ(3, *script->getOutputs()->getChild("v")->get<int32_t>());
        EXPECT_FLOAT_EQ(3.1415f, *script->getOutputs()->getChild("pi")->get<float>());
    }

    TEST_F(ALuaScriptWithModule, MultipleModules)
    {
        const auto script = m_logicEngine.createLuaScript(R"(
            modules("mymath", "mymath2")

            function interface()
                OUT.v = INT
            end

            function run()
                OUT.v = mymath.add(1,2) + mymath2.sub(20,10)
            end
        )", createDeps({ { "mymath", m_moduleSourceCode }, { "mymath2", m_moduleSourceCode2 } }));

        m_logicEngine.update();
        EXPECT_EQ(13, *script->getOutputs()->getChild("v")->get<int32_t>());
    }

    TEST_F(ALuaScriptWithModule, UsesSameModuleUnderMultipleNames)
    {
        const auto module = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "mymathmodule");
        ASSERT_NE(nullptr, module);

        LuaConfig config;
        config.addDependency("mymath", *module);
        config.addDependency("mymath2", *module);

        const auto script = m_logicEngine.createLuaScript(R"(
            modules("mymath", "mymath2")

            function interface()
                OUT.v = INT
            end

            function run()
                OUT.v = mymath.add(1,2) + mymath2.add(20,10)
            end
        )", config);
        ASSERT_NE(nullptr, script);

        m_logicEngine.update();
        EXPECT_EQ(33, *script->getOutputs()->getChild("v")->get<int32_t>());
    }

    TEST_F(ALuaScriptWithModule, TwoScriptsUseSameModule)
    {
        const auto module = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "mymathmodule");
        ASSERT_NE(nullptr, module);

        LuaConfig config1;
        config1.addDependency("mymath", *module);

        const auto script1 = m_logicEngine.createLuaScript(R"(
            modules("mymath")

            function interface()
                OUT.v = INT
            end

            function run()
                OUT.v = mymath.add(1,2)
            end
        )", config1);
        ASSERT_NE(nullptr, script1);

        LuaConfig config2;
        config2.addDependency("mymathother", *module);

        const auto script2 = m_logicEngine.createLuaScript(R"(
            modules("mymathother")

            function interface()
                OUT.v = INT
            end

            function run()
                OUT.v = mymathother.add(10,20)
            end
        )", config2);
        ASSERT_NE(nullptr, script1);

        m_logicEngine.update();
        EXPECT_EQ(3, *script1->getOutputs()->getChild("v")->get<int32_t>());
        EXPECT_EQ(30, *script2->getOutputs()->getChild("v")->get<int32_t>());
    }

    TEST_F(ALuaScriptWithModule, ErrorIfModuleDoesNotReturnTable)
    {
        std::vector<std::string> errorCases = {
            "return nil",
            "return IN",
            "return OUT",
            "return 5",
            "return \"TheModule\"",
            "return false",
            "return true",
            "return print",
        };

        for (const auto& moduleSrc : errorCases)
        {
            LuaModule* luaModule = m_logicEngine.createLuaModule(moduleSrc, {}, "mod");
            EXPECT_EQ(nullptr, luaModule);

            EXPECT_FALSE(m_logicEngine.getErrors().empty());
            EXPECT_EQ("[mod] Error while loading module. Module script must return a table!", m_logicEngine.getErrors()[0].message);
        }

    }

    TEST_F(ALuaScriptWithModule, CanUseTableDataAndItsTypeDefinitionFromModule)
    {
        const auto script = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                OUT.color = mymath.colorType()
            end
            function run()
                OUT.color = mymath.color
            end
        )", createDeps({ { "mymath", m_moduleSourceCode2 } }));
        ASSERT_TRUE(script);

        m_logicEngine.update();
        const Property* colorOutput = script->getOutputs()->getChild("color");
        ASSERT_TRUE(colorOutput && colorOutput->getChild("red") && colorOutput->getChild("green") && colorOutput->getChild("blue"));
        EXPECT_EQ(255, *colorOutput->getChild("red")->get<int32_t>());
        EXPECT_EQ(128, *colorOutput->getChild("green")->get<int32_t>());
        EXPECT_EQ(72, *colorOutput->getChild("blue")->get<int32_t>());
    }

    TEST_F(ALuaScriptWithModule, UsesModuleThatDependsOnAnotherModule)
    {
        const std::string_view wrappedModuleSrc = R"(
            modules("mymath")
            local wrapped = {}
            function wrapped.add(a,b)
                return mymath.add(a, b) + 5
            end
            return wrapped
        )";

        const auto wrapped = m_logicEngine.createLuaModule(wrappedModuleSrc, createDeps({ { "mymath", m_moduleSourceCode } }));

        LuaConfig config;
        config.addDependency("wrapped", *wrapped);

        const auto script = m_logicEngine.createLuaScript(R"(
            modules("wrapped")
            function interface()
                OUT.result = INT
            end
            function run()
                OUT.result = wrapped.add(10, 20)
            end
        )", config);

        EXPECT_TRUE(m_logicEngine.update());
        const Property* result = script->getOutputs()->getChild("result");
        ASSERT_TRUE(result);
        EXPECT_EQ(35, *result->get<int32_t>());
    }

    TEST_F(ALuaScriptWithModule, SecondLevelDependenciesAreHidden)
    {
        const std::string_view wrappedModuleSrc = R"(
            modules("mymath")
            local wrapped = {}
            function wrapped.add(a,b)
                return a + b + 100
            end
            wrapped.PI=42
            return wrapped
        )";

        const auto wrapped = m_logicEngine.createLuaModule(wrappedModuleSrc, createDeps({{"mymath", m_moduleSourceCode}}));

        LuaConfig config;
        config.addDependency("wrapped", *wrapped);

        const auto script = m_logicEngine.createLuaScript(R"(
            modules("wrapped")
            function interface()
                OUT.add = INT
                OUT.PI = FLOAT
            end
            function run()
                -- This tests that the indirect dependency is correctly hidden
                if mymath ~= nil then
                    error("If this error happens, mymath module is not properly wrapped!")
                end
                OUT.add = wrapped.add(10, 20)
                OUT.PI = wrapped.PI
            end
        )", config);

        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(130, *script->getOutputs()->getChild("add")->get<int32_t>());
        EXPECT_FLOAT_EQ(42.f, *script->getOutputs()->getChild("PI")->get<float>());
    }

    TEST_F(ALuaScriptWithModule, ReloadsModuleUsingTheSameNameCausesItToBeRecompiled)
    {
        const std::string_view moduleSource = R"(
            local mymath = {}
            mymath.pi=3.1415
            return mymath
        )";

        const std::string_view moduleSource_Modified = R"(
            local mymath = {}
            mymath.pi=4
            return mymath
        )";

        const std::string_view scriptSrc = R"(
            modules("module")
            function interface()
                OUT.pi = FLOAT
            end
            function run()
                OUT.pi = module.pi
            end
        )";

        auto module = m_logicEngine.createLuaModule(moduleSource, {}, "module");

        LuaConfig config;
        config.addDependency("module", *module);
        auto script = m_logicEngine.createLuaScript(scriptSrc, config);

        ASSERT_TRUE(m_logicEngine.update());
        const Property* colorOutput = script->getOutputs()->getChild("pi");
        EXPECT_FLOAT_EQ(3.1415f, *colorOutput->get<float>());

        ASSERT_TRUE(m_logicEngine.destroy(*script));
        ASSERT_TRUE(m_logicEngine.destroy(*module));
        module = m_logicEngine.createLuaModule(moduleSource_Modified, {}, "module");

        config = {};
        config.addDependency("module", *module);
        script = m_logicEngine.createLuaScript(scriptSrc, config);

        ASSERT_TRUE(m_logicEngine.update());
        colorOutput = script->getOutputs()->getChild("pi");
        EXPECT_FLOAT_EQ(4.0f, *colorOutput->get<float>());
    }

    TEST_F(ALuaScriptWithModule, CanBeSerialized)
    {
        WithTempDirectory tempDir;

        {
            LogicEngine logic;
            // 2 scripts, one module used by first script, other module used by both scripts
            const auto module1 = logic.createLuaModule(m_moduleSourceCode, {}, "mymodule1");
            const auto module2 = logic.createLuaModule(m_moduleSourceCode2, {}, "mymodule2");
            ASSERT_TRUE(module1 && module2);

            LuaConfig config1;
            config1.addDependency("mymath", *module1);
            config1.addDependency("mymathother", *module2);

            LuaConfig config2;
            config2.addDependency("mymath", *module2);

            logic.createLuaScript(R"(
                modules("mymath", "mymathother")
                function interface()
                    OUT.v = INT
                    OUT.color = mymathother.colorType()
                end
                function run()
                    OUT.v = mymath.add(1,2) + mymathother.sub(60,30)
                    OUT.color = mymathother.color
                end
            )", config1, "script1");
            logic.createLuaScript(R"(
                modules("mymath")
                function interface()
                    OUT.v = INT
                end
                function run()
                    OUT.v = mymath.sub(90,60)
                end
            )", config2, "script2");

            EXPECT_TRUE(logic.saveToFile("scriptmodules.tmp"));
        }

        EXPECT_TRUE(m_logicEngine.loadFromFile("scriptmodules.tmp"));
        m_logicEngine.update();

        const auto module1 = m_logicEngine.findLuaModule("mymodule1");
        const auto module2 = m_logicEngine.findLuaModule("mymodule2");
        const auto script1 = m_logicEngine.findScript("script1");
        const auto script2 = m_logicEngine.findScript("script2");
        ASSERT_TRUE(module1 && module2 && script1 && script2);
        EXPECT_THAT(script1->m_script.getModules(), UnorderedElementsAre(Pair("mymath", module1), Pair("mymathother", module2)));
        EXPECT_THAT(script2->m_script.getModules(), UnorderedElementsAre(Pair("mymath", module2)));

        m_logicEngine.update();
        EXPECT_EQ(33, *script1->getOutputs()->getChild("v")->get<int32_t>());
        const Property* colorOutput = script1->getOutputs()->getChild("color");
        ASSERT_TRUE(colorOutput && colorOutput->getChild("red") && colorOutput->getChild("green") && colorOutput->getChild("blue"));
        EXPECT_EQ(255, *colorOutput->getChild("red")->get<int32_t>());
        EXPECT_EQ(128, *colorOutput->getChild("green")->get<int32_t>());
        EXPECT_EQ(72, *colorOutput->getChild("blue")->get<int32_t>());

        EXPECT_EQ(30, *script2->getOutputs()->getChild("v")->get<int32_t>());
    }

    TEST_F(ALuaScriptWithModule, UsesStructPropertyInInterfaceDefinedInModule)
    {
        const std::string_view moduleDefiningInterfaceType = R"(
            local mytypes = {}
            function mytypes.mystruct()
                return {
                    name = STRING,
                    address =
                    {
                        street = STRING,
                        number = INT
                    }
                }
            end
            return mytypes
        )";

        const auto script = m_logicEngine.createLuaScript(R"(
            modules("mytypes")
            function interface()
                IN.struct = mytypes.mystruct()
                OUT.struct = mytypes.mystruct()
            end

            function run()
                OUT.struct = IN.struct
            end
        )", createDeps({ { "mytypes", moduleDefiningInterfaceType } }));
        ASSERT_NE(nullptr, script);

        for (auto rootProp : std::initializer_list<const Property*>{ script->getInputs(), script->getOutputs() })
        {
            ASSERT_EQ(1u, rootProp->getChildCount());
            const auto structChild = rootProp->getChild(0);

            EXPECT_EQ("struct", structChild->getName());
            EXPECT_EQ(EPropertyType::Struct, structChild->getType());
            ASSERT_EQ(2u, structChild->getChildCount());
            const auto name = structChild->getChild("name");
            EXPECT_EQ(EPropertyType::String, name->getType());
            const auto address = structChild->getChild("address");
            ASSERT_EQ(2u, address->getChildCount());
            EXPECT_EQ(EPropertyType::Struct, address->getType());
            const auto addressStr = address->getChild("street");
            const auto addressNr = address->getChild("number");
            EXPECT_EQ(EPropertyType::String, addressStr->getType());
            EXPECT_EQ(EPropertyType::Int32, addressNr->getType());
        }
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScriptWithModule, UsesStructPropertyInInterfaceDefinedInModule_UseInArray)
    {
        const std::string_view moduleDefiningInterfaceType = R"(
            local mytypes = {}
            function mytypes.mystruct()
                return {
                    name = STRING,
                    address =
                    {
                        street = STRING,
                        number = INT
                    }
                }
            end
            return mytypes
        )";

        const auto script = m_logicEngine.createLuaScript(R"(
            modules("mytypes")
            function interface()
                IN.array_of_structs = ARRAY(2, mytypes.mystruct())
                OUT.array_of_structs = ARRAY(2, mytypes.mystruct())
            end

            function run()
                OUT.array_of_structs = IN.array_of_structs
            end
        )", createDeps({ { "mytypes", moduleDefiningInterfaceType } }));
        ASSERT_NE(nullptr, script);

        for (auto rootProp : std::initializer_list<const Property*>{ script->getInputs(), script->getOutputs() })
        {
            ASSERT_EQ(1u, rootProp->getChildCount());
            const auto arrayOfStructs = rootProp->getChild(0);

            EXPECT_EQ("array_of_structs", arrayOfStructs->getName());
            EXPECT_EQ(EPropertyType::Array, arrayOfStructs->getType());
            ASSERT_EQ(2u, arrayOfStructs->getChildCount());

            for (size_t i = 0; i < 2; ++i)
            {
                const auto structChild = arrayOfStructs->getChild(i);
                EXPECT_EQ(EPropertyType::Struct, structChild->getType());
                EXPECT_EQ("", structChild->getName());
                ASSERT_EQ(2u, structChild->getChildCount());
                const auto name = structChild->getChild("name");
                EXPECT_EQ(EPropertyType::String, name->getType());
                const auto address = structChild->getChild("address");
                ASSERT_EQ(2u, address->getChildCount());
                EXPECT_EQ(EPropertyType::Struct, address->getType());
                const auto addressStr = address->getChild("street");
                const auto addressNr = address->getChild("number");
                EXPECT_EQ(EPropertyType::String, addressStr->getType());
                EXPECT_EQ(EPropertyType::Int32, addressNr->getType());
            }
        }
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScriptWithModule, ScriptOverwritingBaseLibraryWontAffectOtherScriptUsingIt)
    {
        const auto script1 = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.v = FLOAT
                OUT.v = INT
            end
            function run()
                OUT.v = math.floor(IN.v)
                math.floor = nil
            end
        )", WithStdMath());
        ASSERT_NE(nullptr, script1);

        const auto script2 = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.v = FLOAT
                OUT.v = INT
            end
            function run()
                OUT.v = math.floor(IN.v + 1.0)
            end
        )", WithStdMath());
        ASSERT_NE(nullptr, script2);

        // first update runs fine
        script1->getInputs()->getChild("v")->set(1.2f);
        script2->getInputs()->getChild("v")->set(1.3f);
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(1, *script1->getOutputs()->getChild("v")->get<int32_t>());
        EXPECT_EQ(2, *script2->getOutputs()->getChild("v")->get<int32_t>());

        // force update of script2 again, after math.floor was set nil in script1
        // script2 is NOT affected
        script2->getInputs()->getChild("v")->set(2.3f);
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(3, *script2->getOutputs()->getChild("v")->get<int32_t>());

        // script1 broke itself by setting its dependency to nil and fails to update
        script1->getInputs()->getChild("v")->set(2.2f);
        EXPECT_FALSE(m_logicEngine.update());
    }

    TEST_F(ALuaScriptWithModule, ScriptOverwritingBaseLibraryViaModuleWontAffectOtherScriptUsingIt)
    {
        const std::string_view maliciousModuleSrc = R"(
            local mymath = {}
            function mymath.breakFloor(v)
                ret = math.floor(v)
                math.floor = nil
                return ret
            end
            return mymath
        )";

        const auto maliciousModule = m_logicEngine.createLuaModule(maliciousModuleSrc, WithStdMath());

        LuaConfig withMaliciousModule;
        withMaliciousModule.addDependency("mymath", *maliciousModule);
        const auto script1 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                IN.v = FLOAT
                OUT.v = INT
            end
            function run()
                OUT.v = mymath.breakFloor(IN.v)
            end
        )", withMaliciousModule);
        ASSERT_NE(nullptr, script1);

        const auto script2 = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.v = FLOAT
                OUT.v = INT
            end
            function run()
                OUT.v = math.floor(IN.v + 1.0)
            end
        )", WithStdMath());
        ASSERT_NE(nullptr, script2);

        // first update runs fine
        script1->getInputs()->getChild("v")->set(1.2f);
        script2->getInputs()->getChild("v")->set(1.3f);
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(1, *script1->getOutputs()->getChild("v")->get<int32_t>());
        EXPECT_EQ(2, *script2->getOutputs()->getChild("v")->get<int32_t>());

        // force update of script2 again, after math.floor was set nil in script1 via module
        // script2 is NOT affected
        script2->getInputs()->getChild("v")->set(2.3f);
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(3, *script2->getOutputs()->getChild("v")->get<int32_t>());

        // module broke itself by setting its math dependency to nil and script1 using it fails to update
        script1->getInputs()->getChild("v")->set(2.2f);
        EXPECT_FALSE(m_logicEngine.update());
    }

    class ALuaScriptDependencyMatch : public ALuaScriptWithModule
    {
    };

    TEST_F(ALuaScriptDependencyMatch, FailsToBeCreatedIfDeclaredDependencyDoesNotMatchProvidedDependency_NotProvidedButDeclared)
    {
        constexpr std::string_view src = R"(
            modules("dep1", "dep2")
            function interface()
            end
            function run()
            end
        )";
        EXPECT_EQ(nullptr, m_logicEngine.createLuaScript(src, createDeps({ {"dep2", m_moduleSourceCode} })));
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_THAT(m_logicEngine.getErrors().front().message, ::testing::HasSubstr("Module dependencies declared in source code: dep1, dep2\n  Module dependencies provided on create API: dep2"));
    }

    TEST_F(ALuaScriptDependencyMatch, FailsToBeCreatedIfDeclaredDependencyDoesNotMatchProvidedDependency_ProvidedButNotDeclared)
    {
        constexpr std::string_view src = R"(
            modules("dep1", "dep2")
            function interface()
            end
            function run()
            end
        )";
        EXPECT_EQ(nullptr, m_logicEngine.createLuaScript(src, createDeps({ {"dep1", m_moduleSourceCode}, {"dep2", m_moduleSourceCode}, {"dep3", m_moduleSourceCode} })));
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_THAT(m_logicEngine.getErrors().front().message, ::testing::HasSubstr("Module dependencies declared in source code: dep1, dep2\n  Module dependencies provided on create API: dep1, dep2, dep3"));
    }

    TEST_F(ALuaScriptDependencyMatch, FailsToBeCreatedIfDeclaredDependencyDoesNotMatchProvidedDependency_ExractionError)
    {
        constexpr std::string_view src = R"(
            modules("dep1", "dep1") -- duplicate dependency
            function interface()
            end
            function run()
            end
        )";
        EXPECT_EQ(nullptr, m_logicEngine.createLuaScript(src, createDeps({ {"dep1", m_moduleSourceCode} })));
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_THAT(m_logicEngine.getErrors().front().message, ::testing::HasSubstr("Error while extracting module dependencies: 'dep1' appears more than once in dependency list"));
    }

    // Tests specifically modules isolation
    class ALuaScriptWithModule_Isolation : public ALuaScriptWithModule
    {
    protected:
    };

    // This test reflects behavior which will be fixed in a next release (adapt the tests after the fix)
    TEST_F(ALuaScriptWithModule_Isolation, ScriptOverwritingModuleFunctionAffectsOtherScriptUsingIt_InRunFunction)
    {
        const std::string_view mymathModuleSrc = R"(
            local mymath = {}
            function mymath.floor1(v)
                return math.floor(v)
            end
            function mymath.floor2(v)
                return math.floor(v) + 100
            end
            return mymath
        )";
        const auto mymathModule = m_logicEngine.createLuaModule(mymathModuleSrc, WithStdMath(), "mymath");
        ASSERT_NE(nullptr, mymathModule);

        LuaConfig config;
        config.addDependency("mymath", *mymathModule);

        const auto script1 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
            end
            function run()
                mymath.floor1 = mymath.floor2
            end
        )", config);
        ASSERT_NE(nullptr, script1);

        // This will overwrite the module function floor1 with floor2
        EXPECT_TRUE(m_logicEngine.update());

        const auto script2 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                OUT.floor1 = INT
                OUT.floor2 = INT
            end
            function run()
                OUT.floor1 = mymath.floor1(1.5)
                OUT.floor2 = mymath.floor2(1.5)
            end
        )", config);
        ASSERT_NE(nullptr, script2);

        EXPECT_TRUE(m_logicEngine.update());

        // Both floor1 and floor2 have the modified code
        EXPECT_EQ(101, *script2->getOutputs()->getChild("floor1")->get<int32_t>());
        EXPECT_EQ(101, *script2->getOutputs()->getChild("floor2")->get<int32_t>());
    }

    // This test reflects behavior which will be fixed in a next release (adapt the tests after the fix)
    TEST_F(ALuaScriptWithModule_Isolation, ScriptOverwritingModuleFunctionAffectsOtherScriptUsingIt_InInterfaceFunction)
    {
        const std::string_view mymathModuleSrc = R"(
            local mymath = {}
            function mymath.floor1(v)
                return math.floor(v)
            end
            function mymath.floor2(v)
                return math.floor(v) + 100
            end
            return mymath
        )";
        const auto mymathModule = m_logicEngine.createLuaModule(mymathModuleSrc, WithStdMath());
        ASSERT_NE(nullptr, mymathModule);

        LuaConfig config;
        config.addDependency("mymath", *mymathModule);

        const auto script1 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                mymath.floor1 = mymath.floor2
            end
            function run()
            end
        )", config);
        ASSERT_NE(nullptr, script1);

        // This will overwrite the module function floor1 with floor2
        EXPECT_TRUE(m_logicEngine.update());

        const auto script2 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                OUT.floor1 = INT
                OUT.floor2 = INT
            end
            function run()
                OUT.floor1 = mymath.floor1(1.5)
                OUT.floor2 = mymath.floor2(1.5)
            end
        )", config);
        ASSERT_NE(nullptr, script2);

        EXPECT_TRUE(m_logicEngine.update());

        // Both floor1 and floor2 have the modified code
        EXPECT_EQ(101, *script2->getOutputs()->getChild("floor1")->get<int32_t>());
        EXPECT_EQ(101, *script2->getOutputs()->getChild("floor2")->get<int32_t>());
    }

    // This test reflects behavior which will be fixed in a next release (adapt the tests after the fix)
    TEST_F(ALuaScriptWithModule_Isolation, ScriptOverwritingModuleDataAffectsOtherScriptUsingIt_InRunFunction)
    {
        const std::string_view mymathModuleSrc = R"(
            local mymath = {}
            mymath.data = 1
            return mymath
        )";
        const auto mymathModule = m_logicEngine.createLuaModule(mymathModuleSrc);
        ASSERT_NE(nullptr, mymathModule);

        LuaConfig config;
        config.addDependency("mymath", *mymathModule);

        const auto script1 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
            end
            function run()
                mymath.data = 42
            end
        )", config);
        ASSERT_NE(nullptr, script1);

        // This will overwrite the module function floor1 with floor2
        EXPECT_TRUE(m_logicEngine.update());

        const auto script2 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                OUT.data = INT
            end
            function run()
                OUT.data = mymath.data
            end
        )", config);
        ASSERT_NE(nullptr, script2);

        EXPECT_TRUE(m_logicEngine.update());

        // data in script2 has modified value from script1
        EXPECT_EQ(42, *script2->getOutputs()->getChild("data")->get<int32_t>());
    }

    // This test reflects behavior which will be fixed in a next release (adapt the tests after the fix)
    TEST_F(ALuaScriptWithModule_Isolation, ScriptOverwritingModuleDataAffectsOtherScriptUsingIt_InInterfaceFunction)
    {
        const std::string_view mymathModuleSrc = R"(
            local mymath = {}
            mymath.data = 1
            return mymath
        )";
        const auto mymathModule = m_logicEngine.createLuaModule(mymathModuleSrc);
        ASSERT_NE(nullptr, mymathModule);

        LuaConfig config;
        config.addDependency("mymath", *mymathModule);

        const auto script1 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                mymath.data = 42
            end
            function run()
            end
        )", config);
        ASSERT_NE(nullptr, script1);

        // This will overwrite the module function floor1 with floor2
        EXPECT_TRUE(m_logicEngine.update());

        const auto script2 = m_logicEngine.createLuaScript(R"(
            modules("mymath")
            function interface()
                OUT.data = INT
            end
            function run()
                OUT.data = mymath.data
            end
        )", config);
        ASSERT_NE(nullptr, script2);

        EXPECT_TRUE(m_logicEngine.update());

        // data in script2 has modified value from script1
        EXPECT_EQ(42, *script2->getOutputs()->getChild("data")->get<int32_t>());
    }

    // This is something we want to forbid and catch as error. When we do, rewrite this test
    TEST_F(ALuaScriptWithModule_Isolation, ModuleOfDataCanBeModifiedByScript)
    {
        const std::string_view moduleSrc = R"(
            local mod = {}
            mod.value = 1
            function mod.getValue()
                return mod.value
            end
            return mod
        )";

        const std::string_view scriptSrc = R"(
            modules("mappedMod")
            function interface()
                OUT.fromScript = INT
                OUT.fromModule = INT
            end

            function run()
                mappedMod.value = 5
                OUT.fromScript = mappedMod.value
                OUT.fromModule = mappedMod.getValue()
            end
        )";

        const auto script = m_logicEngine.createLuaScript(scriptSrc, createDeps({ { "mappedMod", moduleSrc } }));

        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(5, *script->getOutputs()->getChild("fromScript")->get<int32_t>());
        EXPECT_EQ(5, *script->getOutputs()->getChild("fromModule")->get<int32_t>());
    }

    TEST_F(ALuaScriptWithModule_Isolation, ModuleCanModifyOutsideDataWhenExplicitlyPassedAsArgument)
    {
        const std::string_view moduleSrc = R"(
            local mod = {}
            mod.value = 1
            function mod.modifyModule(theModule)
                theModule.value = 42
            end
            return mod
        )";

        const std::string_view scriptSrc = R"(
            modules("mappedMod")
            function interface()
                OUT.result = INT
            end

            function run()
                -- Will modify the module because it's passed as argument by the
                -- script to the module
                mappedMod.modifyModule(mappedMod)
                OUT.result = mappedMod.value
            end
        )";

        const auto script = m_logicEngine.createLuaScript(scriptSrc, createDeps({ { "mappedMod", moduleSrc } }));

        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(42, *script->getOutputs()->getChild("result")->get<int32_t>());
    }

    // This test reflects behavior which will be fixed in a next release (adapt the tests after the fix)
    TEST_F(ALuaScriptWithModule_Isolation, DataIsNotIsolatedBetweenModuleAndScript_WhenNested)
    {
        const std::string_view moduleSrc = R"(
            local mod = {}
            mod.people = {joe = {age = 20}}
            function mod.getJoeAge()
                return mod.people.joe.age
            end
            return mod
        )";

        const std::string_view scriptSrc = R"(
            modules("mappedMod")
            function interface()
                OUT.resultBeforeMod = INT
                OUT.resultAfterMod = INT
            end

            function run()
                OUT.resultBeforeMod = mappedMod.getJoeAge()
                -- This will modify the module's copy of joe
                mappedMod.people.joe.age = 42
                OUT.resultAfterMod = mappedMod.getJoeAge()
            end
        )";

        const auto script = m_logicEngine.createLuaScript(scriptSrc, createDeps({ { "mappedMod", moduleSrc } }));

        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(20, *script->getOutputs()->getChild("resultBeforeMod")->get<int32_t>());
        EXPECT_EQ(42, *script->getOutputs()->getChild("resultAfterMod")->get<int32_t>());
    }
}
