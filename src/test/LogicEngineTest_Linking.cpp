//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

#include "RamsesTestUtils.h"

#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Node.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/UniformInput.h"
#include "ramses-client-api/Appearance.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesNodeBinding.h"

#include "internals/impl/LogicEngineImpl.h"
#include "internals/impl/RamsesNodeBindingImpl.h"
#include "internals/impl/LogicNodeImpl.h"

#include "fmt/format.h"
#include <array>

namespace rlogic
{
    class ALogicEngine_Linking : public ALogicEngine
    {
    protected:
        ALogicEngine_Linking()
            : m_sourceScript(*m_logicEngine.createLuaScriptFromSource(m_minimalLinkScript, "SourceScript"))
            , m_targetScript(*m_logicEngine.createLuaScriptFromSource(m_minimalLinkScript, "TargetScript"))
            , m_sourceProperty(*m_sourceScript.getOutputs()->getChild("source"))
            , m_targetProperty(*m_targetScript.getInputs()->getChild("target"))
        {
        }

        const std::string_view m_minimalLinkScript = R"(
            function interface()
                IN.target = BOOL
                OUT.source = BOOL
            end
            function run()
            end
        )";

        const std::string_view m_linkScriptMultipleTypes = R"(
            function interface()
                IN.target_INT = INT
                OUT.source_INT = INT
                IN.target_VEC3F = VEC3F
                OUT.source_VEC3F = VEC3F
            end
            function run()
                OUT.source_INT = IN.target_INT
                OUT.source_VEC3F = IN.target_VEC3F
            end
        )";

        LuaScript& m_sourceScript;
        LuaScript& m_targetScript;

        const Property& m_sourceProperty;
        Property& m_targetProperty;


    };

    TEST_F(ALogicEngine_Linking, ProducesErrorIfPropertiesWithMismatchedTypesAreLinked)
    {
        const char* errorString = "Types of source property 'outParam:{}' does not match target property 'inParam:{}'";

        std::array<std::tuple<std::string, std::string, std::string>, 5> errorCases = {
            std::make_tuple("FLOAT", "INT",   fmt::format(errorString, "FLOAT", "INT")),
            std::make_tuple("VEC3F", "VEC3I", fmt::format(errorString, "VEC3F", "VEC3I")),
            std::make_tuple("VEC2F", "VEC4I", fmt::format(errorString, "VEC2F", "VEC4I")),
            std::make_tuple("VEC2I", "FLOAT", fmt::format(errorString, "VEC2I", "FLOAT")),
            std::make_tuple("INT",
            R"({
                param1 = INT,
                param2 = FLOAT
            })", fmt::format(errorString, "INT", "STRUCT"))
        };

        for (const auto& errorCase : errorCases)
        {
            const auto  luaScriptSource = fmt::format(R"(
                function interface()
                    IN.inParam = {}
                    OUT.outParam = {}
                end
                function run()
                end
            )", std::get<1>(errorCase), std::get<0>(errorCase));

            auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);
            auto targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);

            const auto sourceProperty = sourceScript->getOutputs()->getChild("outParam");
            const auto targetProperty = targetScript->getInputs()->getChild("inParam");

            EXPECT_FALSE(m_logicEngine.link(
                *sourceProperty,
                *targetProperty
            ));

            const auto& errors = m_logicEngine.getErrors();
            ASSERT_EQ(1u, errors.size());
            EXPECT_EQ(errors[0], std::get<2>(errorCase));
        }
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfLogicNodeIsLinkedToItself)
    {
        const auto targetPropertyFromTheSameScript = m_sourceScript.getInputs()->getChild("target");

        EXPECT_FALSE(m_logicEngine.link(m_sourceProperty, *targetPropertyFromTheSameScript));

        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        // TODO Violin error message is not giving enough info where the error came from - improve
        EXPECT_EQ(errors[0], "SourceNode and TargetNode are equal");
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfInputIsLinkedToOutput)
    {
        const auto sourceProperty = m_sourceScript.getOutputs()->getChild("source");
        const auto targetProperty = m_targetScript.getInputs()->getChild("target");

        EXPECT_FALSE(m_logicEngine.link(*targetProperty, *sourceProperty));
        const auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Failed to link input property 'target' to output property 'source'. Only outputs can be linked to inputs", errors[0]);
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfInputIsLinkedToInput)
    {
        const auto sourceInput = m_sourceScript.getInputs()->getChild("target");
        const auto targetInput = m_targetScript.getInputs()->getChild("target");

        EXPECT_FALSE(m_logicEngine.link(*sourceInput, *targetInput));
        const auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Failed to link input property 'target' to input property 'target'. Only outputs can be linked to inputs", errors[0]);
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfOutputIsLinkedToOutput)
    {
        auto       sourceScript = m_logicEngine.createLuaScriptFromSource(m_linkScriptMultipleTypes);
        auto       targetScript = m_logicEngine.createLuaScriptFromSource(m_linkScriptMultipleTypes);

        const auto sourceOuput  = sourceScript->getOutputs()->getChild("source_INT");
        const auto targetOutput = targetScript->getOutputs()->getChild("source_INT");

        EXPECT_FALSE(m_logicEngine.link(*sourceOuput, *targetOutput));

        const auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Failed to link output property 'source_INT' to output property 'source_INT'. Only outputs can be linked to inputs", errors[0]);
    }

    TEST_F(ALogicEngine_Linking, ProducesNoErrorIfMatchingPropertiesAreLinked)
    {
        EXPECT_TRUE(m_logicEngine.link(m_sourceProperty, m_targetProperty));
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfPropertyIsLinkedTwiceToSameProperty_LuaScript)
    {
        EXPECT_TRUE(m_logicEngine.link(m_sourceProperty, m_targetProperty));
        EXPECT_FALSE(m_logicEngine.link(m_sourceProperty, m_targetProperty));

        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ(errors[0], "The property 'source' of LogicNode 'SourceScript' is already linked to the property 'target' of LogicNode 'TargetScript'");
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfPropertyIsLinkedTwice_RamsesBinding)
    {
        auto ramsesBinding = m_logicEngine.createRamsesNodeBinding("RamsesBinding");

        const auto visibilityProperty = ramsesBinding->getInputs()->getChild("visibility");

        EXPECT_TRUE(m_logicEngine.link(m_sourceProperty, *visibilityProperty));
        EXPECT_FALSE(m_logicEngine.link(m_sourceProperty, *visibilityProperty));

        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ(errors[0], "The property 'source' of LogicNode 'SourceScript' is already linked to the property 'visibility' of LogicNode 'RamsesBinding'");
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfNotLinkedPropertyIsUnlinked_LuaScript)
    {
        EXPECT_FALSE(m_logicEngine.unlink(m_sourceProperty, m_targetProperty));

        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        // TODO Violin error message is not giving enough info where the error came from
        EXPECT_EQ(errors[0], "No link available from source property 'source' to target property 'target'");
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfNotLinkedPropertyIsUnlinked_RamsesNodeBinding)
    {
        auto ramsesBinding = m_logicEngine.createRamsesNodeBinding("RamsesBinding");

        const auto visibilityProperty = ramsesBinding->getInputs()->getChild("visibility");

        EXPECT_FALSE(m_logicEngine.unlink(m_sourceProperty, *visibilityProperty));

        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ(errors[0], "No link available from source property 'source' to target property 'visibility'");
    }

    TEST_F(ALogicEngine_Linking, ProducesNoErrorIfLinkedToMatchingType)
    {
        const auto  luaScriptSource = R"(
            function interface()
                IN.boolTarget  = BOOL
                IN.intTarget   = INT
                IN.floatTarget = FLOAT
                IN.vec2Target  = VEC2F
                IN.vec3Target  = VEC3F
                OUT.boolSource  = BOOL
                OUT.intSource   = INT
                OUT.floatSource = FLOAT
                OUT.vec2Source  = VEC2F
                OUT.vec3Source  = VEC3F
            end
            function run()
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);
        auto targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);

        const auto outputs  = sourceScript->getOutputs();
        const auto inputs   = targetScript->getInputs();

        auto boolTarget = inputs->getChild("boolTarget");
        auto intTarget = inputs->getChild("intTarget");
        auto floatTarget = inputs->getChild("floatTarget");
        auto vec2Target = inputs->getChild("vec2Target");
        auto vec3Target = inputs->getChild("vec3Target");

        auto boolSource = outputs->getChild("boolSource");
        auto intSource = outputs->getChild("intSource");
        auto floatSource = outputs->getChild("floatSource");
        auto vec2Source = outputs->getChild("vec2Source");
        auto vec3Source = outputs->getChild("vec3Source");

        EXPECT_TRUE(m_logicEngine.link(*boolSource, *boolTarget));
        EXPECT_TRUE(m_logicEngine.link(*intSource, *intTarget));
        EXPECT_TRUE(m_logicEngine.link(*floatSource, *floatTarget));
        EXPECT_TRUE(m_logicEngine.link(*vec2Source, *vec2Target));
        EXPECT_TRUE(m_logicEngine.link(*vec3Source, *vec3Target));
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorOnLinkingStructs)
    {
        const auto  luaScriptSource = R"(
            function interface()
                IN.intTarget = INT
                IN.structTarget = {
                    intTarget = INT,
                    floatTarget = FLOAT
                }
                OUT.intSource = INT
                OUT.structSource  = {
                    intTarget = INT,
                    floatTarget = FLOAT
                }
            end
            function run()
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);
        auto targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);

        const auto output = sourceScript->getOutputs();
        const auto input  = targetScript->getInputs();

        auto structTarget = input->getChild("structTarget");
        auto structSource = output->getChild("structSource");

        EXPECT_FALSE(m_logicEngine.link(*structSource, *structTarget));
        auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Can't link properties of type 'Struct' directly, currently only primitive properties can be linked", errors[0]);

        EXPECT_FALSE(m_logicEngine.link(*output, *input));
        errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Can't link properties of type 'Struct' directly, currently only primitive properties can be linked", errors[0]);
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfNotLinkedPropertyIsUnlinked_WhenAnotherLinkFromTheSameScriptExists)
    {
        const auto  luaScriptSource = R"(
            function interface()
                IN.intTarget1 = INT
                IN.intTarget2 = INT
                OUT.intSource = INT
            end
            function run()
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);
        auto targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource);

        const auto sourceProperty = sourceScript->getOutputs()->getChild("intSource");
        const auto targetProperty1 = targetScript->getInputs()->getChild("intTarget1");
        const auto targetProperty2 = targetScript->getInputs()->getChild("intTarget2");

        EXPECT_TRUE(m_logicEngine.link(*sourceProperty, *targetProperty1));

        EXPECT_FALSE(m_logicEngine.unlink(*sourceProperty, *targetProperty2));

        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        // TODO Violin error message is not giving enough info where the error came from
        EXPECT_EQ(errors[0], "No link available from source property 'intSource' to target property 'intTarget2'");
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfNotLinkedPropertyIsUnlinked_RamsesBinding)
    {
        auto targetBinding = m_logicEngine.createRamsesNodeBinding("NodeBinding");
        const auto visibilityProperty = targetBinding->getInputs()->getChild("visibility");
        const auto unlinkedTargetProperty = targetBinding->getInputs()->getChild("translation");

        EXPECT_TRUE(m_logicEngine.link(m_sourceProperty, *visibilityProperty));

        EXPECT_FALSE(m_logicEngine.unlink(m_sourceProperty, *unlinkedTargetProperty));

        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        // TODO Violin error message is not giving enough info where the error came from
        EXPECT_EQ(errors[0], "No link available from source property 'source' to target property 'translation'");
    }

    TEST_F(ALogicEngine_Linking, UnlinksPropertiesWhichAreLinked)
    {
        ASSERT_TRUE(m_logicEngine.link(
            m_sourceProperty,
            m_targetProperty
        ));

        EXPECT_TRUE(m_logicEngine.unlink(
            m_sourceProperty,
            m_targetProperty
        ));
        // TODO Violin This is already tested below, isn't it? (and the other test also checks what happens with the values - this one only checks return value of unlink())
    }

    TEST_F(ALogicEngine_Linking, ProducesNoErrorsIfMultipleLinksFromSameSourceAreUnlinked)
    {
        auto targetScript2 = m_logicEngine.createLuaScriptFromSource(m_minimalLinkScript);

        const auto targetProperty2 = targetScript2->getInputs()->getChild("target");

        m_logicEngine.link(
            m_sourceProperty,
            m_targetProperty
        );

        m_logicEngine.link(
            m_sourceProperty,
            *targetProperty2
        );

        EXPECT_TRUE(m_logicEngine.unlink(
            m_sourceProperty,
            m_targetProperty
        ));

        EXPECT_TRUE(m_logicEngine.unlink(
            m_sourceProperty,
            *targetProperty2
        ));

        // TODO Violin What happens after they are unlinked? Probably should test that the link has no effect, i.e. doesn't propagate values any more
    }

    TEST_F(ALogicEngine_Linking, PropagatesOutputsToInputsIfLinked)
    {
        auto sourceScript = m_logicEngine.createLuaScriptFromSource(m_linkScriptMultipleTypes, "SourceScript");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(m_linkScriptMultipleTypes, "TargetScript");

        auto output = sourceScript->getOutputs()->getChild("source_INT");
        auto input  = targetScript->getInputs()->getChild("target_INT");

        EXPECT_TRUE(m_logicEngine.link(*output, *input));

        sourceScript->getInputs()->getChild("target_INT")->set(42);

        m_logicEngine.update();

        EXPECT_EQ(42, *targetScript->getOutputs()->getChild("source_INT")->get<int32_t>());
    }

    // TODO Violin test more corner cases - especially with the value of the unlinked input and different ordering of link/unlink/update calls
    TEST_F(ALogicEngine_Linking, DoesNotPropagateOutputsToInputsAfterUnlink)
    {
        const auto  luaScriptSource = R"(
            function interface()
                IN.intTarget = INT
                OUT.intSource = INT
            end
            function run()
                OUT.intSource = IN.intTarget
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource, "SourceScript");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource, "TargetScript");

        auto output = sourceScript->getOutputs()->getChild("intSource");
        auto input  = targetScript->getInputs()->getChild("intTarget");

        EXPECT_TRUE(m_logicEngine.link(*output, *input));
        sourceScript->getInputs()->getChild("intTarget")->set(42);

        EXPECT_TRUE(m_logicEngine.unlink(
            *output,
            *input
        ));

        m_logicEngine.update();

        EXPECT_EQ(0, *targetScript->getOutputs()->getChild("intSource")->get<int32_t>());
    }

    // TODO Violin add test with 2 scripts , one input in each
    TEST_F(ALogicEngine_Linking, PropagatesOneOutputToMultipleInputs)
    {
        const auto  luaScriptSource1 = R"(
            function interface()
                OUT.intSource = INT
            end
            function run()
                OUT.intSource = 5
            end
        )";

        const auto  luaScriptSource2 = R"(
            function interface()
                IN.intTarget1 = INT
                IN.intTarget2 = INT
            end
            function run()
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource1, "SourceScript");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource2, "TargetScript");

        auto output = sourceScript->getOutputs()->getChild("intSource");
        auto input1 = targetScript->getInputs()->getChild("intTarget1");
        auto input2 = targetScript->getInputs()->getChild("intTarget2");

        EXPECT_TRUE(m_logicEngine.link(*output, *input1));
        EXPECT_TRUE(m_logicEngine.link(*output, *input2));

        m_logicEngine.update();

        EXPECT_EQ(5, *targetScript->getInputs()->getChild("intTarget1")->get<int32_t>());
        EXPECT_EQ(5, *targetScript->getInputs()->getChild("intTarget2")->get<int32_t>());

        EXPECT_TRUE(m_logicEngine.unlink(*output, *input1));
        input1->set(6);

        m_logicEngine.update();

        EXPECT_EQ(6, *input1->get<int32_t>());
        EXPECT_EQ(5, *input2->get<int32_t>());
    }

    // TODO Violin need more tests - what is with default values after unlinking?
    TEST_F(ALogicEngine_Linking, PropagatesOutputsToInputsIfLinkedForRamsesAppearanceBindings)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene = testSetup.createScene();

        ramses::EffectDescription effectDesc;
        effectDesc.setFragmentShader(R"(
        #version 100

        void main(void)
        {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        })");

        effectDesc.setVertexShader(R"(
        #version 100

        uniform highp float floatUniform;

        void main()
        {
            gl_Position = floatUniform * vec4(1.0);
        })");

        const ramses::Effect* effect     = scene->createEffect(effectDesc, ramses::ResourceCacheFlag_DoNotCache, "glsl shader");
        ramses::Appearance*   appearance = scene->createAppearance(*effect, "triangle appearance");

        const auto  luaScriptSource = R"(
            function interface()
                IN.floatInput = FLOAT
                OUT.floatOutput = FLOAT
            end
            function run()
                OUT.floatOutput = IN.floatInput
            end
        )";

        auto sourceScript  = m_logicEngine.createLuaScriptFromSource(luaScriptSource, "SourceScript");
        auto targetBinding = m_logicEngine.createRamsesAppearanceBinding("TargetBinding");
        targetBinding->setRamsesAppearance(appearance);

        auto sourceInput  = sourceScript->getInputs()->getChild("floatInput");
        auto sourceOutput = sourceScript->getOutputs()->getChild("floatOutput");
        auto targetInput  = targetBinding->getInputs()->getChild("floatUniform");

        m_logicEngine.link(*sourceOutput, *targetInput);

        sourceInput->set(47.11f);
        m_logicEngine.update();

        ramses::UniformInput floatUniform;
        effect->findUniformInput("floatUniform", floatUniform);
        float result = 0.0f;
        appearance->getInputValueFloat(floatUniform, result);
        EXPECT_FLOAT_EQ(47.11f, result);
    }

    // TODO Violin test should actually test that the links propagates the value *even if the output is NOT set any more in the source script!*
    TEST_F(ALogicEngine_Linking, PropagatesValueIfLinkIsCreatedAndOutputValueIsSetBeforehand)
    {
        const auto  luaScriptSource1 = R"(
            function interface()
                OUT.output = INT
            end
            function run()
                OUT.output = 5
            end
        )";

        const auto  luaScriptSource2 = R"(
            function interface()
                IN.input = INT
            end
            function run()
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource1, "source");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource2, "target");

        auto sourceOutput = sourceScript->getOutputs()->getChild("output");
        auto targetInput  = targetScript->getInputs()->getChild("input");

        //propagates source input to source output
        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_EQ(5, *sourceOutput->get<int32_t>());
        EXPECT_EQ(0, *targetInput->get<int32_t>());

        ASSERT_TRUE(m_logicEngine.link(*sourceOutput, *targetInput));
        m_logicEngine.update();

        EXPECT_EQ(5, *targetInput->get<int32_t>());
    }

    TEST_F(ALogicEngine_Linking, PropagatesValueIfLinkIsCreatedAndInputValueIsSetBeforehand)
    {
        const auto  luaScriptSource1 = R"(
            function interface()
                OUT.output = INT
            end
            function run()
                OUT.output = 5
            end
        )";

        const auto  luaScriptSource2 = R"(
            function interface()
                IN.input = INT
            end
            function run()
            end
        )";

        auto        sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource1, "source");
        auto        targetScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource2, "target");

        auto sourceOutput = sourceScript->getOutputs()->getChild("output");
        auto targetInput = targetScript->getInputs()->getChild("input");

        targetInput->set<int32_t>(100);
        ASSERT_TRUE(m_logicEngine.update());

        ASSERT_EQ(5, *sourceOutput->get<int32_t>());
        ASSERT_EQ(100, *targetInput->get<int32_t>());

        m_logicEngine.link(*sourceOutput, *targetInput);
        m_logicEngine.update();

        EXPECT_EQ(5, *targetInput->get<int32_t>());

        m_logicEngine.unlink(*sourceOutput, *targetInput);
        m_logicEngine.update();

        // Value was overwritten after link + update
        EXPECT_EQ(5, *targetInput->get<int32_t>());
    }

    TEST_F(ALogicEngine_Linking, ProducesErrorIfLinkIsCreatedBetweenDifferentLogicEngines)
    {
        LogicEngine otherLogicEngine;
        const auto  luaScriptSource = R"(
            function interface()
                IN.floatInput = FLOAT
                OUT.floatOutput = FLOAT
            end
            function run()
                OUT.floatOutput = IN.floatInput
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(luaScriptSource, "SourceScript");
        auto targetScript = otherLogicEngine.createLuaScriptFromSource(luaScriptSource, "TargetScript");

        const auto sourceOutput = sourceScript->getOutputs()->getChild("floatOutput");
        const auto targetInput  = targetScript->getInputs()->getChild("floatInput");

        EXPECT_FALSE(m_logicEngine.link(*sourceOutput, *targetInput));
        {
            auto errors = m_logicEngine.getErrors();
            ASSERT_EQ(1u, errors.size());
            EXPECT_EQ("LogicNode 'TargetScript' is not an instance of this LogicEngine", errors[0]);
        }

        EXPECT_FALSE(otherLogicEngine.link(*sourceOutput, *targetInput));
        {
            auto errors = otherLogicEngine.getErrors();
            ASSERT_EQ(1u, errors.size());
            EXPECT_EQ("LogicNode 'SourceScript' is not an instance of this LogicEngine", errors[0]);
        }
    }

    TEST_F(ALogicEngine_Linking, PropagatesValuesFromMultipleOutputScriptsToOneInputScript)
    {
        const auto  sourceScript = R"(
            function interface()
                IN.floatInput = FLOAT
                OUT.floatOutput = FLOAT
            end
            function run()
                OUT.floatOutput = IN.floatInput
            end
        )";
        const auto  targetScript = R"(
            function interface()
                IN.floatInput1 = FLOAT
                IN.floatInput2 = FLOAT
                OUT.floatOutput1 = FLOAT
                OUT.floatOutput2 = FLOAT
            end
            function run()
                OUT.floatOutput1 = IN.floatInput1
                OUT.floatOutput2 = IN.floatInput2
            end
        )";

        auto scriptA = m_logicEngine.createLuaScriptFromSource(sourceScript, "ScriptA");
        auto scriptB = m_logicEngine.createLuaScriptFromSource(sourceScript, "ScriptB");
        auto scriptC = m_logicEngine.createLuaScriptFromSource(targetScript, "ScriptC");

        auto inputA  = scriptA->getInputs()->getChild("floatInput");
        auto outputA = scriptA->getOutputs()->getChild("floatOutput");
        auto inputB  = scriptB->getInputs()->getChild("floatInput");
        auto outputB = scriptB->getOutputs()->getChild("floatOutput");

        auto inputC1  = scriptC->getInputs()->getChild("floatInput1");
        auto inputC2  = scriptC->getInputs()->getChild("floatInput2");
        auto outputC1 = scriptC->getOutputs()->getChild("floatOutput1");
        auto outputC2 = scriptC->getOutputs()->getChild("floatOutput2");

        m_logicEngine.link(*outputA, *inputC1);
        m_logicEngine.link(*outputB, *inputC2);

        inputA->set(42.f);
        inputB->set(24.f);

        m_logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *outputC1->get<float>());
        EXPECT_FLOAT_EQ(24.f, *outputC2->get<float>());
    }

    TEST_F(ALogicEngine_Linking, PropagatesValuesFromOutputScriptToMultipleInputScripts)
    {
        const auto  scriptSource = R"(
            function interface()
                IN.floatInput = FLOAT
                OUT.floatOutput = FLOAT
            end
            function run()
                OUT.floatOutput = IN.floatInput
            end
        )";

        auto scriptA = m_logicEngine.createLuaScriptFromSource(scriptSource, "ScriptA");
        auto scriptB = m_logicEngine.createLuaScriptFromSource(scriptSource, "ScriptB");
        auto scriptC = m_logicEngine.createLuaScriptFromSource(scriptSource, "ScriptC");

        auto inputA  = scriptA->getInputs()->getChild("floatInput");
        auto outputA = scriptA->getOutputs()->getChild("floatOutput");
        auto inputB  = scriptB->getInputs()->getChild("floatInput");
        auto outputB = scriptB->getOutputs()->getChild("floatOutput");
        auto inputC  = scriptC->getInputs()->getChild("floatInput");
        auto outputC = scriptC->getOutputs()->getChild("floatOutput");

        m_logicEngine.link(*outputA, *inputB);
        m_logicEngine.link(*outputA, *inputC);

        inputA->set<float>(42.f);

        m_logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *outputB->get<float>());
        EXPECT_FLOAT_EQ(42.f, *outputC->get<float>());
    }

    TEST_F(ALogicEngine_Linking, PropagatesOutputToMultipleScriptsWithMultipleInputs)
    {
        const auto  sourceScript = R"(
            function interface()
                IN.floatInput = FLOAT
                OUT.floatOutput = FLOAT
            end
            function run()
                OUT.floatOutput = IN.floatInput
            end
        )";
        const auto  targetScript = R"(
            function interface()
                IN.floatInput1 = FLOAT
                IN.floatInput2 = FLOAT
                OUT.floatOutput1 = FLOAT
                OUT.floatOutput2 = FLOAT
            end
            function run()
                OUT.floatOutput1 = IN.floatInput1
                OUT.floatOutput2 = IN.floatInput2
            end
        )";

        auto scriptA = m_logicEngine.createLuaScriptFromSource(sourceScript, "ScriptA");
        auto scriptB = m_logicEngine.createLuaScriptFromSource(targetScript, "ScriptB");
        auto scriptC = m_logicEngine.createLuaScriptFromSource(targetScript, "ScriptC");

        auto inputA  = scriptA->getInputs()->getChild("floatInput");
        auto outputA = scriptA->getOutputs()->getChild("floatOutput");

        auto inputB1  = scriptB->getInputs()->getChild("floatInput1");
        auto inputB2  = scriptB->getInputs()->getChild("floatInput2");
        auto outputB1 = scriptB->getOutputs()->getChild("floatOutput1");
        auto outputB2 = scriptB->getOutputs()->getChild("floatOutput2");
        auto inputC1  = scriptC->getInputs()->getChild("floatInput1");
        auto inputC2  = scriptC->getInputs()->getChild("floatInput2");
        auto outputC1 = scriptC->getOutputs()->getChild("floatOutput1");
        auto outputC2 = scriptC->getOutputs()->getChild("floatOutput2");

        m_logicEngine.link(*outputA, *inputB1);
        m_logicEngine.link(*outputA, *inputB2);
        m_logicEngine.link(*outputA, *inputC1);
        m_logicEngine.link(*outputA, *inputC2);

        inputA->set(42.f);

        m_logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *outputB1->get<float>());
        EXPECT_FLOAT_EQ(42.f, *outputB2->get<float>());
        EXPECT_FLOAT_EQ(42.f, *outputC1->get<float>());
        EXPECT_FLOAT_EQ(42.f, *outputC2->get<float>());
    }

    TEST_F(ALogicEngine_Linking, DoesNotPropagateValuesIfScriptIsDestroyed)
    {
        const auto  scriptSource = R"(
            function interface()
                IN.floatInput = FLOAT
                OUT.floatOutput = FLOAT
            end
            function run()
                OUT.floatOutput = IN.floatInput
            end
        )";

        auto scriptA = m_logicEngine.createLuaScriptFromSource(scriptSource, "ScriptA");
        auto scriptB = m_logicEngine.createLuaScriptFromSource(scriptSource, "ScriptB");
        auto scriptC = m_logicEngine.createLuaScriptFromSource(scriptSource, "ScriptC");

        auto inputA  = scriptA->getInputs()->getChild("floatInput");
        auto outputA = scriptA->getOutputs()->getChild("floatOutput");
        auto inputB  = scriptB->getInputs()->getChild("floatInput");
        auto outputB = scriptB->getOutputs()->getChild("floatOutput");
        auto inputC  = scriptC->getInputs()->getChild("floatInput");
        auto outputC = scriptC->getOutputs()->getChild("floatOutput");

        m_logicEngine.link(*outputA, *inputB);
        m_logicEngine.link(*outputB, *inputC);

        m_logicEngine.destroy(*scriptB);

        inputA->set(42.f);

        m_logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *outputA->get<float>());
        EXPECT_FLOAT_EQ(0.f,  *inputC->get<float>());
        EXPECT_FLOAT_EQ(0.f,  *outputC->get<float>());
    }

    TEST_F(ALogicEngine_Linking, LinksNestedPropertiesBetweenScripts)
    {
        const auto  srcScriptA = R"(
            function interface()
                OUT.output = STRING
                OUT.nested = {
                    str1 = STRING,
                    str2 = STRING
                }
            end
            function run()
                OUT.output = "foo"
                OUT.nested = {str1 = "str1", str2 = "str2"}
            end
        )";
        const auto  srcScriptB = R"(
            function interface()
                IN.input = STRING
                IN.nested = {
                    str1 = STRING,
                    str2 = STRING
                }
                OUT.concat_all = STRING
            end
            function run()
                OUT.concat_all = IN.input .. " {" .. IN.nested.str1 .. ", " .. IN.nested.str2 .. "}"
            end
        )";

        // Create scripts in reversed order to make it more likely that order will be wrong unless ordered by dependencies
        auto scriptB = m_logicEngine.createLuaScriptFromSource(srcScriptB, "ScriptB");
        auto scriptA = m_logicEngine.createLuaScriptFromSource(srcScriptA, "ScriptA");

        auto scriptAOutput = scriptA->getOutputs()->getChild("output");
        auto scriptAnested_str1 = scriptA->getOutputs()->getChild("nested")->getChild("str1");
        auto scriptAnested_str2 = scriptA->getOutputs()->getChild("nested")->getChild("str2");

        auto scriptBInput = scriptB->getInputs()->getChild("input");
        auto scriptBnested_str1 = scriptB->getInputs()->getChild("nested")->getChild("str1");
        auto scriptBnested_str2 = scriptB->getInputs()->getChild("nested")->getChild("str2");

        // Do a crossover link between nested property and non-nested property
        EXPECT_TRUE(m_logicEngine.link(*scriptAOutput, *scriptBnested_str1));
        EXPECT_TRUE(m_logicEngine.link(*scriptAnested_str1, *scriptBInput));
        EXPECT_TRUE(m_logicEngine.link(*scriptAnested_str2, *scriptBnested_str2));

        EXPECT_TRUE(m_logicEngine.update());

        auto scriptB_concatenated = scriptB->getOutputs()->getChild("concat_all");
        EXPECT_EQ(std::string("str1 {foo, str2}"), *scriptB_concatenated->get<std::string>());
    }

    TEST_F(ALogicEngine_Linking, LinksNestedScriptPropertiesToBindingInputs)
    {
        const auto  scriptSrc = R"(
            function interface()
                OUT.nested = {
                    bool = BOOL,
                    vec3f = VEC3F
                }
            end
            function run()
                OUT.nested = {bool = false, vec3f = {0.1, 0.2, 0.3}}
            end
        )";

        auto script = m_logicEngine.createLuaScriptFromSource(scriptSrc);
        // TODO Violin add appearance binding here too, once test PR #305 is merged
        auto nodeBinding = m_logicEngine.createRamsesNodeBinding("NodeBinding");

        auto nestedOutput_bool = script->getOutputs()->getChild("nested")->getChild("bool");
        auto nestedOutput_vec3f = script->getOutputs()->getChild("nested")->getChild("vec3f");

        auto nodeBindingInput_bool = nodeBinding->getInputs()->getChild("visibility");
        auto nodeBindingInput_vec3f = nodeBinding->getInputs()->getChild("translation");

        ASSERT_TRUE(m_logicEngine.link(*nestedOutput_bool, *nodeBindingInput_bool));
        ASSERT_TRUE(m_logicEngine.link(*nestedOutput_vec3f, *nodeBindingInput_vec3f));

        ASSERT_TRUE(m_logicEngine.update());

        EXPECT_EQ(false, *nodeBindingInput_bool->get<bool>());
        EXPECT_THAT(*nodeBindingInput_vec3f->get<vec3f>(), ::testing::ElementsAre(0.1f, 0.2f, 0.3f));
    }

    TEST_F(ALogicEngine_Linking, PropagatesValuesCorrectlyAfterUnlink)
    {
        /*
         *            --> ScriptB
         *          /            \
         *  ScriptA ---------------->ScriptC
         */

        LogicEngine logicEngine;
        const auto  sourceScript = R"(
            function interface()
                IN.floatInput = FLOAT
                OUT.floatOutput = FLOAT
            end
            function run()
                OUT.floatOutput = IN.floatInput
            end
        )";
        const auto  targetScript = R"(
            function interface()
                IN.floatInput1 = FLOAT
                IN.floatInput2 = FLOAT
                OUT.floatOutput1 = FLOAT
                OUT.floatOutput2 = FLOAT
            end
            function run()
                OUT.floatOutput1 = IN.floatInput1
                OUT.floatOutput2 = IN.floatInput2
            end
        )";

        auto scriptA = logicEngine.createLuaScriptFromSource(sourceScript, "ScriptA");
        auto scriptB = logicEngine.createLuaScriptFromSource(sourceScript, "ScriptB");
        auto scriptC = logicEngine.createLuaScriptFromSource(targetScript, "ScriptC");

        auto scriptAInput  = scriptA->getInputs()->getChild("floatInput");
        auto scriptAOutput = scriptA->getOutputs()->getChild("floatOutput");

        auto scriptBInput = scriptB->getInputs()->getChild("floatInput");
        auto scriptBOutput = scriptB->getOutputs()->getChild("floatOutput");

        auto scriptCInput1 = scriptC->getInputs()->getChild("floatInput1");
        auto scriptCInput2 = scriptC->getInputs()->getChild("floatInput2");
        auto scriptCOutput1 = scriptC->getOutputs()->getChild("floatOutput1");
        auto scriptCOutput2 = scriptC->getOutputs()->getChild("floatOutput2");

        logicEngine.link(*scriptAOutput, *scriptBInput);
        logicEngine.link(*scriptAOutput, *scriptCInput1);
        logicEngine.link(*scriptBOutput, *scriptCInput2);

        scriptAInput->set(42.f);

        logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *scriptCOutput1->get<float>());
        EXPECT_FLOAT_EQ(42.f, *scriptCOutput2->get<float>());

        /*
         *           ScriptB
         *                  \
         *  ScriptA ----------->ScriptC
         */
        logicEngine.unlink(*scriptAOutput, *scriptBInput);

        scriptBInput->set(23.f);

        logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *scriptCOutput1->get<float>());
        EXPECT_FLOAT_EQ(23.f, *scriptCOutput2->get<float>());
    }


    TEST_F(ALogicEngine_Linking, PreservesLinksBetweenScriptsAfterSavingAndLoadingFromFile)
    {
        {
            /*
             *            ->  ScriptB --
             *          /               \
             *  ScriptA ------------------> ScriptC
             */

            LogicEngine tmpLogicEngine;
            const auto  srcScriptAB = R"(
                function interface()
                    IN.input = STRING
                    OUT.output = STRING
                end
                function run()
                    OUT.output = "forward " .. tostring(IN.input)
                end
            )";
            const auto  srcScriptCsrc = R"(
                function interface()
                    IN.fromA = STRING
                    IN.fromB = STRING
                    OUT.concatenate_AB = STRING
                end
                function run()
                    OUT.concatenate_AB = "A: " .. IN.fromA .. " & B: " .. IN.fromB
                end
            )";

            // Create them in reversed order to make sure they are ordered wrongly if not ordered explicitly
            auto scriptC = tmpLogicEngine.createLuaScriptFromSource(srcScriptCsrc, "ScriptC");
            auto scriptB = tmpLogicEngine.createLuaScriptFromSource(srcScriptAB, "ScriptB");
            auto scriptA = tmpLogicEngine.createLuaScriptFromSource(srcScriptAB, "ScriptA");

            auto scriptAInput  = scriptA->getInputs()->getChild("input");
            auto scriptAOutput = scriptA->getOutputs()->getChild("output");

            auto scriptBInput = scriptB->getInputs()->getChild("input");
            auto scriptBOutput = scriptB->getOutputs()->getChild("output");

            auto scriptC_fromA = scriptC->getInputs()->getChild("fromA");
            auto scriptC_fromB = scriptC->getInputs()->getChild("fromB");
            auto scriptC_concatenate_AB = scriptC->getOutputs()->getChild("concatenate_AB");

            tmpLogicEngine.link(*scriptAOutput, *scriptBInput);
            tmpLogicEngine.link(*scriptAOutput, *scriptC_fromA);
            tmpLogicEngine.link(*scriptBOutput, *scriptC_fromB);

            scriptAInput->set<std::string>("'From A'");

            tmpLogicEngine.update();

            ASSERT_EQ(std::string("A: forward 'From A' & B: forward forward 'From A'"), *scriptC_concatenate_AB->get<std::string>());

            tmpLogicEngine.saveToFile("links.bin");
        }

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("links.bin"));

            // Internal check that deserialization did not result in more link copies
            const auto& links = m_logicEngine.m_impl->getLogicNodeConnector().getLinks();
            EXPECT_EQ(links.size(), 3u);

            // Load all scripts and their properties
            auto scriptC = findLuaScriptByName("ScriptC");
            auto scriptB = findLuaScriptByName("ScriptB");
            auto scriptA = findLuaScriptByName("ScriptA");

            auto scriptAInput = scriptA->getInputs()->getChild("input");
            auto scriptAOutput = scriptA->getOutputs()->getChild("output");

            auto scriptBInput = scriptB->getInputs()->getChild("input");
            auto scriptBOutput = scriptB->getOutputs()->getChild("output");

            auto scriptC_fromA = scriptC->getInputs()->getChild("fromA");
            auto scriptC_fromB = scriptC->getInputs()->getChild("fromB");
            auto scriptC_concatenate_AB = scriptC->getOutputs()->getChild("concatenate_AB");

            // Before update, values should be still as before saving
            EXPECT_EQ(std::string("forward 'From A'"), *scriptAOutput->get<std::string>());
            EXPECT_EQ(std::string("forward forward 'From A'"), *scriptBOutput->get<std::string>());
            EXPECT_EQ(std::string("A: forward 'From A' & B: forward forward 'From A'"), *scriptC_concatenate_AB->get<std::string>());

            EXPECT_TRUE(m_logicEngine.update());

            // Values should be still the same - because the data didn't change
            EXPECT_EQ(std::string("forward 'From A'"), *scriptAOutput->get<std::string>());
            EXPECT_EQ(std::string("forward forward 'From A'"), *scriptBOutput->get<std::string>());
            EXPECT_EQ(std::string("A: forward 'From A' & B: forward forward 'From A'"), *scriptC_concatenate_AB->get<std::string>());

            // Set different data manually
            EXPECT_TRUE(scriptAInput->set<std::string>("'A++'"));
            // these values should be overwritten by links
            EXPECT_TRUE(scriptBInput->set<std::string>("xxx"));
            EXPECT_TRUE(scriptC_fromA->set<std::string>("yyy"));
            EXPECT_TRUE(scriptC_fromB->set<std::string>("zzz"));

            EXPECT_TRUE(m_logicEngine.update());

            EXPECT_EQ(std::string("forward 'A++'"), *scriptAOutput->get<std::string>());
            EXPECT_EQ(std::string("forward forward 'A++'"), *scriptBOutput->get<std::string>());
            EXPECT_EQ(std::string("A: forward 'A++' & B: forward forward 'A++'"), *scriptC_concatenate_AB->get<std::string>());
        }

        // TODO Violin discuss moving removal of files to test fixtures dtor
        std::remove("links.bin");
    }

    TEST_F(ALogicEngine_Linking, PreservesNestedLinksBetweenScriptsAfterSavingAndLoadingFromFile)
    {
        {
            LogicEngine tmpLogicEngine;
            const auto  srcScriptA = R"(
                function interface()
                    IN.appendixNestedStr2 = STRING
                    OUT.output = STRING
                    OUT.nested = {
                        str1 = STRING,
                        str2 = STRING
                    }
                end
                function run()
                    OUT.output = "foo"
                    OUT.nested = {str1 = "str1", str2 = "str2" .. IN.appendixNestedStr2}
                end
            )";
            const auto  srcScriptB = R"(
                function interface()
                    IN.input = STRING
                    IN.nested = {
                        str1 = STRING,
                        str2 = STRING
                    }
                    OUT.concat_all = STRING
                end
                function run()
                    OUT.concat_all = IN.input .. " {" .. IN.nested.str1 .. ", " .. IN.nested.str2 .. "}"
                end
            )";

            // Create scripts in reversed order to make it more likely that order will be wrong unless ordered by dependencies
            auto scriptB = tmpLogicEngine.createLuaScriptFromSource(srcScriptB, "ScriptB");
            auto scriptA = tmpLogicEngine.createLuaScriptFromSource(srcScriptA, "ScriptA");

            auto scriptAOutput = scriptA->getOutputs()->getChild("output");
            auto scriptAnested_str1 = scriptA->getOutputs()->getChild("nested")->getChild("str1");
            auto scriptAnested_str2 = scriptA->getOutputs()->getChild("nested")->getChild("str2");

            auto scriptBInput = scriptB->getInputs()->getChild("input");
            auto scriptBnested_str1 = scriptB->getInputs()->getChild("nested")->getChild("str1");
            auto scriptBnested_str2 = scriptB->getInputs()->getChild("nested")->getChild("str2");

            // Do a crossover link between nested property and non-nested property
            ASSERT_TRUE(tmpLogicEngine.link(*scriptAOutput, *scriptBnested_str1));
            ASSERT_TRUE(tmpLogicEngine.link(*scriptAnested_str1, *scriptBInput));
            ASSERT_TRUE(tmpLogicEngine.link(*scriptAnested_str2, *scriptBnested_str2));

            ASSERT_TRUE(tmpLogicEngine.update());

            auto scriptB_concatenated = scriptB->getOutputs()->getChild("concat_all");
            ASSERT_EQ(std::string("str1 {foo, str2}"), *scriptB_concatenated->get<std::string>());

            tmpLogicEngine.saveToFile("nested_links.bin");
        }

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("nested_links.bin"));

            // Internal check that deserialization did not result in more link copies
            const auto& links = m_logicEngine.m_impl->getLogicNodeConnector().getLinks();
            EXPECT_EQ(links.size(), 3u);

            // Load all scripts and their properties
            auto scriptA = findLuaScriptByName("ScriptA");
            auto scriptB = findLuaScriptByName("ScriptB");

            auto scriptAOutput = scriptA->getOutputs()->getChild("output");
            auto scriptAnested_str1 = scriptA->getOutputs()->getChild("nested")->getChild("str1");
            auto scriptAnested_str2 = scriptA->getOutputs()->getChild("nested")->getChild("str2");

            auto scriptBInput = scriptB->getInputs()->getChild("input");
            auto scriptBnested_str1 = scriptB->getInputs()->getChild("nested")->getChild("str1");
            auto scriptBnested_str2 = scriptB->getInputs()->getChild("nested")->getChild("str2");
            auto scriptB_concatenated = scriptB->getOutputs()->getChild("concat_all");

            // Before update, values should be still as before saving
            EXPECT_EQ(std::string("foo"), *scriptAOutput->get<std::string>());
            EXPECT_EQ(std::string("str1"), *scriptAnested_str1->get<std::string>());
            EXPECT_EQ(std::string("str2"), *scriptAnested_str2->get<std::string>());
            EXPECT_EQ(std::string("str1"), *scriptBInput->get<std::string>());
            EXPECT_EQ(std::string("foo"), *scriptBnested_str1->get<std::string>());
            EXPECT_EQ(std::string("str2"), *scriptBnested_str2->get<std::string>());
            EXPECT_EQ(std::string("str1 {foo, str2}"), *scriptB_concatenated->get<std::string>());

            EXPECT_TRUE(m_logicEngine.update());

            // Values should be still the same - because the data didn't change
            EXPECT_EQ(std::string("str1 {foo, str2}"), *scriptB_concatenated->get<std::string>());

            // Set different data manually
            auto scriptAappendix = scriptA->getInputs()->getChild("appendixNestedStr2");
            EXPECT_TRUE(scriptAappendix->set<std::string>("!bar"));
            // these values should be overwritten by links
            EXPECT_TRUE(scriptBInput->set<std::string>("xxx"));
            EXPECT_TRUE(scriptBnested_str1->set<std::string>("yyy"));
            EXPECT_TRUE(scriptBnested_str2->set<std::string>("zzz"));

            EXPECT_TRUE(m_logicEngine.update());

            EXPECT_EQ(std::string("str1 {foo, str2!bar}"), *scriptB_concatenated->get<std::string>());
        }

        std::remove("nested_links.bin");
    }

    class ALogicEngine_Linking_WithBindings : public ALogicEngine_Linking
    {
    protected:
        ALogicEngine_Linking_WithBindings()
        {
            // TODO Violin clean this up once PR #305 went in
            std::array<const char*, 3> commandLineConfigForTest = { "test", "-l", "off" };
            ramses::RamsesFrameworkConfig frameworkConfig(static_cast<uint32_t>(commandLineConfigForTest.size()), commandLineConfigForTest.data());
            m_ramsesFramework = std::make_unique<ramses::RamsesFramework>(frameworkConfig);
            m_ramsesClient = m_ramsesFramework->createClient("TheClient");
            m_scene = m_ramsesClient->createScene(ramses::sceneId_t(1));
        }

        using ENodePropertyStaticIndex = rlogic::internal::ENodePropertyStaticIndex;

        static void ExpectValues(ramses::Node& node, ENodePropertyStaticIndex prop, vec3f expectedValues)
        {
            std::array<float, 3> values = { 0.0f, 0.0f, 0.0f };
            if (prop == ENodePropertyStaticIndex::Rotation)
            {
                node.getRotation(values[0], values[1], values[2]);
            }
            if (prop == ENodePropertyStaticIndex::Translation)
            {
                node.getTranslation(values[0], values[1], values[2]);
            }
            if (prop == ENodePropertyStaticIndex::Scaling)
            {
                node.getScaling(values[0], values[1], values[2]);
            }
            EXPECT_THAT(values, ::testing::ElementsAre(expectedValues[0], expectedValues[1], expectedValues[2]));
        }

        static void ExpectVec3f(ramses::Appearance& appearance, const char* uniformName, vec3f expectedValues)
        {
            std::array<float, 3> values = { 0.0f, 0.0f, 0.0f };
            ramses::UniformInput uniform;
            appearance.getEffect().findUniformInput(uniformName, uniform);
            appearance.getInputValueVector3f(uniform, values[0], values[1], values[2]);
            EXPECT_THAT(values, ::testing::ElementsAre(expectedValues[0], expectedValues[1], expectedValues[2]));
        }

        ramses::Effect& createTestEffect(std::string_view vertShader, std::string_view fragShader)
        {
            ramses::EffectDescription effectDesc;
            effectDesc.setVertexShader(vertShader.data());
            effectDesc.setFragmentShader(fragShader.data());
            return *m_scene->createEffect(effectDesc);
        }

        ramses::Appearance& createTestAppearance(ramses::Effect& effect)
        {
            return *m_scene->createAppearance(effect, "test appearance");
        }

        const std::string_view m_vertShader = R"(
            #version 300 es

            uniform highp vec3 uniform1;
            uniform highp vec3 uniform2;

            void main()
            {
                gl_Position = vec4(uniform1 + uniform2, 1.0);
            })";

        const std::string_view m_fragShader = R"(
            #version 300 es

            out lowp vec4 color;
            void main(void)
            {
                color = vec4(1.0, 0.0, 0.0, 1.0);
            })";

        std::unique_ptr<ramses::RamsesFramework> m_ramsesFramework;
        ramses::RamsesClient* m_ramsesClient = nullptr;
        ramses::Scene* m_scene = nullptr;
    };

    TEST_F(ALogicEngine_Linking_WithBindings, PreservesLinksToNodeBindingsAfterSavingAndLoadingFromFile)
    {
        auto ramsesNode1 = m_scene->createNode();
        auto ramsesNode2 = m_scene->createNode();

        ramsesNode1->setTranslation(1.1f, 1.2f, 1.3f);
        ramsesNode1->setRotation(2.1f, 2.2f, 2.3f);
        ramsesNode1->setScaling(3.1f, 3.2f, 3.3f);

        ramsesNode2->setTranslation(11.1f, 11.2f, 11.3f);

        {
            LogicEngine tmpLogicEngine;
            const auto  scriptSrc = R"(
                function interface()
                    OUT.vec3f = VEC3F
                    OUT.visibility = BOOL
                end
                function run()
                    OUT.vec3f = {100.0, 200.0, 300.0}
                    OUT.visibility = false
                end
            )";

            auto script = tmpLogicEngine.createLuaScriptFromSource(scriptSrc, "Script");
            auto nodeBinding1 = tmpLogicEngine.createRamsesNodeBinding("NodeBinding1");
            auto nodeBinding2 = tmpLogicEngine.createRamsesNodeBinding("NodeBinding2");
            nodeBinding1->setRamsesNode(ramsesNode1);
            nodeBinding2->setRamsesNode(ramsesNode2);

            auto scriptOutputVec3f = script->getOutputs()->getChild("vec3f");
            auto scriptOutputBool = script->getOutputs()->getChild("visibility");
            auto binding1TranslationInput = nodeBinding1->getInputs()->getChild("translation");
            auto binding2RotationInput = nodeBinding2->getInputs()->getChild("rotation");
            auto binding1VisibilityInput = nodeBinding1->getInputs()->getChild("visibility");

            ASSERT_TRUE(tmpLogicEngine.link(*scriptOutputBool, *binding1VisibilityInput));
            ASSERT_TRUE(tmpLogicEngine.link(*scriptOutputVec3f, *binding1TranslationInput));
            ASSERT_TRUE(tmpLogicEngine.link(*scriptOutputVec3f, *binding2RotationInput));

            ASSERT_TRUE(tmpLogicEngine.update());

            ASSERT_THAT(*binding1TranslationInput->get<vec3f>(), ::testing::ElementsAre(100.0f, 200.0f, 300.0f));
            ASSERT_THAT(*binding2RotationInput->get<vec3f>(), ::testing::ElementsAre(100.0f, 200.0f, 300.0f));
            ASSERT_EQ(false, *binding1VisibilityInput->get<bool>());

            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Rotation, { 2.1f, 2.2f, 2.3f });
            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Scaling, { 3.1f, 3.2f, 3.3f });
            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Translation, { 100.0f, 200.0f, 300.0f });
            EXPECT_EQ(ramsesNode1->getVisibility(), ramses::EVisibilityMode::Invisible);

            ExpectValues(*ramsesNode2, ENodePropertyStaticIndex::Rotation, { 100.0f, 200.0f, 300.0f });
            ExpectValues(*ramsesNode2, ENodePropertyStaticIndex::Scaling, { 1.0f, 1.0f, 1.0f });
            ExpectValues(*ramsesNode2, ENodePropertyStaticIndex::Translation, { 11.1f, 11.2f, 11.3f });
            EXPECT_EQ(ramsesNode2->getVisibility(), ramses::EVisibilityMode::Visible);

            tmpLogicEngine.saveToFile("binding_links.bin");
        }

        // Make sure loading of bindings doesn't do anything to the node until update() is called
        // To test that, we reset one node's properties to default
        ramsesNode1->setTranslation(0.0f, 0.0f, 0.0f);
        ramsesNode1->setRotation(0.0f, 0.0f, 0.0f);
        ramsesNode1->setScaling(1.0f, 1.0f, 1.0f);
        ramsesNode1->setVisibility(ramses::EVisibilityMode::Visible);

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("binding_links.bin", m_scene));

            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Rotation, { 0.0f, 0.0f, 0.0f });
            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Scaling, { 1.0f, 1.0f, 1.0f });
            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Translation, { 0.0f, 0.0f, 0.0f });
            EXPECT_EQ(ramsesNode1->getVisibility(), ramses::EVisibilityMode::Visible);

            auto nodeBinding1 = findRamsesNodeBindingByName("NodeBinding1");
            auto nodeBinding2 = findRamsesNodeBindingByName("NodeBinding2");

            auto binding1TranslationInput = nodeBinding1->getInputs()->getChild("translation");
            auto binding2RotationInput = nodeBinding2->getInputs()->getChild("rotation");
            auto notLinkedManualInputProperty = nodeBinding2->getInputs()->getChild("translation");
            auto bindingVisibilityInput = nodeBinding1->getInputs()->getChild("visibility");

            // These values should be overwritten by the link - set them to a different value to make sure that happens
            ASSERT_TRUE(binding1TranslationInput->set<vec3f>({ 99.0f, 99.0f, 99.0f }));
            ASSERT_TRUE(binding2RotationInput->set<vec3f>({ 99.0f, 99.0f, 99.0f }));
            ASSERT_TRUE(bindingVisibilityInput->set<bool>(true));
            // This should not be overwritten, but should keep the manual value instead
            ASSERT_TRUE(notLinkedManualInputProperty->set<vec3f>({100.0f, 101.0f, 102.0f}));
            EXPECT_TRUE(m_logicEngine.update());

            // These have default values
            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Rotation, { 0.0f, 0.0f, 0.0f });
            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Scaling, { 1.0f, 1.0f, 1.0f });
            // These came over the link
            ExpectValues(*ramsesNode1, ENodePropertyStaticIndex::Translation, { 100.0f, 200.0f, 300.0f });
            EXPECT_EQ(ramsesNode1->getVisibility(), ramses::EVisibilityMode::Invisible);

            // These came over the link
            ExpectValues(*ramsesNode2, ENodePropertyStaticIndex::Rotation, { 100.0f, 200.0f, 300.0f });
            // These came over manual set after loading
            ExpectValues(*ramsesNode2, ENodePropertyStaticIndex::Translation, { 100.0f, 101.0f, 102.0f });
            // These have default values
            ExpectValues(*ramsesNode2, ENodePropertyStaticIndex::Scaling, { 1.0f, 1.0f, 1.0f });
            EXPECT_EQ(ramsesNode2->getVisibility(), ramses::EVisibilityMode::Visible);
        }

        std::remove("binding_links.bin");
    }

    TEST_F(ALogicEngine_Linking_WithBindings, PreservesLinksToAppearanceBindingsAfterSavingAndLoadingFromFile)
    {
        ramses::Effect& effect = createTestEffect(m_vertShader, m_fragShader);
        auto& appearance1 = createTestAppearance(effect);
        auto& appearance2 = createTestAppearance(effect);
        ramses::UniformInput uniform1;
        ramses::UniformInput uniform2;
        appearance1.getEffect().findUniformInput("uniform1", uniform1);
        appearance1.getEffect().findUniformInput("uniform2", uniform2);

        appearance1.setInputValueVector3f(uniform1, 1.1f, 1.2f, 1.3f);
        appearance1.setInputValueVector3f(uniform2, 2.1f, 2.2f, 2.3f);
        appearance2.setInputValueVector3f(uniform1, 3.1f, 3.2f, 3.3f);
        appearance2.setInputValueVector3f(uniform2, 4.1f, 4.2f, 4.3f);

        {
            LogicEngine tmpLogicEngine;
            const auto  scriptSrc = R"(
                function interface()
                    OUT.uniform = VEC3F
                end
                function run()
                    OUT.uniform = {100.0, 200.0, 300.0}
                end
            )";

            auto script = tmpLogicEngine.createLuaScriptFromSource(scriptSrc, "Script");
            auto appBinding1 = tmpLogicEngine.createRamsesAppearanceBinding("AppBinding1");
            auto appBinding2 = tmpLogicEngine.createRamsesAppearanceBinding("AppBinding2");
            appBinding1->setRamsesAppearance(&appearance1);
            appBinding2->setRamsesAppearance(&appearance2);

            auto scriptOutput = script->getOutputs()->getChild("uniform");
            auto binding1uniform1 = appBinding1->getInputs()->getChild("uniform1");
            auto binding2uniform1 = appBinding2->getInputs()->getChild("uniform1");
            auto binding2uniform2 = appBinding2->getInputs()->getChild("uniform2");

            ASSERT_TRUE(tmpLogicEngine.link(*scriptOutput, *binding1uniform1));
            ASSERT_TRUE(tmpLogicEngine.link(*scriptOutput, *binding2uniform1));
            ASSERT_TRUE(tmpLogicEngine.link(*scriptOutput, *binding2uniform2));

            ASSERT_TRUE(tmpLogicEngine.update());

            ExpectVec3f(appearance1, "uniform1", { 100.0f, 200.0f, 300.0f });
            ExpectVec3f(appearance1, "uniform2", { 2.1f, 2.2f, 2.3f });
            ExpectVec3f(appearance2, "uniform1", { 100.0f, 200.0f, 300.0f });
            ExpectVec3f(appearance2, "uniform2", { 100.0f, 200.0f, 300.0f });

            tmpLogicEngine.saveToFile("binding_links.bin");
        }

        // Make sure loading of bindings doesn't do anything to the appearance until update() is called
        // To test that, we reset one appearance's properties to zeroes
        appearance1.setInputValueVector3f(uniform1, 0.0f, 0.0f, 0.0f);
        appearance1.setInputValueVector3f(uniform2, 0.0f, 0.0f, 0.0f);
        appearance2.setInputValueVector3f(uniform1, 0.0f, 0.0f, 0.0f);
        appearance2.setInputValueVector3f(uniform2, 0.0f, 0.0f, 0.0f);

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("binding_links.bin", m_scene));

            ExpectVec3f(appearance1, "uniform1", { 0.0f, 0.0f, 0.0f });
            ExpectVec3f(appearance1, "uniform2", { 0.0f, 0.0f, 0.0f });
            ExpectVec3f(appearance2, "uniform1", { 0.0f, 0.0f, 0.0f });
            ExpectVec3f(appearance2, "uniform2", { 0.0f, 0.0f, 0.0f });

            auto appBinding1 = findRamsesAppearanceBindingByName("AppBinding1");
            auto appBinding2 = findRamsesAppearanceBindingByName("AppBinding2");

            auto binding1uniform1 = appBinding1->getInputs()->getChild("uniform1");
            auto binding1uniform2 = appBinding1->getInputs()->getChild("uniform2");
            auto binding2uniform1 = appBinding2->getInputs()->getChild("uniform1");
            auto binding2uniform2 = appBinding2->getInputs()->getChild("uniform2");

            // These values should be overwritten by the link - set them to a different value to make sure that happens
            ASSERT_TRUE(binding1uniform1->set<vec3f>({ 99.0f, 99.0f, 99.0f }));
            // This should not be overwritten, but should keep the manual value instead, because no link points to it
            ASSERT_TRUE(binding1uniform2->set<vec3f>({ 100.0f, 101.0f, 102.0f }));
            // These values should be overwritten by the link - set them to a different value to make sure that happens
            ASSERT_TRUE(binding2uniform1->set<vec3f>({ 99.0f, 99.0f, 99.0f }));
            ASSERT_TRUE(binding2uniform2->set<vec3f>({ 99.0f, 99.0f, 99.0f }));
            EXPECT_TRUE(m_logicEngine.update());

            ExpectVec3f(appearance1, "uniform1", { 100.0f, 200.0f, 300.0f });
            ExpectVec3f(appearance1, "uniform2", { 100.0f, 101.0f, 102.0f });
            ExpectVec3f(appearance2, "uniform1", { 100.0f, 200.0f, 300.0f });
            ExpectVec3f(appearance2, "uniform2", { 100.0f, 200.0f, 300.0f });
        }

        std::remove("binding_links.bin");
    }

    TEST_F(ALogicEngine_Linking, ReturnsTrueIfLogicNodeIsLinked)
    {
        LogicEngine logicEngine;
        auto        scriptSource = R"(
            function interface()
                IN.input = {
                    inBool = BOOL
                }
                OUT.output = {
                    outBool = BOOL
                }
            end
            function run()
            end
        )";

        auto sourceScript = logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto middleScript = logicEngine.createLuaScriptFromSource(scriptSource, "MiddleScript");
        auto targetBinding = logicEngine.createRamsesNodeBinding("NodeBinding");

        auto sourceOutputBool = sourceScript->getOutputs()->getChild("output")->getChild("outBool");
        auto middleInputBool  = middleScript->getInputs()->getChild("input")->getChild("inBool");
        auto middleOutputBool = middleScript->getOutputs()->getChild("output")->getChild("outBool");
        auto targetInputBool  = targetBinding->getInputs()->getChild("visibility");

        logicEngine.link(*sourceOutputBool, *middleInputBool);
        logicEngine.link(*middleOutputBool, *targetInputBool);

        EXPECT_TRUE(logicEngine.isLinked(*sourceScript));
        EXPECT_TRUE(logicEngine.isLinked(*middleScript));
        EXPECT_TRUE(logicEngine.isLinked(*targetBinding));

        logicEngine.unlink(*middleOutputBool, *targetInputBool);

        EXPECT_TRUE(logicEngine.isLinked(*sourceScript));
        EXPECT_TRUE(logicEngine.isLinked(*middleScript));
        EXPECT_FALSE(logicEngine.isLinked(*targetBinding));

        logicEngine.unlink(*sourceOutputBool, *middleInputBool);

        EXPECT_FALSE(logicEngine.isLinked(*sourceScript));
        EXPECT_FALSE(logicEngine.isLinked(*middleScript));
        EXPECT_FALSE(logicEngine.isLinked(*targetBinding));
    }

    TEST_F(ALogicEngine_Linking, SetsTargetNodeToDirtyAfterLinking)
    {
        LogicEngine logicEngine;
        auto        scriptSource = R"(
            function interface()
                IN.input = BOOL
                OUT.output = BOOL
            end
            function run()
            end
        )";

        auto sourceScript  = logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto targetBinding = logicEngine.createRamsesNodeBinding("RamsesBinding");

        logicEngine.update();

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_FALSE(targetBinding->m_impl.get().isDirty());

        auto output = sourceScript->getOutputs()->getChild("output");
        auto input  = targetBinding->getInputs()->getChild("visibility");

        logicEngine.link(*output, *input);

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_TRUE(targetBinding->m_impl.get().isDirty());
    }

    TEST_F(ALogicEngine_Linking, SetsTargetNodeToDirtyAfterLinkingWithStructs)
    {
        LogicEngine logicEngine;
        auto scriptSource = R"(
            function interface()
                IN.struct = {
                    inBool = BOOL
                }
                OUT.struct = {
                    outBool = BOOL
                }
            end
            function run()
            end
        )";

        auto sourceScript  = logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto targetScript = logicEngine.createLuaScriptFromSource(scriptSource, "TargetScript");

        logicEngine.update();

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_FALSE(targetScript->m_impl.get().isDirty());

        auto output = sourceScript->getOutputs()->getChild("struct")->getChild("outBool");
        auto input = targetScript->getInputs()->getChild("struct")->getChild("inBool");

        logicEngine.link(*output, *input);

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_TRUE(targetScript->m_impl.get().isDirty());
    }

    TEST_F(ALogicEngine_Linking, SetsTargetNodeToDirtyAfterUnlink)
    {
        LogicEngine logicEngine;
        auto        scriptSource = R"(
            function interface()
                IN.input = BOOL
                OUT.output = BOOL
            end
            function run()
            end
        )";

        auto sourceScript = logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto targetBinding = logicEngine.createRamsesNodeBinding("RamsesBinding");

        auto output = sourceScript->getOutputs()->getChild("output");
        auto input  = targetBinding->getInputs()->getChild("visibility");

        logicEngine.link(*output, *input);

        logicEngine.update();

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_FALSE(targetBinding->m_impl.get().isDirty());

        logicEngine.unlink(*output, *input);

        EXPECT_FALSE(sourceScript->m_impl.get().isDirty());
        EXPECT_TRUE(targetBinding->m_impl.get().isDirty());
    }
}
