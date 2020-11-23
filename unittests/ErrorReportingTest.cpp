//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"

#include "internals/ErrorReporting.h"
#include "ramses-logic/Logger.h"

namespace rlogic::internal
{
    class AErrorReporting : public ::testing::Test
    {
    protected:
        AErrorReporting()
        {
            // Explicitly check that default logging does not affect custom error logs
            Logger::SetDefaultLogging(false);

            Logger::SetLogHandler([this](ELogMessageType type, std::string_view message) {
                EXPECT_EQ(ELogMessageType::ERROR, type);
                m_loggedErrors.emplace_back(std::string(message));
                });
        }

        void TearDown() override
        {
            // Unset custom logger to avoid interference with other tests which use logs
            Logger::SetLogHandler({});
        }

        ErrorReporting       m_errorReporting;

        std::vector<std::string> m_loggedErrors;
    };

    TEST_F(AErrorReporting, ProducesNoErrorsDuringConstruction)
    {
        EXPECT_EQ(0u, m_errorReporting.getErrors().size());
    }

    TEST_F(AErrorReporting, ProducesNoLogsDuringConstruction)
    {
        EXPECT_EQ(0u, m_loggedErrors.size());
    }

    TEST_F(AErrorReporting, StoresErrorsInTheOrderAdded)
    {
        m_errorReporting.add("error 1");
        m_errorReporting.add("error 2");

        EXPECT_THAT(m_errorReporting.getErrors(), ::testing::ElementsAre("error 1", "error 2"));
    }


    TEST_F(AErrorReporting, LogsErrorsInTheOrderAdded)
    {
        m_errorReporting.add("error 1");
        m_errorReporting.add("error 2");

        EXPECT_THAT(m_loggedErrors, ::testing::ElementsAre("error 1", "error 2"));
    }

    TEST_F(AErrorReporting, ClearsErrors)
    {
        m_errorReporting.add("error 1");

        ASSERT_EQ(1u, m_errorReporting.getErrors().size());

        m_errorReporting.clear();

        ASSERT_TRUE(m_errorReporting.getErrors().empty());
    }
}
