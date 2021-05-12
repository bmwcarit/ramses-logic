//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "generated/logicnode_gen.h"

namespace flatbuffers
{
    template<typename T> struct Offset;
    class FlatBufferBuilder;
}

namespace rlogic_serialization
{
    struct LogicNode;
}

namespace rlogic
{
    class Property;
}

namespace rlogic::internal
{
    class PropertyImpl;

    struct LogicNodeRuntimeError { std::string message; };

    class LogicNodeImpl
    {
    public:
        // Deleted methods must be public (other lifecycle methods are protected, this is a base class)
        LogicNodeImpl(const LogicNodeImpl& other) = delete;
        LogicNodeImpl& operator=(const LogicNodeImpl& other) = delete;

        [[nodiscard]] Property*       getInputs();
        [[nodiscard]] const Property* getInputs() const;

        [[nodiscard]] Property* getOutputs();
        [[nodiscard]] const Property* getOutputs() const;

        virtual std::optional<LogicNodeRuntimeError> update() = 0;

        [[nodiscard]] std::string_view getName() const;
        void setName(std::string_view name);

        void setDirty(bool dirty);
        [[nodiscard]] bool isDirty() const;

    protected:
        // Move-able (noexcept); Not copy-able
        explicit LogicNodeImpl(std::string_view name) noexcept;
        virtual ~LogicNodeImpl() noexcept;
        LogicNodeImpl(LogicNodeImpl&& other) noexcept = default;
        LogicNodeImpl& operator=(LogicNodeImpl && other) noexcept = default;

        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::LogicNode> Serialize(const LogicNodeImpl & logicNode, flatbuffers::FlatBufferBuilder & builder);
        // The deserialization code must be a constructor because LogicNodeImpl is a base class
        // TODO Violin put the code which deserializes property trees here as a static function, instead of each subclass having their own copy
        LogicNodeImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, std::unique_ptr<PropertyImpl> outputs);

    private:
        std::string               m_name;
        std::unique_ptr<Property> m_inputs;
        std::unique_ptr<Property> m_outputs;
        bool                      m_dirty = true;

    };
}
