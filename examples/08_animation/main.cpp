//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/AnimationTypes.h"
#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/TimerNode.h"

#include "ramses-client.h"
#include "ramses-utils.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

/**
* This example demonstrates how to use animation to animate
* Ramses scene content.
*/

struct SceneAndNodes
{
    ramses::Scene* scene;
    ramses::Node* node1;
    ramses::Node* node2;
};

/**
* Helper method which creates a simple ramses scene. For more ramses
* examples, check the ramses docs at https://covesa.github.io/ramses
*/
SceneAndNodes CreateSceneWithTriangles(ramses::RamsesClient& client);

int main(int argc, char* argv[])
{
    /**
     * Create Ramses framework and client objects. Ramses Logic does not manage
     * or encapsulate Ramses objects - it only interacts with existing Ramses objects.
     * The application must take special care to not destroy Ramses objects while a
     * LogicEngine instance is still referencing them!
     */
    ramses::RamsesFramework ramsesFramework(argc, argv);
    ramses::RamsesClient* ramsesClient = ramsesFramework.createClient("example client");

    /**
     * To keep this example simple, we don't include a Renderer, but only provide the scene
     * over network. Start a ramses daemon and a renderer additionally to see the visual result!
     * The connect() ensures the scene published in this example will be distributed over network.
     */
    ramsesFramework.connect();

    /**
     * Create a test Ramses scene with two simple triangles to be animated separately.
     */
    auto [scene, tri1, tri2] = CreateSceneWithTriangles(*ramsesClient);

    rlogic::LogicEngine logicEngine;

    /**
    * Create a binding object which serves as a bridge between logic nodes and animations on one end
    * and a Ramses scene on the other end.
    */
    rlogic::RamsesNodeBinding* nodeBinding1 = logicEngine.createRamsesNodeBinding(*tri1);
    rlogic::RamsesNodeBinding* nodeBinding2 = logicEngine.createRamsesNodeBinding(*tri2);

    /**
     * Create two simple animations (cubic and step) by providing keyframes and timestamps.
     * Animations have a single key-frame channel in this example for simplicity.
     *
     * First, create the data arrays which contain the time stamp data, the key-frame data points, and tangent arrays for the cubic animation.
     */
    rlogic::DataArray* animTimestamps = logicEngine.createDataArray(std::vector<float>{ 0.f, 0.5f, 1.f, 1.5f }); // will be interpreted as seconds
    rlogic::DataArray* animKeyframes = logicEngine.createDataArray(std::vector<rlogic::vec3f>{ {0.f, 0.f, 0.f}, {0.f, 0.f, 180.f}, {0.f, 0.f, 100.f}, {0.f, 0.f, 360.f} });
    rlogic::DataArray* cubicAnimTangentsIn = logicEngine.createDataArray(std::vector<rlogic::vec3f>{ {0.f, 0.f, 0.f}, { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f } });
    rlogic::DataArray* cubicAnimTangentsOut = logicEngine.createDataArray(std::vector<rlogic::vec3f>{ {0.f, 0.f, 0.f}, { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f } });

    /**
     * Create a channel for each animation - cubic and nearest/step.
     */
    const rlogic::AnimationChannel cubicAnimChannel { "rotationZcubic", animTimestamps, animKeyframes, rlogic::EInterpolationType::Cubic, cubicAnimTangentsIn, cubicAnimTangentsOut };
    const rlogic::AnimationChannel stepAnimChannel { "rotationZstep", animTimestamps, animKeyframes, rlogic::EInterpolationType::Step };

    /**
     * Finally, create the animation nodes by passing in the channel data
     */
    rlogic::AnimationNode* cubicAnimNode = logicEngine.createAnimationNode({ cubicAnimChannel });
    rlogic::AnimationNode* stepAnimNode = logicEngine.createAnimationNode({ stepAnimChannel });

    /**
    * Connect the animation channel 'rotationZ' output with the rotation property of the RamsesNodeBinding object.
    * After this, the value computed in the animation output channel will be propagated to the ramses node's rotation property.
    */
    logicEngine.link(
        *cubicAnimNode->getOutputs()->getChild("rotationZcubic"),
        *nodeBinding1->getInputs()->getChild("rotation"));
    logicEngine.link(
        *stepAnimNode->getOutputs()->getChild("rotationZstep"),
        *nodeBinding2->getInputs()->getChild("rotation"));

    /**
    * We need to provide time information to the animation nodes, the easiest way is to create a TimerNode
    * and link its 'timeDelta' output to all animation nodes' 'timeDelta' input.
    * Timer node will internally get a system time ticker on every update, it will calculate a 'timeDelta' (time period since last update)
    * which is exactly what an animation node needs to advance its animation. See TimerNode API for more use cases.
    */
    rlogic::TimerNode* timer = logicEngine.createTimerNode();
    logicEngine.link(
        *timer->getOutputs()->getChild("timeDelta"),
        *cubicAnimNode->getInputs()->getChild("timeDelta"));
    logicEngine.link(
        *timer->getOutputs()->getChild("timeDelta"),
        *stepAnimNode->getInputs()->getChild("timeDelta"));

    /**
     * Start the cubic animation right away.
     */
    cubicAnimNode->getInputs()->getChild("play")->set(true);

    /**
     * Simulate an application loop.
     */
    for(int loop = 0; loop < 500; ++loop)
    {
        /**
         * Query progress of cubic animation and if finished, trigger play of step animation.
         * Note that this logic can also be implemented as a simple Lua script plugged
         * in between 'progress' output of cubic animation and 'play' input of step animation.
         */
        if (cubicAnimNode->getOutputs()->getChild("progress")->get<float>() > 0.999f)
            stepAnimNode->getInputs()->getChild("play")->set(true);

        /**
        * Update the LogicEngine. This will apply changes to Ramses scene from any running animation.
        */
        logicEngine.update();

        /**
        * In order to commit the changes to Ramses scene caused by animations logic we need to "flush" them.
        */
        scene->flush();

        /**
        * Throttle the simulation loop by sleeping for a bit.
        */
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    /**
    * Ramses logic objects are managed and will be automatically released with destruction of the LogicEngine instance,
    * however it is good practice to destroy objects if they are not going to be needed anymore.
    * When destroying manually, keep order in mind, any logic content referencing a Ramses scene should be destroyed
    * before the scene. Similarly objects using DataArray instances (e.g. AnimationNodes) should be destroyed before
    * the data arrays. Generally objects referencing other objects should always be destroyed first.
    */
    logicEngine.destroy(*cubicAnimNode);
    logicEngine.destroy(*stepAnimNode);
    logicEngine.destroy(*animTimestamps);
    logicEngine.destroy(*animKeyframes);
    logicEngine.destroy(*cubicAnimTangentsIn);
    logicEngine.destroy(*cubicAnimTangentsOut);
    logicEngine.destroy(*nodeBinding1);
    logicEngine.destroy(*nodeBinding2);
    ramsesClient->destroy(*scene);

    return 0;
}


SceneAndNodes CreateSceneWithTriangles(ramses::RamsesClient& client)
{
    ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "red triangle scene");

    ramses::PerspectiveCamera* camera = scene->createPerspectiveCamera();
    camera->setFrustum(19.0f, 1280.f/800.f, 0.1f, 100.0f);
    camera->setViewport(0, 0, 1280, 800);
    camera->setTranslation(0.0f, 0.0f, 10.0f);
    ramses::RenderPass* renderPass = scene->createRenderPass();
    renderPass->setClearFlags(ramses::EClearFlags_None);
    renderPass->setCamera(*camera);
    ramses::RenderGroup* renderGroup = scene->createRenderGroup();
    renderPass->addRenderGroup(*renderGroup);

    std::array<float, 9u> vertexPositionsArray = { -1.f, 0.f, -1.f, 1.f, 0.f, -1.f, 0.f, 1.f, -1.f };
    ramses::ArrayResource* vertexPositions = scene->createArrayResource(ramses::EDataType::Vector3F, 3, vertexPositionsArray.data());

    ramses::EffectDescription effectDesc;
    effectDesc.setVertexShader(R"(
        #version 100

        uniform highp mat4 mvpMatrix;

        attribute vec3 a_position;

        void main()
        {
            gl_Position = mvpMatrix * vec4(a_position, 1.0);
        }
        )");
    effectDesc.setFragmentShader(R"(
        #version 100

        void main(void)
        {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
        )");

    effectDesc.setUniformSemantic("mvpMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix);

    const ramses::Effect* effect = scene->createEffect(effectDesc, ramses::ResourceCacheFlag_DoNotCache);
    ramses::Appearance* appearance = scene->createAppearance(*effect);

    ramses::GeometryBinding* geometry = scene->createGeometryBinding(*effect);
    ramses::AttributeInput positionsInput;
    effect->findAttributeInput("a_position", positionsInput);
    geometry->setInputBuffer(positionsInput, *vertexPositions);

    ramses::MeshNode* meshNode1 = scene->createMeshNode("triangle mesh node 1");
    ramses::MeshNode* meshNode2 = scene->createMeshNode("triangle mesh node 2");
    meshNode1->setAppearance(*appearance);
    meshNode1->setIndexCount(3);
    meshNode1->setGeometryBinding(*geometry);
    meshNode2->setAppearance(*appearance);
    meshNode2->setIndexCount(3);
    meshNode2->setGeometryBinding(*geometry);

    meshNode1->setTranslation(-1.f, -0.8f, 0.f);
    meshNode2->setTranslation(1.f, -0.8f, 0.f);

    renderGroup->addMeshNode(*meshNode1);
    renderGroup->addMeshNode(*meshNode2);

    scene->flush();
    scene->publish();

    return SceneAndNodes{ scene, meshNode1, meshNode2 };
}
