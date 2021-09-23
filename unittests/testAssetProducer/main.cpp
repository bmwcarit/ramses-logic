//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"

#include "ramses-client.h"
#include "ramses-utils.h"

ramses::Appearance* createTestAppearance(ramses::Scene& scene)
{
    const std::string_view vertShader = R"(
                #version 100

                uniform highp float floatUniform;
                uniform highp float animatedFloatUniform;
                attribute vec3 a_position;

                void main()
                {
                    gl_Position = floatUniform * animatedFloatUniform * vec4(a_position, 1.0);
                })";

    const std::string_view fragShader = R"(
                #version 100

                void main(void)
                {
                    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
                })";

    ramses::EffectDescription effectDesc;
    effectDesc.setUniformSemantic("u_DisplayBufferResolution", ramses::EEffectUniformSemantic::DisplayBufferResolution);
    effectDesc.setVertexShader(vertShader.data());
    effectDesc.setFragmentShader(fragShader.data());

    return scene.createAppearance(*scene.createEffect(effectDesc), "test appearance");
}

int main(int argc, char* argv[])
{
    ramses::RamsesFramework ramsesFramework(argc, argv);
    ramses::RamsesClient* ramsesClient = ramsesFramework.createClient("");

    ramses::Scene* scene = ramsesClient->createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "");
    scene->flush();
    rlogic::LogicEngine logicEngine;

    rlogic::LuaScript* script1 = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            IN.intInput =      INT
            IN.vec2iInput =    VEC2I
            IN.vec3iInput =    VEC3I
            IN.vec4iInput =    VEC4I
            IN.floatInput =    FLOAT
            IN.vec2fInput =    VEC2F
            IN.vec3fInput =    VEC3F
            IN.vec4fInput =    VEC4F
            IN.boolInput  =     BOOL
            IN.stringInput =   STRING
            IN.structInput = {
                nested = {
                    data1 = STRING,
                    data2 = INT
                }
            }
            IN.arrayInput =    ARRAY(9, FLOAT)
            OUT.floatOutput = FLOAT
            OUT.nodeTranslation = VEC3F
        end
        function run()
            OUT.floatOutput = IN.floatInput
            OUT.nodeTranslation = {IN.floatInput, 2, 3}
        end
    )", "script1");
    rlogic::LuaScript* script2 = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            IN.floatInput = FLOAT

            OUT.cameraViewport = {
                offsetX = INT,
                offsetY = INT,
                width = INT,
                height = INT
            }
            OUT.floatUniform = FLOAT
        end
        function run()
            OUT.floatUniform = IN.floatInput + 5.0
            local roundedFloat = math.ceil(IN.floatInput)
            OUT.cameraViewport = {
                offsetX = 2 + roundedFloat,
                offsetY = 4 + roundedFloat,
                width = 100 + roundedFloat,
                height = 200 + roundedFloat
            }
        end
    )", "script2");

    ramses::Node* node = { scene->createNode("test node") };
    ramses::OrthographicCamera* camera = { scene->createOrthographicCamera("test camera") };
    ramses::Appearance* appearance = { createTestAppearance(*scene) };

    rlogic::RamsesNodeBinding* nodeBinding = logicEngine.createRamsesNodeBinding(*node, rlogic::ERotationType::Euler_XYZ, "nodebinding");
    rlogic::RamsesCameraBinding* camBinding = logicEngine.createRamsesCameraBinding(*camera, "camerabinding");
    rlogic::RamsesAppearanceBinding* appBinding = logicEngine.createRamsesAppearanceBinding(*appearance, "appearancebinding");
    const auto dataArray = logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f }, "dataarray");
    const auto animNode = logicEngine.createAnimationNode({ { "channel", dataArray, dataArray, rlogic::EInterpolationType::Linear } }, "animNode");

    logicEngine.link(*script1->getOutputs()->getChild("floatOutput"), *script2->getInputs()->getChild("floatInput"));
    logicEngine.link(*script1->getOutputs()->getChild("nodeTranslation"), *nodeBinding->getInputs()->getChild("translation"));
    logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("offsetX"), *camBinding->getInputs()->getChild("viewport")->getChild("offsetX"));
    logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("offsetY"), *camBinding->getInputs()->getChild("viewport")->getChild("offsetY"));
    logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("width"), *camBinding->getInputs()->getChild("viewport")->getChild("width"));
    logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("height"), *camBinding->getInputs()->getChild("viewport")->getChild("height"));
    logicEngine.link(*script2->getOutputs()->getChild("floatUniform"), *appBinding->getInputs()->getChild("floatUniform"));
    logicEngine.link(*animNode->getOutputs()->getChild("channel"), *appBinding->getInputs()->getChild("animatedFloatUniform"));

    logicEngine.update();

    logicEngine.saveToFile("testLogic.bin");
    scene->saveToFile("testScene.bin", false);

    logicEngine.destroy(*script1);
    logicEngine.destroy(*script2);
    logicEngine.destroy(*nodeBinding);
    logicEngine.destroy(*camBinding);
    logicEngine.destroy(*appBinding);
    ramsesClient->destroy(*scene);

    return 0;
}
