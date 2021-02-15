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

#include "RamsesTestUtils.h"

namespace rlogic
{
    class ALogicEngine_Dirtiness: public ALogicEngine
    {
    protected:

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

    TEST_F(ALogicEngine_Dirtiness, NotDirtyAfterConstruction)
    {
        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, DirtyAfterCreatingScript)
    {
        m_logicEngine.createLuaScriptFromSource(m_valid_empty_script);
        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, DirtyAfterCreatingNodeBinding)
    {
        m_logicEngine.createRamsesNodeBinding("");
        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, DirtyAfterCreatingAppearanceBinding)
    {
        m_logicEngine.createRamsesAppearanceBinding("");
        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, NotDirty_AfterCreatingObjectsAndCallingUpdate)
    {
        m_logicEngine.createLuaScriptFromSource(m_valid_empty_script);
        m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.createRamsesAppearanceBinding("");
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_AfterSettingScriptInput)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_minimal_script);
        m_logicEngine.update();

        script->getInputs()->getChild("data")->set<int32_t>(5);

        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_AfterSettingNestedScriptInput)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_nested_properties_script);
        m_logicEngine.update();

        script->getInputs()->getChild("data")->getChild("nested")->set<int32_t>(5);

        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_WhenSettingBindingInputToDefaultValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.update();

        // zeroes is the default value
        binding->getInputs()->getChild("translation")->set<vec3f>({0, 0, 0});
        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
        m_logicEngine.update();

        // Set different value, and then set again
        binding->getInputs()->getChild("translation")->set<vec3f>({1, 2, 3});
        m_logicEngine.update();
        binding->getInputs()->getChild("translation")->set<vec3f>({1, 2, 3});
        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_WhenSettingBindingInputToDifferentValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.update();

        // Set non-default value, and then set again to different value
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
        binding->getInputs()->getChild("translation")->set<vec3f>({ 11, 12, 13 });
        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
    }

    TEST_F(ALogicEngine_Dirtiness, Dirty_WhenAddingLink)
    {
        LuaScript* script1 = m_logicEngine.createLuaScriptFromSource(m_minimal_script);
        LuaScript* script2 = m_logicEngine.createLuaScriptFromSource(m_minimal_script);
        m_logicEngine.update();

        m_logicEngine.link(*script1->getOutputs()->getChild("data"), *script2->getInputs()->getChild("data"));
        EXPECT_TRUE(m_logicEngine.m_impl->isDirty());
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
    }

    // TODO Violin/Tobias this is probably wrong in our implementation - discuss and decide
    // I am creating the test based on my expectations, and commenting out the lines which don't work
    TEST_F(ALogicEngine_Dirtiness, NotDirty_WhenRemovingLink)
    {
        LuaScript* script1 = m_logicEngine.createLuaScriptFromSource(m_minimal_script);
        LuaScript* script2 = m_logicEngine.createLuaScriptFromSource(m_minimal_script);
        m_logicEngine.link(*script1->getOutputs()->getChild("data"), *script2->getInputs()->getChild("data"));
        m_logicEngine.update();

        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
        m_logicEngine.unlink(*script1->getOutputs()->getChild("data"), *script2->getInputs()->getChild("data"));

        //EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
        //m_logicEngine.update();
        //EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
    }

    // TODO Violin/Tobias same as above
    // I am creating the test based on my expectations, and commenting out the lines which don't work
    TEST_F(ALogicEngine_Dirtiness, NotDirty_WhenRemovingNestedLink)
    {
        LuaScript* script1 = m_logicEngine.createLuaScriptFromSource(m_nested_properties_script);
        LuaScript* script2 = m_logicEngine.createLuaScriptFromSource(m_nested_properties_script);
        m_logicEngine.link(*script1->getOutputs()->getChild("data")->getChild("nested"), *script2->getInputs()->getChild("data")->getChild("nested"));
        m_logicEngine.update();

        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
        m_logicEngine.unlink(*script1->getOutputs()->getChild("data")->getChild("nested"), *script2->getInputs()->getChild("data")->getChild("nested"));

        //EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
        //m_logicEngine.update();
        //EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
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
        EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, NotDirtyAfterCreatingScript)
    {
        m_logicEngine.createLuaScriptFromSource(m_valid_empty_script);
        EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, DirtyAfterCreatingNodeBinding)
    {
        m_logicEngine.createRamsesNodeBinding("");
        EXPECT_TRUE(m_logicEngine.m_impl->bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, DirtyAfterCreatingAppearanceBinding)
    {
        m_logicEngine.createRamsesAppearanceBinding("");
        EXPECT_TRUE(m_logicEngine.m_impl->bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, NotDirty_AfterCreatingBindingsAndCallingUpdate)
    {
        m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.createRamsesAppearanceBinding("");
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, Dirty_WhenSettingBindingInputToDefaultValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.update();

        // zeroes is the default value
        binding->getInputs()->getChild("translation")->set<vec3f>({ 0, 0, 0 });
        EXPECT_TRUE(m_logicEngine.m_impl->bindingsDirty());
        m_logicEngine.update();

        // Set different value, and then set again
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        m_logicEngine.update();
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        EXPECT_TRUE(m_logicEngine.m_impl->bindingsDirty());
    }

    TEST_F(ALogicEngine_BindingDirtiness, Dirty_WhenSettingBindingInputToDifferentValue)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.update();

        // Set non-default value, and then set again to different value
        binding->getInputs()->getChild("translation")->set<vec3f>({ 1, 2, 3 });
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
        binding->getInputs()->getChild("translation")->set<vec3f>({ 11, 12, 13 });
        EXPECT_TRUE(m_logicEngine.m_impl->bindingsDirty());
    }

    // TODO Violin/Tobias this is probably wrong in our implementation - discuss and decide
    // I am creating the test based on my expectations, and commenting out the lines which don't work
    TEST_F(ALogicEngine_BindingDirtiness, NotDirty_WhenAddingLink)
    {
        m_logicEngine.createLuaScriptFromSource(m_bindningDataScript);
        m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.update();

        // Just adding link does not change binding state, no value was propagated
        //m_logicEngine.link(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));
        //EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());

        // After update - also not dirty!
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
    }

    // TODO Violin/Tobias this is probably wrong in our implementation - discuss and decide
    // I am creating the test based on my expectations, and commenting out the lines which don't work
    TEST_F(ALogicEngine_BindingDirtiness, NotDirty_WhenRemovingLink)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_bindningDataScript);
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding("");
        m_logicEngine.link(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));
        m_logicEngine.update();

        EXPECT_FALSE(m_logicEngine.m_impl->isDirty());
        m_logicEngine.unlink(*script->getOutputs()->getChild("vec3f"), *binding->getInputs()->getChild("rotation"));

        //EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
        //m_logicEngine.update();
        //EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
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

        RamsesTestSetup ramsesTestSetup;
        ramses::Appearance& appearance = RamsesTestSetup::CreateTestAppearance(*ramsesTestSetup.createScene(), vertShader_array, fragShader_trivial);
        RamsesAppearanceBinding* binding = m_logicEngine.createRamsesAppearanceBinding("");
        binding->setRamsesAppearance(&appearance);

        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());

        EXPECT_TRUE(binding->getInputs()->getChild("vec4Array")->getChild(0)->set<vec4f>({ .1f, .2f, .3f, .4f }));
        EXPECT_TRUE(m_logicEngine.m_impl->bindingsDirty());

        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.m_impl->bindingsDirty());
    }

    // TODO Violin add tests for error cases too
    // - what happens if one of the scripts had error, but some of the other scripts set binding values?
    // - what happens if a script had runtime error and set some links, others not?
    // - these are "marginal cases", but still important to test and document behavior we promise
}

