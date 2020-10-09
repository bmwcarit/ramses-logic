//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RamsesTestUtils.h"

#include "internals/impl/LogicEngineImpl.h"
#include "internals/impl/RamsesAppearanceBindingImpl.h"
#include "internals/impl/PropertyImpl.h"
#include "internals/RamsesHelper.h"

#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/RamsesAppearanceBinding.h"

#include "ramses-client-api/Effect.h"
#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/UniformInput.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Appearance.h"

#include "ramses-utils.h"

namespace rlogic
{
    class ARamsesAppearanceBinding : public ::testing::Test
    {
    protected:
        RamsesAppearanceBinding& createAppearanceBindingForTest(std::string_view name, ramses::Appearance* ramsesAppearance = nullptr)
        {
            auto appBinding = m_logicEngine.createRamsesAppearanceBinding(name);
            if (ramsesAppearance)
            {
                appBinding->setRamsesAppearance(ramsesAppearance);
            }
            return *appBinding;
        }

        RamsesAppearanceBinding* findBindingByName(std::string_view name)
        {
            auto maybeBinding = std::find_if(m_logicEngine.ramsesAppearanceBindings().begin(), m_logicEngine.ramsesAppearanceBindings().end(),
                [name](const RamsesAppearanceBinding* binding)
                {
                    return binding->getName() == name;
                }
            );
            if (maybeBinding == m_logicEngine.ramsesAppearanceBindings().end())
            {
                return nullptr;
            }
            return *maybeBinding;
        }

        LogicEngine m_logicEngine;
    };

    TEST_F(ARamsesAppearanceBinding, HasANameAfterCreation)
    {
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");
        EXPECT_EQ("AppearanceBinding", appearanceBinding.getName());
    }

    TEST_F(ARamsesAppearanceBinding, HasEmptyInputsAfterCreation)
    {
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");
        EXPECT_EQ(appearanceBinding.getInputs()->getChildCount(), 0u);
        EXPECT_EQ(appearanceBinding.getInputs()->getType(), EPropertyType::Struct);
        EXPECT_EQ(appearanceBinding.getInputs()->getName(), "IN");
    }

    TEST_F(ARamsesAppearanceBinding, HasNoOutputsAfterCreation)
    {
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");
        EXPECT_EQ(nullptr, appearanceBinding.getOutputs());
    }

    TEST_F(ARamsesAppearanceBinding, ProducesNoErrorsDuringUpdate_IfNoRamsesAppearanceIsAssigned)
    {
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");
        EXPECT_TRUE(appearanceBinding.m_impl.get().update());
        EXPECT_TRUE(appearanceBinding.m_impl.get().getErrors().empty());
    }

    TEST_F(ARamsesAppearanceBinding, KeepsItsPropertiesAfterDeserialization_WhenNoRamsesLinksAndSceneProvided)
    {
        {
            createAppearanceBindingForTest("AppearanceBinding");
            ASSERT_TRUE(m_logicEngine.saveToFile("appearancebinding.bin"));
        }

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("appearancebinding.bin"));
            auto loadedAppearanceBinding = findBindingByName("AppearanceBinding");
            EXPECT_EQ(loadedAppearanceBinding->getRamsesAppearance(), nullptr);
            EXPECT_EQ(loadedAppearanceBinding->getInputs()->getChildCount(), 0u);
            EXPECT_EQ(loadedAppearanceBinding->getOutputs(), nullptr);
            EXPECT_EQ(loadedAppearanceBinding->getName(), "AppearanceBinding");
        }
        std::remove("appearancebinding.bin");
    }

    class ARamsesAppearanceBinding_WithRamses : public ARamsesAppearanceBinding
    {
    protected:
        ARamsesAppearanceBinding_WithRamses()
            : m_ramsesSceneIdWhichIsAlwaysTheSame(1)
            , m_scene(m_ramsesTestSetup.createScene(m_ramsesSceneIdWhichIsAlwaysTheSame))
        {
        }

        const std::string_view m_vertShader_simple = R"(
            #version 300 es

            uniform highp float floatUniform;

            void main()
            {
                gl_Position = floatUniform * vec4(1.0);
            })";

        const std::string_view m_vertShader_allTypes = R"(
            #version 300 es

            uniform highp float floatUniform;
            uniform highp int   intUniform;
            uniform highp ivec2 ivec2Uniform;
            uniform highp ivec3 ivec3Uniform;
            uniform highp ivec4 ivec4Uniform;
            uniform highp vec2  vec2Uniform;
            uniform highp vec3  vec3Uniform;
            uniform highp vec4  vec4Uniform;
            uniform highp vec4  vec4Uniform_shouldHaveDefaultValue;

            void main()
            {
                gl_Position = floatUniform * vec4(1.0);
            })";

        const std::string_view m_fragShader_trivial = R"(
            #version 300 es

            out lowp vec4 color;
            void main(void)
            {
                color = vec4(1.0, 0.0, 0.0, 1.0);
            })";

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

        void recreateRamsesScene()
        {
            m_ramsesTestSetup.destroyScene(*m_scene);
            m_scene = m_ramsesTestSetup.createScene(m_ramsesSceneIdWhichIsAlwaysTheSame);
        }

        void expectErrorWhenLoadingFile(std::string_view fileName, std::string_view errorMessage)
        {
            EXPECT_FALSE(m_logicEngine.loadFromFile(fileName, m_scene));
            ASSERT_EQ(1u, m_logicEngine.getErrors().size());
            EXPECT_EQ(std::string(errorMessage), m_logicEngine.getErrors()[0]);
        }

        RamsesTestSetup m_ramsesTestSetup;
        ramses::sceneId_t m_ramsesSceneIdWhichIsAlwaysTheSame;
        ramses::Scene* m_scene = nullptr;
    };

    TEST_F(ARamsesAppearanceBinding_WithRamses, ReturnsPointerToRamsesAppearance)
    {
        ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");

        EXPECT_EQ(nullptr, appearanceBinding.getRamsesAppearance());
        appearanceBinding.setRamsesAppearance(&appearance);
        EXPECT_EQ(&appearance, appearanceBinding.getRamsesAppearance());
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, HasInputsAfterSettingAppearance)
    {
        ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");

        appearanceBinding.setRamsesAppearance(&appearance);
        auto inputs = appearanceBinding.getInputs();

        ASSERT_EQ(1u, inputs->getChildCount());
        auto floatUniform = inputs->getChild(0);
        ASSERT_NE(nullptr, floatUniform);
        EXPECT_EQ("floatUniform", floatUniform->getName());
        EXPECT_EQ(EPropertyType::Float, floatUniform->getType());
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, MarksInputsAsInput)
    {
        ramses::Appearance& appearance        = createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));
        auto&               appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");

        appearanceBinding.setRamsesAppearance(&appearance);
        auto       inputs     = appearanceBinding.getInputs();
        const auto inputCount  = inputs->getChildCount();
        for (size_t i = 0; i < inputCount; ++i)
        {
            EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild(i)->m_impl->getInputOutputProperty());
        }
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, ClearsInputsAfterAppearanceIsSetToNull)
    {
        ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding", &appearance);
        appearanceBinding.setRamsesAppearance(nullptr);

        auto inputs = appearanceBinding.getInputs();
        EXPECT_EQ(0u, inputs->getChildCount());
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, CreatesOnlyInputsForSupportedUniformTypes)
    {

        const std::string_view fragShader_ManyUniformTypes = R"(
            #version 300 es

            // This is the same uniform like in the vertex shader - that's intended!
            uniform highp float floatUniform;
            // Other types, mixed up on purpose with some types which are not supported yet
            uniform highp vec2 u_vec2f;
            uniform highp sampler2D u_tex2d;
            //uniform highp samplerCube cubeTex;    // Not supported
            uniform highp vec4 u_vec4f;
            uniform highp sampler3D u_tex3d;        // Not supported
            uniform lowp int u_int;
            uniform highp samplerCube u_texCube;    // Not supported
            uniform mediump mat2 u_mat2;            // Not supported
            uniform mediump mat3 u_mat3;            // Not supported
            uniform mediump mat4 u_mat4;            // Not supported
            uniform highp ivec2 u_vec2i;

            out lowp vec4 color;
            void main(void)
            {
                color = vec4(floatUniform, 0.0, 0.0, 1.0);
                color.xy += u_vec2f;
                color += texture(u_tex2d, u_vec2f);
                color += texture(u_tex3d, vec3(u_vec2f, 1.0));
                color += texture(u_texCube, vec3(u_vec2f, 1.0));
                color.xy += vec2(float(u_vec2i.x), float(u_vec2i.y));
            })";

        ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_simple, fragShader_ManyUniformTypes));
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding", &appearance);

        const auto inputs = appearanceBinding.getInputs();
        ASSERT_EQ(5u, inputs->getChildCount());
        EXPECT_EQ("floatUniform", inputs->getChild(0)->getName());
        EXPECT_EQ(EPropertyType::Float, inputs->getChild(0)->getType());
        EXPECT_EQ("u_vec2f", inputs->getChild(1)->getName());
        EXPECT_EQ(EPropertyType::Vec2f, inputs->getChild(1)->getType());
        EXPECT_EQ("u_vec4f", inputs->getChild(2)->getName());
        EXPECT_EQ(EPropertyType::Vec4f, inputs->getChild(2)->getType());
        EXPECT_EQ("u_int", inputs->getChild(3)->getName());
        EXPECT_EQ(EPropertyType::Int32, inputs->getChild(3)->getType());
        EXPECT_EQ("u_vec2i", inputs->getChild(4)->getName());
        EXPECT_EQ(EPropertyType::Vec2i, inputs->getChild(4)->getType());
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, UpdatesAppearanceIfInputValuesWereSet)
    {
        ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_allTypes, m_fragShader_trivial));
        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding", &appearance);
        auto inputs = appearanceBinding.getInputs();
        ASSERT_TRUE(inputs->getChild("floatUniform")->set(42.42f));
        ASSERT_TRUE(inputs->getChild("intUniform")->set(42));
        ASSERT_TRUE(inputs->getChild("vec2Uniform")->set<vec2f>({ 0.1f, 0.2f }));
        ASSERT_TRUE(inputs->getChild("vec3Uniform")->set<vec3f>({ 1.1f, 1.2f, 1.3f }));
        ASSERT_TRUE(inputs->getChild("vec4Uniform")->set<vec4f>({ 2.1f, 2.2f, 2.3f, 2.4f }));
        ASSERT_TRUE(inputs->getChild("ivec2Uniform")->set<vec2i>({ 1, 2 }));
        ASSERT_TRUE(inputs->getChild("ivec3Uniform")->set<vec3i>({ 3, 4, 5 }));
        ASSERT_TRUE(inputs->getChild("ivec4Uniform")->set<vec4i>({ 6, 7, 8, 9 }));

        ASSERT_TRUE(appearanceBinding.m_impl.get().update());

        ramses::UniformInput uniform;
        {
            float resultFloat = 0.f;
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("floatUniform", uniform));
            appearance.getInputValueFloat(uniform, resultFloat);
            EXPECT_FLOAT_EQ(42.42f, resultFloat);
        }
        {
            int32_t resultInt32 = 0;
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("intUniform", uniform));
            appearance.getInputValueInt32(uniform, resultInt32);
            EXPECT_EQ(42, resultInt32);
        }
        {
            std::array<float, 2> result = { 0.0f, 0.0f };
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("vec2Uniform", uniform));
            appearance.getInputValueVector2f(uniform, result[0], result[1]);
            EXPECT_THAT(result, ::testing::ElementsAre(0.1f, 0.2f));
        }
        {
            std::array<float, 3> result = { 0.0f, 0.0f, 0.0f };
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("vec3Uniform", uniform));
            appearance.getInputValueVector3f(uniform, result[0], result[1], result[2]);
            EXPECT_THAT(result, ::testing::ElementsAre(1.1f, 1.2f, 1.3f));
        }
        {
            std::array<float, 4> result = { 0.0f, 0.0f, 0.0f, 0.0f };
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("vec4Uniform", uniform));
            appearance.getInputValueVector4f(uniform, result[0], result[1], result[2], result[3]);
            EXPECT_THAT(result, ::testing::ElementsAre(2.1f, 2.2f, 2.3f, 2.4f));

            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("vec4Uniform_shouldHaveDefaultValue", uniform));
            appearance.getInputValueVector4f(uniform, result[0], result[1], result[2], result[3]);
            EXPECT_THAT(result, ::testing::ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
        }
        {
            std::array<int32_t, 2> result = { 0, 0 };
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("ivec2Uniform", uniform));
            appearance.getInputValueVector2i(uniform, result[0], result[1]);
            EXPECT_THAT(result, ::testing::ElementsAre(1, 2));
        }
        {
            std::array<int32_t, 3> result = { 0, 0, 0 };
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("ivec3Uniform", uniform));
            appearance.getInputValueVector3i(uniform, result[0], result[1], result[2]);
            EXPECT_THAT(result, ::testing::ElementsAre(3, 4, 5));
        }
        {
            std::array<int32_t, 4> result = { 0, 0, 0, 0 };
            ASSERT_EQ(ramses::StatusOK, appearance.getEffect().findUniformInput("ivec4Uniform", uniform));
            appearance.getInputValueVector4i(uniform, result[0], result[1], result[2], result[3]);
            EXPECT_THAT(result, ::testing::ElementsAre(6, 7, 8, 9));
        }
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, UpdatesItsInputsAfterADifferentRamsesAppearanceWasAssigned)
    {
        ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));

        const std::string_view verShaderDifferentUniformNames = R"(
            #version 300 es

            uniform highp float floatUniform;
            uniform highp int newUniform;

            void main()
            {
                gl_Position = float(newUniform) * vec4(1.0);
            })";

        ramses::Appearance& differentAppearance = createTestAppearance(createTestEffect(verShaderDifferentUniformNames, m_fragShader_trivial));

        auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding");

        appearanceBinding.setRamsesAppearance(&appearance);
        ASSERT_EQ(1u, appearanceBinding.getInputs()->getChildCount());
        EXPECT_EQ("floatUniform", appearanceBinding.getInputs()->getChild(0)->getName());

        const Property* inputsPointerBeforeAppearanceChanged = appearanceBinding.getInputs();
        Property* propertyPointerBeforeAppearanceChanged = appearanceBinding.getInputs()->getChild("floatUniform");

        // Can't compare pointers to make sure property is recreated, because we can't assume pointer address is
        // different. Use value comparison instead - when recreated, the property will receive a new value
        ASSERT_TRUE(propertyPointerBeforeAppearanceChanged->set<float>(0.5f));

        appearanceBinding.setRamsesAppearance(&differentAppearance);

        const Property* inputsPointerAfterAppearanceChanged = appearanceBinding.getInputs();
        const Property* recreatedProperty = inputsPointerAfterAppearanceChanged->getChild("floatUniform");
        const Property* newProperty = inputsPointerAfterAppearanceChanged->getChild("newUniform");

        ASSERT_EQ(2u, inputsPointerAfterAppearanceChanged->getChildCount());
        EXPECT_EQ("floatUniform", recreatedProperty->getName());
        EXPECT_EQ("newUniform", newProperty->getName());
        EXPECT_EQ(EPropertyType::Float, recreatedProperty->getType());
        EXPECT_EQ(EPropertyType::Int32, newProperty->getType());

        EXPECT_EQ(inputsPointerBeforeAppearanceChanged, inputsPointerAfterAppearanceChanged);

        EXPECT_FLOAT_EQ(0.0f, *recreatedProperty->get<float>());
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, ContainsItsInputsAfterDeserialization_WithoutReorderingThem)
    {
        std::vector<std::string> inputOrderBeforeSaving;

        ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_allTypes, m_fragShader_trivial));
        {
            auto& rAppearanceBinding = createAppearanceBindingForTest("AppearanceBinding", &appearance);
            auto inputs = rAppearanceBinding.getInputs();
            inputOrderBeforeSaving.reserve(inputs->getChildCount());

            for (size_t i = 0; i < inputs->getChildCount(); ++i)
            {
                inputOrderBeforeSaving.emplace_back(std::string(inputs->getChild(i)->getName()));
            }

            inputs->getChild("floatUniform")->set(42.42f);
            inputs->getChild("intUniform")->set(42);
            inputs->getChild("vec2Uniform")->set<vec2f>({ 0.1f, 0.2f });
            inputs->getChild("vec3Uniform")->set<vec3f>({ 1.1f, 1.2f, 1.3f });
            inputs->getChild("vec4Uniform")->set<vec4f>({ 2.1f, 2.2f, 2.3f, 2.4f });
            inputs->getChild("ivec2Uniform")->set<vec2i>({ 1, 2 });
            inputs->getChild("ivec3Uniform")->set<vec3i>({ 3, 4, 5 });
            inputs->getChild("ivec4Uniform")->set<vec4i>({ 6, 7, 8, 9 });
            ASSERT_TRUE(m_logicEngine.saveToFile("logic.bin"));
        }

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("logic.bin", m_scene));
            auto loadedAppearanceBinding = findBindingByName("AppearanceBinding");
            ASSERT_EQ(loadedAppearanceBinding->getRamsesAppearance()->getSceneObjectId(), appearance.getSceneObjectId());

            const auto& inputs = loadedAppearanceBinding->getInputs();
            ASSERT_EQ(9u, inputs->getChildCount());

            ASSERT_EQ(9u, inputs->getChildCount());

            // check order after deserialization
            for (size_t i = 0; i < inputOrderBeforeSaving.size(); ++i)
            {
                EXPECT_EQ(inputOrderBeforeSaving[i], inputs->getChild(i)->getName());
            }

            auto expectValues = [&inputs](){
                EXPECT_FLOAT_EQ(42.42f, *inputs->getChild("floatUniform")->get<float>());
                EXPECT_EQ(42, *inputs->getChild("intUniform")->get<int32_t>());
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("intUniform")->m_impl->getInputOutputProperty());
                EXPECT_THAT(*inputs->getChild("vec2Uniform")->get<vec2f>(), ::testing::ElementsAre(0.1f, 0.2f));
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("vec2Uniform")->m_impl->getInputOutputProperty());
                EXPECT_THAT(*inputs->getChild("vec3Uniform")->get<vec3f>(), ::testing::ElementsAre(1.1f, 1.2f, 1.3f));
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("vec3Uniform")->m_impl->getInputOutputProperty());
                EXPECT_THAT(*inputs->getChild("vec4Uniform")->get<vec4f>(), ::testing::ElementsAre(2.1f, 2.2f, 2.3f, 2.4f));
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("vec4Uniform")->m_impl->getInputOutputProperty());
                EXPECT_THAT(*inputs->getChild("vec4Uniform_shouldHaveDefaultValue")->get<vec4f>(), ::testing::ElementsAre(.0f, .0f, .0f, .0f));
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("vec4Uniform_shouldHaveDefaultValue")->m_impl->getInputOutputProperty());
                EXPECT_THAT(*inputs->getChild("ivec2Uniform")->get<vec2i>(), ::testing::ElementsAre(1, 2));
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("ivec2Uniform")->m_impl->getInputOutputProperty());
                EXPECT_THAT(*inputs->getChild("ivec3Uniform")->get<vec3i>(), ::testing::ElementsAre(3, 4, 5));
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("ivec3Uniform")->m_impl->getInputOutputProperty());
                EXPECT_THAT(*inputs->getChild("ivec4Uniform")->get<vec4i>(), ::testing::ElementsAre(6, 7, 8, 9));
                EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild("ivec4Uniform")->m_impl->getInputOutputProperty());
            };

            expectValues();

            EXPECT_TRUE(m_logicEngine.update());

            expectValues();
        }
        std::remove("logic.bin");
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, ContainsItsInputsAfterDeserialization_WhenRamsesSceneIsRecreatedBetweenSaveAndLoad)
    {
        std::vector<std::string> inputOrderBeforeSaving;

        // Enough to test ordering
        const std::string_view m_vertShader_threeUniforms = R"(
            #version 300 es

            uniform highp float floatUniform1;
            uniform highp float floatUniform2;
            uniform highp float floatUniform3;

            void main()
            {
                gl_Position = floatUniform1 * floatUniform2 * floatUniform3 * vec4(1.0);
            })";

        {
            ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_threeUniforms, m_fragShader_trivial));
            auto& rAppearanceBinding  = createAppearanceBindingForTest("AppearanceBinding", &appearance);
            auto inputs = rAppearanceBinding.getInputs();

            for (size_t i = 0; i < inputs->getChildCount(); ++i)
            {
                inputOrderBeforeSaving.emplace_back(std::string(inputs->getChild(i)->getName()));
            }

            inputs->getChild("floatUniform1")->set(42.42f);
            ASSERT_TRUE(m_logicEngine.saveToFile("logic.bin"));
        }

        // Create identical ramses scene, but a different instance (emulates save/load of ramses)
        recreateRamsesScene();
        ramses::Appearance& recreatedAppearance = createTestAppearance(createTestEffect(m_vertShader_threeUniforms, m_fragShader_trivial));

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("logic.bin", m_scene));
            auto loadedAppearanceBinding = findBindingByName("AppearanceBinding");
            ASSERT_EQ(loadedAppearanceBinding->getRamsesAppearance()->getSceneObjectId(), recreatedAppearance.getSceneObjectId());

            const auto& inputs = loadedAppearanceBinding->getInputs();
            ASSERT_EQ(3u, inputs->getChildCount());

            // check order after deserialization
            for (size_t i = 0; i < inputOrderBeforeSaving.size(); ++i)
            {
                EXPECT_EQ(inputOrderBeforeSaving[i], inputs->getChild(i)->getName());
            }

            EXPECT_FLOAT_EQ(42.42f, *inputs->getChild("floatUniform1")->get<float>());
        }
        std::remove("logic.bin");
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, ProducesErrorIfAppearanceDoesNotHaveSameAmountOfInputsThanSerializedAppearanceBinding)
    {
        {
            ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_allTypes, m_fragShader_trivial));
            auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding", &appearance);
            auto inputs = appearanceBinding.getInputs();

            inputs->getChild("floatUniform")->set(42.42f);
            inputs->getChild("intUniform")->set(42);
            inputs->getChild("vec2Uniform")->set<vec2f>({4.f, 2.f});
            inputs->getChild("vec3Uniform")->set<vec3f>({4.f, 2.f, 4.f});
            inputs->getChild("vec4Uniform")->set<vec4f>({4.f, 2.f, 4.f, 2.f});
            inputs->getChild("ivec2Uniform")->set<vec2i>({4, 2});
            inputs->getChild("ivec3Uniform")->set<vec3i>({4, 2, 4});
            inputs->getChild("ivec4Uniform")->set<vec4i>({ 4, 2, 4, 2 });
            ASSERT_TRUE(m_logicEngine.saveToFile("logic.bin"));
        }

        // Simulate that a difference appearance with the same ID was created, but with different inputs
        recreateRamsesScene();
        createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));

        expectErrorWhenLoadingFile("logic.bin",
            "Fatal error while loading from file: ramses appearance binding input (Name: intUniform) was not found in appearance 'test appearance'!)");
        EXPECT_EQ(nullptr, findBindingByName("AppearanceBinding"));
        std::remove("logic.bin");
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, ProducesErrorIfAppearanceInputsHasDifferentNamesThanSerializedAppearanceBinding)
    {
        {
            ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));
            auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding", &appearance);

            auto inputs = appearanceBinding.getInputs();

            inputs->getChild("floatUniform")->set(42.42f);
            ASSERT_TRUE(m_logicEngine.saveToFile("logic.bin"));
        }

        // Simulate that a difference appearance with the same ID was created, but with different in names
        recreateRamsesScene();

        const std::string_view m_vertShader_simple_with_renamed_uniform = R"(
            #version 300 es

            uniform highp float floatUniform_renamed;

            void main()
            {
                gl_Position = floatUniform_renamed * vec4(1.0);
            })";

        createTestAppearance(createTestEffect(m_vertShader_simple_with_renamed_uniform, m_fragShader_trivial));

        expectErrorWhenLoadingFile("logic.bin",
            "Fatal error while loading from file: ramses appearance binding input (Name: floatUniform) was not found in appearance 'test appearance'!)");
        EXPECT_EQ(nullptr, findBindingByName("AppearanceBinding"));
        std::remove("logic.bin");
    }

    TEST_F(ARamsesAppearanceBinding_WithRamses, ProducesErrorIfAppearanceInputsHasDifferentTypeThanSerializedAppearanceBinding)
    {
        {
            ramses::Appearance& appearance = createTestAppearance(createTestEffect(m_vertShader_simple, m_fragShader_trivial));
            auto& appearanceBinding = createAppearanceBindingForTest("AppearanceBinding", &appearance);
            auto inputs = appearanceBinding.getInputs();

            inputs->getChild("floatUniform")->set(42.42f);
            ASSERT_TRUE(m_logicEngine.saveToFile("logic.bin"));
        }

        // Simulate that a difference appearance with the same ID was created, but with different types for the same inputs
        recreateRamsesScene();

        const std::string_view m_vertShader_simple_with_different_type = R"(
            #version 300 es

            uniform highp vec2 floatUniform;

            void main()
            {
                gl_Position = floatUniform.x * vec4(1.0);
            })";

        createTestAppearance(createTestEffect(m_vertShader_simple_with_different_type, m_fragShader_trivial));

        expectErrorWhenLoadingFile("logic.bin",
            "Fatal error while loading from file: ramses appearance binding input (Name: floatUniform) is expected to be of type FLOAT, but instead it is VEC2F!)");
        EXPECT_EQ(nullptr, findBindingByName("AppearanceBinding"));
        std::remove("logic.bin");
    }
}
