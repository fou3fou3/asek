#include "globals.hpp"
#include "index.hpp"
#include "search.hpp"
#include <GLFW/glfw3.h>
#include <Windows.h>
#include <format>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <shellapi.h>
#include <string.h>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int argc, char *argv[]) {

  if (argc == 1) {
    std::cout << "Select a mode (index, search)!\n";
    return 1;
  }

  std::string first_argument = argv[1];

  if (first_argument == "index") {
    index();
    return 0;
  } else if (first_argument == "search") {
    auto tfIdfIndex = extract_json_from_file(TFIDF_INDEX_PATH);
    auto tfIdfOfAllWords = tfIdfIndex[0];
    auto numberOfDocuments = tfIdfIndex[1];
    auto documentsTfIdfSumSquared = tfIdfIndex[2];

    std::string second_argument = argv[2];
    if (second_argument == "imgui") {
      glfwSetErrorCallback(glfw_error_callback);
      if (!glfwInit())
        return 1;

      const char *glsl_version = "#version 130";
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

      GLFWwindow *window =
          glfwCreateWindow(1280, 720, "Asek", nullptr, nullptr);
      if (window == nullptr)
        return 1;
      glfwMakeContextCurrent(window);
      glfwSwapInterval(1); // Enable vsync

      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO &io = ImGui::GetIO();
      (void)io;
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

      ImGui::StyleColorsDark();

      ImGui_ImplGlfw_InitForOpenGL(window, true);
      ImGui_ImplOpenGL3_Init(glsl_version);

      ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
      ImVec4 link_color = ImVec4(0.3f, 0.6f, 1.0f, 1.0f);
      ImVec4 hovered_color = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
      std::multiset<SearchResult, CompareSearchResults> searchResults;

      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
          int display_w, display_h;
          glfwGetFramebufferSize(window, &display_w, &display_h);

          ImGui::SetNextWindowPos(ImVec2(0, 0));
          ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

          ImGuiWindowFlags window_flags =
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

          ImGui::Begin("Asek", nullptr, window_flags);
          ImGui::Text("Asek search engine!");

          ImGui::NewLine();

          static std::string searchQuery = "";
          char temp_buffer[256];
          strncpy_s(temp_buffer, searchQuery.c_str(), sizeof(temp_buffer));
          temp_buffer[sizeof(temp_buffer) - 1] = '\0';

          if (ImGui::InputText(":", temp_buffer, sizeof(temp_buffer))) {
            searchQuery = temp_buffer;
          }

          ImGui::SameLine();
          if (ImGui::Button("search")) {
            searchResults.clear();
            searchResults =
                search_tfidf(tfIdfOfAllWords, searchQuery, numberOfDocuments,
                             documentsTfIdfSumSquared);
          };

          ImGui::Separator();

          ImGui::Text("Results:");

          if (!searchResults.empty()) {
            size_t i = 0;
            for (auto it = searchResults.begin();
                 it != searchResults.end() && i < 30; ++it, ++i) {

              std::string filePath = std::format(
                  "{}//{}//{}", std::filesystem::current_path().string(),
                  DATA_DIR, it->document);

              bool is_hovered = false;

              ImGui::PushID(it->document.c_str());

              ImGui::TextColored(link_color, "%s: %.5f", it->document.c_str(),
                                 it->cosineSimilarity);

              if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                ImGui::SetTooltip("%s", filePath.c_str());

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                  ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL,
                                SW_SHOWNORMAL);
                }
              }

              ImGui::PopID();
            }
          }

          ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w,
                     clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
      }

      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();

      glfwDestroyWindow(window);
      glfwTerminate();

      return 0;
    }
  }

  return 0;
}
