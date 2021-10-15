//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/LuaModule.h"

#include "impl/LogicNodeImpl.h"

#include "WithTempDirectory.h"

#include <fstream>

namespace rlogic
{
    // There are more specific "create/destroy" tests in ApiObjects unit tests!
    class ALogicEngine_Factory : public ALogicEngine
    {
    protected:
        WithTempDirectory tempFolder;
    };

    TEST_F(ALogicEngine_Factory, ProducesErrorWhenCreatingEmptyScript)
    {
        const LuaScript* script = m_logicEngine.createLuaScript("");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, CreatesScriptFromValidLuaWithoutErrors)
    {
        const LuaScript* script = m_logicEngine.createLuaScript(m_valid_empty_script);
        ASSERT_TRUE(nullptr != script);
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, DestroysScriptWithoutErrors)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_valid_empty_script);
        ASSERT_TRUE(script);
        ASSERT_TRUE(m_logicEngine.destroy(*script));
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingScriptFromAnotherEngineInstance)
    {
        LogicEngine otherLogicEngine;
        auto script = otherLogicEngine.createLuaScript(m_valid_empty_script);
        ASSERT_TRUE(script);
        ASSERT_FALSE(m_logicEngine.destroy(*script));
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Can't find script in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, CreatesLuaModule)
    {
        const auto module = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "mymodule");
        ASSERT_NE(nullptr, module);
        EXPECT_TRUE(m_logicEngine.getErrors().empty());

        EXPECT_EQ(module, m_logicEngine.findLuaModule("mymodule"));
        ASSERT_EQ(1u, m_logicEngine.luaModules().size());
        EXPECT_EQ(module, *m_logicEngine.luaModules().cbegin());

        const auto& constLogicEngine = m_logicEngine;
        EXPECT_EQ(module, constLogicEngine.findLuaModule("mymodule"));
    }

    TEST_F(ALogicEngine_Factory, AllowsCreatingLuaModuleWithEmptyName)
    {
        EXPECT_NE(nullptr, m_logicEngine.createLuaModule(m_moduleSourceCode, {}, ""));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, AllowsCreatingLuaModuleWithNameContainingNonAlphanumericChars)
    {
        EXPECT_NE(nullptr, m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "!@#$"));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, AllowsCreatingLuaModuleWithDupliciteNameEvenIfSourceDiffers)
    {
        ASSERT_TRUE(m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "mymodule"));
        // same name and same source is OK
        EXPECT_TRUE(m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "mymodule"));

        // same name and different source is also OK
        EXPECT_TRUE(m_logicEngine.createLuaModule("return {}", {}, "mymodule"));
    }

    TEST_F(ALogicEngine_Factory, CanDestroyLuaModule)
    {
        LuaModule* module = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "mymodule");
        ASSERT_NE(nullptr, module);
        EXPECT_TRUE(m_logicEngine.destroy(*module));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
        EXPECT_FALSE(m_logicEngine.findLuaModule("mymodule"));
    }

    TEST_F(ALogicEngine_Factory, FailsToDestroyLuaModuleIfFromOtherLogicInstance)
    {
        LogicEngine otherLogic;
        LuaModule* module = otherLogic.createLuaModule(m_moduleSourceCode);
        ASSERT_NE(nullptr, module);

        EXPECT_FALSE(m_logicEngine.destroy(*module));
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_EQ(m_logicEngine.getErrors().front().message, "Can't find Lua module in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, FailsToDestroyLuaModuleIfUsedInLuaScript)
    {
        LuaModule* module = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "mymodule");
        ASSERT_NE(nullptr, module);

        constexpr std::string_view valid_empty_script = R"(
            modules("mymodule")
            function interface()
            end
            function run()
            end
        )";
        EXPECT_TRUE(m_logicEngine.createLuaScript(valid_empty_script, CreateDeps({ { "mymodule", module } }), "script"));

        EXPECT_FALSE(m_logicEngine.destroy(*module));
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_EQ(m_logicEngine.getErrors().front().message, "Failed to destroy LuaModule 'mymodule', it is used in LuaScript 'script'");
    }

    TEST_F(ALogicEngine_Factory, CanDestroyModuleAfterItIsNotUsedAnymore)
    {
        LuaModule* module = m_logicEngine.createLuaModule(m_moduleSourceCode);
        ASSERT_NE(nullptr, module);

        constexpr std::string_view valid_empty_script = R"(
            modules("mymodule")
            function interface()
            end
            function run()
            end
        )";
        auto script = m_logicEngine.createLuaScript(valid_empty_script, CreateDeps({ { "mymodule", module } }));
        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(m_logicEngine.destroy(*module));

        EXPECT_TRUE(m_logicEngine.destroy(*script));
        EXPECT_TRUE(m_logicEngine.destroy(*module));
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorWhenCreatingLuaScriptUsingModuleFromAnotherLogicInstance)
    {
        LogicEngine other;
        const auto module = other.createLuaModule(m_moduleSourceCode);
        ASSERT_NE(nullptr, module);

        EXPECT_EQ(nullptr, m_logicEngine.createLuaScript(m_valid_empty_script, CreateDeps({ { "name", module } })));
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_EQ(m_logicEngine.getErrors().front().message,
            "Failed to map Lua module 'name'! It was created on a different instance of LogicEngine.");
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorWhenCreatingLuaModuleUsingModuleFromAnotherLogicInstance)
    {
        LogicEngine other;
        const auto module = other.createLuaModule(m_moduleSourceCode);
        ASSERT_NE(nullptr, module);

        LuaConfig config;
        config.addDependency("name", *module);
        EXPECT_EQ(nullptr, m_logicEngine.createLuaModule(m_valid_empty_script, config));
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_EQ(m_logicEngine.getErrors().front().message,
            "Failed to map Lua module 'name'! It was created on a different instance of LogicEngine.");
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingRamsesNodeBindingFromAnotherEngineInstance)
    {
        LogicEngine otherLogicEngine;

        auto ramsesNodeBinding = otherLogicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        ASSERT_TRUE(ramsesNodeBinding);
        ASSERT_FALSE(m_logicEngine.destroy(*ramsesNodeBinding));
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Can't find RamsesNodeBinding in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingRamsesAppearanceBindingFromAnotherEngineInstance)
    {
        LogicEngine otherLogicEngine;
        auto binding = otherLogicEngine.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_FALSE(m_logicEngine.destroy(*binding));
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Can't find RamsesAppearanceBinding in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, DestroysRamsesCameraBindingWithoutErrors)
    {
        auto binding = m_logicEngine.createRamsesCameraBinding(*m_camera, "CameraBinding");
        ASSERT_TRUE(binding);
        ASSERT_TRUE(m_logicEngine.destroy(*binding));
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingRamsesCameraBindingFromAnotherEngineInstance)
    {
        LogicEngine otherLogicEngine;
        auto binding = otherLogicEngine.createRamsesCameraBinding(*m_camera, "CameraBinding");
        ASSERT_TRUE(binding);
        ASSERT_FALSE(m_logicEngine.destroy(*binding));
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Can't find RamsesCameraBinding in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, RenamesObjectsAfterCreation)
    {
        auto script = m_logicEngine.createLuaScript(m_valid_empty_script);
        auto ramsesNodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        auto ramsesAppearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        auto ramsesCameraBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "CameraBinding");

        script->setName("same name twice");
        ramsesNodeBinding->setName("same name twice");
        ramsesAppearanceBinding->setName("");
        ramsesCameraBinding->setName("");

        EXPECT_EQ("same name twice", script->getName());
        EXPECT_EQ("same name twice", ramsesNodeBinding->getName());
        EXPECT_EQ("", ramsesAppearanceBinding->getName());
        EXPECT_EQ("", ramsesCameraBinding->getName());
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorIfWrongObjectTypeIsDestroyed)
    {
        struct UnknownObjectImpl: internal::LogicNodeImpl
        {
            UnknownObjectImpl()
                : LogicNodeImpl("name")
            {
            }

            std::optional<internal::LogicNodeRuntimeError> update() override { return std::nullopt; }
        };

        struct UnknownObject : LogicNode
        {
            explicit UnknownObject(std::unique_ptr<UnknownObjectImpl> impl)
                : LogicNode(std::move(impl))
            {
            }
        };

        UnknownObject     unknownObject(std::make_unique<UnknownObjectImpl>());
        EXPECT_FALSE(m_logicEngine.destroy(unknownObject));
        const auto& errors = m_logicEngine.getErrors();
        EXPECT_EQ(1u, errors.size());
        EXPECT_EQ(errors[0].message, "Tried to destroy object 'name' with unknown type");
    }

    TEST_F(ALogicEngine_Factory, CanBeMoved)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_valid_empty_script, {}, "Script");
        RamsesNodeBinding* ramsesNodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        RamsesAppearanceBinding* appBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        RamsesCameraBinding* camBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "CameraBinding");

        LogicEngine movedLogicEngine(std::move(m_logicEngine));
        EXPECT_EQ(script, movedLogicEngine.findScript("Script"));
        EXPECT_EQ(ramsesNodeBinding, movedLogicEngine.findNodeBinding("NodeBinding"));
        EXPECT_EQ(appBinding, movedLogicEngine.findAppearanceBinding("AppearanceBinding"));
        EXPECT_EQ(camBinding, movedLogicEngine.findCameraBinding("CameraBinding"));

        movedLogicEngine.update();

        LogicEngine moveAssignedLogicEngine;
        moveAssignedLogicEngine = std::move(movedLogicEngine);

        EXPECT_EQ(script, moveAssignedLogicEngine.findScript("Script"));
        EXPECT_EQ(ramsesNodeBinding, moveAssignedLogicEngine.findNodeBinding("NodeBinding"));
        EXPECT_EQ(appBinding, moveAssignedLogicEngine.findAppearanceBinding("AppearanceBinding"));
        EXPECT_EQ(camBinding, moveAssignedLogicEngine.findCameraBinding("CameraBinding"));

        moveAssignedLogicEngine.update();
    }
}
