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

#define LOG_FATAL(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::Fatal, __VA_ARGS__)

#define LOG_ERROR(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::Error, __VA_ARGS__)

#define LOG_WARN(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::Warn, __VA_ARGS__)

#define LOG_INFO(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::Info, __VA_ARGS__)

#define LOG_DEBUG(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::Debug, __VA_ARGS__)

#define LOG_TRACE(...) \
internal::LoggerImpl::GetInstance().log(rlogic::ELogMessageType::Trace, __VA_ARGS__)

namespace rlogic::internal
{
    static inline const char* GetLogMessageTypeString(ELogMessageType type)
    {
        switch (type)
        {
        case ELogMessageType::Off:
            assert(false && "Should never call this!");
            return "";
        case ELogMessageType::Fatal:
            return "FATAL ";
        case ELogMessageType::Error:
            return "ERROR";
        case ELogMessageType::Warn:
            return "WARN ";
        case ELogMessageType::Info:
            return "INFO ";
        case ELogMessageType::Debug:
            return "DEBUG";
        case ELogMessageType::Trace:
            return "TRACE";
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

        void setLogVerbosityLimit(ELogMessageType verbosityLimit);
        ELogMessageType getLogVerbosityLimit() const;
        void setLogHandler(Logger::LogHandlerFunc logHandlerFunc);
        void setDefaultLogging(bool loggingEnabled);

        static LoggerImpl& GetInstance();

    private:
        LoggerImpl() noexcept;

        [[nodiscard]] bool logMessageExceedsVerbosityLimit(ELogMessageType messageType) const
        {
            return (messageType > m_logVerbosityLimit);
        }

        Logger::LogHandlerFunc m_logHandler;
        bool                   m_defaultLogging = true;

        ELogMessageType m_logVerbosityLimit = ELogMessageType::Info;

    };

    template <typename... ARGS>
    inline void LoggerImpl::log(ELogMessageType messageType, const ARGS&... args)
    {
        // Early exit if log level exceeded, or no logger configured
        if (logMessageExceedsVerbosityLimit(messageType) || (!m_defaultLogging && !m_logHandler))
        {
            return;
        }

        const auto formattedMessage = fmt::format(args...);
        if (m_defaultLogging)
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
