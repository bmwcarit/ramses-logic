//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

#include "ramses-logic/Logger.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/ELogMessageType.h"

#include "impl/LoggerImpl.h"

#include "LogTestUtils.h"
#include "WithTempDirectory.h"
#include "RamsesTestUtils.h"

namespace rlogic
{
    class ALogicEngine_Validation : public ALogicEngine
    {
    protected:
    };

    TEST_F(ALogicEngine_Validation, LogsNoWarningsWhenSavingFile_WhenContentValid)
    {
        ScopedLogContextLevel logCollector{ ELogMessageType::Trace, [&](ELogMessageType type, std::string_view /*message*/)
            {
                if (type == ELogMessageType::Warn)
                {
                    FAIL() << "Should have no warnings!";
                }
            }
        };

        WithTempDirectory tmpDir;
        // Empty logic engine has ho warnings
        m_logicEngine.saveToFile("noWarnings.rlogic");
    }

    TEST_F(ALogicEngine_Validation, LogsWarningsWhenSavingFile_WhenContentHasValidationIssues)
    {
        std::vector<std::string> warnings;
        ScopedLogContextLevel logCollector{ ELogMessageType::Trace, [&](ELogMessageType type, std::string_view message)
            {
                if (type == ELogMessageType::Warn)
                {
                    warnings.emplace_back(message);
                }
        }
        };

        WithTempDirectory tmpDir;
        // Cause some validation issues on purpose
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        nodeBinding.getInputs()->getChild("scaling")->set<vec3f>({1.5f, 1.f, 1.f});

        m_logicEngine.saveToFile("noWarnings.rlogic");

        ASSERT_EQ(warnings.size(), 1u);
        EXPECT_EQ(warnings[0], "Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!");

        // Fixing the problems -> removes the warning
        warnings.clear();
        m_logicEngine.update();

        m_logicEngine.saveToFile("noWarnings.rlogic");

        EXPECT_TRUE(warnings.empty());
    }

    TEST_F(ALogicEngine_Validation, LogsNoContentWarningsWhenSavingFile_WhenContentHasValidationIssues_ButValidationIsDisabled)
    {
        std::vector<std::string> infoLogs;
        ScopedLogContextLevel logCollector{ ELogMessageType::Trace, [&](ELogMessageType type, std::string_view message)
            {
                if (type == ELogMessageType::Info)
                {
                    infoLogs.emplace_back(message);
                }
                else
                {
                    FAIL() << "Unexpected log!";
                }
        }
        };

        WithTempDirectory tmpDir;
        // Cause some validation issues on purpose
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        nodeBinding.getInputs()->getChild("scaling")->set<vec3f>({ 1.5f, 1.f, 1.f });

        SaveFileConfig conf;
        conf.setValidationEnabled(false);

        // Disabling the validation causes a warning
        ASSERT_EQ(infoLogs.size(), 1u);
        EXPECT_EQ(infoLogs[0], "Validation before saving was disabled during save*() calls! Possible content issues will not yield further warnings.");
        infoLogs.clear();

        // Content warning doesn't show up because disabled
        m_logicEngine.saveToFile("noWarnings.rlogic", conf);
    }

    TEST_F(ALogicEngine_Validation, ProducesWarningIfBindingValuesHaveDirtyValue)
    {
        // Create a binding in a "dirty" state - has a non-default value, but update() wasn't called and didn't pass the value to ramses
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding");
        nodeBinding->getInputs()->getChild("visibility")->set<bool>(false);

        // Expects warning
        auto warnings = m_logicEngine.validate();

        ASSERT_EQ(1u, warnings.size());
        EXPECT_EQ(warnings[0].message, "Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!");
        EXPECT_EQ(warnings[0].type, EWarningType::UnsafeDataState);
    }

    TEST_F(ALogicEngine_Validation, ProducesWarningIfInterfaceHasUnboundOutputs)
    {
        LuaInterface* intf = m_logicEngine.createLuaInterface(R"(
            function interface(IN,OUT)

                IN.param1 = Type:Int32()
                IN.param2 = {a=Type:Float(), b=Type:Int32()}

            end
        )", "intf name");
        ASSERT_NE(nullptr, intf);

        // Expects warning
        auto warnings = m_logicEngine.validate();

        ASSERT_EQ(3u, warnings.size());
        EXPECT_THAT(warnings, ::testing::Each(::testing::Field(&WarningData::message, ::testing::HasSubstr("Interface [intf name] has unlinked output"))));
        EXPECT_THAT(warnings, ::testing::Each(::testing::Field(&WarningData::type, ::testing::Eq(EWarningType::UnusedContent))));
    }

    TEST(GetVerboseDescriptionFunction, ReturnsCorrectString)
    {
        EXPECT_STREQ("Performance", GetVerboseDescription(EWarningType::Performance));
        EXPECT_STREQ("Unsafe Data State", GetVerboseDescription(EWarningType::UnsafeDataState));
        EXPECT_STREQ("Uninitialized Data", GetVerboseDescription(EWarningType::UninitializedData));
        EXPECT_STREQ("Precision Loss", GetVerboseDescription(EWarningType::PrecisionLoss));
        EXPECT_STREQ("Unused Content", GetVerboseDescription(EWarningType::UnusedContent));
        EXPECT_STREQ("Duplicate Content", GetVerboseDescription(EWarningType::DuplicateContent));
        EXPECT_STREQ("Other", GetVerboseDescription(EWarningType::Other));
    }
}
