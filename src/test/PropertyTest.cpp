//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "internals/impl/PropertyImpl.h"
#include "internals/impl/LuaScriptImpl.h"
#include "internals/impl/LuaStateImpl.h"

#include "ramses-logic/Property.h"
#include "ramses-logic/LogicEngine.h"

#include "flatbuffers/flatbuffers.h"

#include "generated/property_gen.h"

#include <memory>
#include "LogicNodeDummy.h"

namespace rlogic::internal
{
    class AProperty : public ::testing::Test
    {
    public:
        AProperty();

        std::vector<std::string>       m_unusedScriptErrorVector;
        LuaStateImpl                   state;
        std::unique_ptr<LuaScriptImpl> script;

        static std::unique_ptr < PropertyImpl> Deserialize(const void* buffer)
        {
            const auto propertyFB = rlogic_serialization::GetProperty(buffer);
            return PropertyImpl::Create(propertyFB, EInputOutputProperty::Input);
        }
    };

    AProperty::AProperty()
        : script(LuaScriptImpl::Create(state, "", "", "", m_unusedScriptErrorVector))
    {
    }


    TEST_F(AProperty, HasANameAfterCreation)
    {
        Property desc(std::make_unique<PropertyImpl>("PropertyName", EPropertyType::Float, EInputOutputProperty::Input));
        EXPECT_EQ("PropertyName", desc.getName());
    }

    TEST_F(AProperty, HasATypeAfterCreation)
    {
        Property desc(std::make_unique<PropertyImpl>("PropertyName", EPropertyType::Float, EInputOutputProperty::Input));
        EXPECT_EQ(EPropertyType::Float, desc.getType());
    }

    TEST_F(AProperty, HasUserValueOnlyAfterSetIsCalledSuccessfully)
    {
        Property desc(std::make_unique<PropertyImpl>("PropertyName", EPropertyType::Float, EInputOutputProperty::Input));
        EXPECT_FALSE(desc.m_impl->wasSet());
        EXPECT_FALSE(desc.set<int32_t>(5));
        EXPECT_FALSE(desc.m_impl->wasSet());
        EXPECT_TRUE(desc.set<float>(0.5f));
        EXPECT_TRUE(desc.m_impl->wasSet());
    }

    TEST_F(AProperty, DoesntHaveChildrenAfterCreation)
    {
        Property desc(std::make_unique<PropertyImpl>("PropertyName", EPropertyType::Float, EInputOutputProperty::Input));
        ASSERT_EQ(0u, desc.getChildCount());
    }

    TEST_F(AProperty, ReturnsDefaultValue_ForPrimitiveTypes)
    {
        Property aFloat(std::make_unique<PropertyImpl>("", EPropertyType::Float, EInputOutputProperty::Input));
        ASSERT_TRUE(aFloat.get<float>());
        EXPECT_FLOAT_EQ(0.0f, *aFloat.get<float>());

        Property aInt(std::make_unique<PropertyImpl>("", EPropertyType::Int32, EInputOutputProperty::Input));
        ASSERT_TRUE(aInt.get<int32_t>());
        EXPECT_EQ(0, *aInt.get<int32_t>());

        Property aBool(std::make_unique<PropertyImpl>("", EPropertyType::Bool, EInputOutputProperty::Input));
        ASSERT_TRUE(aBool.get<bool>());
        EXPECT_EQ(false, *aBool.get<bool>());

        Property aString(std::make_unique<PropertyImpl>("", EPropertyType::String, EInputOutputProperty::Input));
        ASSERT_TRUE(aString.get<std::string>());
        EXPECT_EQ("", *aString.get<std::string>());
    }

    TEST_F(AProperty, ReturnsDefaultValue_VectorTypes)
    {
        Property aVec2f(std::make_unique<PropertyImpl>("", EPropertyType::Vec2f, EInputOutputProperty::Input));
        Property aVec3f(std::make_unique<PropertyImpl>("", EPropertyType::Vec3f, EInputOutputProperty::Input));
        Property aVec4f(std::make_unique<PropertyImpl>("", EPropertyType::Vec4f, EInputOutputProperty::Input));
        Property aVec2i(std::make_unique<PropertyImpl>("", EPropertyType::Vec2i, EInputOutputProperty::Input));
        Property aVec3i(std::make_unique<PropertyImpl>("", EPropertyType::Vec3i, EInputOutputProperty::Input));
        Property aVec4i(std::make_unique<PropertyImpl>("", EPropertyType::Vec4i, EInputOutputProperty::Input));

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
        Property aFloat(std::make_unique<PropertyImpl>("", EPropertyType::Float, EInputOutputProperty::Input));
        Property aInt32(std::make_unique<PropertyImpl>("", EPropertyType::Int32, EInputOutputProperty::Input));
        Property aBool(std::make_unique<PropertyImpl>("", EPropertyType::Bool, EInputOutputProperty::Input));
        Property aString(std::make_unique<PropertyImpl>("", EPropertyType::String, EInputOutputProperty::Input));

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
        Property avec2f(std::make_unique<PropertyImpl>("", EPropertyType::Vec2f, EInputOutputProperty::Input));
        Property avec3f(std::make_unique<PropertyImpl>("", EPropertyType::Vec3f, EInputOutputProperty::Input));
        Property avec4f(std::make_unique<PropertyImpl>("", EPropertyType::Vec4f, EInputOutputProperty::Input));

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
        Property avec2i(std::make_unique<PropertyImpl>("", EPropertyType::Vec2i, EInputOutputProperty::Input));
        Property avec3i(std::make_unique<PropertyImpl>("", EPropertyType::Vec3i, EInputOutputProperty::Input));
        Property avec4i(std::make_unique<PropertyImpl>("", EPropertyType::Vec4i, EInputOutputProperty::Input));

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
        Property inputProperty(std::make_unique<PropertyImpl>("Input", EPropertyType::Float, EInputOutputProperty::Input));
        Property outputProperty(std::make_unique<PropertyImpl>("Output", EPropertyType::Int32, EInputOutputProperty::Output));

        EXPECT_TRUE(inputProperty.m_impl->isInput());
        EXPECT_FALSE(inputProperty.m_impl->isOutput());
        EXPECT_EQ(internal::EInputOutputProperty::Input, inputProperty.m_impl->getInputOutputProperty());
        EXPECT_TRUE(outputProperty.m_impl->isOutput());
        EXPECT_FALSE(outputProperty.m_impl->isInput());
        EXPECT_EQ(internal::EInputOutputProperty::Output, outputProperty.m_impl->getInputOutputProperty());
    }

    TEST_F(AProperty, ReturnsNoValueWhenAccessingWithWrongType)
    {
        Property floatProp(std::make_unique<PropertyImpl>("", EPropertyType::Float, EInputOutputProperty::Input));
        Property int32Prop(std::make_unique<PropertyImpl>("", EPropertyType::Int32, EInputOutputProperty::Input));
        Property boolProp(std::make_unique<PropertyImpl>("", EPropertyType::Bool, EInputOutputProperty::Input));
        Property stringProp(std::make_unique<PropertyImpl>("", EPropertyType::String, EInputOutputProperty::Input));
        Property structProp(std::make_unique<PropertyImpl>("", EPropertyType::Struct, EInputOutputProperty::Input));

        EXPECT_TRUE(floatProp.get<float>());
        EXPECT_FALSE(floatProp.get<int32_t>());
        EXPECT_FALSE(floatProp.get<bool>());
        EXPECT_FALSE(floatProp.get<std::string>());

        EXPECT_TRUE(int32Prop.get<int32_t>());
        EXPECT_FALSE(int32Prop.get<float>());
        EXPECT_FALSE(int32Prop.get<bool>());
        EXPECT_FALSE(int32Prop.get<std::string>());

        EXPECT_TRUE(boolProp.get<bool>());
        EXPECT_FALSE(boolProp.get<int32_t>());
        EXPECT_FALSE(boolProp.get<float>());
        EXPECT_FALSE(boolProp.get<std::string>());

        EXPECT_TRUE(stringProp.get<std::string>());
        EXPECT_FALSE(stringProp.get<bool>());
        EXPECT_FALSE(stringProp.get<int32_t>());
        EXPECT_FALSE(stringProp.get<float>());

        EXPECT_FALSE(structProp.get<std::string>());
        EXPECT_FALSE(structProp.get<bool>());
        EXPECT_FALSE(structProp.get<int32_t>());
        EXPECT_FALSE(structProp.get<float>());
    }

    TEST_F(AProperty, ReturnsNullptrForGetChildByIndexIfPropertyHasNoChildren)
    {
        Property property_float(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Float, EInputOutputProperty::Input));

        EXPECT_EQ(nullptr, property_float.getChild(0));
    }

    TEST_F(AProperty, ReturnsNullptrForGetChildByNameIfPropertyHasNoChildren)
    {
        Property property_float(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Float, EInputOutputProperty::Input));

        EXPECT_EQ(nullptr, property_float.getChild("child"));
    }

    TEST_F(AProperty, DoesNotAddChildIfTypeIsNotStruct)
    {
        auto rootImpl = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Float, EInputOutputProperty::Input);
        rootImpl->addChild(std::make_unique<PropertyImpl>("ChildProperty", EPropertyType::Float, EInputOutputProperty::Input));

        Property root(std::move(rootImpl));

        EXPECT_EQ(0, root.getChildCount());
        EXPECT_EQ(nullptr, root.getChild(0));
    }

    TEST_F(AProperty, AddsChildIfTypeIsStruct)
    {
        auto rootImpl = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, EInputOutputProperty::Input);
        rootImpl->addChild(std::make_unique<PropertyImpl>("ChildProperty", EPropertyType::Float, EInputOutputProperty::Input));

        Property root(std::move(rootImpl));

        ASSERT_EQ(1, root.getChildCount());
        EXPECT_EQ("ChildProperty", root.getChild(0)->getName());
        EXPECT_EQ(EPropertyType::Float, root.getChild(0)->getType());
    }

    TEST_F(AProperty, CanBeEmptyAndConst)
    {
        auto           rootImpl = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, EInputOutputProperty::Input);
        const Property root(std::move(rootImpl));

        const auto child = root.getChild(0);
        EXPECT_EQ(nullptr, child);
    }

    TEST_F(AProperty, CanHaveNestedProperties)
    {
        auto rootImpl = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, EInputOutputProperty::Input);

        rootImpl->addChild(std::make_unique<PropertyImpl>("PropertyChild1", EPropertyType::Int32, EInputOutputProperty::Input));
        rootImpl->addChild(std::make_unique<PropertyImpl>("PropertyChild2", EPropertyType::Float, EInputOutputProperty::Input));

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

    TEST_F(AProperty, SetsValueIfTheTypeMatches)
    {
        Property floatProperty(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Float, EInputOutputProperty::Input));
        Property int32Property(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Int32, EInputOutputProperty::Input));
        Property stringProperty(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::String, EInputOutputProperty::Input));
        Property boolProperty(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Bool, EInputOutputProperty::Input));

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
        Property floatProperty(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Float, EInputOutputProperty::Input));
        Property int32Property(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Int32, EInputOutputProperty::Input));
        Property stringProperty(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::String, EInputOutputProperty::Input));
        Property boolProperty(std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Bool, EInputOutputProperty::Input));

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
        auto rootImpl = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, EInputOutputProperty::Input);

        rootImpl->addChild(std::make_unique<PropertyImpl>("PropertyChild1", EPropertyType::Int32, EInputOutputProperty::Input));
        rootImpl->addChild(std::make_unique<PropertyImpl>("PropertyChild2", EPropertyType::Float, EInputOutputProperty::Input));

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
        auto rootImpl = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, EInputOutputProperty::Input);

        rootImpl->addChild(std::make_unique<PropertyImpl>("PropertyChild1", EPropertyType::Int32, EInputOutputProperty::Input));
        rootImpl->addChild(std::make_unique<PropertyImpl>("PropertyChild2", EPropertyType::Float, EInputOutputProperty::Input));

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
            auto rootImpl = std::make_unique<PropertyImpl>("EmptyProperty", EPropertyType::Struct, EInputOutputProperty::Input);

            rootImpl->serialize(builder);
        }
        {
            auto root = Deserialize(builder.GetBufferPointer());

            ASSERT_EQ(0u, root->getChildCount());
            EXPECT_EQ(EPropertyType::Struct, root->getType());
            EXPECT_EQ("EmptyProperty", root->getName());
            EXPECT_EQ(EInputOutputProperty::Input, root->getInputOutputProperty());
        }
    }

    TEST_F(AProperty, CanBeSerializedAndDeserialized_ForAllSupportedTypes)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto rootImpl      = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, EInputOutputProperty::Input);
            auto propInt32     = std::make_unique<PropertyImpl>("PropertyInt32", EPropertyType::Int32, EInputOutputProperty::Input);
            auto propFloat     = std::make_unique<PropertyImpl>("PropertyFloat", EPropertyType::Float, EInputOutputProperty::Input);
            auto propBool      = std::make_unique<PropertyImpl>("PropertyBool", EPropertyType::Bool, EInputOutputProperty::Input);
            auto propString    = std::make_unique<PropertyImpl>("PropertyString", EPropertyType::String, EInputOutputProperty::Input);
            auto propVec2f     = std::make_unique<PropertyImpl>("PropertyVec2f", EPropertyType::Vec2f, EInputOutputProperty::Input);
            auto propVec3f     = std::make_unique<PropertyImpl>("PropertyVec3f", EPropertyType::Vec3f, EInputOutputProperty::Input);
            auto propVec4f     = std::make_unique<PropertyImpl>("PropertyVec4f", EPropertyType::Vec4f, EInputOutputProperty::Input);
            auto propVec2i     = std::make_unique<PropertyImpl>("PropertyVec2i", EPropertyType::Vec2i, EInputOutputProperty::Input);
            auto propVec3i     = std::make_unique<PropertyImpl>("PropertyVec3i", EPropertyType::Vec3i, EInputOutputProperty::Input);
            auto propVec4i     = std::make_unique<PropertyImpl>("PropertyVec4i", EPropertyType::Vec4i, EInputOutputProperty::Input);
            auto propWasNotSet = std::make_unique<PropertyImpl>("PropertyDefaultValue", EPropertyType::Vec4i, EInputOutputProperty::Input);

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

            rootImpl->addChild(std::move(propInt32));
            rootImpl->addChild(std::move(propFloat));
            rootImpl->addChild(std::move(propBool));
            rootImpl->addChild(std::move(propString));
            rootImpl->addChild(std::move(propVec2f));
            rootImpl->addChild(std::move(propVec3f));
            rootImpl->addChild(std::move(propVec4f));
            rootImpl->addChild(std::move(propVec2i));
            rootImpl->addChild(std::move(propVec3i));
            rootImpl->addChild(std::move(propVec4i));
            rootImpl->addChild(std::move(propWasNotSet));

            rootImpl->serialize(builder);
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

            EXPECT_EQ("PropertyInt32", propInt32->getName());
            EXPECT_EQ("PropertyFloat", propFloat->getName());
            EXPECT_EQ("PropertyBool", propBool->getName());
            EXPECT_EQ("PropertyString", propString->getName());
            EXPECT_EQ("PropertyVec2f", propVec2f->getName());
            EXPECT_EQ("PropertyVec3f", propVec3f->getName());
            EXPECT_EQ("PropertyVec4f", propVec4f->getName());
            EXPECT_EQ("PropertyVec2i", propVec2i->getName());
            EXPECT_EQ("PropertyVec3i", propVec3i->getName());
            EXPECT_EQ("PropertyVec4i", propVec4i->getName());
            EXPECT_EQ("PropertyDefaultValue", propDefValue->getName());


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
            EXPECT_FALSE(propDefValue->m_impl->wasSet());
        }
    }

    TEST_F(AProperty, KeepsOriginalPropertyOrderAfterDeserialization)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto propertyRoot = std::make_unique<PropertyImpl>("PropertyInt", EPropertyType::Struct, EInputOutputProperty::Input);
            auto c1 = std::make_unique<PropertyImpl>("PropertyFloat1", EPropertyType::Float, EInputOutputProperty::Input);
            auto c2 = std::make_unique<PropertyImpl>("PropertyFloat2", EPropertyType::Float, EInputOutputProperty::Input);
            auto c3 = std::make_unique<PropertyImpl>("PropertyFloat3", EPropertyType::Float, EInputOutputProperty::Input);

            propertyRoot->addChild(std::move(c1));
            propertyRoot->addChild(std::move(c2));
            propertyRoot->addChild(std::move(c3));

            propertyRoot->serialize(builder);
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
            auto propertyRoot    = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, EInputOutputProperty::Input);
            auto propertyNested1 = std::make_unique<PropertyImpl>("PropertyNested", EPropertyType::Struct, EInputOutputProperty::Input);
            auto propertyFloat1  = std::make_unique<PropertyImpl>("PropertyFloat", EPropertyType::Float, EInputOutputProperty::Input);
            auto propertyNested2 = std::make_unique<PropertyImpl>("PropertyNested", EPropertyType::Struct, EInputOutputProperty::Input);
            auto propertyFloat2  = std::make_unique<PropertyImpl>("PropertyFloat", EPropertyType::Float, EInputOutputProperty::Input);

            propertyNested1->addChild(std::move(propertyFloat1));
            propertyNested2->addChild(std::move(propertyFloat2));
            propertyNested1->addChild(std::move(propertyNested2));
            propertyRoot->addChild(std::move(propertyNested1));


            propertyRoot->serialize(builder);
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

    TEST_F(AProperty, SetsLogicNodeRecursiveToAllChildren)
    {
        auto logicNode = LogicNodeDummy::Create("LogicNode");
        auto propertyRoot = std::make_unique<PropertyImpl>("PropertyRoot", EPropertyType::Struct, internal::EInputOutputProperty::Input);
        propertyRoot->addChild(std::make_unique<PropertyImpl>("PropertyFloat1", EPropertyType::Float, internal::EInputOutputProperty::Input));
        propertyRoot->addChild(std::make_unique<PropertyImpl>("PropertyFloat2", EPropertyType::Float, internal::EInputOutputProperty::Input));

        propertyRoot->setLogicNode(logicNode->m_impl.get());

        EXPECT_EQ(&logicNode->m_impl.get(), &propertyRoot->getLogicNode());
        EXPECT_EQ(&logicNode->m_impl.get(), &propertyRoot->getChild("PropertyFloat1")->m_impl->getLogicNode());
        EXPECT_EQ(&logicNode->m_impl.get(), &propertyRoot->getChild("PropertyFloat2")->m_impl->getLogicNode());
    }
}
