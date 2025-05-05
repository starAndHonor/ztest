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
    for (auto &[name, result] : context.getAllResults()) {
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
  ~ZTestController() {
    if (_test_thread.joinable()) {
      _test_thread.join();
    }
  }
  void runAllTests() {
    if (!_model.tryStartRunning()) {
      std::cout << "[INFO] Another test is already running." << std::endl;
      return;
    }

    if (_test_thread.joinable()) {
      _test_thread.join();
    }

    _test_thread = std::thread([this] {
      try {
        // 1️⃣ 单线程运行 z_unsafe 测试
        {
          std::lock_guard<std::mutex> lock(_model._mutex);
          std::cout << "[THREAD] Running unsafe tests..." << std::endl;
          _context.runUnsafeOnly(); // 新增方法，单线程执行
        }

        // 2️⃣ 多线程运行 z_safe 测试
        {
          std::lock_guard<std::mutex> lock(_model._mutex);
          std::cout << "[THREAD] Running safe tests in parallel..."
                    << std::endl;
          _context.resetQueue();        // 重置队列
          _context.runSafeInParallel(); // 使用线程池
        }

      } catch (...) {
        std::cerr << "[THREAD] Exception caught during test execution."
                  << std::endl;
        throw;
      }

      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = 1.0f;
      }

      std::cout << "[THREAD] All tests completed." << std::endl;
    });

    _test_thread.detach(); // 或 join()
  }

  void runSelectedTest(const std::string &test_name) {
    if (!_model.tryStartRunning()) {
      std::cout << "[INFO] Another test is already running." << std::endl;
      return;
    }

    if (_test_thread.joinable()) {
      std::cout << "[INFO] Joining previous thread..." << std::endl;
      _test_thread.join();
    }

    std::cout << "[INFO] Starting new thread for test: " << test_name
              << std::endl;
    _test_thread = std::thread([this, test_name] {
      bool found = false;
      try {
        auto tests = _context.getTestList();
        std::cout << "[THREAD] Total tests: " << tests.size() << std::endl;

        for (auto &weak_test : tests) {
          if (auto test = weak_test.lock()) {
            std::cout << "[THREAD] Checking test: " << test->getName()
                      << std::endl;
            if (test->getName() == test_name) {
              std::cout << "[THREAD] Running test: " << test_name << std::endl;
              _context.runTest(test);

              {
                std::lock_guard<std::mutex> lock(_model._mutex);
                _model._progress = 1.0f;
              }

              found = true;
              break;
            }
          }
        }

        if (!found) {
          std::cout << "[THREAD] Test not found: " << test_name << std::endl;
        }

      } catch (const std::exception &e) {
        std::cerr << "[THREAD] Exception caught: " << e.what() << std::endl;
        {
          std::lock_guard<std::mutex> lock(_model._mutex);
          _model._is_running = false;
          _model._progress = 0.0f;
        }
        throw;
      } catch (...) {
        std::cerr << "[THREAD] Unknown exception caught." << std::endl;
        {
          std::lock_guard<std::mutex> lock(_model._mutex);
          _model._is_running = false;
          _model._progress = 0.0f;
        }
        throw;
      }

      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = 0.0f;
      }

      std::cout << "[THREAD] Thread function completed." << std::endl;
    });
  }

private:
  ZTestModel &_model;
  ZTestContext &_context;
  std::thread _test_thread;
  bool isRunning() const { return _model._is_running.load(); }
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

    // Control buttons
    if (ImGui::Button("Run All"))
      controller.runAllTests();
    ImGui::SameLine();
    if (ImGui::Button("Run Selected") && !model._selected_test.empty()) {
      controller.runSelectedTest(model._selected_test);
    }
    ImGui::Separator();

    std::lock_guard<std::mutex> lock(model._mutex);

    // Group tests by suite
    std::map<std::string, std::vector<ZTestModel::ZTestCaseInfo>> suiteMap;
    for (const auto &test : model._test_cases) {
      size_t dotPos = test._name.find('.');
      std::string suiteName = (dotPos != std::string::npos)
                                  ? test._name.substr(0, dotPos)
                                  : "Other";
      suiteMap[suiteName].push_back(test);
    }

    for (const auto &[suiteName, tests] : suiteMap) {
      bool anyFailed =
          std::any_of(tests.begin(), tests.end(), [](const auto &t) {
            return t._state == ZState::z_failed;
          });

      ImGui::PushStyleColor(ImGuiCol_Header,
                            anyFailed ? ImVec4(0.4f, 0.0f, 0.0f, 0.3f)
                                      : ImVec4(0.0f, 0.4f, 0.0f, 0.3f));

      if (ImGui::CollapsingHeader(suiteName.c_str())) {
        for (const auto &test : tests) {
          ImGui::Columns(3);
          renderTestCaseRow(test, model);
        }
        ImGui::Columns(1);
      }
      ImGui::PopStyleColor();
    }

    ImGui::End();
  }
  void renderTestCaseRow(const ZTestModel::ZTestCaseInfo &test,
                         ZTestModel &model) {
    // 添加缩进
    ImGui::Indent(10.0f);

    // 测试名称列
    ImGui::Selectable(test._name.c_str(), model._selected_test == test._name);
    if (ImGui::IsItemClicked())
      model._selected_test = test._name;
    ImGui::Unindent(10.0f);
    ImGui::NextColumn();

    // 状态列
    ImGui::TextColored(
        getStateColor(test._state), "%s",
        test._state == ZState::z_unknown ? "Not Run" : toString(test._state));
    ImGui::NextColumn();

    // 时间列
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
        ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 40));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 40));

    ImGui::Begin("StatusBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoNav);

    if (model._is_running) {
      ImGui::ProgressBar(model._progress, ImVec2(-1, 15),
                         model._progress < 1.0f ? "Running..." : "Done...");
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