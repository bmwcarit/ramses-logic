//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "ramses-logic/Logger.h"
#include "ramses-logic/ELogMessageType.h"

#include "impl/LoggerImpl.h"

namespace rlogic
{
    TEST(ALogger, CanLogDifferentLogLevels)
    {
        internal::LoggerImpl::GetInstance().log(ELogMessageType::INFO, "Info Message");
        internal::LoggerImpl::GetInstance().log(ELogMessageType::ERROR, "Error Message");
        internal::LoggerImpl::GetInstance().log(ELogMessageType::WARNING, "Warning Message");
        internal::LoggerImpl::GetInstance().log(ELogMessageType::DEBUG, "Debug Message");
    }

    TEST(ALogger, CanLogFormattedMessage)
    {
        internal::LoggerImpl::GetInstance().log(ELogMessageType::INFO, "Info Message {}", 42);
        internal::LoggerImpl::GetInstance().log(ELogMessageType::ERROR, "Error Message {}", 42);
        internal::LoggerImpl::GetInstance().log(ELogMessageType::WARNING, "Warning Message {}", 42);
        internal::LoggerImpl::GetInstance().log(ELogMessageType::DEBUG, "Debug Message {}", 42);
    }

    TEST(ALogger, CanLogFormattedMessageWithMultipleArguments)
    {
        internal::LoggerImpl::GetInstance().log(ELogMessageType::INFO, "Info Message {} {} {}", 42, 42.0f, "42");
        internal::LoggerImpl::GetInstance().log(ELogMessageType::ERROR, "Error Message {} {} {}", 42, 42.0f, "42");
        internal::LoggerImpl::GetInstance().log(ELogMessageType::WARNING, "Warning Message {} {} {}", 42, 42.0f, "42");
        internal::LoggerImpl::GetInstance().log(ELogMessageType::DEBUG, "Debug Message {} {} {}", 42, 42.0f, "42");
    }

    TEST(ALogger, CanLogDifferentLogLevelsWithMacros)
    {
        internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::INFO, "Info message");
        LOG_INFO("Info message");
        LOG_ERROR("Error message");
        LOG_WARN("Warning message");
        LOG_DEBUG("Debug message");
    }

    TEST(ALogger, CanLogFormattedMessageWithMacros)
    {
        LOG_INFO("Info message {}", 42);
        LOG_ERROR("Error message {}", 42);
        LOG_WARN("Warning message {}", 42);
        LOG_DEBUG("Debug message {}", 42);
    }

    TEST(ALogger, CanLogFormattedMessageWithMultipleArgumentsWithMacros)
    {
        LOG_INFO("Info Message {} {} {}", 42, 42.0f, "42");
        LOG_ERROR("Error Message {} {} {}", 42, 42.0f, "42");
        LOG_WARN("Warning Message {} {} {}", 42, 42.0f, "42");
        LOG_DEBUG("Debug Message {} {} {}", 42, 42.0f, "42");
    }

    TEST(ALogger, SetsDefaultLoggingOffAndOnAgain)
    {
        Logger::SetDefaultLogging(false);
        LOG_INFO("Info Message {} {} {}", 42, 42.0f, "42");
        Logger::SetDefaultLogging(true);
        LOG_INFO("Info Message {} {} {}", 42, 42.0f, "43");
    }

    TEST(ALogger, CallsLogHandlerIfRegistered)
    {
        bool called = false;
        Logger::SetLogHandler([&called](ELogMessageType type, std::string_view message) {
            EXPECT_EQ(ELogMessageType::ERROR, type);
            EXPECT_EQ("Error message", message);
            called = true;
        });

        LOG_ERROR("Error message");
        EXPECT_TRUE(called);
        called = false;

        Logger::SetLogHandler([&called](ELogMessageType type, std::string_view message) {
            EXPECT_EQ(ELogMessageType::WARNING, type);
            EXPECT_EQ("Warn message", message);
            called = true;
        });

        LOG_WARN("Warn message");
        EXPECT_TRUE(called);
        called = false;

        Logger::SetLogHandler([&called](ELogMessageType type, std::string_view message) {
            EXPECT_EQ(ELogMessageType::DEBUG, type);
            EXPECT_EQ("Debug message", message);
            called = true;
        });

        LOG_DEBUG("Debug message");
        EXPECT_TRUE(called);
        called = false;

        Logger::SetLogHandler([&called](ELogMessageType type, std::string_view message) {
            EXPECT_EQ(ELogMessageType::INFO, type);
            EXPECT_EQ("Info message", message);
            called = true;
        });

        LOG_INFO("Info message");
        EXPECT_TRUE(called);

        // Can't "unset" a custom logger because of the lambda approach
        // Set to an empty lambda, otherwise this test influences other tests which trigger logs
        Logger::SetLogHandler([](ELogMessageType type, std::string_view message){
            (void)type;
            (void)message;
        });
    }
}
