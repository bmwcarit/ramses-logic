//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include "ramses-logic/ELogMessageType.h"

#include <functional>
#include <string_view>

/**
 * Interface to interact with the internal logger. If you want to handle log messages by yourself, you can
 * register an own log handler function with rlogic::Logger::SetLogHandler, which is called each time
 * a log message is logged. In addition you can silence the standard output of the log messages
 */
namespace rlogic::Logger
{
    /**
    * The LogHandlerFunc can be used to implement an own log handler. The
    * function is called each time a log message occurs. After the call to the function
    * the message is invalid. If you want to keep the message, you can store it in a std::string.
    * E.g.
    * \code{.cpp}
    *   rlogic::Logger::SetLogHandler([](ElogMessageType msgType, std::string_view message){
    *       std::cout << message std::endl;
    *   });
    * \endcode
    */
    using LogHandlerFunc = std::function<void(ELogMessageType, std::string_view)>;

    /**
    * Sets a custom log handler function, which is called each time a log message occurs.
    *
    * @ param logHandlerFunc which is called for each log message
    */
    RLOGIC_API void SetLogHandler(const LogHandlerFunc& logHandlerFunc);

    /**
    * Sets the default logging to std::out to enabled or disabled
    *
    * @param loggingEnabled state for logging to std::out.
    */
    RLOGIC_API void SetDefaultLogging(bool loggingEnabled);
}
