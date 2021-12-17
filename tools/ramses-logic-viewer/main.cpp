//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "Arguments.h"
#include "SceneSetup.h"
#include "ImguiClientHelper.h"
#include "LogicViewerGui.h"
#include "LogicViewer.h"

#include "ramses-client.h"
#include "ramses-utils.h"

#include "ramses-renderer-api/RamsesRenderer.h"
#include "ramses-renderer-api/DisplayConfig.h"
#include "ramses-renderer-api/RendererSceneControl.h"
#include "ramses-renderer-api/IRendererEventHandler.h"
#include "ramses-renderer-api/IRendererSceneControlEventHandler.h"

#include <iostream>
#include <thread>
#include "internals/StdFilesystemWrapper.h"


bool getPreferredSize(const ramses::Scene& scene, uint32_t& width, uint32_t& height)
{
    ramses::SceneObjectIterator it(scene, ramses::ERamsesObjectType_RenderPass);
    ramses::RamsesObject*       obj = nullptr;
    while (nullptr != (obj = it.getNext()))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto* rp = static_cast<ramses::RenderPass*>(obj);
        if (!rp->getRenderTarget())
        {
            auto* camera = rp->getCamera();
            if (camera)
            {
                width = camera->getViewportWidth();
                height = camera->getViewportHeight();
                return true;
            }
        }
    }
    return false;
}

int main(int argc, char* argv[])
{
    Arguments args(argc, argv);
    if (!args.valid())
    {
        args.printErrorMessage(std::cerr);
        Arguments::printUsage(std::cerr);
        return -1;
    }

    ramses::RamsesFrameworkConfig frameworkConfig(argc, argv);
    frameworkConfig.setPeriodicLogsEnabled(false);
    ramses::RamsesFramework framework(frameworkConfig);
    ramses::RamsesClient* client = framework.createClient("ramses-logic-viewer");
    if (!client)
    {
        std::cerr << "Could not create ramses client" << std::endl;
        return 1;
    }

    ramses::RendererConfig  rendererConfig(argc, argv);
    ramses::RamsesRenderer* renderer = framework.createRenderer(rendererConfig);
    if (!renderer)
    {
        std::cerr << "Could not create ramses renderer" << std::endl;
        return 2;
    }

    ramses::RendererSceneControl* sceneControl = renderer->getSceneControlAPI();
    if (!sceneControl)
    {
        std::cerr << "Could not create scene Control" << std::endl;
        return 3;
    }
    renderer->startThread();
    framework.connect();

    ramses::Scene* scene = client->loadSceneFromFile(args.sceneFile.c_str());
    if (!scene)
    {
        std::cerr << "Failed to load scene: " << args.sceneFile << std::endl;
        return 4;
    }
    scene->publish();
    scene->flush();

    const auto guiSceneId = ramses::sceneId_t(scene->getSceneId().getValue() + 1);

    ramses::DisplayConfig displayConfig(argc, argv);
    displayConfig.setResizable(true);
    uint32_t width = 0;
    uint32_t height = 0;
    if (args.autoDetectViewportSize && getPreferredSize(*scene, width, height))
    {
        displayConfig.setWindowRectangle(0, 0, width, height);
    }

    if (args.multiSampleRate > 0)
    {
        displayConfig.setMultiSampling(args.multiSampleRate);
    }

    int32_t    winX = 0;
    int32_t    winY = 0;
    displayConfig.getWindowRectangle(winX, winY, width, height);
    ramses::ImguiClientHelper imguiHelper(*client, width, height, guiSceneId);
    imguiHelper.setRenderer(renderer);

    const ramses::displayId_t display = renderer->createDisplay(displayConfig);
    imguiHelper.setDisplayId(display);
    renderer->flush();

    std::unique_ptr<ISceneSetup> sceneSetup;
    if (args.noOffscreen)
    {
        sceneSetup = std::make_unique<FramebufferSetup>(imguiHelper, renderer, scene, display);
    }
    else
    {
        sceneSetup = std::make_unique<OffscreenSetup>(imguiHelper, renderer, scene, display, width, height);
    }

    auto takeScreenshot = [&](const std::string& filename) {
        static ramses::sceneVersionTag_t ver = 42;
        ++ver;
        scene->flush(ver);
        if (imguiHelper.waitForSceneVersion(scene->getSceneId(), ver))
        {
            if (imguiHelper.saveScreenshot(filename, sceneSetup->getOffscreenBuffer(), 0, 0, sceneSetup->getWidth(), sceneSetup->getHeight()))
            {
                if (imguiHelper.waitForScreenshot())
                {
                    return true;
                }
            }
        }
        return false;
    };

    rlogic::LogicViewer viewer(takeScreenshot);

    if (!viewer.loadRamsesLogic(args.logicFile, scene))
    {
        std::cerr << "Failed to load logic file: " << args.logicFile << std::endl;
        return 5;
    }

    rlogic::LogicViewerGui gui(viewer);
    gui.setSceneTexture(sceneSetup->getTextureSampler(), width, height);

    sceneSetup->apply();

    auto loadStatus = args.luaFile.empty() ? rlogic::LogicViewer::Result() : viewer.loadLuaFile(args.luaFile);

    if (!args.luaFunction.empty())
    {
        if (loadStatus.ok())
        {
            loadStatus = viewer.call(args.luaFunction);
        }
        if (!loadStatus.ok())
        {
            std::cerr << loadStatus.getMessage() << std::endl;
            return 6;
        }
    }
    else
    {
        while (imguiHelper.isRunning())
        {
            const auto updateStatus = viewer.update();
            scene->flush();
            imguiHelper.dispatchEvents();
            ImGui::NewFrame();
            gui.draw();
            if (!loadStatus.ok())
            {
                gui.openErrorPopup(loadStatus.getMessage());
                loadStatus = rlogic::LogicViewer::Result();
            }
            if (!updateStatus.ok())
                gui.openErrorPopup(updateStatus.getMessage());
            ImGui::EndFrame();
            imguiHelper.draw();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    return 0;
}
