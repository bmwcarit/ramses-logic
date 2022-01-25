//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include <gmock/gmock.h>

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"

#include "LogicEngineTest_Base.h"
#include "impl/LogicEngineImpl.h"
#include "internals/ApiObjects.h"

#include "RamsesTestUtils.h"

namespace rlogic::internal
{
    class ALogicEngine_DirtinessBase : public ALogicEngineBase
    {
    protected:
        ApiObjects& m_apiObjects = { m_logicEngine.m_impl->getApiObjects() };

        const std::string_view m_minimal_script = R"(
            function interface()
                IN.data = INT
                OUT.data = INT
            end
            function run()
                OUT.data = IN.data
            end
        )";

        const std::string_view m_nested_properties_script = R"(
            function interface()
                IN.data = {
                    nested = INT
                }
                OUT.data = {
                    nested = INT
                }
            end
            function run()
                OUT.data.nested = IN.data.nested
            end
        )";
    };

    class ALogicEngine_Dirtiness : public ALogicEngine_DirtinessBase, public ::testing::Test
    {
    };

    TEST_F(ALogicEngine_Dirtiness, NotDirtyAfterConstruction)
    {
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, DirtyAfterCreatingScript)
    {
        m_logicEngine.createLuaScript(m_valid_empty_script);
        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, DirtyAfterCreatingNodeBinding)
    {
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, DirtyAfterCreatingAppearanceBinding)
    {
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "");
        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, DirtyAfterCreatingCameraBinding)
    {
        m_logicEngine.createRamsesCameraBinding(*m_camera, "");
        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, NotDirty_AfterCreatingObjectsAndCallingUpdate)
    {
        m_logicEngine.createLuaScript(m_valid_empty_script);
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "");
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_AfterSettingScriptInput)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_minimal_script);
        m_logicEngine.update();

        script->getInputs()->getChild("data")->set<int32_t>(5);

        EXPECT_TRUE(m_apiObjects.isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_AfterSettingNestedScriptInput)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_nested_properties_script);
        m_logicEngine.update();

        script->getInputs()->getChild("data")->getChild("nested")->set<int32_t>(5);

        EXPECT_TRUE(m_apiObjects.isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_WhenSettingBindingInputToDefaultValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.update();

        // zeroes is the default value
        binding->getInputs()->getChild("translation")->set<vec3f>({0, 0, 0});
        EXPECT_TRUE(m_apiObjects.isDirty());
        m_logicEngine.update();

        // Set different value, and then set again
        binding->getInputs()->getChild("translation")->set<vec3f>({1, 2, 3});
        m_logicEngine.update();
        binding->getInputs()->getChild("translation")->set<vec3f>({1, 2, 3});
        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_WhenSettingBindingInputToDifferentValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.update();

        // Set non-default value, and then set again to different value
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
        binding->getInputs()->getChild("translation")->set<vec3f>({ 11, 12, 13 });
        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    class ALogicEngine_DirtinessViaLink : public ALogicEngine_DirtinessBase, public ::testing::TestWithParam<bool>
    {
    protected:
        void link(const Property& src, const Property& dst)
        {
            if (GetParam())
            {
                EXPECT_TRUE(m_logicEngine.linkWeak(src, dst));
            }
            else
            {
                EXPECT_TRUE(m_logicEngine.link(src, dst));
            }
        }
    };

    INSTANTIATE_TEST_SUITE_P(
        ALogicEngine_DirtinessViaLink_TestInstances,
        ALogicEngine_DirtinessViaLink,
        ::testing::Values(false, true));

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions) TEST_P defines copy ctor/assign but not dtor
    TEST_P(ALogicEngine_DirtinessViaLink, Dirty_WhenAddingLink)
    {
        LuaScript* script1 = m_logicEngine.createLuaScript(m_minimal_script);
        LuaScript* script2 = m_logicEngine.createLuaScript(m_minimal_script);
        m_logicEngine.update();

        link(*script1->getOutputs()->getChild("data"), *script2->getInputs()->getChild("data"));
        EXPECT_TRUE(m_apiObjects.isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions) TEST_P defines copy ctor/assign but not dtor
    TEST_P(ALogicEngine_DirtinessViaLink, NotDirty_WhenRemovingLink)
    {
        LuaScript* script1 = m_logicEngine.createLuaScript(m_minimal_script);
        LuaScript* script2 = m_logicEngine.createLuaScript(m_minimal_script);
        link(*script1->getOutputs()->getChild("data"), *script2->getInputs()->getChild("data"));
        m_logicEngine.update();

        EXPECT_FALSE(m_apiObjects.isDirty());
        m_logicEngine.unlink(*script1->getOutputs()->getChild("data"), *script2->getInputs()->getChild("data"));

        EXPECT_FALSE(m_apiObjects.isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions) TEST_P defines copy ctor/assign but not dtor
    TEST_P(ALogicEngine_DirtinessViaLink, NotDirty_WhenRemovingNestedLink)
    {
        LuaScript* script1 = m_logicEngine.createLuaScript(m_nested_properties_script);
        LuaScript* script2 = m_logicEngine.createLuaScript(m_nested_properties_script);
        link(*script1->getOutputs()->getChild("data")->getChild("nested"), *script2->getInputs()->getChild("data")->getChild("nested"));
        m_logicEngine.update();

        EXPECT_FALSE(m_apiObjects.isDirty());
        m_logicEngine.unlink(*script1->getOutputs()->getChild("data")->getChild("nested"), *script2->getInputs()->getChild("data")->getChild("nested"));

        EXPECT_FALSE(m_apiObjects.isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    // Removing link does not mark things dirty, but setting value does
    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions) TEST_P defines copy ctor/assign but not dtor
    TEST_P(ALogicEngine_DirtinessViaLink, Dirty_WhenRemovingLink_AndSettingValueByCallingSetAfterwards)
    {
        LuaScript* script1 = m_logicEngine.createLuaScript(m_nested_properties_script);
        LuaScript* script2 = m_logicEngine.createLuaScript(m_nested_properties_script);
        m_logicEngine.update();

        link(*script1->getOutputs()->getChild("data")->getChild("nested"), *script2->getInputs()->getChild("data")->getChild("nested"));
        m_logicEngine.update();
        ASSERT_FALSE(m_apiObjects.isDirty());

        m_logicEngine.unlink(*script1->getOutputs()->getChild("data")->getChild("nested"), *script2->getInputs()->getChild("data")->getChild("nested"));
        script2->getInputs()->getChild("data")->getChild("nested")->set<int32_t>(5);
        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_WhenScriptHadRuntimeError)
    {
        const std::string_view scriptWithError = R"(
            function interface()
            end
            function run()
                error("Snag!")
            end
        )";

        m_logicEngine.createLuaScript(scriptWithError);
        EXPECT_FALSE(m_logicEngine.update());

        EXPECT_TRUE(m_apiObjects.isDirty());
    }

    // This is a bit of a special case, but an important one. If script A provides a value for script B and script A has an error
    // AFTER it set a new value to B, then script B will be dirty (and not executed!) until the error in script A was fixed
    TEST_F(ALogicEngine_Dirtiness, KeepsDirtynessStateOfDependentScript_UntilErrorInSourceScriptIsFixed)
    {
        const std::string_view scriptWithFixableError = R"(
            function interface()
                IN.triggerError = BOOL
                IN.data = INT
                OUT.data = INT
            end
            function run()
                OUT.data = IN.data
                if IN.triggerError then
                    error("Snag!")
                end
            end
        )";

        LuaScript* script1 = m_logicEngine.createLuaScript(scriptWithFixableError);
        LuaScript* script2 = m_logicEngine.createLuaScript(m_minimal_script);

        // No error -> have normal run -> nothing is dirty
        script1->getInputs()->getChild("triggerError")->set<bool>(false);
        m_logicEngine.link(*script1->getOutputs()->getChild("data"), *script2->getInputs()->getChild("data"));
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());

        // Trigger error -> keep in dirty state
        script1->getInputs()->getChild("triggerError")->set<bool>(true);
        EXPECT_FALSE(m_logicEngine.update());
        EXPECT_FALSE(m_logicEngine.update());
        EXPECT_TRUE(m_apiObjects.isDirty());
        // "fix" the error and set a value -> expect nothing is dirty and value was propagated
        script1->getInputs()->getChild("triggerError")->set<bool>(false);
        script1->getInputs()->getChild("data")->set<int32_t>(15);

        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
        EXPECT_EQ(15, *script2->getOutputs()->getChild("data")->get<int32_t>());
    }

    class ALogicEngine_BindingDirtiness : public ALogicEngine_Dirtiness
    {
    protected:
        const std::string_view m_bindningDataScript = R"(
            function interface()
                OUT.vec3f = VEC3F
            end
            function run()
                OUT.vec3f = {1, 2, 3}
            end
        )";

    };

    TEST_F(ALogicEngine_BindingDirtiness, NotDirtyAfterConstruction)
    {
        EXPECT_FALSE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, NotDirtyAfterCreatingScript)
    {
        m_logicEngine.createLuaScript(m_valid_empty_script);
        EXPECT_FALSE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, DirtyAfterCreatingNodeBinding)
    {
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        EXPECT_TRUE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, DirtyAfterCreatingAppearanceBinding)
    {
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "");
        EXPECT_TRUE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, NotDirty_AfterCreatingBindingsAndCallingUpdate)
    {
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "");
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, Dirty_WhenSettingBindingInputToDefaultValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.update();

        // zeroes is the default value
        binding->getInputs()->getChild("translation")->set<vec3f>({ 0, 0, 0 });
        EXPECT_TRUE(m_apiObjects.bindingsDirty());
        m_logicEngine.update();

        // Set different value, and then set again
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        m_logicEngine.update();
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        EXPECT_TRUE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, Dirty_WhenSettingBindingInputToDifferentValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.update();

        // Set non-default value, and then set again to different value
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.bindingsDirty());
        binding->getInputs()->getChild("translation")->set<vec3f>({ 11, 12, 13 });
        EXPECT_TRUE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, Dirty_WhenAddingLink)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_bindningDataScript);
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.update();

        m_logicEngine.link(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));
        EXPECT_TRUE(m_apiObjects.bindingsDirty());

        // After update - not dirty
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, NotDirty_WhenRemovingLink)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_bindningDataScript);
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        m_logicEngine.link(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));
        m_logicEngine.update();

        EXPECT_FALSE(m_apiObjects.isDirty());
        m_logicEngine.unlink(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));

        EXPECT_FALSE(m_apiObjects.bindingsDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.bindingsDirty());
    }

    // Special case, but worth testing as we want that bindings are always
    // executed when adding link, even if the link was just "re-added"
    TEST_F(ALogicEngine_BindingDirtiness, Dirty_WhenReAddingLink)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_bindningDataScript);
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation")));
        m_logicEngine.update();

        EXPECT_FALSE(m_apiObjects.isDirty());
        m_logicEngine.unlink(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));
        m_logicEngine.link(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));

        EXPECT_TRUE(m_apiObjects.isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.isDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, Dirty_WhenSettingDataToNestedAppearanceBindingInputs)
    {
        // Vertex shader with array -> results in nested binding inputs
        const std::string_view vertShader_array = R"(
            #version 300 es

            uniform highp vec4  vec4Array[2];

            void main()
            {
                gl_Position = vec4Array[1];
            })";

        const std::string_view fragShader_trivial = R"(
            #version 300 es

            out lowp vec4 color;
            void main(void)
            {
                color = vec4(1.0, 0.0, 0.0, 1.0);
            })";

        RamsesAppearanceBinding* binding = m_logicEngine.createRamsesAppearanceBinding(RamsesTestSetup::CreateTestAppearance(*m_scene, vertShader_array, fragShader_trivial), "");

        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.bindingsDirty());

        EXPECT_TRUE(binding->getInputs()->getChild("vec4Array")->getChild(0)->set<vec4f>({ .1f, .2f, .3f, .4f }));
        EXPECT_TRUE(m_apiObjects.bindingsDirty());

        m_logicEngine.update();
        EXPECT_FALSE(m_apiObjects.bindingsDirty());
    }
}

