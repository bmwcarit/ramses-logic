//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#if defined(_WIN32)
    #define RLOGIC_API_EXPORT __declspec(dllexport)
    #define RLOGIC_API_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    #define RLOGIC_API_EXPORT __attribute((visibility ("default")))
    #define RLOGIC_API_IMPORT
#else
    #error "Compiler unknown/not supported"
#endif

#if defined(RLOGIC_LINK_SHARED_EXPORT)
    #define RLOGIC_API RLOGIC_API_EXPORT
#elif defined(RLOGIC_LINK_SHARED_IMPORT)
    #define RLOGIC_API RLOGIC_API_IMPORT
#else
    #define RLOGIC_API
#endif
