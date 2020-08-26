//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"

#include <vector>
#include <string>
#include <functional>

namespace rlogic::internal
{
    class LogicNodeImpl;
}

namespace rlogic
{
    class Property;

    class LogicNode
    {
    public:

        /**
         * Returns a tree like structure with the inputs of the LogicNode.
         *
         * Returns the root Property of the LogicNode (type Struct) which contains potentially
         * nested list of properties. The properties are different for the classes which derive from
         * LogicNode. Look at the documentation of each derived class for more information on the properties.             *
         * @return a tree like structure with the inputs of the LogicNode
         */
        // TODO this can not be null, actually. Should we make it a reference instead?
        RLOGIC_API Property* getInputs();

        /**
         * @copydoc getInputs()
         */
        RLOGIC_API const Property* getInputs() const;

        /**
         * Returns a tree like structure with the outputs of the LogicNode
         *
         * Returns the root Property of the LogicNode (type Struct) which contains potentially
         * nested list of properties. The properties are different for the classes which derive from
         * LogicNode. Look at the documentation of each derived class for more information on the properties.             *
         * @return a tree like structure with the outputs of the LogicNode
         */
        RLOGIC_API const Property* getOutputs() const;

        /**
         * Returns the name of this LogicNode. The name can be used to find the object after deserialization
         * at the LogicEngine
         * @return the name of the LogicNode
         */
        RLOGIC_API std::string_view getName() const;

        /**
        * Destructor of LogicNode
        */
        // TODO Clang-Tidy prefers to do the defaulting in this case in the header - Discuss with Tobias
        ~LogicNode() noexcept = default;

        /**
        * Copy Constructor of LogicNode is deleted because LogicNodes are not supposed to be copied
        *
        * @param other LogicNode to copy from
        */
        LogicNode(const LogicNode& other) = delete;

        /**
        * Move Constructor of LogicNode is deleted because LogicNodes are not supposed to be moved
        *
        * @param other LogicNode to move from
        */
        LogicNode(LogicNode&& other) = delete;

        /**
        * Assignment operator of LogicNode is deleted because LogicNodes are not supposed to be copied
        *
        * @param other LogicNode to assign from
        */
        LogicNode& operator=(const LogicNode& other) = delete;

        /**
        * Move assignment operator of LogicNode is deleted because LogicNodes are not supposed to be moved
        *
        * @param other LogicNode to move from
        */
        LogicNode& operator=(LogicNode&& other) = delete;

        /**
         * Implementation detail of LuaScript
         */
        std::reference_wrapper<internal::LogicNodeImpl> m_impl;
    protected:

        /**
         * Constructor of LogicNode. User is not supposed to call this - LogcNodes are created by subclasses
         *
         * @param impl implementation details of the LogicNode
         */
        explicit LogicNode(std::reference_wrapper<internal::LogicNodeImpl> impl) noexcept;

    };
}
