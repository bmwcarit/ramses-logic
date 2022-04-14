#  -------------------------------------------------------------------------
#  Copyright (C) 2022 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

file(GLOB headers *.h)

add_library(ramses-logic-utils INTERFACE)

target_sources(ramses-logic-utils INTERFACE ${headers})

add_library(rlogic::ramses-logic-utils ALIAS ramses-logic-utils)
