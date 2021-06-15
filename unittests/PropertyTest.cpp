//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "impl/PropertyImpl.h"
#include "impl/LuaScriptImpl.h"
#include "internals/SolState.h"
#include "internals/ErrorReporting.h"

#include "ramses-logic/Property.h"
#include "ramses-logic/LogicEngine.h"

#include "flatbuffers/flatbuffers.h"

#include "generated/property_gen.h"

#include "LogicNodeDummy.h"
#include "LogTestUtils.h"

#include <memory>

namespace rlogic::internal
{
    class AProperty : public ::testing::Test
    {
    public:
        LogicNodeDummyImpl m_dummyNode{ "DummyNode" };

        static std::unique_ptr<PropertyImpl> Deserialize(const void* buffer)
        {
            return PropertyImpl::Deserialize(*rlogic_serialization::GetProperty(buffer), EPropertySemantics::ScriptInput);
        }

        std::unique_ptr<PropertyImpl> CreateInputProperty(std::string_view name, EPropertyType type, bool assignDummyLogicNode = true)
        {
            return CreateProperty(name, type, EPropertySemantics::ScriptInput, assignDummyLogicNode);

        }

        std::unique_ptr<PropertyImpl> CreateOutputProperty(std::string_view name, EPropertyType type, bool assignDummyLogicNode = true)
        {
            return CreateProperty(name, type, EPropertySemantics::ScriptOutput, assignDummyLogicNode);
        }

    protected:
        std::unique_ptr<PropertyImpl> CreateProperty(std::string_view name, EPropertyType type, EPropertySemantics semantics, bool assignDummyLogicNode)
        {
            auto property = std::make_unique<PropertyImpl>(name, type, semantics);
            if (assignDummyLogicNode)
            {
                property->setLogicNode(m_dummyNode);
            }
            return property;
        }

        // Silence logs, unless explicitly enabled, to reduce spam and speed up tests
        ScopedLogContextLevel m_silenceLogs{ELogMessageType::Off};
    };

    TEST_F(AProperty, HasANameAfterCreation)
    {
        Property desc(CreateInputProperty("PropertyName", EPropertyType::Float));
        EXPECT_EQ("PropertyName", desc.getName());
    }

    TEST_F(AProperty, HasATypeAfterCreation)
    {
        Property desc(CreateInputProperty("PropertyName", EPropertyType::Float));
        EXPECT_EQ(EPropertyType::Float, desc.getType());
    }

    TEST_F(AProperty, BindingInputHasNoUserValueBeforeSetExplicitly)
    {
        Property prop(CreateProperty("", EPropertyType::Float, EPropertySemantics::BindingInput, true));

        EXPECT_FALSE(prop.m_impl->bindingInputHasNewValue());
        EXPECT_FALSE(prop.m_impl->checkForBindingInputNewValueAndReset());
    }

    TEST_F(AProperty, BindingInputHasUserValueAfterSetIsCalledSuccessfully)
    {
        Property prop(CreateProperty("", EPropertyType::Float, EPropertySemantics::BindingInput, true));

        EXPECT_FALSE(prop.m_impl->bindingInputHasNewValue());
        // Set with wrong type (failed sets) have no effect on user value status
        EXPECT_FALSE(prop.set<int32_t>(5));
        EXPECT_FALSE(prop.m_impl->bindingInputHasNewValue());
        EXPECT_TRUE(prop.set<float>(0.5f));
        EXPECT_TRUE(prop.m_impl->checkForBindingInputNewValueAndReset());
    }

    TEST_F(AProperty, BindingInputHasUserValueAfterLinkIsActivated_andValueChanged)
    {
        Property linkTarget(CreateProperty("", EPropertyType::Float, EPropertySemantics::BindingInput, true));
        Property linkSource(CreateProperty("", EPropertyType::Float, EPropertySemantics::ScriptOutput, true));

        // Set to different than default value
        linkSource.m_impl->setManually<float>(0.5f);

        // Simulate link behavior
        linkTarget.m_impl->setInternal(*linkSource.m_impl);
        EXPECT_TRUE(linkTarget.m_impl->checkForBindingInputNewValueAndReset());
    }

    TEST_F(AProperty, BindingInputHasNewUserValueAfterLinkIsActivated_whenNewValueSameAsOldValue)
    {
        Property linkTarget(CreateProperty("", EPropertyType::Float, EPropertySemantics::BindingInput, true));
        Property linkSource(CreateProperty("", EPropertyType::Float, EPropertySemantics::ScriptOutput, true));

        // Set same value to both
        linkSource.set<float>(.5f);
        linkTarget.set<float>(.5f);

        // Simulate link behavior
        linkTarget.m_impl->setInternal(*linkSource.m_impl);
        EXPECT_TRUE(linkTarget.m_impl->checkForBindingInputNewValueAndReset());
    }

    TEST_F(AProperty, BindingInputHasNoUserValueAnymore_WhenConsumed)
    {
        Property prop(CreateProperty("", EPropertyType::Float, EPropertySemantics::BindingInput, true));

        ASSERT_FALSE(prop.m_impl->bindingInputHasNewValue());
        EXPECT_TRUE(prop.set<float>(0.5f));
        // Consume value => has no value any more
        EXPECT_TRUE(prop.m_impl->checkForBindingInputNewValueAndReset());
        EXPECT_FALSE(prop.m_impl->bindingInputHasNewValue());
    }

    TEST_F(AProperty, DoesntHaveChildrenAfterCreation)
    {
        Property desc(CreateInputProperty("PropertyName", EPropertyType::Float));
        ASSERT_EQ(0u, desc.getChildCount());
    }

    TEST_F(AProperty, ReturnsDefaultValue_ForPrimitiveTypes)
    {
        Property aFloat(CreateInputProperty("", EPropertyType::Float));
        ASSERT_TRUE(aFloat.get<float>());
        EXPECT_FLOAT_EQ(0.0f, *aFloat.get<float>());

        Property aInt(CreateInputProperty("", EPropertyType::Int32));
        ASSERT_TRUE(aInt.get<int32_t>());
        EXPECT_EQ(0, *aInt.get<int32_t>());

        Property aBool(CreateInputProperty("", EPropertyType::Bool));
        ASSERT_TRUE(aBool.get<bool>());
        EXPECT_EQ(false, *aBool.get<bool>());

        Property aString(CreateInputProperty("", EPropertyType::String));
        ASSERT_TRUE(aString.get<std::string>());
        EXPECT_EQ("", *aString.get<std::string>());
    }

    TEST_F(AProperty, ReturnsDefaultValue_VectorTypes)
    {
        Property aVec2f(CreateInputProperty("", EPropertyType::Vec2f));
        Property aVec3f(CreateInputProperty("", EPropertyType::Vec3f));
        Property aVec4f(CreateInputProperty("", EPropertyType::Vec4f));
        Property aVec2i(CreateInputProperty("", EPropertyType::Vec2i));
        Property aVec3i(CreateInputProperty("", EPropertyType::Vec3i));
        Property aVec4i(CreateInputProperty("", EPropertyType::Vec4i));

        EXPECT_TRUE(aVec2f.get<vec2f>());
        EXPECT_TRUE(aVec3f.get<vec3f>());
        EXPECT_TRUE(aVec4f.get<vec4f>());
        EXPECT_TRUE(aVec2i.get<vec2i>());
        EXPECT_TRUE(aVec3i.get<vec3i>());
        EXPECT_TRUE(aVec4i.get<vec4i>());

        vec2f vec2fValue = *aVec2f.get<vec2f>();
        vec3f vec3fValue = *aVec3f.get<vec3f>();
        vec4f vec4fValue = *aVec4f.get<vec4f>();

        ASSERT_EQ(2u, vec2fValue.size());
        EXPECT_FLOAT_EQ(0.0f, vec2fValue[0]);
        EXPECT_FLOAT_EQ(0.0f, vec2fValue[1]);

        ASSERT_EQ(3u, vec3fValue.size());
        EXPECT_FLOAT_EQ(0.0f, vec3fValue[0]);
        EXPECT_FLOAT_EQ(0.0f, vec3fValue[1]);
        EXPECT_FLOAT_EQ(0.0f, vec3fValue[2]);

        ASSERT_EQ(4u, vec4fValue.size());
        EXPECT_FLOAT_EQ(0.0f, vec4fValue[0]);
        EXPECT_FLOAT_EQ(0.0f, vec4fValue[1]);
        EXPECT_FLOAT_EQ(0.0f, vec4fValue[2]);
        EXPECT_FLOAT_EQ(0.0f, vec4fValue[3]);

        vec2i vec2iValue = *aVec2i.get<vec2i>();
        vec3i vec3iValue = *aVec3i.get<vec3i>();
        vec4i vec4iValue = *aVec4i.get<vec4i>();

        ASSERT_EQ(2u, vec2iValue.size());
        EXPECT_EQ(0, vec2iValue[0]);
        EXPECT_EQ(0, vec2iValue[1]);

        ASSERT_EQ(3u, vec3iValue.size());
        EXPECT_EQ(0, vec3iValue[0]);
        EXPECT_EQ(0, vec3iValue[1]);
        EXPECT_EQ(0, vec3iValue[2]);

        ASSERT_EQ(4u, vec4iValue.size());
        EXPECT_EQ(0, vec4iValue[0]);
        EXPECT_EQ(0, vec4iValue[1]);
        EXPECT_EQ(0, vec4iValue[2]);
        EXPECT_EQ(0, vec4iValue[3]);
    }

    TEST_F(AProperty, ReturnsValueIfItIsSetBeforehand_PrimitiveTypes)
    {
        Property aFloat(CreateInputProperty("", EPropertyType::Float));
        Property aInt32(CreateInputProperty("", EPropertyType::Int32));
        Property aBool(CreateInputProperty("", EPropertyType::Bool));
        Property aString(CreateInputProperty("", EPropertyType::String));

        EXPECT_TRUE(aFloat.set<float>(47.11f));
        EXPECT_TRUE(aInt32.set<int>(5));
        EXPECT_TRUE(aBool.set<bool>(true));
        EXPECT_TRUE(aString.set<std::string>("hello"));

        auto valueFloat = aFloat.get<float>();
        auto valueInt32 = aInt32.get<int>();
        auto valueBool = aBool.get<bool>();
        auto valueString = aString.get<std::string>();
        ASSERT_TRUE(valueFloat);
        ASSERT_TRUE(valueInt32);
        ASSERT_TRUE(valueBool);
        ASSERT_TRUE(valueString);

        EXPECT_FLOAT_EQ(47.11f, *valueFloat);
        EXPECT_EQ(5, *valueInt32);
        EXPECT_EQ(true, *valueBool);
        EXPECT_EQ("hello", *valueString);
    }

    TEST_F(AProperty, ReturnsValueIfItIsSetBeforehand_VectorTypes_Float)
    {
        Property avec2f(CreateInputProperty("", EPropertyType::Vec2f));
        Property avec3f(CreateInputProperty("", EPropertyType::Vec3f));
        Property avec4f(CreateInputProperty("", EPropertyType::Vec4f));

        EXPECT_TRUE(avec2f.set<vec2f>({ 0.1f, 0.2f }));
        EXPECT_TRUE(avec3f.set<vec3f>({ 0.1f, 0.2f, 0.3f }));
        EXPECT_TRUE(avec4f.set<vec4f>({ 0.1f, 0.2f, 0.3f, 0.4f }));

        auto valueVec2f = avec2f.get<vec2f>();
        auto valueVec3f = avec3f.get<vec3f>();
        auto valueVec4f = avec4f.get<vec4f>();
        ASSERT_TRUE(valueVec2f);
        ASSERT_TRUE(valueVec3f);
        ASSERT_TRUE(valueVec4f);

        vec2f expectedValueVec2f {0.1f, 0.2f};
        vec3f expectedValueVec3f {0.1f, 0.2f, 0.3f};
        vec4f expectedValueVec4f {0.1f, 0.2f, 0.3f, 0.4f};
        EXPECT_TRUE(expectedValueVec2f == *valueVec2f);
        EXPECT_TRUE(expectedValueVec3f == *valueVec3f);
        EXPECT_TRUE(expectedValueVec4f == *valueVec4f);
    }

    TEST_F(AProperty, ReturnsValueIfItIsSetBeforehand_VectorTypes_Int)
    {
        Property avec2i(CreateInputProperty("", EPropertyType::Vec2i));
        Property avec3i(CreateInputProperty("", EPropertyType::Vec3i));
        Property avec4i(CreateInputProperty("", EPropertyType::Vec4i));

        EXPECT_TRUE(avec2i.set<vec2i>({1, 2}));
        EXPECT_TRUE(avec3i.set<vec3i>({1, 2, 3}));
        EXPECT_TRUE(avec4i.set<vec4i>({1, 2, 3, 4}));

        auto valueVec2i = avec2i.get<vec2i>();
        auto valueVec3i = avec3i.get<vec3i>();
        auto valueVec4i = avec4i.get<vec4i>();
        ASSERT_TRUE(valueVec2i);
        ASSERT_TRUE(valueVec3i);
        ASSERT_TRUE(valueVec4i);

        vec2i expectedValueVec2i{1, 2};
        vec3i expectedValueVec3i{1, 2, 3};
        vec4i expectedValueVec4i{1, 2, 3, 4};
        EXPECT_TRUE(expectedValueVec2i == *valueVec2i);
        EXPECT_TRUE(expectedValueVec3i == *valueVec3i);
        EXPECT_TRUE(expectedValueVec4i == *valueVec4i);
    }

    TEST_F(AProperty, IsInitializedAsInputOrOutput)
    {
        Property inputProperty(CreateInputProperty("Input", EPropertyType::Float));
        Property outputProperty(CreateOutputProperty("Output", EPropertyType::Int32));

        EXPECT_TRUE(inputProperty.m_impl->isInput());
        EXPECT_FALSE(inputProperty.m_impl->isOutput());
        EXPECT_EQ(internal::EPropertySemantics::ScriptInput, inputProperty.m_impl->getPropertySemantics());
        EXPECT_TRUE(outputProperty.m_impl->isOutput());
        EXPECT_FALSE(outputProperty.m_impl->isInput());
        EXPECT_EQ(internal::EPropertySemantics::ScriptOutput, outputProperty.m_impl->getPropertySemantics());
    }

    TEST_F(AProperty, CannotSetOutputManually)
    {
        Property outputProperty(CreateOutputProperty("Output", EPropertyType::Int32));

        EXPECT_TRUE(outputProperty.m_impl->isOutput());
        EXPECT_EQ(internal::EPropertySemantics::ScriptOutput, outputProperty.m_impl->getPropertySemantics());

        EXPECT_FALSE(outputProperty.set<int32_t>(45));
    }

    TEST_F(AProperty, ReturnsNoValueWhenAccessingWithWrongType)
    {
        Property floatProp(CreateInputProperty("", EPropertyType::Float));
        Property vec2fProp(CreateInputProperty("", EPropertyType::Vec2f));
        Property vec3fProp(CreateInputProperty("", EPropertyType::Vec3f));
        Property vec4fProp(CreateInputProperty("", EPropertyType::Vec4f));
        Property int32Prop(CreateInputProperty("", EPropertyType::Int32));
        Property vec2iProp(CreateInputProperty("", EPropertyType::Vec2i));
        Property vec3iProp(CreateInputProperty("", EPropertyType::Vec3i));
        Property vec4iProp(CreateInputProperty("", EPropertyType::Vec4i));
        Property boolProp(CreateInputProperty("", EPropertyType::Bool));
        Property stringProp(CreateInputProperty("", EPropertyType::String));
        Property structProp(CreateInputProperty("", EPropertyType::Struct));
        Property arrayProp(CreateInputProperty("", EPropertyType::Array));

        // Floats
        EXPECT_TRUE(floatProp.get<float>());
        EXPECT_FALSE(floatProp.get<int32_t>());

        EXPECT_TRUE(vec2fProp.get<vec2f>());
        EXPECT_FALSE(vec2fProp.get<vec2i>());

        EXPECT_TRUE(vec3fProp.get<vec3f>());
        EXPECT_FALSE(vec3fProp.get<vec3i>());

        EXPECT_TRUE(vec4fProp.get<vec4f>());
        EXPECT_FALSE(vec4fProp.get<vec4i>());

        // Integers
        EXPECT_TRUE(int32Prop.get<int32_t>());
        EXPECT_FALSE(int32Prop.get<float>());

        EXPECT_TRUE(vec2iProp.get<vec2i>());
        EXPECT_FALSE(vec2iProp.get<vec2f>());

        EXPECT_TRUE(vec3iProp.get<vec3i>());
        EXPECT_FALSE(vec3iProp.get<vec3f>());

        EXPECT_TRUE(vec4iProp.get<vec4i>());
        EXPECT_FALSE(vec4iProp.get<vec4f>());

        // Others
        EXPECT_TRUE(boolProp.get<bool>());
        EXPECT_FALSE(boolProp.get<int32_t>());

        EXPECT_TRUE(stringProp.get<std::string>());
        EXPECT_FALSE(stringProp.get<bool>());

        // Complex types never have value
        EXPECT_FALSE(structProp.get<std::string>());
        EXPECT_FALSE(structProp.get<bool>());

        EXPECT_FALSE(arrayProp.get<int32_t>());
        EXPECT_FALSE(arrayProp.get<vec2f>());
    }

    TEST_F(AProperty, ReturnsNullptrForGetChildByIndexIfPropertyHasNoChildren)
    {
        Property property_float(CreateInputProperty("PropertyRoot", EPropertyType::Float));

        EXPECT_EQ(nullptr, property_float.getChild(0));
    }

    TEST_F(AProperty, ReturnsNullptrForGetChildByNameIfPropertyHasNoChildren)
    {
        Property property_float(CreateInputProperty("PropertyRoot", EPropertyType::Float));

        EXPECT_EQ(nullptr, property_float.getChild("child"));
    }

    TEST_F(AProperty, AddsChildIfTypeIsStruct)
    {
        auto rootImpl = CreateInputProperty("PropertyRoot", EPropertyType::Struct);
        rootImpl->addChild(CreateInputProperty("ChildProperty", EPropertyType::Float, false));

        Property root(std::move(rootImpl));

        ASSERT_EQ(1, root.getChildCount());
        EXPECT_EQ("ChildProperty", root.getChild(0)->getName());
        EXPECT_EQ(EPropertyType::Float, root.getChild(0)->getType());
    }

    TEST_F(AProperty, AddsChildIfTypeIsArray)
    {
        auto rootImpl = CreateInputProperty("PropertyRoot", EPropertyType::Array);
        rootImpl->addChild(CreateInputProperty("ChildProperty", EPropertyType::Float, false));

        Property root(std::move(rootImpl));

        ASSERT_EQ(1, root.getChildCount());
        EXPECT_EQ("ChildProperty", root.getChild(0)->getName());
        EXPECT_EQ(EPropertyType::Float, root.getChild(0)->getType());
    }

    TEST_F(AProperty, CanBeEmptyAndConst)
    {
        auto           rootImpl = CreateInputProperty("PropertyRoot", EPropertyType::Struct);
        const Property root(std::move(rootImpl));

        const auto child = root.getChild(0);
        EXPECT_EQ(nullptr, child);
    }

    TEST_F(AProperty, CanHaveNestedProperties)
    {
        auto rootImpl = CreateInputProperty("PropertyRoot", EPropertyType::Struct);

        rootImpl->addChild(CreateInputProperty("PropertyChild1", EPropertyType::Int32, false));
        rootImpl->addChild(CreateInputProperty("PropertyChild2", EPropertyType::Float, false));

        Property root(std::move(rootImpl));

        ASSERT_EQ(2u, root.getChildCount());

        auto c1 = root.getChild(0);
        auto c2 = root.getChild(1);

        EXPECT_EQ("PropertyChild1", c1->getName());
        EXPECT_EQ("PropertyChild2", c2->getName());

        const auto& const_root = root;
        auto        c3         = const_root.getChild(0);
        auto        c4         = const_root.getChild(1);

        EXPECT_EQ("PropertyChild1", c3->getName());
        EXPECT_EQ("PropertyChild2", c4->getName());
    }

    TEST_F(AProperty, ImplCreatesDeepCopyOfItself)
    {
        auto rootImpl = CreateInputProperty("PropertyRoot", EPropertyType::Array, false);
        rootImpl->addChild(CreateInputProperty("PropertyChild1", EPropertyType::Int32, false));
        rootImpl->addChild(CreateInputProperty("PropertyChild2", EPropertyType::Float, false));

        auto deepCopy = rootImpl->deepCopy();
        EXPECT_NE(deepCopy.get(), rootImpl.get());
        ASSERT_EQ(2u, deepCopy->getChildCount());

        auto c1 = deepCopy->getChild(0);
        auto c2 = deepCopy->getChild(1);

        EXPECT_EQ("PropertyChild1", c1->getName());
        EXPECT_EQ("PropertyChild2", c2->getName());

        EXPECT_NE(deepCopy->getChild(0)->m_impl.get(), rootImpl->getChild(0)->m_impl.get());

        // Check that deep copy does not have values assigned
        EXPECT_FALSE(deepCopy->checkForBindingInputNewValueAndReset());
        EXPECT_FALSE(deepCopy->getChild(0)->m_impl->checkForBindingInputNewValueAndReset());
        EXPECT_FALSE(deepCopy->getChild(1)->m_impl->checkForBindingInputNewValueAndReset());
    }

    TEST_F(AProperty, SetsValueIfTheTypeMatches)
    {
        Property floatProperty(CreateInputProperty("PropertyRoot", EPropertyType::Float));
        Property int32Property(CreateInputProperty("PropertyRoot", EPropertyType::Int32));
        Property stringProperty(CreateInputProperty("PropertyRoot", EPropertyType::String));
        Property boolProperty(CreateInputProperty("PropertyRoot", EPropertyType::Bool));

        EXPECT_TRUE(floatProperty.set<float>(47.11f));
        EXPECT_TRUE(int32Property.set<int32_t>(4711));
        EXPECT_TRUE(stringProperty.set<std::string>("4711"));
        EXPECT_TRUE(boolProperty.set<bool>(true));

        auto floatValue  = floatProperty.get<float>();
        auto intValue    = int32Property.get<int32_t>();
        auto stringValue = stringProperty.get<std::string>();
        auto boolValue   = boolProperty.get<bool>();

        ASSERT_TRUE(floatValue);
        ASSERT_TRUE(intValue);
        ASSERT_TRUE(stringValue);
        ASSERT_TRUE(boolValue);

        ASSERT_EQ(47.11f, *floatValue);
        ASSERT_EQ(4711, *intValue);
        ASSERT_EQ("4711", *stringValue);
        ASSERT_EQ(true, *boolValue);
    }

    TEST_F(AProperty, DoesNotSetValueIfTheTypeDoesNotMatch)
    {
        Property floatProperty(CreateInputProperty("PropertyRoot", EPropertyType::Float));
        Property int32Property(CreateInputProperty("PropertyRoot", EPropertyType::Int32));
        Property stringProperty(CreateInputProperty("PropertyRoot", EPropertyType::String));
        Property boolProperty(CreateInputProperty("PropertyRoot", EPropertyType::Bool));

        EXPECT_FALSE(floatProperty.set<int32_t>(4711));
        EXPECT_FALSE(int32Property.set<float>(47.11f));
        EXPECT_FALSE(stringProperty.set<bool>(true));
        EXPECT_FALSE(boolProperty.set<std::string>("4711"));
        EXPECT_FALSE(floatProperty.set<vec2f>({0.1f, 0.2f}));

        auto floatValue  = floatProperty.get<float>();
        auto intValue    = int32Property.get<int32_t>();
        auto stringValue = stringProperty.get<std::string>();
        auto boolValue   = boolProperty.get<bool>();

        EXPECT_TRUE(floatValue);
        EXPECT_TRUE(intValue);
        EXPECT_TRUE(stringValue);
        EXPECT_TRUE(boolValue);
        EXPECT_EQ(0.0f, floatValue);
        EXPECT_EQ(0, intValue);
        EXPECT_EQ("", stringValue);
        EXPECT_EQ(false, boolValue);
    }

    TEST_F(AProperty, ReturnsChildByName)
    {
        auto rootImpl = CreateInputProperty("PropertyRoot", EPropertyType::Struct);

        rootImpl->addChild(CreateInputProperty("PropertyChild1", EPropertyType::Int32, false));
        rootImpl->addChild(CreateInputProperty("PropertyChild2", EPropertyType::Float, false));

        Property root(std::move(rootImpl));

        const Property* c1 = root.getChild("PropertyChild1");
        EXPECT_EQ("PropertyChild1", c1->getName());

        const Property* c2 = root.getChild("PropertyChild1");
        EXPECT_EQ("PropertyChild1", c2->getName());

        const Property* c3 = root.getChild("does_not_exist");
        EXPECT_FALSE(c3);
    }

    TEST_F(AProperty, ReturnsConstChildByName)
    {
        auto rootImpl = CreateInputProperty("PropertyRoot", EPropertyType::Struct);

        rootImpl->addChild(CreateInputProperty("PropertyChild1", EPropertyType::Int32, false));
        rootImpl->addChild(CreateInputProperty("PropertyChild2", EPropertyType::Float, false));

        const Property root(std::move(rootImpl));

        const Property* c1 = root.getChild("PropertyChild1");
        EXPECT_EQ("PropertyChild1", c1->getName());

        const Property* c2 = root.getChild("PropertyChild1");
        EXPECT_EQ("PropertyChild1", c2->getName());

        const Property* c3 = root.getChild("does_not_exist");
        EXPECT_FALSE(c3);
    }

    TEST_F(AProperty, CanBeSerializedAndDeserializedWhenEmpty)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto rootImpl = CreateInputProperty("EmptyProperty", EPropertyType::Struct);

            (void)PropertyImpl::Serialize(*rootImpl, builder);
        }
        {
            auto root = Deserialize(builder.GetBufferPointer());

            ASSERT_EQ(0u, root->getChildCount());
            EXPECT_EQ(EPropertyType::Struct, root->getType());
            EXPECT_EQ("EmptyProperty", root->getName());
            EXPECT_EQ(EPropertySemantics::ScriptInput, root->getPropertySemantics());
        }
    }

    TEST_F(AProperty, CanBeSerializedAndDeserialized_ForAllSupportedTypes)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto rootImpl = CreateInputProperty("Root", EPropertyType::Struct);

            rootImpl->addChild(CreateInputProperty("Int32", EPropertyType::Int32, false));
            rootImpl->addChild(CreateInputProperty("Float", EPropertyType::Float, false));
            rootImpl->addChild(CreateInputProperty("Bool", EPropertyType::Bool, false));
            rootImpl->addChild(CreateInputProperty("String", EPropertyType::String, false));
            rootImpl->addChild(CreateInputProperty("Vec2f", EPropertyType::Vec2f, false));
            rootImpl->addChild(CreateInputProperty("Vec3f", EPropertyType::Vec3f, false));
            rootImpl->addChild(CreateInputProperty("Vec4f", EPropertyType::Vec4f, false));
            rootImpl->addChild(CreateInputProperty("Vec2i", EPropertyType::Vec2i, false));
            rootImpl->addChild(CreateInputProperty("Vec3i", EPropertyType::Vec3i, false));
            rootImpl->addChild(CreateInputProperty("Vec4i", EPropertyType::Vec4i, false));
            rootImpl->addChild(CreateInputProperty("DefaultValue", EPropertyType::Vec4i, false));

            auto propInt32  = rootImpl->getChild("Int32");
            auto propFloat  = rootImpl->getChild("Float");
            auto propBool   = rootImpl->getChild("Bool");
            auto propString = rootImpl->getChild("String");
            auto propVec2f  = rootImpl->getChild("Vec2f");
            auto propVec3f  = rootImpl->getChild("Vec3f");
            auto propVec4f  = rootImpl->getChild("Vec4f");
            auto propVec2i  = rootImpl->getChild("Vec2i");
            auto propVec3i  = rootImpl->getChild("Vec3i");
            auto propVec4i  = rootImpl->getChild("Vec4i");

            propInt32->set(4711);
            propFloat->set(47.11f);
            propBool->set(true);
            propString->set<std::string>("4711");
            propVec2f->set<vec2f>({0.1f, 0.2f});
            propVec3f->set<vec3f>({1.1f, 1.2f, 1.3f});
            propVec4f->set<vec4f>({2.1f, 2.2f, 2.3f, 2.4f});
            propVec2i->set<vec2i>({1, 2});
            propVec3i->set<vec3i>({3, 4, 5});
            propVec4i->set<vec4i>({6, 7, 8, 9});

            (void)PropertyImpl::Serialize(*rootImpl, builder);
        }
        {

            auto root = Deserialize(builder.GetBufferPointer());

            ASSERT_EQ(11u, root->getChildCount());
            EXPECT_EQ(EPropertyType::Struct, root->getType());

            auto propInt32  = root->getChild(0);
            auto propFloat  = root->getChild(1);
            auto propBool   = root->getChild(2);
            auto propString = root->getChild(3);
            auto propVec2f  = root->getChild(4);
            auto propVec3f  = root->getChild(5);
            auto propVec4f  = root->getChild(6);
            auto propVec2i  = root->getChild(7);
            auto propVec3i  = root->getChild(8);
            auto propVec4i  = root->getChild(9);
            auto propDefValue  = root->getChild(10);

            EXPECT_EQ("Int32", propInt32->getName());
            EXPECT_EQ("Float", propFloat->getName());
            EXPECT_EQ("Bool", propBool->getName());
            EXPECT_EQ("String", propString->getName());
            EXPECT_EQ("Vec2f", propVec2f->getName());
            EXPECT_EQ("Vec3f", propVec3f->getName());
            EXPECT_EQ("Vec4f", propVec4f->getName());
            EXPECT_EQ("Vec2i", propVec2i->getName());
            EXPECT_EQ("Vec3i", propVec3i->getName());
            EXPECT_EQ("Vec4i", propVec4i->getName());
            EXPECT_EQ("DefaultValue", propDefValue->getName());


            vec2f expectedValueVec2f{0.1f, 0.2f};
            vec3f expectedValueVec3f{1.1f, 1.2f, 1.3f};
            vec4f expectedValueVec4f{2.1f, 2.2f, 2.3f, 2.4f};
            vec2i expectedValueVec2i{1, 2};
            vec3i expectedValueVec3i{3, 4, 5};
            vec4i expectedValueVec4i{6, 7, 8, 9};
            EXPECT_EQ(4711, *propInt32->get<int32_t>());
            EXPECT_FLOAT_EQ(47.11f, *propFloat->get<float>());
            EXPECT_TRUE(*propBool->get<bool>());
            EXPECT_EQ("4711", *propString->get<std::string>());
            EXPECT_EQ(expectedValueVec2f, *propVec2f->get<vec2f>());
            EXPECT_EQ(expectedValueVec3f, *propVec3f->get<vec3f>());
            EXPECT_EQ(expectedValueVec4f, *propVec4f->get<vec4f>());
            EXPECT_EQ(expectedValueVec2i, *propVec2i->get<vec2i>());
            EXPECT_EQ(expectedValueVec3i, *propVec3i->get<vec3i>());
            EXPECT_EQ(expectedValueVec4i, *propVec4i->get<vec4i>());
            EXPECT_FALSE(propDefValue->m_impl->checkForBindingInputNewValueAndReset());
        }
    }

    TEST_F(AProperty, KeepsOriginalPropertyOrderAfterDeserialization)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto propertyRoot = CreateInputProperty("PropertyInt", EPropertyType::Struct);
            auto c1 = CreateInputProperty("PropertyFloat1", EPropertyType::Float, false);
            auto c2 = CreateInputProperty("PropertyFloat2", EPropertyType::Float, false);
            auto c3 = CreateInputProperty("PropertyFloat3", EPropertyType::Float, false);

            propertyRoot->addChild(std::move(c1));
            propertyRoot->addChild(std::move(c2));
            propertyRoot->addChild(std::move(c3));

            (void)PropertyImpl::Serialize(*propertyRoot, builder);
        }
        {
            auto root = Deserialize(builder.GetBufferPointer());

            ASSERT_EQ(3u, root->getChildCount());
            EXPECT_EQ(EPropertyType::Struct, root->getType());

            auto c1    = root->getChild(0);
            auto c2    = root->getChild(1);
            auto c3    = root->getChild(2);

            EXPECT_EQ("PropertyFloat1", c1->getName());
            EXPECT_EQ("PropertyFloat2", c2->getName());
            EXPECT_EQ("PropertyFloat3", c3->getName());
        }
    }

    TEST_F(AProperty, CanSerializeAndDeserializeMultiLevelProperties)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto propertyRoot    = CreateInputProperty("PropertyRoot", EPropertyType::Struct);
            auto propertyNested1 = CreateInputProperty("PropertyNested", EPropertyType::Struct, false);
            auto propertyFloat1  = CreateInputProperty("PropertyFloat", EPropertyType::Float, false);
            auto propertyNested2 = CreateInputProperty("PropertyNested", EPropertyType::Struct, false);
            auto propertyFloat2  = CreateInputProperty("PropertyFloat", EPropertyType::Float, false);

            propertyNested1->addChild(std::move(propertyFloat1));
            propertyNested2->addChild(std::move(propertyFloat2));
            propertyNested1->addChild(std::move(propertyNested2));
            propertyRoot->addChild(std::move(propertyNested1));

            (void)PropertyImpl::Serialize(*propertyRoot, builder);
        }
        {
            auto root = Deserialize(builder.GetBufferPointer());

            ASSERT_EQ(1u, root->getChildCount());
            EXPECT_EQ(EPropertyType::Struct, root->getType());

            auto propertyNested1 = root->getChild(0u);
            EXPECT_EQ(EPropertyType::Struct, propertyNested1->getType());
            EXPECT_EQ("PropertyNested", propertyNested1->getName());

            ASSERT_EQ(2u, propertyNested1->getChildCount());
            auto propertyFloat1 = propertyNested1->getChild(0u);
            auto propertyNested2 = propertyNested1->getChild(1u);

            EXPECT_EQ(EPropertyType::Float, propertyFloat1->getType());
            EXPECT_EQ("PropertyFloat", propertyFloat1->getName());
            EXPECT_EQ(EPropertyType::Struct, propertyNested2->getType());
            EXPECT_EQ("PropertyNested", propertyNested2->getName());

            ASSERT_EQ(1u, propertyNested2->getChildCount());
            auto propertyFloat2 = propertyNested2->getChild(0u);

            EXPECT_EQ(EPropertyType::Float, propertyFloat2->getType());
            EXPECT_EQ("PropertyFloat", propertyFloat2->getName());
        }
    }

    TEST_F(AProperty, InheritsLogicNodeAssignmentFromParent_AfterAddedAsChild)
    {
        auto parent = CreateInputProperty("PropertyRoot", EPropertyType::Struct);

        // Explicitly create child without logic node assigned to it - we want to test this here
        auto childWithoutParent = std::make_unique<PropertyImpl>("child", EPropertyType::Float, EPropertySemantics::ScriptInput);

        parent->addChild(std::move(childWithoutParent));

        EXPECT_EQ(&m_dummyNode, &parent->getLogicNode());
        EXPECT_EQ(&m_dummyNode, &parent->getChild("child")->m_impl->getLogicNode());
    }

    TEST_F(AProperty, DoesNotSetLogicNodeToDirtyIfValueIsNotChanged)
    {
        auto intProperty    = CreateInputProperty("Property", EPropertyType::Int32);
        auto floatProperty  = CreateInputProperty("Property", EPropertyType::Float);
        auto vec2fProperty  = CreateInputProperty("Property", EPropertyType::Vec2f);
        auto vec3iProperty  = CreateInputProperty("Property", EPropertyType::Vec3i);
        auto stringProperty = CreateInputProperty("Property", EPropertyType::String);

        intProperty->setManually(42);
        floatProperty->setManually(42.f);
        vec2fProperty->setManually(vec2f{4.f, 2.f});
        vec3iProperty->setManually(vec3i{4, 2, 3});
        stringProperty->setManually(std::string("42"));

        intProperty->getLogicNode().setDirty(false);
        floatProperty->getLogicNode().setDirty(false);
        vec2fProperty->getLogicNode().setDirty(false);
        vec3iProperty->getLogicNode().setDirty(false);
        stringProperty->getLogicNode().setDirty(false);

        EXPECT_TRUE(intProperty->setManually(42));
        EXPECT_TRUE(floatProperty->setManually(42.f));
        EXPECT_TRUE(vec2fProperty->setManually(vec2f{4.f, 2.f}));
        EXPECT_TRUE(vec3iProperty->setManually(vec3i{4, 2, 3}));
        EXPECT_TRUE(stringProperty->setManually(std::string("42")));

        EXPECT_FALSE(intProperty->getLogicNode().isDirty());
        EXPECT_FALSE(floatProperty->getLogicNode().isDirty());
        EXPECT_FALSE(vec2fProperty->getLogicNode().isDirty());
        EXPECT_FALSE(vec3iProperty->getLogicNode().isDirty());
        EXPECT_FALSE(stringProperty->getLogicNode().isDirty());
    }

    TEST_F(AProperty, SetsLogicNodeToDirtyIfValueIsChanged)
    {
        auto intProperty    = CreateInputProperty("Property", EPropertyType::Int32);
        auto floatProperty  = CreateInputProperty("Property", EPropertyType::Float);
        auto vec2fProperty  = CreateInputProperty("Property", EPropertyType::Vec2f);
        auto vec3iProperty  = CreateInputProperty("Property", EPropertyType::Vec3i);
        auto stringProperty = CreateInputProperty("Property", EPropertyType::String);

        intProperty->setManually(42);
        floatProperty->setManually(42.f);
        vec2fProperty->setManually(vec2f{4.f, 2.f});
        vec3iProperty->setManually(vec3i{4, 2, 3});
        stringProperty->setManually(std::string("42"));

        EXPECT_TRUE(intProperty->setManually(43));
        EXPECT_TRUE(floatProperty->setManually(43.f));
        EXPECT_TRUE(vec2fProperty->setManually(vec2f{4.f, 3.f}));
        EXPECT_TRUE(vec3iProperty->setManually(vec3i{4, 3, 3}));
        EXPECT_TRUE(stringProperty->setManually(std::string("43")));

        EXPECT_TRUE(intProperty->getLogicNode().isDirty());
        EXPECT_TRUE(floatProperty->getLogicNode().isDirty());
        EXPECT_TRUE(vec2fProperty->getLogicNode().isDirty());
        EXPECT_TRUE(vec3iProperty->getLogicNode().isDirty());
        EXPECT_TRUE(stringProperty->getLogicNode().isDirty());
    }
}
