//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

namespace rlogic
{
    /**
     * Modes determining what data to store when serializing #rlogic::LuaScript or #rlogic::LuaModule.
     * Each mode has different implications when it comes to exported file size, loading performance and compatibility.
     */
    enum class ELuaSavingMode
    {
        /// Will store only source code as provided when script/module is created.
        /// Will produce bigger file size and take longest to load (all code must be compiled from scratch) but does not rely on bytecode.
        /// **Important!** Scripts/modules saved with source code only will fail to load in some older releases of Ramses logic (for feature levels above 01).
        /// This mode is the only valid mode when using feature level #rlogic::EFeatureLevel_01.
        SourceCodeOnly,
        /// Will store only bytecode generated from source code at creation time (available for feature levels #rlogic::EFeatureLevel_02 and higher).
        /// Will produce the smallest file size and fast loading speed but fully relies on bytecode being compatible on target platform.
        ByteCodeOnly,
        /// Will store both source code and generated bytecode (available for feature levels #rlogic::EFeatureLevel_02 and higher).
        /// Will produce the largest file size but allow fast loading speed if bytecode is compatible on target platform
        /// and a fallback solution if not - Lua will be recompiled from source code.
        SourceAndByteCode
    };
}
