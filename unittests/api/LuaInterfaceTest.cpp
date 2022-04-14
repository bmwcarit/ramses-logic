//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaInterface.h"
#include "impl/LuaInterfaceImpl.h"
#include "impl/LogicEngineImpl.h"
#include "impl/PropertyImpl.h"
#include "internals/ErrorReporting.h"

#include "generated/LuaInterfaceGen.h"

#include "SerializationTestUtils.h"

namespace rlogic::internal
{
    class ALuaInterface : public ::testing::Test
    {
    protected:
        LuaInterface* createTestInterface(std::string_view source, std::string_view interfaceName = "")
        {
            return m_logicEngine.createLuaInterface(source, interfaceName);
        }

        void createTestInterfaceAndExpectFailure(std::string_view source, std::string_view interfaceName = "")
        {
            const auto intf = m_logicEngine.createLuaInterface(source, interfaceName);
            EXPECT_EQ(intf, nullptr);
        }

        const std::string_view m_minimalInterface = R"(
            function interface(inputs)

                inputs.param1 = Type:Int32()
                inputs.param2 = Type:Float()

            end
        )";

        LogicEngine m_logicEngine;
        flatbuffers::FlatBufferBuilder m_flatBufferBuilder;
        SerializationTestUtils m_testUtils{ m_flatBufferBuilder };
        SerializationMap m_serializationMap;
        DeserializationMap m_deserializationMap;
    };

    TEST_F(ALuaInterface, CanCompileLuaInterface)
    {
        const LuaInterface* intf = createTestInterface(m_minimalInterface, "intf name");
        ASSERT_NE(nullptr, intf);
        EXPECT_STREQ("intf name", intf->getName().data());
    }

    TEST_F(ALuaInterface, CanExtractInputsFromLuaInterface)
    {
        const LuaInterface* intf = createTestInterface(m_minimalInterface, "intf name");
        ASSERT_NE(nullptr, intf);

        EXPECT_EQ(2u, intf->getInputs()->getChildCount());
        EXPECT_EQ("", intf->getInputs()->getName());

        EXPECT_EQ("param1", intf->getInputs()->getChild(0)->getName());
        EXPECT_EQ(rlogic::EPropertyType::Int32, intf->getInputs()->getChild(0)->getType());

        EXPECT_EQ("param2", intf->getInputs()->getChild(1)->getName());
        EXPECT_EQ(rlogic::EPropertyType::Float, intf->getInputs()->getChild(1)->getType());
    }

    TEST_F(ALuaInterface, ReturnsSameResultForOutputsAsInputs)
    {
        const LuaInterface* intf = createTestInterface(m_minimalInterface, "intf name");
        ASSERT_NE(nullptr, intf);

        EXPECT_EQ(2u, intf->getOutputs()->getChildCount());
        EXPECT_EQ("", intf->getOutputs()->getName());

        EXPECT_EQ("param1", intf->getInputs()->getChild(0)->getName());
        EXPECT_EQ(rlogic::EPropertyType::Int32, intf->getOutputs()->getChild(0)->getType());

        EXPECT_EQ("param2", intf->getOutputs()->getChild(1)->getName());
        EXPECT_EQ(rlogic::EPropertyType::Float, intf->getOutputs()->getChild(1)->getType());
    }

    TEST_F(ALuaInterface, FailsIfNameEmpty)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(
            function interface(inputs)
            end
        )SCRIPT", "");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Can't create interface with empty name!"));
    }

    TEST_F(ALuaInterface, FailsIfNameExists)
    {
        const LuaInterface* intf = createTestInterface(m_minimalInterface, "intf name");

        createTestInterfaceAndExpectFailure(R"SCRIPT(
            function interface(inputs)
            end
        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Interface object with name 'intf name' already exists!"));
        EXPECT_EQ(intf, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaInterface, UpdatingInputsLeadsToUpdatingOutputs)
    {
        LuaInterface* intf = createTestInterface(m_minimalInterface, "intf name");
        ASSERT_NE(nullptr, intf);

        ASSERT_EQ(intf->getInputs()->getChild(0)->get<int32_t>(), intf->getOutputs()->getChild(0)->get<int32_t>());

        intf->getInputs()->getChild(0)->set<int32_t>(123);
        ASSERT_EQ(intf->getInputs()->getChild(0)->get<int32_t>(), intf->getOutputs()->getChild(0)->get<int32_t>());

        intf->m_impl.update();
        EXPECT_EQ(123, intf->getOutputs()->getChild(0)->get<int32_t>());
    }

    TEST_F(ALuaInterface, InterfaceFunctionIsExecutedOnlyOnce)
    {
        LuaInterface* intf = createTestInterface(R"SCRIPT(
            local firstExecution = true

            function interface(inputs)
                if not firstExecution then
                    error("a problem happened")
                end

                firstExecution = false
                inputs.param1 = Type:Int32()
                inputs.param2 = Type:Float()
            end

        )SCRIPT", "intf name");

        ASSERT_NE(nullptr, intf);
        EXPECT_STREQ("intf name", intf->getName().data());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 0u);
    }

    TEST_F(ALuaInterface, ReportsErrorIfInterfaceDidNotCompile)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(
            function interface(inputs)
                not.a.valid.lua.syntax
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message,
            ::testing::HasSubstr("[intf name] Error while loading interface. Lua stack trace:\n[string \"intf name\"]:3: unexpected symbol near 'not'"));
    }

    TEST_F(ALuaInterface, ReportsErrorIfNoInterfaceFunctionDefined)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT()SCRIPT", "intf name");
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_STREQ("[intf name] No 'interface' function defined!", m_logicEngine.getErrors()[0].message.c_str());
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorIfInitFunctionDefined)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)
                inputs.param1 = Type:Int32()
                inputs.param2 = Type:Float()
            end

            function init()
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Unexpected function name 'init'! Only 'interface' function can be declared!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorIfRunFunctionDefined)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)

                inputs.param1 = Type:Int32()
                inputs.param2 = Type:Float()
            end

            function run(IN,OUT)
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Unexpected function name 'run'! Only 'interface' function can be declared!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorIfGlobalSpecialVariableAccessed)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)

                inputs.param1 = Type:Int32()
                inputs.param2 = Type:Float()

                GLOBAL.param3 = Type:Float()
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Unexpected global access to key 'GLOBAL' in interface()!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorIfLuaGlobalVariablesDefined)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            someGlobal = 10

            function interface(inputs)

                inputs.param1 = Type:Int32()
                inputs.param2 = Type:Float()
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Declaring global variables is forbidden (exception: the 'interface' function)! (found value of type 'number')"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorWhenTryingToReadUnknownGlobals)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)
                local t = IN
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Unexpected global access to key 'IN' in interface()!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorWhenAccessingGlobalsOutsideInterfaceFunction)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            table.getn(_G)
            function interface(inputs)
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Trying to read global variable 'table' in an interface!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorWhenSettingGlobals)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)
                thisCausesError = 'bad'
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Unexpected variable definition 'thisCausesError' in interface()!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorWhenTryingToOverrideSpecialGlobalVariable)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)
                GLOBAL = {}
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Unexpected variable definition 'GLOBAL' in interface()!"));
    }

    TEST_F(ALuaInterface, Sandboxing_CanDeclareLocalVariables)
    {
        LuaInterface* intf = createTestInterface(R"SCRIPT(

            function interface(inputs)
                local multiplexersAreAwesomeIfYouLearnThem = 12
                inputs.param = Type:Int32()
            end

        )SCRIPT", "intf name");

        EXPECT_NE(nullptr, intf);
        EXPECT_EQ(m_logicEngine.getErrors().size(), 0u);
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorIfUnknownFunctionDefined)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)

                inputs.param1 = Type:Int32()
                inputs.param2 = Type:Float()

            end

            function HackToCatchDeadlineCozNobodyChecksDeliveries()
            end

        )SCRIPT", "intf name");

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Unexpected function name 'HackToCatchDeadlineCozNobodyChecksDeliveries'! Only 'interface' function can be declared!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ReportsErrorWhenTryingToDeclareInterfaceFunctionTwice)
    {
        createTestInterfaceAndExpectFailure(R"SCRIPT(

            function interface(inputs)
            end

            function interface(inputs)
            end

        )SCRIPT", "intf name");

        EXPECT_THAT(m_logicEngine.getErrors().begin()->message,
            ::testing::HasSubstr("Function 'interface' can only be declared once!"));
    }

    TEST_F(ALuaInterface, Sandboxing_ForbidsCallingSpecialFunctionsFromInsideInterface)
    {
        for (const auto& specialFunction : std::vector<std::string>{ "init", "run", "interface" })
        {
            createTestInterfaceAndExpectFailure(fmt::format(R"SCRIPT(

                function interface(inputs)
                    {}()
                end
            )SCRIPT", specialFunction), "intf name");

            EXPECT_THAT(m_logicEngine.getErrors()[0].message,
                ::testing::HasSubstr(fmt::format("Unexpected global access to key '{}' in interface()!", specialFunction)));
        }
    }

    TEST_F(ALuaInterface, CanSerializeAndDeserializeLuaInterface)
    {
        // Serialize
        {
            LuaInterface* intf = createTestInterface(m_minimalInterface, "intf name");
            LuaScript* inputsScript = m_logicEngine.createLuaScript(R"LUA_SCRIPT(
            function interface(IN,OUT)

                IN.param1 = Type:Int32()
                IN.param2 = Type:Float()

            end

            function run(IN,OUT)
            end
            )LUA_SCRIPT", {});

            m_logicEngine.link(*intf->getOutputs()->getChild(0), *inputsScript->getInputs()->getChild(0));
            m_logicEngine.link(*intf->getOutputs()->getChild(1), *inputsScript->getInputs()->getChild(1));

            (void)LuaInterfaceImpl::Serialize(dynamic_cast<LuaInterfaceImpl&>(intf->m_impl), m_flatBufferBuilder, m_serializationMap);
        }

        const auto& serializedScript = *flatbuffers::GetRoot<rlogic_serialization::LuaInterface>(m_flatBufferBuilder.GetBufferPointer());

        ASSERT_TRUE(serializedScript.base());
        ASSERT_TRUE(serializedScript.base()->name());
        EXPECT_EQ(serializedScript.base()->name()->string_view(), "intf name");
        EXPECT_EQ(serializedScript.base()->id(), 1u);

        ASSERT_TRUE(serializedScript.rootProperty());
        EXPECT_EQ(serializedScript.rootProperty()->rootType(), rlogic_serialization::EPropertyRootType::Struct);
        ASSERT_TRUE(serializedScript.rootProperty()->children());
        EXPECT_EQ(serializedScript.rootProperty()->children()->size(), 2u);

        // Deserialize
        {
            ErrorReporting errorReporting;
            std::unique_ptr<LuaInterfaceImpl> deserializedScript = LuaInterfaceImpl::Deserialize(serializedScript, errorReporting, m_deserializationMap);

            ASSERT_TRUE(deserializedScript);
            EXPECT_TRUE(errorReporting.getErrors().empty());

            EXPECT_EQ(deserializedScript->getName(), "intf name");
            EXPECT_EQ(deserializedScript->getId(), 1u);
        }
    }

    TEST_F(ALuaInterface, CanSerializeLuaInterfaceIfInputsAreNotLinked)
    {
        LuaInterface* intf = createTestInterface(m_minimalInterface, "intf name");

        LuaScript* inputsScript = m_logicEngine.createLuaScript(R"LUA_SCRIPT(
        function interface(IN,OUT)

            IN.param1 = Type:Int32()
            IN.param2 = Type:Float()

        end

        function run(IN,OUT)
        end
        )LUA_SCRIPT", {});

        m_logicEngine.link(*intf->getOutputs()->getChild(0), *inputsScript->getInputs()->getChild(0));
        m_logicEngine.link(*intf->getOutputs()->getChild(1), *inputsScript->getInputs()->getChild(1));

        auto serializationResult = LuaInterfaceImpl::Serialize(dynamic_cast<LuaInterfaceImpl&>(intf->m_impl), m_flatBufferBuilder, m_serializationMap);
        EXPECT_FALSE(serializationResult.IsNull());
    }

    TEST_F(ALuaInterface, CanCreateInterfaceWithComplexTypes)
    {
        const std::string_view interfaceScript = R"(
            function interface(inputs)

                inputs.array_int = Type:Array(2, Type:Int32())
                inputs.array_struct = Type:Array(3, {a=Type:Int32(), b=Type:Float()})
                inputs.struct = {a=Type:Int32(), b={c = Type:Int32(), d=Type:Float()}}

            end
        )";
        LuaInterface* intf = createTestInterface(interfaceScript, "intf name");
        ASSERT_NE(nullptr, intf);

        ASSERT_EQ(intf->getInputs()->getChildCount(), 3);
        ASSERT_EQ(intf->getOutputs()->getChildCount(), 3);

        ASSERT_STREQ(intf->getInputs()->getChild(0)->getName().data(), "array_int");
        EXPECT_EQ(intf->getInputs()->getChild(0)->getType(), EPropertyType::Array);
        EXPECT_EQ(intf->getOutputs()->getChild(0)->getType(), EPropertyType::Array);
        EXPECT_EQ(intf->getOutputs()->getChild(0)->getChild(0)->getType(), EPropertyType::Int32);

        ASSERT_STREQ(intf->getInputs()->getChild(1)->getName().data(), "array_struct");
        EXPECT_EQ(intf->getInputs()->getChild(1)->getType(), EPropertyType::Array);
        EXPECT_EQ(intf->getOutputs()->getChild(1)->getType(), EPropertyType::Array);
        EXPECT_EQ(intf->getOutputs()->getChild(1)->getChild(0)->getType(), EPropertyType::Struct);

        ASSERT_STREQ(intf->getInputs()->getChild(2)->getName().data(), "struct");
        EXPECT_EQ(intf->getInputs()->getChild(2)->getType(), EPropertyType::Struct);
        EXPECT_EQ(intf->getOutputs()->getChild(2)->getType(), EPropertyType::Struct);
        EXPECT_EQ(intf->getOutputs()->getChild(2)->getChild(0)->getType(), EPropertyType::Int32);
        EXPECT_EQ(intf->getOutputs()->getChild(2)->getChild(1)->getType(), EPropertyType::Struct);
    }

    TEST_F(ALuaInterface, CanUpdateInterfaceValuesWithComplexTypes)
    {
        const std::string_view interfaceScript = R"(
            function interface(inputs)

                inputs.array_int = Type:Array(2, Type:Int32())
                inputs.array_struct = Type:Array(3, {a=Type:Int32(), b=Type:Float()})
                inputs.struct = {a=Type:Int32(), b={c = Type:Int32(), d=Type:Float()}}

            end
        )";
        LuaInterface* intf = createTestInterface(interfaceScript, "intf name");
        ASSERT_NE(nullptr, intf);

        intf->getInputs()->getChild(0)->getChild(0)->set<int32_t>(123);
        intf->getInputs()->getChild(2)->getChild(0)->set<int32_t>(456);

        ASSERT_EQ(intf->getOutputs()->getChild(0)->getChild(0)->get<int32_t>(), 123);
        ASSERT_EQ(intf->getOutputs()->getChild(2)->getChild(0)->get<int32_t>(), 456);

        intf->m_impl.update();
        EXPECT_EQ(123, intf->getOutputs()->getChild(0)->getChild(0)->get<int32_t>());
        EXPECT_EQ(456, intf->getOutputs()->getChild(2)->getChild(0)->get<int32_t>());
    }

    TEST_F(ALuaInterface, CanCheckIfOutputsAreLinked)
    {
        LuaInterface* intf = createTestInterface(R"(
            function interface(IN,OUT)

                IN.param1 = Type:Int32()
                IN.param2 = {a=Type:Float(), b=Type:Int32()}

            end
        )", "intf name");

        const auto& intfImpl = dynamic_cast<const LuaInterfaceImpl&>(intf->m_impl);

        const auto* output1 = intf->getOutputs()->getChild(0);
        const auto* output21 = intf->getOutputs()->getChild(1)->getChild(0);
        const auto* output22 = intf->getOutputs()->getChild(1)->getChild(1);

        std::vector<const Property*> unlinkedOutputs;
        EXPECT_FALSE(intfImpl.checkAllOutputsLinked(unlinkedOutputs));
        EXPECT_THAT(unlinkedOutputs, ::testing::UnorderedElementsAre(output1, output21, output22));

        LuaScript* inputsScript = m_logicEngine.createLuaScript(R"LUA_SCRIPT(
        function interface(IN,OUT)

            IN.param1 = Type:Int32()
            IN.param21 = Type:Float()
            IN.param22 = Type:Int32()

        end

        function run(IN,OUT)
        end
        )LUA_SCRIPT", {});

        //link 1 output
        m_logicEngine.link(*output1, *inputsScript->getInputs()->getChild(0));
        unlinkedOutputs.clear();
        EXPECT_FALSE(intfImpl.checkAllOutputsLinked(unlinkedOutputs));
        EXPECT_THAT(unlinkedOutputs, ::testing::UnorderedElementsAre(output21, output22));

        //link 2nd output
        m_logicEngine.link(*output21, *inputsScript->getInputs()->getChild(1));
        unlinkedOutputs.clear();
        EXPECT_FALSE(intfImpl.checkAllOutputsLinked(unlinkedOutputs));
        EXPECT_THAT(unlinkedOutputs, ::testing::UnorderedElementsAre(output22));

        m_logicEngine.link(*output22, *inputsScript->getInputs()->getChild(2));
        unlinkedOutputs.clear();
        EXPECT_TRUE(intfImpl.checkAllOutputsLinked(unlinkedOutputs));
        EXPECT_TRUE(unlinkedOutputs.empty());
    }
}
