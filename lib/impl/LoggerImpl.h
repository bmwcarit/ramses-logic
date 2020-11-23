//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/ELogMessageType.h"
#include "ramses-logic/Logger.h"

#include "fmt/format.h"

#include <iostream>

#define LOG_ERROR(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::ERROR, __VA_ARGS__)

#define LOG_WARN(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::WARNING, __VA_ARGS__)

#define LOG_INFO(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::INFO, __VA_ARGS__)

#define LOG_DEBUG(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::DEBUG, __VA_ARGS__)

namespace rlogic::internal
{
    static inline const char* GetLogMessageTypeString(ELogMessageType type)
    {
        switch (type)
        {
        case ELogMessageType::INFO:
            return "INFO ";
        case ELogMessageType::ERROR:
            return "ERROR";
        case ELogMessageType::WARNING:
            return "WARN ";
        case ELogMessageType::DEBUG:
            return "DEBUG";
        }
        assert(false);
        return "INFO ";
    }

    class LoggerImpl
    {
    public:
        ~LoggerImpl() noexcept = default;
        LoggerImpl(const LoggerImpl& other) = delete;
        LoggerImpl(LoggerImpl&& other) = delete;
        LoggerImpl& operator=(const LoggerImpl& other) = delete;
        LoggerImpl& operator=(LoggerImpl&& other) = delete;

        template<typename ...ARGS>
        void log(ELogMessageType messageType, const ARGS&... args);

        void setLogHandler(Logger::LogHandlerFunc logHandlerFunc);
        void setDefaultLogging(bool loggingEnabled);

        static LoggerImpl& GetInstance();

    private:
        LoggerImpl() noexcept;

        [[nodiscard]] bool logMessageExceedsAllowedLogLevel(ELogMessageType messageType) const
        {
            return (messageType > m_allowedLogLevel);
        }

        Logger::LogHandlerFunc m_logHandler;
        bool                   m_defaultLogging = true;

        // TODO Violin add API to set this
        const ELogMessageType m_allowedLogLevel = ELogMessageType::INFO;

    };

    template <typename... ARGS>
    inline void LoggerImpl::log(ELogMessageType messageType, const ARGS&... args)
    {
        if (!m_defaultLogging && !m_logHandler)
        {
            return;
        }

        const auto formattedMessage = fmt::format(args...);
        if (m_defaultLogging && !logMessageExceedsAllowedLogLevel(messageType))
        {
            std::cout << "[ " << GetLogMessageTypeString(messageType) << " ] " << formattedMessage << std::endl;
        }
        if (nullptr != m_logHandler)
        {
            m_logHandler(messageType, formattedMessage);
        }
    }

    inline LoggerImpl& LoggerImpl::GetInstance()
    {
        static LoggerImpl logger;
        return logger;
    }
}
