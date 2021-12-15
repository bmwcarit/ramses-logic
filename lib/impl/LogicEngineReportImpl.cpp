//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicEngineReportImpl.h"

namespace rlogic::internal
{
    LogicEngineReportImpl::LogicEngineReportImpl() = default;

    LogicEngineReportImpl::LogicEngineReportImpl(UpdateReport reportData)
        : m_reportData{ std::move(reportData) }
    {
    }

    const LogicNodesTimed& LogicEngineReportImpl::getNodesExecuted() const
    {
        return m_reportData.getNodesExecuted();
    }

    const LogicNodes& LogicEngineReportImpl::getNodesSkippedExecution() const
    {
        return m_reportData.getNodesSkippedExecution();
    }

    std::chrono::microseconds LogicEngineReportImpl::getTopologySortExecutionTime() const
    {
        return m_reportData.getSectionExecutionTime(UpdateReport::ETimingSection::TopologySort);
    }

    std::chrono::microseconds LogicEngineReportImpl::getTotalUpdateExecutionTime() const
    {
        return m_reportData.getSectionExecutionTime(UpdateReport::ETimingSection::TotalUpdate);
    }

    size_t LogicEngineReportImpl::getTotalLinkActivations() const
    {
        return m_reportData.getLinkActivations();
    }

}
