// gui.h
#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "test.hpp"
#include <GLFW/glfw3.h>

class ZTestModel {
public:
  struct ZTestCaseInfo {
    std::string _name;
    ZState _state;
    double _duration;
    std::string _error;
  };

  std::vector<ZTestCaseInfo> _test_cases;
  std::atomic<bool> _is_running{false};
  float _progress = 0.0f;
  std::string _selected_test;
  std::mutex _mutex;

  void updateFromContext(ZTestContext &context) {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto &result : context.getResults()) {
      auto it = std::find_if(
          _test_cases.begin(), _test_cases.end(),
          [&](const auto &t) { return t._name == result.getName(); });

      if (it != _test_cases.end()) {
        it->_state = result.getState();
        it->_duration = result.getUsedTime();
        it->_error = result.getErrorMsg();
      }
    }
  }

  bool tryStartRunning() {
    bool expected = false;
    return _is_running.compare_exchange_strong(expected, true);
  }
  void initializeFromRegistry(ZTestContext &context) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto &registry = ZTestRegistry::instance();
    auto tests = registry.takeTests();

    _test_cases.clear();
    for (auto &test : tests) {
      ZTestCaseInfo info{test->getName(), ZState::z_unknown, 0.0, ""};
      _test_cases.push_back(info);
    }
    for (auto &&test : tests) {
      context.addTest(std::move(test));
    }
  }
};

class ZTestController {
public:
  ZTestController(ZTestModel &model, ZTestContext &context)
      : _model(model), _context(context) {}

  void runAllTests() {
    // Use atomic check instead of raw boolean
    if (!_model.tryStartRunning())
      return;
    _test_thread = std::thread([this] {
      try {
        _context.runAll();
      } catch (...) {
        // Ensure state cleanup
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        throw;
      }
      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = 1.0f; // Ensure progress completion
      }
    });
    _test_thread.join();
  }

  void runSelectedTest(const std::string &test_name) {
    if (!_model.tryStartRunning())
      return;

    _test_thread = std::thread([this, test_name] {
      try {
        auto tests = _context.getTests();
        // Add progress tracking
        float total = tests.size();
        float completed = 0;
        std::cout << total << std::endl;

        for (auto &test : tests) {
          if (test && test->getName() == test_name) {
            _context.runTest(std::move(test));
            {
              std::lock_guard<std::mutex> lock(_model._mutex);
              _model._progress = ++completed / total;
            }
            break;
          }
        }
      } catch (...) {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        throw;
      }
      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = 1.0f;
      }
    });
    _test_thread.join();
  }

private:
  ZTestModel &_model;
  ZTestContext &_context;
  std::thread _test_thread;
};

class ZTestView {
public:
  void render(ZTestModel &model, ZTestController &controller) {
    renderMainMenu();
    renderTestList(model, controller);
    renderStatusBar(model);
    renderDetailsWindow(model);
  }

  void setWindow(GLFWwindow *window) { _window = window; }

private:
  void renderMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit"))
          glfwSetWindowShouldClose(_window, true);
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
  }

  void renderTestList(ZTestModel &model, ZTestController &controller) {
    ImGui::Begin("Test Cases", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::Button("Run All")) {
      controller.runAllTests();
    }

    ImGui::SameLine();
    if (ImGui::Button("Run Selected")) {
      if (!model._selected_test.empty()) {

        controller.runSelectedTest(model._selected_test);
      }
    }

    ImGui::Separator();

    ImGui::Columns(3, "testColumns");
    ImGui::Text("Test Name");
    ImGui::NextColumn();
    ImGui::Text("Status");
    ImGui::NextColumn();
    ImGui::Text("Duration (ms)");
    ImGui::NextColumn();
    ImGui::Separator();

    std::lock_guard<std::mutex> lock(model._mutex);
    for (auto &test : model._test_cases) {
      renderTestCaseRow(test, model);
    }

    ImGui::Columns(1);
    ImGui::End();
  }

  void renderTestCaseRow(const ZTestModel::ZTestCaseInfo &test,
                         ZTestModel &model) {
    ImGui::Selectable(test._name.c_str(), model._selected_test == test._name);
    if (ImGui::IsItemClicked()) {
      model._selected_test = test._name;
    }
    ImGui::NextColumn();

    ImGui::TextColored(
        getStateColor(test._state), "%s",
        test._state == ZState::z_unknown ? "Not Run" : toString(test._state));
    ImGui::NextColumn();

    ImGui::Text("%.2f", test._duration);
    ImGui::NextColumn();
  }

  ImVec4 getStateColor(ZState state) {
    switch (state) {
    case ZState::z_success:
      return ImVec4(0, 1, 0, 1);
    case ZState::z_failed:
      return ImVec4(1, 0, 0, 1);
    default:
      return ImVec4(1, 1, 1, 1);
    }
  }

  void renderStatusBar(ZTestModel &model) {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 30));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));

    ImGui::Begin("StatusBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoNav);

    if (model._is_running) {
      ImGui::ProgressBar(model._progress, ImVec2(-1, 15), "Running...");
    } else {
      int passed = 0, failed = 0;
      for (auto &test : model._test_cases) {
        if (test._state == ZState::z_success)
          passed++;
        else if (test._state == ZState::z_failed)
          failed++;
      }
      ImGui::Text("Total: %d  Passed: %d  Failed: %d", model._test_cases.size(),
                  passed, failed);
    }

    ImGui::End();
  }

  void renderDetailsWindow(ZTestModel &model) {
    ImGui::Begin("Test Details");

    if (!model._selected_test.empty()) {
      auto it = std::find_if(
          model._test_cases.begin(), model._test_cases.end(),
          [&](const auto &t) { return t._name == model._selected_test; });

      if (it != model._test_cases.end()) {
        ImGui::Text("Test Name: %s", it->_name.c_str());
        ImGui::TextColored(getStateColor(it->_state), "Status: %s",
                           toString(it->_state));
        ImGui::Text("Duration: %.2f ms", it->_duration);

        if (!it->_error.empty()) {
          ImGui::Separator();
          ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error Message:");
          ImGui::TextWrapped("%s", it->_error.c_str());
        }
      }
    }

    ImGui::End();
  }

  GLFWwindow *_window = nullptr;
};