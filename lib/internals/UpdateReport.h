//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include <vector>
#include <chrono>
#include <optional>
#include <array>

namespace rlogic
{
    class LogicNode;
}

namespace rlogic::internal
{
    using ReportTimeUnits = std::chrono::microseconds;
    using LogicNodesTimed = std::vector<std::pair<rlogic::LogicNode*, ReportTimeUnits>>;
    using LogicNodes = std::vector<rlogic::LogicNode*>;

    class UpdateReport
    {
    public:
        enum class ETimingSection
        {
            TotalUpdate = 0,
            TopologySort
        };

        void sectionStarted(ETimingSection section);
        void sectionFinished(ETimingSection section);
        void nodeExecutionStarted(LogicNode* node);
        void nodeExecutionFinished();
        void nodeSkippedExecution(LogicNode* node);
        inline void linksActivated(size_t activatedLinks)
        {
            m_activatedLinks += activatedLinks;
        }
        void clear();

        [[nodiscard]] const LogicNodesTimed& getNodesExecuted() const;
        [[nodiscard]] const LogicNodes& getNodesSkippedExecution() const;
        [[nodiscard]] ReportTimeUnits getSectionExecutionTime(ETimingSection section) const;
        [[nodiscard]] size_t getLinkActivations() const;

    private:
        using Clock = std::chrono::steady_clock;
        using TimePoint = std::chrono::time_point<Clock>;

        LogicNodesTimed m_nodesExecuted;
        LogicNodes m_nodesSkippedExecution;
        std::array<ReportTimeUnits, 2u> m_sectionExecutionTime = { ReportTimeUnits{ 0 } };
        size_t m_activatedLinks {0u};

        std::optional<TimePoint> m_nodeExecutionStarted;
        std::array<std::optional<TimePoint>, 2u> m_sectionStarted;
    };
}
