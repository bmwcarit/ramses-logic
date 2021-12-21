//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/StdFilesystemWrapper.h"
#include <iostream>
#include <vector>
#include <string>

struct Arguments
{
    Arguments(int argc, char const* const* argv)
    {
        // the last 3 arguments may be filenames (scenefile, logicfile, luafile)
        // but only the scenefile is mandatory: logicfile and luafile can be found
        // by naming convention if not explicitly specified

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) bounds are checked
        const std::vector<const char*> args(argv, argv + argc);

        const auto argsSize = args.size();

        fs::path maybeSceneFile;
        fs::path maybeLogicFile;
        fs::path maybeLuaFile;

        if (argsSize >= 4)
        {
            maybeSceneFile = args[argsSize - 3];
            maybeLogicFile = args[argsSize - 2];
            maybeLuaFile = args[argsSize - 1];
        }
        else if (argsSize >= 3)
        {
            maybeSceneFile = args[argsSize - 2];
            maybeLogicFile = args[argsSize - 1];
        }
        else if (argsSize >= 2)
        {
            maybeSceneFile = args[argsSize - 1];
        }

        if (fs::exists(maybeSceneFile))
        {
            sceneFile = maybeSceneFile.string();
            logicFile = GetOrFind(maybeLogicFile, maybeSceneFile, "rlogic");
            luaFile   = GetOrFind(maybeLuaFile, maybeSceneFile, "lua");
        }
        else if (fs::exists(maybeLogicFile))
        {
            sceneFile = maybeLogicFile.string();
            logicFile = GetOrFind(maybeLuaFile, maybeLogicFile, "rlogic");
            luaFile   = GetOrFind(fs::path(), maybeLogicFile, "lua");
        }
        else if (fs::exists(maybeLuaFile))
        {
            sceneFile = maybeLuaFile.string();
            logicFile = GetOrFind(fs::path(), maybeLuaFile, "rlogic");
            luaFile   = GetOrFind(fs::path(), maybeLuaFile, "lua");
        }

        const std::string argExec = "--exec=";
        const std::string argMultiSample = "--multi-sample=";

        for (const std::string argString : args)
        {
            if (argString == "--no-offscreen")
            {
                noOffscreen = true;
            }
            else if (argString.find(argMultiSample, 0) == 0)
            {
                std::string sampleValue = argString.substr(argString.find('=') + 1);
                multiSampleRate = std::stoi(sampleValue);
            }
            else if (argString.rfind(argExec, 0) == 0)
            {
                luaFunction = argString.substr(argExec.size());
            }
            else if ((argString.rfind("--width", 0) == 0) || (argString == "-w") || (argString.rfind("--height", 0) == 0) || (argString == "-h"))
            {
                autoDetectViewportSize = false;
            }
        }
    }

    [[nodiscard]] static std::string GetOrFind(const fs::path& preferred, fs::path existing, const std::string& extension)
    {
        std::string retval;
        if (fs::exists(preferred))
        {
            retval = preferred.string();
        }
        else
        {
            existing.replace_extension(extension);
            if (fs::exists(existing))
            {
                retval = existing.string();
            }
        }
        return retval;
    }

    [[nodiscard]] bool valid() const
    {
        return !sceneFile.empty() && !logicFile.empty() && multiSampleRate >= 0 && (multiSampleRate <= 2 || multiSampleRate == 4);
    }

    void printErrorMessage(std::ostream& out) const
    {
        if (sceneFile.empty())
        {
            out << "<ramsesfile> is missing" << std::endl;
        }
        else if (logicFile.empty())
        {
            out << "<logicfile> is missing" << std::endl;
        }
        else if (multiSampleRate < 0 || multiSampleRate > 4 || multiSampleRate == 3)
        {
            out << "invalid multi sampling rate" << std::endl;
        }

        out << std::endl;
    }

    static void printUsage(std::ostream& out)
    {
        out << "Usage: ramses-logic-viewer [options] <ramsesfile> [<logicfile> <luafile>]" << std::endl << std::endl
            << "Loads and shows a ramses scene from the <ramsesfile>." << std::endl
            << "<logicfile> and <luafile> are auto-resolved if matching files with *.rlogic and *.lua extensions " << std::endl
            << "are found in the same path as <ramsesfile>. (Explicit arguments override autodetection.)" << std::endl << std::endl
            << "Options:" << std::endl
            << "--no-offscreen" << std::endl
            << "  Renders the scene directly to the window's framebuffer. Screenshot size will be the current window size." << std::endl
            << "--multi-sample=<rate>" << std::endl
            << "  Instructs the renderer to apply multi-sampling. Valid rates are 1, 2 and 4." << std::endl
            << "--exec=<luaFunction>" << std::endl
            << "  Calls the given lua function and exits." << std::endl;
    }

    std::string sceneFile;
    std::string logicFile;
    std::string luaFile;

    bool noOffscreen = false;
    bool autoDetectViewportSize = true;
    uint32_t    multiSampleRate = 0;
    std::string luaFunction;
};

