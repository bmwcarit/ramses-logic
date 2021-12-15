//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/LogicNode.h"
#include "internals/UpdateReport.h"

namespace rlogic::internal
{
    class LogicEngineReportImpl
    {
    public:
        LogicEngineReportImpl();
        explicit LogicEngineReportImpl(UpdateReport reportData);

        [[nodiscard]] const LogicNodesTimed& getNodesExecuted() const;
        [[nodiscard]] const LogicNodes& getNodesSkippedExecution() const;
        [[nodiscard]] std::chrono::microseconds getTopologySortExecutionTime() const;
        [[nodiscard]] std::chrono::microseconds getTotalUpdateExecutionTime() const;
        [[nodiscard]] size_t getTotalLinkActivations() const;

    private:
        UpdateReport m_reportData;
    };
}
