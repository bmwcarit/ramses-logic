//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/Property.h"

#include "ramses-client.h"
#include "ramses-utils.h"

#include <string>
#include <array>
#include <algorithm>
#include <cassert>
#include <iostream>

/**
 * This example demonstrates how to save the contents of the LogicEngine class
 * to a file and then load it again, including references to a Ramses scene.
 */


void CreateAndSaveContent(const std::string& ramsesSceneFile, const std::string& ramsesLogicFile);

int main()
{
    /**
     * Define file names to use where content will be saved
     */
    const std::string ramsesSceneFile = "scene.ramses";
    const std::string ramsesLogicFile = "scene.logic";

    /**
     * Create a simple triangle scene and a script which controls it. Save both to its own file.
     */
    CreateAndSaveContent(ramsesSceneFile, ramsesLogicFile);

    /**
     * Load the Ramses scene from the file. It has to be loaded first, so that we can
     * resolve the Ramses objects when we load the logic content later.
     */
    ramses::RamsesFramework ramsesFramework;
    ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
    ramses::Scene* scene = client.loadSceneFromFile(ramsesSceneFile.c_str());

    /**
     * Load the logic content from its file, and provide a pointer to the ramses scene as argument.
     * After loadFromFile() returns, the bindings which point to Ramses objects will point
     * to objects from the scene provided as an argument.
     */
    rlogic::LogicEngine logicEngine;
    logicEngine.loadFromFile(ramsesLogicFile, scene);

    /**
     * rlogic::LogicEngine provides iterable collections to its objects. We can use them to resolve objects after loading
     */
    rlogic::Collection<rlogic::LuaScript> loadedScripts = logicEngine.scripts();
    rlogic::Collection<rlogic::RamsesNodeBinding> loadedNodeBindings = logicEngine.ramsesNodeBindings();

    /**
     * We can use any STL algorithm on the collections.
     * In this simple example, we use find_if() to search for a specific script by its name.
     * We get an iterator which can be dereferenced to rlogic::LuaScript*
     */
    rlogic::Collection<rlogic::LuaScript>::iterator triangleRotationScript = std::find_if(loadedScripts.begin(), loadedScripts.end(), [](rlogic::LuaScript* script) {return script->getName() == "simple rotation script"; });

    /**
     * We can do the same to find a ramses node binding. We can use the binding to obtain a pointer to the ramses::Node further down.
     * This is an alternative to ramses::Scene::findObjectById()/findObjectByName() methods.
     * Note that this example uses fully qualified names and type traits for documentation sake. You can also just use 'auto'.
     */
    rlogic::Collection<rlogic::RamsesNodeBinding>::const_iterator triangleNodeBinding = std::find_if(loadedNodeBindings.cbegin(), loadedNodeBindings.cend(), [](const rlogic::RamsesNodeBinding* binding) {return binding->getName() == "link to triangle node"; });

    /**
     * The LogicEngine iterators work just like any other STL forward iterator - can be compared, dereferenced, incremented etc.
     */
    assert(triangleRotationScript != loadedScripts.end());
    assert(triangleNodeBinding != loadedNodeBindings.cend());

    /**
     * Changing properties on the freshly loaded LogicEngine and calling update() works as expected - executes all scripts,
     * passes data over the linked logic nodes, and updates the values in the ramses scene.
     */
    triangleRotationScript->getInputs()->getChild("time_msec")->set<int32_t>(300);
    logicEngine.update();

    std::array<float, 3> nodeRotation{};
    ramses::ERotationConvention unused;
    (void)unused;
    triangleNodeBinding->getRamsesNode()->getRotation(nodeRotation[0], nodeRotation[1], nodeRotation[2], unused);

    std::cout << "\n\nRamses node rotation after loading from file and updating: {" << nodeRotation[0] << ", " << nodeRotation[1] << ", " << nodeRotation[2] << "}\n\n";

    return 0;
}


/**
 * Helper method which creates a simple ramses scene, a simple script, and
 * saves the content in two separate files
 */
void CreateAndSaveContent(const std::string &ramsesSceneFile, const std::string& ramsesLogicFile)
{
    /**
     * Boilerplate Ramses code which saves a red triangle scene in a file. For more ramses
     * examples and details, check the ramses docs at https://genivi.github.io/ramses
     */
    ramses::RamsesFramework ramsesFramework;
    ramses::RamsesClient& client = *ramsesFramework.createClient("example client");

    ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "red triangle scene");

    ramses::PerspectiveCamera* camera = scene->createPerspectiveCamera();
    camera->setFrustum(19.0f, 1.0f, 0.1f, 100.0f);
    camera->setViewport(0, 0, 800, 800);
    camera->setTranslation(0.0f, 0.0f, 5.0f);
    ramses::RenderPass* renderPass = scene->createRenderPass();
    renderPass->setClearFlags(ramses::EClearFlags_None);
    renderPass->setCamera(*camera);
    ramses::RenderGroup* renderGroup = scene->createRenderGroup();
    renderPass->addRenderGroup(*renderGroup);

    const std::array<float, 9u> vertexPositionsArray = { -1.f, 0.f, -1.f, 1.f, 0.f, -1.f, 0.f, 1.f, -1.f };
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

    ramses::MeshNode* meshNode = scene->createMeshNode("triangle mesh node");
    meshNode->setAppearance(*appearance);
    meshNode->setIndexCount(3);
    meshNode->setGeometryBinding(*geometry);

    renderGroup->addMeshNode(*meshNode);

    scene->flush();
    /**
     * It doesn't matter if we save the Ramses scene first or the logic scene first, as long as the Node we
     * reference further down has its ID serialized in both files, so that it can be resolved again when loading.
     */
    scene->saveToFile(ramsesSceneFile.c_str(), false);

    /**
     * Create a temporary LogicEngine instance for creating and saving a simple script which references a ramses Node
     */
    rlogic::LogicEngine logicEngine;
    rlogic::RamsesNodeBinding* nodeBinding = logicEngine.createRamsesNodeBinding("link to triangle node");

    /**
     * Assign the meshNode created above to the node binding object.
     */
    nodeBinding->setRamsesNode(meshNode);

    /**
     * Create a simple script which sets the rotation values of a node based on simulated time
     */
    rlogic::LuaScript* simpleScript = logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.time_msec = INT
                OUT.rotationZ = VEC3F
            end

            function run()
                -- Rotate around Z axis with 100 degrees per second
                OUT.rotationZ = {0, 0, IN.time_msec / 10}
            end
        )", "simple rotation script");

    /**
     * Link the script output to the node binding input so that the value produced by the script is passed to the ramses node on update
     */
    logicEngine.link(*simpleScript->getOutputs()->getChild("rotationZ"), *nodeBinding->getInputs()->getChild("rotation"));

    /**
     * Save the script, the node binding and their link to a file so that they can be loaded later
     */
    logicEngine.saveToFile(ramsesLogicFile);
}
