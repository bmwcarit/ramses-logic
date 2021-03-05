//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "FileUtils.h"
#include "StdFilesystemWrapper.h"
#include <fstream>

namespace rlogic::internal
{
    bool FileUtils::SaveBinary(const std::string& filename, const uint8_t* binaryBuffer, size_t bufferLength)
    {
        std::ofstream fileStream(filename, std::ofstream::binary);
        if (!fileStream.is_open())
        {
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) This reinterpret-cast is needed because of signed/unsigned mismatch between flatbuffers and fstream methods
        fileStream.write(reinterpret_cast<const char*>(binaryBuffer), bufferLength);
        return !fileStream.bad();
    }

    std::optional<std::vector<char>> FileUtils::LoadBinary(const std::string& filename)
    {
        std::ifstream fileStream(filename, std::ifstream::binary);
        if (!fileStream.is_open())
        {
            return std::nullopt;
        }
        fileStream.seekg(0, std::ios::end);
        std::vector<char> byteBuffer(static_cast<size_t>(fileStream.tellg()));
        fileStream.seekg(0, std::ios::beg);
        fileStream.read(byteBuffer.data(), byteBuffer.size());
        if (fileStream.bad())
        {
            return std::nullopt;
        }

        return byteBuffer;
    }
}
