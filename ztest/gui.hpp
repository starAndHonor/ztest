#pragma once
#include <glad/glad.h>

#include "core/ztest_base.hpp"
#include "core/ztest_context.hpp"
#include "core/ztest_error.hpp"
#include "core/ztest_macros.hpp"
#include "core/ztest_registry.hpp"
#include "core/ztest_result.hpp"
#include "core/ztest_singlecase.hpp"
#include "core/ztest_suite.hpp"
#include "core/ztest_timer.hpp"
#include "core/ztest_types.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <map>

class ZTestModel {
public:
  std::atomic<bool> _is_running{false};
  float _progress = 0.0f;
  std::string _selected_test;
  std::mutex _mutex;
  bool tryStartRunning() {
    bool expected = false;
    return _is_running.compare_exchange_strong(expected, true);
  }
  void initializeFromRegistry(ZTestContext &context) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto &registry = ZTestRegistry::instance();
    auto tests = registry.takeTests();

    for (auto &test : tests) {
      ZTestResult empty_reslut{test->getName(), 0.0, ZState::z_unknown, ""};
      ZTestResultManager::getInstance().addResult(empty_reslut);
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
        _context.runAllTests();
      } catch (...) {
        std::cerr << "[THREAD] Exception caught during test execution."
                  << std::endl;
      }

      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = 1.0f;
      }

      std::cout << "[THREAD] All tests completed." << std::endl;
    });

    _test_thread.detach();
  }

  void runSelectedTest(const std::string &test_name) {
    if (!_model.tryStartRunning()) {
      std::cout << "[INFO] Another test is already running." << std::endl;
      return;
    }

    if (_test_thread.joinable()) {
      _test_thread.join();
    }

    _test_thread = std::thread([this, test_name] {
      bool found = false;

      try {
        found = _context.runSelectedTest(test_name);
      } catch (const std::exception &e) {
        std::cerr << "[THREAD] Exception caught: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "[THREAD] Unknown exception caught." << std::endl;
      }

      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = found ? 1.0f : 0.0f;
      }

      std::cout << "[THREAD] Thread function completed." << std::endl;
    });

    _test_thread.detach();
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
    std::map<std::string, std::vector<ZTestResult>> suiteMap;
    for (const auto &[test_name, test] :
         ZTestResultManager::getInstance().getResults()) {
      size_t dotPos = test_name.find('.');
      std::string suiteName =
          (dotPos != std::string::npos) ? test_name.substr(0, dotPos) : "Other";
      suiteMap[suiteName].push_back(test);
    }

    // Render each suite as a collapsible section
    for (const auto &[suiteName, tests] : suiteMap) {
      bool anyFailed =
          std::any_of(tests.begin(), tests.end(), [](const auto &t) {
            return t.getState() == ZState::z_failed;
          });

      ImGui::PushStyleColor(ImGuiCol_Header,
                            anyFailed ? ImVec4(0.4f, 0.0f, 0.0f, 0.3f)
                                      : ImVec4(0.0f, 0.4f, 0.0f, 0.3f));

      if (ImGui::CollapsingHeader(suiteName.c_str())) {
        ImGui::Columns(3); // Name | Status | Time
        ImGui::Text("Test Name");
        ImGui::NextColumn();
        ImGui::Text("Status");
        ImGui::NextColumn();
        ImGui::Text("Time (ms)");
        ImGui::Separator();
        ImGui::NextColumn(); // Reset column

        for (const auto &test : tests) {
          ImGui::Columns(3);

          // 测试名称列
          ImGui::Selectable(test.getName().c_str(),
                            model._selected_test == test.getName());
          if (ImGui::IsItemClicked())
            model._selected_test = test.getName();
          ImGui::NextColumn();

          // 状态列
          ImGui::TextColored(getStateColor(test.getState()), "%s",
                             test.getState() == ZState::z_unknown
                                 ? "Not Run"
                                 : toString(test.getState()));
          ImGui::NextColumn();

          // 时间列
          ImGui::Text("%.2f", test.getUsedTime());
          ImGui::NextColumn();
        }

        ImGui::Columns(1);
      }
      ImGui::PopStyleColor();
    }

    ImGui::End();
  }
  void renderTestCaseRow(const ZTestResult &test, ZTestModel &model) {
    // 添加缩进
    ImGui::Indent(10.0f);

    // 测试名称列
    ImGui::Selectable(test.getName().c_str(),
                      model._selected_test == test.getName());
    if (ImGui::IsItemClicked())
      model._selected_test = test.getName();
    ImGui::Unindent(10.0f);
    ImGui::NextColumn();

    // 状态列
    ImGui::TextColored(getStateColor(test.getState()), "%s",
                       test.getState() == ZState::z_unknown
                           ? "Not Run"
                           : toString(test.getState()));
    ImGui::NextColumn();

    // 时间列
    ImGui::Text("%.2f", test.getUsedTime());
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
      const auto &test_cases = ZTestResultManager::getInstance().getResults();
      for (auto &[_, test] : test_cases) {
        if (test.getState() == ZState::z_success)
          passed++;
        else if (test.getState() == ZState::z_failed)
          failed++;
      }
      ImGui::Text("Total: %d  Passed: %d  Failed: %d", test_cases.size(),
                  passed, failed);
    }

    ImGui::End();
  }

  void renderDetailsWindow(ZTestModel &model) {
    ImGui::Begin("Test Details");

    if (!model._selected_test.empty()) {
      auto it =
          ZTestResultManager::getInstance().getResult(model._selected_test);

      ImGui::Text("Test Name: %s", it.getName().c_str());
      ImGui::TextColored(getStateColor(it.getState()), "Status: %s",
                         toString(it.getState()));
      ImGui::Text("Duration: %.2f ms", it.getUsedTime());

      if (!it.getErrorMsg().empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error Message:");
        ImGui::TextWrapped("%s", it.getErrorMsg().c_str());
      }
    }

    ImGui::End();
  }

  GLFWwindow *_window = nullptr;
};
static ZTestModel model;
static ZTestContext testContext;
static ZTestController controller(model, testContext);
static ZTestView view;

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

inline int showUI() {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100 (WebGL 1.0)
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
  const char *glsl_version = "#version 300 es";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "ztest gui", nullptr, nullptr);
  if (window == nullptr)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  // In main.cpp after creating window:
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImFont *font = io.Fonts->AddFontFromFileTTF(
      "Hack-Regular.ttf", 25.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
  // Initialize ImGui backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version); // Use the actual glsl_version variable
  // 初始化测试上下文
  // InitializeTestContext(testContext); // 添加测试用例
  // 初始化MVC组件
  model.initializeFromRegistry(testContext);

  // 主循环
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // 开始ImGui帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 渲染GUI
    view.render(model, controller);

    // 渲染绘制
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // 清理资源
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}