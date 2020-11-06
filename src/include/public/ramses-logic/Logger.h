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
 * register your own log handler function with #rlogic::Logger::SetLogHandler, which is called each time
 * a log message is logged. In addition you can silence the standard output of the log messages
 */
namespace rlogic::Logger
{
    /**
    * The #LogHandlerFunc can be used to implement a custom log handler. The
    * function is called for each log message separately. After the call to the function
    * the string data behind std::string_view is deleted. If you want to keep it, you must
    * copy it e.g. to a std::string.
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
    * Note: setting a custom logger incurs a slight performance cost because log messages
    * will be assembled and reported, even if default logging is disabled (#SetDefaultLogging).
    *
    * @ param logHandlerFunc function which is called for each log message
    */
    RLOGIC_API void SetLogHandler(const LogHandlerFunc& logHandlerFunc);

    /**
    * Sets the default logging to std::out to enabled or disabled. Enabled by default.
    *
    * @param loggingEnabled true if you want to enable logging to std::out, false otherwise
    */
    RLOGIC_API void SetDefaultLogging(bool loggingEnabled);
}
