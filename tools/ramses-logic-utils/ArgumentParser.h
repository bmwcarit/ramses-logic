//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------
#pragma once

#include <pair>
#include <string>
#include <vector>

namespace rlogic
{
    /**
     * @brief ArgumentParser class to split commandline arguments
     */
    class ArgumentParser
    {
    public:
        using Argument = std::pair<std::string, std::string>;

        ArgumentParser(int32_t argc, char const* const* argv)
        {
            Argument argument{};
            for (int32_t i=1; i < argc; ++i)
            {
                std::string arg(argv[i]);
                if (arg.rfind("-", 0) == 0 || arg.rfind("--", 0) == 0)
                {
                    if (argument.first.empty())
                    {
                        int32_t equalPos = static_cast<int32_t>(arg.find('='));
                        if (equalPos != std::string::npos)
                        {
                            auto value = arg.substr(equalPos + 1);
                            auto flag = arg.substr(0, equalPos);
                            m_arguments.push_back({flag, value});
                            argument = Argument();
                        }
                        else
                        {
                            argument.first = arg;
                        }
                    }
                    else
                    {
                        m_arguments.push_back(argument);
                        argument = Argument(arg, "");
                    }
                }
                else
                {
                    if (argument.first.empty())
                    {
                        m_unknown.push_back(arg);
                    }
                    else
                    {
                        argument.second = arg;
                        m_arguments.push_back(argument);
                        argument = Argument();
                    }
                }
            }
        }

        /**
         * Return value of an argument
         */
        std::string value(const std::vector<std::string>& args) const
        {
            auto result = std::find_if(m_arguments.begin(), m_arguments.end(), [&args](const Argument& element)
            {
                for (const auto& arg : args)
                {
                    if (element.first == arg)
                    {
                        return true;
                    }
                }
                return false;
            });
            if (result != m_arguments.end())
            {
                return (*result).second;
            }
            return {};
        }

        /**
         * Return list of unknown parameters
         */
        [[nodiscard]] const std::vector<std::string>& unknown() const
        {
            return m_unknown;
        }

    private:
        std::vector<Argument> m_arguments{};
        std::vector<std::string> m_unknown{};
    };
}
