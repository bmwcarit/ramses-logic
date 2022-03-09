//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/SaveFileConfigImpl.h"

namespace rlogic::internal
{
    void SaveFileConfigImpl::setMetadataString(std::string_view metadata)
    {
        m_metadata = metadata;
    }

    void SaveFileConfigImpl::setExporterVersion(uint32_t major, uint32_t minor, uint32_t patch, uint32_t fileFormatVersion)
    {
        m_exporterMajorVersion = major;
        m_exporterMinorVersion = minor;
        m_exporterPatchVersion = patch;
        m_exporterFileFormatVersion = fileFormatVersion;
    }

    uint32_t SaveFileConfigImpl::getExporterMajorVersion() const
    {
        return m_exporterMajorVersion;
    }

    uint32_t SaveFileConfigImpl::getExporterMinorVersion() const
    {
        return m_exporterMinorVersion;
    }

    uint32_t SaveFileConfigImpl::getExporterPatchVersion() const
    {
        return m_exporterPatchVersion;
    }

    uint32_t SaveFileConfigImpl::getExporterFileFormatVersion() const
    {
        return m_exporterFileFormatVersion;
    }

    const std::string& SaveFileConfigImpl::getMetadataString() const
    {
        return m_metadata;
    }

}
