#include "./include/imgui/imgui.h"
#include "./include/imgui/imgui_impl_sdl2.h"
#include "./include/imgui/imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <stdio.h>
#include "./punktop.h"
#include <iostream>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

int main(){
    std::cout << "[INFO] Running...\n";
    std::thread cpu_worker(GetCpuUsage);
    cpu_worker.detach();
    std::thread net_worker(GetNetworkUsage, "wlp2s0");
    net_worker.detach();
    // std::thread show_thread(ShowCpuUsage);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0){
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_WindowFlags Window_Flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* Window = SDL_CreateWindow("Wtop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, Window_Flags);

    if(Window == nullptr){
        printf("Error Creating Widnow: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if(Renderer == nullptr){
        SDL_Log("Error creating SDL_Renderer!");
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(Window, Renderer);
    ImGui_ImplSDLRenderer2_Init(Renderer);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool dock_window = true;
    bool done = false;
    Uint32 last_update_time = 0; // Time in millis
    
    while(!done){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type == SDL_QUIT){
                is_finished = true;
                done = true;
            }
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(Window)){
                is_finished = true;
                done = true;
            }
        }
        if(SDL_GetWindowFlags(Window) & SDL_WINDOW_MINIMIZED){
            SDL_Delay(10);
            continue;
        }

        // Update process list every 2 seconds before imgui frame starts
        Uint32 current_time = SDL_GetTicks(); // Get time in milliseconds
        if (current_time - last_update_time >= 2000) {
            FetchProcesses();
            // GetCpuUsage();
            last_update_time = current_time;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        if(dock_window)
            ShowDockSpace(dock_window);

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(Renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(Renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(Renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), Renderer);
        SDL_RenderPresent(Renderer);
    }

    // join threads (kinda slows down program exit)
    // is_finished = true;
    // cpu_worker.join();
    // show_thread.join();

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    SDL_Quit();
    
    return 0;
}
