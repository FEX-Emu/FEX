// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/string.h>

#include <chrono>
#include <imgui.h>
#define YES_IMGUIFILESYSTEM
#include <addons/imgui_user.h>

#include <SDL.h>
#include <SDL_scancode.h>

#include <epoxy/gl.h>

#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <tuple>

namespace FEX::GUI {

  using TupleReturn = std::tuple<SDL_Window*, SDL_GLContext>;
  static TupleReturn SetupIMGui(const char *Name, const fextl::string &Config) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return TupleReturn{};
    }

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(Name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 640, window_flags);
    SDL_GLContext gl_context{};
    const char* glsl_version{};

    // Try a GL 3.0 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    gl_context = SDL_GL_CreateContext(window);
    glsl_version = "#version 130";

    if (!gl_context) {
      // 3.0 failed, let's try 2.1
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
      gl_context = SDL_GL_CreateContext(window);
      glsl_version = "#version 120";

      if (!gl_context) {
        // 2.1 failed, let's try ES 2.0
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        gl_context = SDL_GL_CreateContext(window);
        glsl_version = "#version 100";

        if (!gl_context) {
          printf("Couldn't create GL context: %s\n", SDL_GetError());
          return {};
        }
      }
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    io.IniFilename = &Config.at(0);

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    return std::make_tuple(window, gl_context);
  }

  static std::chrono::time_point<std::chrono::high_resolution_clock> LastUpdate{};
  constexpr auto UpdateTimeout = std::chrono::seconds(2);
  void DrawUI(SDL_Window *window, std::function<bool()> DrawFunction) {
    bool Running {true};
    ImGuiIO& io = ImGui::GetIO();
    while (Running) {
      SDL_Event event;
      auto Now = std::chrono::high_resolution_clock::now();
      auto Dur = Now - LastUpdate;

      if (Dur < UpdateTimeout || SDL_WaitEvent(nullptr))
      {
        while (SDL_PollEvent(&event)) {
          ImGui_ImplSDL2_ProcessEvent(&event);
          if (event.type == SDL_QUIT)
            Running = false;
          if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            Running = false;
        }
      }

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame(window);
      Running &= DrawFunction();

      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
      }

      SDL_GL_SwapWindow(window);
    }
  }

  void Shutdown(SDL_Window *window, SDL_GLContext gl_context) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }

  void HadUpdate() {
    LastUpdate = std::chrono::high_resolution_clock::now();

    // Update the window
    SDL_Event Event{};
    Event.type = SDL_USEREVENT;
    SDL_PushEvent(&Event);

  }
}
