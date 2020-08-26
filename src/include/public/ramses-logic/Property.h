//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include "ramses-logic/EPropertyType.h"

#include <cstddef>
#include <optional>
#include <memory>
#include <string>

namespace rlogic::internal
{
    class PropertyImpl;
}

namespace rlogic
{
    /**
    * Represents a generic property slot of the LogicEngine. This can be a script input
    * or output, or a RamsesNodeBinding input slot.
    * TODO discuss class structure we want to achieve with this - do we want inheritance?
    * If yes, how do we handle type information and casting?
    */
    class Property
    {
    public:
        /**
        * Returns the amount of available child (nested) properties. In the case that
        * the Property is a EPropertyType::STRUCT, the returned number will correspond
        * to the number of properties of that struct. getChildCount() returns always
        * zero for primitive properties like EPropertyType::INT or EPropertyType::FLOAT.
        *
        * @return the amount of available children.
        */
        RLOGIC_API size_t getChildCount() const;

        /**
        * Returns the type of this Property
        * TODO extend this doc once we have not only script properties but also others
        *
        * @return the type of this Property
        */
        RLOGIC_API EPropertyType getType() const;

        /**
        * Returns the name of this Property. Note that not all properties have a name -
        * for example an array element does not have a name.
        * TODO extend this doc once we have not only script properties but also others
        *
        * @return the name of this Property
        */
        // TODO discuss: could be wise to use string view here maybe
        RLOGIC_API std::string_view getName() const;

        /**
        * Returns the child with the given index.
        *
        * @return the child with the given index. Only structured properties
        * can return a child. In case of a primitive property nullptr will be returned.
        * If the child index is larger than the value of getChildCount(),
        * nullptr will be returned
        */
        RLOGIC_API const Property* getChild(size_t index) const;

        /**
        * @copydoc getChild(size_t index) const
        */
        RLOGIC_API Property* getChild(size_t index);

        /**
        * Returns the child with the given name. Only structured properties can return a child.
        * In case of a primitive property nullptr will be returned.
        * @return the child with the given name. If the child is not available nullptr will be returned
        * This does only work, if the Property is a struct. If it isn't a struct nullptr will be returned
        * TODO adjust the docs if arrays are supported
        */
        RLOGIC_API Property* getChild(std::string_view name);

        /**
        * @copydoc getChild(std::string_view name)
        */
        RLOGIC_API const Property* getChild(std::string_view name) const;

        /**
        * Returns the value of this Property. The supported template types defined by
        * IsPrimitiveProperty<T>::value == true. Using any other type will result in linker
        * errors since those are not implemented.
        * If the template type and the internal type do not match, std::nullopt will be returned.
        * @return the value of this Property as std::optional.
        */
        // TODO document which types are implemented/supported
        template <typename T> RLOGIC_API std::optional<T> get() const;

        /**
        * Sets the value of this Property
        * @param value to set for this Property
        *
        * @return true if setting the value was successful, false otherwise.
        */
        template <typename T> RLOGIC_API bool set(T value);

        /**
        * Constructor of Property. User is not supposed to call this - properties are created by other factory classes
        *
        * @param impl implementation details of the property
        */
        explicit Property(std::unique_ptr<internal::PropertyImpl> impl) noexcept;

        /**
        * Destructor of Property. User is not supposed to call this - properties are destroyed by other factory classes
        */
        ~Property() noexcept;

        /**
        * Copy Constructor of Property is deleted because properties are not supposed to be copied
        * @param other property to copy from
        */
        Property(const Property& other) = delete;

        /**
        * Move Constructor of Property is deleted because properties are not supposed to be moved
        * @param other property to move from
        */
        Property(Property&& other) = delete;

        /**
        * Assignment operator of Property is deleted because properties are not supposed to be copied
        * @param other property to assign from
        */
        Property& operator=(const Property& other) = delete;

        /**
        * Move assignment operator of Property is deleted because properties are not supposed to be moved
        * @param other property to move from
        */
        Property& operator=(Property&& other) = delete;

        /**
        * Implementation details of the Property class
        */
        std::unique_ptr<internal::PropertyImpl> m_impl;

    };
}
