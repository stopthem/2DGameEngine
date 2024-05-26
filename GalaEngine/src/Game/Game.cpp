#include "Game.h"
#include <SDL.h>
#include <imgui/imgui.h>
#include <imgui/imgui_sdl.h>
#include <imgui/imgui_impl_sdl2.h>
#include "../Components/SpriteComponent.h"
#include "../Components/TextComponent.h"
#include "../Logger/Logger.h"
#include "../ECS/ECS.h"
#include "../Systems/MovementSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/CollisionSystem.h"
#include "../Systems/DamageSystem.h"
#include "../Systems/KeyboardControlSystem.h"
#include "../Systems/CameraMovementSystem.h"
#include "../Systems/RenderColliderSystem.h"
#include "../Systems/ProjectileEmitterSystem.h"
#include "../Systems/LifetimeSystem.h"
#include "../Systems/ShootingSystem.h"
#include "../Systems/RenderTextSystem.h"
#include "../Systems/RenderHealthBarsSystem.h"
#include "../Systems/RenderGUISystem.h"
#include "../Systems/ScriptSystem.h"
#include "../AssetStore/AssetStore.h"
#include "../EventBus/EventBus.h"
#include "../Events/KeyUpEvent.h"
#include "../Level/LevelSerializer.h"
#include "../Core/Editor/GalaEditor.h"
#include "../Systems/NameSystem.h"
#include "../Systems/RenderLevelHierarchyGUISystem.h"
#include "../Level/LevelLoader.h"

namespace gala
{
    int Game::WindowWidth;
    int Game::WindowHeight;

    double Game::MapWidth;
    double Game::MapHeight;

    Game::Game()
        : Registry(std::make_unique<class Registry>()), AssetStore(std::make_unique<class AssetStore>()), EventBus(std::make_unique<class EventBus>())
    {
    }

    Game::~Game()
    = default;

    void Game::Initialize()
    {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            Logger::Err("Error initializing SDL.");
            return;
        }

        if (TTF_Init() != 0)
        {
            Logger::Err("Error initializing SDL TTF");
            return;
        }

        SDL_DisplayMode displayMode;
        SDL_GetCurrentDisplayMode(0, &displayMode);
        // SDL_SetWindowFullscreen(Window, SDL_WINDOW_FULLSCREEN_DESKTOP);

        WindowWidth = CameraRect.w = 1600;
        WindowHeight = CameraRect.h = 1080;
        // WindowWidth = CameraRect.w = displayMode.w;
        // WindowHeight = CameraRect.h = displayMode.h;

        Window = SDL_CreateWindow("GalaEngine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth, WindowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);

        if (!Window)
        {
            Logger::Err("Error creating SDL window.");
            return;
        }

        Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);

        if (!Renderer)
        {
            Logger::Err("Error creating SDL renderer.");
            return;
        }

        ImGui::CreateContext();

        // Add our custom font to imgui
        const ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("./assets/fonts/MonacoB2.otf", 16);

        // Initialize imgui
        ImGuiSDL::Initialize(Renderer, WindowWidth, WindowHeight);
        ImGui_ImplSDL2_InitForSDLRenderer(Window, Renderer);

        // Enable docking on imgui
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        AssetStore->Initialize(Renderer);

        IsRunning = true;
    }

    void Game::Setup()
    {
        GalaEditor = std::make_unique<class GalaEditor>();

        Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os);

        LevelSerializer = std::make_unique<class LevelSerializer>(Registry.get());

        AddSystems();

        LevelLoader = std::make_unique<class LevelLoader>(LevelSerializer.get());
        LevelLoader->LoadLevel(1);
    }

    void Game::AddSystems() const
    {
        Registry->AddSystem<MovementSystem>(EventBus.get());
        Registry->AddSystem<RenderSystem>();
        Registry->AddSystem<RenderColliderSystem>();
        Registry->AddSystem<AnimationSystem>();
        Registry->AddSystem<CollisionSystem>();
        Registry->AddSystem<DamageSystem>(EventBus.get());
        Registry->AddSystem<KeyboardControlSystem>(EventBus.get());
        Registry->AddSystem<CameraMovementSystem>();
        Registry->AddSystem<LifetimeSystem>(Registry.get());
        Registry->AddSystem<ProjectileEmitterSystem>(Registry.get());
        Registry->AddSystem<ShootingSystem>(Registry.get(), EventBus.get());
        Registry->AddSystem<RenderTextSystem>(Renderer, AssetStore.get(), CameraRect);
        Registry->AddSystem<NameSystem>();
        Registry->AddSystem<RenderLevelHierarchyGUISystem>(GalaEditor.get(), EventBus.get());
        Registry->AddSystem<RenderHealthBarsSystem>();
        Registry->AddSystem<RenderGUISystem>();
        Registry->AddSystem<ScriptSystem>(sol::state_view(Lua));
    }

    void Game::Run()
    {
        Setup();

        while (IsRunning)
        {
            ProcessInput();
            Update();
            Render();
        }
    }

    void Game::ProcessInput()
    {
        SDL_Event sdlEvent;

        const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);

        ImGuiIO& imGuiIo = ImGui::GetIO();

        while (SDL_PollEvent(&sdlEvent))
        {
            // We give input to imgui if it wants it
            if (imGuiIo.WantCaptureMouse || imGuiIo.WantCaptureKeyboard)
            {
                int mouseX, mouseY;
                const Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);

                imGuiIo.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
                imGuiIo.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
                imGuiIo.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);

                ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
                return;
            }

            switch (sdlEvent.type)
            {
            case SDL_QUIT:
                IsRunning = false;
                break;

            case SDL_KEYUP:
                const SDL_Keycode keycode = sdlEvent.key.keysym.sym;

                switch (keycode)
                {
                case SDLK_ESCAPE:
                    IsRunning = false;
                    break;

                case SDLK_d:
                    IsDebug = !IsDebug;
                    break;

                case SDLK_s:
                    if (keyboardState[SDL_SCANCODE_LCTRL])
                    {
                        LevelLoader->SaveLevel();
                    }

                default:
                    break;
                }

                EventBus->BroadcastEvent<KeyUpEvent>(keycode);
                break;
            }
        }
    }

    void Game::Update()
    {
        const int currentFrameTicks = static_cast<int>(SDL_GetTicks());

        const int timeToWait = MILISECS_PER_FRAME - (currentFrameTicks - MilisecsPrevFrame);
        if (timeToWait > 0 && timeToWait <= MILISECS_PER_FRAME)
        {
            SDL_Delay(timeToWait);
        }

        DeltaTime = (currentFrameTicks - MilisecsPrevFrame) / 1000.0;

        // Store current frame as a previous frame for next frame.
        MilisecsPrevFrame = currentFrameTicks;

        // Ask all the systems to update.
        Registry->GetSystem<ScriptSystem>().Update(DeltaTime);
        Registry->GetSystem<ProjectileEmitterSystem>().Update();
        Registry->GetSystem<LifetimeSystem>().Update(DeltaTime);
        Registry->GetSystem<MovementSystem>().Update(DeltaTime, Registry);
        Registry->GetSystem<CollisionSystem>().Update(EventBus);
        Registry->GetSystem<AnimationSystem>().Update();
        Registry->GetSystem<CameraMovementSystem>().Update(CameraRect);

        // Update the registry to process the entities that are waiting to be created/deleted.
        Registry->Update();
    }

    void Game::Render() const
    {
        SDL_SetRenderDrawColor(Renderer, 21, 21, 21, 255);
        SDL_RenderClear(Renderer);

        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        // Ask all render systems that needs an update.
        Registry->GetSystem<RenderSystem>().Update(Renderer, CameraRect, AssetStore);
        Registry->GetSystem<RenderHealthBarsSystem>().Update(Registry, Renderer, CameraRect);
        Registry->GetSystem<RenderTextSystem>().Update();
        Registry->GetSystem<RenderLevelHierarchyGUISystem>().Update(Registry);

        if (IsDebug)
        {
            Registry->GetSystem<RenderColliderSystem>().Update(Renderer, CameraRect);
            Registry->GetSystem<RenderGUISystem>().Update(Registry);
        }

        ImGui::Render();
        ImGuiSDL::Render(ImGui::GetDrawData());

        SDL_RenderPresent(Renderer);
    }

    void Game::Destroy() const
    {
        ImGuiSDL::Deinitialize();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(Renderer);
        SDL_DestroyWindow(Window);
        SDL_Quit();
    }
}
