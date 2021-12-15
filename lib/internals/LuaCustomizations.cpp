//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/LuaCustomizations.h"

#include "ramses-logic/Property.h"

#include "internals/SolHelper.h"
#include "internals/WrappedLuaProperty.h"
#include "internals/PropertyTypeExtractor.h"
#include "internals/LuaTypeConversions.h"
#include "internals/TypeUtils.h"

namespace rlogic::internal
{
    void LuaCustomizations::RegisterTypes(sol::state& state)
    {
        state["rl_len"] = rl_len;
        state["rl_next"] = rl_next;
        state["rl_pairs"] = rl_pairs;
        state["rl_ipairs"] = rl_ipairs;
    }

    void LuaCustomizations::MapToEnvironment(sol::state& state, sol::environment& env)
    {
        env["rl_len"] = state["rl_len"];
        env["rl_next"] = state["rl_next"];
        env["rl_pairs"] = state["rl_pairs"];
        env["rl_ipairs"] = state["rl_ipairs"];
    }

    size_t LuaCustomizations::rl_len(const sol::object& obj)
    {
        // Check if normal lua table, or read-only module table
        const std::optional<sol::lua_table> potentialTable = extractLuaTable(obj);
        if (potentialTable)
        {
            return potentialTable->size();
        }

        // Check for custom types (registered by logic engine)
        if (obj.get_type() == sol::type::userdata)
        {
            if (obj.is<WrappedLuaProperty>())
            {
                return obj.as<WrappedLuaProperty>().size();
            }

            if (obj.is<PropertyTypeExtractor>())
            {
                return obj.as<PropertyTypeExtractor>().getNestedExtractors().size();
            }
        }

        // Other type (unsupported) -> report usage error
        sol_helper::throwSolException("rl_len() called on an unsupported type '{}'", sol_helper::GetSolTypeName(obj.get_type()));

        return 0u;
    }

    std::tuple<sol::object, sol::object> LuaCustomizations::rl_next(sol::this_state state, const sol::object& container, const sol::object& indexObject)
    {
        // Runtime property (checked first because performance-wise highest priority)
        if (container.is<WrappedLuaProperty>())
        {
            const WrappedLuaProperty& wrappedProperty = container.as<WrappedLuaProperty>();
            const EPropertyType propType = wrappedProperty.getWrappedProperty().getType();

            // Safe to assert, not possible to obtain a non-container object currently
            assert(TypeUtils::CanHaveChildren(propType));

            // Valid case! If container empty, next() is required to return a tuple of nil's
            if (wrappedProperty.size() == 0u)
            {
                return std::make_tuple(sol::lua_nil, sol::lua_nil);
            }

            if (propType == EPropertyType::Array)
            {
                return rl_next_runtime_array(state, wrappedProperty, indexObject);
            }

            return rl_next_runtime_struct(state, wrappedProperty, indexObject);
        }

        // Standard Lua table or a read-only module
        std::optional<sol::lua_table> potentialModuleTable = extractLuaTable(container);
        if (potentialModuleTable)
        {
            sol::function stdNext = sol::state_view(state)["next"];
            std::tuple<sol::object, sol::object> resultOfStdNext = stdNext(*potentialModuleTable, indexObject);

            return resultOfStdNext;
        }

        // Property extractor - this is not executed during runtime, only during interface(), so it's ok to check last
        if (container.is<PropertyTypeExtractor>())
        {
            const PropertyTypeExtractor& typeExtractor = container.as<PropertyTypeExtractor>();
            const EPropertyType rootType = typeExtractor.getRootTypeData().type;

            // Not possible to get non-iteratable types during extraction, safe to assert here
            assert(TypeUtils::CanHaveChildren(rootType));

            // Valid case! If container empty, next() is required to return a tuple of nil's
            if (typeExtractor.getNestedExtractors().empty())
            {
                return std::make_tuple(sol::lua_nil, sol::lua_nil);
            }

            if (rootType == EPropertyType::Array)
            {
                return rl_next_array_extractor(state, typeExtractor, indexObject);
            }

            return rl_next_struct_extractor(state, typeExtractor, indexObject);
        }

        sol_helper::throwSolException("rl_next() called on an unsupported type '{}'", sol_helper::GetSolTypeName(container.get_type()));

        return std::make_tuple(sol::lua_nil, sol::lua_nil);
    }

    std::tuple<sol::object, sol::object> LuaCustomizations::rl_next_runtime_array(sol::this_state state, const WrappedLuaProperty& wrappedArray, const sol::object& indexObject)
    {
        assert(wrappedArray.getWrappedProperty().getType() == EPropertyType::Array);

        // If index is nil, return the first element
        if (indexObject.get_type() == sol::type::lua_nil)
        {
            // in Lua counting starts at 1; this returns the first tuple of index + value
            return std::make_tuple(sol::make_object(state, 1), wrappedArray.resolveChild(state, 0u));
        }

        const DataOrError<size_t> potentiallyIndex = LuaTypeConversions::ExtractSpecificType<size_t>(indexObject);

        if (potentiallyIndex.hasError())
        {
            sol_helper::throwSolException("Invalid key to rl_next() of type: {}", potentiallyIndex.getError());
        }

        const size_t index = potentiallyIndex.getData();

        if (index == 0 || index > wrappedArray.size())
        {
            sol_helper::throwSolException("Invalid key value '{}' for rl_next(). Expected a number in the range [1, {}]!", index, wrappedArray.size());
        }

        // This is valid - when index is the last element, the 'next' one is idx=nil, value=nil
        if (index == wrappedArray.size())
        {
            return std::make_tuple(sol::lua_nil, sol::lua_nil);
        }

        return std::make_tuple(sol::make_object(state, index + 1), wrappedArray.resolveChild(state, index));
    }

    std::tuple<sol::object, sol::object> LuaCustomizations::rl_next_runtime_struct(sol::this_state state, const WrappedLuaProperty& wrappedStruct, const sol::object& indexObject)
    {
        assert(wrappedStruct.getWrappedProperty().getType() == EPropertyType::Struct);

        // If index is nil, return the first element
        if (indexObject.get_type() == sol::type::lua_nil)
        {
            // return name of first element as key
            return std::make_tuple(sol::make_object(state, wrappedStruct.getWrappedProperty().getChild(0)->getName()), wrappedStruct.resolveChild(state, 0u));
        }

        // TODO Violin rework the error message here so that it's clear where the error comes from
        const size_t structFieldIndex = wrappedStruct.resolvePropertyIndex(indexObject);

        // This is valid - when index is the last element, the 'next' one is idx=nil, value=nil
        if (structFieldIndex == wrappedStruct.size() - 1)
        {
            return std::make_tuple(sol::lua_nil, sol::lua_nil);
        }

        return std::make_tuple(
            sol::make_object(state, wrappedStruct.getWrappedProperty().getChild(structFieldIndex + 1)->getName()),
            wrappedStruct.resolveChild(state, structFieldIndex + 1));
    }

    std::tuple<sol::object, sol::object> LuaCustomizations::rl_next_array_extractor(sol::this_state state, const PropertyTypeExtractor& arrayExtractor, const sol::object& indexObject)
    {
        // If index is nil, return the first element
        if (indexObject.get_type() == sol::type::lua_nil)
        {
            // in Lua counting starts at 1; this returns the first tuple of index + value
            return ResolveExtractorField(state, arrayExtractor, 0u);
        }

        const DataOrError<size_t> potentiallyIndex = LuaTypeConversions::ExtractSpecificType<size_t>(indexObject);

        if (potentiallyIndex.hasError())
        {
            sol_helper::throwSolException("Invalid key to rl_next() of type: {}", potentiallyIndex.getError());
        }

        const size_t arrayElementCount = arrayExtractor.getNestedExtractors().size();

        const size_t index = potentiallyIndex.getData();

        if (index == 0 || index > arrayElementCount)
        {
            sol_helper::throwSolException("Invalid key value '{}' for rl_next(). Expected a number in the range [1, {}]!", index, arrayElementCount);
        }

        // This is valid - when index is the last element, the 'next' one is idx=nil, value=nil
        if (index == arrayElementCount)
        {
            return std::make_tuple(sol::lua_nil, sol::lua_nil);
        }

        // Return next element which is 'index + 1', but because Lua starts at 1, it's just 'index'
        return ResolveExtractorField(state, arrayExtractor, index);
    }

    std::tuple<sol::object, sol::object> LuaCustomizations::rl_next_struct_extractor(sol::this_state state, const PropertyTypeExtractor& structExtractor, const sol::object& indexObject)
    {
        // If index is nil, return the first element
        if (indexObject.get_type() == sol::type::lua_nil)
        {
            return ResolveExtractorField(state, structExtractor, 0);
        }

        const DataOrError<std::string_view> potentiallyStrIndex = LuaTypeConversions::ExtractSpecificType<std::string_view>(indexObject);

        if (potentiallyStrIndex.hasError())
        {
            sol_helper::throwSolException("Invalid key to rl_next(): {}", potentiallyStrIndex.getError());
        }

        const std::string_view strIndex = potentiallyStrIndex.getData();

        const std::vector<PropertyTypeExtractor>& fields = structExtractor.getNestedExtractors();
        const auto fieldIter = std::find_if(fields.cbegin(), fields.cend(),
            [&strIndex](const PropertyTypeExtractor& field) {
                return field.getRootTypeData().name == strIndex;
            });

        if (fieldIter == fields.cend())
        {
            sol_helper::throwSolException("Could not find field named '{}' in struct object '{}'!",
                strIndex, structExtractor.getRootTypeData().name);
        }

        const size_t structFieldIndex = fieldIter - fields.cbegin();

        // This is valid - when index is the last element, the 'next' one is idx=nil, value=nil
        if (structFieldIndex == fields.size() - 1)
        {
            return std::make_tuple(sol::lua_nil, sol::lua_nil);
        }

        return ResolveExtractorField(state, structExtractor, structFieldIndex + 1);
    }

    std::tuple<sol::object, sol::object, sol::object> LuaCustomizations::rl_ipairs(sol::this_state s, sol::object iterableObject)
    {
        if (iterableObject.get_type() == sol::type::userdata)
        {
            // catch error case rl_ipairs(struct)
            sol::optional<const WrappedLuaProperty&> wrappedProperty = iterableObject.as<sol::optional<const WrappedLuaProperty&>>();
            if (wrappedProperty && wrappedProperty->getWrappedProperty().getType() != EPropertyType::Array)
            {
                const std::string_view typeInfo =
                    wrappedProperty ?
                    GetLuaPrimitiveTypeName(wrappedProperty->getWrappedProperty().getType()) :
                    (sol_helper::GetSolTypeName(iterableObject.get_type()));
                sol_helper::throwSolException("rl_ipairs() called on an unsupported type '{}'. Use only with array-like built-in types or modules!", typeInfo);
            }

            sol::optional<const PropertyTypeExtractor&> propertyExtractor = iterableObject.as<sol::optional<const PropertyTypeExtractor&>>();
            if (propertyExtractor && propertyExtractor->getRootTypeData().type != EPropertyType::Array)
            {
                const std::string_view typeInfo =
                    propertyExtractor ?
                    GetLuaPrimitiveTypeName(propertyExtractor->getRootTypeData().type) :
                    (sol_helper::GetSolTypeName(iterableObject.get_type()));
                sol_helper::throwSolException("rl_ipairs() called on an unsupported type '{}'. Use only with array-like built-in types or modules!", typeInfo);
            }

            // Assert that one of the types were found
            assert(wrappedProperty || propertyExtractor);

            return std::make_tuple(sol::state_view(s)["rl_next"], std::move(iterableObject), sol::nil);
        }

        std::optional<sol::lua_table> potentialModuleTable = extractLuaTable(iterableObject);
        if (potentialModuleTable)
        {
            return std::make_tuple(sol::state_view(s)["next"], std::move(*potentialModuleTable), sol::nil);
        }

        sol_helper::throwSolException("rl_ipairs() called on an unsupported type '{}'. Use only with user types like IN/OUT, modules etc.!", sol_helper::GetSolTypeName(iterableObject.get_type()));
        return std::make_tuple(sol::nil, sol::nil, sol::nil);
    }

    std::tuple<sol::object, sol::object, sol::object> LuaCustomizations::rl_pairs(sol::this_state s, sol::object iterableObject)
    {
        if (iterableObject.get_type() == sol::type::userdata)
        {
            return std::make_tuple(sol::state_view(s)["rl_next"], std::move(iterableObject), sol::nil);
        }

        std::optional<sol::lua_table> potentialModuleTable = extractLuaTable(iterableObject);
        if (potentialModuleTable)
        {
            return std::make_tuple(sol::state_view(s)["next"], std::move(*potentialModuleTable), sol::nil);
        }

        sol_helper::throwSolException("rl_pairs() called on an unsupported type '{}'. Use only with user types like IN/OUT, modules etc.!", sol_helper::GetSolTypeName(iterableObject.get_type()));
        return std::make_tuple(sol::nil, sol::nil, sol::nil);
    }

    std::tuple<sol::object, sol::object> LuaCustomizations::ResolveExtractorField(sol::this_state s, const PropertyTypeExtractor& typeExtractor, size_t fieldId)
    {
        const EPropertyType rootType = typeExtractor.getRootTypeData().type;
        std::reference_wrapper<const PropertyTypeExtractor> field = typeExtractor.getChildReference(fieldId);
        const EPropertyType fieldType = field.get().getRootTypeData().type;

        // Provide name as key for structs, index for arrays
        sol::object key =
            rootType == EPropertyType::Struct ?
            sol::make_object(s, field.get().getRootTypeData().name) :
            sol::make_object(s, fieldId + 1);   // Convert to Lua numeric convention (starts at 1)

        // Cast to label, e.g. INT32, FLOAT etc., for primitive types; Otherwise, return the container object for further iteration if needed
        sol::object value =
            TypeUtils::IsPrimitiveType(fieldType) ?
            sol::make_object(s, static_cast<int>(fieldType)) :
            sol::make_object(s, field);

        return std::make_tuple(std::move(key), std::move(value));
    }

    // extracts a plain Lua table, or one which was made read-only (e.g. module data tables)
    std::optional<sol::lua_table> LuaCustomizations::extractLuaTable(const sol::object& object)
    {
        auto potentialTable = object.as<std::optional<sol::lua_table>>();
        if (potentialTable)
        {
            sol::lua_table table = *potentialTable;
            sol::object metaContent = table[sol::metatable_key];

            // Identify read-only data (e.g. logic engine read-only module tables)
            // This is basically a reverse-test for this: https://www.lua.org/pil/13.4.4.html
            // See also LuaCompilationUtils::MakeTableReadOnly for the counterpart
            auto potentialMetatable = metaContent.as<std::optional<sol::lua_table>>();
            if (potentialMetatable)
            {
                sol::object metaIndex = (*potentialMetatable)[sol::meta_function::index];
                auto potentialModuleTable = metaIndex.as<std::optional<sol::lua_table>>();
                if (potentialModuleTable)
                {
                    potentialTable = potentialModuleTable;
                }
            }
        }

        return potentialTable;
    }
}
