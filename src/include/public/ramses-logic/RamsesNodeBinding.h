//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include "ramses-logic/LogicNode.h"

#include <memory>

namespace ramses
{
    class Node;
}

namespace rlogic::internal
{
    class RamsesNodeBindingImpl;
}

namespace rlogic
{
    /**
     * The RamsesNodeBinding is a specialization of LogicNode which allows manipulation of Ramses nodes.
     *
     * The RamsesNodeBinding has a fixed set of inputs which correspond to properties of ramses::Node.
     * They have a fixed type and name:
     * \rst
     * 'visibility' (type bool)
     * 'rotation' (type vec3f)
     * 'translation' (type vec3f)
     * 'scaling' (type vec3f)
     \endrst
     *
     * The RamsesNodeBinding class has no output properties (thus getOutputs() will return nullptr) because
     * the outputs are implicitly the properties of the bound Ramses node.
     *
     * The RamsesNodeBinding is updated every time when rlogic::LogicEngine::update() is executed. The
     * RamsesNodeBinding propagates the values of its inputs to the Ramses node properties accordingly.
     *
     * \rst
    .. warning::

        In case no values were set (because the user neither set a value explicitly
        nor linked the input of rlogic::RamsesNodeBinding to another LogicNode output) the Ramses values
        are not touched. This is because the default values in Ramses are different than the default
        property values in the Logic Engine. For example, Ramses nodes have scaling as {1, 1, 1} by
        default, while Ramses Logic initializes vec3f properties to {0, 0, 0}. It is possible
        to set values directly to ramses::Node which will not be overwritten by rlogic::RamsesNodeBinding
        if you never explicitly assigned a value to the rlogic::RamsesNodeBinding inputs. You can also mix-and-match
        this behavior - assign some properties and others not.

     \endrst
     *
     *
     * The LogicEngine does not restrict which Scene the bound nodes belong to - it is possible to have
     * nodes from different scenes bound to the same LogicEngine, and vice-versa. The effects
     * on the ramses::Node property values which are bound to a rlogic::RamsesNodeBinding are immediately
     * visible after rlogic::LogicEngine::update() returns, however the user has to call ramses::Scene::flush()
     * explicitly based on their scene update logic and frame lifecycle.
     *
     */
    class RamsesNodeBinding : public LogicNode
    {
    public:
        /**
        * Sets the target Ramses node which is bound to this RamsesNodeBinding. Use nullptr as argument to unbind.
        * The Ramses Node is not modified until the next call to rlogic::LogicEngine::update().
        * After a Ramses Node is unbound, its properties are no longer overwritten by rlogic::RamsesNodeBinding, but their value
        * is also not restored to what it was before it was bound to - it keeps its current state.
        *
        * @param node the Ramses node to bind
        * @return true if successful, false otherwise
        */
        RLOGIC_API bool setRamsesNode(ramses::Node* node);

        /**
        * Returns the currently bound Ramses node, or nullptr if none is bound.
        *
        * @return the currently bound Ramses node
        */
        RLOGIC_API ramses::Node* getRamsesNode() const;

        /**
        * Constructor of RamsesNodeBinding. User is not supposed to call this - RamsesNodeBindings are created by other factory classes
        *
        * @param impl implementation details of the RamsesNodeBinding
        */
        explicit RamsesNodeBinding(std::unique_ptr<internal::RamsesNodeBindingImpl> impl) noexcept;

        /**
         * Destructor of RamsesNodeBinding.
         */
        ~RamsesNodeBinding() noexcept;

        /**
         * Copy Constructor of RamsesNodeBinding is deleted because RamsesNodeBindings are not supposed to be copied
         *
         * @param other RamsesNodeBindings to copy from
         */
        RamsesNodeBinding(const RamsesNodeBinding& other) = delete;

        /**
         * Move Constructor of RamsesNodeBinding is deleted because RamsesNodeBindings are not supposed to be moved
         *
         * @param other RamsesNodeBindings to move from
         */
        RamsesNodeBinding(RamsesNodeBinding&& other) = delete;

        /**
         * Assignment operator of RamsesNodeBinding is deleted because RamsesNodeBindings are not supposed to be copied
         *
         * @param other RamsesNodeBindings to assign from
         */
        RamsesNodeBinding& operator=(const RamsesNodeBinding& other) = delete;

        /**
         * Move assignment operator of RamsesNodeBinding is deleted because RamsesNodeBindings are not supposed to be moved
         *
         * @param other RamsesNodeBindings to assign from
         */
        RamsesNodeBinding& operator=(RamsesNodeBinding&& other) = delete;

        /**
         * Implementation detail of RamsesNodeBinding
         */
        std::unique_ptr<internal::RamsesNodeBindingImpl> m_nodeBinding;
    };
}
