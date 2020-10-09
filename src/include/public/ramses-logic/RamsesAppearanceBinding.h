//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include "ramses-logic/RamsesBinding.h"

#include <memory>

namespace ramses
{
    class Appearance;
}

namespace rlogic::internal
{
    class RamsesAppearanceBindingImpl;
}

namespace rlogic
{
    /**
     * The RamsesAppearanceBinding is a type of #rlogic::RamsesBinding which allows the #rlogic::LogicEngine to control instances
     * of ramses::Appearance. Currently, only shader uniforms are supported, except arrays and texture samplers.
     * RamsesAppearanceBinding's can be created with #rlogic::LogicEngine::createRamsesAppearanceBinding.
     *
     * Since the RamsesAppearanceBinding derives from #rlogic::RamsesBinding, it also provides the #rlogic::LogicNode::getInputs
     * and #rlogic::LogicNode::getOutputs method. For this particular implementation, the methods behave as follows:
     *  - #rlogic::LogicNode::getInputs: returns an empty struct with no child properties if no ramses::Appearance is currently assigned
     *  - #rlogic::LogicNode::getInputs: returns the inputs corresponding to the available shader uniforms of ramses::Appearance if such is assigned
     *  - #rlogic::LogicNode::getOutputs: returns always nullptr, because a #RamsesAppearanceBinding does not have outputs,
     *    it implicitly controls the ramses Appearance
     *
     * Warning: any references to #rlogic::Property objects are invalidated after a call to #setRamsesAppearance, even if the newly
     * assigned ramses::Appearance has properties with the same name and type!
     */
    class RamsesAppearanceBinding : public RamsesBinding
    {
    public:
        /**
         * Links this #RamsesAppearanceBinding with a ramses::Appearance. After this call, #rlogic::LogicNode::getInputs will
         * return a struct property with children equivalent to the uniform inputs of the provided ramses Appearance. Setting the
         * Ramses appearance to nullptr will erase all inputs, and further calls with different Appearances will overwrite the
         * inputs according to the new appearance. Bear in mind that after a call to #setRamsesAppearance, pointers to properties of
         * this #RamsesAppearanceBinding from before the call will be invalid and must be re-queried, even if some or all of
         * the new appearance's properties have the same name or type!
         *
         * @param appearance to bind the RamsesAppearanceBinding to.
         */
        RLOGIC_API void setRamsesAppearance(ramses::Appearance* appearance);

        /**
         * Returns the currently assigned Ramses Appearance (or nullptr if none was assigned).
         * @return the currently bound ramses appearance
         */
        RLOGIC_API ramses::Appearance* getRamsesAppearance() const;

        /**
         * Constructor of RamsesAppearanceBinding. User is not supposed to call this - RamsesAppearanceBinding are created by other factory classes
         *
         * @param impl implementation details of the RamsesAppearanceBinding
         */
        explicit RamsesAppearanceBinding(std::unique_ptr<internal::RamsesAppearanceBindingImpl> impl) noexcept;

        /**
         * Destructor of RamsesAppearanceBinding.
         */
        ~RamsesAppearanceBinding() noexcept override;

        /**
         * Copy Constructor of RamsesAppearanceBinding is deleted because RamsesAppearanceBinding are not supposed to be copied
         *
         * @param other RamsesNodeBindings to copy from
         */
        RamsesAppearanceBinding(const RamsesAppearanceBinding& other) = delete;

        /**
         * Move Constructor of RamsesAppearanceBinding is deleted because RamsesAppearanceBinding are not supposed to be moved
         *
         * @param other RamsesAppearanceBinding to move from
         */
        RamsesAppearanceBinding(RamsesAppearanceBinding&& other) = delete;

        /**
         * Assignment operator of RamsesAppearanceBinding is deleted because RamsesAppearanceBinding are not supposed to be copied
         *
         * @param other RamsesAppearanceBinding to assign from
         */
        RamsesAppearanceBinding& operator=(const RamsesAppearanceBinding& other) = delete;

        /**
         * Move assignment operator of RamsesAppearanceBinding is deleted because RamsesAppearanceBinding are not supposed to be moved
         *
         * @param other RamsesAppearanceBinding to assign from
         */
        RamsesAppearanceBinding& operator=(RamsesAppearanceBinding&& other) = delete;

        /**
         * Implementation detail of RamsesAppearanceBinding
         */
        std::unique_ptr<internal::RamsesAppearanceBindingImpl> m_appearanceBinding;
    };
}
